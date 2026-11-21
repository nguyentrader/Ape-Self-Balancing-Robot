/*
 * motor.c
 *
 *  Created on: 21 Nov 2026
 *      Author: LE TRAN NGUYEN
 */

#include "main.h"
#include "motor.h"
#include "tim.h"

void Motor_Set(int16_t pwm_left, int16_t pwm_right)
{
	if (pwm_left > 999)   pwm_left = 999;
	if (pwm_left < -999)  pwm_left = -999;
	if (pwm_right > 999)  pwm_right = 999;
	if (pwm_right < -999) pwm_right = -999;

	if (pwm_left > 0) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwm_left);
	}
	else if (pwm_left < 0) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, -pwm_left);
	}
	else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
	}

	if (pwm_right > 0) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwm_right);
	}
	else if (pwm_right < 0) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, -pwm_right);
	}
	else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
	}
}
