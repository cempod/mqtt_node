#include "led_indicator.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

#if CONFIG_ESP_BOARD_LED_TYPE_RGB
#include "led_strip.h"
#elif CONFIG_ESP_BOARD_LED_TYPE_GPIO
#include "driver/gpio.h"
#if CONFIG_ESP_BOARD_GPIO_LED_PWM
#include "driver/ledc.h"
#endif
#endif

static const char *TAG = "led";

#define LED_TICK_MS   20

typedef struct { uint8_t r, g, b; } rgb_t;

static const rgb_t C_OFF     = {  0,  0,  0 };
static const rgb_t C_WHITE   = { 40, 40, 40 };
static const rgb_t C_RED     = { 40,  0,  0 };
static const rgb_t C_GREEN   = {  0, 40,  0 };
static const rgb_t C_YELLOW  = { 40, 24,  0 };

static volatile led_state_t s_state = LED_STATE_BOOT;
static volatile int64_t s_flash_until_us = 0;
static volatile rgb_t s_flash_color = { 0, 0, 0 };

#if CONFIG_ESP_BOARD_LED_TYPE_RGB
static led_strip_handle_t s_strip;
#endif

#if CONFIG_ESP_BOARD_LED_TYPE_GPIO && CONFIG_ESP_BOARD_GPIO_LED_PWM
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_RES        LEDC_TIMER_10_BIT
#define LEDC_MAX_DUTY   ((1u << 10) - 1u)
#define LEDC_FREQ_HZ    5000
#endif

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
#if CONFIG_ESP_BOARD_LED_TYPE_RGB
    led_strip_set_pixel(s_strip, 0, c.r, c.g, c.b);
    led_strip_refresh(s_strip);

#elif CONFIG_ESP_BOARD_LED_TYPE_GPIO
#if CONFIG_ESP_BOARD_GPIO_LED_PWM
    uint8_t v = c.r;
    if (c.g > v) v = c.g;
    if (c.b > v) v = c.b;

    uint32_t duty = (uint32_t)v * LEDC_MAX_DUTY / 255u;
#if CONFIG_ESP_BOARD_GPIO_LED_ACTIVE_LOW
    duty = LEDC_MAX_DUTY - duty;
#endif
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

#else
    bool on = (c.r | c.g | c.b) != 0;
#if CONFIG_ESP_BOARD_GPIO_LED_ACTIVE_LOW
    int level = on ? 0 : 1;
#else
    int level = on ? 1 : 0;
#endif
    gpio_set_level(CONFIG_ESP_BOARD_GPIO_LED_PIN, level);
#endif

#else
    (void)c;
#endif
}

void led_indicator_set_state(led_state_t st) {
#if CONFIG_ESP_BOARD_LED_TYPE_NONE
    (void)st;
#else
    s_state = st;
#endif
}

void led_indicator_flash(uint8_t r, uint8_t g, uint8_t b, uint32_t ms) {
#if CONFIG_ESP_BOARD_LED_TYPE_NONE
    (void)r; (void)g; (void)b; (void)ms;
#else
    s_flash_color = (rgb_t){ r, g, b };
    s_flash_until_us = esp_timer_get_time() + (int64_t)ms * 1000;
#endif
}

#if !CONFIG_ESP_BOARD_LED_TYPE_NONE
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
            c = blink(t, 100, 100) ? C_RED : C_OFF;
            break;
        }

        draw(c);
        vTaskDelay(pdMS_TO_TICKS(LED_TICK_MS));
        t += LED_TICK_MS;
    }
}
#endif

void led_indicator_init(void) {
#if CONFIG_ESP_BOARD_LED_TYPE_RGB
    led_strip_config_t strip_cfg = {
        .strip_gpio_num = CONFIG_ESP_BOARD_RGB_LED_PIN,
        .max_leds = 1,
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
    ESP_LOGI(TAG, "RGB LED on GPIO %d", CONFIG_ESP_BOARD_RGB_LED_PIN);

#elif CONFIG_ESP_BOARD_LED_TYPE_GPIO
#if CONFIG_ESP_BOARD_GPIO_LED_PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LEDC_MODE,
        .timer_num       = LEDC_TIMER,
        .duty_resolution = LEDC_RES,
        .freq_hz         = LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_ch = {
        .gpio_num   = CONFIG_ESP_BOARD_GPIO_LED_PIN,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_ch));

    uint32_t off = CONFIG_ESP_BOARD_GPIO_LED_ACTIVE_LOW ? LEDC_MAX_DUTY : 0;
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, off);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

    ESP_LOGI(TAG, "GPIO LED (PWM) on pin %d @ %d Hz",
             CONFIG_ESP_BOARD_GPIO_LED_PIN, LEDC_FREQ_HZ);
#else
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << CONFIG_ESP_BOARD_GPIO_LED_PIN,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));
#if CONFIG_ESP_BOARD_GPIO_LED_ACTIVE_LOW
    gpio_set_level(CONFIG_ESP_BOARD_GPIO_LED_PIN, 1);
#else
    gpio_set_level(CONFIG_ESP_BOARD_GPIO_LED_PIN, 0);
#endif
    ESP_LOGI(TAG, "GPIO LED (on/off) on pin %d",
             CONFIG_ESP_BOARD_GPIO_LED_PIN);
#endif

#else
    ESP_LOGI(TAG, "LED indicator disabled by config");
    return;
#endif

    xTaskCreate(led_task, "led", 3072, NULL, 3, NULL);
    ESP_LOGI(TAG, "LED indicator ready");
}