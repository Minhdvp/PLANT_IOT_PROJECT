#ifndef READ_SENSOR_H
#define READ_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
// Cấu trúc lưu trạng thái của các cảm biến trong chậu cây
typedef struct PotState_t {
    float temperature;     // Nhiệt độ không khí (°C)
    float humidity;        // Độ ẩm không khí (%)
    int soil_moisture;     // Độ ẩm đất (0-100%)
    int rain_level;        // Mức độ mưa (0-100%)
    bool pump_running;
    bool warning;
} PotState_t;

// Khởi tạo cảm biến và bắt đầu task đọc dữ liệu
void init_sensors(void);

// Hàm trả về trạng thái cảm biến hiện tại
PotState_t get_current_pot_state(void);

#ifdef __cplusplus
}
#endif

#endif // READ_SENSOR_H
