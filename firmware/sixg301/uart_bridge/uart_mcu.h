#ifndef MCU_UART_BRIDGE_H
#define MCU_UART_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#include "sl_status.h"

#define MCU_UART_TX_BUFFER_SIZE     256u    //1024u
#define MCU_UART_RX_BUFFER_SIZE     128u

/*
 *  Message data structure Prototype
 */
typedef struct {
    uint16_t length;
    uint8_t data[MCU_UART_TX_BUFFER_SIZE];
} uart_tx_message_t;

extern uart_tx_message_t msg;           // MCU Uart Tx message

/*
 *  Exported function prototypes
 */
void mcu_uart_init(void);
sl_status_t mcu_uart_send(const uint8_t *data, size_t len);

#endif