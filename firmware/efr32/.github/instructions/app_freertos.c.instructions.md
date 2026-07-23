---
description: "Guidance for app_freertos.c — FreeRTOS task management, synchronization primitives (mutex, semaphore, queues). Use when: debugging task deadlocks, stack overflows, synchronization issues, or adding new tasks."
applyTo: "**/app_freertos.c"
---

# app_freertos.c — FreeRTOS Task & Synchronization Management

**Role**: Initializes FreeRTOS tasks, queues, mutexes, and semaphores. Glue layer between Bluetooth stack and application tasks.

## Core Initialization: `app_init_bt()`

Called once at startup. Sets up entire task ecosystem:

```c
void app_init_bt(void) {
    // 1. Create queues first (before tasks that use them)
    cli_queue = xQueueCreate(1, CLI_COMMAND_MAX_LEN);
    uartQueue = xQueueCreate(UART_RX_QUEUE_LEN, sizeof(uint8_t));
    
    // 2. Create tasks (now queues are ready)
    xTaskCreate(app_task, "app_task", 512, NULL, 24, &app_task_handle);
    xTaskCreate(cli_task, "cli_task", 1024, NULL, 20, &cli_task_handle);
    
    // 3. Create synchronization primitives
    app_semaphore_handle = xSemaphoreCreateCounting(UINT16_MAX, 0);
    app_mutex_handle = xSemaphoreCreateRecursiveMutex();
    
    // 4. Initialize Bluetooth stack
    sl_stack_init();
}
```

**Order matters**: Always create queues before tasks (tasks block waiting for them).

## Tasks

### `app_task` — Main Application Loop

| Property | Value |
|----------|-------|
| **Function** | `app_task()` |
| **Stack** | 512 bytes |
| **Priority** | 24 (high) |
| **Purpose** | Call `app_process_action()` periodically; main app logic |

```c
static void app_task(void *p_arg) {
    while (1) {
        app_process_action();  // Implement app logic here (or in sl_bt_on_event)
        vTaskDelay(pdMS_TO_TICKS(10));  // Yield every 10ms
    }
}
```

**Key points**:
- Runs in infinite loop with 10ms delays
- Currently minimal logic (LED toggle would go here)
- If adding blocking operations, increase stack size

### `cli_task` — UART ↔ BLE Bridge

| Property | Value |
|----------|-------|
| **Function** | `cli_task()` in `cli.c` |
| **Stack** | 1024 bytes |
| **Priority** | 20 (lower than `app_task`) |
| **Purpose** | Poll UART, forward to BLE; receive BLE via queue, output to UART |

**Created in** `app_freertos.c`:
```c
ret = xTaskCreate(cli_task, "cli_task", 1024, NULL, 20, &cli_task_handle);
```

See [`cli.c.instructions.md`](cli.c.instructions.md) for details.

## Queues

### Global Queue Handles

```c
extern QueueHandle_t uartQueue;    // From app.c → cli_task (BLE→UART)
extern QueueHandle_t cli_queue;    // For future CLI command routing
extern TaskHandle_t  cli_task_handle;
```

### `uartQueue` — BLE Data Reception

```c
uartQueue = xQueueCreate(UART_RX_QUEUE_LEN, sizeof(uint8_t));
```

- **Size**: `UART_RX_QUEUE_LEN` bytes (check `cli.h` for define)
- **Element size**: 1 byte per element
- **Producer**: BLE event handler (in `app.c::sl_bt_on_event()`)
- **Consumer**: `cli_task` non-blocking dequeue
- **Overflow behavior**: Oldest message dropped if queue full (configurable)

### `cli_queue` — CLI Commands (reserved)

```c
cli_queue = xQueueCreate(1, CLI_COMMAND_MAX_LEN);
```

- Currently unused in example code
- Could implement for interactive command parsing
- 1 element capacity (no buffering)

## Synchronization Primitives

### `app_semaphore_handle` — BLE Event Signal

```c
app_semaphore_handle = xSemaphoreCreateCounting(UINT16_MAX, 0);
```

**Type**: Counting semaphore  
**Initial count**: 0 (no signals pending)  
**Max count**: `UINT16_MAX` (can accumulate many signals)

**Usage**:
```c
// In interrupt or event handler:
xSemaphoreGive(app_semaphore_handle);  // Signal event occurred

// In app_task or other task:
xSemaphoreTake(app_semaphore_handle, portMAX_DELAY);  // Block until signaled
```

**Purpose**: Wake `app_task` when BLE events arrive (can be unused if events handled directly in `sl_bt_on_event()`).

### `app_mutex_handle` — BLE Stack Protection

```c
app_mutex_handle = xSemaphoreCreateRecursiveMutex();
```

**Type**: Recursive mutex (same task can acquire multiple times)  
**Initial state**: Unlocked

**Usage — CRITICAL**:
```c
// ✓ SAFE: Acquire before any BLE call outside ISR
xSemaphoreTake(app_mutex_handle, portMAX_DELAY);
{
    sl_bt_connection_get_rssi(...);
    sl_bt_gatt_server_send_notification(...);
    // Other BLE calls
}
xSemaphoreGive(app_mutex_handle);

// ✗ DANGEROUS: Unprotected BLE call
sl_bt_connection_get_rssi(...);  // CRASH if called from task while ISR runs

// ✓ Exception: Inside sl_bt_on_event() already protected
void sl_bt_on_event(sl_bt_msg_t *evt) {
    sl_bt_advertiser_start(...);  // Safe—implicit protection
}
```

**Timeout**: Using `portMAX_DELAY` means infinite wait. For timeout:
```c
if (xSemaphoreTake(app_mutex_handle, pdMS_TO_TICKS(100)) == pdPASS) {
    // Acquired mutex
} else {
    // Timeout—mutex held by another task (potential deadlock)
    app_log("Mutex timeout!\n");
}
```

## Error Handling & Assertions

```c
ret = xTaskCreate(...);
app_assert(ret == pdPASS, "Task creation failed.");

app_assert(cli_queue != NULL, "CLI queue creation failed.");
```

**If assertion fails**: Halts execution. Check:
- Sufficient RAM for tasks & queues
- Proper FreeRTOS config in `config/FreeRTOSConfig.h`

## Debugging Utilities

### `app_proceed()` & `app_is_process_required()`

```c
void app_proceed(void) {
    if (xPortIsInsideInterrupt()) {
        xSemaphoreGiveFromISR(app_semaphore_handle, &woken);
        portYIELD_FROM_ISR(woken);
    } else {
        xSemaphoreGive(app_semaphore_handle);
    }
}

bool app_is_process_required(void) {
    return xSemaphoreGetCount(app_semaphore_handle) > 0;
}
```

These manage the semaphore lifecycle. Called by main event loop (if implemented).

## Priority Levels

```
Priority 24 (app_task)      ← High: Main logic
Priority 20 (cli_task)      ← Lower: I/O polling
Priority [BLE stack]        ← Implicit: Event handling
Priority 0 (idle)           ← Lowest: Idle task
```

**Note**: Higher number = higher priority. Adjust if `cli_task` blocks UI or vice versa.

## Stack Size Tuning

### Current Allocation

| Task | Stack | Notes |
|------|-------|-------|
| `app_task` | 512 bytes | Minimal; may need increase if calling deep functions |
| `cli_task` | 1024 bytes | Larger because `app_log()` uses stack for formatting |

### Stack Overflow Symptoms

- Unexpected resets
- Data corruption
- Task hangs
- Assertion failures

### How to Check

```c
// In debugger or via monitor task:
uxTaskGetStackHighWaterMark(app_task_handle);  // Returns free stack
```

If result < 64 bytes, increase stack in `xTaskCreate()`.

## Common Issues

| Issue | Cause | Fix |
|-------|-------|-----|
| Deadlock (all tasks hung) | `app_task` waits forever on mutex held by another task | Review mutex usage; add timeout |
| Stack overflow crash | Task stack too small | Increase stack size in `xTaskCreate()` |
| Queue overflow (lost data) | Producer faster than consumer | Increase queue size or slow producer |
| BLE crash | Unprotected BLE call outside event handler | Always acquire `app_mutex_handle` first |
| Task creation fails | Insufficient heap | Increase `configTOTAL_HEAP_SIZE` in `FreeRTOSConfig.h` |

## Integration Points

| Component | Role |
|-----------|------|
| `FreeRTOSConfig.h` | Defines heap, tick rate, stack checking |
| `app.c` | Calls `app_init_bt()` from `app_init()` |
| `cli.c` | Uses `uartQueue`, `cli_queue` |
| `spp.c` | Uses `app_mutex_handle` to protect BLE calls (if needed) |
| `main.c` | Calls `app_init_bt()` before `vTaskStartScheduler()` |

