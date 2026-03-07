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
#include "sl_common.h"
#include "sl_iostream_handles.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"

#include "app_log.h"
#include "sl_bt_api.h"
#include "sl_main_init.h"
#include "app_assert.h"
#include "sl_simple_led_instances.h"

#include "spp.h"
#include "cli.h"
#include <string.h>

#define SL_BT_RTOS_APPLICATION_PRIORITY    10u

QueueHandle_t cli_queue;

uint8_t buffer[CLI_COMMAND_MAX_LEN];
uint8_t buffer_stm32[CLI_COMMAND_MAX_LEN];
__ALIGNED(8) static StackType_t thread_application_stk[250 & 0xFFFFFFF8u];
__ALIGNED(4) static StaticTask_t thread_application_cb;
uint8_t led0_state = 0;
uint16_t sent_len = 0;
uint8_t b[20], len = 0;
uint8_t len_stm32 = 0;

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

}

// Application Process Action.
void app_process_action(void)
{ 
  char ch;
  
  if (STATE_SPP_MODE == main_state) {
    /////////////////////////////////////////////////////////////////////////////
    // Put your additional application code here!                              //
    // This is will run each time app_proceed() is called.                     //
    // Do not call blocking functions from here!                               //
    /////////////////////////////////////////////////////////////////////////////
    uint8_t handled = 0;

    if (sl_iostream_getchar(sl_iostream_vcom_handle, &ch) == SL_STATUS_OK) {
      handled = 1;
      // check if it is a EOL
      if ((ch != '\n' && ch != '\r') && len < CLI_COMMAND_MAX_LEN-1) {
        // store the char only if the buffer is not full
        buffer[len++] = (uint8_t)ch;
      }
      else {
        // EOL or buffer full, could handle overflow here
        // Send the buffer only if BLE is connected and buffer is not empty
        send_spp_data(buffer, len);
        for (int i = 0; i < CLI_COMMAND_MAX_LEN-1; i++) {
          buffer[i] = 0;
        }
        if (len == CLI_COMMAND_MAX_LEN-1) {
          buffer[0] = (uint8_t)ch; // store the last char if buffer was full
          len = 1; // reset if buffer was full
        } else {
          len = 0; // reset normally
        }
      }
    }

    /* Poll anche dallo stream STM32 */
    if (sl_iostream_getchar(sl_iostream_stm32_handle, &ch) == SL_STATUS_OK) {
      handled = 1;
      if ((ch != '\n' && ch != '\r') && len_stm32 < CLI_COMMAND_MAX_LEN-1) {
        buffer_stm32[len_stm32++] = (uint8_t)ch;
      } else {
        send_spp_data(buffer_stm32, len_stm32);
        for (int i = 0; i < CLI_COMMAND_MAX_LEN-1; i++) {
          buffer_stm32[i] = 0;
        }
        if (len_stm32 == CLI_COMMAND_MAX_LEN-1) {
          buffer_stm32[0] = (uint8_t)ch;
          len_stm32 = 1;
        } else {
          len_stm32 = 0;
        }
      }
    }

    if (!handled) {
      // non c'erano dati: attendi un po' per non usare CPU al 100%
      vTaskDelay(pdMS_TO_TICKS(10));
    }
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
  uint16_t max_mtu_out;
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      app_log("SPP Role: SPP Server\r\n");
      reset_variables();
      sc = sl_bt_gatt_server_set_max_mtu(247, &max_mtu_out);
      app_assert_status(sc);
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

      // Request connection parameter update.
      // conn.interval min 20ms, max 40ms, slave latency 4 intervals,
      // supervision timeout 2 seconds
      // (These should be compliant with Apple Bluetooth Accessory Design
      // Guidelines, both R7 and R8)
      sl_bt_connection_set_parameters(conn_handle, 24, 40, 0, 200, 0, 0xFFFF);
      break;

    case sl_bt_evt_connection_parameters_id:
      app_log("Conn.parameters: interval %u units\r\n",
              evt->data.evt_connection_parameters.interval);
      break;

    case sl_bt_evt_gatt_mtu_exchanged_id:
      // Calculate maximum data per one notification / write-without-response,
      // this depends on the MTU. up to ATT_MTU-3 bytes can be sent at once.
      max_packet_size = evt->data.evt_gatt_mtu_exchanged.mtu - 3;

      /* Try to send maximum length packets whenever possible */
      min_packet_size = max_packet_size;
      app_log("MTU exchanged: %d\r\n", evt->data.evt_gatt_mtu_exchanged.mtu);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      print_stats(&counters);
      if (STATE_SPP_MODE == main_state) {
        sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
      }
      reset_variables();
      // Generate data for advertising
      // sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
      //                                            sl_bt_advertiser_general_discoverable);
      // app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    ///
    /// My SPP Service
    ///
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
              uint8_t *data = evt->data.evt_gatt_server_attribute_value.value.data;
              uint8_t len = evt->data.evt_gatt_server_attribute_value.value.len;
              app_log("Checkpoint 1: received %u bytes\r\n", len);
              if (len != 0) {
                for (uint8_t i = 0; i < len; i++) {
                  sl_iostream_putchar(sl_iostream_vcom_handle, data[i]);
                }
                counters.num_pack_received++;
                counters.num_bytes_received += len;
              }
            }
            break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

// Function to send CLI output over BLE
void send_cli_data_over_ble(const char* data, size_t len)
{
  // Use the shared connection handle defined in `spp.c` (extern in `spp.h`).
  // `conn_handle` is set on connection open and cleared on close.
  if (conn_handle != 0xFF) {
    // send_notification returns sl_status_t
    sl_status_t res = sl_bt_gatt_server_send_notification(conn_handle,
                                                          gattdb_My_SPP_Write,
                                                          (uint16_t)len,
                                                          (const uint8_t *)data);
    if (res != SL_STATUS_OK) {
      app_log("send_cli_data_over_ble: notify failed: 0x%04x\r\n", res);
    }
  } else {
    app_log("send_cli_data_over_ble: no connection\r\n");
  }
}

void send_cli_output(const char *msg)
{
  size_t len = strlen(msg);
  // Send over VCOM
  for (size_t i = 0; i < len; i++) {
    sl_iostream_putchar(sl_iostream_vcom_handle, msg[i]);
  }
  // Send over BLE SPP
  send_cli_data_over_ble(msg, len);
}