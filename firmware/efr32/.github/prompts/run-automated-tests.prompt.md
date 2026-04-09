---
name: run-automated-tests
description: "Execute the Build-Flash-Verify workflow with options for custom testing scenarios. Use when: need to compile and flash quickly, verify device boots, or automate test cycle."
---

# Run Automated Build-Flash-Verify

Execute the complete development cycle: compile → flash → verify device boot.

## Quick Commands

### Full Workflow (Clean Build)
```powershell
cd c:\Users\rossa\Projects\Gaston_car\firmware\efr32
.\.github\scripts\build-flash-verify.ps1
```
**Time**: ~45-60 seconds. Use when config may have changed or first time.

### Fast Iteration (Skip Config)
```powershell
.\.github\scripts\build-flash-verify.ps1 -SkipConfigure
```
**Time**: ~30-40 seconds. Use when only editing `app.c`, `cli.c`, etc.

### With Boot Timeout
```powershell
.\.github\scripts\build-flash-verify.ps1 -TimeoutSeconds 20
```
**Use if**: Device takes >10 seconds to boot.

### Save VCOM Output
```powershell
.\.github\scripts\build-flash-verify.ps1 -SaveLog .\test-$(Get-Date -Format yyyyMMdd-HHmmss).log
```
**Use if**: Need to analyze boot logs or troubleshoot.

### Specify COM Port Manually
```powershell
.\.github\scripts\build-flash-verify.ps1 -Port COM3
```
**Use if**: VCOM auto-detection fails.

---

## What It Does

1. ✓ **Verify** tools are installed (CMake, Commander)
2. ✓ **Configure** CMake (unless `-SkipConfigure`)
3. ✓ **Build** firmware with Ninja
4. ✓ **Flash** device via Simplicity Commander
5. ✓ **Monitor** VCOM until boot message appears
6. ✓ **Report** results with color-coded status

---

## Expected Success Output

```
✓ Build successful
✓ Flash successful
✓ Device booted successfully
  Received: Bluetooth stack initialized successfully
```

→ Device is running. Ready for BLE testing.

## When to Use

- After code changes before deployment
- Verify hardware still works after builds
- Quick sanity check before committing
- Automated test runs in CI/CD (future)

## Learn More

Full documentation: See [automated-testing-workflow skill](../../skills/automated-testing-workflow/SKILL.md)
- Parameters explained
- Troubleshooting common failures
- Advanced customization
- Integration with dev workflow

