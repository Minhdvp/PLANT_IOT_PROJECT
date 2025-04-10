#ifndef READ_SENSOR_H
#define READ_SENSOR_H

typedef struct PotState_t
{
    float temperature;  // Nhiệt độ
    float humidity;     // Độ ẩm
    int soil_moisture; // Độ ẩm đất
} PotState_t;

void init_sensors();  // Khởi tạo cảm biến và bắt đầu task đọc dữ liệu

#endif // READ_SENSOR_H
