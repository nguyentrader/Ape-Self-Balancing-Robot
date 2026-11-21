/*
 * mpu9250.c
 *
 *  Created on: 21 Nov 2026
 *      Author: LE TRAN NGUYEN
 */

#include "main.h"
#include "app_shared.h"
#include "mpu9250.h"
#include "i2c.h"

void MPU9250_Init(void)
{
    uint8_t data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, 0x68 << 1, 0x6B,
                      I2C_MEMADD_SIZE_8BIT, &data, 1, 100);

    HAL_Delay(10);

    uint8_t dlpf_cfg = 0x03;
    HAL_I2C_Mem_Write(&hi2c1, 0x68 << 1, 0x1A,
                      I2C_MEMADD_SIZE_8BIT, &dlpf_cfg, 1, 100);

    uint8_t gyro_cfg = 0x10;
    HAL_I2C_Mem_Write(&hi2c1, 0x68 << 1, 0x1B,
                      I2C_MEMADD_SIZE_8BIT, &gyro_cfg, 1, 100);

    HAL_Delay(1000);

    MPU9250_CalibrateGyro();
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
