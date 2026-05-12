/***************************************************************************//**
 * @file
 * @brief Core BLE application logic.
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
#include "sl_bt_api.h"
#include "app_assert.h"
#include "app_log.h"
#include "app.h"
#include "gatt_db.h"
#include "spp.h"
#include "cli.h"

// BLE state variables
uint8_t advertising_set_handle = 0xFF;
uint8_t conn_handle = 0xFF;
uint8_t main_state = STATE_ADVERTISING;

/**************************************************************************//**
 * Application Init - called once at startup.
 *****************************************************************************/
void app_init(void)
{
  app_log("=== BLE+FreeRTOS Application Started ===\n");
// app_init_bt();  // Initialize FreeRTOS tasks and BLE resources
}

/**************************************************************************//**
 * Application Process Action - called periodically.
 *****************************************************************************/
void app_process_action(void)
{
  // Empty - BLE events are processed by sl_bt_on_event()
  // This is kept for future extension.
  app_log("app_process_action: called\n");
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
    // -------------------------------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      app_log("BLE System BOOT event received\n");

      // Create an advertising set
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);
      app_log("Advertising set created: %d\n", advertising_set_handle);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160,  // min. adv. interval (milliseconds * 1.6)
        160,  // max. adv. interval (milliseconds * 1.6)
        0,    // adv. duration
        0);   // max. num. adv. events
      app_assert_status(sc);

      // Start advertising and enable connections
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      app_log("BLE Advertising started\n");
      main_state = STATE_ADVERTISING;
      break;

    // -------------------------------------------------------
    // This event indicates that a new connection was opened
    case sl_bt_evt_connection_opened_id:
      conn_handle = evt->data.evt_connection_opened.connection;
      main_state = STATE_CONNECTED;
      app_log("BLE Connection OPENED: handle=%d\n", conn_handle);
      break;

    // -------------------------------------------------------
    // This event indicates that a connection was closed
    case sl_bt_evt_connection_closed_id:
      app_log("BLE Connection CLOSED\n");
      conn_handle = 0xFF;
      main_state = STATE_ADVERTISING;

      // Restart advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      app_log("BLE Advertising restarted\n");
      break;

    // -------------------------------------------------------
    // Default event handler
    default:
      break;
  }

  // Signal the FreeRTOS task that BLE event has been processed
  app_proceed();
}

// Function to send CLI output over BLE
void send_cli_data_over_ble(const char *data, size_t len)
{
  // Use the shared connection handle defined in 'spp.c' (extern in 'spp.c').
  // 'conn_handle' is set on connection open and cleard on connection close.
  if(conn_handle != 0xFF) {
    // send notification return sl_status_t
    sl_status_t res = sl_bt_gatt_server_send_notification(conn_handle,
                                               gattdb_My_SPP_Write,
                                               (uint16_t)len,
                                               (const uint8_t *)data);
    if(res != SL_STATUS_OK) {
      app_log("Failed to send notification: 0x%02X\n", res);
    }
  } else {
    app_log("No active connection, cannot send CLI output over BLE\n");
  }
  /*
    if (main_state == STATE_SPP_MODE) {
    send_spp_data((uint8_t *)msg, strlen(msg));
  }
    */
}

void send_cli_output(const char *msg)
{
  size_t len = strlen(msg);
  // Send over VCOM
  for (size_t i = 0; i < len; i++) {
    sl_iostream_putchar(sl_iostream_vcom_handle, msg[i]);
  }
  // Send over BLE
  send_cli_data_over_ble(msg, strlen(msg));
}

