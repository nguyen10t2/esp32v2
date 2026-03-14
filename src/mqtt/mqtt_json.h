#ifndef MQTT_JSON_H
#define MQTT_JSON_H

#include "mqtt_types.h"
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Chuyển đổi cấu trúc PayloadData thành chuỗi JSON.
     *
     * Hàm này sử dụng snprintf để tạo chuỗi JSON từ dữ liệu cảm biến.
     *
     * @param data Con trỏ đến cấu trúc PayloadData chứa dữ liệu cảm biến
     * @param json_buffer Bộ đệm để lưu trữ chuỗi JSON kết quả
     * @param buffer_size Kích thước của bộ đệm json_buffer
     * @return Số byte đã viết vào json_buffer (không bao gồm ký tự null-terminator), hoặc -1 nếu có lỗi
     */
    uint8_t mqtt_json_serialize(const PayloadData *data, char *json_buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif 