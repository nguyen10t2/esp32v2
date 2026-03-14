#include "buzzer.h"
#include "driver/gpio.h"
#include "esp_log.h"

// ĐỊNH NGHĨA CHÂN CẮM CÒI HÚ (Thay đổi tùy mạch thực tế)
#define BUZZER_PIN GPIO_NUM_5 

static const char *TAG = "ACT_BUZZER";

void buzzer_init(void) {
    // Reset chân và cấu hình nó làm Output
    gpio_reset_pin(BUZZER_PIN);
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    
    // Tắt còi mặc định khi vừa bật máy
    gpio_set_level(BUZZER_PIN, 0); 
    ESP_LOGI(TAG, "Khoi tao coi hu o chan GPIO %d thanh cong", BUZZER_PIN);
}

void buzzer_set_state(bool state) {
    // state = true thì xuất mức cao (1), false thì xuất mức thấp (0)
    gpio_set_level(BUZZER_PIN, state ? 1 : 0);
    ESP_LOGW(TAG, "-> Trang thai coi: %s", state ? "ĐANG HÚ" : "IM LẶNG");
}