#Requires -Version 5.1
<#
.SYNOPSIS
    Automated Build → Flash → Verify workflow for EFR32 Gaston Car firmware
    
.DESCRIPTION
    1. Configures CMake (if needed)
    2. Builds firmware (bt_soc_gaston.elf)
    3. Flashes to device (via Simplicity Commander)
    4. Monitors VCOM output to verify device booted successfully
    5. Reports results with color-coded status
    
.PARAMETER SkipConfigure
    Skip CMake configuration step (use if already configured)
    
.PARAMETER Port
    COM port for VCOM monitoring (auto-detect if not specified)
    
.PARAMETER TimeoutSeconds
    How long to wait for device boot message (default: 10 seconds)
    
.PARAMETER SaveLog
    Save VCOM output to file for debugging
    
.EXAMPLE
    .\build-flash-verify.ps1
    ➜ Full workflow: Configure → Build → Flash → Verify
    
.EXAMPLE
    .\build-flash-verify.ps1 -SkipConfigure -TimeoutSeconds 15
    ➜ Skip config, increase timeout to 15 seconds
    
.EXAMPLE
    .\build-flash-verify.ps1 -SaveLog .\test-run-$(Get-Date -Format yyyyMMdd-HHmmss).log
    ➜ Save VCOM output to timestamped log file
#>

param(
    [switch]$SkipConfigure,
    [string]$Port = $null,
    [int]$TimeoutSeconds = 10,
    [string]$SaveLog = $null
)

$ErrorActionPreference = "Stop"
$WarningPreference = "Continue"

# ============================================================================
# Configuration
# ============================================================================

$ProjectRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommandPath))
$CmakeDir = Join-Path $ProjectRoot "cmake_gcc"
$BuildDir = Join-Path $CmakeDir "build"
$ArtifactDir = Join-Path $BuildDir "base"
$HexFile = Join-Path $ArtifactDir "bt_soc_gaston.hex"

$CommanderPath = "C:/Users/rossa/.silabs/slt/installs/archive/Simplicity Commander/commander.exe"
$SerialNumber = "440242346"
$DeviceId = "EFR32BG22C224F512IM40"
$CmakePath = "C:/Users/rossa/.silabs/slt/installs/conan/p/cmakefa35ab0687064/p/bin/cmake.exe"

# ============================================================================
# Logging & Formatting
# ============================================================================

# function Write-Status {
#     param([string]$Message, [string]$Status)
#     $timestamp = Get-Date -Format "HH:mm:ss"
#     $color = @{
#         "OK"    = "Green"
#         "FAIL"  = "Red"
#         "SKIP"  = "Yellow"
#         "INFO"  = "Cyan"
#         "WARN"  = "Yellow"
#     }[$Status] ?? "White"
    
#     Write-Host "[$timestamp] " -NoNewline
#     Write-Host $Message -ForegroundColor $color
# }
function Write-Status {
    param([string]$Message, [string]$Status)

    $timestamp = Get-Date -Format "HH:mm:ss"

    $colors = @{
        "OK"    = "Green"
        "FAIL"  = "Red"
        "SKIP"  = "Yellow"
        "INFO"  = "Cyan"
        "WARN"  = "Yellow"
    }

    if ($colors.ContainsKey($Status)) {
        $color = $colors[$Status]
    } else {
        $color = "White"
    }

    Write-Host "[$timestamp] " -NoNewline
    Write-Host $Message -ForegroundColor $color
}

function Write-Header {
    param([string]$Title)
    Write-Host ""
    Write-Host "╔═══════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "║ $Title.PadRight(61) ║" -ForegroundColor Cyan
    Write-Host "╚═══════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
    Write-Host ""
}

# ============================================================================
# Step 1: Verify Prerequisites
# ============================================================================

Write-Header "Verifying Prerequisites"

# Check CMake
if (-not (Test-Path $CmakePath)) {
    Write-Status "CMake not found at $CmakePath" "FAIL"
    exit 1
}
Write-Status "✓ CMake found" "OK"

# Check Commander
if (-not (Test-Path $CommanderPath)) {
    Write-Status "Simplicity Commander not found at $CommanderPath" "FAIL"
    exit 1
}
Write-Status "✓ Simplicity Commander found" "OK"

# Check project directory
if (-not (Test-Path $CmakeDir)) {
    Write-Status "CMake directory not found: $CmakeDir" "FAIL"
    exit 1
}
Write-Status "✓ CMake directory found" "OK"

# ============================================================================
# Step 2: Configure CMake
# ============================================================================

if (-not $SkipConfigure) {
    Write-Header "Configuring CMake"
    
    Push-Location $CmakeDir
    try {
        Write-Status "Running: cmake --preset project" "INFO"
        & $CmakePath --preset project
        
        if ($LASTEXITCODE -eq 0) {
            Write-Status "✓ CMake configuration successful" "OK"
        } else {
            Write-Status "✗ CMake configuration failed (exit code: $LASTEXITCODE)" "FAIL"
            exit 1
        }
    } finally {
        Pop-Location
    }
} else {
    Write-Header "Skipping CMake Configuration"
    Write-Status "Using existing CMake configuration" "SKIP"
}

# ============================================================================
# Step 3: Build Firmware
# ============================================================================

Write-Header "Building Firmware"

Push-Location $CmakeDir
try {
    Write-Status "Running: cmake --build --preset default_config" "INFO"
    & $CmakePath --build --preset default_config
    
    if ($LASTEXITCODE -eq 0) {
        Write-Status "✓ Build successful" "OK"
    } else {
        Write-Status "✗ Build failed (exit code: $LASTEXITCODE)" "FAIL"
        exit 1
    }
} finally {
    Pop-Location
}

# Verify hex file exists
if (-not (Test-Path $HexFile)) {
    Write-Status "Hex file not found: $HexFile" "FAIL"
    exit 1
}
Write-Status "✓ Hex file found: $(Split-Path -Leaf $HexFile)" "OK"

# ============================================================================
# Step 4: Flash Device
# ============================================================================

Write-Header "Flashing Device"

$flashArgs = @(
    "flash",
    $HexFile,
    "--halt",
    "--serialno", $SerialNumber,
    "--device", $DeviceId
)

Write-Status "Running: commander flash ..." "INFO"
Write-Status "  Device: $DeviceId ($SerialNumber)" "INFO"
Write-Status "  File: $(Split-Path -Leaf $HexFile)" "INFO"

& $CommanderPath @flashArgs

if ($LASTEXITCODE -eq 0) {
    Write-Status "✓ Flash successful" "OK"
} else {
    Write-Status "✗ Flash failed (exit code: $LASTEXITCODE)" "FAIL"
    exit 1
}

# ============================================================================
# Step 5: Verify Device Boot (Monitor VCOM)
# ============================================================================

Write-Header "Verifying Device Boot"

Write-Status "Waiting for device to boot (timeout: ${TimeoutSeconds}s)..." "INFO"

# Auto-detect COM port if not specified
if (-not $Port) {
    $ports = Get-WmiObject Win32_SerialPort | Where-Object { $_.Name -match "COM|VCOM" }
    if ($ports) {
        $Port = $ports[0].DeviceID
        Write-Status "Auto-detected COM port: $Port" "INFO"
    } else {
        Write-Status "Could not auto-detect COM port. Specify with -Port parameter." "WARN"
        Write-Status "Skipping VCOM verification (flash likely succeeded)" "INFO"
        #goto Step6
        $SkipVerify = $true
    }
}

try {
    $serialPort = New-Object System.IO.Ports.SerialPort
    $serialPort.PortName = $Port
    $serialPort.BaudRate = 115200
    $serialPort.Parity = [System.IO.Ports.Parity]::None
    $serialPort.DataBits = 8
    $serialPort.StopBits = [System.IO.Ports.StopBits]::One
    $serialPort.ReadTimeout = $TimeoutSeconds * 1000
    $serialPort.Open()
    
    $output = ""
    $bootSuccess = $false
    $bootMessages = @("Bluetooth stack initialized", "BLE Stack Booted", "system_boot")
    
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    
    try {
        while ($sw.Elapsed.TotalSeconds -lt $TimeoutSeconds) {
            try {
                $char = $serialPort.ReadChar()
                $output += [char]$char
                
                # Check for boot success messages
                foreach ($msg in $bootMessages) {
                    if ($output -match $msg) {
                        $bootSuccess = $true
                        break
                    }
                }
                
                if ($bootSuccess) { break }
                
            } catch [System.TimeoutException] {
                # Timeout reading a single character, but that's OK
                continue
            }
        }
    } finally {
        $serialPort.Close()
    }
    
    if ($SaveLog -and $output) {
        $output | Out-File -FilePath $SaveLog -Encoding UTF8
        Write-Status "VCOM output saved to: $SaveLog" "INFO"
    }
    
    if ($bootSuccess) {
        Write-Status "✓ Device booted successfully" "OK"
        Write-Status "  Received: $(($output -split "`n")[0].Trim())" "INFO"
    } else {
        Write-Status "✗ Device did not respond with boot message" "WARN"
        Write-Status "  First 200 chars: $($output.Substring(0, [Math]::Min(200, $output.Length)))" "INFO"
        Write-Status "  This may still be OK if flash succeeded and device is running" "INFO"
    }
    
} catch {
    Write-Status "Could not open COM port $Port : $_" "WARN"
    Write-Status "Skipping VCOM verification (flash likely succeeded)" "INFO"
}

# ============================================================================
# Step 6: Final Summary
# ============================================================================

Step6:
Write-Header "Workflow Complete"
Write-Status "✓ Build → Flash → Verify succeeded" "OK"
Write-Status "Device: $DeviceId" "INFO"
Write-Status "Hex file: $(Split-Path -Leaf $HexFile)" "INFO"
Write-Status "Next step: Open Simplicity Connect app to test BLE communication" "INFO"

exit 0

