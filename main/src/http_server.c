#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stdbool.h"

#include "esp_http_server.h"
#include "esp_err.h"
#include "esp_log.h"
#include "cJSON.h"
#include "mdns.h"

#include "http_server.h"

#include "nvs_handle.h"

#include "wifi_handler.h"

static const char *TAG = "HTTP_HANDLE";

extern const unsigned char wificonfig_start[] asm("_binary_wificonfig_html_start");
extern const unsigned char wificonfig_end[] asm("_binary_wificonfig_html_end");

static httpd_handle_t http_server_handle = NULL;

static esp_err_t http_server_wificonfig_html_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "wificonfig.html requested");

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)wificonfig_start, wificonfig_end - wificonfig_start);

    return ESP_OK;
}

esp_err_t connect_post_handler(httpd_req_t *req)
{
    // Đọc dữ liệu JSON từ request
    char content[100];
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(content))
    {
        httpd_resp_send_500(req); // Quá dung lượng bộ đệm
        return ESP_FAIL;
    }

    // Nhận dữ liệu JSON từ yêu cầu POST
    ret = httpd_req_recv(req, content, remaining);
    if (ret <= 0)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0'; // Thêm null-terminator

    // In ra chuỗi JSON nhận được
    ESP_LOGI(TAG, "Received JSON: %s", content);

    // Phân tích cú pháp JSON
    cJSON *json = cJSON_Parse(content);
    if (json == NULL)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Lấy dữ liệu "ssid" và "password" từ JSON
    const cJSON *ssid = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    const cJSON *password = cJSON_GetObjectItemCaseSensitive(json, "password");

    if (cJSON_IsString(ssid) && cJSON_IsString(password))
    {
        ESP_LOGI(TAG, "Received SSID: %s", ssid->valuestring);
        ESP_LOGI(TAG, "Received Password: %s", password->valuestring);
        if (save_wifi_config(ssid->valuestring, password->valuestring) == ESP_OK)
        {
            ESP_LOGI(TAG, "Wi-Fi save successfully.");
            esp_restart();
        }
        else
        {
            ESP_LOGE(TAG, "Failed to connect Wi-Fi.");
            httpd_resp_sendstr(req, "Connection failed");
        }
    }

    cJSON_Delete(json); // Giải phóng bộ nhớ JSON
    return ESP_OK;
}

void start_mdns_service()
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("connect_wifi"));
    ESP_LOGI(TAG, "mDNS hostname set to: connect_wifi");

    ESP_ERROR_CHECK(mdns_instance_name_set("Set WIFI"));
    ESP_ERROR_CHECK(mdns_service_add("Web Server", "_http", "_tcp", 80, NULL, 0));

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"project", "Set Wifi"},
    };
    ESP_ERROR_CHECK(mdns_service_txt_set("_http", "_tcp", serviceTxtData, 2));
}

esp_err_t http_server_configure(void)
{
    // Generate the default configuration
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // The core that the HTTP server will run on
    config.core_id = HTTP_SERVER_TASK_CORE_ID;

    // Adjust the default priority to 1 less than the wifi application task
    config.task_priority = HTTP_SERVER_TASK_PRIORITY;

    // Bump up the stack size (default is 4096)
    config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;

    // Increase uri handlers
    config.max_uri_handlers = 20;

    // Increase the timeout limits
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;

    ESP_LOGI(TAG,
             "http_server_configure: Starting server on port: '%d' with task priority: '%d'",
             config.server_port,
             config.task_priority);

    // Start the httpd server
    if (httpd_start(&http_server_handle, &config) == ESP_OK)
    {
        ESP_LOGI(TAG, "http_server_configure: Registering URI handlers");

        // register wificonfig.html handler
        httpd_uri_t wificonfig_html = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = http_server_wificonfig_html_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(http_server_handle, &wificonfig_html);

        // register connect handler
        httpd_uri_t connect_uri = {
            .uri = "/connect",
            .method = HTTP_POST,
            .handler = connect_post_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(http_server_handle, &connect_uri);
    }
    start_mdns_service();
    return ESP_OK;
}