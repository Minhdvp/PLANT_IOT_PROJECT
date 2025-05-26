#include "water_sensor.h"
#include "esp_log.h"

static const char *TAG = "WATER_SENSOR";
static adc_oneshot_unit_handle_t adc_handle;

void water_sensor_init(adc_oneshot_unit_handle_t *handle)
{
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*handle, WATER_SENSOR_ADC_CHANNEL, &config));

    adc_handle = *handle;

    ESP_LOGI(TAG, "Water sensor initialized.");
}

int get_water_level_percent(void)
{
    int raw = 0;
    esp_err_t err = adc_oneshot_read(adc_handle, WATER_SENSOR_ADC_CHANNEL, &raw);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "ADC read failed!");
        return -1;
    }

    float water_percent = (raw - 900) * 100.0f / (1900 - 900);

    if (water_percent < 0.0f)
        water_percent = 0.0f;
    if (water_percent > 100.0f)
        water_percent = 100.0f;

    ESP_LOGI(TAG, "water level: raw=%d, percent=%.1f%%", raw, water_percent);
    return (int)(water_percent + 0.5f); // Làm tròn lên
}
