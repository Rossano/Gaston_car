#ifndef MCU_UART_BRIDGE_H
#define MCU_UART_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#include "sl_status.h"

//#include "cmsis_os2.h"
//static osMessageQueueId_t uart_tx_queue;
#define MCU_UART_TX_BUFFER_SIZE     256u    //1024u
#define MCU_UART_RX_BUFFER_SIZE     128u

typedef struct {
    uint16_t lenght;
    uint8_t data[MCU_UART_TX_BUFFER_SIZE];
} uart_tx_message_t;

extern uart_tx_message_t msg;

void mcu_uart_init(void);
sl_status_t mcu_uart_send(const uint8_t data[], size_t len);

#endif