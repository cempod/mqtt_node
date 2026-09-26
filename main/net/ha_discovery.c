#include "ha_discovery.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"

static const char *TAG = "ha_disc";

void ha_state_topic(char *buf, size_t n, const char *node, const char *obj)
{
    snprintf(buf, n, "%s/%s/state", node, obj);
}

void ha_command_topic(char *buf, size_t n, const char *node, const char *obj)
{
    snprintf(buf, n, "%s/%s/set", node, obj);
}

static void publish_config(esp_mqtt_client_handle_t client,
                           const char *component,
                           const char *node,
                           const char *object_id,
                           const char *payload)
{
    char topic[192];
    snprintf(topic, sizeof(topic),
             "homeassistant/%s/%s/%s/config",
             component, node, object_id);

    esp_mqtt_client_publish(client, topic, payload, 0, 1, 1);
    ESP_LOGI(TAG, "→ %s", topic);
}

void ha_add_sensor(esp_mqtt_client_handle_t client,
                   const ha_device_t *dev,
                   const char *node,
                   const char *object_id,
                   const char *friendly_name,
                   const char *unit,
                   const char *device_class)
{
    char state[128];
    ha_state_topic(state, sizeof(state), node, object_id);

    char payload[512];
    int n = snprintf(payload, sizeof(payload),
        "{"
            "\"name\":\"%s\","
            "\"unique_id\":\"%s_%s\","
            "\"state_topic\":\"%s\"",
        friendly_name, node, object_id, state);

    if (unit)
        n += snprintf(payload + n, sizeof(payload) - n,
                      ",\"unit_of_measurement\":\"%s\"", unit);

    if (device_class)
        n += snprintf(payload + n, sizeof(payload) - n,
                      ",\"device_class\":\"%s\"", device_class);

    n += snprintf(payload + n, sizeof(payload) - n,
        ",\"device\":{"
            "\"identifiers\":[\"%s\"],"
            "\"name\":\"%s\","
            "\"model\":\"%s\","
            "\"sw_version\":\"%s\""
        "}}",
        node, dev->name, dev->model, dev->sw_version);

    publish_config(client, "sensor", node, object_id, payload);
}

void ha_add_binary_sensor(esp_mqtt_client_handle_t client,
                          const ha_device_t *dev,
                          const char *node,
                          const char *object_id,
                          const char *friendly_name,
                          const char *device_class)
{
    char state[128];
    ha_state_topic(state, sizeof(state), node, object_id);

    char payload[512];
    int n = snprintf(payload, sizeof(payload),
        "{"
            "\"name\":\"%s\","
            "\"unique_id\":\"%s_%s\","
            "\"state_topic\":\"%s\"",
        friendly_name, node, object_id, state);

    if (device_class)
        n += snprintf(payload + n, sizeof(payload) - n,
                      ",\"device_class\":\"%s\"", device_class);

    n += snprintf(payload + n, sizeof(payload) - n,
        ",\"device\":{"
            "\"identifiers\":[\"%s\"],"
            "\"name\":\"%s\","
            "\"model\":\"%s\","
            "\"sw_version\":\"%s\""
        "}}",
        node, dev->name, dev->model, dev->sw_version);

    publish_config(client, "binary_sensor", node, object_id, payload);
}

void ha_add_switch(esp_mqtt_client_handle_t client,
                   const ha_device_t *dev,
                   const char *node,
                   const char *object_id,
                   const char *friendly_name)
{
    char state[128], command[128];
    ha_state_topic  (state,   sizeof(state),   node, object_id);
    ha_command_topic(command, sizeof(command), node, object_id);

    char payload[512];
    snprintf(payload, sizeof(payload),
        "{"
            "\"name\":\"%s\","
            "\"unique_id\":\"%s_%s\","
            "\"state_topic\":\"%s\","
            "\"command_topic\":\"%s\","
            "\"device\":{"
                "\"identifiers\":[\"%s\"],"
                "\"name\":\"%s\","
                "\"model\":\"%s\","
                "\"sw_version\":\"%s\""
            "}"
        "}",
        friendly_name, node, object_id,
        state, command,
        node, dev->name, dev->model, dev->sw_version);

    publish_config(client, "switch", node, object_id, payload);
}