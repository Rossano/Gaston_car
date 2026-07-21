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

#include "sl_bluetooth.h"
#include "gatt_db.h" 
#include "app_log.h"
#include "sl_status.h"
#include <stdint.h>

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static uint8_t ble_connection = 0xff;
static bool notifications_enabled = false;

// Application Init.
void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

// Application Process Action.
void app_process_action(void)
{
  if (app_is_process_required()) {
    /////////////////////////////////////////////////////////////////////////////
    // Put your additional application code here!                              //
    // This is will run each time app_proceed() is called.                     //
    // Do not call blocking functions from here!                               //
    /////////////////////////////////////////////////////////////////////////////
  }
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
      ble_connection = evt->data.evt_connection_opened.connection;
      notifications_enabled = false;

      app_log("BLE Connected handle=%u\r\n", ble_connection);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Generate data for advertising
      // sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
      //                                            sl_bt_advertiser_general_discoverable);
      // app_assert_status(sc);

      // // Restart advertising after client has disconnected.
      // sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
      //                                    sl_bt_legacy_advertiser_connectable);
      // app_assert_status(sc);
      
      app_log("BLE disconnected!\r\n");
      ble_connection = 0xff;
      notifications_enabled = false;
      sl_bt_legacy_advertiser_generate_data(
          advertising_set_handle,
          sl_bt_advertiser_general_discoverable);

      sl_bt_legacy_advertiser_start(
          advertising_set_handle,
          sl_bt_legacy_advertiser_connectable);
      app_log("BLE starting advertising\r\n");
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    case sl_bt_evt_gatt_server_user_write_request_id:
      const sl_bt_evt_gatt_server_user_write_request_t *write_evt = &evt->data.evt_gatt_server_user_write_request;
      uint16_t len = write_evt->value.len;

      if (evt->data.evt_gatt_server_user_write_request.characteristic == gattdb_My_SPP_Write) {

        const uint8_t *data = evt->data.evt_gatt_server_user_write_request.value.data;
        uint16_t len = evt->data.evt_gatt_server_user_write_request.value.len;

        // TODO: mettere data/len nel buffer BLE -> UART
        app_log("Write Request BLE Rx, opcode = 0x%02X, len=%u", write_evt->att_opcode, len);
        for(uint8_t i=0; i < len; i++) {
          app_log("%02X ", data[i]);
        }
        app_log("\r\n");

        // Only answer if operation requires an answer
        // Not to send Write Without Response 
        if (write_evt->att_opcode == sl_bt_gatt_write_response) {
          sc = sl_bt_gatt_server_send_user_write_response(
              evt->data.evt_gatt_server_user_write_request.connection,
              gattdb_My_SPP_Write,
              SL_STATUS_OK);
          //(void)sc;
          app_assert_status(sc);
        }
      }
      break;

    case sl_bt_evt_gatt_server_attribute_value_id:
      if (evt->data.evt_gatt_server_attribute_value.attribute == gattdb_My_SPP_Write) {

        const uint8_t *data = evt->data.evt_gatt_server_attribute_value.value.data;
        uint16_t len = evt->data.evt_gatt_server_attribute_value.value.len;

        // TODO: mettere data/len nel buffer BLE -> UART
        //(void)data;
        //(void)len;
        app_log("BLE Rx, len=%u: ", len);
        for(uint8_t i=0; i < len; i++) {
          app_log("%02X ", data[i]);
        }
        app_log("\r\n");
      }
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_My_SPP_Read) {

        if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config) {

          notifications_enabled = (evt->data.evt_gatt_server_characteristic_status.client_config_flags & sl_bt_gatt_notification) != 0;

          app_log("Notifications: %s\r\n", notifications_enabled ? "enabled" : "disabled");

          if(notifications_enabled) {
            static const uint8_t test_msg[] = "SigxG301 Ready\r\n";
            sl_status_t sc = sl_bt_gatt_server_send_notification(ble_connection, gattdb_My_SPP_Read, sizeof(test_msg)-1, test_msg);
            app_log_status_error(sc);
            app_log(test_msg);
            app_log("Notification result: 0x%081X\r\n", (unsigned long)sc);
          }
        }
        else app_log("Notifications: evt not checked");
      }
      break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}
