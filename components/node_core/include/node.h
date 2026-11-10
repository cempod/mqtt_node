#ifndef NODE_H
#define NODE_H

#include <stdbool.h>
#include "esp_err.h"

typedef struct node_entity node_entity_t;

typedef struct {
    const char *id;
    const char *name;
    const char *unit;
    const char *device_class;
    const char *state_class;
    int         precision;
} node_sensor_cfg_t;

esp_err_t node_init(void);
esp_err_t node_start(void);

node_entity_t *node_add_sensor(const node_sensor_cfg_t *cfg);
void node_sensor_update(node_entity_t *entity, float value);

void node_set_fault(bool fault);

#endif
