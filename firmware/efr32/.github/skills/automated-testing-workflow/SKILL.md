---
name: automated-testing-workflow
description: "Execute and monitor Build → Flash → Verify workflow with step-by-step verification. Use when: need automated build+flash cycle, want to verify device boot, or testing after code changes."
---

# Automated Testing Workflow for Build-Flash-Verify

Comprehensive guide to compile → flash → verify device using automated PowerShell script.

## What This Does

The script orchestrates a complete development cycle:

```
┌─────────────────────────────────────────────────────┐
│ 1. Verify Prerequisites (CMake, Commander, paths)   │
├─────────────────────────────────────────────────────┤
│ 2. Configure CMake (unless skipped)                 │
├─────────────────────────────────────────────────────┤
│ 3. Compile firmware (ninja build)                   │
├─────────────────────────────────────────────────────┤
│ 4. Flash device (Simplicity Commander)              │
├─────────────────────────────────────────────────────┤
│ 5. Monitor VCOM (verify boot message)               │
├─────────────────────────────────────────────────────┤
│ 6. Report results (color-coded status)              │
└─────────────────────────────────────────────────────┘
```

**Time**: Typically 30-60 seconds end-to-end.

## Quick Start

### Run Full Workflow (Recommended for Clean Build)

```powershell
cd c:\Users\rossa\Projects\Gaston_car\firmware\efr32
.\.github\scripts\build-flash-verify.ps1
```

**Output example:**
```
╔═══════════════════════════════════════════════════════╗
║ Verifying Prerequisites                              ║
╚═══════════════════════════════════════════════════════╝
[14:32:15] ✓ CMake found
[14:32:15] ✓ Simplicity Commander found
✓ CMake configuration successful

╔═══════════════════════════════════════════════════════╗
║ Building Firmware                                    ║
╚═══════════════════════════════════════════════════════╝
[14:32:45] ✓ Build successful

╔═══════════════════════════════════════════════════════╗
║ Flashing Device                                      ║
╚═══════════════════════════════════════════════════════╝
[14:32:52] ✓ Flash successful

╔═══════════════════════════════════════════════════════╗
║ Verifying Device Boot                                ║
╚═══════════════════════════════════════════════════════╝
[14:32:57] ✓ Device booted successfully
   Received: Bluetooth stack initialized successfully
```

### Skip CMake (Faster Iteration)

If you've already configured, reuse existing CMake:

```powershell
.\.github\scripts\build-flash-verify.ps1 -SkipConfigure
```

**Time saved**: ~3-5 seconds (skips configure step)

### Increase Boot Timeout

If device takes >10 seconds to boot:

```powershell
.\.github\scripts\build-flash-verify.ps1 -TimeoutSeconds 20
```

### Save VCOM Output for Analysis

Troubleshooting? Save what device outputs:

```powershell
.\.github\scripts\build-flash-verify.ps1 -SaveLog .\vcom-output.log
cat .\vcom-output.log
```

---

## Understanding Output

### Color-Coded Status

| Color | Meaning | Action |
|-------|---------|--------|
| 🟢 Green `✓ OK` | Step succeeded | Continue |
| 🔴 Red `✗ FAIL` | Critical error | Fix and retry |
| 🟡 Yellow `⚠ WARN` | Non-critical issue | Investigate, may still work |
| 🔵 Cyan `ℹ INFO` | Progress message | Informational |

### Common Success Output

```
✓ Build successful
✓ Flash successful
✓ Device booted successfully
   Received: Bluetooth stack initialized successfully
```

**Interpretation**: Everything worked. Device is running firmware.

### Common Warnings & Solutions

| Message | Likely Cause | Fix |
|---------|--------------|-----|
| `Could not auto-detect COM port` | VCOM not connected or drivers missing | Connect device or specify `-Port COM3` |
| `Device did not respond with boot message` | Device running but no log output | Check `config/app_log_config.h`; device likely OK |
| `CMake configuration failed` | Build system corrupted | Delete `build/` folder and retry |
| `Build failed` | Syntax error in code | Check compiler errors; fix code |
| `Flash failed` | Device not detected | Verify device serial number; check J-Link connection |

---

## Step-by-Step Explanation

### 1. Verify Prerequisites

Script checks:
- CMake executable exists at hardcoded path
- Simplicity Commander available
- Project directory structure is intact

**Why**: Catch environment issues early (bad install, missing tools)

### 2. Configure CMake

```powershell
cmake --preset project
```

- Generates ninja build files
- Configures toolchain, flags, includes
- **Only needed once** (unless .slcp or CMakeLists.txt changed)

**Skip if**: You know nothing changed in config files.

### 3. Build Firmware

```powershell
cmake --build --preset default_config
```

- Invokes Ninja compiler
- Produces `bt_soc_gaston.elf`, `.hex`, `.bin`, `.s37`
- Takes ~15-20 seconds typically

**Failures**: Check compiler errors—likely C syntax bugs in your code.

### 4. Flash Device

```powershell
commander flash bt_soc_gaston.hex --halt --serialno 440242346 --device EFR32BG22C224F512IM40
```

- Transfers firmware to device flash memory
- Halts CPU (allows debugger attach)
- Takes ~5-8 seconds

**Failures**: Check J-Link connection, serial number, or device detection.

### 5. Monitor VCOM & Verify Boot

Script opens COM port (115200 baud), waits for device to print:
```
Bluetooth stack initialized successfully
```

If message appears, device booted → firmware running ✓

**Timeout**: If device takes >10 seconds, increase with `-TimeoutSeconds`

**No message**: Device may still be OK (logging disabled), but script can't confirm.

### 6. Report & Exit

Green checkmarks = all steps passed → device ready for testing.

---

## Integration with Development Workflow

### After Every Code Change

```powershell
# Edit app.c or cli.c...
# Then:
.\.github\scripts\build-flash-verify.ps1 -SkipConfigure

# If successful:
# → Open Simplicity Connect app to test BLE
# → Monitor VCOM console at 115200 for logs
```

### Before Committing

```powershell
# Clean build (ensure no stale files)
if (Test-Path "cmake_gcc\build") { Remove-Item "cmake_gcc\build" -Recurse }

.\.github\scripts\build-flash-verify.ps1

# If green ✓ → safe to commit
# If red ✗ → fix bugs first
```

### Batch Testing Multiple Scenarios

```powershell
# Test 1: Clean build
Remove-Item "cmake_gcc\build" -Recurse -Force
.\.github\scripts\build-flash-verify.ps1

# Test 2: Incremental rebuild (faster)
.\.github\scripts\build-flash-verify.ps1 -SkipConfigure

# Test 3: Save logs
.\.github\scripts\build-flash-verify.ps1 -SkipConfigure -SaveLog .\test-run.log
```

---

## Script Parameters

### `-SkipConfigure` (boolean, default: false)

Skip CMake configuration. Use if:
- Nothing in `.slcp` or `CMakeLists.txt` changed
- You just edited `app.c`, `cli.c`, etc.
- Testing rapid iteration

**Saves**: ~3-5 seconds

**Warning**: If config is stale, build may fail unexpectedly. When in doubt, don't skip.

### `-Port <string>` (default: auto-detect)

Manually specify COM port for VCOM monitoring.

**Use if**:
- Auto-detection fails
- Your device is on a non-standard port (e.g., `COM5`)

**Example**:
```powershell
.\.github\scripts\build-flash-verify.ps1 -Port COM3
```

### `-TimeoutSeconds <int>` (default: 10)

How long to wait (seconds) for boot message before giving up.

**Use if**:
- Device boots slow (>10 seconds)
- You want faster fail detection (<5 seconds)

**Example**:
```powershell
.\.github\scripts\build-flash-verify.ps1 -TimeoutSeconds 20
```

### `-SaveLog <string>` (default: none)

Path to save VCOM output for debugging.

**Useful for**:
- Analyzing device behavior offline
- Capturing logs before they scroll away
- Compliance/documentation

**Example**:
```powershell
.\.github\scripts\build-flash-verify.ps1 -SaveLog .\logs\test-$(Get-Date -Format yyyyMMdd-HHmmss).log
```

---

## Customization (Advanced)

### Change Serial Number or Device ID

Edit `.github/scripts/build-flash-verify.ps1`:

```powershell
$SerialNumber = "440242346"          # ← Your J-Link serial
$DeviceId = "EFR32BG22C224F512IM40"  # ← Your device model
```

### Change Baud Rate

If VCOM runs at different baud rate:

```powershell
$serialPort.BaudRate = 115200  # ← Change this
```

### Add Custom Verification Logic

After boot message detected, add:

```powershell
# Check for specific app message
if ($output -match "CLI task created successfully") {
    Write-Status "✓ CLI task running" "OK"
}
```

---

## Troubleshooting

### Script Won't Run: "cannot be loaded because running scripts is disabled"

**Fix**: Enable script execution:
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

### CMake Not Found

**Fix**: Update path in script:
```powershell
$CmakePath = "C:/your/path/to/cmake.exe"
```

### Commander Not Found

**Fix**: Check Simplicity Studio installation; update path:
```powershell
$CommanderPath = "C:/your/path/to/commander.exe"
```

### Flash Fails: Device Not Detected

**Steps**:
1. Check J-Link USB connection
2. Verify serial number matches (in script)
3. Run J-Link command line to verify: `jlink -CommandFile verify.jlink`
4. May need to update J-Link driver

### VCOM Port Not Found

**Steps**:
1. Check device USB connection
2. Open Device Manager → Ports (COM & LPT) 
3. Find VCOM COM port (usually COM3 or COM4)
4. Run script with: `.\.github\scripts\build-flash-verify.ps1 -Port COM3`

---

## Next Steps After Successful Verification

✓ Device booted and running → Test BLE communication:

1. **Connect via Simplicity Connect app** (iOS/Android)
   - Open app, scan for "Gaston_car" or your device name
   - Connect and verify link established

2. **Monitor VCOM logs** (terminal at 115200 baud)
   - Data flowing both directions?
   - Any error messages?

3. **Stress test** (send large data, reconnect, etc.)
   - See [debug-ble-serial-bridge skill](../skills/debug-ble-serial-bridge/SKILL.md) for systematic testing

