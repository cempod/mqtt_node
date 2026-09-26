#ifndef HA_DISCOVERY_H
#define HA_DISCOVERY_H

#include "mqtt_client.h"

typedef struct {
    const char *name;
    const char *model;
    const char *sw_version;
} ha_device_t;

void ha_add_sensor(esp_mqtt_client_handle_t client,
                   const ha_device_t *dev,
                   const char *node,
                   const char *object_id,
                   const char *friendly_name,
                   const char *unit,
                   const char *device_class);

void ha_add_binary_sensor(esp_mqtt_client_handle_t client,
                          const ha_device_t *dev,
                          const char *node,
                          const char *object_id,
                          const char *friendly_name,
                          const char *device_class);

void ha_add_switch(esp_mqtt_client_handle_t client,
                   const ha_device_t *dev,
                   const char *node,
                   const char *object_id,
                   const char *friendly_name);

void ha_state_topic  (char *buf, size_t n, const char *node, const char *object_id);
void ha_command_topic(char *buf, size_t n, const char *node, const char *object_id);

#endif