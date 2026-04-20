/*
 * cli.c
 *
 *  Created on: 21 août 2025
 *      Author: rossa
 */

#include "sl_bluetooth.h"
#include "sl_common.h"
#include "app.h"
#include "spp.h"
#include "cli.h"
#include <string.h>
#include <stdio.h>
#include "gatt_db.h"
//#include "sl_cli.h"
#include "em_eusart.h"
#include "efr32bg22_eusart.h"
#include "sl_iostream_handles.h"
#include "sl_iostream_init_eusart_instances.h"

//#include "uartdrv.h"
#include "app_log.h"

//extern sl_cli_handle_t sl_cli_default_handle;
extern sl_iostream_t * sl_iostream_vcom_handle;

QueueHandle_t uartQueue;
QueueHandle_t cli_queue;
TaskHandle_t cli_task_handle = NULL;


void cli_task(void *pvParameters)
{
  (void)pvParameters;
  char ch;
  uint8_t buffer[CLI_COMMAND_MAX_LEN];
  uint8_t len = 0;
  uint8_t handled_data;
  uint8_t byte_from_queue;

  // Clear buffer
  memset(buffer, 0, sizeof(buffer));

  // Attendi che il sistema sia stabile
  vTaskDelay(pdMS_TO_TICKS(100));
  app_log("[CLI] Task started on vcom_handle: %p\n", (void*)sl_iostream_vcom_handle);

  for (;;) {
    handled_data = 0;
    
    // Sicurezza: se l'handle non è inizializzato, prova a usare il default
    if (sl_iostream_vcom_handle == NULL) {
      sl_iostream_vcom_handle = sl_iostream_get_default();
      if (sl_iostream_vcom_handle == NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
        continue;
      }
    }
    
    // Direction 1: VCOM -> BLE
    // Read character from VCOM
    if (sl_iostream_getchar(sl_iostream_vcom_handle, &ch) == SL_STATUS_OK) {
      handled_data = 1;
      // Is it a new line?
      if ((ch != '\n') && (ch != '\r')) {
        // Not a new line, so buffer it
        if (len < CLI_COMMAND_MAX_LEN - 1) {
          buffer[len] = (uint8_t)ch;
          len++;
        }
      } else {
        // New line character received, so send the buffer
        if (len > 0) {
          send_spp_data(buffer, len);
        }
        len = 0;
      }
    } else {
      // Opzionale: piccolo delay per non saturare la CPU se getchar fallisce
      // ma uartQueue è vuota
    }

    // Direction 2: BLE (via queue) -> VCOM
    if (xQueueReceive(uartQueue, &byte_from_queue, 0) == pdPASS) {
      handled_data = 1;
      sl_iostream_putchar(sl_iostream_vcom_handle, byte_from_queue);
    }

    // If no data was handled in either direction, delay the task to yield CPU
    if (!handled_data) {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}