---
description: "Guidance for app.c — BLE event handler, connection lifecycle, advertising management. Use when: editing Bluetooth stack events, connection state, advertising logic, or investigating connection-related bugs."
applyTo: "**/app.c"
---

# app.c — Bluetooth Event Handler

**Role**: Core BLE event processing. Manages connection lifecycle, advertising, state transitions, and delegates data to SPP service.

## Event Handling Flow

```c
sl_bt_on_event(sl_bt_msg_t *evt)
    ├─ sl_bt_evt_system_boot_id         → Initialize advertising
    ├─ sl_bt_evt_connection_opened_id   → Store handle, update state
    ├─ sl_bt_evt_connection_closed_id   → Restart advertising
    └─ sl_bt_evt_gatt_server_characteristic_status_id → Handle subscription/write
```

### Key Handlers

1. **`sl_bt_evt_system_boot_id`** — Called once at startup
   - Must create advertising set: `sl_bt_advertiser_create_set()`
   - Generate advertising data: `sl_bt_legacy_advertiser_generate_data()`
   - Set timing and start: `sl_bt_legacy_advertiser_start()`
   - ⚠️ **Critical**: Do NOT skip this—device will not advertise

2. **`sl_bt_evt_connection_opened_id`** — Remote device connected
   - Store `conn_handle = evt->data.evt_connection_opened.connection`
   - Update `main_state` if tracking states
   - This handle is used for all subsequent notifications/writes

3. **`sl_bt_evt_connection_closed_id`** — Remote device disconnected
   - **MUST** restart advertising, else device is unreachable
   - Call `sl_bt_legacy_advertiser_start()` again
   - Reset connection-dependent state (queue depths, counters, etc.)

## Thread-Safety Checklist

❌ **Never call BLE stack functions without mutex:**
```c
// WRONG
void some_task() {
    sl_bt_connection_get_rssi(conn_handle, &rssi);  // CRASH/undefined behavior
}

// CORRECT
void some_task() {
    xSemaphoreTake(app_mutex_handle, portMAX_DELAY);
    sl_bt_connection_get_rssi(conn_handle, &rssi);
    xSemaphoreGive(app_mutex_handle);
}
```

**Exception**: Code inside `sl_bt_on_event()` already holds implicit lock—no explicit mutex needed.

## State Machine (if used)

Current states found in code:
- `STATE_ADVERTISING` — Idle, waiting for connection
- `STATE_CONNECTED` — Client connected, ready for data

Update transitions when adding new states.

## Debugging Tips

- **Connection not appearing**: Check `sl_bt_evt_system_boot_id`—is advertising set created?
- **Device connects then disconnects**: Verify `sl_bt_evt_connection_closed_id` restarts advertising
- **Data never received**: Check if `conn_handle` is valid (not `0xFF`)
- **BLE stack crashes**: Enable mutex trace in `app_freertos.c` to find unprotected calls

## LED & Visual Feedback

- Toggle in `app_process_action()` via `sl_simple_led_toggle()` (currently toggles every 50 iterations)
- Can use LED state to indicate connection status—save LED0 in connection handler

## Performance Notes

- `sl_bt_on_event()` should **not block**—use queues to pass data to `app_task` or `cli_task`
- Avoid long-running operations inside event handler
- If processing large BLE messages, split into chunks and defer via queue

## Common Pitfalls

1. **Advertising set handle = 0xff**: Not initialized. Check `system_boot_id` handler.
2. **Reconnect fails**: Connection closed handler didn't restart advertising.
3. **Mutex deadlock**: Calling BLE function twice without releasing mutex.
4. **Stack overflow in callback**: `app_log()` inside event handler can use large stack.
