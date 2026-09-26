#ifndef AHT20_H
#define AHT20_H

#include "esp_err.h"
#include "mqtt_client.h"

esp_err_t aht20_sensor_init(void);

esp_err_t aht20_sensor_start_task(esp_mqtt_client_handle_t client,
                                  const char *node_name,
                                  uint32_t interval_ms);

#endif