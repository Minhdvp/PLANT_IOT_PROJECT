#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_handle.h"
#include "wifi_handler.h"
#include "sys_config.h"
#include "read_sensor.h"

#include "mesh_handler.h"
#include "esp_sntp.h"
#include "esp_log.h"

extern EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "MAIN";

void init_sntp(void) {
    ESP_LOGI(TAG, "Khởi tạo SNTP");
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    // Đặt múi giờ Việt Nam (ICT, UTC+7)
    setenv("TZ", "ICT-7", 1); // ICT-7: Asia/Ho_Chi_Minh
    tzset();

    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retry < retry_count) {
        ESP_LOGI(TAG, "Đang chờ đồng bộ thời gian... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        retry++;
    }
    if (retry < retry_count) {
        ESP_LOGI(TAG, "Đồng bộ thời gian thành công");
    } else {
        ESP_LOGE(TAG, "Không thể đồng bộ thời gian");
    }
}


void app_main(void)
{
    nvs_init();

    wifi_start();
#if IS_ROOT == 1
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    
    init_sntp();
    disible_event();
#else
    init_sensors();
#endif
    mesh_app_start();
}
