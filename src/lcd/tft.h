#ifndef _TFT_H_
#define _TFT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Các màu cơ bản
#define TFT_COLOR_BLACK 0x0000
#define TFT_COLOR_WHITE 0xFFFF
#define TFT_COLOR_RED 0xF800
#define TFT_COLOR_GREEN 0x07E0
#define TFT_COLOR_BLUE 0x001F

    /**
     * @brief Enum chuẩn hướng sơ tán
     */
    typedef enum {
        DIR_OFF = 0,
        DIR_N   = 1,
        DIR_S   = 2,
        DIR_E   = 3,
        DIR_W   = 4,
    } direction_t;

    /**
     * @brief Khởi tạo màn hình TFT LCD.
     * @return ESP_OK nếu thành công, ngược lại báo lỗi.
     */
    esp_err_t tft_init(void);

    /**
     * @brief Đổ màu toàn bộ màn hình với màu chỉ định.
     * @param color Mã màu 16-bit RGB565.
     */
    void tft_fill_screen(uint16_t color);

    /**
     * @brief Vẽ chuỗi ký tự tại tọa độ (x, y).
     * @param x Tọa độ X.
     * @param y Tọa độ Y.
     * @param str Chuỗi ký tự cần vẽ.
     * @param color Mã màu 16-bit RGB565 của văn bản.
     */
    void tft_draw_string(int x, int y, const char *str, uint16_t color);

    /**
     * @brief Chuyển đổi trạng thái màn hình và vẽ hướng sơ tán dựa trên lệnh gửi xuống
     */
    void tft_display_direction(direction_t dir);

#ifdef __cplusplus
}
#endif

#endif // _TFT_H_
