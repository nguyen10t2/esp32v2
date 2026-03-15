#include "mqtt_network.h"
#include "mqtt_config.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <stdio.h>
#include <string.h> 
#include "cJSON.h"

// Hàm trung gian đẩy data sang Task điều khiển màn hình (viết ở main.c)
extern void update_lcd_direction(uint8_t dir); 

static const char *TAG = "F5_NETWORK";

static esp_mqtt_client_handle_t client = NULL;
static bool is_connected = false;

//Xử lý sự kiện
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Ket noi thanh cong den Broker: %s", MQTT_BROKER);
            is_connected = true;
            
            // Đăng ký lắng nghe lệnh từ Server cho TẤT CẢ các node (dùng Wildcard +)
            esp_mqtt_client_subscribe(client, "esp32/cmd/+", 0);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Mat ket noi MQTT! Dang tu dong thu lai...");
            is_connected = false;
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "=> Nhan lenh tu Topic: %.*s", event->topic_len, event->topic);

            // 1. Copy payload nhận được ra một chuỗi string chuẩn C
            char *json_str = malloc(event->data_len + 1);
            if (json_str == NULL) {
                ESP_LOGE(TAG, "Loi: Khong du RAM de xu ly JSON!");
                break;
            }
            memcpy(json_str, event->data, event->data_len);
            json_str[event->data_len] = '\0';

            // 2. Mổ xẻ JSON
            cJSON *root = cJSON_Parse(json_str);
            if (root != NULL) {
                
                // --- XỬ LÝ LỆNH: HƯỚNG SƠ TÁN (dir) ---
                cJSON *dir_item = cJSON_GetObjectItem(root, "dir");
                
                // Kiểm tra xem backend có gửi đúng định dạng số (0-4) không
                if (cJSON_IsNumber(dir_item)) {
                    int dir_value = dir_item->valueint;
                    ESP_LOGW(TAG, "-> LENH XUONG: TFT LCD = HUONG (ID: %d)", dir_value);
                    
                    // Bắn tín hiệu sang cho Task màn hình xử lý
                    update_lcd_direction((uint8_t)dir_value);
                } else {
                    ESP_LOGE(TAG, "Loi: Truong 'dir' khong phai la so nguyen!");
                }

                // Dọn dẹp RAM
                cJSON_Delete(root); 
            } else {
                ESP_LOGE(TAG, "Loi: Payload khong dung chuan JSON!");
            }
            
            free(json_str); 
            break;
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "Lỗi MQTT Client!");
            break;
            
        default:
            break;
    }
}

void mqtt_network_init(void) {
    char broker_uri[64];
    snprintf(broker_uri, sizeof(broker_uri), "mqtt://%s:%d", MQTT_BROKER, MQTT_PORT);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,
        
        // Nếu Node này mất điện/mất mạng đột ngột, Broker sẽ tự động bắn tin nhắn này đi
        .session.last_will.topic = "esp32/status",
        .session.last_will.msg = "{\"status\": \"OFFLINE\", \"reason\": \"connection_lost\"}",
        .session.last_will.qos = 1,
        .session.last_will.retain = 1 // Lưu trên server để App mở lên là thấy ngay chữ Offline
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

bool mqtt_network_is_connected(void) {
    return is_connected;
}

void mqtt_network_publish(const char *topic, const char *payload) {
    if (is_connected && client != NULL) {
        int msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
        ESP_LOGI(TAG, "Ban JSON len thanh cong -> Topic: %s", topic);
    } else {
        ESP_LOGE(TAG, "Chua co ket noi MQTT, dua vao hang doi Offline...");
    }
}