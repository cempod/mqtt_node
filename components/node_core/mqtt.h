#ifndef MQTT_H
#define MQTT_H

#include <stdbool.h>
#include "esp_err.h"
#include "mqtt_client.h"

typedef void (*mqtt_connected_cb_t)(esp_mqtt_client_handle_t client);

esp_err_t mqtt_init(esp_mqtt_client_handle_t *client, mqtt_connected_cb_t on_connected);
bool mqtt_is_connected(void);

#endif // MQTT_H
