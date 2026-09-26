#include "aht20_sensor.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "i2c_bus.h"
#include "aht20.h"

#include "mqtt.h"
#include "ha_discovery.h"

static const char *TAG = "AHT20";

#define I2C_MASTER_SCL_IO   7
#define I2C_MASTER_SDA_IO   6
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_FREQ_HZ  100000

static i2c_bus_handle_t s_i2c_bus  = NULL;
static aht20_dev_handle_t      s_aht20    = NULL;

typedef struct {
    esp_mqtt_client_handle_t client;
    char                     node_name[64];
    uint32_t                 interval_ms;
} sensor_task_ctx_t;

static sensor_task_ctx_t s_ctx = {0};

static esp_err_t i2c_master_init(void) {
    ESP_LOGI(TAG, "I2C init (SDA=%d, SCL=%d)", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = I2C_MASTER_SDA_IO,
        .scl_io_num       = I2C_MASTER_SCL_IO,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    s_i2c_bus = i2c_bus_create(I2C_MASTER_NUM, &conf);
    if (s_i2c_bus == NULL) {
        ESP_LOGE(TAG, "i2c_bus_create failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "I2C ok");
    return ESP_OK;
}

esp_err_t aht20_sensor_init(void) {
    esp_err_t err = i2c_master_init();
    if (err != ESP_OK) return err;

    aht20_i2c_config_t i2c_conf = {
        .bus_inst = s_i2c_bus,
        .i2c_addr = AHT20_ADDRRES_0,
    };

    err = aht20_new_sensor(&i2c_conf, &s_aht20);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "aht20_new_sensor failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "AHT20 ok");
    return ESP_OK;
}

static void sensor_task(void *arg) {
    sensor_task_ctx_t *ctx = (sensor_task_ctx_t *)arg;

    char temp_topic[160];
    char hum_topic[160];
    ha_state_topic(temp_topic, sizeof(temp_topic), ctx->node_name, "temperature");
    ha_state_topic(hum_topic,  sizeof(hum_topic),  ctx->node_name, "humidity");

    ESP_LOGI(TAG, "Task started. Topics:");
    ESP_LOGI(TAG, "  temp: %s", temp_topic);
    ESP_LOGI(TAG, "  hum : %s", hum_topic);

    vTaskDelay(pdMS_TO_TICKS(5000));

    while (1) {
        if (!mqtt_is_connected()) {
            ESP_LOGW(TAG, "MQTT not connected, skip");
            vTaskDelay(pdMS_TO_TICKS(ctx->interval_ms));
            continue;
        }

        uint32_t temp_raw = 0, hum_raw = 0;
        float    temperature = 0.0f;
        float    humidity    = 0.0f;

        esp_err_t err = aht20_read_temperature_humidity(
            s_aht20,
            &temp_raw,
            &temperature,
            &hum_raw,
            &humidity
        );

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "T=%.2f°C  H=%.2f%%", temperature, humidity);

            char buf[16];
            snprintf(buf, sizeof(buf), "%.2f", temperature);
            esp_mqtt_client_publish(ctx->client, temp_topic, buf, 0, 1, 0);

            snprintf(buf, sizeof(buf), "%.2f", humidity);
            esp_mqtt_client_publish(ctx->client, hum_topic, buf, 0, 1, 0);
        } else {
            ESP_LOGE(TAG, "read failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(ctx->interval_ms));
    }
}

esp_err_t aht20_sensor_start_task(esp_mqtt_client_handle_t client,
                                  const char *node_name,
                                  uint32_t interval_ms) {
    if (!client || !node_name) {
        return ESP_ERR_INVALID_ARG;
    }

    s_ctx.client      = client;
    s_ctx.interval_ms = interval_ms;
    strncpy(s_ctx.node_name, node_name, sizeof(s_ctx.node_name) - 1);
    s_ctx.node_name[sizeof(s_ctx.node_name) - 1] = '\0';

    BaseType_t ret = xTaskCreate(
        sensor_task,
        "aht20_pub",
        4096,
        &s_ctx,
        5,
        NULL
    );

    return (ret == pdPASS) ? ESP_OK : ESP_FAIL;
}