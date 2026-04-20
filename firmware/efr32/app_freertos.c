/***************************************************************************//**
 * @file
 * @brief FreeRTOS helper functions for the application task.
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "sl_main_init.h"
#include "app_assert.h"
#include "app.h"
#include "sl_simple_led_instances.h"
#include "app_log.h"
#include "cli.h"
#include "queue.h"

#define APP_TASK_NAME          "app_task"
#define APP_TASK_STACK_SIZE    512u
#define APP_TASK_PRIO          24u
#define APP_MUTEX_WAIT         100 // Timeout to wait for mutex in ticks
#define BLINK_PERIOD_MS        500

// Application task.
static void app_task(void *p_arg);

// Task handle
static TaskHandle_t      app_task_handle  = NULL;
// Semaphore handle
static SemaphoreHandle_t app_semaphore_handle = NULL;
// Mutex handle
static SemaphoreHandle_t app_mutex_handle = NULL;

// Application Runtime Init.
void app_init_bt(void)
{
  BaseType_t ret;
  
  // // Create Queues FIRST - CRITICAL
  // cli_queue = xQueueCreate(1, CLI_COMMAND_MAX_LEN);
  // if(cli_queue == NULL) {
  //   app_log("CLI queue creation failed");
  //   while(1);
  // } else {
  //   app_log("CLI queue created successfully");
  // }
  
  // uartQueue = xQueueCreate(UART_RX_QUEUE_LEN, sizeof(uint8_t));
  // if(uartQueue == NULL) {
  //   app_log("UART queue creation failed");
  //   while(1);
  // } else {
  //   app_log("UART queue created successfully");
  // }
  
  // Create the app_task
  ret = xTaskCreate(app_task,
                    APP_TASK_NAME,
                    APP_TASK_STACK_SIZE,
                    NULL,
                    APP_TASK_PRIO,
                    &app_task_handle);
  if(ret != pdPASS) {
    app_log("Application task creation failed");
    while(1);
  } else {
    app_log("Application task created successfully");
  }
  
  // Create the cli_task
  ret = xTaskCreate(cli_task,
                    "cli_task",
                    512,  // Stack size in words
                    NULL,
                    20,   // Priority
                    &cli_task_handle);
  if(ret != pdPASS) {
    app_log("CLI task creation failed");
    while(1);
  } else {
    app_log("CLI task created successfully");
  }
  
  // // Create the semaphore
  // app_semaphore_handle = xSemaphoreCreateCounting(UINT16_MAX, 0);
  // if(app_semaphore_handle == NULL) {
  //   app_log("Semaphore creation failed");
  //   while(1);
  // } else {
  //   app_log("Semaphore created successfully");
  // }
  
  // // Create the mutex
  // app_mutex_handle = xSemaphoreCreateRecursiveMutex();
  // if(app_mutex_handle == NULL) {
  //   app_log("Mutex creation failed");
  //   while(1);
  // } else {
  //   app_log("Mutex created successfully");
  // }
  
  // app_log("app_init_bt() completed successfully");
}

/******************************************************************************
 * Application task.
 *****************************************************************************/
static void app_task(void *p_arg)
{
  (void)p_arg;
  while (1) {
    //app_process_action();
    TickType_t last_blink_time = xTaskGetTickCount();

    // Controllo del LED (ogni 500ms)
    if ((xTaskGetTickCount() - last_blink_time) >= pdMS_TO_TICKS(BLINK_PERIOD_MS)) {
      sl_led_toggle(&sl_led_led0);
      last_blink_time = xTaskGetTickCount();
    }
  }
}

// Proceed with execution.
void app_proceed(void)
{
  if (xPortIsInsideInterrupt()) {
    // Interrupt context
    BaseType_t woken = pdFALSE;
    (void)xSemaphoreGiveFromISR(app_semaphore_handle, &woken);
    portYIELD_FROM_ISR(woken);
  } else {
    // Non-interrupt context
    (void)xSemaphoreGive(app_semaphore_handle);
  }
}

// Check if it is required to process with execution.
bool app_is_process_required(void)
{
  BaseType_t ret = xSemaphoreTake(app_semaphore_handle, portMAX_DELAY);
  return (ret == pdTRUE);
}

// Acquire access to protected variables
bool app_mutex_acquire(void)
{
  BaseType_t response;
  response = xSemaphoreTakeRecursive(app_mutex_handle, (TickType_t)APP_MUTEX_WAIT);
  return response == pdTRUE;
}

// Finish access to protected variables
void app_mutex_release(void)
{
  (void)xSemaphoreGiveRecursive(app_mutex_handle);
}
