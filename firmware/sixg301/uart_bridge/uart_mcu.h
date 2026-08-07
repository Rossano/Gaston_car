#ifndef MCU_UART_BRIDGE_H
#define MCU_UART_BRIDGE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "sl_status.h"
#include "cmsis_os2.h"

/*
 *  Module Global Defines
 */
#define MCU_UART_TX_BUFFER_SIZE     256u    //1024u
#define MCU_UART_RX_BUFFER_SIZE     128u

#define MCU_UART_RX_ENABLED             0

/*
 *  Message data structure Prototype
 */
typedef struct {
    uint16_t length;
    uint8_t data[MCU_UART_TX_BUFFER_SIZE];
} uart_tx_message_t;
typedef struct {
    uint16_t length;
    uint8_t data[MCU_UART_RX_BUFFER_SIZE];
} uart_rx_message_t;

extern uart_tx_message_t tx_msg;            // MCU Uart Tx message
extern uart_rx_message_t rx_msg;            // MCU Uart Rx message
extern osMessageQueueId_t uart_rx_queue;    // MCU -> BLE Rx Queue
/*
 *  Exported function prototypes
 */
void mcu_uart_init(void);
sl_status_t mcu_uart_send(const uint8_t *data, size_t len);
void mcu_uart_set_ble_state(uint8_t connection, bool notifications_enabled);

#endif