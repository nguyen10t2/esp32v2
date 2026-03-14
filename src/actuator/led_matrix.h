#ifndef LED_MATRIX_H
#define LED_MATRIX_H

/**
 * @brief Khởi tạo giao tiếp với màn hình LED Matrix (SPI/I2C)
 */
void led_matrix_init(void);

/**
 * @brief Vẽ mũi tên chỉ hướng lên màn hình LED
 * @param dir Ký tự hướng nhận từ Server ("N", "S", "E", "W", "OFF")
 */
void led_matrix_draw_direction(const char* dir);

#endif