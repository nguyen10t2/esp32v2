#ifndef _TFT_H_
#define _TFT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Base colors
#define TFT_COLOR_BLACK 0x0000
#define TFT_COLOR_WHITE 0xFFFF
#define TFT_COLOR_RED 0xF800
#define TFT_COLOR_GREEN 0x07E0
#define TFT_COLOR_BLUE 0x001F

    /**
     * @brief Initialize the TFT LCD.
     * @return ESP_OK on success, otherwise error.
     */
    esp_err_t tft_init(void);

    /**
     * @brief Fill the entire screen with a specific color.
     * @param color 16-bit RGB565 color.
     */
    void tft_fill_screen(uint16_t color);

    /**
     * @brief Draw a string at coordinates (x, y).
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param str The string to draw.
     * @param color 16-bit RGB565 color of the text.
     */
    void tft_draw_string(int x, int y, const char *str, uint16_t color);

#ifdef __cplusplus
}
#endif

#endif // _TFT_H_
