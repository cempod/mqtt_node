#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>
#include "esp_err.h"

esp_err_t wifi_start(void);

bool wifi_is_connected(void);

#endif // WIFI_H
