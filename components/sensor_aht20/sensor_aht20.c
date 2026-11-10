#include "sensor_aht20.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "i2c_bus.h"
#include "aht20.h"
#include "sdkconfig.h"

#include "node.h"

static const char *TAG = "AHT20";

static i2c_bus_handle_t   s_i2c_bus = NULL;
static aht20_dev_handle_t s_aht20   = NULL;
static node_entity_t     *s_temp    = NULL;
static node_entity_t     *s_hum     = NULL;

static esp_err_t i2c_master_init(void) {
    ESP_LOGI(TAG, "I2C init (SDA=%d, SCL=%d)", CONFIG_NODE_AHT20_SDA_PIN, CONFIG_NODE_AHT20_SCL_PIN);

    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = CONFIG_NODE_AHT20_SDA_PIN,
        .scl_io_num       = CONFIG_NODE_AHT20_SCL_PIN,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = CONFIG_NODE_AHT20_I2C_FREQ_HZ,
    };

    s_i2c_bus = i2c_bus_create(CONFIG_NODE_AHT20_I2C_PORT, &conf);
    if (s_i2c_bus == NULL) {
        ESP_LOGE(TAG, "i2c_bus_create failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "I2C ok");
    return ESP_OK;
}

static void sensor_task(void *arg) {
    while (1) {
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
            node_sensor_update(s_temp, temperature);
            node_sensor_update(s_hum,  humidity);
        } else {
            ESP_LOGE(TAG, "read failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(CONFIG_NODE_AHT20_INTERVAL_MS));
    }
}

esp_err_t sensor_aht20_start(void) {
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

    s_temp = node_add_sensor(&(node_sensor_cfg_t){
        .id           = "temperature",
        .name         = "Температура",
        .unit         = "°C",
        .device_class = "temperature",
        .state_class  = "measurement",
        .precision    = 2,
    });
    s_hum = node_add_sensor(&(node_sensor_cfg_t){
        .id           = "humidity",
        .name         = "Влажность",
        .unit         = "%",
        .device_class = "humidity",
        .state_class  = "measurement",
        .precision    = 2,
    });

    BaseType_t ret = xTaskCreate(sensor_task, "aht20_pub", 4096, NULL, 5, NULL);
    return (ret == pdPASS) ? ESP_OK : ESP_FAIL;
}
