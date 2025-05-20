#include "soil_moisture.h"
#include "esp_log.h"

static const char *TAG = "SOIL_MOISTURE";
static adc_oneshot_unit_handle_t adc_handle;

void soil_moisture_init(adc_oneshot_unit_handle_t *handle)
{
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*handle, SOIL_MOISTURE_ADC_CHANNEL, &config));

    adc_handle = *handle;

    ESP_LOGI(TAG, "Soil moisture sensor initialized!");
}

int get_soil_moisture(void)
{
    int raw = 0;
    esp_err_t err = adc_oneshot_read(adc_handle, SOIL_MOISTURE_ADC_CHANNEL, &raw);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "ADC read failed!");
        return -1;
    }

    ESP_LOGI(TAG, "Soil moisture raw value: %d", raw);
    return raw;
}
