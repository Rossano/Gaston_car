/*
 * cli.c
 *
 *  Created on: 21 août 2025
 *      Author: rossa
 */

#include "sl_bluetooth.h"
#include "sl_common.h"
#include "app.h"
#include "cli.h"
#include <string.h>
#include <stdio.h>
#include "gatt_db.h"
//#include "sl_cli.h"
#include "em_eusart.h"
#include "efr32bg22_eusart.h"
//#include "sl_eusart_instances.h"

//extern sl_cli_handle_t sl_cli_default_handle;

QueueHandle_t uartQueue;

void eusart_init(void)
{
  //sl_eusart_config_t
}

//void EUART_RX_IRQHandler();

void cli_task(void *pvParameters)
{
  char cmd[CLI_COMMAND_MAX_LEN];
  (void)pvParameters;
  uint8_t rx_byte;

  while(1) {
      if(xQueueReceive(uartQueue, &rx_byte, portMAX_DELAY) == pdPASS) {
          sl_bt_gatt_server_send_notification(connection_handle, gattdb_My_SPP_Write, 1, &rx_byte);
      }
      //sl_cli_tick();
      /*
      if (xQueueReceive(cli_queue, &cmd, portMAX_DELAY) == pdPASS) {
          if(strcmp(cmd, "status") == 0) {
              //send_spp_data("Device OK\n");
              strcpy(buffer, "Device OK\n");
          } else if (strcmp(cmd, "reset") == 0) {
              //send_spp_data("Rebooting ...\n");
              strcpy(buffer, "Rebooting ...\n");
              vTaskDelay(pdMS_TO_TICKS(100));
              NVIC_SystemReset();
          } else if (strcmp(cmd, "cmd ") > 0) {
              //send_spp_data("Command ???\n");
              strcpy(buffer, "Command ???\n");
          } else {
              strcpy(buffer, "Unknwown command\n");
          }
     }*/
  }
}

/*
void EUSART0_init()
{
  // Enable EUSART Interrupt NVIC
//  NVIC_EnableIRQ(EUSART_RX_IRQn);
  // Enable specific EUSART interrupts//
  ESART_IntEnable(EUART0, EUSART_IEN_RXFL);
}

void ESUART0_IRQHandler()
{
  // Read interrupt flag
  uint32_t flags = EUSART_IntGet(EUART0);

  // handle RX FIFO buffer
  if(flags & EUSART_IF_RXFL) {
      uint8_t data = EUART0->RXDATA;
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      xQueueSendFromISR(uartQueue, &data, &xHigherPriorityTaskWoken);
  }
  // Clear handled falgs
  ESUART_IntClear(EUART0, flags);
}
*/
