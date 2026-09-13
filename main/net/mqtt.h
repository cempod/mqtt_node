#ifndef MQTT_H
#define MQTT_H

#include "esp_err.h"
#include "mqtt_client.h"

esp_err_t mqtt_init(esp_mqtt_client_handle_t *client);
bool mqtt_is_connected(void);

#endif // MQTT_H