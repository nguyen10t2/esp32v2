#include "mq.h"

#include "driver/adc.h"

void mq_sensor_init()
{
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(MQ_ADC, ADC_ATTEN_DB_12);
}

uint16_t get_raw_mq_analog_value()
{
	return (uint16_t)adc1_get_raw(MQ_ADC);
}
