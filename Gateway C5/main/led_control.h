#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "esp_err.h"

// Initialize the WS2812B LED strip.
esp_err_t led_control_init(void);

// Update LEDs according to the vehicle direction.
void led_control_set_direction(const char *direction);

#endif
