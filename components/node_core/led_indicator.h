#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include <stdint.h>

typedef enum {
    LED_STATE_BOOT,
    LED_STATE_NO_WIFI,
    LED_STATE_NO_MQTT,
    LED_STATE_READY,
    LED_STATE_ERROR,
} led_state_t;

void led_indicator_init(void);
void led_indicator_set_state(led_state_t state);
void led_indicator_flash(uint8_t r, uint8_t g, uint8_t b, uint32_t ms);

#endif