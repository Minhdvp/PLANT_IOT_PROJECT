#ifndef MESH_SEND_H
#define MESH_SEND_H

#include "esp_mesh.h"
#include "read_sensor.h"

// Buffer size for JSON communication

// Function declarations
esp_err_t send_json_to_root(PotState_t *pot);

void task_mesh_rx_json(void *pvParameter);
void process_received_json(char *json_string);

#endif // MESH_SEND_H