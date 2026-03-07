/*
 * shell.h
 *
 *  Created on: Dec 10, 2023
 *      Author: rossa
 *      Copied from Github: https://github.com/Nanaud7/shell-stm32/tree/main/stm32-freertos
 */

#ifndef SRC_SHELL_H_
#define SRC_SHELL_H_

/**
 ******************************************************************************
 * @file	shell.h
 * @author 	Arnaud C.
 * @brief	shell for STM32 with FreeRTOS
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef INC_SHELL_H_
#define INC_SHELL_H_

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "main.h" //"usart.h"
#include "FreeRTOS.h"
#include "queue.h"

#include <stm32f4xx.h>

/* Exported types ------------------------------------------------------------*/
/* End of exported types -----------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/
#define BLE_SPP
#ifdef BLE_SPP
	#define UART_DEVICE huart6
#else
	#define UART_DEVICE huart2
#endif
#define RX_BUFFER_SIZE 256
/* End of exported macros ----------------------------------------------------*/

/* External variables --------------------------------------------------------*/
extern volatile uint8_t shell_rx_char;
extern volatile uint8_t vcom_rx_char;
extern uint8_t shell_rx_buffer[256];
extern uint8_t vcom_rx_buffer[256];
extern QueueHandle_t qShell;
/* End of external variables -------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
uint8_t uart_write(char *s, uint16_t size);
uint8_t shell_init(UART_HandleTypeDef* huart);
uint8_t shell_add(char * cmd, int (* pfunc)(int argc, char ** argv), char * description);
uint8_t shell_char_received();
uint8_t shell_exec(char * cmd);

#endif /* INC_SHELL_H_ */

#endif /* SRC_SHELL_H_ */
