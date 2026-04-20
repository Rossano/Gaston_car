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
#include "task.h"
#include "app.h"

#include "app_log.h"
#include "app_assert.h"

#include "spp.h"


/* maximum number of iterations when polling UART RX data before sending data
 *   over BLE connection
 * set value to 0 to disable optimization -> minimum latency but may decrease
 *   throughput */
#define UART_POLL_TIMEOUT  5000

/******************************************************************************
 *    Local Variables
 ******************************************************************************/

// The advertising set handle allocated from Bluetooth stack.
uint8_t advertising_set_handle = 0xff;
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

void send_spp_data(uint8_t *data, uint8_t len)
{
  sl_status_t result;

  if (len == 0) {
    return;
  }
  app_log("Checkpoint 4: entering spp_send_data(len=%d)\r\n", len);

  if(conn_handle != 0xFF) {    
    // Stack may return "out-of-memory" (SL_STATUS_NO_MORE_RESOURCE) error if
    //   the local buffer is full -> in that case, just keep trying until the
    //   command succeeds
    do {
      result = sl_bt_gatt_server_send_notification(conn_handle,
                                                  gattdb_My_SPP_Write, //gattdb_spp_data,
                                                  len,
                                                  data);
      counters.num_writes++;
      
      if (result == SL_STATUS_NO_MORE_RESOURCE) {
        // Buffer pieno: aspetta 1 tick (o 1ms) per lasciare tempo allo stack di trasmettere
        vTaskDelay(pdMS_TO_TICKS(1));
      }
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