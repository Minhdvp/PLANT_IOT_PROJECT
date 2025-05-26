#pragma once

#include "esp_adc/adc_oneshot.h"

#define WATER_SENSOR_ADC_CHANNEL ADC_CHANNEL_3 // GPIO39 = ADC1_CH3

void water_sensor_init(adc_oneshot_unit_handle_t *handle);
int get_water_level_percent(void);
