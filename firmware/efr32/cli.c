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
#include "FreeRTOS.h"
#include "queue.h"

//#include "uartdrv.h"
#include "app_log.h"

//extern sl_cli_handle_t sl_cli_default_handle;
extern sl_iostream_t * sl_iostream_vcom_handle;

QueueHandle_t uartQueue;
TaskHandle_t cli_task_handle = NULL;


void cli_task(void *pvParameters)
{
  (void)pvParameters;
  char ch;
  uint8_t b;

  for (;;) {
    // if (sl_iostream_getchar(sl_iostream_vcom_handle, &ch) == SL_STATUS_OK) {
    //   b = (uint8_t)ch;
    //   // inoltra via BLE se connesso
    //   if (conn_handle != 0xFF) {
    //     sl_status_t res = sl_bt_gatt_server_send_notification(conn_handle,
    //                                                           gattdb_My_SPP_Write,
    //                                                           1,
    //                                                           &b);
    //     if (res != SL_STATUS_OK) {
    //       app_log("notify failed: 0x%04x\r\n", res);
    //     }
    //   }
    // } else {
    //   // non c'erano dati: attendi un po' per non usare CPU al 100%
    //   vTaskDelay(pdMS_TO_TICKS(10));
    // }
    if(sl_iostream_getchar(sl_iostream_vcom_handle, &ch) == SL_STATUS_OK) {
      b = (uint8_t)ch;
      // inoltra via BLE se connesso
      xQueueSend(uartQueue, &b, 0);
    } else {
      // non c'erano dati: attendi un po' per non usare CPU al 100%
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}