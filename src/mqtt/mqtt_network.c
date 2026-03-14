#include "mqtt_network.h"
#include "mqtt_config.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <stdio.h>
#include "cJSON.h"

static const char *TAG = "F5_NETWORK";

static esp_mqtt_client_handle_t client = NULL;
static bool is_connected = false;

//Xử lý sự kiện
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Ket noi thanh cong: %s", MQTT_BROKER);
            is_connected = true;
            
            //Tự động đăng ký lắng nghe lệnh từ Server khi vừa kết nối xong
            esp_mqtt_client_subscribe(client, TOPIC_SUBSCRIBE, 0);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Mat ket noi MQTT! Dang tu dong thu lai...");
            is_connected = false;
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Nhan lenh dieu khien tu Topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Noi dung lenh: %.*s", event->data_len, event->data);

            case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Nhan lenh tu Server ở Topic: %.*s", event->topic_len, event->topic);

            // 1. Copy payload nhận được ra một chuỗi string chuẩn C (có ký tự '\0' ở cuối)
            char *json_str = malloc(event->data_len + 1);
            memcpy(json_str, event->data, event->data_len);
            json_str[event->data_len] = '\0';

            // 2. Dùng cJSON mổ xẻ cái chuỗi vừa nhận
            cJSON *root = cJSON_Parse(json_str);
            if (root != NULL) {
                
                // --- XỬ LÝ LỆNH 1: CÒI HÚ (buzzer) ---
                cJSON *buzzer_item = cJSON_GetObjectItem(root, "buzzer");
                if (cJSON_IsBool(buzzer_item)) {
                    bool buzzer_state = cJSON_IsTrue(buzzer_item);
                    ESP_LOGW(TAG, "=> LỆNH XUỐNG: CÒI HÚ = %s", buzzer_state ? "BAT" : "TAT");
                    
                    // TODO: Tương lai bạn sẽ gọi hàm phần cứng bật/tắt còi ở đây
                    // Ví dụ: hardware_buzzer_set(buzzer_state);
                }

                // --- XỬ LÝ LỆNH 2: HƯỚNG SƠ TÁN (dir) ---
                cJSON *dir_item = cJSON_GetObjectItem(root, "dir");
                if (cJSON_IsString(dir_item) && (dir_item->valuestring != NULL)) {
                    ESP_LOGW(TAG, "=> LỆNH XUỐNG: LED MATRIX = HƯỚNG %s", dir_item->valuestring);
                    
                    // TODO: Tương lai bạn sẽ gọi hàm phần cứng vẽ LED ở đây
                    // Ví dụ: hardware_led_matrix_draw(dir_item->valuestring);
                }

                // Xóa object JSON sau khi dùng xong để tránh tràn RAM
                cJSON_Delete(root); 
            } else {
                ESP_LOGE(TAG, "Lỗi: Payload tải xuống không phải là JSON hợp lệ!");
            }
            
            // Xóa chuỗi string mượn tạm lúc nãy
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
    //Tạo chuỗi địa chỉ chuẩn: mqtt://f5.soict.io:1883
    char broker_uri[64];
    snprintf(broker_uri, sizeof(broker_uri), "mqtt://%s:%d", MQTT_BROKER, MQTT_PORT);

    //Cấu hình Client
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,
    };

    //Khởi tạo đối tượng
    client = esp_mqtt_client_init(&mqtt_cfg);
    
    //Đăng ký cái hàm xử lý sự kiện ở trên vào hệ thống
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    
    //Bấm nút Start (Từ đây nó tự lo mọi việc)
    esp_mqtt_client_start(client);
}

bool mqtt_network_is_connected(void) {
    return is_connected;
}

void mqtt_network_publish(const char *topic, const char *payload) {
    if (is_connected && client != NULL) {
        //Tham số: client, topic, data, len(0 = tự động đếm), qos(1 = đảm bảo gửi), retain(0)
        int msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
        ESP_LOGI(TAG, "Ban JSON len thanh cong %s (msg_id=%d)", topic, msg_id);
    } else {
        ESP_LOGE(TAG, "Chua co ket noi MQTT, khong the gui data!");
    }
}