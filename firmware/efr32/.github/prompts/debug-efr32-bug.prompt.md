---
name: debug-efr32-bug
description: "Find and diagnose bugs in EFR32 firmware (BLE crashes, thread-safety, data loss, disconnects). Use when: investigating stability issues, connection problems, or unexpected behavior."
---

# Debug EFR32 Firmware Bug

Use this prompt to systematically investigate firmware bugs—connection failures, data loss, crashes, timeout issues, etc.

## What to Debug

Choose one:
- **BLE Connection**: Won't connect, reconnects unexpectedly, drops immediately
- **Data Transfer**: Data lost, corrupted, or never arrives
- **Thread Safety**: Crashes with mutex/semaphore errors
- **Performance**: Latency spikes, power consumption high, CPU busy
- **Other**: Describe the symptom

## How to Use

Run this prompt and provide:
1. **Symptom**: What you observe (e.g., "device disconnects 5 seconds after connect")
2. **Reproduction**: Steps to reproduce consistently
3. **Context**: What was changed recently (code, config, hardware)?

## Diagnostic Steps

I will:

1. **Identify the component**
   - Is it BLE stack (`app.c`), UART bridge (`cli.c`), or synchronization (`app_freertos.c`)?

2. **Review for common issues**
   - Thread-safety violations (unprotected BLE calls)
   - Queue overflow or deadlock
   - Stack overflow or heap exhaustion
   - Disconnect not restarting advertising

3. **Check the code path**
   - Trace the data flow from symptom back to root
   - Look for missing error checks or null pointer dereferences

4. **Propose fixes**
   - Suggest code changes with explanations
   - Recommend testing strategy to verify fix

5. **Provide debug output**
   - Enable verbose logging to isolate issue
   - Suggest GDB breakpoints or VCOM monitoring

## References

- Read [EFR32 Gaston Car Firmware Guidelines](./.github/copilot-instructions.md) for architecture overview
- File-specific guidance:
  - [app.c.instructions.md](./.github/instructions/app.c.instructions.md) — BLE events & threading
  - [cli.c.instructions.md](./.github/instructions/cli.c.instructions.md) — UART bridge & queues
  - [spp.c.instructions.md](./.github/instructions/spp.c.instructions.md) — Data transmission
  - [app_freertos.c.instructions.md](./.github/instructions/app_freertos.c.instructions.md) — Task management
