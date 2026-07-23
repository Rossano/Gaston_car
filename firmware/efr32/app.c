/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "sl_bt_api.h"
#include "sl_main_init.h"
#include "app_assert.h"
#include "app.h"

#include "spp.h"
#include "cli.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"

#include <string.h>
#include "sl_iostream_handles.h"
#include "app_log.h"
#include "sl_common.h"

#define SL_BT_RTOS_APPLICATION_PRIOITY 10u

QueueHandle_t cli_queue;

uint8_t  buffer[CLI_COMMAND_MAX_LEN];
uint8_t led0_state = 0;
uint16_t sent_len = 0;
uint8_t b[20], len = 0;

// The advertising set handle allocated from Bluetooth stack.
//static uint8_t advertising_set_handle = 0xff;

// Application Init.
void app_init(void)
{
    // Create CLI queues and task here (after SDK second-stage init)
  // so the Bluetooth stack has initialized and heap is available.
  cli_queue = xQueueCreate(4, CLI_COMMAND_MAX_LEN);
  if (cli_queue == NULL) {
    app_log("Failed to create CLI queue\n");
  }

  uartQueue = xQueueCreate(UART_RX_QUEUE_LEN, sizeof(uint8_t));
  if (uartQueue == NULL) {
    app_log("Failed to create UART RX queue\n");
  }
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
  BaseType_t ret;
  ret = xTaskCreate(
    cli_task,
    CLI_TASK_NAME,
    CLI_TASK_STACK_SIZE,
    NULL,
    CLI_TASK_PRIO,
    &cli_task_handle);
  if (ret != pdPASS) {
    app_log("Failed to create CLI task\n");
  }
  else {
    app_log("CLI task created successfully\n");
  }
  reset_variables();
}

// Application Process Action.
void app_process_action(void)
{
  // if (app_is_process_required()) {
  //   /////////////////////////////////////////////////////////////////////////////
  //   // Put your additional application code here!                              //
  //   // This is will run each time app_proceed() is called.                     //
  //   // Do not call blocking functions from here!                               //
  //   /////////////////////////////////////////////////////////////////////////////
  // }
  char ch;
  if(STATE_SPP_MODE == main_state) {
    while (xQueueReceive(uartQueue, &ch, 0) == pdPASS) {
      b[len++] = ch;
      if (len == 20 || ch == '\n') {
        sl_bt_gatt_server_send_notification(
          conn_handle,
          gattdb_My_SPP_Write,
          len,
          b);
        sent_len += len;
        len = 0;
      }
    }
    // else {
    //   // No data received from UART
    //   vTaskDelay(pdMS_TO_TICKS(10)); // Sleep for a while to avoid busy waiting
    // }
  }
  return;
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the default weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      conn_handle = evt->data.evt_connection_opened.connection;
      app_log("Connection opened\r\n");
      main_state = STATE_CONNECTED;

      sl_bt_connection_set_parameters(conn_handle,
                                   24,  // min. connection interval (milliseconds * 1.25)
                                   40,  // max. connection interval (milliseconds * 1.25)
                                   0,   // latency
                                   200,
                                   0,
                                   0xFFFF); // supervision timeout (milliseconds * 10)
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      print_stats(&counters);
      if(STATE_SPP_MODE == main_state) {
        sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
      }
      reset_variables();
      // Generate data for advertising
      //sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
      //                                           sl_bt_advertiser_general_discoverable);
      //app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    case sl_bt_evt_gatt_server_characteristic_status_id: //sl_bt_evt_gatt_server_user_write_request_id:
      // Write to the client
        {
          sl_bt_evt_gatt_server_characteristic_status_t char_status;
          char_status = evt->data.evt_gatt_server_characteristic_status;
          app_log("Checkpoint 3\n");
          if (char_status.characteristic == gattdb_My_SPP_Write) {
            if (char_status.status_flags == sl_bt_gatt_server_client_config) {
              // Characteristic client configuration (CCC) for spp_data has been
              //   changed
              if (char_status.client_config_flags
                  == sl_bt_gatt_server_notification) {
                main_state = STATE_SPP_MODE;
                sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
                app_log("SPP Mode ON\r\n");
              } else {
                app_log("SPP Mode OFF\r\n");
                main_state = STATE_CONNECTED;
                sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
              }
            }
          }
        }
        break;

        case sl_bt_evt_gatt_server_attribute_value_id: //sl_bt_evt_gatt_server_user_read_request_id:
        {
          // Read from the client
          app_log("Checkpoint 1: %s", evt->data.evt_gatt_server_attribute_value.value.data);
          if (evt->data.evt_gatt_server_attribute_value.value.len != 0) {
            for (uint8_t i = 0;
                 i < evt->data.evt_gatt_server_attribute_value.value.len; i++) {
              sl_iostream_putchar(
                sl_iostream_vcom_handle,
                evt->data.evt_gatt_server_attribute_value.value.data[i]);
            }
            counters.num_pack_received++;
            counters.num_bytes_received +=
              evt->data.evt_gatt_server_attribute_value.value.len;
          }
        }
        break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}
