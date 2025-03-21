
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

static const char *TAG_WIFI = "Wifi_Handler";

// #define SSID "S20 FE"
// #define PASS "25102004"

// #define SSID "CEEC_Tenda"
// #define PASS "1denmuoi1"

esp_netif_t *sta_netif = NULL;

esp_netif_t *esp_netif_ap = NULL;
EventGroupHandle_t s_wifi_event_group;

uint8_t retryTime = 0;
#define MAX_RETRY_TIMES 10

bool is_wifi_connect;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG_WIFI, "Wifi Start");
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        if (retryTime++ < MAX_RETRY_TIMES)
            esp_wifi_connect();
        ESP_LOGI(TAG_WIFI, "Retrying to connect to the AP");
        break;
    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG_WIFI, "Connected to AP");
        break;
    case IP_EVENT_STA_GOT_IP:
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_WIFI, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        is_wifi_connect = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
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

void wifi_start()
{
    // Thiết lập chế độ hỗn hợp (APSTA) cho WiFi
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // Khởi tạo WiFi một lần

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    wifi_init_sta();    // Cấu hình chế độ STA
    wifi_init_softap(); // Cấu hình chế độ SoftAP
    vTaskDelay(1000 / portTICK_PERIOD_MS);

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
    /*
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               portMAX_DELAY);

        if (bits & WIFI_CONNECTED_BIT)
        {
            ESP_LOGI(TAG_WIFI, "connected to ap SSID:%s PASS:%s",
                     WIFI_SSID, WIFI_PASS);
        }
        else if (bits & WIFI_FAIL_BIT)
        {
            ESP_LOGI(TAG_WIFI, "Failed to connect to SSID:%s, PASS:%s",
                     WIFI_SSID, WIFI_PASS);
        }
        else
        {
            ESP_LOGE(TAG_WIFI, "UNEXPECTED EVENT");
        }
    */
}
