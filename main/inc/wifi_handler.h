#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_DISCONNECTED_BIT BIT2

#define WIFI_AP_SSID "ESP32_AP"          // AP name
#define WIFI_AP_PASSWORD "25102004"      // AP password
#define WIFI_AP_CHANNEL 1                // AP channel
#define WIFI_AP_SSID_HIDDEN 0            // AP visibility
#define WIFI_AP_MAX_CONNECTIONS 5        // AP max clients
#define WIFI_AP_BEACON_INTERVAL 100      // AP beacon: 100 milliseconds recommended
#define WIFI_AP_IP "192.168.4.1"         // AP default IP
#define WIFI_AP_GATEWAY "192.168.0.1"    // AP default Gateway (should be the same as the IP)
#define WIFI_AP_NETMASK "255.255.255.0"  // AP netmask
#define WIFI_AP_BANDWIDTH WIFI_BW_HT20   // AP bandwidth 20 MHz (40 MHz is the other option)
#define WIFI_STA_POWER_SAVE WIFI_PS_NONE // Power save not used
#define MAX_SSID_LENGTH 32               // IEEE standard maximum
#define MAX_PASSWORD_LENGTH 64           // IEEE standard maximum
#define MAX_CONNECTION_RETRIES 5         // Retry number on disconnect

#define DEFAULT_SCAN_LIST_SIZE 5

void wifi_init_softap();

void wifi_init_sta();

void wifi_start();

#endif