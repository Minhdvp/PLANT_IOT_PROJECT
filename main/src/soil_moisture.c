#include "soil_moisture.h"

static const char *TAG = "SOIL_MOISTURE";
static adc_oneshot_unit_handle_t adc_handle;

void soil_moisture_init(void) {
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1
    };
    esp_err_t ret = adc_oneshot_new_unit(&unit_cfg, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC unit: %d", ret);
        return;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12
    };
    ret = adc_oneshot_config_channel(adc_handle, SOIL_MOISTURE_ADC_CHANNEL, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel: %d", ret);
        return;
    }

    ESP_LOGI(TAG, "Soil moisture sensor initialized!");
}

int get_soil_moisture(void) {
    int value = 0;
    esp_err_t ret = adc_oneshot_read(adc_handle, SOIL_MOISTURE_ADC_CHANNEL, &value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ADC: %d", ret);
        return -1;  // Trả về -1 nếu đọc thất bại
    }
    ESP_LOGI(TAG, "Soil moisture raw value: %d", value);
    return value;  // Trả về giá trị thô (0-4095)
}