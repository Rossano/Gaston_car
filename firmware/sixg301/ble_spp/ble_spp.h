#ifndef __BLE_SPP__
#define __BLE_SPP__

#include <stdint.h>
#include <stdbool.h>

#include "cmsis_os2.h"
#include "sl_status.h"
#include "uart_bridge/uart_mcu.h"

/*
 *  Module Global Defines
 */
#define MCU_UART_TX_BUFFER_SIZE     256u    //1024u
#define MCU_UART_RX_BUFFER_SIZE     128u

/*
 *  Message data structure Prototype
 */
// typedef struct {
//     uint16_t length;
//     uint8_t data[MCU_UART_TX_BUFFER_SIZE];
// } uart_tx_message_t;
// typedef struct {
//     uint16_t length;
//     uint8_t data[MCU_UART_RX_BUFFER_SIZE];
// } uart_rx_message_t;

extern uart_tx_message_t tx_msg;            // MCU Uart Tx message
extern uart_rx_message_t rx_msg;            // MCU Uart Rx message
extern osMessageQueueId_t ble_rx_queue;    // MCU -> BLE Rx Queue

/*
 *  Function Prototypes
 */
void ble_uart_init(osMessageQueueId_t queue);
//static void ble_tx_task(void *pvargs);
#if MCU_UART_RX_ENABLED
sl_status_t mcu_uart_test_receive(const uint8_t *date, size_t len);
#endif

#endif