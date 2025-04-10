#include <stdio.h>
#include "read_sensor.h"
#include "AHT20.h"
#include "soil_moisture.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "mesh_send.h"
#include "connect_FB.h"

static const char *TAG = "SENSOR_TASK";

// Task đọc dữ liệu cảm biến và ghi lên Firebase
void sensor_task(void *pvParameters)
{
    while (1)
    {
        float temperature = -1;
        float humidity = -1;
        int soil_moisture = -1;

        // Đọc dữ liệu từ AHT20
        aht20_read(&temperature, &humidity);
        if (!(temperature >= -40 && temperature <= 85 && humidity >= 0 && humidity <= 100))
        {
            printf("[Sensor] AHT20 -> No sensor signal detected!\n");
            temperature = -1;
            humidity = -1;
        }

        // Đọc dữ liệu từ Soil Moisture
        soil_moisture = get_soil_moisture();
        if (!(soil_moisture >= 0 && soil_moisture <= 1023))
        {
            printf("[Sensor] Soil Moisture -> No sensor signal detected!\n");
            soil_moisture = -1;
        }

        // Chỉ ghi lên Firebase nếu dữ liệu hợp lệ
        if (temperature != -1 && humidity != -1 && soil_moisture != -1)
        {
            PotState_t pot_state;
            pot_state.temperature = temperature;
            pot_state.humidity = humidity;
            pot_state.soil_moisture = soil_moisture;

            send_json_to_root(&pot_state); // Gửi dữ liệu JSON đến root node
        }
        else
        {
            printf("[Warning] Invalid sensor data, skipping Firebase update.\n");
        }

        printf("-------------------------------\n");
        vTaskDelay(2000 / portTICK_PERIOD_MS); // Delay 2 giây
    }
}

// Hàm khởi tạo và bắt đầu task đọc cảm biến
void init_sensors()
{
    ESP_LOGI(TAG, "Initializing sensors...");
    i2c_master_init();         // Khởi tạo I2C
    aht20_init();              // Khởi tạo cảm biến AHT20
    soil_moisture_init();      // Khởi tạo cảm biến độ ẩm đất
    firebase_init(NULL, NULL); // Khởi tạo Firebase (API Key và URL đã đặt trước)

    // Tạo task đọc dữ liệu cảm biến
    xTaskCreate(sensor_task, "Sensor Task", 4096, NULL, 5, NULL);
}
