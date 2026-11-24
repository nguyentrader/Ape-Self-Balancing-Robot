#include "vehicle_state.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_timer.h"

static SemaphoreHandle_t state_mutex = NULL;

static char current_direction[VEHICLE_DIRECTION_SIZE] = "S";
static char current_speed[VEHICLE_TELEMETRY_SIZE] = "0.00";
static char current_angle[VEHICLE_TELEMETRY_SIZE] = "0.00";

static int64_t last_active_us = 0;

esp_err_t vehicle_state_init(void)
{
    if (state_mutex != NULL) {
        return ESP_OK;
    }

    state_mutex = xSemaphoreCreateMutex();

    if (state_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }

    current_direction[0] = 'S';
    current_direction[1] = '\0';

    current_speed[0] = '0';
    current_speed[1] = '.';
    current_speed[2] = '0';
    current_speed[3] = '0';
    current_speed[4] = '\0';

    current_angle[0] = '0';
    current_angle[1] = '.';
    current_angle[2] = '0';
    current_angle[3] = '0';
    current_angle[4] = '\0';

    last_active_us = esp_timer_get_time();

    return ESP_OK;
}

void vehicle_state_set_direction(const char *direction)
{
    if (state_mutex == NULL || direction == NULL) {
        return;
    }

    xSemaphoreTake(state_mutex, portMAX_DELAY);

    snprintf(
        current_direction,
        sizeof(current_direction),
        "%s",
        direction
    );

    xSemaphoreGive(state_mutex);
}

void vehicle_state_get_direction(char *buffer, size_t size)
{
    if (buffer == NULL || size == 0) {
        return;
    }

    if (state_mutex == NULL) {
        snprintf(buffer, size, "S");
        return;
    }

    xSemaphoreTake(state_mutex, portMAX_DELAY);

    snprintf(
        buffer,
        size,
        "%s",
        current_direction
    );

    xSemaphoreGive(state_mutex);
}

void vehicle_state_set_telemetry(const char *speed, const char *angle)
{
    if (state_mutex == NULL) {
        return;
    }

    xSemaphoreTake(state_mutex, portMAX_DELAY);

    if (speed != NULL) {
        snprintf(
            current_speed,
            sizeof(current_speed),
            "%s",
            speed
        );
    }

    if (angle != NULL) {
        snprintf(
            current_angle,
            sizeof(current_angle),
            "%s",
            angle
        );
    }

    xSemaphoreGive(state_mutex);
}

void vehicle_state_get_telemetry(
    char *speed,
    size_t speed_size,
    char *angle,
    size_t angle_size
)
{
    if (state_mutex == NULL) {
        if (speed != NULL && speed_size > 0) {
            snprintf(speed, speed_size, "0.00");
        }

        if (angle != NULL && angle_size > 0) {
            snprintf(angle, angle_size, "0.00");
        }

        return;
    }

    xSemaphoreTake(state_mutex, portMAX_DELAY);

    if (speed != NULL && speed_size > 0) {
        snprintf(speed, speed_size, "%s", current_speed);
    }

    if (angle != NULL && angle_size > 0) {
        snprintf(angle, angle_size, "%s", current_angle);
    }

    xSemaphoreGive(state_mutex);
}

void vehicle_state_set_last_active(int64_t time_us)
{
    if (state_mutex == NULL) {
        return;
    }

    xSemaphoreTake(state_mutex, portMAX_DELAY);

    last_active_us = time_us;

    xSemaphoreGive(state_mutex);
}

int64_t vehicle_state_get_last_active(void)
{
    int64_t result = 0;

    if (state_mutex == NULL) {
        return result;
    }

    xSemaphoreTake(state_mutex, portMAX_DELAY);

    result = last_active_us;

    xSemaphoreGive(state_mutex);

    return result;
}

bool vehicle_state_is_moving(void)
{
    bool moving = false;

    if (state_mutex == NULL) {
        return false;
    }

    xSemaphoreTake(state_mutex, portMAX_DELAY);

    moving = (current_direction[0] != 'S' ||
              current_direction[1] != '\0');

    xSemaphoreGive(state_mutex);

    return moving;
}
