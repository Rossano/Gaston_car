/*
 * cli.h
 *
 *  Created on: 21 août 2025
 *      Author: rossa
 */

#ifndef CLI_H_
#define CLI_H_

#include "FreeRTOS.h"
#include "queue.h"

#define UART_RX_QUEUE_LEN   64

extern QueueHandle_t uartQueue;

void EUSART0_init();
void cli_task(void *);

#endif /* CLI_H_ */
