/**
 * C Library
 */
#include <stdint.h>
#include <string.h>
#include "esp_timer.h"

/**
 * ESP-IDF
 */
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mac.h"
#include "cJSON.h"

/**
 * Custom Library
 */
#include "nvs_handle.h"
#include "sys_config.h"
#include "mesh_handler.h"
#include "mesh_send.h"
#include "connect_FB.h"

#include "esp_sntp.h"
#include "time.h"

static const char *TAG_SEND = "MESH_SEND";

// Buffer sizes for JSON communication
#define JSON_BUFFER_SIZE 1024

// Buffers for sending and receiving JSON
static uint8_t json_tx_buf[JSON_BUFFER_SIZE] = {0};
static uint8_t json_rx_buf[JSON_BUFFER_SIZE] = {0};

extern char mesh_root_addr[20];

esp_err_t send_json_to_root(PotState_t *pot)
{
    cJSON *root = cJSON_CreateObject();
    // cJSON_AddStringToObject(root, "device", "Pot_1");
    cJSON_AddNumberToObject(root, "Temperature", pot->temperature);
    cJSON_AddNumberToObject(root, "Humidity", pot->humidity);
    cJSON_AddNumberToObject(root, "Soil_moisture", pot->soil_moisture);
    cJSON_AddNumberToObject(root, "RainLevel", pot->rain_level);
    cJSON_AddBoolToObject(root, "Status", pot->pump_running);
    cJSON_AddBoolToObject(root, "Warning", pot->warning);
    char *json_string = cJSON_Print(root);

    // Kiểm tra nếu không thể tạo chuỗi JSON
    if (json_string == NULL)
    {
        ESP_LOGE(TAG_SEND, "Failed to create JSON string");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    if (strlen(json_string) >= JSON_BUFFER_SIZE)
    {
        ESP_LOGE(TAG_SEND, "JSON data too large for buffer");
        cJSON_free(json_string); // Giải phóng bộ nhớ
        cJSON_Delete(root);      // Giải phóng đối tượng JSON
        return ESP_ERR_NO_MEM;
    }

    mesh_data_t mesh_send;
    mesh_send.data = json_tx_buf;
    mesh_send.proto = MESH_PROTO_JSON;

    memset(json_tx_buf, 0, JSON_BUFFER_SIZE);
    memcpy(json_tx_buf, json_string, strlen(json_string));
    mesh_send.size = strlen(json_string) + 1;

    // Giải phóng chuỗi JSON sau khi đã sao chép vào buffer
    cJSON_free(json_string);

    // Giải phóng đối tượng JSON
    cJSON_Delete(root);

    ESP_LOGI(TAG_SEND, "Sending JSON to root: %s", (char *)mesh_send.data);

    // Phần còn lại của hàm giữ nguyên
    esp_err_t err = esp_mesh_send(NULL, &mesh_send, MESH_DATA_P2P, NULL, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_SEND, "Failed to send JSON to root: %s", esp_err_to_name(err));
    }
    else
    {
        ESP_LOGI(TAG_SEND, "Successfully sent JSON to root");
    }

    return err;
}

void task_mesh_rx_json(void *pvParameter)
{
    mesh_addr_t from;
    mesh_data_t mesh_recv;
    int flag = 0;

    mesh_recv.data = json_rx_buf;
    mesh_recv.size = JSON_BUFFER_SIZE;

    ESP_LOGI(TAG_SEND, "Mesh JSON receiver task started");

    while (1)
    {
        // Reset buffer size before receiving
        mesh_recv.size = JSON_BUFFER_SIZE;
        memset(json_rx_buf, 0, JSON_BUFFER_SIZE);

        // Wait for incoming data
        esp_err_t err = esp_mesh_recv(&from, &mesh_recv, portMAX_DELAY, &flag, NULL, 0);
        if (err != ESP_OK || mesh_recv.size == 0)
        {
            ESP_LOGE(TAG_SEND, "Failed to receive JSON: %s, size: %d",
                     esp_err_to_name(err), mesh_recv.size);
            continue;
        }

        // Ensure null termination
        json_rx_buf[mesh_recv.size - 1] = '\0';

        ESP_LOGI(TAG_SEND, "Received JSON from " MACSTR ": %s",
                 MAC2STR(from.addr), (char *)mesh_recv.data);

        // Process the received JSON
        process_received_json((char *)mesh_recv.data);
    }
}
/**
 * Hàm chổ này dùng để gửi chuyển Json lên Firebase
 */
void process_received_json(char *json_string) {
    // Kiểm tra JSON hợp lệ
    if (json_string == NULL || strlen(json_string) == 0) {
        ESP_LOGE(TAG_SEND, "Chuỗi JSON không hợp lệ hoặc rỗng");
        return;
    }

    // Phân tích JSON
    cJSON *json = cJSON_Parse(json_string);
    if (json == NULL) {
        ESP_LOGE(TAG_SEND, "Lỗi phân tích JSON: %s", cJSON_GetErrorPtr());
        return;
    }

    // Gửi JSON gốc lên /SensorData (không thêm Timestamp)
    char *new_json_string = cJSON_PrintUnformatted(json);
    if (new_json_string == NULL) {
        ESP_LOGE(TAG_SEND, "Lỗi tạo chuỗi JSON mới cho /SensorData");
        cJSON_Delete(json);
        return;
    }

    ESP_LOGI(TAG_SEND, "Gửi JSON lên /SensorData: %s", new_json_string);
    esp_err_t err = firebase_write("/SensorData", new_json_string);
    if (err == ESP_OK) {
        ESP_LOGI(TAG_SEND, "Gửi thành công lên /SensorData");
    } else {
        ESP_LOGE(TAG_SEND, "Lỗi gửi lên /SensorData: %s", esp_err_to_name(err));
    }

    // Kiểm tra trạng thái máy bơm
    cJSON *status = cJSON_GetObjectItem(json, "Status");
    if (status && cJSON_IsTrue(status)) {
        // Lấy thời gian thực
        time_t now;
        struct tm timeinfo;
        char timestamp[64];
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

        // Tạo JSON cho /Water_Pump
        cJSON *pump_json = cJSON_CreateObject();
        cJSON_AddStringToObject(pump_json, "PumpOnTime", timestamp);
        char *pump_json_string = cJSON_PrintUnformatted(pump_json);
        if (pump_json_string == NULL) {
            ESP_LOGE(TAG_SEND, "Lỗi tạo chuỗi JSON cho /Water_Pump");
            cJSON_Delete(pump_json);
            cJSON_free(new_json_string);
            cJSON_Delete(json);
            return;
        }

        // Gửi lên /Water_Pump
        ESP_LOGI(TAG_SEND, "Gửi JSON lên /Water_Pump: %s", pump_json_string);
        err = firebase_post("/Water_Pump", pump_json_string);
        if (err == ESP_OK) {
            ESP_LOGI(TAG_SEND, "Gửi thành công lên /Water_Pump");
        } else {
            ESP_LOGE(TAG_SEND, "Lỗi gửi lên /Water_Pump: %s", esp_err_to_name(err));
        }

        cJSON_free(pump_json_string);
        cJSON_Delete(pump_json);
    }

    cJSON_free(new_json_string);
    cJSON_Delete(json);
}