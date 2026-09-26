#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "mqtt.h"
#include "led_indicator.h"
#include "aht20_sensor.h"
#include "device_id.h"

static const char *TAG = "main";

#define PUBLISH_INTERVAL_MS  30000

void app_main(void) {
    ESP_LOGI(TAG, "Init LED indicator");
    led_indicator_init();

    ESP_LOGI(TAG, "Init NVS");    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    led_indicator_flash(0, 40, 0, 80);

    ESP_LOGI(TAG, "Starting Wi-Fi...");
    ESP_ERROR_CHECK(wifi_init_sta());
    led_indicator_flash(0, 40, 0, 80);

    esp_mqtt_client_handle_t mqtt_client = NULL;
    ESP_ERROR_CHECK(mqtt_init(&mqtt_client));
    const char *node = device_id_get();

    ESP_ERROR_CHECK(aht20_sensor_init());
    ESP_ERROR_CHECK(aht20_sensor_start_task(mqtt_client, node, PUBLISH_INTERVAL_MS));

    while (1) {
        if (wifi_is_connected()) {
            if (mqtt_is_connected()) {
                led_indicator_set_state(LED_STATE_READY);
            } else {
                led_indicator_set_state(LED_STATE_NO_MQTT);
            }
        } else {
            led_indicator_set_state(LED_STATE_NO_WIFI);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}