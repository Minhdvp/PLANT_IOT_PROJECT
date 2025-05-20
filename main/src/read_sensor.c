#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <time.h>
#include <esp_sntp.h>
#include <esp_log.h>

#include "read_sensor.h"
#include "AHT20.h"
#include "soil_moisture.h"
#include "rain_sensor.h"
#include "connect_FB.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"

#include "esp_log.h"
#include "mesh_send.h"
#include "connect_FB.h"

#include "oled_display.h"

#include "mosfet_control.h"


static adc_oneshot_unit_handle_t adc1_handle;  // Shared ADC handle

static const char *TAG = "SENSOR_TASK";

void sync_time_from_string(const char *time_str)
{
    struct tm tm;
    if (strptime(time_str, "%Y-%m-%d %H:%M:%S", &tm) == NULL) {
        ESP_LOGE("TIME_SYNC", "Failed to parse time string: %s", time_str);
        return;
    }

    time_t t = mktime(&tm);
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, NULL);

    ESP_LOGI("TIME_SYNC", "Time updated from root: %s", time_str);
}

// Task đọc dữ liệu cảm biến và ghi lên Firebase
void sensor_task(void *pvParameters)
{
    PotState_t pot_state;
    static bool pump_running = false;
    while (1)
    {
        float temperature = -1;
        float humidity = -1;
        int soil_moisture = -1;
        int rain_level = -1;
        bool warning;

        // Đọc dữ liệu từ Water_sensor
        rain_level = get_rain_level_percent();
        if (rain_level < 0 || rain_level > 100) {
            printf("[Sensor] Rain Sensor -> No sensor signal detected!\n");
            rain_level = -1;
        }
        // Đọc dữ liệu từ AHT20
        aht20_read(&temperature, &humidity);
        if (!(temperature >= -40 && temperature <= 85 && humidity >= 0 && humidity <= 100))
        {
            printf("[Sensor] AHT20 -> No sensor signal detected!\n");
            temperature = -1;
            humidity = -1;
        }

        // Đọc dữ liệu từ Soil Moisture
        int adc_value = get_soil_moisture();
        if (adc_value >= 0 && adc_value <= 4095)
        {
            float soil_percent = ((adc_value - 1500.0f) / (4095.0f - 1500.0f)) * 100.0f;
            soil_percent = 100.0f - soil_percent; // Đảo chiều
            if (soil_percent < 0.0f) soil_percent = 0.0f;
            if (soil_percent > 100.0f) soil_percent = 100.0f;

            soil_moisture = (int)soil_percent; // Gán lại dưới dạng % nguyên
        }
        else
        {
            printf("[Sensor] Soil Moisture -> No sensor signal detected!\n");
            soil_moisture = -1;
        }

        // Chỉ ghi lên Firebase nếu dữ liệu hợp lệ
        
        if (temperature != -1 && humidity != -1 && soil_moisture != -1 && rain_level != -1)
        {
        // Logic điều khiển cảnh báo
        warning = (rain_level >=0 && rain_level<=25)? true:false;

        // Logic điều khiển máy bơm
        if (soil_moisture >= 0) {
            if (soil_moisture < 30 && !pump_running) {
                printf("💧 Soil moisture low (%d%%) -> Turning ON pump\n", soil_moisture);
                pump_on();
                pump_running = true;  
            } else if (soil_moisture > 45 && pump_running) {
                printf("🌱 Soil moisture sufficient (%d%%) -> Turning OFF pump\n", soil_moisture);
                pump_off();
                pump_running = false;
            }
        }

            pot_state.temperature = temperature;
            pot_state.humidity = humidity;
            pot_state.soil_moisture = soil_moisture;
            pot_state.rain_level = rain_level;
            pot_state.pump_running = pump_running;
            pot_state.warning = warning;

            send_json_to_root(&pot_state); // Gửi dữ liệu JSON đến root node
            display_sensor_data(temperature, humidity, soil_moisture, rain_level);
        }
        else
        {
            printf("[Warning] Invalid sensor data, skipping Firebase update.\n");
        }

        printf("-------------------------------\n");
        vTaskDelay(60000 / portTICK_PERIOD_MS); // Delay 1 phút
    }
}

// Hàm khởi tạo và bắt đầu task đọc cảm biến
void init_sensors()
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    ESP_LOGI(TAG, "Initializing sensors...");
    mosfet_init(); // Khởi tạo MOSFET
    oled_display_init();
    i2c_master_init();         // Khởi tạo I2C
    aht20_init();              // Khởi tạo cảm biến AHT20
    soil_moisture_init(&adc1_handle);// Khởi tạo cảm biến độ ẩm đất
    rain_sensor_init(&adc1_handle);  // Khởi tạo cảm biến Water   
    firebase_init(NULL, NULL); // Khởi tạo Firebase (API Key và URL đã đặt trước)

    // Tạo task đọc dữ liệu cảm biến
    xTaskCreate(sensor_task, "Sensor Task", 8192, NULL, 5, NULL);
}