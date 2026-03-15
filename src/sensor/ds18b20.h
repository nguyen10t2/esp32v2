#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Định nghĩa chân phần cứng của DS18B20
#define DS18B20_PIN GPIO_NUM_16

/**
 * @brief Khởi tạo cảm biến DS18B20
 * Thiết lập giao thức 1-wire để giao tiếp với điện trở kéo lên 5.7k bên ngoài.
 */
void ds18b20_init(void);

/**
 * @brief Đọc nhiệt độ từ DS18B20
 * Đọc đồng bộ để cân bằng thời gian với các cảm biến khác.
 *
 * @return Nhiệt độ tính bằng độ C, hoặc NAN nếu đọc thất bại.
 */
float ds18b20_get_temperature(void);

#ifdef __cplusplus
}
#endif

#endif /* DS18B20_H */
