#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "lwip/netdb.h"

#include "wifi_handler.h"
#include "nvs_handle.h"
#include "mesh_handler.h"

#include "espNow_handler.h"

#include "http_server.h"
static const char *TAG_WIFI = "Wifi_Handler";

// #define SSID "S20 FE"
// #define PASS "25102004"

// #define SSID "CEEC_Tenda"
// #define PASS "1denmuoi1"

esp_netif_t *sta_netif = NULL;

esp_netif_t *esp_netif_ap = NULL;

EventGroupHandle_t s_wifi_event_group;

uint8_t retryTime = 0;
#define MAX_RETRY_TIMES 4

bool is_wifi_connect;

// Thêm biến cờ để theo dõi trạng thái SoftAP
bool softap_active = false;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG_WIFI, "Wifi Start");
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG_WIFI, "Disconnected from AP, retry: %d/%d", retryTime, MAX_RETRY_TIMES);

        if (retryTime++ < MAX_RETRY_TIMES)
        {
            esp_wifi_connect();
            ESP_LOGI(TAG_WIFI, "Retrying to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
#if IS_ROOT == 1
            if (!softap_active)
            {
                ESP_LOGI(TAG_WIFI, "Failed to connect to AP, starting SoftAP mode");
                softap_active = true; // Đánh dấu rằng SoftAP đã được kích hoạt
                ESP_ERROR_CHECK(esp_wifi_stop());
                esp_wifi_set_mode(WIFI_MODE_APSTA);
                wifi_init_softap(); // Khởi tạo SoftAP
                esp_wifi_start();   // Bắt đầu WiFi
            }
            else
            {
                ESP_LOGI(TAG_WIFI, "SoftAP mode is already active, no need to start again");
            }
#endif
        }
        break;

    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG_WIFI, "Connected to AP");
        break;

    case IP_EVENT_STA_GOT_IP:
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_WIFI, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        is_wifi_connect = true;
        retryTime = 0; // Reset biến đếm retry
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        // Đã kết nối WiFi thành công, tắt SoftAP nếu đang bật
        if (softap_active)
        {
            ESP_LOGI(TAG_WIFI, "WiFi connected successfully, disabling SoftAP mode");

            // ESP_ERROR_CHECK(esp_wifi_stop()); // Dừng WiFi trước
            // ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            // ESP_ERROR_CHECK(esp_wifi_start()); // Khởi động lại với chế độ mới

            softap_active = false;
        }
        break;

    case WIFI_EVENT_AP_STACONNECTED:
        wifi_event_ap_staconnected_t *ap_event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG_WIFI, "Station " MACSTR " joined, AID=%d", MAC2STR(ap_event->mac), ap_event->aid);

        static bool http_server_started = false;
        if (!http_server_started && softap_active)
        {
            http_server_configure();
            http_server_started = true;
        }

        break;

    case WIFI_EVENT_AP_STADISCONNECTED:
        wifi_event_ap_stadisconnected_t *ap_event_disc = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG_WIFI, "Station " MACSTR " left, AID=%d", MAC2STR(ap_event_disc->mac), ap_event_disc->aid);
        break;

    default:
        break;
    }
}

void wifi_init_sta()
{
    // Tạo mạng STA
    sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };
    char ssid[100];
    char password[100];
    if (load_wifi_config(ssid, password) == ESP_OK)
    {
        printf("Loaded SSID: %s\n", ssid);
        printf("Loaded Password: %s\n", password);
    }
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0'; // Đảm bảo kết thúc chuỗi

    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0'; // Đảm bảo kết thúc chuỗi

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Đăng ký các sự kiện cho WiFi và IP
}

void wifi_init_softap(void)
{

    esp_netif_ap = esp_netif_create_default_wifi_ap();
    assert(esp_netif_ap);

    // SoftAP - WiFi access point configuration
    wifi_config_t ap_config =
        {
            .ap = {
                .ssid = WIFI_AP_SSID,
                .ssid_len = strlen(WIFI_AP_SSID),
                .password = WIFI_AP_PASSWORD,
                .channel = WIFI_AP_CHANNEL,
                .ssid_hidden = WIFI_AP_SSID_HIDDEN,
                .authmode = WIFI_AUTH_WPA2_PSK,
                .max_connection = WIFI_AP_MAX_CONNECTIONS,
                .beacon_interval = WIFI_AP_BEACON_INTERVAL,
            },
        };

    // Configure DHCP for the AP
    esp_netif_ip_info_t ap_ip_info;
    memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));

    esp_netif_dhcps_stop(esp_netif_ap);             ///> must call this first
    inet_pton(AF_INET, WIFI_AP_IP, &ap_ip_info.ip); ///> Assign access point's static IP, GW, and netmask
    inet_pton(AF_INET, WIFI_AP_GATEWAY, &ap_ip_info.gw);
    inet_pton(AF_INET, WIFI_AP_NETMASK, &ap_ip_info.netmask);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info)); ///> Statically configure the network interface
    ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap));              ///> Start the AP DHCP server (for connecting stations e.g. your mobile device)

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));       ///> Set our configuration
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_AP_BANDWIDTH)); ///> Our default bandwidth 20 MHz
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_STA_POWER_SAVE));                  ///> Power save set to "NONE"
}

// Sửa hàm wifi_start để chỉ bắt đầu với STA mode
void wifi_start()
{
    // Thiết lập chế độ STA cho WiFi ban đầu
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // Khởi tạo WiFi một lần

    // Bắt đầu với chế độ STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Khởi tạo STA
    wifi_init_sta();

    // Khởi tạo softap sẽ được gọi sau nếu cần thiết trong event handler

    // Đăng ký các event handler
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // Bắt đầu WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    // EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
    //                                        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
    //                                        pdFALSE,
    //                                        pdFALSE,
    //                                        portMAX_DELAY);

    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    // vEventGroupDelete(s_wifi_event_group);
}
