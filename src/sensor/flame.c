#include "flame.h"

#include "driver/gpio.h"

void flame_sensor_init()
{
	gpio_set_direction(FLAME_DO, GPIO_MODE_INPUT);
}

uint8_t get_raw_flame_digital_value()
{
	const int raw_value = gpio_get_level(FLAME_DO);
	return (raw_value == 0) ? 1U : 0U;
}
