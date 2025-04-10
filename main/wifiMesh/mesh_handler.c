/**
 * C Library
 */
#include <string.h>
/**
 * ESP-IDF
 */
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_mesh.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mesh_internal.h"
#include "cJSON.h"
#include "esp_event.h"
/**
 * FreeRTOS
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * Custom Library
 */
#include "esp_heap_caps.h"
#include "wifi_handler.h"
#include "sys_config.h"
#include "nvs_handle.h"
#include "mesh_handler.h"
#include "mesh_send.h"
#include "read_sensor.h"

static const char *TAG = "Mesh_Handler";
static const uint8_t MESH_ID[6] = MESH_ID_DEFINE;

extern esp_netif_t *sta_netif;

int mesh_layer = -1;
char mesh_root_addr[20];

static mesh_addr_t mesh_parent_addr;

bool is_mesh_root = false;

mesh_addr_t route_table[CONFIG_MESH_ROUTE_TABLE_SIZE];

// Hàm xử lý sự kiện mạng Mesh
void mesh_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    mesh_addr_t id = {0};
    static uint16_t last_layer = 0;

    switch (event_id)
    {
    case MESH_EVENT_STARTED:
    {
        esp_mesh_get_id(&id);
        ESP_LOGI(TAG, "<MESH_EVENT_MESH_STARTED>ID:" MACSTR "", MAC2STR(id.addr));
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_STOPPED:
    {
        ESP_LOGI(TAG, "<MESH_EVENT_STOPPED>");
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_CHILD_CONNECTED:
    {
        mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
        ESP_LOGI(TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, " MACSTR "",
                 child_connected->aid,
                 MAC2STR(child_connected->mac));
    }
    break;
    case MESH_EVENT_CHILD_DISCONNECTED:
    {
        mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
        ESP_LOGI(TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, " MACSTR "",
                 child_disconnected->aid,
                 MAC2STR(child_disconnected->mac));
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_ADD:
    {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d, layer:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new, mesh_layer);
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE:
    {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d, layer:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new, mesh_layer);
    }
    break;
    case MESH_EVENT_NO_PARENT_FOUND:
    {
        mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
        ESP_LOGI(TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                 no_parent->scan_times);
    }
    break;
    case MESH_EVENT_PARENT_CONNECTED:
    {
        mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
        esp_mesh_get_id(&id);
        mesh_layer = connected->self_layer;
        memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);

        is_mesh_root = esp_mesh_is_root();

        ESP_LOGI(TAG,
                 "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:" MACSTR "%s, ID:" MACSTR "",
                 last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                 is_mesh_root ? "<ROOT>" : (mesh_layer == 2) ? "<layer2>"
                                                             : "",
                 MAC2STR(id.addr));

        last_layer = mesh_layer;

        if (is_mesh_root)
        {
            // Root node: Start DHCP client and task to receive JSON from nodes
            ESP_LOGI(TAG, "This node is ROOT, starting JSON receiver task");
            esp_netif_dhcpc_stop(sta_netif);
            esp_netif_dhcpc_start(sta_netif);

            // Start JSON receiver task for root node
            xTaskCreate(task_mesh_rx_json, "task_mesh_rx_json", 4096, NULL, 5, NULL);
        }
        else
        {
            // Non-root node: Just log the connection
            ESP_LOGI(TAG, "This node is NOT ROOT, will send data to root");

            // send_json_to_root();
        }
    }
    break;
    case MESH_EVENT_ROOT_ADDRESS:
    {
        mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
        sprintf(mesh_root_addr, MACSTR, MAC2STR(root_addr->addr));
        ESP_LOGI(TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:" MACSTR "",
                 MAC2STR(root_addr->addr));
    }
    break;
    case MESH_EVENT_ROOT_SWITCH_REQ:
    {
        mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
        ESP_LOGI(TAG,
                 "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:" MACSTR "",
                 switch_req->reason,
                 MAC2STR(switch_req->rc_addr.addr));
    }
    break;
    case MESH_EVENT_ROOT_FIXED:
    {
        mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
        ESP_LOGI(TAG, "<MESH_EVENT_ROOT_FIXED>%s",
                 root_fixed->is_fixed ? "fixed" : "not fixed");
    }
    break;
    case MESH_EVENT_SCAN_DONE:
    {
        mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
        ESP_LOGI(TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
                 scan_done->number);
    }
    break;
    case MESH_EVENT_FIND_NETWORK:
    {
        mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
        ESP_LOGI(TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:" MACSTR "",
                 find_network->channel, MAC2STR(find_network->router_bssid));
    }
    break;
    case MESH_EVENT_PS_PARENT_DUTY:
    {
        mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
        ESP_LOGI(TAG, "<MESH_EVENT_PS_PARENT_DUTY>duty:%d", ps_duty->duty);
    }
    break;
    default:
        ESP_LOGI(TAG, "unknown id:%" PRId32 "", event_id);
        break;
    }
}

void check_system_memory()
{
    printf("Free heap: %u bytes\n", (unsigned int)esp_get_free_heap_size());
    printf("Free internal RAM: %u bytes\n", (unsigned int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    printf("Free SPIRAM: %u bytes\n", (unsigned int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    printf("Remaining stack size: %u bytes\n", (unsigned int)(uxTaskGetStackHighWaterMark(NULL) * 4));
}

void mesh_app_start()
{
    check_system_memory();

    // Configure ESP-MESH network
    if (sta_netif != NULL)
    {
        ESP_ERROR_CHECK(esp_netif_dhcpc_stop(sta_netif));
    }
    else
    {
        ESP_LOGW(TAG, "sta_netif is NULL, skipping DHCP client stop");
    }
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));

    // Configure mesh network topology
    ESP_ERROR_CHECK(esp_mesh_set_topology(MESH_TOPO_TREE));
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(3));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_xon_qsize(2048));
    ESP_ERROR_CHECK(esp_mesh_disable_ps());
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));

    esp_mesh_set_self_organized(true, true);
    // Configure mesh network settings
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    memcpy((uint8_t *)&cfg.mesh_id, MESH_ID, 6);
    cfg.channel = CONFIG_MESH_CHANNEL;

    // Đọc thông tin WiFi từ NVS
    char ssid[100];
    char password[100];
    esp_err_t ret = load_wifi_config(ssid, password);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to load WiFi credentials from NVS, using defaults");
        // Có thể đặt giá trị mặc định hoặc trả về lỗi tùy thuộc vào yêu cầu của bạn
    }
    else
    {
        ESP_LOGI(TAG, "Using WiFi credentials from NVS: SSID=%s", ssid);
    }

    // Configure router with NVS credentials
    cfg.router.ssid_len = strlen(ssid);
    memcpy((uint8_t *)&cfg.router.ssid, ssid, cfg.router.ssid_len);
    memcpy((uint8_t *)&cfg.router.password, password, strlen(password));

    // Configure mesh AP
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(WIFI_AUTH_WPA2_PSK));
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    cfg.mesh_ap.nonmesh_max_connection = 1;
    memcpy((uint8_t *)&cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD, strlen(CONFIG_MESH_AP_PASSWD));

    // Configure root node if needed
#if IS_ROOT == 1
    ESP_LOGI(TAG, "Configuring this node as ROOT");
    esp_mesh_set_type(MESH_ROOT);
    esp_mesh_fix_root(true);
#endif

    // Start mesh network
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    esp_err_t err = esp_mesh_start();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_mesh_start failed: %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "Mesh started successfully, heap:%" PRId32 ", %s<%d>%s, ps:%d",
             esp_get_minimum_free_heap_size(),
             esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed",
             esp_mesh_get_topology(),
             esp_mesh_get_topology() ? "(chain)" : "(tree)",
             esp_mesh_is_ps_enabled());
}