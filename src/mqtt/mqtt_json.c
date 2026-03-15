#include "mqtt_json.h"

#include <stdio.h>

uint8_t mqtt_json_serialize(const PayloadData *data, char *json_buffer, size_t buffer_size) {
    // Biểu diễn dữ liệu cảm biến dưới dạng JSON (Sắp xếp theo đúng Struct Rust)
    int written =
        snprintf(json_buffer, buffer_size,
                 "{\"timestamp\":%lld,\"temperature\":%.2f,\"smoke\":%.2f,\"node_id\":%u,\"flame\":"
                 "%s,\"battery\":%u,\"status\":%u}",
                 (long long)data->timestamp, data->temperature, data->smoke, data->node_id,
                 data->flame ? "true" : "false",  // Chuyển bool thành "true"/"false" cho JSON
                 data->battery, (uint8_t)data->status);

    if (written > 0 && (size_t)written < buffer_size) {
        return (uint8_t)written;
    }

    return (uint8_t)-1;  // Lỗi: buffer không đủ hoặc lỗi định dạng
}