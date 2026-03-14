#include "mqtt_time.h"
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"

void mqtt_time_init(void) {
    //Cấu hình múi giờ Việt Nam 
    tzset();

    //Khởi tạo SNTP mặc định của ESP32
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_setservername(1, "time.nist.gov");
    sntp_init();
}

mqtt_status_t mqtt_time_is_synced(void) {
    time_t now = 0;
    struct tm timeinfo = { 0 };
    time(&now);
    localtime_r(&now, &timeinfo);
    
    //đồng bộ nếu năm > 2020 (1970 + 50)
    if (timeinfo.tm_year > (2020 - 1900)) {
        return MQTT_TIME_OK;
    }
    return MQTT_TIME_ERROR;
}

int64_t mqtt_time_get_timestamp(void) {
    time_t now;
    time(&now);
    return (int64_t)now;
}