#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_handle.h"
#include "wifi_handler.h"
#include "sys_config.h"
#include "read_sensor.h"

#include "mesh_handler.h"

extern EventGroupHandle_t s_wifi_event_group;

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
    disible_event();
#else
    init_sensors();
#endif
    mesh_app_start();
}
