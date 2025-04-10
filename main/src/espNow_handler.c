#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"

#include "wifi_handler.h"
#include "esp_mac.h"

#include "esp_now.h"
#include "nvs_handle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "espNow_handler.h"
#define ELECTION_TIME_MS 5000
#define MAX_PEERS 20
#define MAC_STR_SIZE 13 // 12 hex chars for MAC + null terminator

static uint8_t my_mac[6];
static bool isSoftAPStarted = false;

// Sử dụng mảng đơn giản thay vì struct phức tạp
static char other_ids[MAX_PEERS][MAC_STR_SIZE];
static int peer_count = 0;

static const char *TAG = "ESP-NOW Election";

extern bool softap_active;

// ESP-NOW receive callback
void on_data_recv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    if (peer_count >= MAX_PEERS)
    {
        ESP_LOGW(TAG, "Maximum peers reached, ignoring new peer");
        return;
    }

    if (len >= MAC_STR_SIZE)
    {
        ESP_LOGE(TAG, "Received data too long");
        return;
    }

    memcpy(other_ids[peer_count], incomingData, len);
    other_ids[peer_count][len] = '\0';
    ESP_LOGI(TAG, "Received ID: %s", other_ids[peer_count]);
    peer_count++;
}

// ESP-NOW send callback
void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status != ESP_NOW_SEND_SUCCESS)
    {
        ESP_LOGW(TAG, "Failed to send ESP-NOW data");
    }
}

// Function to send broadcast message with MAC
void send_election_broadcast()
{
    char my_id[MAC_STR_SIZE];
    sprintf(my_id, "%02X%02X%02X%02X%02X%02X", my_mac[0], my_mac[1], my_mac[2], my_mac[3], my_mac[4], my_mac[5]);

    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)my_id, strlen(my_id));

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Error sending ESP-NOW message: %s", esp_err_to_name(result));
    }
}

// Function to check if the node's MAC is the lowest
bool is_my_id_lowest()
{
    char my_id[MAC_STR_SIZE];
    sprintf(my_id, "%02X%02X%02X%02X%02X%02X", my_mac[0], my_mac[1], my_mac[2], my_mac[3], my_mac[4], my_mac[5]);

    ESP_LOGI(TAG, "My ID: %s", my_id);
    ESP_LOGI(TAG, "Found %d peers during election", peer_count);

    for (int i = 0; i < peer_count; i++)
    {
        ESP_LOGI(TAG, "Comparing with peer: %s", other_ids[i]);
        if (strcmp(other_ids[i], my_id) < 0)
        {
            return false;
        }
    }
    return true;
}

void Init_espNow()
{
    // Get MAC address
    ESP_ERROR_CHECK(esp_read_mac(my_mac, ESP_MAC_WIFI_STA));

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "ESP-NOW initialization failed");
        return;
    }

    // Register callbacks
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_data_recv));
    ESP_ERROR_CHECK(esp_now_register_send_cb(on_data_sent));

    // Add broadcast peer
    esp_now_peer_info_t peerInfo = {0};
    memcpy(peerInfo.peer_addr, (uint8_t[]){0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));

    ESP_LOGI(TAG, "Election process started for %d seconds...", ELECTION_TIME_MS / 1000);
    int64_t start = esp_timer_get_time();

    while (esp_timer_get_time() - start < ELECTION_TIME_MS * 1000LL)
    {
        send_election_broadcast();
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    if (is_my_id_lowest())
    {
        if (!softap_active)
        {
            // Chuyển sang chế độ APSTA
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

            wifi_init_softap();

            softap_active = true;
        }
    }
    else
    {
        ESP_LOGI(TAG, "I am not the node with the lowest ID -> Waiting for configuration");
    }
}