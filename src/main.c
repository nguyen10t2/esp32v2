#include <math.h>
#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lcd/tft.h"
#include "mqtt/f5_mqtt.h"
#include "nvs_flash.h"
#include "sensor/ds18b20.h"
#include "sensor/flame.h"
#include "sensor/mq.h"
#include "time_manager/esp_time.h"
#include "wifi_manager/wifi_manager.h"

static const char *TAG = "FIRE_NODE";

// Cấu trúc gói dữ liệu đọc từ sensor
typedef struct {
    uint8_t flame;
    uint16_t mq;
    float temp;
} SensorData_t;

// Hàng đợi truyền dữ liệu giữa các Task
QueueHandle_t sensorQueue;
QueueHandle_t lcdQueue;  // ĐỔI TÊN: ledQueue -> lcdQueue

// Interface cập nhật thông điệp cho LCD Task (Gọi từ mạng)
void update_lcd_direction(uint8_t dir) {
    if (lcdQueue != NULL) {
        direction_t d = (direction_t)dir;
        // Đẩy vào queue để task LCD nhận (không đợi nếu queue đầy)
        xQueueSend(lcdQueue, &d, 0);
    }
}

// 1. Task Đọc Cảm Biến (Chạy độc lập)
void task_read_sensors(void *pvParameters) {
    SensorData_t data;
    while (1) {
        data.flame = get_raw_flame_digital_value();
        data.mq = get_raw_mq_analog_value();
        data.temp = ds18b20_get_temperature();

        ESP_LOGI(TAG, "Flame DO: %d | MQ AO: %d | Temp: %.2f", data.flame, data.mq, data.temp);
        if (isnan(data.temp)) {
            ESP_LOGW(TAG, "DS18B20 Temp: FAIL");
        }

        // Gắn vào hàng đợi cho task khác xử lý (không block task này thêm)
        if (sensorQueue != NULL) {
            xQueueSend(sensorQueue, &data, portMAX_DELAY);
        }

        // Tần suất lấy mẫu và gửi (Hạ xuống còn 5s để tránh Server cảnh báo DEAD nhưng vẫn đạt
        // chuẩn an toàn)
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// 2. Task MQTT (Chờ dữ liệu từ Queue rồi mới xử lý)
void task_mqtt_process(void *pvParameters) {
    SensorData_t data;

    while (1) {
        // Đợi có data mới trong queue (block nếu chưa có data)
        if (xQueueReceive(sensorQueue, &data, portMAX_DELAY)) {
            // Giao cho thư viện chuyên dụng f5_mqtt lo việc map struct, JSON, Kalman, và Push mạng
            // Do trong flame.c đã có sắn logic: return (raw_value == 0) ? 1U : 0U;
            // nên data.flame = 1 (khi có lửa), data.flame = 0 (khi bình thường)
            bool is_flame = (data.flame == 1);
            float smoke_f32 = (float)data.mq;
            uint8_t battery = 100;  // Mặc định pin 100%
            uint32_t current_millis = xTaskGetTickCount() * portTICK_PERIOD_MS;

            ESP_LOGI(TAG, "Đưa dữ liệu vào filter & publish qua f5_mqtt...");
            f5_mqtt_update(data.temp, smoke_f32, is_flame, battery, current_millis);
        }
        // Thêm delay nhẹ nếu cần nhường tài nguyên
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// 3. Task LCD TFT 2.2 inch (Tối ưu tiết kiệm pin triệt để)
// ĐỔI TÊN TASK: task_led_matrix -> task_lcd_display
void task_lcd_display(void *pvParameters) {
    direction_t current_dir = DIR_OFF;

    // Vừa vào task, cho LCD ngủ đông ngay lập tức để tiết kiệm điện
    tft_display_direction(DIR_OFF);

    while (1) {
        direction_t new_dir;
        // Dùng portMAX_DELAY: Task sẽ NGỦ KỸ TRONG OS (0% CPU) tới khi có lệnh từ MQTT
        if (xQueueReceive(lcdQueue, &new_dir, portMAX_DELAY)) {
            if (new_dir != current_dir) {
                // Chỉ đánh thức/cập nhật LCD khi hướng thực sự thay đổi
                tft_display_direction(new_dir);
                current_dir = new_dir;
                ESP_LOGI(TAG, "LCD Updated DIR: %d", new_dir);
            }
        }
    }
}

void app_main(void) {
    /* Khởi tạo NVS — bắt buộc cho Wi-Fi và WiFi Manager */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_manager_start();

    // Chờ cho đến khi thực sự có WiFi mới chạy phần còn lại (MQTT, báo cáo dữ liệu...)
    ESP_LOGI(TAG, "Đang chờ kết nối WiFi...");
    while (!wifi_manager_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "Đã kết nối WiFi! Bắt đầu khởi động MQTT và cảm biến...");

    init_sntp();
    set_time_zone();
    while (!wait_for_time_sync()) {
        ESP_LOGW(TAG, "Đồng bộ thời gian thất bại, đang thử lại...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Mạng có rồi mới được gọi f5_mqtt_init
    f5_mqtt_init();

    mq_sensor_init();
    flame_sensor_init();
    ds18b20_init();

    // Khởi tạo và thiết lập màn hình LCD
    if (tft_init() == ESP_OK) {
        ESP_LOGI(TAG, "TFT 2.2\" khởi tạo THÀNH CÔNG");
    } else {
        ESP_LOGE(TAG, "TFT 2.2\" khởi tạo THẤT BẠI");
    }

    // Khởi tạo Queue
    sensorQueue = xQueueCreate(10, sizeof(SensorData_t));
    lcdQueue = xQueueCreate(5, sizeof(direction_t));  // ĐỔI TÊN KHỞI TẠO QUEUE

    // Phân nhỏ luồng bằng FreeRTOS
    xTaskCreate(task_lcd_display, "lcd_task", 2048, NULL, 5, NULL);  // ĐỔI TÊN TASK
    xTaskCreate(task_mqtt_process, "mqtt_task", 4096, NULL, 4, NULL);
    xTaskCreate(task_read_sensors, "sensor_task", 4096, NULL, 3, NULL);

    ESP_LOGI(TAG, "Hệ thống bắt đầu chạy đa luồng!");

    // GIỮ LUỒNG CHÍNH KHÔNG TRÔI (Tránh ESP32 kết thúc app_main hoặc bị Watchdog Reset)
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}