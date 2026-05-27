<#
.SYNOPSIS
    Build script for canbox firmware (Windows PowerShell)
.DESCRIPTION
    Compiles firmware for all 3 targets: VW-NC03 (NUC131), Volvo OD2 (STM32F1), QEMU.
    Usage: .\build.ps1 [target] [action]
    Targets: all, vw_nc03, volvo_od2, qemu
    Actions: build (default), clean, flash
.EXAMPLE
    .\build.ps1                  # build all targets
    .\build.ps1 clean            # remove build artifacts
    .\build.ps1 volvo_od2        # build only Volvo OD2
    .\build.ps1 vw_nc03 flash    # flash VW NC03 (requires OpenOCD)
#>

param(
    [ValidateSet("all", "vw_nc03", "volvo_od2", "qemu", "clean", "run_qemu", "flash_vw_nc03", "flash_volvo_od2")]
    [string]$Target = "all"
)

$ErrorActionPreference = "Stop"

# Toolchain — override via environment variables
$CC = if ($env:CC) { $env:CC } else { "arm-none-eabi-gcc" }
$OBJCOPY = if ($env:OBJCOPY) { $env:OBJCOPY } else { "arm-none-eabi-objcopy" }
$QEMU = if ($env:QEMU_STM32) { $env:QEMU_STM32 } else { "qemu-system-arm" }

# Verify toolchain is available
if (-not (Get-Command $CC -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: $CC not found in PATH" -ForegroundColor Red
    Write-Host "Download: https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases" -ForegroundColor Yellow
    exit 1
}

# ===== Common flags =====
$COMMON_INCLUDES = "-Iinclude", "-I."
$COMMON_CFLAGS = @("-std=gnu99", "-Os", "-fno-common", "-ffunction-sections", "-fdata-sections",
                   "-Wstrict-prototypes", "-Wundef", "-Wextra", "-Wshadow", "-Wredundant-decls")
$COMMON_LDFLAGS = @("--static", "-lc", "-lm", "-Wl,--cref", "-Wl,--gc-sections")

# ===== STM32F1 (Volvo OD2 + QEMU) =====
$F103_ARCH = @("-mcpu=cortex-m3", "-mthumb", "-mfix-cortex-m3-ldrd", "-msoft-float", "-Wall")
$F103_INCLUDES = $COMMON_INCLUDES + "-Ilibopencm3/include"
$F103_CFLAGS = $F103_ARCH + "-DSTM32F1" + $F103_INCLUDES + $COMMON_CFLAGS + "--specs=nosys.specs"
$F103_LDFLAGS = $F103_ARCH + $COMMON_LDFLAGS + "--specs=nosys.specs", "-nostartfiles"

# ===== NUC131 (VW NC03) =====
$NUC131_BSP_DIR = "vw_nc03/fw/nuc131bsp"
$NUC131_INCLUDES = $COMMON_INCLUDES +
    "-I$NUC131_BSP_DIR/Library/CMSIS/Include",
    "-I$NUC131_BSP_DIR/Library/Device/Nuvoton/NUC131/Include",
    "-I$NUC131_BSP_DIR/Library/StdDriver/inc"
$NUC131_ARCH = @("-mcpu=cortex-m0", "-mthumb", "-msoft-float", "-Wall")
$NUC131_CFLAGS = $NUC131_ARCH + $NUC131_INCLUDES + $COMMON_CFLAGS
$NUC131_LDFLAGS = $NUC131_ARCH + $COMMON_LDFLAGS + "-L.", "-Wl,--allow-multiple-definition"

# Core source files (shared across all targets)
$CORE_SOURCES = @(
    "src/main.c", "src/canbox.c", "src/ring.c",
    "src/car.c", "src/tick.c", "src/hw.c", "src/sbrk.c"
)

# ===== libopencm3 objects =====
$LIBOPENCM3_SOURCES = @(
    "libopencm3/lib/stm32/f1/gpio.c",
    "libopencm3/lib/stm32/f1/rcc.c",
    "libopencm3/lib/stm32/f1/pwr.c",
    "libopencm3/lib/stm32/f1/flash.c",
    "libopencm3/lib/stm32/common/gpio_common_all.c",
    "libopencm3/lib/stm32/common/usart_common_all.c",
    "libopencm3/lib/stm32/common/usart_common_f124.c",
    "libopencm3/lib/stm32/common/rcc_common_all.c",
    "libopencm3/lib/stm32/common/pwr_common_all.c",
    "libopencm3/lib/cm3/vector.c",
    "libopencm3/lib/cm3/systick.c",
    "libopencm3/lib/cm3/nvic.c",
    "libopencm3/lib/cm3/sync.c",
    "libopencm3/lib/stm32/common/flash_common_f01.c",
    "libopencm3/lib/stm32/can.c",
    "libopencm3/lib/stm32/common/iwdg_common_all.c",
    "libopencm3/lib/stm32/common/exti_common_all.c"
)

# ===== Platform-specific sources =====
$VOLVO_OD2_SOURCES = @("hw_clock.c","hw_can.c","hw_gpio.c","hw_tick.c","hw_usart.c","hw_sleep.c","hw_conf.c" |
    ForEach-Object { "volvo_od2/fw/$_" })

$QEMU_SOURCES = @("hw_clock.c","hw_can.c","hw_gpio.c","hw_tick.c","hw_usart.c","hw_sleep.c","hw_conf.c" |
    ForEach-Object { "qemu/fw/$_" })

$VW_NC03_SOURCES = @("hw_clock.c","hw_can.c","hw_gpio.c","hw_tick.c","hw_usart.c","hw_sleep.c","hw_conf.c" |
    ForEach-Object { "vw_nc03/fw/$_" })

$NUC131_BSP_SOURCES = @(
    "$NUC131_BSP_DIR/Library/StdDriver/src/sys.c",
    "$NUC131_BSP_DIR/Library/StdDriver/src/clk.c",
    "$NUC131_BSP_DIR/Library/StdDriver/src/gpio.c",
    "$NUC131_BSP_DIR/Library/StdDriver/src/uart.c",
    "$NUC131_BSP_DIR/Library/StdDriver/src/can.c",
    "$NUC131_BSP_DIR/Library/StdDriver/src/fmc.c",
    "$NUC131_BSP_DIR/Library/Device/Nuvoton/NUC131/Source/system_NUC131.c",
    "$NUC131_BSP_DIR/Library/Device/Nuvoton/NUC131/Source/GCC/startup_NUC131.S"
)

# ===== Functions =====

function Invoke-Compile {
    param([string[]]$Flags, [string]$Source, [string]$Output)
    $msg = "  CC  $Source"
    Write-Host $msg -ForegroundColor Cyan
    $args = $Flags + "-c", $Source, "-o", $Output
    & $CC $args 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Compilation failed: $Source" -ForegroundColor Red
        exit 1
    }
}

# Object file suffix per target (prevents cross-target cache conflicts)
function Get-ObjSuffix { param([string]$Target)
    switch ($Target) {
        "volvo_od2" { return ".vo" }
        "vw_nc03"   { return ".vwo" }
        "qemu"       { return ".qemu" }
        default      { return ".o" }
    }
}

function Invoke-Link {
    param([string[]]$Flags, [string[]]$Objects, [string]$Output, [string]$LinkerScript)
    $msg = "  LD  $Output"
    Write-Host $msg -ForegroundColor Yellow
    $args = $Flags + "-T$LinkerScript" + $Objects + "-o", $Output
    & $CC $args 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Linking failed: $Output" -ForegroundColor Red
        exit 1
    }
}

function Build-Target {
    param(
        [string]$Name,
        [string[]]$CFLAGS,
        [string[]]$Sources,
        [string[]]$ExtraSources,
        [string[]]$LDFLAGS,
        [string]$LinkerScript,
        [string]$BinOutput
    )

    Write-Host "`n===== Building $Name =====" -ForegroundColor Green
    $elf = $BinOutput -replace '\.bin$', '.elf'
    $map = $BinOutput -replace '\.bin$', '.map'

    # Ensure out/ directory exists
    New-Item -ItemType Directory -Path out -Force | Out-Null

    $allSources = $Sources + $ExtraSources
    $suffix = Get-ObjSuffix -Target $Name
    $objects = @()
    foreach ($src in $allSources) {
        $obj = "out/$src" -replace '\.c$', $suffix -replace '\.S$', $suffix
        # Ensure subdirectory exists for nested objects
        $objDir = Split-Path $obj -Parent
        if ($objDir -and -not (Test-Path $objDir)) { New-Item -ItemType Directory -Path $objDir -Force | Out-Null }
        $objects += $obj
        if (-not (Test-Path $obj) -or ((Get-Item $src).LastWriteTime -gt (Get-Item $obj -ErrorAction SilentlyContinue).LastWriteTime)) {
            Invoke-Compile -Flags $CFLAGS -Source $src -Output $obj
        }
    }

    Invoke-Link -Flags $LDFLAGS -Objects $objects -Output $elf -LinkerScript $LinkerScript

    Write-Host "  OBJCOPY  $BinOutput" -ForegroundColor Magenta
    & $OBJCOPY -O binary $elf $BinOutput
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: objcopy failed" -ForegroundColor Red
        exit 1
    }

    $size = (Get-Item $BinOutput).Length
    Write-Host "  OK  $BinOutput ($([math]::Round($size/1024,1)) KB)" -ForegroundColor Green
}

function Build-VolvoOD2 {
    Build-Target -Name "volvo_od2" `
        -CFLAGS $F103_CFLAGS `
        -Sources $CORE_SOURCES `
        -ExtraSources ($VOLVO_OD2_SOURCES + $LIBOPENCM3_SOURCES) `
        -LDFLAGS ($F103_LDFLAGS + "-Llibopencm3/lib/stm32/f1") `
        -LinkerScript "volvo_od2/fw/x32f103x8.ld" `
        -BinOutput "out/volvo_od2.bin"
}

function Build-Qemu {
    Build-Target -Name "qemu" `
        -CFLAGS $F103_CFLAGS `
        -Sources $CORE_SOURCES `
        -ExtraSources ($QEMU_SOURCES + $LIBOPENCM3_SOURCES) `
        -LDFLAGS ($F103_LDFLAGS + "-Llibopencm3/lib/stm32/f1") `
        -LinkerScript "qemu/fw/stm32vldiscovery.ld" `
        -BinOutput "out/qemu.bin"
}

function Build-VwNc03 {
    Build-Target -Name "vw_nc03" `
        -CFLAGS $NUC131_CFLAGS `
        -Sources $CORE_SOURCES `
        -ExtraSources ($VW_NC03_SOURCES + $NUC131_BSP_SOURCES) `
        -LDFLAGS $NUC131_LDFLAGS `
        -LinkerScript "$NUC131_BSP_DIR/Library/Device/Nuvoton/NUC131/Source/GCC/gcc_arm.ld" `
        -BinOutput "out/vw_nc03.bin"
}

function Invoke-Clean {
    Write-Host "Cleaning build artifacts..." -ForegroundColor Yellow
    if (Test-Path out) {
        Remove-Item -Recurse -Force out
        Write-Host "Removed out/ directory." -ForegroundColor Green
    } else {
        Write-Host "Nothing to clean." -ForegroundColor Green
    }
}

function Show-Help {
    Write-Host @"
canbox build script (Windows PowerShell)

Usage: .\build.ps1 [target] [action]

Targets:
  all         Build all 3 firmware targets (default)
  vw_nc03     Build VW-NC03 (Nuvoton NUC131, Cortex-M0)
  volvo_od2   Build OD-Volvo-02 (STM32F103x8, Cortex-M3)
  qemu        Build QEMU target (STM32VLDISCOVERY)

Actions:
  clean       Remove all build artifacts
  run_qemu    Build and run in QEMU (requires patched QEMU)
  flash_*     Flash via OpenOCD (requires hardware + OpenOCD)

Examples:
  .\build.ps1                 Build everything
  .\build.ps1 clean           Remove build artifacts
  .\build.ps1 volvo_od2       Build only Volvo OD2
  .\build.ps1 vw_nc03         Build only VW NC03

Requirements:
  arm-none-eabi-gcc (xPack GNU Arm Embedded GCC)
  Download: https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases

"@
}

# ===== Main =====

Push-Location $PSScriptRoot

try {
    switch ($Target) {
        "clean" {
            Invoke-Clean
            exit 0
        }
        "all" {
            Build-VwNc03
            Build-VolvoOD2
            Build-Qemu
        }
        "vw_nc03" { Build-VwNc03 }
        "volvo_od2" { Build-VolvoOD2 }
        "qemu" { Build-Qemu }
        "flash_vw_nc03" {
            Build-VwNc03
            $ocd = if (Get-Command "openocd" -ErrorAction SilentlyContinue) { "openocd" } else { $null }
            if ($ocd) {
                Write-Host "Flashing VW-NC03..." -ForegroundColor Yellow
                & $ocd -f vw_nc03/fw/nuc131.cfg -c "init; reset halt; numicro chip_erase; program out/vw_nc03.bin 0x0 verify; reset; exit"
            } else {
                Write-Host "OpenOCD not found in PATH." -ForegroundColor Yellow
            }
        }
        "flash_volvo_od2" {
            Build-VolvoOD2
            $ocd = if (Get-Command "openocd" -ErrorAction SilentlyContinue) { "openocd" } else { $null }
            if ($ocd) {
                Write-Host "Flashing OD-Volvo-02 (3-stage RDP unlock)..." -ForegroundColor Yellow
                & $ocd -f volvo_od2/fw/stm32f103x8.cfg -c "init; reset halt" -c "stm32f1x unlock 0; reset halt; program out/volvo_od2.bin 0x8000000 verify; reset; exit"
            } else {
                Write-Host "OpenOCD not found in PATH." -ForegroundColor Yellow
            }
        }
        "run_qemu" {
            Build-Qemu
            if (Get-Command $QEMU -ErrorAction SilentlyContinue) {
                Write-Host "Starting QEMU (stm32vldiscovery)..." -ForegroundColor Yellow
                & $QEMU -M stm32vldiscovery -kernel out/qemu.bin -serial stdio -display none
            } else {
                Write-Host "$QEMU not found in PATH." -ForegroundColor Yellow
            }
        }
        default {
            Show-Help
        }
    }
}
finally {
    Pop-Location
}
