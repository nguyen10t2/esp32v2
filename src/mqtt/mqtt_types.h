#ifndef MQTT_TYPES_H
#define MQTT_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Trạng thái đồng bộ thời gian, được sử dụng để đánh giá xem node cảm biến đã đồng bộ thời
 * gian với máy chủ MQTT hay chưa.
 */
typedef enum {
    MQTT_TIME_OK = 0,     /**< Thời gian đã được đồng bộ thành công */
    MQTT_TIME_ERROR = -1, /**< Lỗi đồng bộ thời gian, có thể do mất kết nối hoặc lỗi phần mềm */
} mqtt_status_t;

/**
 * @brief Trạng thái của node cảm biến, được sử dụng để đánh giá tình trạng hoạt động của node.
 */
typedef enum {
    NODEALIVE = 0,   /**< Node đang hoạt động bình thường */
    NODEWARNING = 1, /**< Node có cảnh báo */
    NODEFIRE = 2,    /**< Node phát hiện cháy */
    NODEDEAD = 3,    /**< Node đã chết, mất kết nối */
    NODE_ERROR = 4   /**< Node gặp lỗi, có thể do cảm biến hỏng hoặc dữ liệu không hợp lệ */
} node_status_t;

/**
 * @brief Cấu trúc dữ liệu chứa thông tin cảm biến và trạng thái của node, được sử dụng làm payload
 * trong MQTT.
 */
typedef struct {
    int64_t timestamp; /**< Thời điểm ghi nhận dữ liệu (milliseconds since epoch) */

    float temperature; /**< Nhiệt độ tính bằng độ C */
    float smoke;       /**< Mức độ khói đã được lọc */

    uint16_t node_id; /**< ID duy nhất của node cảm biến */

    bool flame;      /**< Sửa tên: Trạng thái phát hiện cháy (true hoặc false) - Khớp với Rust */
    uint8_t battery; /**< Mức pin còn lại tính bằng phần trăm */
    node_status_t status; /**< Trạng thái của node cảm biến */
} PayloadData;

#ifdef __cplusplus
}
#endif

#endif