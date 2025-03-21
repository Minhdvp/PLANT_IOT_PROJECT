#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "http_server.h"
#include "nvs_handle.h"
#include "wifi_handler.h"

void app_main(void)
{
    nvs_init();
    wifi_start();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    http_server_configure();
}
