// connect_FB.h
#ifndef CONNECT_FB_H
#define CONNECT_FB_H

#include "esp_err.h"

// Hàm khởi tạo Firebase với API Key và Database URL
esp_err_t firebase_init(const char *api_key, const char *database_url);

// Ghi dữ liệu lên Firebase tại đường dẫn chỉ định
esp_err_t firebase_write(const char *path, const char *data);

// Đọc dữ liệu từ Firebase tại đường dẫn chỉ định
esp_err_t firebase_read(const char *path, char *buffer, size_t buffer_size);

// Hàm gửi dữ liệu lên Firebase (được gọi từ task cảm biến)
esp_err_t send_data_to_firebase(const char *data_json);

esp_err_t firebase_post(const char *path, const char *data);
#endif // CONNECT_FB_H
