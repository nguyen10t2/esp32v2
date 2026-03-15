#ifndef FLAME_DRIVER_H
#define FLAME_DRIVER_H

#include <stdint.h>

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif
#define FLAME_DO GPIO_NUM_17

/**
 * @brief Khởi tạo cảm biến lửa (Flame Sensor)
 */
void flame_sensor_init();

/**
 * @brief Đọc giá trị số từ cảm biến lửa
 * @return 0: Không phát hiện lửa, 1: Phát hiện lửa
 */
uint8_t get_raw_flame_digital_value();

#ifdef __cplusplus
}
#endif

#endif