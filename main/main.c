#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_handle.h"
#include "wifi_handler.h"
#include "sys_config.h"
#include "read_sensor.h"

#include "mesh_handler.h"

void app_main(void)
{
    nvs_init();

    wifi_start();

    mesh_app_start();
}
