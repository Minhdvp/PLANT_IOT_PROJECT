#ifndef MESH_HANDLER_H
#define MESH_HANDLER_H

#include "esp_mesh.h"

// Mesh network configuration
#define MESH_CHANNEL 6
#define CONFIG_MESH_CHANNEL 0
#define CONFIG_MESH_MAX_LAYER 3
#define CONFIG_MESH_AP_AUTHMODE WIFI_AUTH_WPA2_PSK
#define CONFIG_MESH_AP_CONNECTIONS 6
#define CONFIG_MESH_ROUTE_TABLE_SIZE 50
#define CONFIG_MESH_AP_PASSWD "12345678"

// Buffer sizes for mesh communication
#define RX_SIZE (1500)
#define TX_SIZE (1460)

// External variables
extern int mesh_layer;
extern char mesh_root_addr[20];
extern bool is_mesh_root;

// Function declarations
void mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void check_system_memory(void);
void mesh_reconnect(void);
void mesh_app_start(void);

#endif // MESH_HANDLER_H