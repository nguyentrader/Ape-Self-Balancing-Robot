
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_shared.h"
#include "usart.h"
#include "command_task.h"
#include "balance_task.h"
#include "speed_task.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

osThreadId CommandTaskHandle;
osThreadId BalanceTaskHandle;
osThreadId SpeedTaskHandle;

osMessageQId UartQueueHandle;

osMutexId DataMutexHandle;

osSemaphoreId I2CSemaphoreHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void MX_FREERTOS_Init(void);

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory(
    StaticTask_t **ppxIdleTaskTCBBuffer,
    StackType_t **ppxIdleTaskStackBuffer,
    uint32_t *pulIdleTaskStackSize
);

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */

static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(
    StaticTask_t **ppxIdleTaskTCBBuffer,
    StackType_t **ppxIdleTaskStackBuffer,
    uint32_t *pulIdleTaskStackSize
)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = &xIdleStack[0];
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void)
{
    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Create the mutex(es) */
    osMutexDef(DataMutex);
    DataMutexHandle = osMutexCreate(osMutex(DataMutex));

    /* USER CODE BEGIN RTOS_MUTEX */

    /* USER CODE END RTOS_MUTEX */

    /* Create the semaphore(s) */
    osSemaphoreDef(I2CSemaphore);
    I2CSemaphoreHandle = osSemaphoreCreate(osSemaphore(I2CSemaphore), 1);

    osSemaphoreWait(I2CSemaphoreHandle, 0);

    /* USER CODE BEGIN RTOS_SEMAPHORES */

    /* USER CODE END RTOS_SEMAPHORES */

    /* USER CODE BEGIN RTOS_TIMERS */

    /* USER CODE END RTOS_TIMERS */

    /* Create the queue(s) */
    osMessageQDef(UartQueue, 16, uint8_t);
    UartQueueHandle = osMessageCreate(osMessageQ(UartQueue), NULL);

    /* USER CODE BEGIN RTOS_QUEUES */

    /* USER CODE END RTOS_QUEUES */

    /* Create the thread(s) */

    osThreadDef(CommandTask, StartCommandTask, 2, 0, 256);
    CommandTaskHandle = osThreadCreate(osThread(CommandTask), NULL);

    osThreadDef(BalanceTask, StartBalanceTask, 3, 0, 512);
    BalanceTaskHandle = osThreadCreate(osThread(BalanceTask), NULL);

    osThreadDef(SpeedTask, StartSpeedTask, 1, 0, 256);
    SpeedTaskHandle = osThreadCreate(osThread(SpeedTask), NULL);

    /* USER CODE BEGIN RTOS_THREADS */

    /* USER CODE END RTOS_THREADS */

    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

