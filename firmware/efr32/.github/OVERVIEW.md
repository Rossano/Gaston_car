---
name: Overview
description: Navigation guide for EFR32 workspace customizations and debugging resources
---

# EFR32 Gaston Car Firmware — Documentation & Customization Overview

This workspace has been extended with comprehensive guidelines, file-specific instructions, debugging prompts, and skills to accelerate development and bug-fix workflows.

## 📋 What's Available

### 1. Workspace-Level Guidelines
- **File**: [`.github/copilot-instructions.md`](./.github/copilot-instructions.md)
- **Scope**: Applies to all requests across the entire project
- **Content**: Architecture overview, code style, build commands, conventions, synchronization rules
- **Use when**: Getting started, understanding project constraints, or asking general questions

### 2. File-Specific Instructions

When editing these files, Copilot automatically loads relevant guidance:

| File | Instructions | Key Topics |
|------|--------------|------------|
| `app.c` | [app.c.instructions.md](./.github/instructions/app.c.instructions.md) | BLE events, connection lifecycle, state machine, thread-safety |
| `cli.c` | [cli.c.instructions.md](./.github/instructions/cli.c.instructions.md) | UART ↔ BLE bridge, queue handling, bidirectional data flow |
| `spp.c` | [spp.c.instructions.md](./.github/instructions/spp.c.instructions.md) | SPP data transmission, backpressure, MTU negotiation |
| `app_freertos.c` | [app_freertos.c.instructions.md](./.github/instructions/app_freertos.c.instructions.md) | Task creation, synchronization primitives, stack management |

**How it works**: When you open one of these files, the corresponding `.instructions.md` is automatically available for reference.

### 3. Custom Prompt
- **Name**: `/debug-efr32-bug`
- **Location**: `.github/prompts/debug-efr32-bug.prompt.md`
- **Use when**: Need to debug a specific issue (connection problems, data loss, crashes)
- **Type**: Focused single-task prompt for bug investigation

**Example usage**:
```
/debug-efr32-bug
Symptom: Device connects for 5 seconds then disconnects. 
Reproduction: Connect via Simplicity Connect, wait 5 sec, device resets.
Context: Recently added longer logging in SPP send function.
```

### 4. Debugging Skill
- **Name**: `debug-ble-serial-bridge`
- **Location**: `.github/skills/debug-ble-serial-bridge/`
- **Use when**: Systematic debugging of BLE/UART communication issues
- **Workflow**: Symptom classification → enable logging → test plan → root cause analysis

**How to use**:
```
/debug-ble-serial-bridge
```
Then follow the guided workflow.

### 5. Automated Testing Workflow

#### PowerShell Script: Build → Flash → Verify
- **Script**: `.github/scripts/build-flash-verify.ps1`
- **Purpose**: Fully automated compile + flash + device boot verification
- **Time**: 30-60 seconds per cycle

**Quick start**:
```powershell
cd c:\Users\rossa\Projects\Gaston_car\firmware\efr32
.\.github\scripts\build-flash-verify.ps1
```

**Options**:
- `-SkipConfigure` — Skip CMake config (faster iteration)
- `-TimeoutSeconds 20` — Longer boot wait time
- `-SaveLog .\test.log` — Save VCOM output for analysis
- `-Port COM3` — Manual COM port if auto-detect fails

#### Skill: Automated Testing Workflow
- **Name**: `automated-testing-workflow`
- **Location**: `.github/skills/automated-testing-workflow/`
- **Use when**: Need structured guide to build-flash-verify cycle
- **Includes**: Parameters explained, troubleshooting, integration with dev workflow

**How to use**:
```
/automated-testing-workflow
```

#### Prompt: Run Automated Tests
- **Name**: `/run-automated-tests`
- **Location**: `.github/prompts/run-automated-tests.prompt.md`
- **Quick access**: Fast launcher for the build-flash-verify script

---

## 🚀 Quick Start Scenarios

### I'm editing `app.c` — Understanding connection logic
1. Open `app.c`
2. Read [app.c.instructions.md](./.github/instructions/app.c.instructions.md) for event handler patterns
3. Check [Workspace Guidelines](./.github/copilot-instructions.md#blesecurethread-safety-checklist) for mutex rules

### Data isn't flowing from UART to BLE
1. Use `/debug-ble-serial-bridge` skill
2. Enable logging as suggested in step 2 of the workflow
3. Trace data through `cli.c` → `spp.c` → verify sends succeed
4. Check `conn_handle != 0xFF`

### Connection drops after N seconds
1. Review [app.c.instructions.md](./.github/instructions/app.c.instructions.md#event-handling-flow) — check `connection_closed` handler
2. Verify `sl_bt_legacy_advertiser_start()` is called on disconnect
3. Use `/debug-efr32-bug` prompt if issues persist

### Stack overflow or task deadlock
1. Reference [app_freertos.c.instructions.md](./.github/instructions/app_freertos.c.instructions.md#common-issues)
2. Check mutex acquisition order and timeouts
3. Verify task stack sizes are adequate

---

## 📂 File Organization

```
.github/
├── OVERVIEW.md                           ← This file
├── copilot-instructions.md               ← Main workspace guidelines
├── instructions/
│   ├── app.c.instructions.md
│   ├── cli.c.instructions.md
│   ├── spp.c.instructions.md
│   └── app_freertos.c.instructions.md
├── prompts/
│   ├── debug-efr32-bug.prompt.md
│   └── run-automated-tests.prompt.md
├── scripts/
│   └── build-flash-verify.ps1            ← Main automation script
└── skills/
    ├── debug-ble-serial-bridge/
    │   └── SKILL.md
    └── automated-testing-workflow/
        └── SKILL.md
```

---

## 🔗 Key References

### BLE & Architecture
- [Workspace Guidelines — Architecture](./.github/copilot-instructions.md#architecture)
- [Silicon Labs Bluetooth SDK Documentation](https://docs.silabs.com/bluetooth/latest/)
- [EFR32BG22 Datasheet](https://www.silabs.com/documents/public/data-sheets/efr32-mighty-gecko-efr32-series-2-data-sheet.pdf)

### FreeRTOS & Synchronization
- [Workspace Guidelines — FreeRTOS Best Practices](./.github/copilot-instructions.md#freertos-best-practices)
- [app_freertos.c.instructions.md — Full sync reference](./.github/instructions/app_freertos.c.instructions.md)
- [FreeRTOS Kernel Documentation](https://www.freertos.org/)

### Project Documentation
- [PROJECT_CONTEXT.md](./PROJECT_CONTEXT.md) — High-level project overview
- [GEMINI.md](./GEMINI.md) — Detailed firmware analysis
- [readme.md](./readme.md) — Silicon Labs template info

### Build & Debug
- [Workspace Guidelines — Build and Test](./.github/copilot-instructions.md#build-and-test)
- [Debugging Skill](./.github/skills/debug-ble-serial-bridge) — Structured debugging workflow

---

## 🎯 Common Tasks & Where to Find Help

| Task | Resource |
|------|----------|
| **Automated build → flash → verify** | `.github/scripts/build-flash-verify.ps1` or `/run-automated-tests` prompt |
| **Build and flash firmware (manual)** | [Workspace Guidelines — Build Commands](./.github/copilot-instructions.md#build-commands) |
| **Understand BLE event flow** | [app.c.instructions.md — Event Handling](./.github/instructions/app.c.instructions.md#event-handling-flow) |
| **Debug data loss or corruption** | `/debug-ble-serial-bridge` skill |
| **Fix thread-safety issues** | [Workspace Guidelines — Thread-Safety](./.github/copilot-instructions.md#key-patterns) + [app_freertos.c.instructions.md](./.github/instructions/app_freertos.c.instructions.md) |
| **Optimize UART↔BLE throughput** | [cli.c.instructions.md — Performance Tuning](./.github/instructions/cli.c.instructions.md#performance-tuning) |
| **Investigate connection drops** | [app.c.instructions.md — Debugging](./.github/instructions/app.c.instructions.md#debugging-tips) |
| **Add new FreeRTOS task** | [app_freertos.c.instructions.md — Task Creation](./.github/instructions/app_freertos.c.instructions.md#common-issues) |

---

## 💡 Workflow Examples

### Example 1: Fixing a Connection Bug
```
1. Use /debug-efr32-bug prompt
   → Describe: "Device won't reconnect after disconnect"
   
2. Prompt guides me to app.c::connection_closed handler
   
3. I open app.c, read app.c.instructions.md (auto-loaded)
   
4. Find that connection_closed doesn't restart advertising (bug!)
   
5. Apply fix:
      case sl_bt_evt_connection_closed_id:
          // Add restart logic
          sc = sl_bt_legacy_advertiser_start(...);
          
6. Flash and test
```

### Example 2: Increasing Throughput
```
1. Read cli.c.instructions.md — Performance Tuning section
   → Identifies: Newline-based buffering, 10ms task delay
   
2. Understand trade-off: Latency vs. throughput
   
3. Option A: Reduce task delay (1ms instead of 10ms)
      vTaskDelay(pdMS_TO_TICKS(1));
   
4. Option B: Batch data before sending
   
5. Monitor queue depth with uxQueueMessagesWaiting()
```

### Example 3: Stack Overflow Investigation
```
1. Firmware crashes; errors suggest stack overflow
   
2. Read app_freertos.c.instructions.md — Stack Size Tuning
   
3. Check which task: app_task (512 bytes) or cli_task (1024 bytes)
   
4. Use uxTaskGetStackHighWaterMark() in debugger
   
5. Increase stack:
      xTaskCreate(app_task, ..., 1024, ...);  // was 512
      
6. Verify fix and monitor heap
```

### Example 4: Quick Build-Flash-Verify Cycle
```
1. Edit app.c (add logging or fix bug)

2. Run automated workflow:
   .\.github\scripts\build-flash-verify.ps1 -SkipConfigure
   
   (or use /run-automated-tests prompt)

3. Output shows:
   ✓ Build successful
   ✓ Flash successful
   ✓ Device booted successfully

4. If green ✓ → Device running. Test with Simplicity Connect app.
   If red ✗ → Fix compiler/linker errors or flash issues.

5. Monitor VCOM (115200 baud) for logs:
   "BLE Stack Booted Successfully!"
   "Connection opened..."
   etc.
```

---

## 🔄 Iteration & Feedback

These customizations are **living documents**. As you fix bugs or add features:

1. If you discover new patterns or gotchas → Update the relevant `.instructions.md` file
2. If a task/queue/mutex interaction isn't clear → Expand the corresponding section
3. If you find undocumented constraints → Add to Workspace Guidelines

This keeps the guidance current and valuable for the team.

---

## 📞 Quick Help

- **"How do I..."** → Check the table above, then read the specific `.instructions.md`
- **"Debug issue X"** → Use `/debug-efr32-bug` or `/debug-ble-serial-bridge`
- **"Understand component Y"** → Read the corresponding file instruction (`*.instructions.md`)
- **"General question"** → Start with [Workspace Guidelines](./.github/copilot-instructions.md)

