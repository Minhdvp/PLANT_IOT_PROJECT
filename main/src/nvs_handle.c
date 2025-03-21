#include <stdio.h>
#include <string.h>

#include <time.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"

#include "nvs_handle.h"

#define ARRAY_SIZE 5 // Kích thước mảng

// Hàm khởi tạo NVS
void nvs_init()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

// Hàm tải dữ liệu từ NVS
void load_data(uint16_t data_array_out[ARRAY_SIZE], int64_t time_array_out[ARRAY_SIZE])
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK)
    {
        size_t data_size = sizeof(uint16_t) * ARRAY_SIZE;
        size_t time_size = sizeof(int64_t) * ARRAY_SIZE;

        // Đọc mảng dữ liệu từ NVS
        err = nvs_get_blob(my_handle, "data_array", data_array_out, &data_size);
        if (err != ESP_OK)
        {
            printf("No data array found in NVS, initializing with zeros\n");
            memset(data_array_out, 0, data_size); // Nếu không tìm thấy, khởi tạo bằng 0
        }

        // Đọc mảng thời gian từ NVS
        err = nvs_get_blob(my_handle, "time_array", time_array_out, &time_size);
        if (err != ESP_OK)
        {
            printf("No time array found in NVS, initializing with zeros\n");
            memset(time_array_out, 0, time_size); // Nếu không tìm thấy, khởi tạo bằng 0
        }

        nvs_close(my_handle);
    }
    else
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
}

// Hàm lưu mảng dữ liệu và thời gian vào NVS
static void save_data_to_nvs(const uint16_t data_array[ARRAY_SIZE], const int64_t time_array[ARRAY_SIZE])
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK)
    {
        // Lưu mảng dữ liệu vào NVS
        err = nvs_set_blob(my_handle, "data_array", data_array, sizeof(uint16_t) * ARRAY_SIZE);
        if (err != ESP_OK)
        {
            printf("Failed to save data array to NVS\n");
        }

        // Lưu mảng thời gian vào NVS
        err = nvs_set_blob(my_handle, "time_array", time_array, sizeof(int64_t) * ARRAY_SIZE);
        if (err != ESP_OK)
        {
            printf("Failed to save time array to NVS\n");
        }

        nvs_commit(my_handle);
        nvs_close(my_handle);
    }
    else
    {
        printf("Error (%s) opening NVS handle for writing!\n", esp_err_to_name(err));
    }
}

// Hàm ghi dữ liệu mới vào NVS
void save_new_data(uint16_t new_data)
{
    uint16_t data_array[ARRAY_SIZE];
    int64_t time_array[ARRAY_SIZE];
    time_t now;
    time(&now);

    // Tải dữ liệu hiện tại từ NVS
    load_data(data_array, time_array);

    // Dịch chuyển mảng để thêm dữ liệu và thời gian mới nhất vào đầu mảng
    for (int i = ARRAY_SIZE - 1; i > 0; i--)
    {
        data_array[i] = data_array[i - 1];
        time_array[i] = time_array[i - 1];
    }
    data_array[0] = new_data;
    time_array[0] = now;

    // Lưu lại các mảng đã cập nhật vào NVS
    save_data_to_nvs(data_array, time_array);
}

// Hàm xóa toàn bộ dữ liệu trong NVS
void clear_data()
{
    uint16_t data_array[ARRAY_SIZE] = {0};
    int64_t time_array[ARRAY_SIZE] = {0};

    // Lưu lại các mảng rỗng vào NVS
    save_data_to_nvs(data_array, time_array);

    printf("Data and time arrays have been cleared\n");
}

// Hàm tạo JSON với "data" và "date"
char *create_json_response()
{
    uint16_t data_array[ARRAY_SIZE];
    int64_t time_array[ARRAY_SIZE];
    load_data(data_array, time_array); // Tải dữ liệu từ NVS

    cJSON *root = cJSON_CreateObject();

    // Tạo mảng cho "data" (cJSON không hỗ trợ uint16_t trực tiếp, nên thêm từng phần tử vào mảng JSON)
    cJSON *data = cJSON_CreateArray();
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        cJSON_AddItemToArray(data, cJSON_CreateNumber(data_array[i]));
    }
    cJSON_AddItemToObject(root, "data", data);

    // Tạo mảng cho "date"
    cJSON *date = cJSON_CreateArray();
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        char time_str[64];
        struct tm timeinfo;
        localtime_r(&time_array[i], &timeinfo);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo); // Định dạng thời gian
        cJSON_AddItemToArray(date, cJSON_CreateString(time_str));
    }
    cJSON_AddItemToObject(root, "date", date);

    // Chuyển JSON object thành chuỗi
    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);

    return json_string; // Trả về chuỗi JSON (đừng quên giải phóng bộ nhớ sau khi dùng)
}

esp_err_t save_wifi_config(const char *ssid, const char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Mở NVS với tên namespace là "storage"
    err = nvs_open("WIFI", NVS_READWRITE, &nvs_handle);
    // Lưu SSID
    err = nvs_set_str(nvs_handle, "wifi_ssid", ssid);
    if (err != ESP_OK)
    {
        printf("Failed to save SSID to NVS!\n");
        nvs_close(nvs_handle);
        return err;
    }

    // Lưu PASSWORD
    err = nvs_set_str(nvs_handle, "wifi_password", password);
    if (err != ESP_OK)
    {
        printf("Failed to save PASSWORD to NVS! \n");
        nvs_close(nvs_handle);
        return err;
    }

    // Ghi vào NVS
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        printf("Failed to commit data to NVS!\n");
    }

    nvs_close(nvs_handle); // Đóng NVS handle
    printf("Wi-Fi credentials saved successfully.\n");
    return ESP_OK;
}

esp_err_t load_wifi_config(char *ssid, char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Xác định kích thước mảng ssid và password trước khi truyền vào
    size_t ssid_len = 100;     // Kích thước tối đa của mảng ssid
    size_t password_len = 100; // Kích thước tối đa của mảng password

    // Mở NVS với tên namespace là "WIFI"
    err = nvs_open("WIFI", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return err;
    }

    // Load SSID từ NVS
    err = nvs_get_str(nvs_handle, "wifi_ssid", ssid, &ssid_len);
    if (err != ESP_OK)
    {
        printf("Failed to load SSID from NVS!\n");
        nvs_close(nvs_handle);
        return err;
    }

    // Load PASSWORD từ NVS
    err = nvs_get_str(nvs_handle, "wifi_password", password, &password_len);
    if (err != ESP_OK)
    {
        printf("Failed to load PASSWORD from NVS!\n");
        nvs_close(nvs_handle);
        return err;
    }

    nvs_close(nvs_handle); // Đóng NVS handle
    printf("Wi-Fi credentials loaded successfully.\n");
    printf("SSID: %s, PASSWORD: %s\n", ssid, password);
    return ESP_OK;
}
