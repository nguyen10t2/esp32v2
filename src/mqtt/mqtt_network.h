#ifndef MQTT_NETWORK_H
#define MQTT_NETWORK_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Khởi tạo kết nối MQTT, đăng ký sự kiện và subscribe topic
     */
    void mqtt_network_init(void);

    /**
     * @brief Kiểm tra trạng thái kết nối MQTT
     * @return true nếu đã kết nối, false nếu chưa
     */
    bool mqtt_network_is_connected(void);

    /**
     * @brief Gửi dữ liệu lên topic MQTT
     * @param topic Chủ đề cần gửi
     * @param payload Dữ liệu cần gửi (dạng chuỗi)
     */
    void mqtt_network_publish(const char *topic, const char *payload);

#ifdef __cplusplus
}
#endif

#endif
