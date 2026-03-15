#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Khởi tạo WiFi Manager.
 *
 * - Nếu đã có thông tin Wi-Fi lưu trong NVS → tự động kết nối STA.
 * - Nếu chưa có hoặc kết nối thất bại → bật AP "ESP32" + HTTP server
 *   để người dùng nhập SSID/password qua trình duyệt.
 *
 * Hàm này blocking cho đến khi kết nối STA thành công (hoặc AP được bật).
 */
void wifi_manager_start(void);

/**
 * @brief Kết nối đến Wi-Fi với SSID và password cho trước.
 *
 * Lưu thông tin vào NVS, sau đó kết nối.
 *
 * @param ssid     Tên mạng Wi-Fi (tối đa 32 ký tự).
 * @param password Mật khẩu (tối đa 64 ký tự, rỗng nếu mạng mở).
 * @return ESP_OK nếu kết nối thành công, ESP_FAIL nếu thất bại.
 */
esp_err_t wifi_manager_connect(const char *ssid, const char *password);

/**
 * @brief Kiểm tra đã kết nối Wi-Fi STA thành công chưa.
 * @return true nếu đã kết nối, false nếu chưa kết nối hoặc đang ở chế độ AP.
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Xóa thông tin Wi-Fi đã lưu trong NVS.
 */
void wifi_manager_reset(void);

#ifdef __cplusplus
}
#endif

#endif
