#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <math.h>
#include "freertos/queue.h"

#include "wifi_manager/wifi_manager.h"
#include "sensor/mq.h"
#include "sensor/flame.h"
#include "sensor/ds18b20.h"
#include "mqtt/f5_mqtt.h"
// #include "mqtt/f5_mqtt.h" // TODO: Include MQTT header
// #include "led_matrix.h"   // TODO: Include LED header

static const char *TAG = "FIRE_NODE";

// Cấu trúc gói dữ liệu đọc từ sensor
typedef struct {
    uint8_t flame;
    uint16_t mq;
    float temp;
} SensorData_t;

// Hàng đợi truyền dữ liệu giữa các Task
QueueHandle_t sensorQueue;

// 1. Task Đọc Cảm Biến (Chạy độc lập)
void task_read_sensors(void *pvParameters) {
    SensorData_t data;
    while(1) {
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

        // Tần suất lấy mẫu và gửi (1.5 - 2s/lần)
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// 2. Task MQTT (Chờ dữ liệu từ Queue rồi mới xử lý)
void task_mqtt_process(void *pvParameters) {
    SensorData_t data;

    while(1) {
        // Đợi có data mới trong queue (block nếu chưa có data)
        if (xQueueReceive(sensorQueue, &data, portMAX_DELAY)) {
            
            // Giao cho thư viện chuyên dụng f5_mqtt lo việc map struct, JSON, Kalman, và Push mạng
            // Do trong flame.c đã có sắn logic: return (raw_value == 0) ? 1U : 0U; 
            // nên data.flame = 1 (khi có lửa), data.flame = 0 (khi bình thường)
            bool is_flame = (data.flame == 1); 
            float smoke_f32 = (float)data.mq;
            uint8_t battery = 100;             // Mặc định pin 100%
            uint32_t current_millis = xTaskGetTickCount() * portTICK_PERIOD_MS;

            ESP_LOGI(TAG, "Đưa dữ liệu vào filter & publish qua f5_mqtt...");
            f5_mqtt_update(data.temp, smoke_f32, is_flame, battery, current_millis);
        }
        // Thêm delay nhẹ nếu cần nhường tài nguyên
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// 3. Task LED Matrix 8x8 (Cập nhật cực nhanh, chớp nháy mượt mà)
void task_led_matrix(void *pvParameters) {
    while(1) {
        // Hàm quét LED Matrix
        // led_matrix_scan();
        
        // Render 20fps -> độ trễ 50ms, không bị giật
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


void app_main(void)
{
    /* Khởi tạo NVS — bắt buộc cho Wi-Fi và WiFi Manager */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_manager_start();

    // Chờ cho đến khi thực sự có WiFi mới chạy phần còn lại (MQTT, báo cáo dữ liệu...)
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    while(!wifi_manager_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "WiFi connected! Starting MQTT and sensors...");

    // Mạng có rồi mới được gọi f5_mqtt_init
    f5_mqtt_init();

    mq_sensor_init();
    flame_sensor_init();
    ds18b20_init();

    // Khởi tạo Queue với kích thước 10 packet
    sensorQueue = xQueueCreate(10, sizeof(SensorData_t));

    // Phân nhỏ luồng bằng FreeRTOS
    xTaskCreate(task_led_matrix, "led_task", 2048, NULL, 5, NULL);     
    xTaskCreate(task_mqtt_process, "mqtt_task", 4096, NULL, 4, NULL);   
    xTaskCreate(task_read_sensors, "sensor_task", 4096, NULL, 3, NULL); 

    ESP_LOGI(TAG, "System started asynchronously!");

    // GIỮ LUỒNG CHÍNH KHÔNG TRÔI (Tránh ESP32 kết thúc app_main hoặc bị Watchdog Reset)
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}