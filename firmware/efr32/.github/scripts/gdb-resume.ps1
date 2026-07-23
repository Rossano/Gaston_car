// Flow control è OFF nel software:
#define SL_IOSTREAM_EUSART_VCOM_FLOW_CONTROL_TYPE     SL_IOSTREAM_EUSART_UART_FLOW_CTRL_NONE

// MA i PIN sono ancora configurati:
// EUART0 CTS on PA08
// EUART0 RTS on PA07#############################################################################
# gdb-resume.ps1
# Resume execution on EFR32 after halt due to --halt flag during flash
# 
# This script uses J-Link GDB server to connect and resume execution
#############################################################################

param(
    [string]$SerialNo = "440242346",
    [string]$Device = "EFR32BG22C224F512IM40",
    [int]$TimeoutSeconds = 10
)

# Path to J-Link GDB Server
$JLinkGDBServer = "C:/Users/rossa/.silabs/slt/installs/archive/JLink/JLinkGDBServer.exe"

# Check if JLinkGDBServer exists
if (-not (Test-Path $JLinkGDBServer)) {
    Write-Host "❌ J-Link GDB Server not found at: $JLinkGDBServer" -ForegroundColor Red
    exit 1
}

Write-Host "🔄 Resuming execution on device (Serial: $SerialNo)..." -ForegroundColor Cyan

# Create GDB commands to resume
$gdbCommands = @"
target remote localhost:2331
device $Device
attach 1
continue
quit
"@

# Save commands to temp file
$tempGdbScript = [System.IO.Path]::GetTempFileName() -replace "\.tmp$", ".gdb"
Set-Content -Path $tempGdbScript -Value $gdbCommands -Encoding UTF8

try {
    # Start J-Link GDB Server in background
    Write-Host "Starting J-Link GDB Server..." -ForegroundColor Yellow
    $gdbServerProcess = Start-Process -FilePath $JLinkGDBServer `
        -ArgumentList "-select", "usb=$SerialNo", "-device", $Device, "-if", "SWD", "-speed", "4000", "-silent" `
        -NoNewWindow `
        -PassThru

    # Wait a bit for GDB server to start
    Start-Sleep -Milliseconds 2000

    # Use GDB to connect and resume
    Write-Host "Connecting to target via GDB..." -ForegroundColor Yellow
    
    # Try to use arm-none-eabi-gdb if available
    $gdbPath = "arm-none-eabi-gdb"
    
    # Run GDB with script
    & $gdbPath --batch -x $tempGdbScript 2>&1 | ForEach-Object {
        Write-Host $_
    }

    Write-Host "✅ Device resumed! VCOM should now be active on COM3 @ 115200 baud" -ForegroundColor Green
    Write-Host "You can now connect to COM3 in Simplicity Studio or with putty/miniterm" -ForegroundColor Green

} catch {
    Write-Host "❌ Error: $_" -ForegroundColor Red
    exit 1
} finally {
    # Kill GDB server
    if ($gdbServerProcess) {
        try {
            Stop-Process -Id $gdbServerProcess.Id -Force -ErrorAction SilentlyContinue
        } catch {}
    }
    
    # Cleanup temp file
    if (Test-Path $tempGdbScript) {
        Remove-Item $tempGdbScript -Force
    }
}
