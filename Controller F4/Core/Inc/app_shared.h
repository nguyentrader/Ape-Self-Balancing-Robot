/*
 * app_shared.h
 *
 *  Created on: 21 Nov 2026
 *      Author: LE TRAN NGUYEN
 */

#ifndef APP_SHARED_H
#define APP_SHARED_H

#include <stdint.h>

extern uint8_t mpu_raw_data[14];

extern float gyro_bias_x;
extern float gyro_bias_y;
extern float gyro_bias_z;

extern float angle;
extern float angle_gyro;
extern int16_t pwm;
extern float angle_offset;

extern int16_t enc1;
extern int16_t enc2;
extern int16_t pwm_left;
extern int16_t pwm_right;

extern float current_speed;

extern float target_speed;
extern int16_t turn_offset;

extern float Kp_angle;
extern float Kd_angle;

extern float Kp_speed;
extern float Ki_speed;
extern float Kff_speed;

extern float speed_integral;
extern float filtered_speed;

extern uint8_t rx_byte;
extern char buff[128];

#endif /* APP_SHARED_H */
