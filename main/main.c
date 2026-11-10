#include "esp_log.h"
#include "sdkconfig.h"
#include "node.h"
#include "sensor_aht20.h"

#if CONFIG_NODE_AHT20
static const char *TAG = "main";
#endif

void app_main(void) {
    ESP_ERROR_CHECK(node_init());

#if CONFIG_NODE_AHT20
    if (sensor_aht20_start() != ESP_OK) {
        ESP_LOGE(TAG, "AHT20 unavailable");
        node_set_fault(true);
    }
#endif

    ESP_ERROR_CHECK(node_start());
}
