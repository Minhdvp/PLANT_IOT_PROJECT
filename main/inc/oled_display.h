// oled_display.h
#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

// Hàm hiển thị dữ liệu cảm biến lên màn hình OLED
void display_sensor_data(float temperature, float humidity, int soil_moisture, int rain_level);

// Hàm khởi tạo OLED (tùy chọn nếu muốn tách ra)
void oled_display_init(void);

#ifdef __cplusplus
}
#endif

#endif // OLED_DISPLAY_H
