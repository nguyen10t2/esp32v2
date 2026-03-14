#ifndef BUZZER_H
#define BUZZER_H

#include <stdbool.h>

/**
 * @brief Khởi tạo chân GPIO cho còi hú
 */
void buzzer_init(void);

/**
 * @brief Bật hoặc tắt còi hú
 * @param state true = Bật còi, false = Tắt còi
 */
void buzzer_set_state(bool state);

#endif