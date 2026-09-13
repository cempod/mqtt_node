#include "led_indicator.h"
#include "led_strip.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "led";

#define LED_GPIO      CONFIG_ESP_BOARD_RGB_LED_PIN
#define LED_COUNT     1
#define LED_TICK_MS   20

typedef struct { uint8_t r, g, b; } rgb_t;

static const rgb_t C_OFF     = {  0,  0,  0 };
static const rgb_t C_WHITE   = { 40, 40, 40 };
static const rgb_t C_RED     = { 40,  0,  0 };
static const rgb_t C_GREEN   = {  0, 40,  0 };
static const rgb_t C_YELLOW  = { 40, 24,  0 };

static led_strip_handle_t s_strip;
static volatile led_state_t s_state = LED_STATE_BOOT;
static volatile int64_t s_flash_until_us = 0;
static volatile rgb_t s_flash_color = { 0, 0, 0 };

static rgb_t scale(rgb_t c, float k) {
    if (k < 0.0f) k = 0.0f;
    if (k > 1.0f) k = 1.0f;
    return (rgb_t){ (uint8_t)(c.r * k),
                    (uint8_t)(c.g * k),
                    (uint8_t)(c.b * k) };
}

static float breath(uint32_t t, uint32_t period_ms) {
    float phase = (float)(t % period_ms) / (float)period_ms;
    return 0.5f - 0.5f * cosf(phase * 2.0f * (float)M_PI);
}

static bool blink(uint32_t t, uint32_t period_ms, uint32_t on_ms) {
    return (t % period_ms) < on_ms;
}

static void draw(rgb_t c) {
    led_strip_set_pixel(s_strip, 0, c.r, c.g, c.b);
    led_strip_refresh(s_strip);
}

void led_indicator_set_state(led_state_t st) {
    s_state = st;
}

void led_indicator_flash(uint8_t r, uint8_t g, uint8_t b, uint32_t ms) {
    s_flash_color = (rgb_t){ r, g, b };
    s_flash_until_us = esp_timer_get_time() + (int64_t)ms * 1000;
}

static void led_task(void *arg) {
    uint32_t t = 0;
    for (;;) {
        if (esp_timer_get_time() < s_flash_until_us) {
            draw(s_flash_color);
            vTaskDelay(pdMS_TO_TICKS(LED_TICK_MS));
            t += LED_TICK_MS;
            continue;
        }

        rgb_t c = C_OFF;
        switch (s_state) {
        case LED_STATE_BOOT:
            c = scale(C_WHITE, breath(t, 3000));
            break;

        case LED_STATE_NO_WIFI:
            c = blink(t, 1000, 500) ? C_RED : C_OFF;
            break;

        case LED_STATE_NO_MQTT:
            c = blink(t, 1000, 300) ? C_YELLOW : C_OFF;
            break;

        case LED_STATE_READY:
            c = scale(C_GREEN, 0.15f + 0.25f * breath(t, 3000));
            break;

        case LED_STATE_ERROR:
            c = blink(t, 200, 100) ? C_RED : C_OFF;
            break;
        }

        draw(c);
        vTaskDelay(pdMS_TO_TICKS(LED_TICK_MS));
        t += LED_TICK_MS;
    }
}

void led_indicator_init(void) {
    led_strip_config_t strip_cfg = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_COUNT,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };
    led_strip_rmt_config_t rmt_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &s_strip));
    ESP_ERROR_CHECK(led_strip_clear(s_strip));

    xTaskCreate(led_task, "led", 3072, NULL, 3, NULL);
    ESP_LOGI(TAG, "LED indicator ready on GPIO %d", LED_GPIO);
}