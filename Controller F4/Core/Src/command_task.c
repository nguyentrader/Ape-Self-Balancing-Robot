/*
 * command_task.c
 *
 *  Created on: 21 Nov 2026
 *      Author: LE TRAN NGUYEN
 */

#include "main.h"
#include "rtos_handles.h"
#include "app_shared.h"
#include "freertos.h"
#include "command_task.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "string.h"



void StartCommandTask(void const * argument)
{
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
}
