#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "esp_err.h"

esp_err_t led_control_init(void);

void led_control_set_direction(const char *direction);

#endif