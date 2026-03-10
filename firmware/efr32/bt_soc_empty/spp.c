/*
 * spp.c
 *
 *  Created on: 14 août 2025
 *      Author: rossa
 */

#include <stdint.h>
#include "sl_common.h"
#include "sl_iostream_handles.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"

#include "app_log.h"
#include "app_assert.h"

#include "spp.h"

//// SPP service UUID: 4880c12c-fdcb-4077-8920-a450d7f9b907
//const uint8_t serviceUUID[16] = { 0x07,
//                                  0xb9,
//                                  0xf9,
//                                  0xd7,
//                                  0x50,
//                                  0xa4,
//                                  0x20,
//                                  0x89,
//                                  0x77,
//                                  0x40,
//                                  0xcb,
//                                  0xfd,
//                                  0x2c,
//                                  0xc1,
//                                  0x80,
//                                  0x48 };
//
//// SPP data UUID: fec26ec4-6d71-4442-9f81-55bc21d658d6
//const uint8_t charUUID[16] = { 0xd6,
//                               0x58,
//                               0xd6,
//                               0x21,
//                               0xbc,
//                               0x55,
//                               0x81,
//                               0x9f,
//                               0x42,
//                               0x44,
//                               0x71,
//                               0x6d,
//                               0xc4,
//                               0x6e,
//                               0xc2,
//                               0xfe };

/* maximum number of iterations when polling UART RX data before sending data
 *   over BLE connection
 * set value to 0 to disable optimization -> minimum latency but may decrease
 *   throughput */
#define UART_POLL_TIMEOUT  5000

/*Bookkeeping struct for storing amount of received/sent data  */
/*typedef struct
{
  uint32_t num_pack_sent;
  uint32_t num_bytes_sent;
  uint32_t num_pack_received;
  uint32_t num_bytes_received;
  uint32_t num_writes; //Total number of send attempts
} ts_counters; */


/******************************************************************************
 *    Local Variables
 ******************************************************************************/

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
uint8_t conn_handle = 0xFF;
uint8_t main_state;
uint32_t service_handle;
uint16_t char_handle;

ts_counters counters;

// Default maximum packet size is 20 bytes. This is adjusted after connection is
// opened based on the connection parameters
uint8_t max_packet_size = 20;
uint8_t min_packet_size = 20;  // Target minimum bytes for one packet

void reset_variables()
{
  conn_handle = 0xFF;
  main_state = STATE_ADVERTISING;
  service_handle = 0;
  char_handle = 0;
  max_packet_size = 20;

  memset(&counters, 0, sizeof(counters));
}


void print_stats(ts_counters *p_counters)
{
  app_log("Outgoing data:\r\n");
  app_log(" bytes/packets sent: %lu / %lu ",
          p_counters->num_bytes_sent,
          p_counters->num_pack_sent);
  app_log(", num writes: %lu\r\n", p_counters->num_writes);
  app_log("(RX buffer overflow is not tracked)\r\n");
  app_log("Incoming data:\r\n");
  app_log(" bytes/packets received: %lu / %lu\r\n",
          p_counters->num_bytes_received,
          p_counters->num_pack_received);

  return;
}

void send_spp_data()//uint8_t *buffer)
{
  uint8_t len = 0;
  uint8_t data[256];
  sl_status_t result, read_result;
  uint32_t timeout = 0;
  char c;

  app_log("Checkpoint 4: entering spp_send_data()");
  // Read up to max_packet_size characters from local buffer
  while (len < max_packet_size) {
    read_result = sl_iostream_getchar(sl_iostream_vcom_handle, &c);
    if (SL_STATUS_OK == read_result) {
      data[len++] = (uint8_t)c;
    } else if (len == 0) {
      /* If the first ReadChar() fails then return immediately */
      return;
    } else {
      if (len < CLI_COMMAND_MAX_LEN) {
          data[len] = buffer[len++];
      } else {
        // Speed optimization: if there are some bytes to be sent but the length
        // is still below the preferred minimum packet size, then wait for
        // additional bytes until timeout. Target is to put as many bytes as
        // possible into each air packet.

        // Conditions for exiting the while loop and proceed to send data:
        if (timeout++ > UART_POLL_TIMEOUT) {
          break;
        } else if (len >= min_packet_size) {
          break;
        }
      }
    }
  }

  if (len > 0) {
    // Stack may return "out-of-memory" (SL_STATUS_NO_MORE_RESOURCE) error if
    //   the local buffer is full -> in that case, just keep trying until the
    //   command succeeds
    do {
      result = sl_bt_gatt_server_send_notification(conn_handle,
                                                   gattdb_My_SPP_Write, //gattdb_spp_data,
                                                   len,
                                                   data);
      counters.num_writes++;
      app_log("Check_point 2: %s", data);
    } while (result == SL_STATUS_NO_MORE_RESOURCE);

    if (result != 0) {
      app_log("Unexpected error: %lu\r\n", result);
    } else {
      counters.num_pack_sent++;
      counters.num_bytes_sent += len;
    }
  }
  return;
}

