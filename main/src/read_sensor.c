#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <time.h>
#include <esp_sntp.h>
#include <esp_log.h>

#include "read_sensor.h"
#include "AHT20.h"
#include "soil_moisture.h"
#include "water_sensor.h"
#include "connect_FB.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"

#include "esp_log.h"
#include "mesh_send.h"
#include "connect_FB.h"

#include "oled_display.h"

#include "mosfet_control.h"

static adc_oneshot_unit_handle_t adc1_handle; // Shared ADC handle

static const char *TAG = "SENSOR_TASK";

void sync_time_from_string(const char *time_str)
{
    struct tm tm;
    if (strptime(time_str, "%Y-%m-%d %H:%M:%S", &tm) == NULL)
    {
        ESP_LOGE("TIME_SYNC", "Failed to parse time string: %s", time_str);
        return;
    }

    time_t t = mktime(&tm);
    struct timeval now = {.tv_sec = t};
    settimeofday(&now, NULL);

    ESP_LOGI("TIME_SYNC", "Time updated from root: %s", time_str);
}

float normalize_increasing(float val, float min_val, float max_val)
{
    if (val <= min_val)
        return 0.0f;
    if (val >= max_val)
        return 1.0f;
    return (val - min_val) / (max_val - min_val);
}

float normalize_decreasing(float val, float min_val, float max_val)
{
    if (val <= min_val)
        return 1.0f;
    if (val >= max_val)
        return 0.0f;
    return (max_val - val) / (max_val - min_val);
}

// Hàm lấy trị tuyệt đối thủ công
float abs_val(float x)
{
    return (x < 0.0f) ? -x : x;
}

// Hàm giới hạn thủ công
float clamp(float x, float min_val, float max_val)
{
    if (x < min_val)
        return min_val;
    if (x > max_val)
        return max_val;
    return x;
}

float calculate_score(float soil, float humidity, float temperature)
{
    float soil_score = normalize_increasing(soil, 40.0f, 70.0f);
    float humidity_score = normalize_increasing(humidity, 50.0f, 80.0f);
    float temp_score = normalize_decreasing(abs_val(temperature - 25.0f), 0.0f, 10.0f);

    float score = (soil_score * 0.7f +
                   humidity_score * 0.2f +
                   temp_score * 0.1f) *
                  100.0f;

    return score;
}

int calculate_watering_duration(float score)
{
    float ratio = (100.0f - score) / 100.0f;
    ratio = clamp(ratio, 0.0f, 1.0f);

    float volume_ml = 50.0f + ratio * 100.0f;      // 50 → 150ml
    float duration_sec = volume_ml / 1300.0f * 60; // tốc độ 1L/phút

    return duration_sec;
}

// Task đọc dữ liệu cảm biến và ghi lên Firebase
void sensor_task(void *pvParameters)
{
    PotState_t pot_state;
    static bool pump_running = false;
    float score = 0;
    while (1)
    {
        float temperature = -1;
        float humidity = -1;
        int soil_moisture = -1;
        int water_level = -1;
        bool warning;

        // Đọc dữ liệu từ Water_sensor
        water_level = get_water_level_percent();
        if (water_level < 0 || water_level > 100)
        {
            printf("[Sensor] water Sensor -> No sensor signal detected!\n");
            water_level = -1;
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
            if (soil_percent < 0.0f)
                soil_percent = 0.0f;
            if (soil_percent > 100.0f)
                soil_percent = 100.0f;

            soil_moisture = (int)soil_percent; // Gán lại dưới dạng % nguyên
        }
        else
        {
            printf("[Sensor] Soil Moisture -> No sensor signal detected!\n");
            soil_moisture = -1;
        }

        // Chỉ ghi lên Firebase nếu dữ liệu hợp lệ

        if (temperature != -1 && humidity != -1 && soil_moisture != -1 && water_level != -1)
        {
            // Logic điều khiển cảnh báo
            warning = (water_level >= 0 && water_level <= 25) ? true : false;

            score = calculate_score(soil_moisture, humidity, temperature);
            // Logic điều khiển máy bơm
            if (score >= 30)
            {
                printf("Score is normal (%.2f). No need to water.\n", score);
                pump_off();
            }
            else
            {
                pump_running = true;
                // int duration_sec = calculate_watering_duration(score);
                // int volume_ml = duration_sec / 60.0f * 1000.0f;
                // printf("Score = %.2f → Watering %d ml in %d seconds\n", score, volume_ml, duration_sec);
                pump_on();
                vTaskDelay(5000 / portTICK_PERIOD_MS); // Đợi 5 giây để máy bơm hoạt động
                pump_off();                            // Tắt máy bơm sau 5 giây
            }

            pot_state.temperature = temperature;
            pot_state.humidity = humidity;
            pot_state.soil_moisture = soil_moisture;
            pot_state.water_level = water_level;
            pot_state.pump_running = pump_running;
            pot_state.warning = warning;

            send_json_to_root(&pot_state); // Gửi dữ liệu JSON đến root node
            display_sensor_data(temperature, humidity, soil_moisture, water_level);

            pump_running = false;
        }
        else
        {
            printf("[Warning] Invalid sensor data, skipping Firebase update.\n");
        }

        printf("-------------------------------\n");
        vTaskDelay(10000 / portTICK_PERIOD_MS); // Delay 1 phút
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
    i2c_master_init();                // Khởi tạo I2C
    aht20_init();                     // Khởi tạo cảm biến AHT20
    soil_moisture_init(&adc1_handle); // Khởi tạo cảm biến độ ẩm đất
    water_sensor_init(&adc1_handle);  // Khởi tạo cảm biến Water
    firebase_init(NULL, NULL);        // Khởi tạo Firebase (API Key và URL đã đặt trước)

    // Tạo task đọc dữ liệu cảm biến
    xTaskCreate(sensor_task, "Sensor Task", 9216, NULL, 10, NULL);
}