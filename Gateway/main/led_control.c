#include "led_control.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stddef.h>

#include "led_strip.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_timer.h"

#include "vehicle_state.h"

#define LED_GPIO               21
#define LED_COUNT              16
#define LED_BRIGHTNESS         70

#define LED_TASK_STACK         3072
#define LED_TASK_PRIORITY      5

#define LED_UPDATE_INTERVAL_US 30000

#define COLOR_NORMAL_R         200
#define COLOR_NORMAL_G         200
#define COLOR_NORMAL_B         200

#define COLOR_TURN_R           255
#define COLOR_TURN_G           120
#define COLOR_TURN_B           0

static led_strip_handle_t led_strip = NULL;

static uint16_t chase_offset = 0;
static int64_t last_led_update_us = 0;

static char previous_direction[8] = "";

static uint8_t apply_brightness(uint8_t value)
{
    return (uint8_t)((value * LED_BRIGHTNESS) / 255);
}

static uint8_t gamma_correct(uint8_t value)
{
    float normalized = (float)value / 255.0f;
    float corrected = powf(normalized, 2.6f) * 255.0f;

    return (uint8_t)corrected;
}

static void set_led(uint32_t index, uint8_t red, uint8_t green, uint8_t blue)
{
    red = apply_brightness(red);
    green = apply_brightness(green);
    blue = apply_brightness(blue);

    led_strip_set_pixel(
        led_strip,
        index,
        red,
        green,
        blue
    );
}

static void clear_leds(void)
{
    led_strip_clear(led_strip);
}

static void show_leds(void)
{
    led_strip_refresh(led_strip);
}

static void set_normal_color(void)
{
    for (int i = 0; i < LED_COUNT; i++) {
        set_led(
            i,
            COLOR_NORMAL_R,
            COLOR_NORMAL_G,
            COLOR_NORMAL_B
        );
    }

    show_leds();
}

static void set_left_turn_color(void)
{
    clear_leds();

    for (int i = 8; i < 16; i++) {
        set_led(
            i,
            COLOR_TURN_R,
            COLOR_TURN_G,
            COLOR_TURN_B
        );
    }

    show_leds();
}

static void set_right_turn_color(void)
{
    clear_leds();

    for (int i = 0; i < 8; i++) {
        set_led(
            i,
            COLOR_TURN_R,
            COLOR_TURN_G,
            COLOR_TURN_B
        );
    }

    show_leds();
}

static void hsv_to_rgb(
    uint16_t hue,
    uint8_t *red,
    uint8_t *green,
    uint8_t *blue
)
{
    float h = (float)hue * 6.0f / 65536.0f;

    int sector = (int)h;
    float fraction = h - sector;

    float p = 0.0f;
    float q = 1.0f - fraction;
    float t = fraction;

    float r;
    float g;
    float b;

    switch (sector % 6) {
        case 0:
            r = 1.0f;
            g = t;
            b = p;
            break;

        case 1:
            r = q;
            g = 1.0f;
            b = p;
            break;

        case 2:
            r = p;
            g = 1.0f;
            b = t;
            break;

        case 3:
            r = p;
            g = q;
            b = 1.0f;
            break;

        case 4:
            r = t;
            g = p;
            b = 1.0f;
            break;

        default:
            r = 1.0f;
            g = p;
            b = q;
            break;
    }

    *red = (uint8_t)(r * 255.0f);
    *green = (uint8_t)(g * 255.0f);
    *blue = (uint8_t)(b * 255.0f);
}

static void run_chase_animation(void)
{
    for (int i = 0; i < LED_COUNT; i++) {
        uint16_t pixel_hue =
            chase_offset + (uint16_t)(i * 65536UL / LED_COUNT);

        uint8_t red;
        uint8_t green;
        uint8_t blue;

        hsv_to_rgb(
            pixel_hue,
            &red,
            &green,
            &blue
        );

        red = gamma_correct(red);
        green = gamma_correct(green);
        blue = gamma_correct(blue);

        set_led(i, red, green, blue);
    }

    show_leds();

    chase_offset += 512;
}

static void update_leds(const char *direction)
{
    if (strcmp(direction, previous_direction) == 0) {
        return;
    }

    snprintf(
        previous_direction,
        sizeof(previous_direction),
        "%s",
        direction
    );

    if (strchr(direction, 'L') != NULL) {
        set_left_turn_color();
    }
    else if (strchr(direction, 'R') != NULL) {
        set_right_turn_color();
    }
    else if (strcmp(direction, "S") == 0) {
        last_led_update_us = 0;
    }
    else {
        set_normal_color();
    }
}

static void led_task(void *arg)
{
    char direction[VEHICLE_DIRECTION_SIZE];

    while (1) {
        vehicle_state_get_direction(
            direction,
            sizeof(direction)
        );

        update_leds(direction);

        int64_t current_time = esp_timer_get_time();

        if (strcmp(direction, "S") == 0) {
            if (current_time - last_led_update_us >= LED_UPDATE_INTERVAL_US) {
                last_led_update_us = current_time;
                run_chase_animation();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t led_control_init(void)
{
    const led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags.invert_out = false
    };

    const led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10000000,
        .mem_block_symbols = 64,
        .flags.with_dma = false
    };

    esp_err_t ret = led_strip_new_rmt_device(
        &strip_config,
        &rmt_config,
        &led_strip
    );

    if (ret != ESP_OK) {
        return ret;
    }

    clear_leds();
    show_leds();

    BaseType_t task_result = xTaskCreate(
        led_task,
        "led_task",
        LED_TASK_STACK,
        NULL,
        LED_TASK_PRIORITY,
        NULL
    );

    if (task_result != pdPASS) {
        led_strip_del(led_strip);
        led_strip = NULL;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

void led_control_set_direction(const char *direction)
{
    if (direction == NULL) {
        return;
    }

    update_leds(direction);
}