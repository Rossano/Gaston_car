---
description: "Guidance for cli.c — UART ↔ BLE bidirectional bridge, queue management. Use when: implementing serial I/O, debugging UART data flow, queue overflow, or improving throughput."
applyTo: "**/cli.c"
---

# cli.c — UART ↔ BLE Bridge Task

**Role**: Bidirectional data bridge between VCOM (serial console) and BLE SPP characteristic. Runs as a dedicated FreeRTOS task.

## Data Flow Architecture

```
VCOM Input (UART)
    ↓ sl_iostream_getchar()
    ↓ Buffer & collect until newline
    ↓ send_spp_data() → BLE Notification
    
BLE RX (from remote)
    ↓ Queue (uartQueue)
    ↓ Dequeue in cli_task
    ↓ sl_iostream_putchar() → VCOM Output
```

## Task Loop (main implementation)

```c
void cli_task(void *pvParameters) {
    for (;;) {
        handled_data = 0;
        
        // Direction 1: VCOM → BLE
        if (sl_iostream_getchar(...) == SL_STATUS_OK) {
            // Collect until newline
            if (ch == '\n' || ch == '\r') {
                send_spp_data(buffer, len);  // Send via BLE
                len = 0;
            } else {
                buffer[len++] = ch;
            }
            handled_data = 1;
        }
        
        // Direction 2: BLE → VCOM
        if (xQueueReceive(uartQueue, &byte, 0) == pdPASS) {
            sl_iostream_putchar(sl_iostream_vcom_handle, byte);
            handled_data = 1;
        }
        
        // Yield CPU if idle
        if (!handled_data) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
```

## Critical Points

### 1. Queue-Based Reception (BLE → VCOM)
- **Source**: `app_freertos.c` fills `uartQueue` when BLE data arrives
- **Polling**: `cli_task` dequeues non-blocking: `xQueueReceive(uartQueue, ..., 0)`
- **Timeout**: Zero timeout = non-blocking; won't stall the bridge
- ⚠️ **Risk**: Queue overflow → data loss. Monitor with `uxQueueMessagesWaiting()`

### 2. VCOM Input (VCOM → BLE)
- **Blocking behavior**: `sl_iostream_getchar()` returns `SL_STATUS_OK` only if data ready
- **Buffering**: Accumulate bytes until newline (`\n` or `\r`), then send
- **Buffer limit**: `CLI_COMMAND_MAX_LEN` — prevent overflow by checking `len < limit - 1`

### 3. CPU Efficiency
- **Busy-wait danger**: If always calling `getchar()`, CPU spins → high power
- **Solution**: `vTaskDelay(10ms)` only when both queues are empty → yields to other tasks
- **Trade-off**: 10ms latency vs. CPU idle time

## Queue Handling

### `uartQueue` — BLE-to-UART (RX from BLE)
```c
extern QueueHandle_t uartQueue;
```
- **Size**: `UART_RX_QUEUE_LEN` (check `cli.h`), 1 byte per element
- **Production**: Filled by BLE event handler or `app_freertos.c` in mutex-protected code
- **Consumption**: Dequeued in `cli_task()` non-blocking
- **Monitoring**: Use `uxQueueMessagesWaiting(uartQueue)` to detect bottlenecks

### `cli_queue` — CLI Commands (for future use)
```c
extern QueueHandle_t cli_queue;
```
- Currently unused; reserved for extended command processing
- Follows same pattern as `uartQueue` if needed

## Buffer Management

- **`buffer[]`**: Stores VCOM characters until newline
- **`len`**: Current fill level
- **Max size**: `CLI_COMMAND_MAX_LEN` (typically 128 or 256)
- **Overflow check**: `if (len >= CLI_COMMAND_MAX_LEN - 1)` → drop character or send early

## Debugging Checklist

- **No data from VCOM → BLE**:
  - Is `send_spp_data()` being called? Add log before it.
  - Is there a connection? Check `conn_handle != 0xFF` in `spp.c`.
  - Buffer size? Try sending very short messages first.

- **No data from BLE → VCOM**:
  - Is the remote device sending? Check BLE app (Simplicity Connect).
  - Queue empty? Call `uxQueueMessagesWaiting(uartQueue)` to check depth.
  - Is dequeue working? Add `app_log()` after `xQueueReceive()`.

- **VCOM output garbled**:
  - Check UART baud rate in `config/pin_config.h` (should match terminal).
  - Verify `sl_iostream_vcom_handle` is initialized.

- **High latency**:
  - The 10ms delay is intentional for power saving. If unacceptable, reduc:
    ```c
    if (!handled_data) {
        vTaskDelay(pdMS_TO_TICKS(1));  // 1ms instead of 10ms
    }
    ```
  - But monitor power consumption after change.

## Performance Tuning

### Throughput Bottlenecks
1. **Queue depth**: If `uartQueue` fills up, increase `UART_RX_QUEUE_LEN`
2. **SPP MTU**: Limited by BLE packet size (default 20 bytes); see `spp.c::max_packet_size`
3. **Newline handling**: Buffering until newline reduces throughput; consider binary mode if needed

### Memory Pressure
- Stack size: 1024 bytes (see `app_freertos.c::xTaskCreate()`)
- If overflow: Increase stack or reduce buffer sizes
- Check `FreeRTOS::uxTaskGetStackHighWaterMark()` in debugger

## Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| Data loss | Queue overflow | Increase `UART_RX_QUEUE_LEN` or reduce BLE data rate |
| Latency spikes | 10ms delay blocking | Reduce delay, trade CPU for responsiveness |
| Garbled UART | Baud rate mismatch | Verify `pin_config.h` matches terminal setting |
| Task stalls | Deadlock in getchar | Rarely happens; if so, add timeout to `getchar()` |

