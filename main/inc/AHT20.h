#ifndef AHT20_H
#define AHT20_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

// Định nghĩa các hằng số
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_FREQ_HZ 100000
#define AHT20_ADDR 0x38
#define AHT20_CMD_TRIGGER 0xAC
#define AHT20_CMD_SOFTRESET 0xBA
#define AHT20_CMD_INIT 0xBE

// Khai báo các hàm
void i2c_master_init(void);
esp_err_t aht20_write_command(uint8_t cmd);
esp_err_t aht20_trigger_measurement(void);
esp_err_t aht20_read_data(uint8_t *data, size_t length);
void aht20_init(void);
void aht20_get_temp_humidity(float *temperature, float *humidity);

#endif // AHT20_H