#include "device_id.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "esp_mac.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "device_id";

#define MAC_STR_LEN      18
#define DEVICE_ID_MAX    64

#define PREFIX_MAX_ALLOWED  (DEVICE_ID_MAX - 1 - (MAC_STR_LEN - 1))

_Static_assert(sizeof(CONFIG_MQTT_NODE_NAME) - 1 <= PREFIX_MAX_ALLOWED,
               "CONFIG_MQTT_NODE_NAME too long! "
               "Shorten it in menuconfig or increase DEVICE_ID_MAX in device_id.c");

static char  s_mac_str[MAC_STR_LEN]     = {0};
static char  s_device_id[DEVICE_ID_MAX] = {0};
static bool  s_ready                    = false;

static void build_ids_once(void)
{
    if (s_ready) return;

    uint8_t mac[6] = {0};
    esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_read_mac failed: %s, using zero-MAC fallback",
                 esp_err_to_name(err));
        snprintf(s_mac_str, sizeof(s_mac_str), "00-00-00-00-00-00");
    } else {
        snprintf(s_mac_str, sizeof(s_mac_str),
                 "%02X-%02X-%02X-%02X-%02X-%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    int n = snprintf(s_device_id, sizeof(s_device_id),
                     "%s-%s", CONFIG_MQTT_NODE_NAME, s_mac_str);

    if (n < 0 || n >= (int)sizeof(s_device_id)) {
        ESP_LOGW(TAG, "device_id truncated (need %d, have %zu). "
                      "Falling back to MAC-only.",
                 n, sizeof(s_device_id));
        snprintf(s_device_id, sizeof(s_device_id), "%s", s_mac_str);
    }

    s_ready = true;
    ESP_LOGI(TAG, "Device ID: %s", s_device_id);
}

const char *device_id_get(void)
{
    build_ids_once();
    return s_device_id;
}

const char *device_id_get_mac(void)
{
    build_ids_once();
    return s_mac_str;
}