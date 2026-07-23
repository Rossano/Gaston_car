#ifndef MCU_UART_BRIDGE_H
#define MCU_UART_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#include "sl_status.h"

void mcu_uart_init(void);
sl_status_t mcu_uart_send(const uint8_t *data, size_t len);

#endif