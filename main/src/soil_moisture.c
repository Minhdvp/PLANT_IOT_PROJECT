#include "soil_moisture.h"
#include "driver/adc.h"
#include "esp_log.h"

static const char *TAG = "SOIL_MOISTURE";
#define SOIL_MOISTURE_ADC_CHANNEL ADC1_CHANNEL_0  // GPIO36

void soil_moisture_init() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(SOIL_MOISTURE_ADC_CHANNEL, ADC_ATTEN_DB_11);
    ESP_LOGI(TAG, "Soil moisture sensor initialized!");
}

int get_soil_moisture() {
    int value = adc1_get_raw(SOIL_MOISTURE_ADC_CHANNEL);
    ESP_LOGI(TAG, "Soil moisture value: %d", value);
    return value;
}
