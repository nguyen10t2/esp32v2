#include "led_matrix.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ACT_LED";

void led_matrix_init(void) {
    ESP_LOGI(TAG, "Khoi tao module LED Matrix...");
    // TODO: Khởi tạo chân SPI (MOSI, CLK, CS) hoặc I2C tại đây
}

void led_matrix_draw_direction(const char* dir) {
    if (dir == NULL) return;

    if (strcmp(dir, "N") == 0) {
        ESP_LOGI(TAG, ">> Hien thi mui ten: DI LEN (Bac)");
        // TODO: Gọi hàm phần cứng xuất mã HEX mũi tên đi lên
        
    } else if (strcmp(dir, "S") == 0) {
        ESP_LOGI(TAG, ">> Hien thi mui ten: DI XUONG (Nam)");
        // TODO: Gọi hàm phần cứng xuất mã HEX mũi tên đi xuống
        
    } else if (strcmp(dir, "E") == 0) {
        ESP_LOGI(TAG, ">> Hien thi mui ten: SANG PHAI (Dong)");
        // TODO: Gọi hàm phần cứng xuất mã HEX mũi tên sang phải
        
    } else if (strcmp(dir, "W") == 0) {
        ESP_LOGI(TAG, ">> Hien thi mui ten: SANG TRAI (Tay)");
        // TODO: Gọi hàm phần cứng xuất mã HEX mũi tên sang trái
        
    } else if (strcmp(dir, "OFF") == 0) {
        ESP_LOGI(TAG, ">> Hien thi: TAT MAN HINH");
        // TODO: Gọi hàm xóa trắng màn hình LED
        
    } else {
        ESP_LOGE(TAG, "Loi: Huong dieu huong khong hop le (%s)", dir);
    }
}