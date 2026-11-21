/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "math.h"
#include "FreeRTOS.h"
#include "string.h"
#include <stdio.h>
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
I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_rx;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

osThreadId CommandTaskHandle;
osThreadId BalanceTaskHandle;
osThreadId SpeedTaskHandle;
osMessageQId UartQueueHandle;
osMutexId DataMutexHandle;
osSemaphoreId I2CSemaphoreHandle;
/* USER CODE BEGIN PV */
uint8_t mpu_raw_data[14];
float gyro_bias_x = 0.0f;
float gyro_bias_y = 0.0f;
float gyro_bias_z = 0.0f;

// --- Biến trạng thái điều khiển ---
float angle = 0.0f;
float angle_gyro = 0.0f;
int16_t pwm = 0;
float angle_offset = -4.2f;
int16_t enc1 = 0;
int16_t enc2 = 0;
int16_t pwm_left = 0;
int16_t pwm_right = 0;
float current_speed = 0;

// --- Biến giao tiếp lệnh (Bảo vệ bằng Mutex) ---
float target_speed = 0.0f;
int16_t turn_offset = 0;

// --- Thông số PID góc (Vòng trong) ---
float Kp_angle = 88.8f;
float Kd_angle = 1.6f;

// --- Thông số PID tốc độ (Vòng ngoài) ---
float Kp_speed = 0.35f;
float Ki_speed = 0.003f;
float Kff_speed = 0.08f;

float speed_integral = 0.0f;
float filtered_speed = 0.0f;

// --- Dữ liệu UART RX ---
uint8_t rx_byte;
char buff[128];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM1_Init(void);
void StartCommandTask(void const * argument);
void StartBalanceTask(void const * argument);
void StartSpeedTask(void const * argument);

/* USER CODE BEGIN PFP */
void MPU9250_CalibrateGyro(void);
void Motor_Set(int16_t pwm_left, int16_t pwm_right);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);
  __HAL_TIM_MOE_ENABLE(&htim1);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
  Motor_Set(0, 0);
  uint8_t data = 0x00;
  HAL_I2C_Mem_Write(&hi2c1, 0x68 << 1, 0x6B, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
  HAL_Delay(10);
  uint8_t dlpf_cfg = 0x03; // Bật bộ lọc thông thấp ~42Hz chống rung chấn va chạm
  HAL_I2C_Mem_Write(&hi2c1, 0x68 << 1, 0x1A, I2C_MEMADD_SIZE_8BIT, &dlpf_cfg, 1, 100);
  uint8_t gyro_cfg = 0x10; // Nâng dải đo Gyro lên +-1000 deg/s
  HAL_I2C_Mem_Write(&hi2c1, 0x68 << 1, 0x1B, I2C_MEMADD_SIZE_8BIT, &gyro_cfg, 1, 100);
  HAL_Delay(1000);
  MPU9250_CalibrateGyro();
  /* USER CODE END 2 */

  /* Create the mutex(es) */
  /* definition and creation of DataMutex */
  osMutexDef(DataMutex);
  DataMutexHandle = osMutexCreate(osMutex(DataMutex));

  /* USER CODE BEGIN RTOS_MUTEX */

  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of I2CSemaphore */
  osSemaphoreDef(I2CSemaphore);
  I2CSemaphoreHandle = osSemaphoreCreate(osSemaphore(I2CSemaphore), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  osSemaphoreWait(I2CSemaphoreHandle, 0);
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of UartQueue */
  osMessageQDef(UartQueue, 16, uint8_t);
  UartQueueHandle = osMessageCreate(osMessageQ(UartQueue), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of CommandTask */
  osThreadDef(CommandTask, StartCommandTask, 2, 0, 256);
  CommandTaskHandle = osThreadCreate(osThread(CommandTask), NULL);

  /* definition and creation of BalanceTask */
  osThreadDef(BalanceTask, StartBalanceTask, 3, 0, 512);
  BalanceTaskHandle = osThreadCreate(osThread(BalanceTask), NULL);

  /* definition and creation of SpeedTask */
  osThreadDef(SpeedTask, StartSpeedTask, 1, 0, 256);
  SpeedTaskHandle = osThreadCreate(osThread(SpeedTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 5;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB12 PB13 PB14 PB15
                           PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == I2C1)
  {
    osSemaphoreRelease(I2CSemaphoreHandle);
  }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
//  if (hi2c->Instance == I2C1)
//  {
//    osSemaphoreRelease(I2CSemaphoreHandle);
//  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART2){
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xQueueSendFromISR((QueueHandle_t)UartQueueHandle, &rx_byte, &xHigherPriorityTaskWoken);
		HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        __HAL_UART_CLEAR_OREFLAG(huart);
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}

void MPU9250_CalibrateGyro(void){
	uint8_t gyro_data[6];
	int32_t sum_x = 0, sum_y = 0, sum_z = 0;
	for(int i = 0; i < 500; i++){
		HAL_I2C_Mem_Read(&hi2c1, 0x68 << 1, 0x43, I2C_MEMADD_SIZE_8BIT, gyro_data, 6, 100);
		sum_x += (int16_t)((gyro_data[0] << 8) | gyro_data[1]);
		sum_y += (int16_t)((gyro_data[2] << 8) | gyro_data[3]);
		sum_z += (int16_t)((gyro_data[4] << 8) | gyro_data[5]);
		HAL_Delay(2);
	}
	gyro_bias_x = sum_x / 500.0f;
	gyro_bias_y = sum_y / 500.0f;
	gyro_bias_z = sum_z / 500.0f;
}

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
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartCommandTask */
/**
  * @brief  Function implementing the CommandTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartCommandTask */
void StartCommandTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
	char rx_line[32];
	uint8_t idx = 0;
	while(1)
	{
		osEvent event = osMessageGet(UartQueueHandle, osWaitForever);
	    if(event.status == osEventMessage){
	    	uint8_t byte = (uint8_t)event.value.v;
	    	if(byte != '\n' && byte != '\r'){
	    		rx_line[idx++] = (char)byte;
	    		if(idx >= sizeof(rx_line) - 1){
	    			idx = 0;
	    		}
	    	}
	    	else{
	    		if(idx > 0){
	    			rx_line[idx] = '\0';
	    			float next_speed = 0.0f;
	    			int16_t next_turn = 0;
	    			uint8_t update = 0;
	    			if(strcmp(rx_line, "F") == 0){
	    				next_speed = -36.0f;
	    				update = 1;
	    			}
	    			else if(strcmp(rx_line, "B") == 0){
	    				next_speed = 30.0f;
	    				update = 1;
	    			}
	    			else if(strcmp(rx_line, "L") == 0){
	    				next_turn = -190;
	    				update = 2;
	    			}
	    			else if(strcmp(rx_line, "R") == 0){
	    				next_turn = 190;
	    				update = 2;
	    			}
	    			else if(strcmp(rx_line, "S") == 0){
	    			    next_speed = 0.0f;
	    			    next_turn = 0;
	    			    update = 1;
	    			}
	    			else{
	    				next_speed = 0.0f;
	    				next_turn = 0;
	    				update = 0;
	    			}
	    			if(update > 0 && osMutexWait(DataMutexHandle, osWaitForever) == osOK){
	    				if(update == 1){
	    					target_speed = next_speed;
	    					turn_offset = next_turn;
	    				}
	    				else if(update == 2){
	    					turn_offset = next_turn;
	    				}
	    				osMutexRelease(DataMutexHandle);
	    			}
	    		}
	    		idx = 0;
	    	}
	    }
	}
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartBalanceTask */
/**
* @brief Function implementing the BalanceTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartBalanceTask */
void StartBalanceTask(void const * argument)
{
  /* USER CODE BEGIN StartBalanceTask */
  /* Infinite loop */
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
  /* USER CODE END StartBalanceTask */
}

/* USER CODE BEGIN Header_StartSpeedTask */
/**
* @brief Function implementing the SpeedTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSpeedTask */
void StartSpeedTask(void const * argument)
{
  /* USER CODE BEGIN StartSpeedTask */
  /* Infinite loop */
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
  /* USER CODE END StartSpeedTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM5 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM5) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

