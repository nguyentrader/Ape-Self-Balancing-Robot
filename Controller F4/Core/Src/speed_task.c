/*
 * speed_task.c
 *
 *  Created on: 21 Nov 2026
 *      Author: LE TRAN NGUYEN
 */

#include "main.h"
#include "app_shared.h"
#include "rtos_handles.h"
#include "speed_task.h"
#include "usart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "math.h"
#include "stdio.h"

void StartSpeedTask(void const * argument)
{
  for(;;)
  {
	  if (huart2.gState == HAL_UART_STATE_READY)
	  {
		  int16_t speed_x10 = (int16_t)(fabsf(filtered_speed) * 10.0f);
		  const char *angle_sign = (angle < -0.05f) ? "-" : "";
		  int16_t angle_x10 = (int16_t)(fabsf(angle) * 10.0f);
		  int len = snprintf(buff, sizeof(buff), "D:%d.%d,%s%d.%d\n", speed_x10 / 10, speed_x10 % 10, angle_sign, angle_x10 / 10, angle_x10 % 10);
		  taskENTER_CRITICAL();
		  HAL_UART_Transmit_IT(&huart2, (uint8_t*)buff, len);
		  taskEXIT_CRITICAL();
	  }

	  // Ngủ 200ms nhường CPU cho BalanceTask và CommandTask
	  osDelay(200);
  }
}
