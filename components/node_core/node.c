#include "node.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_app_desc.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "wifi.h"
#include "mqtt.h"
#include "ha_discovery.h"
#include "device_id.h"
#include "led_indicator.h"

static const char *TAG = "node";

#define NODE_MAX_ENTITIES  16
#define NODE_TOPIC_LEN     160

typedef enum {
    NODE_KIND_SENSOR,
} node_kind_t;

struct node_entity {
    node_kind_t       kind;
    node_sensor_cfg_t sensor;
    char              state_topic[NODE_TOPIC_LEN];
    bool              has_value;
    float             value;
};

static node_entity_t            s_entities[NODE_MAX_ENTITIES];
static size_t                   s_count;
static bool                     s_started;
static volatile bool            s_fault;
static esp_mqtt_client_handle_t s_client;
static portMUX_TYPE             s_lock = portMUX_INITIALIZER_UNLOCKED;

static void publish_sensor_state(node_entity_t *e) {
    bool  has;
    float value;

    taskENTER_CRITICAL(&s_lock);
    has   = e->has_value;
    value = e->value;
    taskEXIT_CRITICAL(&s_lock);

    if (!has) return;

    char buf[24];
    snprintf(buf, sizeof(buf), "%.*f", e->sensor.precision, value);
    esp_mqtt_client_publish(s_client, e->state_topic, buf, 0, 1, 0);
}

static void on_mqtt_connected(esp_mqtt_client_handle_t client) {
    const char *node = device_id_get();

    ha_device_t dev = {
        .name       = CONFIG_MQTT_NODE_NAME,
        .model      = CONFIG_NODE_DEVICE_MODEL,
        .sw_version = esp_app_get_description()->version,
    };

    for (size_t i = 0; i < s_count; i++) {
        node_entity_t *e = &s_entities[i];
        switch (e->kind) {
        case NODE_KIND_SENSOR:
            ha_add_sensor(client, &dev, node, e->sensor.id, e->sensor.name,
                          e->sensor.unit, e->sensor.device_class, e->sensor.state_class);
            publish_sensor_state(e);
            break;
        }
    }
}

node_entity_t *node_add_sensor(const node_sensor_cfg_t *cfg) {
    if (!cfg || !cfg->id || !cfg->name) return NULL;
    if (s_started) {
        ESP_LOGE(TAG, "add %s after node_start", cfg->id);
        return NULL;
    }
    if (s_count >= NODE_MAX_ENTITIES) {
        ESP_LOGE(TAG, "too many entities, drop %s", cfg->id);
        return NULL;
    }

    node_entity_t *e = &s_entities[s_count++];
    memset(e, 0, sizeof(*e));
    e->kind   = NODE_KIND_SENSOR;
    e->sensor = *cfg;
    ha_state_topic(e->state_topic, sizeof(e->state_topic), device_id_get(), cfg->id);

    ESP_LOGI(TAG, "sensor %s -> %s", cfg->id, e->state_topic);
    return e;
}

void node_sensor_update(node_entity_t *e, float value) {
    if (!e) return;

    taskENTER_CRITICAL(&s_lock);
    e->value     = value;
    e->has_value = true;
    taskEXIT_CRITICAL(&s_lock);

    if (mqtt_is_connected()) {
        publish_sensor_state(e);
    }
}

void node_set_fault(bool fault) {
    s_fault = fault;
}

static void status_task(void *arg) {
    while (1) {
        if (s_fault) {
            led_indicator_set_state(LED_STATE_ERROR);
        } else if (!wifi_is_connected()) {
            led_indicator_set_state(LED_STATE_NO_WIFI);
        } else if (!mqtt_is_connected()) {
            led_indicator_set_state(LED_STATE_NO_MQTT);
        } else {
            led_indicator_set_state(LED_STATE_READY);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

esp_err_t node_init(void) {
    led_indicator_init();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) return err;
    led_indicator_flash(0, 40, 0, 80);

    err = wifi_start();
    if (err != ESP_OK) return err;

    return ESP_OK;
}

esp_err_t node_start(void) {
    s_started = true;

    esp_err_t err = mqtt_init(&s_client, on_mqtt_connected);
    if (err != ESP_OK) return err;

    if (xTaskCreate(status_task, "node_status", 2560, NULL, 2, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
