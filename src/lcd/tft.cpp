#include "tft.h"

// Use LovyanGFX v1
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

/* 
 * Production driver for generic 2.2" SPI LCD.
 * 2.2" displays are usually ILI9341. 
 * If you have a different driver IC (e.g. ILI9225 or ST7789), 
 * update the lgfx::Panel_... accordingly!
 */
class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9341     _panel_instance;
    lgfx::Bus_SPI           _bus_instance;

public:
    LGFX(void)
    {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI3_HOST; // Use VSPI/SPI3_HOST for ESP32
            cfg.spi_mode = 0;
            cfg.freq_write = 20000000; // Hạ xuống 20MHz để đảm bảo tín hiệu ổn định hơn
            cfg.freq_read  = 16000000;
            cfg.spi_3wire  = false;
            cfg.use_lock   = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO; 
            
            // Default wiring for ESP32 DOIT DevKit V1
            cfg.pin_sclk = 18;
            cfg.pin_mosi = 23;
            cfg.pin_miso = 19;
            cfg.pin_dc   = 2;
            
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = 5;
            cfg.pin_rst          = 4;
            cfg.pin_busy         = -1;
            cfg.panel_width      = 240;
            cfg.panel_height     = 320;
            cfg.offset_x         = 0;
            cfg.offset_y         = 0;
            cfg.offset_rotation  = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable         = true;
            cfg.invert           = false;
            cfg.rgb_order        = false;
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = false;

            _panel_instance.config(cfg);
        }

        setPanel(&_panel_instance);
    }
};

static LGFX tft;

extern "C" {

esp_err_t tft_init(void)
{
    if (tft.init()) {
        tft.setRotation(1); // Set to landscape by default
        return ESP_OK;
    }
    return ESP_FAIL;
}

void tft_fill_screen(uint16_t color)
{
    tft.fillScreen(color);
}

void tft_draw_string(int x, int y, const char *str, uint16_t color)
{
    // Thêm màu nền (Black) để xoá đi điểm ảnh cũ, tránh hiện tượng chữ đè lên nhau thành cục hình vuông
    tft.setTextColor(color, TFT_COLOR_BLACK);
    tft.drawString(str, x, y);
}

} // extern "C"
