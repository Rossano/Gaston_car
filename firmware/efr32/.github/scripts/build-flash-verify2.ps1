#Requires -Version 5.1

param(
    [switch]$SkipConfigure,
    [string]$Port = $null,
    [int]$TimeoutSeconds = 10,
    [string]$SaveLog = $null
)

$ErrorActionPreference = "Stop"

# --- Configuration ---
$ProjectRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommandPath))
$CmakeDir    = Join-Path $ProjectRoot "cmake_gcc"
$HexFile     = Join-Path $CmakeDir "build/base/bt_soc_gaston.hex"

$CommanderPath = "C:/Users/rossa/.silabs/slt/installs/archive/Simplicity Commander/commander.exe"
$SerialNumber  = "440242346"
$DeviceId      = "EFR32BG22C224F512IM40"
$CmakePath     = "C:/Users/rossa/.silabs/slt/installs/conan/p/cmakefa35ab0687064/p/bin/cmake.exe"

# --- Funzioni di log semplificate ---
function Write-Log($msg, $color = "White") {
    $timestamp = Get-Date -Format "HH:mm:ss"
    Write-Host "[$timestamp] $msg" -ForegroundColor $color
}

# --- 1. Verifiche ---
Write-Log "--- Verifying Prerequisites ---" "Cyan"
if (-not (Test-Path $CmakePath))     { Write-Log "FAIL: CMake non trovato" "Red"; exit 1 }
if (-not (Test-Path $CommanderPath)) { Write-Log "FAIL: Commander non trovato" "Red"; exit 1 }

# --- 2. Build ---
if (-not $SkipConfigure) {
    Write-Log "Configuring CMake..." "Cyan"
    Set-Location $CmakeDir
    & $CmakePath --preset project
}

Write-Log "Building..." "Cyan"
Set-Location $CmakeDir
& $CmakePath --build --preset default_config
if ($LASTEXITCODE -ne 0) { Write-Log "Build FAILED" "Red"; exit 1 }

# --- 3. Flash ---
Write-Log "Flashing Device $SerialNumber..." "Cyan"
& $CommanderPath flash $HexFile --halt --serialno $SerialNumber --device $DeviceId
if ($LASTEXITCODE -ne 0) { Write-Log "Flash FAILED" "Red"; exit 1 }
Write-Log "Flash OK" "Green"

# --- 4. Verify VCOM ---
Write-Log "--- Verifying Boot ---" "Cyan"
if (-not $Port) {
    $p = Get-CimInstance Win32_SerialPort | Where-Object { $_.Name -match "COM|VCOM|JLink" }
    if ($p) { $Port = $p[0].DeviceID }
}

if ($Port) {
    Write-Log "Monitoring $Port for $TimeoutSeconds seconds..." "Yellow"
    try {
        $s = New-Object System.IO.Ports.SerialPort($Port, 115200, "None", 8, "One")
        $s.ReadTimeout = 1000
        $s.Open()
        $found = $false
        $timer = [System.Diagnostics.Stopwatch]::StartNew()
        
        while ($timer.Elapsed.TotalSeconds -lt $TimeoutSeconds) {
            try {
                $line = $s.ReadLine()
                if ($line -match "system_boot|Bluetooth stack initialized") {
                    $found = $true
                    break
                }
            } catch { continue }
        }
        $s.Close()
        if ($found) { Write-Log "SUCCESS: Device Booted!" "Green" }
        else { Write-Log "WARNING: Boot message not detected (timeout)" "Yellow" }
    } catch {
        Write-Log "Could not open port $Port" "Red"
    }
}

Write-Log "Workflow Complete." "Cyan"
exit 0