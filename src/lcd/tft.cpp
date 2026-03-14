#include "tft.h"

// Sử dụng LovyanGFX phiên bản v1
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

/* 
 * Driver sản phẩm cho màn hình 2.2" SPI LCD thông dụng.
 * Màn hình 2.2" thường dùng IC ILI9341. 
 * Nếu bạn có IC điều khiển khác (ví dụ: ILI9225 hoặc ST7789), 
 * hãy cập nhật lgfx::Panel_... cho phù hợp!
 */
class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9341     _panel_instance;
    lgfx::Bus_SPI           _bus_instance;
    lgfx::Light_PWM         _light_instance;

public:
    LGFX(void)
    {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI3_HOST; // Sử dụng VSPI/SPI3_HOST cho ESP32
            cfg.spi_mode = 0;
            cfg.freq_write = 20000000; // Hạ xuống 20MHz để đảm bảo tín hiệu ổn định hơn
            cfg.freq_read  = 16000000;
            cfg.spi_3wire  = false;
            cfg.use_lock   = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO; 
            
            // Cấu hình chân mặc định cho ESP32 DOIT DevKit V1
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
        
        { // Điều khiển đèn nền (Backlight) bằng PWM
            auto cfg = _light_instance.config();
            // CHÂN LED trên mạch bạn nối vào GPIO 21
            cfg.pin_bl = 21;        
            cfg.invert = false;
            cfg.freq   = 1000; // 1kHz là quá đủ cho LED nền, tiết kiệm tải CPU hơn so với 44kHz
            cfg.pwm_channel = 7;
            
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        setPanel(&_panel_instance);
    }
};

static LGFX tft;

extern "C" {

esp_err_t tft_init(void)
{
    if (tft.init()) {
        tft.setRotation(1); // Mặc định xoay ngang
        // Đảm bảo vừa khởi tạo xong LCD sẽ được tắt đèn nền và tắt hiển thị ngay lập tức
        tft.setBrightness(0);
        tft.writecommand(0x28); // Lệnh ILI9341: Display OFF
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

void tft_display_direction(direction_t dir)
{
    if (dir == DIR_OFF) {
        // Tắt đèn nền 100% tiết kiệm pin tuyệt đói!
        tft.setBrightness(0);
        
        // Cản luôn Driver LCD xuất tín hiệu (Nhanh hơn fillScreen + tiết kiệm thêm 20mA của IC màn hình)
        // Command 0x28 (Display OFF) giúp màn hình tắt hiển thị mà không cần SPI gửi hàng triệu pixel đen
        tft.writecommand(0x28);
    } else {
        // Có dữ liệu báo chạy => Mở vi mạch màn hình lên lại
        tft.writecommand(0x29); // Command 0x29: Display ON
        
        // Dọn màn hình trước trong khi đèn vẫn tắt để người dùng không thấy nét quét (Flicker)
        tft.fillScreen(TFT_COLOR_BLACK);
        
        tft.setTextSize(4);
        tft.setTextColor(TFT_COLOR_RED, TFT_COLOR_BLACK);
        
        const char* text = "";
        switch(dir) {
            case DIR_N: text = "NORTH (N) "; break;
            case DIR_S: text = "SOUTH (S) "; break;
            case DIR_E: text = "EAST (E)  "; break;
            case DIR_W: text = "WEST (W)  "; break;
            default: text = "  SYNC..  "; break;
        }
        
        tft_draw_string(10, 10, "EVACUATE!", TFT_COLOR_BLUE);
        tft_draw_string(10, 100, text, TFT_COLOR_RED);

        // Sau khi vẽ xong toàn bộ dữ liệu, bật đèn LED lên một phát ăn ngay!
        tft.setBrightness(255);
    }
}

} // extern "C"
