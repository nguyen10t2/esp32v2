#ifndef MQTT_TIME_H
#define MQTT_TIME_H

#include <stdint.h>
#include "mqtt_types.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Khởi tạo đồng bộ thời gian SNTP.
     *
     * Hàm này cấu hình múi giờ Việt Nam và khởi tạo SNTP client để đồng bộ thời gian.
     * Nên gọi hàm này một lần duy nhất khi khởi động node cảm biến.
     */
    void mqtt_time_init(void);

    /**
     * @brief Kiểm tra xem thời gian đã được đồng bộ thành công chưa.
     * @return MQTT_TIME_OK nếu đã đồng bộ, MQTT_TIME_ERROR nếu chưa đồng bộ hoặc có lỗi
     */
    mqtt_status_t mqtt_time_is_synced(void);

    /**
     * @brief Lấy timestamp hiện tại tính bằng milliseconds since epoch.
     * @return Timestamp hiện tại, hoặc -1 nếu có lỗi
     */
    int64_t mqtt_time_get_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif