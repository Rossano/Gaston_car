---
name: debug-ble-serial-bridge
description: "Systematically debug BLE/SPP communication issues (data loss, disconnects, slow throughput). Use when: investigating why data doesn't arrive, connection drops, or unexpected latency."
---

# Debug BLE Serial Bridge

Structured debugging workflow for EFR32 Gaston Car firmware BLE↔UART bridge.

## Problems This Helps With

- ✓ **Data Loss**: Sent from phone but never appears on UART
- ✓ **Disconnect Loops**: Connection closes immediately or after N seconds
- ✓ **Slow Throughput**: High latency or data blocked
- ✓ **No Connection**: Device not visible or won't accept connections
- ✓ **Queue Overflow**: `uartQueue` fills and overflows
- ✓ **Thread Safety**: ESP32 crashes or hangs with mutex errors

## Workflow

### 1. Symptom Classification

Ask yourself:
- **When does it happen?** (on startup, after connect, with large data?)
- **Is it intermittent?** (happens 50% of time, or always?)
- **What changed?** (code, config, hardware, distance?)

Use the matrix below to narrow down the component:

| Symptom | Likely Component |
|---------|------------------|
| Device won't advertise | `app.c::system_boot` handler |
| Won't accept connection | GATT database or advertising state |
| Connects then disconnects | `app.c::connection_closed` not restarting advertising |
| Data not sent via BLE | `spp.c::send_spp_data()` or no connection |
| Data from BLE not on UART | `uartQueue` overflow or `cli_task` stalled |
| High CPU usage | Busy-wait in `cli_task` or infinite retry loop |
| Crashes with mutex error | Unprotected BLE call outside `sl_bt_on_event()` |

### 2. Enable Debug Logging

Add log points to trace data flow:

```c
// In app.c::sl_bt_on_event() for each case:
case sl_bt_evt_system_boot_id:
    app_log("🔧 system_boot: Creating advertising set\n");
    // ... existing code ...

case sl_bt_evt_connection_opened_id:
    app_log("✅ connection_opened: handle=%d\n", conn_handle);

case sl_bt_evt_connection_closed_id:
    app_log("❌ connection_closed: Restarting advertising\n");
```

```c
// In cli.c::cli_task() at data boundaries:
if (sl_iostream_getchar(...) == SL_STATUS_OK) {
    app_log("VCOM RX: '%c' (0x%02x)\n", ch, ch);
}

if (xQueueReceive(uartQueue, &byte_from_queue, 0) == pdPASS) {
    app_log("BLE→VCOM: 0x%02x\n", byte_from_queue);
}
```

```c
// In spp.c::send_spp_data() before and after send:
app_log("📤 SPP TX: len=%d, conn_handle=%d\n", len, conn_handle);
result = sl_bt_gatt_server_send_notification(...);
app_log("   result: %lu (%s)\n", result, 
        result == SL_STATUS_NO_MORE_RESOURCE ? "BUFFER_FULL" : "OK");
```

### 3. Observation & Hypothesis

**Monitor VCOM console while testing** (115200 baud):

```
BLE Stack Booted Successfully!
🔧 system_boot: Creating advertising set
✅ connection_opened: handle=0
📤 SPP TX: len=5, conn_handle=0
   result: 0 (OK)
BLE→VCOM: H
BLE→VCOM: e
BLE→VCOM: l
BLE→VCOM: l
BLE→VCOM: o
```

**Ask**: What sequence do you expect vs. what appears?

### 4. Build Test Plan

For each component, verify:

#### BLE Advertising & Connection
```bash
# Flash firmware, connect via Simplicity Connect app
# Expected in VCOM:
#   - "system_boot" message
#   - "connection_opened" when device connects
#   - "connection_closed" when disconnected, then "system_boot" repeats
```

#### Data TX (UART → BLE)
```bash
# In Simplicity Connect, enable notifications on SPP characteristic
# In terminal, type: "hello"
# Expected in VCOM:
#   - Each character logged as VCOM RX
#   - "SPP TX" message showing send attempt
# Expected in Simplicity Connect:
#   - "hello" received as notification or indication
```

#### Data RX (BLE → UART)
```bash
# In Simplicity Connect, write "world" to SPP Write characteristic
# Expected in VCOM:
#   - "BLE→VCOM" message for each byte
#   - Characters echoed on console: "world"
```

#### Queue Health
```c
// Add to cli_task or app_task periodically:
UBaseType_t queue_depth = uxQueueMessagesWaiting(uartQueue);
if (queue_depth > 0) {
    app_log("⚠️  uartQueue depth: %lu bytes\n", queue_depth);
}
if (queue_depth > UART_RX_QUEUE_LEN - 5) {
    app_log("🚨 uartQueue near full! Increase size or reduce BLE RX rate\n");
}
```

### 5. Root Cause Analysis

**If BLE won't connect:**
1. Check `system_boot` → `sl_bt_advertiser_create_set()` called?
2. Check advertising started: `sl_bt_legacy_advertiser_start()` returned success?
3. Try reducing advertising interval (currently 100ms)

**If data lost from BLE:**
1. Is `send_spp_data()` called? Check log.
2. Does it return error? If so, which one?
3. Is `conn_handle != 0xFF`? If not, not connected.
4. Check MTU: Might be sending >20 bytes when MTU is 20.

**If data lost from UART:**
1. Is `uartQueue` overflowing? Check `uxQueueMessagesWaiting()` spikes.
2. Is `cli_task` running? (Should print "BLE→VCOM" after sends via Simplicity Connect)
3. Increase queue size: `#define UART_RX_QUEUE_LEN 256` (was 128?)

**If crashes or hangs:**
1. Search for unprotected BLE calls outside `app_mutex_handle` lock
2. Check for deadlock: `app_task` and `cli_task` waiting on same queue?
3. Check stack overflow: Build with stack checking enabled

### 6. Implementation of Fix

Once root cause found, make targeted fix:

```c
// Example: Connection closed not restarting advertising
// BEFORE (bug):
case sl_bt_evt_connection_closed_id:
    break;  // ← Missing restart!

// AFTER (fixed):
case sl_bt_evt_connection_closed_id:
    sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                               sl_bt_advertiser_general_discoverable);
    sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                       sl_bt_legacy_advertiser_connectable);
    app_log("Advertising restarted\n");
    break;
```

### 7. Verify Fix

Test systematically:
1. **Build & flash**
2. **Reproduce original symptom** → should NOT occur
3. **Verify normal operation** → data flows both directions
4. **Long-term test** → leave running for 10 min, check for glitches

### 8. Review Against Guidelines

Check fixed code against [Workspace Guidelines](./.github/copilot-instructions.md):
- ✓ BLE calls protected by mutex?
- ✓ Queue sizes appropriate?
- ✓ Stack sizes adequate?
- ✓ Reconnect logic present?

---

## Checklist for Common Bugs

- [ ] Advertising set handle not initialized (`0xff`)?
- [ ] Connection not restarting advertising on disconnect?
- [ ] Unprotected BLE call outside event handler?
- [ ] Queue overflow (producer faster than consumer)?
- [ ] Stack overflow or insufficient heap?
- [ ] Data not checked for NULL or length overflow?
- [ ] MTU mismatch (sending >20 bytes)?
- [ ] Characteristic handle incorrect?

## Quick Reference: Log Tags

|Tag|Meaning|Component|
|-|-|-|
|🔧 system_boot|BLE ready|app.c|
|✅ connection_opened|Device connected|app.c|
|❌ connection_closed|Device disconnected|app.c|
|📤 SPP TX|Data sent via BLE|spp.c|
|BLE→VCOM|Data from BLE|cli.c|
|VCOM RX|Data from USB console|cli.c|
|⚠️ Queue|Queue status info|any|
|🚨 Error|Critical issue|any|

