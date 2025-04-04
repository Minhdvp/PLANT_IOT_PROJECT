#ifndef SOIL_MOISTURE_H
#define SOIL_MOISTURE_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

// Định nghĩa hằng số
#define SOIL_MOISTURE_ADC_CHANNEL ADC_CHANNEL_0  // GPIO36

// Khai báo hàm
void soil_moisture_init(void);
int get_soil_moisture(void);

#endif // SOIL_MOISTURE_H