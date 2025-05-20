#pragma once

#include "esp_adc/adc_oneshot.h"

#define RAIN_SENSOR_ADC_CHANNEL ADC_CHANNEL_3 // GPIO39 = ADC1_CH3

void rain_sensor_init(adc_oneshot_unit_handle_t *handle);
int get_rain_level_percent(void);
