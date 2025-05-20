#pragma once

#include "esp_adc/adc_oneshot.h"

#define SOIL_MOISTURE_ADC_CHANNEL ADC_CHANNEL_0 // GPIO34 = ADC1_CH6

void soil_moisture_init(adc_oneshot_unit_handle_t *handle);
int get_soil_moisture(void);
