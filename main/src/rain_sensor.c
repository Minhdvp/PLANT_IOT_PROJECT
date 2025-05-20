#include "rain_sensor.h"
#include "esp_log.h"

static const char *TAG = "RAIN_SENSOR";
static adc_oneshot_unit_handle_t adc_handle;

void rain_sensor_init(adc_oneshot_unit_handle_t *handle)
{
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*handle, RAIN_SENSOR_ADC_CHANNEL, &config));

    adc_handle = *handle;

    ESP_LOGI(TAG, "Rain sensor initialized.");
}

int get_rain_level_percent(void)
{
    int raw = 0;
    esp_err_t err = adc_oneshot_read(adc_handle, RAIN_SENSOR_ADC_CHANNEL, &raw);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "ADC read failed!");
        return -1;
    }

    float rain_percent = (4095.0f - raw) / 4095.0f * 100.0f;

    if (rain_percent < 0.0f) rain_percent = 0.0f;
    if (rain_percent > 100.0f) rain_percent = 100.0f;

    ESP_LOGI(TAG, "Rain level: raw=%d, percent=%.1f%%", raw, rain_percent);
    return (int)rain_percent;
}
