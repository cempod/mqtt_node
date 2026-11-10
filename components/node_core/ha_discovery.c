#include "ha_discovery.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "ha_disc";

void ha_state_topic(char *buf, size_t n, const char *node, const char *obj) {
    snprintf(buf, n, "%s/%s/state", node, obj);
}

void ha_command_topic(char *buf, size_t n, const char *node, const char *obj) {
    snprintf(buf, n, "%s/%s/set", node, obj);
}

void ha_status_topic(char *buf, size_t n, const char *node) {
    snprintf(buf, n, "%s/status", node);
}

static void add_opt_string(cJSON *obj, const char *key, const char *val) {
    if (val) cJSON_AddStringToObject(obj, key, val);
}

static cJSON *base_config(const ha_device_t *dev,
                          const char *node,
                          const char *object_id,
                          const char *friendly_name) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    char buf[160];

    cJSON_AddStringToObject(root, "name", friendly_name);

    snprintf(buf, sizeof(buf), "%s_%s", node, object_id);
    cJSON_AddStringToObject(root, "unique_id", buf);

    ha_state_topic(buf, sizeof(buf), node, object_id);
    cJSON_AddStringToObject(root, "state_topic", buf);

    ha_status_topic(buf, sizeof(buf), node);
    cJSON_AddStringToObject(root, "availability_topic", buf);
    cJSON_AddStringToObject(root, "payload_available", "online");
    cJSON_AddStringToObject(root, "payload_not_available", "offline");

    cJSON *device = cJSON_AddObjectToObject(root, "device");
    cJSON *ids    = cJSON_AddArrayToObject(device, "identifiers");
    cJSON_AddItemToArray(ids, cJSON_CreateString(node));
    add_opt_string(device, "name",       dev->name);
    add_opt_string(device, "model",      dev->model);
    add_opt_string(device, "sw_version", dev->sw_version);

    return root;
}

static void publish_config(esp_mqtt_client_handle_t client,
                           const char *component,
                           const char *node,
                           const char *object_id,
                           cJSON *root) {
    if (!root) {
        ESP_LOGE(TAG, "no memory for %s config", object_id);
        return;
    }

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!payload) {
        ESP_LOGE(TAG, "serialize failed: %s", object_id);
        return;
    }

    char topic[192];
    int n = snprintf(topic, sizeof(topic),
                     "homeassistant/%s/%s/%s/config",
                     component, node, object_id);
    if (n < 0 || (size_t)n >= sizeof(topic)) {
        ESP_LOGE(TAG, "topic too long: %s", object_id);
    } else {
        esp_mqtt_client_publish(client, topic, payload, 0, 1, 1);
        ESP_LOGI(TAG, "→ %s", topic);
    }

    cJSON_free(payload);
}

void ha_add_sensor(esp_mqtt_client_handle_t client,
                   const ha_device_t *dev,
                   const char *node,
                   const char *object_id,
                   const char *friendly_name,
                   const char *unit,
                   const char *device_class,
                   const char *state_class) {
    cJSON *root = base_config(dev, node, object_id, friendly_name);
    if (root) {
        add_opt_string(root, "unit_of_measurement", unit);
        add_opt_string(root, "device_class",        device_class);
        add_opt_string(root, "state_class",         state_class);
    }
    publish_config(client, "sensor", node, object_id, root);
}

void ha_add_binary_sensor(esp_mqtt_client_handle_t client,
                          const ha_device_t *dev,
                          const char *node,
                          const char *object_id,
                          const char *friendly_name,
                          const char *device_class) {
    cJSON *root = base_config(dev, node, object_id, friendly_name);
    if (root) {
        add_opt_string(root, "device_class", device_class);
    }
    publish_config(client, "binary_sensor", node, object_id, root);
}

void ha_add_switch(esp_mqtt_client_handle_t client,
                   const ha_device_t *dev,
                   const char *node,
                   const char *object_id,
                   const char *friendly_name) {
    cJSON *root = base_config(dev, node, object_id, friendly_name);
    if (root) {
        char command[160];
        ha_command_topic(command, sizeof(command), node, object_id);
        cJSON_AddStringToObject(root, "command_topic", command);
    }
    publish_config(client, "switch", node, object_id, root);
}
