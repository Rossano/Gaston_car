---
description: "Guidance for spp.c — SPP data transmission, BLE notifications, buffer management, backpressure handling. Use when: debugging data not sent, handling buffer full errors, or optimizing BLE throughput."
applyTo: "**/spp.c"
---

# spp.c — Bluetooth SPP Service

**Role**: Manages BLE Serial Port Profile (SPP) data transmission via GATT notifications. Handles backpressure, retries, and counters.

## Core Function: `send_spp_data()`

```c
void send_spp_data(uint8_t *data, uint8_t len)
{
    sl_status_t result;
    
    if (len == 0) return;  // Sanity check
    
    if (conn_handle != 0xFF) {  // Ensure connected
        do {
            // Send notification on SPP characteristic
            result = sl_bt_gatt_server_send_notification(
                conn_handle,
                gattdb_My_SPP_Write,     // Characteristic handle
                len,
                data);
            
            counters.num_writes++;
            
            // Backpressure handling: retry if buffer full
            if (result == SL_STATUS_NO_MORE_RESOURCE) {
                vTaskDelay(pdMS_TO_TICKS(1));  // Wait 1ms for stack to drain
            }
        } while (result == SL_STATUS_NO_MORE_RESOURCE);
        
        // Error handling
        if (result != 0) {
            app_log("SPP send error: %lu\r\n", result);
        } else {
            counters.num_pack_sent++;
            counters.num_bytes_sent += len;
        }
    }
}
```

## Key Concepts

### 1. Connection Handle (`conn_handle`)

- **Value**: Set by `app.c::sl_bt_evt_connection_opened_id`
- **Invalid state**: `0xFF` means no active connection
- **Check before sending**: `if (conn_handle != 0xFF)` — always do this

### 2. Characteristic Handle (`gattdb_My_SPP_Write`)

- **Auto-generated**: Defined in `autogen/gatt_db.h`
- **Type**: Writable+Notifiable characteristic
- **Purpose**: Client writes commands here; device sends responses via notifications
- **Do NOT hardcode**: Always use symbol from `gatt_db.h`

### 3. Backpressure & Retry Logic

**Problem**: BLE stack buffer can fill if sending too fast.

**Response**: `SL_STATUS_NO_MORE_RESOURCE` error
```c
// BLE stack says "I'm busy, try again later"
if (result == SL_STATUS_NO_MORE_RESOURCE) {
    vTaskDelay(1);  // Let stack drain, retry immediately after
}
```
**Trade-off**: This loop can block `cli_task` briefly. If problematic:
- Increase delay: `vTaskDelay(pdMS_TO_TICKS(5))` for more breathing room
- Or implement queue-based retry (advanced)

### 4. Data Counters

```c
ts_counters counters;  // Tracks stats

typedef struct {
    uint32_t num_bytes_sent;     // Total bytes transmitted
    uint32_t num_pack_sent;      // Total packets (separate sends)
    uint32_t num_writes;         // Total write attempts (incl. retries)
    uint32_t num_bytes_received; // From BLE RX
    uint32_t num_pack_received;
} ts_counters;
```

- **Useful for debugging**: Verify data actually sent via `print_stats()`
- **Reset on disconnect**: Call `reset_variables()` in `app.c::connection_closed`

## Packet Size Management

```c
uint8_t max_packet_size = 20;    // MTU-negotiated max per packet
uint8_t min_packet_size = 20;    // Target minimum for batching
```

### MTU Negotiation
- **Default**: 20 bytes
- **Updated**: After connection, BLE stack negotiates MTU up to device capability
- **Impact**: Larger MTU → fewer packets, lower overhead
- **Do NOT override**: Respect the negotiated value

### Batching Strategy
- If sending multiple small messages, batch them to reach `min_packet_size` for efficiency
- Example: 4× 5-byte commands → 1× 20-byte packet (if allowed by protocol)

## State & Initialization

```c
void reset_variables() {
    conn_handle = 0xFF;
    main_state = STATE_ADVERTISING;
    service_handle = 0;
    char_handle = 0;
    max_packet_size = 20;
    memset(&counters, 0, sizeof(counters));
}
```

**When to call**: 
- On boot (before any BLE activity)
- On disconnect (before re-advertising)
- **NOT during normal operation**

## Debug & Logging

### Print Statistics
```c
print_stats(&counters);  // Call from console command or periodic timer
```
Output example:
```
Outgoing data:
 bytes/packets sent: 1024 / 102, num writes: 105
Incoming data:
 bytes/packets received: 512 / 26
```

### Common Errors

| Error | Likely Cause | Action |
|-------|--------------|--------|
| `SL_STATUS_INVALID_HANDLE` | `conn_handle = 0xFF` (not connected) | Wait for connection opened event |
| `SL_STATUS_NO_MORE_RESOURCE` | BLE stack buffer full | Retry with backpressure (already handled) |
| `SL_STATUS_INVALID_STATE` | Stack not ready | Check `system_boot` completed |
| `SL_STATUS_INVALID_PARAMETER` | Bad characteristic handle | Verify `gattdb_My_SPP_Write` exists |

## Optimization Notes

### Latency vs. Throughput

**Current approach**: Send-on-newline (from `cli.c`)
- Pros: Simple, low CPU overhead
- Cons: Variable latency based on user input

**Alternative**: Timeout-based batching
```c
// Send after timeout OR buffer full
#define SEND_TIMEOUT_MS 10
if (buffer_full || (now - last_send) > SEND_TIMEOUT_MS) {
    send_spp_data(buffer, len);
}
```

### Power Consumption

- Backpressure loop uses `vTaskDelay()` → allows other tasks to run
- Frequent small notifications → more overhead than batched sends
- Investigate if power budget require changes

## Integration with Other Files

| File | Interaction |
|------|-------------|
| `app.c` | Filled by `sl_bt_on_event()` when connection opens; handle stored in `conn_handle` |
| `cli.c` | Calls `send_spp_data()` when VCOM newline received |
| `autogen/gatt_db.h` | Provides `gattdb_My_SPP_Write` handle |
| `app_freertos.c` | Sets up task/mutex (does NOT directly call SPP functions except in event handler) |

