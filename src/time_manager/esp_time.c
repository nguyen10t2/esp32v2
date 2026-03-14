#include "esp_log.h"
#include "esp_time.h"

static const char *TAG = "ESP_TIME_INIT";

void init_sntp(void) {
    ESP_LOGI(TAG, "Đang khởi tạo SNTP...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_init();
}

void set_time_zone(void) {
    //Cấu hình múi giờ Việt Nam (GMT+7)
    setenv("TZ", "GMT-7", 1);
    tzset();
}

bool wait_for_time_sync(void) {
    uint8_t retry = 0;
    const uint8_t retry_count = 15;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Đang chờ đồng bộ... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    
    if (retry >= retry_count) {
        ESP_LOGE(TAG, "Không thể đồng bộ thời gian sau %d lần thử.", retry_count);
        return false;
    } else {
        ESP_LOGI(TAG, "Thời gian đã được đồng bộ!");
        return true;
    }
}

int64_t esp_time_get_timestamp(void) {
    time_t now;
    time(&now);
    return (int64_t)now;
}