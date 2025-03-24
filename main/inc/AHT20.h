#ifndef AHT20_H
#define AHT20_H

#include "driver/i2c.h"

// Địa chỉ I2C của AHT20
#define AHT20_ADDR 0x38

// Chân I2C trên ESP32
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

void aht20_init();
void aht20_read(float *temperature, float *humidity);
void i2c_master_init();

#endif // AHT20_H
