#include "AHT20.h"
#include "esp_log.h"

#define I2C_MASTER_NUM I2C_NUM_0           // Use I2C number 0
#define I2C_MASTER_SCL_IO 22               // SCL pin
#define I2C_MASTER_SDA_IO 21               // SDA pin
#define I2C_MASTER_FREQ_HZ 100000          // I2C frequency 100kHz

static const char *TAG = "AHT20";

void i2c_master_init() {
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    ESP_LOGI(TAG, "I2C initialized!");
}

void aht20_init() {
    uint8_t cmd[3] = {0xBE, 0x08, 0x00};
    i2c_master_write_to_device(I2C_MASTER_NUM, AHT20_ADDR, cmd, 3, 1000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "AHT20 initialized!");
}

void aht20_read(float *temperature, float *humidity) {
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};  // Lệnh trigger đo
    uint8_t data[6] = {0};

    // Gửi lệnh đo
    if (i2c_master_write_to_device(I2C_MASTER_NUM, AHT20_ADDR, cmd, 3, 1000 / portTICK_PERIOD_MS) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send measurement command to AHT20");
        return;
    }

    // Chờ cảm biến sẵn sàng
    do {
        if (i2c_master_read_from_device(I2C_MASTER_NUM, AHT20_ADDR, data, 1, 1000 / portTICK_PERIOD_MS) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read status byte from AHT20");
            return;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    } while (data[0] & 0x80);  // Bit 7 = busy

    // Đọc 6 byte dữ liệu
    if (i2c_master_read_from_device(I2C_MASTER_NUM, AHT20_ADDR, data, 6, 1000 / portTICK_PERIOD_MS) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read sensor data from AHT20");
        return;
    }

    uint32_t hum = ((data[1] << 12) | (data[2] << 4) | (data[3] >> 4));
    uint32_t temp = (((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5]);

    *humidity = (float)hum * 100.0 / 1048576.0;
    *temperature = (float)temp * 200.0 / 1048576.0 - 50.0;

    if (hum == 0 && temp == 0) {
        ESP_LOGW(TAG, "[Sensor] AHT20 -> No sensor signal detected!");
    }

    ESP_LOGI(TAG, "Temperature: %.2f °C, Humidity: %.2f %%", *temperature, *humidity);
}

    