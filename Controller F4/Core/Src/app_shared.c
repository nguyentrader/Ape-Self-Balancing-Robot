/*
 * app_shared.c
 *
 *  Created on: 21 Nov 2026
 *      Author: LE TRAN NGUYEN
 */

#include "app_shared.h"

uint8_t mpu_raw_data[14];

float gyro_bias_x = 0.0f;
float gyro_bias_y = 0.0f;
float gyro_bias_z = 0.0f;

float angle = 0.0f;
float angle_gyro = 0.0f;
int16_t pwm = 0;
float angle_offset = -4.2f;

int16_t enc1 = 0;
int16_t enc2 = 0;
int16_t pwm_left = 0;
int16_t pwm_right = 0;

float current_speed = 0;

float target_speed = 0.0f;
int16_t turn_offset = 0;

float Kp_angle = 88.8f;
float Kd_angle = 1.6f;

float Kp_speed = 0.35f;
float Ki_speed = 0.003f;
float Kff_speed = 0.08f;

float speed_integral = 0.0f;
float filtered_speed = 0.0f;

uint8_t rx_byte;
char buff[128];
