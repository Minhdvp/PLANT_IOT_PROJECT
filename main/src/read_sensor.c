#include <stdio.h>
#include "read_sensor.h"
#include "AHT20.h"
#include "soil_moisture.h"
#include "connect_FB.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



// Task đọc dữ liệu cảm biến và ghi lên Firebase
void sensor_task(void *pvParameters) {
    while (1) {
        float temperature = -1;
        float humidity = -1;
        int soil_moisture = -1;

        // Đọc dữ liệu từ AHT20
        aht20_get_temp_humidity(&temperature, &humidity);
        if (!(temperature >= -40 && temperature <= 85 && humidity >= 0 && humidity <= 100)) {
            printf("[Sensor] AHT20 -> No sensor signal detected!\n");
            temperature = -1;
            humidity = -1;
        }

        // Đọc dữ liệu từ Soil Moisture
        soil_moisture = get_soil_moisture();
        if (!(soil_moisture >= 0 && soil_moisture <= 4095)) {
            printf("[Sensor] Soil Moisture -> No sensor signal detected!\n");
            soil_moisture = -1;
        }

        // Chỉ ghi lên Firebase nếu dữ liệu hợp lệ
        if (temperature != -1 && humidity != -1 && soil_moisture != -1) {
            char data_json[512];

            // Tạo một JSON chứa tất cả dữ liệu
            snprintf(data_json, sizeof(data_json),
                     "{"
                     "\"Temperature\": %.2f,"
                     "\"Humidity\": %.2f,"
                     "\"SoilMoisture\": %d"
                     "}",
                     temperature, humidity, soil_moisture);

            // Gửi một lần lên Firebase
            if (firebase_write("/SensorData", data_json) == ESP_OK) {
                printf("[Firebase] Data sent successfully: %s\n", data_json);
            } else {
                printf("[Firebase] Failed to send sensor data!\n");
            }
        } else {
            printf("[Warning] Invalid sensor data, skipping Firebase update.\n");
        }

        printf("-------------------------------\n");
        vTaskDelay(2000 / portTICK_PERIOD_MS);  // Delay 2 giây
    }
}


// Hàm khởi tạo và bắt đầu task đọc cảm biến
void init_sensors() {
    i2c_master_init();         // Khởi tạo I2C
    aht20_init();              // Khởi tạo cảm biến AHT20
    soil_moisture_init();      // Khởi tạo cảm biến độ ẩm đất
    firebase_init(NULL, NULL); // Khởi tạo Firebase (API Key và URL đã đặt trước)

    // Tạo task đọc dữ liệu cảm biến
    xTaskCreate(sensor_task, "Sensor Task", 4096, NULL, 5, NULL);
}