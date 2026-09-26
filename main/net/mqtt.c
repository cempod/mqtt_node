#include "mqtt.h"
#include "esp_log.h"
#include "esp_system.h"
#include <string.h>
#include "device_id.h"
#include "ha_discovery.h"

static const char *TAG = "mqtt";

extern const uint8_t client_cert_pem_start[] asm("_binary_client_crt_start");
extern const uint8_t client_cert_pem_end[]   asm("_binary_client_crt_end");
extern const uint8_t client_key_pem_start[]  asm("_binary_client_key_start");
extern const uint8_t client_key_pem_end[]    asm("_binary_client_key_end");
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_crt_start");
extern const uint8_t server_cert_pem_end[]   asm("_binary_ca_crt_end");

bool is_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to broker");
            is_connected = true;
            
            const char *node = device_id_get();

            ha_device_t dev = {
                .name       = "Sensor",
                .model      = "ESP32-AHT20",
                .sw_version = "1.0.0",
            };

            ha_add_sensor(client, &dev, node, "temperature", "Температура", "°C", "temperature");
            ha_add_sensor(client, &dev, node, "humidity",    "Влажность",    "%",  "humidity");
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Disconnected, trying to reconnect...");
            is_connected = false;
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_mqtt_client_reconnect(client);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "Error event");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "TCP/TLS error: esp_tls_last_esp_err=0x%x, stack_err=0x%x, sock_errno=%d",
                         event->error_handle->esp_tls_last_esp_err,
                         event->error_handle->esp_tls_stack_err,
                         event->error_handle->esp_transport_sock_errno);
            }
            break;

        default:
            break;
    }
}

esp_err_t mqtt_init(esp_mqtt_client_handle_t *client) {
    const char *node_name = device_id_get();

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = CONFIG_MQTT_BROKER_URI,
            .verification.certificate = (const char *)server_cert_pem_start,
        },
        .credentials = {
            .authentication = {
                .certificate = (const char *)client_cert_pem_start,
                .key = (const char *)client_key_pem_start,
            },
        },
        .credentials.client_id = node_name,
        .session.keepalive = 60,
    };

    *client = esp_mqtt_client_init(&mqtt_cfg);
    if (*client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_ERR_NO_MEM;
    }

    esp_mqtt_client_register_event(*client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_err_t err = esp_mqtt_client_start(*client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: 0x%x", err);
        return err;
    }

    ESP_LOGI(TAG, "MQTT client started");
    return ESP_OK;
}

bool mqtt_is_connected(void) {
    return is_connected;
}