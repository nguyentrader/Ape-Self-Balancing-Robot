/*
 * balance_task.c
 *
 *  Created on: 21 Nov 2026
 *      Author: LE TRAN NGUYEN
 */

#include "main.h"
#include "app_shared.h"
#include "rtos_handles.h"
#include "balance_task.h"
#include "motor.h"

#include "i2c.h"
#include "tim.h"

#include "FreeRTOS.h"
#include "task.h"
#include "math.h"

void StartBalanceTask(void const * argument)
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	const TickType_t xFrequency = pdMS_TO_TICKS(10);
	const float dt = 0.01f;

	static float smooth_target_angle = 0.0f;
	static int32_t sync_integral = 0;
	static float filtered_gy = 0.0f;
	static float ramped_target_speed = 0.0f;
	static uint8_t boost_active = 0;
	static uint16_t stable_cnt = 0;
	static float dynamic_launch_limit = 4.5f;

	while(1)
	{
		if(HAL_I2C_Mem_Read_DMA(&hi2c1, 0x68 << 1, 0x3B, I2C_MEMADD_SIZE_8BIT, (uint8_t *)&mpu_raw_data, 14) == HAL_OK){
			if(osSemaphoreWait(I2CSemaphoreHandle, 5) == osOK){
				int16_t accel_x = (int16_t)((mpu_raw_data[0] << 8) | mpu_raw_data[1]);
				int16_t accel_z = (int16_t)((mpu_raw_data[4] << 8) | mpu_raw_data[5]);
				int16_t gyro_y  = (int16_t)((mpu_raw_data[10] << 8) | mpu_raw_data[11]);

				float ax = accel_x / 16384.0f;
				float az = accel_z / 16384.0f;
				float gy = -(gyro_y - gyro_bias_y) / 32.8f;

				float angle_acc = atan2f(ax, az) * 180.0f / 3.14159265f;
				angle_acc -= angle_offset;
				angle_gyro += gy * dt;
				angle = 0.98f * angle_gyro + 0.02f * angle_acc;
				angle_gyro = angle;

				enc1 = -(int16_t)__HAL_TIM_GET_COUNTER(&htim2);
				enc2 = -(int16_t)__HAL_TIM_GET_COUNTER(&htim1);
				__HAL_TIM_SET_COUNTER(&htim2, 0);
				__HAL_TIM_SET_COUNTER(&htim1, 0);

				current_speed = -(enc1 + enc2) / 2.0f;
				filtered_speed = filtered_speed * 0.85f + current_speed * 0.15f;

				static float local_target_speed = 0.0f;
				static int16_t local_turn_offset = 0;
				if (osMutexWait(DataMutexHandle, osWaitForever) == osOK) {
					local_target_speed = target_speed;
					local_turn_offset = turn_offset;
					osMutexRelease(DataMutexHandle);
				}

				// 2. Xác định pha: Pha 1 Đề-pa (khóa -23.8f) -> Pha 2 Bứt tốc (Boost)
				const float LAUNCH_TARGET_MAX = -23.9f;
				float effective_target_speed = local_target_speed;

				if (local_target_speed < LAUNCH_TARGET_MAX) {
				    if (!boost_active) {
				        effective_target_speed = LAUNCH_TARGET_MAX;

				        // Chỉ bật pha 2 khi xong ramp đề pa, xe đã có đà và góc ổn định 300ms
				        if (ramped_target_speed <= (LAUNCH_TARGET_MAX + 0.5f) &&
				            fabsf(filtered_speed) >= 12.0f &&
				            fabsf(angle) < 4.5f) {
				            if (++stable_cnt >= 30) {
				                boost_active = 1;
				            }
				        } else {
				            stable_cnt = 0;
				        }
				    } else {
				        effective_target_speed = local_target_speed;
				    }
				} else {
				    boost_active = 0;
				    stable_cnt = 0;
				}

				// 2b. Ramp tốc độ mục tiêu theo từng pha
				float ramp_step;
				if (local_target_speed == 0.0f) {
				    // Phanh từ tốc độ cao giảm nhẹ ramp để tránh bật ngửa ra sau
				    ramp_step = (fabsf(filtered_speed) > 22.0f) ? 0.30f : 0.45f;
				} else if (boost_active) {
				    // Pha 2: Tăng tốc êm; nếu góc chúi > 7.0 độ thì tạm dừng tăng tốc chờ cân lại
				    ramp_step = (fabsf(angle) > 7.0f) ? 0.0f : 0.10f;
				} else if (fabsf(filtered_speed) < 11.0f) {
				    ramp_step = 0.12f; // Giữ nguyên pha đề pa
				} else {
				    ramp_step = 0.27f; // Giữ nguyên pha đề pa
				}

				if (ramped_target_speed < effective_target_speed) {
				    ramped_target_speed += ramp_step;
				    if (ramped_target_speed > effective_target_speed) ramped_target_speed = effective_target_speed;
				} else if (ramped_target_speed > effective_target_speed) {
				    ramped_target_speed -= ramp_step;
				    if (ramped_target_speed < effective_target_speed) ramped_target_speed = effective_target_speed;
				}

				// 3. Kẹp góc động: giữ 4.5/6.0 lúc đề pa, nới mượt lên 7.5 khi vào pha 2
				float min_angle = -15.0f;
				float max_angle =  15.0f;

				if (local_target_speed == 0.0f) {
				    dynamic_launch_limit = 4.5f;
				} else {
				    float target_limit;
				    if (boost_active) {
				        target_limit = 7.5f;
				    } else {
				        target_limit = (fabsf(filtered_speed) < 12.0f) ? 4.5f : 6.0f;
				    }

				    if (dynamic_launch_limit < target_limit) {
				        dynamic_launch_limit += 0.03f;
				        if (dynamic_launch_limit > target_limit) dynamic_launch_limit = target_limit;
				    } else if (dynamic_launch_limit > target_limit) {
				        dynamic_launch_limit -= 0.05f;
				        if (dynamic_launch_limit < target_limit) dynamic_launch_limit = target_limit;
				    }
				}

				if (local_target_speed < 0.0f) {
				    min_angle = -dynamic_launch_limit;
				} else if (local_target_speed > 0.0f) {
				    max_angle = dynamic_launch_limit;
				}

				// 4. PID vòng ngoài có Anti-windup
				float speed_error = ramped_target_speed - filtered_speed;
				float ff_angle = Kff_speed * ramped_target_speed;
				float raw_target_angle = ff_angle + (Kp_speed * speed_error) + (Ki_speed * speed_integral);

				if (fabsf(angle) < 10.0f) {
				    if ((raw_target_angle > min_angle && raw_target_angle < max_angle) ||
				        (raw_target_angle <= min_angle && speed_error > 0.0f) ||
				        (raw_target_angle >= max_angle && speed_error < 0.0f)) {
				        speed_integral += speed_error;
				    }
				}
				float integral_limit = boost_active ? 1200.0f : 600.0f;
				if (speed_integral >  integral_limit) speed_integral =  integral_limit;
				if (speed_integral < -integral_limit) speed_integral = -integral_limit;

				float target_angle = ff_angle + (Kp_speed * speed_error) + (Ki_speed * speed_integral);
				if (target_angle > max_angle) target_angle = max_angle;
				if (target_angle < min_angle) target_angle = min_angle;

				// 5. Lọc bất đối xứng: chúi vào êm (0.8/0.2), nhả góc cứu xe nhanh (0.5/0.5)
				if (fabsf(target_angle) > fabsf(smooth_target_angle) && (target_angle * smooth_target_angle >= 0.0f)) {
				    smooth_target_angle = smooth_target_angle * 0.6f + target_angle * 0.4f;
				} else {
				    smooth_target_angle = smooth_target_angle * 0.5f + target_angle * 0.5f;
				}

				// 6. PID vòng trong (Góc)
				float angle_error = angle - smooth_target_angle;
				filtered_gy = filtered_gy * 0.4f + gy * 0.6f;
				float pwm_out = (Kp_angle * angle_error) + (Kd_angle * filtered_gy);
				pwm = (int16_t)pwm_out;

				// 7. Xuất PWM và bảo vệ an toàn khi ngã quá 60 độ
				if (angle > 60.0f || angle < -60.0f) {
				    pwm = 0;
				    speed_integral = 0.0f;
				    sync_integral = 0;
				    ramped_target_speed = 0.0f;
				    smooth_target_angle = 0.0f;
				    boost_active = 0;
				    stable_cnt = 0;
				    dynamic_launch_limit = 4.5f;
				    Motor_Set(0, 0);
				}
				else {
				    // Tính toán đồng bộ 2 bánh (Sync)
				    int16_t enc_diff = enc1 - enc2;
				    if (local_turn_offset != 0) {
				        sync_integral = 0;
				    }
				    else {
				        sync_integral += enc_diff;
				        if (sync_integral > 250)  sync_integral = 250;
				        if (sync_integral < -250) sync_integral = -250;
				    }
				    float Kp_sync = 6.0f;
				    float Ki_sync = 0.8f;
				    int16_t sync_offset = (int16_t)(enc_diff * Kp_sync + sync_integral * Ki_sync);
				    if (local_turn_offset != 0) {
				        sync_offset = 0;
				    }

				    // Tính PWM thô cho từng bánh (Gộp PID, rẽ và đồng bộ)
				    pwm_left  = pwm + local_turn_offset - sync_offset;
				    pwm_right = pwm - local_turn_offset + sync_offset;

				    // Bù vùng chết độc lập (Deadzone)
				    int16_t min_pwm_left  = 50;
				    int16_t min_pwm_right = 45;
				    if (pwm_left > 1) {
				        pwm_left += min_pwm_left;
				    }
				    else if (pwm_left < -1) {
				        pwm_left -= min_pwm_left;
				    }
				    else {
				        pwm_left = 0;
				    }

				    if (pwm_right > 1) {
				        pwm_right += min_pwm_right;
				    }
				    else if (pwm_right < -1) {
				        pwm_right -= min_pwm_right;
				    }
				    else {
				        pwm_right = 0;
				    }

				    Motor_Set(pwm_left, pwm_right);
				}
			}
			else {
				HAL_I2C_Master_Abort_IT(&hi2c1, 0x68 << 1);
				HAL_I2C_DeInit(&hi2c1);
				HAL_I2C_Init(&hi2c1);
			}
		}
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}
