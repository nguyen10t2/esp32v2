#ifndef INIT_H
#define INIT_H

#include <time.h>

#include "esp_sntp.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Khởi tạo SNTP và đồng bộ thời gian
 */
void init_sntp(void);

/**
 * @brief Đặt múi giờ cho hệ thống
 */
void set_time_zone(void);

/**
 * @brief Chờ cho đến khi thời gian được đồng bộ
 * @return true nếu đồng bộ thành công, false nếu vượt quá số lần thử
 */
bool wait_for_time_sync(void);

/**
 * @brief Lấy timestamp hiện tại (tính bằng giây từ Epoch)
 * @return Timestamp giây hiện tại
 */
int64_t esp_time_get_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif  // INIT_H