/*
 * cli.h
 *
 *  Created on: RTOS SPP Bridge Utility
 *      Author: Auto-regenerated
 */

#ifndef CLI_H_
#define CLI_H_

#include "FreeRTOS.h"
#include "queue.h"

/* SPP UART queue configuration - optimized for single connection */
#define UART_RX_QUEUE_LEN   32  /* Optimized: 32 bytes sufficient for single SPP connection */
#define CLI_COMMAND_MAX_LEN 64

extern TaskHandle_t cli_task_handle;
extern QueueHandle_t uartQueue;
extern QueueHandle_t cli_queue;

void cli_task(void *pvParameters);

#endif /* CLI_H_ */
