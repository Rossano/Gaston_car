/***************************************************************************//**
 * @file
 * @brief FreeRTOS task management for BLE application.
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "app_assert.h"
#include "app_log.h"
#include "app.h"
#include "sl_simple_led_instances.h"
#include "cli.h"
#include "queue.h"

#define APP_TASK_NAME          "app_task"
#define APP_TASK_STACK_SIZE    512u
#define APP_TASK_PRIO          24u
#define CLI_TASK_STACK_SIZE    1024u
#define CLI_TASK_PRIO          20u
#define BLINK_PERIOD_MS        500

// Application task
static void app_task(void *p_arg);
static void cli_task_wrapper(void *p_arg);

// Task handles
static TaskHandle_t      app_task_handle  = NULL;
TaskHandle_t             cli_task_handle  = NULL;  // Must be extern for cli.h

// Semaphore and mutex handles
static SemaphoreHandle_t app_semaphore_handle = NULL;
static SemaphoreHandle_t app_mutex_handle = NULL;

// Global queues
QueueHandle_t cli_queue = NULL;
QueueHandle_t uartQueue = NULL;

/******************************************************************************
 * Application Runtime Init - called after BLE stack is ready.
 *****************************************************************************/
void app_init_bt(void)
{
  static bool initialized = false;
  if (initialized) {
    return;  // Prevent double initialization
  }
  initialized = true;

  BaseType_t ret;

  app_log("=== Initializing FreeRTOS Tasks and Synchronization ===\n");

  // Create queues FIRST, before tasks start using them
  cli_queue = xQueueCreate(4, CLI_COMMAND_MAX_LEN);
  if(cli_queue == NULL) {
    app_log("X CLI queue NOT created\n");
  } else {
    app_log("o CLI queue created\n");
  }

  uartQueue = xQueueCreate(UART_RX_QUEUE_LEN, sizeof(uint8_t));
  if(uartQueue == NULL) {
    app_log("X UART RX queue NOT created\n");
  } else {
    app_log("o UART RX queue created\n");
  }

  // // Create the semaphore for BLE event signaling
  // app_semaphore_handle = xSemaphoreCreateCounting(UINT16_MAX, 0);
  // if(app_semaphore_handle == NULL) {
  //   app_log("X Semaphore NOT created\n");
  // } else {
  //   app_log("o Semaphore created\n");
  // }

  // // Create the mutex for BLE stack thread-safety protection
  // app_mutex_handle = xSemaphoreCreateRecursiveMutex();
  // if(app_mutex_handle == NULL) {
  //   app_log("X Mutex NOT created\n");
  // } else {
  //   app_log("o Mutex created\n");
  // }

  // Create application task
  ret = xTaskCreate(app_task,
                    APP_TASK_NAME,
                    APP_TASK_STACK_SIZE,
                    NULL,
                    APP_TASK_PRIO,
                    &app_task_handle);
  if(app_task_handle == NULL) {
    app_log("X Application task NOT created\n");
  } else {
    app_log("o Application task created\n");
  }

  // Create CLI task
  ret = xTaskCreate(cli_task_wrapper,
                    "cli_task",
                    CLI_TASK_STACK_SIZE,
                    NULL,
                    CLI_TASK_PRIO,
                    &cli_task_handle);
  if(cli_task_handle == NULL) {
    app_log("X CLI task NOT created\n");
  } else {
    app_log("o CLI task created\n");
  }

  app_log("=== FreeRTOS Initialization Complete ===\n");
}

/******************************************************************************
 * Application Task - main application loop
 *****************************************************************************/
static void app_task(void *p_arg)
{
  (void)p_arg;
  uint32_t blink_counter = 0;

  app_log("app_task: Started\n");

  while (1) {
    app_process_action();  // Process BLE events and other app actions
    // Blink LED every 500ms
    if (++blink_counter >= 50) {
      sl_led_toggle(&sl_led_led0);
      blink_counter = 0;
      app_log("app_task: Heartbeat (LED toggle)\n");
    }

    // Delay 10ms to prevent CPU hogging
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/******************************************************************************
 * CLI Task Wrapper - delegates to actual CLI task
 *****************************************************************************/
static void cli_task_wrapper(void *p_arg)
{
  app_log("cli_task: Started\n");
  cli_task(p_arg);  // Call the actual CLI task from cli.c
}

/******************************************************************************
 * Signal Handler - called by BLE event handler to wake app_task
 *****************************************************************************/
void app_proceed(void)
{
  if (xPortIsInsideInterrupt()) {
    // Interrupt context - use ISR-safe version
    BaseType_t woken = pdFALSE;
    (void)xSemaphoreGiveFromISR(app_semaphore_handle, &woken);
    portYIELD_FROM_ISR(woken);
  } else {
    // Non-interrupt context
    (void)xSemaphoreGive(app_semaphore_handle);
  }
}

/******************************************************************************
 * Process Check - check if BLE event processing is required
 *****************************************************************************/
bool app_is_process_required(void)
{
  // Wait on semaphore with 100ms timeout to allow periodic processing
  BaseType_t ret = xSemaphoreTake(app_semaphore_handle, pdMS_TO_TICKS(100));
  return (ret == pdTRUE);
}

/******************************************************************************
 * Mutex Acquire - protect BLE stack access
 *****************************************************************************/
bool app_mutex_acquire(void)
{
  BaseType_t response;
  response = xSemaphoreTakeRecursive(app_mutex_handle, pdMS_TO_TICKS(100));
  return response == pdTRUE;
}

/******************************************************************************
 * Mutex Release - unprotect BLE stack
 *****************************************************************************/
void app_mutex_release(void)
{
  (void)xSemaphoreGiveRecursive(app_mutex_handle);
}

