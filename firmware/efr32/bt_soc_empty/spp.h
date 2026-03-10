/*
 * spp.h
 *
 *  Created on: 14 août 2025
 *      Author: rossa
 */

#ifndef SPP_H_
#define SPP_H_

/*******************************************************************************
 *    Local Macros and Definitions
 ******************************************************************************/

/* Set here the operation mode of the SPP device: */
#define SPP_SERVER_MODE    0
#define SPP_CLIENT_MODE    1

#define SPP_OPERATION_MODE SPP_SERVER_MODE

/*Main states */
#define DISCONNECTED       0
#define SCANNING           1
#define FIND_SERVICE       2
#define FIND_CHAR          3
#define ENABLE_NOTIF       4
#define DATA_MODE          5
#define DISCONNECTING      6

#define STATE_ADVERTISING  1
#define STATE_CONNECTED    2
#define STATE_SPP_MODE     3

// SPP service UUID: 4880c12c-fdcb-4077-8920-a450d7f9b907
extern const uint8_t serviceUUID[16];

// SPP data UUID: fec26ec4-6d71-4442-9f81-55bc21d658d6
extern const uint8_t charUUID[16];

/* maximum number of iterations when polling UART RX data before sending data
 *   over BLE connection
 * set value to 0 to disable optimization -> minimum latency but may decrease
 *   throughput */
#define UART_POLL_TIMEOUT  5000

/*Bookkeeping struct for storing amount of received/sent data  */
typedef struct
{
  uint32_t num_pack_sent;
  uint32_t num_bytes_sent;
  uint32_t num_pack_received;
  uint32_t num_bytes_received;
  uint32_t num_writes; /* Total number of send attempts */
} ts_counters;

/* Common local functions*/
void print_stats(ts_counters *p_counters);
void reset_variables();
void send_spp_data();  //uint8_t *);

/******************************************************************************
 *    Local Variables
 ******************************************************************************/

// The advertising set handle allocated from Bluetooth stack.
//extern uint8_t advertising_set_handle;
extern uint8_t conn_handle;
extern uint8_t main_state;
extern uint32_t service_handle;
extern uint16_t char_handle;

extern ts_counters counters;

// Default maximum packet size is 20 bytes. This is adjusted after connection is
// opened based on the connection parameters
extern uint8_t max_packet_size;
extern uint8_t min_packet_size;  // Target minimum bytes for one packet

#endif /* SPP_H_ */
