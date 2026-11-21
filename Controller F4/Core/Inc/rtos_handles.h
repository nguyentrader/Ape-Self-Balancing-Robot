/*
 * rtos_handles.h
 *
 *  Created on: 21 Nov 2026
 *      Author: LE TRAN NGUYEN
 */


#ifndef RTOS_HANDLES_H
#define RTOS_HANDLES_H

#include "cmsis_os.h"

extern osThreadId CommandTaskHandle;
extern osThreadId BalanceTaskHandle;
extern osThreadId SpeedTaskHandle;

extern osMessageQId UartQueueHandle;

extern osMutexId DataMutexHandle;

extern osSemaphoreId I2CSemaphoreHandle;

void MX_FREERTOS_Init(void);

#endif /* RTOS_HANDLES_H */
