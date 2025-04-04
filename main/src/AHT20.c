#include <stdio.h>
#include "AHT20.h"

void i2c_master_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) printf("I2C param config failed: %d\n", ret);
    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) printf("I2C driver install failed: %d\n", ret);
    else printf("I2C driver installed\n");
}

esp_err_t aht20_write_command(uint8_t cmd) {
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    i2c_master_start(handle);
    i2c_master_write_byte(handle, (AHT20_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(handle, cmd, true);
    i2c_master_stop(handle);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(handle);
    return ret;
}

esp_err_t aht20_trigger_measurement() {
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    i2c_master_start(handle);
    i2c_master_write_byte(handle, (AHT20_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(handle, AHT20_CMD_TRIGGER, true);
    i2c_master_write_byte(handle, 0x33, true);
    i2c_master_write_byte(handle, 0x00, true);
    i2c_master_stop(handle);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(handle);
    return ret;
}

esp_err_t aht20_read_data(uint8_t *data, size_t length) {
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    i2c_master_start(handle);
    i2c_master_write_byte(handle, (AHT20_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(handle, data, length, I2C_MASTER_LAST_NACK);
    i2c_master_stop(handle);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(handle);
    return ret;
}

void aht20_init() {
    esp_err_t ret;
    uint8_t status[1];

    ret = aht20_write_command(AHT20_CMD_SOFTRESET);
    if (ret != ESP_OK) printf("Soft reset failed: %d\n", ret);
    vTaskDelay(pdMS_TO_TICKS(50));

    ret = aht20_write_command(AHT20_CMD_INIT);
    if (ret != ESP_OK) printf("Init failed: %d\n", ret);
    vTaskDelay(pdMS_TO_TICKS(50));

    ret = aht20_read_data(status, 1);
    if (ret != ESP_OK) printf("Read status failed: %d\n", ret);
    else printf("Status after init: 0x%02x\n", status[0]);
}

void aht20_get_temp_humidity(float *temperature, float *humidity) {
    uint8_t data[6];
    esp_err_t ret;

    ret = aht20_trigger_measurement();
    if (ret != ESP_OK) {
        printf("Trigger failed: %d\n", ret);
        *temperature = -50.0;
        *humidity = -1.0;
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    ret = aht20_read_data(data, 6);
    if (ret != ESP_OK) {
        printf("Read failed: %d\n", ret);
        *temperature = -50.0;
        *humidity = -1.0;
        return;
    }

    if (data[0] & 0x80) {
        printf("Sensor busy: 0x%02x\n", data[0]);
        *temperature = -50.0;
        *humidity = -1.0;
        return;
    }

    printf("Raw data: %02x %02x %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3], data[4], data[5]);
    uint32_t raw_humidity = ((data[1] << 16) | (data[2] << 8) | data[3]) >> 4;
    uint32_t raw_temperature = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];

    *humidity = ((float)raw_humidity / 1048576.0) * 100.0;
    *temperature = ((float)raw_temperature / 1048576.0) * 200.0 - 50.0;
}