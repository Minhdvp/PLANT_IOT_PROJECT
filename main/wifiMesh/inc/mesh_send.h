#ifndef MESH_SEND_H
#define MESH_SEND_H

#include "esp_mesh.h"
#include "read_sensor.h"

// Buffer size for JSON communication
#define JSON_BUFFER_SIZE 1024

// Function declarations
esp_err_t send_json_to_root(PotState_t *pot);

void task_mesh_rx_json(void *pvParameter);
void process_received_json(char *json_string);

#endif // MESH_SEND_H