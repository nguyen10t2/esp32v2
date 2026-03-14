#ifndef WIFI_MANAGER_HTTP_H
#define WIFI_MANAGER_HTTP_H

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Khởi động HTTP server phục vụ trang cấu hình Wi-Fi.
     *
     * Server phục vụ các endpoint:
     *   GET  /       -> Trang HTML captive portal
     *   GET  /scan   -> Quét danh sách Wi-Fi (JSON)
     *   POST /connect -> Nhận SSID + password, kết nối Wi-Fi
     *
     * @return ESP_OK nếu thành công.
     */
    esp_err_t wifi_manager_http_start(void);

    /**
     * @brief Dừng HTTP server.
     */
    void wifi_manager_http_stop(void);

#ifdef __cplusplus
}
#endif

#endif
