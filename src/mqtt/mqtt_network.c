#include "mqtt_network.h"
#include "mqtt_config.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <stdio.h>

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

            //CODE LED

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