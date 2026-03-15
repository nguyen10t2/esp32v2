#include "ds18b20.h"

#include <math.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"

// Các lệnh 1-Wire
#define DS18B20_CMD_SKIP_ROM 0xCC
#define DS18B20_CMD_CONVERT_T 0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE

// Timing được điều chỉnh một chút cho pull-up yếu (ví dụ: 5.7k thay vì 4.7k tiêu chuẩn)
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Thiết lập mức thấp cho chân GPIO
static inline void onewire_drive_low(void) {
    gpio_set_level(DS18B20_PIN, 0);
}

// Nhả chân GPIO mức cao (dựa vào điện trở kéo lên 5.7k bên ngoài)
static inline void onewire_release(void) {
    gpio_set_level(DS18B20_PIN, 1);
}

// Quá trình reset
static bool onewire_reset(void) {
    bool presence;
    portENTER_CRITICAL(&mux);
    onewire_drive_low();
    ets_delay_us(480);  // TX Reset

    // RX Presence
    onewire_release();
    // Điện trở 5.7k hồi phục chậm, đợi gần mức giới hạn trên của 15-60us trước khi đọc
    ets_delay_us(65);
    presence = !gpio_get_level(DS18B20_PIN);
    ets_delay_us(420);  // Hoàn thành slot
    portEXIT_CRITICAL(&mux);

    return presence;
}

// Viết một bit duy nhất
static void onewire_write_bit(uint8_t bit) {
    portENTER_CRITICAL(&mux);
    onewire_drive_low();
    if (bit) {
        ets_delay_us(5);
        onewire_release();
        ets_delay_us(60);
    } else {
        ets_delay_us(60);
        onewire_release();
        ets_delay_us(5);
    }
    // Thời gian phục hồi
    onewire_release();
    ets_delay_us(2);
    portEXIT_CRITICAL(&mux);
}

// Đọc một bit duy nhất
static uint8_t onewire_read_bit(void) {
    uint8_t bit;
    portENTER_CRITICAL(&mux);
    onewire_drive_low();
    ets_delay_us(3);
    onewire_release();
    // Điện trở kéo lên chậm cần chút thời gian ổn định thêm trước khi lấy mẫu
    ets_delay_us(12);
    bit = gpio_get_level(DS18B20_PIN);
    ets_delay_us(50);
    portEXIT_CRITICAL(&mux);
    return bit;
}

// Viết byte bắt đầu từ LSB
static void onewire_write_byte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        onewire_write_bit(data & 0x01);
        data >>= 1;
    }
}

// Đọc byte bắt đầu từ LSB
static uint8_t onewire_read_byte(void) {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        if (onewire_read_bit()) {
            data |= (1 << i);
        }
    }
    return data;
}

// Tính CRC 8-bit chuẩn Maxim
static uint8_t ds18b20_crc8(const uint8_t *addr, uint8_t len) {
    uint8_t crc = 0;
    while (len--) {
        uint8_t inbyte = *addr++;
        for (uint8_t i = 8; i; i--) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix)
                crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

void ds18b20_init(void) {
    // Cấu hình open-drain hoạt động hoàn hảo với điện trở kéo ngoài 5.7k
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pin_bit_mask = (1ULL << DS18B20_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        // Vô hiệu hoá điện trở kéo bên trong (internal pull-up) vì ta đã có trở 5.7k bên ngoài
        .pull_up_en = GPIO_PULLUP_DISABLE};
    gpio_config(&io_conf);
    onewire_release();
}

float ds18b20_get_temperature(void) {
    if (!onewire_reset())
        return NAN;

    onewire_write_byte(DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(DS18B20_CMD_CONVERT_T);

    // Khối đồng bộ (Sync block): Chờ chuẩn 750ms để đọc tối đa 12-bit chuyển đổi
    vTaskDelay(pdMS_TO_TICKS(750));

    if (!onewire_reset())
        return NAN;

    onewire_write_byte(DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(DS18B20_CMD_READ_SCRATCHPAD);

    uint8_t scratchpad[9];
    for (int i = 0; i < 9; i++) {
        scratchpad[i] = onewire_read_byte();
    }

    if (ds18b20_crc8(scratchpad, 8) != scratchpad[8]) {
        return NAN;  // Lỗi kiểm tra CRC
    }

    int16_t raw_temp = (scratchpad[1] << 8) | scratchpad[0];
    return (float)raw_temp / 16.0f;
}
