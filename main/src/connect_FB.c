#include "connect_FB.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include <string.h>
#include "esp_wifi.h"

static const char *TAG = "FIREBASE";
static char firebase_api_key[128] = "AIzaSyALwE3UNOpbOJs4kJEKrGgdssnDJtg0k1M";
static char firebase_database_url[256] = "https://plantproject-b3b1b-default-rtdb.asia-southeast1.firebasedatabase.app/";

esp_err_t firebase_init(const char *api_key, const char *database_url) {
    if (api_key != NULL) {
        strncpy(firebase_api_key, api_key, sizeof(firebase_api_key) - 1);
    }
    if (database_url != NULL) {
        strncpy(firebase_database_url, database_url, sizeof(firebase_database_url) - 1);
    }

    ESP_LOGI(TAG, "Firebase initialized with API Key: %s", firebase_api_key);
    return ESP_OK;
}


esp_err_t firebase_write(const char *path, const char *data) {
    // Kiểm tra kết nối WiFi
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        ESP_LOGE(TAG, "WiFi chưa kết nối!");
        return ESP_FAIL;
    }

    char url[512];
    snprintf(url, sizeof(url), "%s%s.json?auth=%s", firebase_database_url, path, firebase_api_key);

    // Lấy chứng chỉ SSL của Firebase
    extern const uint8_t firebase_cert_pem_start[] asm("_binary_firebase_cert_pem_start");
    extern const uint8_t firebase_cert_pem_end[] asm("_binary_firebase_cert_pem_end");

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_PUT,
        .cert_pem = (const char *)firebase_cert_pem_start,
        .timeout_ms = 10000
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client!");
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");

    // Mở kết nối HTTP
    esp_err_t err = esp_http_client_open(client, strlen(data));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    // Ghi dữ liệu JSON lên Firebase
    int written_len = esp_http_client_write(client, data, strlen(data));
    if (written_len <= 0) {
        ESP_LOGE(TAG, "Failed to write data to Firebase");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Kiểm tra phản hồi từ Firebase
    err = esp_http_client_perform(client);
    if (err == ESP_OK && esp_http_client_get_status_code(client) == 200) {
        ESP_LOGI(TAG, "Data written to Firebase: %s -> %s", path, data);
    } else {
        ESP_LOGE(TAG, "Failed to write data! HTTP Status Code: %d, Error: %s",
                 esp_http_client_get_status_code(client), esp_err_to_name(err));
    }

    // Đóng kết nối HTTP
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    return err;
}

