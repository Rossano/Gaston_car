<<<<<<< HEAD
/*
 * commands.c
 *
 *  Created on: Dec 17, 2023
 *      Author: rossa
 */

/**
 * Includes Files
 */
#include "Motor.h"
#include <stdio.h>
#include <stdlib.h>

// Button Variable
int btn = 0;

int toggle_button(int argc, char **argv)
{
	if(btn == 0)
	{
		printf("ON!!!\r\n");
		btn = 1;
	}
	else
	{
		btn = 0;
		printf("OFF!!!\r\n");
	}
	return 0;
}

/**
 * Commands to move motor
 */
int forward(int argc, char **argv)
{
	if(argc < 2) return -1;
	else
	{
		int speed = atoi(argv[1]);
		if (speed != 0)
		{
			SetMotorPWM(MOTOR_A, speed);
			SetMotorPWM(MOTOR_B, speed);
			return 0;
		}
	}
	return -2;
}

int backward(int argc, char **argv)
{
	if(argc < 2) return -1;
	else
	{
		int speed = atoi(argv[1]);
		if (speed != 0)
		{
			SetMotorPWM(MOTOR_A, -speed);
			SetMotorPWM(MOTOR_B, -speed);
			return 0;
		}
	}
	return -2;
}

int stop(int argc, char **argv)
{
	motor_StopMotorA();
	motor_StopMotorB();
	return 0;
}


/**
 * Command to turn left
 */
int left(int argc, char **argv)
{
	if (argc < 1) return -1;
	else
	{
		if (bridgeA != ENABLED && bridgeB != ENABLED) return -3;
		if (stateA == stateB && stateA == RUN)
		{
			int valA, valB;
			if (speed_A < speed_B)
			{
				// Do nothing it is already turning left
				return 0;
			}
			else if (dirA == dirB && dirA == FORWARD)
			{
				valA = speed_A - PWM_TURN_OFFSET;
				valB = speed_B + PWM_TURN_OFFSET;
			}
			else if (dirA == dirB && dirA == BACKWARD)
			{
				valA = speed_A + PWM_TURN_OFFSET;
				valB = speed_B - PWM_TURN_OFFSET;
			}
			else
			{
				// Direction Unknown or bridge A and B diverging
				return -4;
			}
			// All checks passed, PWM may be set
			SetMotorPWM(MOTOR_A, valA);
			SetMotorPWM(MOTOR_B, valB);
			speed_A = valA;
			speed_B = valB;
			return 0;
		}
		else
		{
			// Bridge status diverging or not in RUN state
			return -5;
		}
	}
	return -2;
}

/**
 * Command to turn Right
 */
int right(int argc, char **argv)
{
	if (argc < 1) return -1;
	else
	{
		if (bridgeA != ENABLED && bridgeB != ENABLED) return -3;
		if (stateA == stateB && stateA == RUN)
		{
			int valA, valB;
			if (speed_A > speed_B)
			{
				// Do nothing it is already turning Right
				return 0;
			}
			else if (dirA == dirB && dirA == FORWARD)
			{
				valA = speed_A + PWM_TURN_OFFSET;
				valB = speed_B - PWM_TURN_OFFSET;
			}
			else if (dirA == dirB && dirA == BACKWARD)
			{
				valA = speed_A - PWM_TURN_OFFSET;
				valB = speed_B + PWM_TURN_OFFSET;
			}
			else
			{
				// Direction Unknown or bridge A and B diverging
				return -4;
			}
			// All checks passed, PWM may be set
			SetMotorPWM(MOTOR_A, valA);
			SetMotorPWM(MOTOR_B, valB);
			speed_A = valA;
			speed_B = valB;
			return 0;
		}
		else
		{
			// Bridge status diverging or not in RUN state
			return -5;
		}
	}
	return -2;
}

/**
 * Command to stop turning
 */
int straight(int argc, char **argv)
{
	if (argc < 2) return -1;
	else
	{
		if (bridgeA != ENABLED && bridgeB != ENABLED) return -3;
		if (stateA == stateB && stateA == RUN)
		{
			int val;
			if (speed_A == speed_B)
			{
				// Do nothing it is already going straight
				return 0;
			}
			else if (speed_A > speed_B)
			{
				val = speed_A - PWM_TURN_OFFSET;
			}
			else
			{
				val = speed_A + PWM_TURN_OFFSET;
			}
			// All checks passed, PWM may be set
			SetMotorPWM(MOTOR_A, val);
			SetMotorPWM(MOTOR_B, val);
			speed_A = val;
			speed_B = val;
			return 0;
		}
		else
		{
			// Bridge status diverging or not in RUN state
			return -5;
		}
	}
	return -2;
}

/**
 * Command to get motor speeds and direction
 */
int get_status(int argc, char **argv)
{
	if (argc < 1) return -1;
	else
	{
		printf("Gaston status: SpeedA: %d SpeedB: %d\r\n", speed_A, speed_B);
		return 0;
	}
}
=======
/*
 * commands.c
 *
 *  Created on: 18 avr. 2021
 *      Author: Ross
 */


/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include <controller.h>
#include "stm32f4xx_hal.h"

//#include "tim.h"
#include "stm32f4xx_hal_uart.h"
//#include "gpio.h"*/

#include "com.h"
#include "app_x-cube-mems1.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "stspin240_250.h"
#include "x_nucleo_ihmxx.h"
#include "commands.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define PROMPT		"\r\nSTM32> "
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

char str[20];
uint8_t command[UARTBUFFERSIZE];
uint8_t msg[UARTBUFFERSIZE];
uint8_t sbt_rx_p = 0;
uint8_t sbt_tx_p = 0;
uint8_t rx_char[1];
uint8_t tx_char[1];
//uint8_t newCmd = false;
uint8_t done = 0;
uint32_t count = 0;
//UART_HandleTypeDef huart2;
extern volatile uint16_t pwm;
extern float angle;

static TMsg msg_cmd;

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
void print_welcome()
{
	// Welcome message
	sprintf((char *)msg, "#\r\n# Servo Motor Control\r\n# type help for list of commands\r\n#\r\n");
	//HAL_UART_Transmit_IT(&huart2, msg, strlen((char *)msg));
	HAL_UART_Transmit(&hcom_uart[COM1], msg, strlen((char *)msg), 5000);
	sprintf((char *)msg, PROMPT);
	HAL_UART_Transmit(&hcom_uart[COM1], msg, 9, 5000);
}

void init_parser()
{

}

int parse_execute(void)
{
	HAL_StatusTypeDef res;

	if(newCmd == true)
	{
/*		// Get UART buffer
		if (UART_ReceivedMSG((TMsg *)&msg_cmd) != 1)
		{
//			if (msg_cmd.Data[0] == DEV_ADDR)
//		    {
//				(void)HandleMSG((TMsg *)&msg_cmd);
//		    }
			msg_cmd.Data[msg_cmd.Len] = 0;
		}
*/
		// HELP Command
		if(!strcmp((char *)command, "help"))
		{
			sprintf((char *)msg,"\r\nang -> get Tilt, Pitch, Gravity angles\r\n");
			res = HAL_UART_Transmit(&hcom_uart[COM1], msg, strlen((char *)msg), 5000);
			//HAL_UART_Transmit_IT(&hcom_uart[COM1], msg, strlen((char *)msg));
			sprintf((char *)msg, "mot <COMMAND>\r\n\tstart -> start the motors\r\n");
			res = HAL_UART_Transmit(&hcom_uart[COM1], msg, strlen((char *)msg), 5000);
			//HAL_UART_Transmit_IT(&hcom_uart[COM1], msg, strlen((char *)msg));
			sprintf((char *)msg, "\tstop -> stop the motors\r\n\trst -> rest the chip\r\n");
			res = HAL_UART_Transmit(&hcom_uart[COM1], msg, strlen((char *)msg), 5000);
			//HAL_UART_Transmit_IT(&hcom_uart[COM1], msg, strlen((char *)msg));
			sprintf((char *)msg, "pwm <-100..+100> -> set motor speed\r\nget -> get PWM data\r\n");
			res = HAL_UART_Transmit(&hcom_uart[COM1], msg, strlen((char *)msg), 5000);
			//HAL_UART_Transmit_IT(&hcom_uart[COM1], msg, strlen((char *)msg));
			sprintf((char *)msg, "ctrl <COMMAND>\r\n\tstart -> start the controller\r\n\tstop -> stop the controller\r\n");
			res = HAL_UART_Transmit(&hcom_uart[COM1], msg, strlen((char *)msg), 5000);
			//HAL_UART_Transmit_IT(&hcom_uart[COM1], msg, strlen((char *)msg));
			sprintf((char *)msg, "set <Value K1> <Value K2> <Value K3> <Value K4> -> set controller values\r\n");
			res = HAL_UART_Transmit(&hcom_uart[COM1], msg, strlen((char *)msg), 5000);
			//HAL_UART_Transmit_IT(&hcom_uart[COM1], msg, strlen((char *)msg));
		}
		else if(!strncmp((char *)command, "mot ", 4))
		{
			char foo[3];
			char cmd[UARTBUFFERSIZE];
			if(sscanf((char *)command, "%s %s", foo, cmd))
			{
				if(!strncmp((char *)cmd, "stop",4))
				{
					// Hard stop the motor
					/* Stop both motors and disable bridge */
					BSP_MotorControl_CmdHardHiZ(0);
			        BSP_MotorControl_CmdHardHiZ(1);
				}
				else if(!strncmp((char *) cmd, "start", 4))
				{
					// Start the motor Forward - no speed
					// Put motor speed to 0
					BSP_MotorControl_SetMaxSpeed(1,0);
					BSP_MotorControl_SetMaxSpeed(0,0);
			        /* Start both motors to go forward*/
			        BSP_MotorControl_Run(0,FORWARD);
			        BSP_MotorControl_Run(1,FORWARD);
				}
				else if(!strncmp((char *) cmd, "rst", 3))
				{
					// Stop both motors and put the chip in reset mode
					BSP_MotorControl_Reset(0);
				}
			}
			else
			{
				printError("[error]: Motor command error!\r\n");
			}
		}
		else if(!strncmp((char *)command, "pwm ", 4))
		{
			char foo[3];
			char val[5];
			if(sscanf((char *)command, "%s %s", foo, val))
			{
				uint16_t v = atoi(val);
				pwm = v;
				BSP_MotorControl_SetMaxSpeed(0, pwm);
				BSP_MotorControl_SetMaxSpeed(1, pwm);
				BSP_MotorControl_Run(0, FORWARD);
				BSP_MotorControl_Run(1, FORWARD);
			}
			else
			{
				printError("[error]: PWM setting command error!\r\n");
			}
		}
		else if(!strncmp((char *)command, "get", 3))
		{
			sprintf((char *)msg, "PWM = %d\r\n", pwm);
			res = HAL_UART_Transmit(&hcom_uart[COM1], msg, strlen((char *)msg), 5000);
		}
		else if(!strncmp((char *)command, "ctrl  ", 5))
		{
			char foo[3];
			char cmd[UARTBUFFERSIZE];
			if(sscanf((char *)command, "%s %s", foo, cmd))
			{
				if(!strncmp((char *) cmd, "start", 5))
				{
					// STart Controller
					pid_start();
				}
				else if(!strncmp((char *) cmd, "stop", 4))
				{
					// Stop the controller
					pid_stop();
				}
			}
			else
			{
				printError("[error]: controller command error!\r\n");
			}
		}
		else if(!strncmp((char *)command, "set ", 4))
		{
			char foo[3];
			char val1[16], val2[16], val3[16], val4[16];
			if(sscanf((char *) command, "%s %s %s %s %s", foo, val1, val2, val3, val4))
			{
				int16_t K1, K2, K3, K4;
				K1 = atoi(val1);
				K2 = atoi(val2);
				K3 = atoi(val3);
				K4 = atoi(val4);
#ifdef __USE_PID__
				kp = K1/1000.0;
				ki = K2/1000.0;
				kd = K3/1000.0;
#endif
			}
		}
		else if(!strncmp((char *)command, "ang", 3))
		{
			char foo[5];
			int16_t a1, a2, a3;
			sprintf((char *)msg,"\nTilt = %f Roll = %f Pitch = %f\r\n", 1*data_out.angles_array[2],
					1*data_out.angles_array[1], 1*data_out.angles_array[0]);
			res = HAL_UART_Transmit(&hcom_uart[COM1], msg, strlen((char *)msg), 5000);
		}
		else
		{
			printError("[error]: command not recognized!\r\n");
//			sprintf((char *)msg, "[error]: command not recognized!\r\n");
//			HAL_UART_Transmit(&hcom_uart[COM1], msg, strlen((char *)msg), 5000);
			newCmd = false;
//			sbt_rx_p = 0;
//			sbt_tx_p = 0;
//			return -1;
//			sprintf((char *)msg, PROMPT);
//			HAL_UART_Transmit(&hcom_uart[COM1], msg, 9, 5000);
		}
		newCmd = false;
		sbt_rx_p = 0;
		sbt_tx_p = 0;
		sprintf((char *)msg, PROMPT);
		res = HAL_UART_Transmit(&hcom_uart[COM1], msg, 9, 5000);
	}
//	while (HAL_UART_GetState(&hcom_uart[COM1]) != HAL_UART_STATE_READY);

//	HAL_UART_Receive_IT(&huart2, (uint8_t *)rx_char, 1);
	HAL_UART_Receive_IT(&hcom_uart[COM1], (uint8_t*) rx_char, 1);
	return res;
	/* USER CODE END 3 */
}



HAL_StatusTypeDef printError(char *str)
{
	char p[UARTBUFFERSIZE];
	HAL_StatusTypeDef res;
	for(uint8_t i=0; i < UARTBUFFERSIZE; i++)
	{
		p[i] = str[i];
		if(!p[i]) break;
	}
	return HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)p, strlen(str), 5000);
}

/* USER CODE END 4 */
>>>>>>> 6503406ba0a9f92bfb0325a558ed63026d2a0e5d
