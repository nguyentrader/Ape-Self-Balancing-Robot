#ifndef VEHICLE_STATE_H
#define VEHICLE_STATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#define VEHICLE_DIRECTION_SIZE  8
#define VEHICLE_TELEMETRY_SIZE  16

// Initialize the shared vehicle state.
esp_err_t vehicle_state_init(void);

// Set and get the current vehicle direction.
void vehicle_state_set_direction(const char *direction);
void vehicle_state_get_direction(char *buffer, size_t size);

// Set and get vehicle telemetry.
void vehicle_state_set_telemetry(const char *speed, const char *angle);

void vehicle_state_get_telemetry(
    char *speed,
    size_t speed_size,
    char *angle,
    size_t angle_size
);

// Set and get the last client activity timestamp.
void vehicle_state_set_last_active(int64_t time_us);
int64_t vehicle_state_get_last_active(void);

// Check whether the vehicle is moving.
bool vehicle_state_is_moving(void);

#endif
