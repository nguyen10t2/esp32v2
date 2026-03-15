#ifndef MQ_DRIVER_H
#define MQ_DRIVER_H

#include <stdint.h>

#include "driver/adc.h"

#ifdef __cplusplus
extern "C" {
#endif
#define MQ_ADC ADC1_CHANNEL_7  // GPIO35
/**
 * @brief Khởi tạo cảm biến khí (MQ Sensor)
 */
void mq_sensor_init();

/**
 * @brief Đọc giá trị analog từ cảm biến MQ
 * @return Giá trị analog thô từ cảm biến MQ (0-4095)
 */
uint16_t get_raw_mq_analog_value();

#ifdef __cplusplus
}
#endif

#endif