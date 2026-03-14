#include "f5_mqtt.h"
#include "mqtt_config.h"
#include "mqtt_filter.h"
#include "mqtt_json.h"
#include "mqtt_network.h"
#include "mqtt_time.h"
#include <stddef.h>

#include "esp_attr.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// CHỈ SỬ DỤNG 1 LẦN KHAI BÁO CÓ RTC_DATA_ATTR
RTC_DATA_ATTR static kalman_state_t mySmokeFilter;
RTC_DATA_ATTR static debounce_state_t myFlameCheck;
RTC_DATA_ATTR static ror_state_t myRoR;

#define MAX_OFFLINE_PAYLOADS 10 // Lưu trữ tối đa 10 chuỗi payload khi rớt mạng
RTC_DATA_ATTR static PayloadData offline_queue[MAX_OFFLINE_PAYLOADS];
RTC_DATA_ATTR static uint8_t queue_head = 0;  
RTC_DATA_ATTR static uint8_t queue_tail = 0;  
RTC_DATA_ATTR static uint8_t queue_count = 0; 

// Hàm nhét data vào hàng đợi khi mất mạng
static void enqueue_payload(PayloadData data) {
    if (queue_count < MAX_OFFLINE_PAYLOADS) {
        offline_queue[queue_tail] = data;
        queue_tail = (queue_tail + 1) % MAX_OFFLINE_PAYLOADS;
        queue_count++;
    } else {
        // Hàng đợi đầy -> Ghi đè lên data cũ nhất
        offline_queue[queue_tail] = data;
        queue_tail = (queue_tail + 1) % MAX_OFFLINE_PAYLOADS;
        queue_head = (queue_head + 1) % MAX_OFFLINE_PAYLOADS;
    }
}

// Hàm đẩy toàn bộ data kẹt lên Server khi có mạng lại
static void flush_offline_queue(void) {
    while (queue_count > 0) {
        PayloadData old_data = offline_queue[queue_head];
        
        char json_buffer[256];
        if (mqtt_json_serialize(&old_data, json_buffer, sizeof(json_buffer))) {
            mqtt_network_publish(TOPIC_PUBLISH, json_buffer);
        }
        
        queue_head = (queue_head + 1) % MAX_OFFLINE_PAYLOADS;
        queue_count--;
    }
}

void f5_mqtt_init(void) {
    // CHỐNG MẤT TRÍ NHỚ: Chỉ khởi tạo lại bộ lọc nếu không phải tỉnh dậy từ Deep Sleep
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_TIMER) {
        //Khởi tạo bộ lọc (Mặc định: sai số 10, nhiễu 0.1, trễ lửa 500ms)
        kalman_init(&mySmokeFilter, DEFAULT_KALMAN_ERROR, DEFAULT_KALMAN_ERROR, DEFAULT_KALMAN_NOISE);
        debounce_init(&myFlameCheck, DEFAULT_DEBOUNCE_DELAY);
        
        ror_init(&myRoR, DEFAULT_ROR_THRESHOLD); // Khởi tạo thuật toán RoR
    }

    //Khởi tạo mạng & Đồng hồ
    mqtt_time_init();
    mqtt_network_init();
}

bool f5_mqtt_update(float raw_temp, float raw_smoke, bool raw_flame, uint8_t battery, uint32_t current_millis) {
    
    //LỌC DỮ LIỆU
    float clean_smoke = kalman_apply(&mySmokeFilter, raw_smoke);
    bool is_fire_real = debounce_check(&myFlameCheck, raw_flame, current_millis);

    // Chạy thuật toán kiểm tra nhiệt độ tăng sốc
    bool is_temp_spiking = ror_check(&myRoR, raw_temp, current_millis);

    //QUYẾT ĐỊNH TRẠNG THÁI (STATUS)
    // todo: có thể thêm logic phức tạp hơn để xác định trạng thái NODE_WARNING,
    //NODE FIRE dựa trên nhiều yếu tố chứ không chỉ mỗi ngưỡng khói hoặc lửa,
    // ví dụ: có thể kết hợp nhiệt độ cao + khói nhiều nhưng chưa đủ để gọi là cháy thì vẫn là cảnh báo.
    // Hoặc có thể thêm trạng thái NODE_ERROR nếu cảm biến có dấu hiệu lỗi (ví dụ: nhiệt độ âm hoặc khói âm).
    // dựa trên nhiều yếu tố (ví dụ: nhiệt độ cao + khói nhiều nhưng chưa đủ để gọi là cháy)
    node_status_t current_status;
    if (is_fire_real || is_temp_spiking) {
        current_status = NODEFIRE;     // Sửa: Bỏ dấu gạch dưới
    } else if (clean_smoke > 300.0f || raw_temp > 50.0f) { // WARMING hardcode threshold
        current_status = NODEWARNING;  // Sửa: Bỏ dấu gạch dưới
    } else {
        current_status = NODEALIVE;    // Sửa: Bỏ dấu gạch dưới
    }

    //ĐÓNG KHUÔN PAYLOAD
    PayloadData myData = {
        .node_id = NODE_ID,
        .temperature = raw_temp,
        .smoke = clean_smoke,
        .flame = is_fire_real,         // Sửa: Đổi từ .fire sang .flame
        .battery = battery,
        .status = current_status,
        .timestamp = mqtt_time_get_timestamp()
    };

    bool is_sent = false;

    //Ép Json và bắn
    if (mqtt_network_is_connected()) {
        // CÓ MẠNG: Mở kho xả hết data cũ bị kẹt lên trước (nếu có)
        flush_offline_queue(); 
        
        // Bắn tiếp data mới nhất của chu kỳ này
        char json_buffer[256];
        if (mqtt_json_serialize(&myData, json_buffer, sizeof(json_buffer))) {
            mqtt_network_publish(TOPIC_PUBLISH, json_buffer);
            is_sent = true;
        }
    } else {
        // MẤT MẠNG: Cất data mới nhất vào kho chờ sóng hồi
        enqueue_payload(myData);
    }
    
    // THỰC THI DEEP SLEEP
    // Tuyệt đối KHÔNG ngủ nếu có cháy (NODEFIRE) để duy trì kết nối hú còi/báo động
    if (current_status == NODEALIVE || current_status == NODEWARNING) {
        // Cho chip thức thêm 500ms để chờ module WiFi gửi xong data
        vTaskDelay(pdMS_TO_TICKS(500)); 

        // Đặt báo thức ngủ 10 giây (Tính bằng micro giây)
        uint64_t sleep_time_us = 10 * 1000000; 
        esp_sleep_enable_timer_wakeup(sleep_time_us);
        
        // Sập nguồn! 
        vTaskDelay(pdMS_TO_TICKS(10)); //esp_deep_sleep_start(); 
    }

    return is_sent; 
}