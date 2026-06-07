<#
.SYNOPSIS
    Скрипт сборки прошивки canbox для Windows PowerShell
.DESCRIPTION
    Компилирует прошивку для всех 3 целей: VW-NC03 (NUC131), Volvo OD2 (STM32F1), QEMU.
    Является зеркалом логики Makefile, но на чистом PowerShell (не требует make).
    Использование: .\build.ps1 [цель] [действие]
    Цели: all, vw_nc03, volvo_od2, qemu
    Действия: build (по умолчанию), clean, flash
.EXAMPLE
    .\build.ps1                  # сборка всех целей
    .\build.ps1 clean            # удаление артефактов сборки
    .\build.ps1 volvo_od2        # сборка только Volvo OD2
    .\build.ps1 vw_nc03 flash    # прошивка VW NC03 (требуется OpenOCD)
#>

# Параметр командной строки: цель сборки
# Допустимые значения:
#   all            — сборка всех 3 прошивок (по умолчанию)
#   vw_nc03        — сборка VW-NC03 (Nuvoton NUC131, Cortex-M0)
#   volvo_od2      — сборка OD-Volvo-02 (STM32F103x8, Cortex-M3)
#   qemu           — сборка эмулятора QEMU (STM32VLDISCOVERY)
#   clean          — удаление артефактов сборки
#   run_qemu       — сборка и запуск в QEMU
#   flash_vw_nc03  — прошивка NUC131 через OpenOCD
#   flash_volvo_od2 — прошивка STM32F1 через OpenOCD (с RDP-разблокировкой)
param(
    [ValidateSet("all", "vw_nc03", "volvo_od2", "qemu", "clean", "run_qemu", "flash_vw_nc03", "flash_volvo_od2")]
    [string]$Target = "all"
)

# Политика обработки ошибок: немедленная остановка при любой ошибке
# Эквивалент 'set -e' в bash — предотвращает продолжение сборки при сбое компиляции/линковки
$ErrorActionPreference = "Stop"

# ===== Toolchain: кросс-компилятор ARM =====
# Переопределение через переменные окружения (для нестандартных путей установки)
# $env:CC        — путь к arm-none-eabi-gcc
# $env:OBJCOPY   — путь к arm-none-eabi-objcopy
# $env:QEMU_STM32 — путь к qemu-system-arm (с патчем для STM32)
$CC = if ($env:CC) { $env:CC } else { "arm-none-eabi-gcc" }
$OBJCOPY = if ($env:OBJCOPY) { $env:OBJCOPY } else { "arm-none-eabi-objcopy" }
$QEMU = if ($env:QEMU_STM32) { $env:QEMU_STM32 } else { "qemu-system-arm" }

# Проверка доступности toolchain в PATH
# Если arm-none-eabi-gcc не найден — выводится инструкция по установке и скрипт завершается
if (-not (Get-Command $CC -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: $CC not found in PATH" -ForegroundColor Red
    Write-Host "Download: https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases" -ForegroundColor Yellow
    exit 1
}

# ===== Общие флаги компиляции (все 3 цели) =====
# $COMMON_INCLUDES: пути include для общих заголовков прошивки
# $COMMON_CFLAGS: флаги компилятора (оптимизация, предупреждения, секции)
# $COMMON_LDFLAGS: флаги линковщика (статическая линковка, удаление мёртвого кода)
$COMMON_INCLUDES = "-Iinclude", "-I."
$COMMON_CFLAGS = @("-std=gnu99", "-Os", "-fno-common", "-ffunction-sections", "-fdata-sections",
                   "-Wstrict-prototypes", "-Wundef", "-Wextra", "-Wshadow", "-Wredundant-decls")
$COMMON_LDFLAGS = @("--static", "-lc", "-lm", "-Wl,--cref", "-Wl,--gc-sections")

# ===== Флаги STM32F1 (Volvo OD2 + QEMU) =====
# $F103_ARCH: архитектурные флаги для Cortex-M3 (STM32F103)
#   -mcpu=cortex-m3: целевой процессор
#   -mthumb: Thumb-инструкции
#   -mfix-cortex-m3-ldrd: обход бага LDRD
#   -msoft-float: программная эмуляция FPU
# $F103_INCLUDES: общие include + include libopencm3
# $F103_CFLAGS: полные флаги компиляции (архитектура + STM32F1 + include + оптимизация + nosys)
# $F103_LDFLAGS: полные флаги линковки (архитектура + статика + nosys + без startup-файлов)
$F103_ARCH = @("-mcpu=cortex-m3", "-mthumb", "-mfix-cortex-m3-ldrd", "-msoft-float", "-Wall")
$F103_INCLUDES = $COMMON_INCLUDES + "-Ilibopencm3/include"
$F103_CFLAGS = $F103_ARCH + "-DSTM32F1" + $F103_INCLUDES + $COMMON_CFLAGS + "--specs=nosys.specs"
$F103_LDFLAGS = $F103_ARCH + $COMMON_LDFLAGS + "--specs=nosys.specs", "-nostartfiles"

# ===== Флаги NUC131 (VW NC03) =====
# $NUC131_BSP_DIR: директория с Board Support Package Nuvoton NUC131
# $NUC131_INCLUDES: общие include + CMSIS + заголовки периферии NUC131
# $NUC131_ARCH: архитектурные флаги для Cortex-M0 (нет -mfix-cortex-m3-ldrd, т.к. M0)
# $NUC131_CFLAGS: полные флаги компиляции NUC131
# $NUC131_LDFLAGS: полные флаги линковки NUC131 (+ --allow-multiple-definition для startup)
$NUC131_BSP_DIR = "nuc131bsp"
$NUC131_INCLUDES = $COMMON_INCLUDES +
    "-I$NUC131_BSP_DIR/Library/CMSIS/Include",
    "-I$NUC131_BSP_DIR/Library/Device/Nuvoton/NUC131/Include",
    "-I$NUC131_BSP_DIR/Library/StdDriver/inc"
$NUC131_ARCH = @("-mcpu=cortex-m0", "-mthumb", "-msoft-float", "-Wall")
$NUC131_CFLAGS = $NUC131_ARCH + $NUC131_INCLUDES + $COMMON_CFLAGS
$NUC131_LDFLAGS = $NUC131_ARCH + $COMMON_LDFLAGS + "-L.", "-Wl,--allow-multiple-definition"

# ===== Общие исходные файлы ядра (shared core) =====
# Компилируются для всех 3 целей (Volvo, VW, QEMU) с разными флагами
# main.c    — точка входа, главный цикл
# canbox.c  — протокол обмена с Android (включает canbox_protos/*.c)
# ring.c    — кольцевой буфер для USART
# car.c     — парсинг CAN-сообщений (включает cars/*.c)
# tick.c    — системный таймер (systick)
# hw.c      — инициализация hardware abstraction layer
# sbrk.c    — реализация _sbrk для malloc на bare-metal
$CORE_SOURCES = @(
    "src/main.c", "src/canbox.c", "src/ring.c",
    "src/car.c", "src/tick.c", "src/hw.c", "src/sbrk.c"
)

# ===== Исходники libopencm3 (только для STM32F1-целей: Volvo + QEMU) =====
# Драйверы периферии STM32F1: GPIO, RCC, FLASH, USART, SYSTICK, NVIC, CAN, IWDG, EXTI, PWR
# Компилируются один раз для обеих целей, но с разными суффиксами объектных файлов
$LIBOPENCM3_SOURCES = @(
    "libopencm3/lib/stm32/f1/gpio.c",
    "libopencm3/lib/stm32/f1/rcc.c",
    "libopencm3/lib/stm32/f1/flash.c",
    "libopencm3/lib/stm32/common/gpio_common_all.c",
    "libopencm3/lib/stm32/common/usart_common_all.c",
    "libopencm3/lib/stm32/common/usart_common_f124.c",
    "libopencm3/lib/stm32/common/rcc_common_all.c",
    "libopencm3/lib/cm3/vector.c",
    "libopencm3/lib/cm3/systick.c",
    "libopencm3/lib/cm3/nvic.c",
    "libopencm3/lib/cm3/sync.c",
    "libopencm3/lib/stm32/common/flash_common_f01.c",
    "libopencm3/lib/stm32/common/flash_common_all.c",
    "libopencm3/lib/stm32/can.c",
    "libopencm3/lib/stm32/common/iwdg_common_all.c",
    "libopencm3/lib/stm32/common/exti_common_all.c",
    "libopencm3/lib/stm32/common/pwr_common_v1.c"
)

# ===== Платформенно-специфичные исходники (реализация hw_*.h API) =====
# $VOLVO_OD2_SOURCES: HAL для STM32F103 (Volvo OD2) — clock, CAN, GPIO, tick, USART, sleep, conf
# $QEMU_SOURCES:      HAL-заглушки для QEMU — те же модули, но с эмуляцией вместо регистров
# $VW_NC03_SOURCES:   HAL для NUC131 (VW NC03) — аналогичный набор модулей
# Каждый список формируется динамически из массива имён файлов + пути к платформе
$VOLVO_OD2_SOURCES = @("hw_clock.c","hw_can.c","hw_gpio.c","hw_tick.c","hw_usart.c","hw_sleep.c","hw_conf.c" |
    ForEach-Object { "volvo_od2/fw/$_" })

$QEMU_SOURCES = @("hw_clock.c","hw_can.c","hw_gpio.c","hw_tick.c","hw_usart.c","hw_sleep.c","hw_conf.c" |
    ForEach-Object { "qemu/fw/$_" })

$VW_NC03_SOURCES = @("hw_clock.c","hw_can.c","hw_gpio.c","hw_tick.c","hw_usart.c","hw_sleep.c","hw_conf.c" |
    ForEach-Object { "vw_nc03/fw/$_" })

# ===== Исходники BSP NUC131 (только для VW NC03) =====
# Стандартные драйверы Nuvoton: sys, clk, gpio, uart, can, fmc
# system_NUC131.c: инициализация системных часов
# startup_NUC131.S: векторная таблица и startup-код (ассемблер, компилируется как .c через -c)
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

# ===== Функции сборки =====

# Компиляция одного исходного файла в объектный
# Параметры:
#   $Flags  — массив флагов компилятора (CFLAGS для конкретной цели)
#   $Source — путь к исходнику (.c или .S)
#   $Output — путь к выходному объектному файлу
# Выводит цветное сообщение "CC source.c" и запускает компилятор
# При ошибке компиляции — аварийное завершение скрипта
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

# Возвращает суффикс объектного файла для указанной цели сборки
# Используется для предотвращения конфликтов кэша при параллельной сборке:
#   volvo_od2 → .vo  (Volvo Object)
#   vw_nc03   → .vwo (VW Object)
#   qemu      → .qemu (QEMU Object)
# Параметры:
#   $Target — имя цели (volvo_od2 / vw_nc03 / qemu)
# Возвращает: строку с суффиксом (включая точку)
function Get-ObjSuffix { param([string]$Target)
    switch ($Target) {
        "volvo_od2" { return ".vo" }
        "vw_nc03"   { return ".vwo" }
        "qemu"       { return ".qemu" }
        default      { return ".o" }
    }
}

# Линковка объектных файлов в ELF
# Параметры:
#   $Flags        — массив флагов линковщика (LDFLAGS)
#   $Objects      — массив путей к объектным файлам
#   $Output       — путь к выходному ELF-файлу
#   $LinkerScript — путь к скрипту линкера (.ld)
# Выводит цветное сообщение "LD output.elf" и запускает линковщик
# При ошибке линковки — аварийное завершение скрипта
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

# Универсальная функция сборки цели: компиляция + линковка + objcopy
# Параметры:
#   $Name          — имя цели (для заголовка, например "volvo_od2")
#   $CFLAGS        — массив флагов компиляции
#   $Sources       — массив основных исходников (shared core)
#   $ExtraSources  — массив дополнительных исходников (HAL + библиотеки)
#   $LDFLAGS       — массив флагов линковки
#   $LinkerScript  — путь к скрипту линкера
#   $BinOutput     — путь к выходному .bin файлу
# Логика:
#   1. Создаёт out/ директорию
#   2. Для каждого исходника: определяет путь объекта (out/src/file.vo)
#   3. Создаёт поддиректории в out/ при необходимости
#   4. Проверяет время модификации: перекомпилирует только изменённые файлы
#   5. Линкует все объектные файлы в ELF
#   6. Конвертирует ELF → BIN через objcopy
#   7. Выводит размер полученной прошивки в KB
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

    # Создание директории out/ (с очищением ошибки если уже существует)
    New-Item -ItemType Directory -Path out -Force | Out-Null

    $allSources = $Sources + $ExtraSources
    $suffix = Get-ObjSuffix -Target $Name
    $objects = @()
    foreach ($src in $allSources) {
        # Формирование пути объекта: out/src/main.c → out/src/main.vo
        $obj = "out/$src" -replace '\.c$', $suffix -replace '\.S$', $suffix
        # Создание поддиректорий в out/ для вложенных путей
        $objDir = Split-Path $obj -Parent
        if ($objDir -and -not (Test-Path $objDir)) { New-Item -ItemType Directory -Path $objDir -Force | Out-Null }
        $objects += $obj
        # Инкрементальная сборка: перекомпилировать только если исходник новее объекта
        # (или объект отсутствует)
        if (-not (Test-Path $obj) -or ((Get-Item $src).LastWriteTime -gt (Get-Item $obj -ErrorAction SilentlyContinue).LastWriteTime)) {
            Invoke-Compile -Flags $CFLAGS -Source $src -Output $obj
        }
    }

    Invoke-Link -Flags $LDFLAGS -Objects $objects -Output $elf -LinkerScript $LinkerScript

    # Конвертация ELF → raw binary (плоский бинарник для загрузки во flash)
    Write-Host "  OBJCOPY  $BinOutput" -ForegroundColor Magenta
    & $OBJCOPY -O binary $elf $BinOutput
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: objcopy failed" -ForegroundColor Red
        exit 1
    }

    # Вывод размера прошивки
    $size = (Get-Item $BinOutput).Length
    Write-Host "  OK  $BinOutput ($([math]::Round($size/1024,1)) KB)" -ForegroundColor Green
}

# Сборка прошивки Volvo OD2 (STM32F103x8, Cortex-M3)
# Использует флаги F103_* (Cortex-M3 + libopencm3)
# Скрипт линкера: volvo_od2/fw/x32f103x8.ld (совместимый с STM32F103)
function Build-VolvoOD2 {
    Build-Target -Name "volvo_od2" `
        -CFLAGS $F103_CFLAGS `
        -Sources $CORE_SOURCES `
        -ExtraSources ($VOLVO_OD2_SOURCES + $LIBOPENCM3_SOURCES) `
        -LDFLAGS ($F103_LDFLAGS + "-Llibopencm3/lib/stm32/f1") `
        -LinkerScript "volvo_od2/fw/x32f103x8.ld" `
        -BinOutput "out/volvo_od2.bin"
}

# Сборка прошивки QEMU (эмулятор STM32VLDISCOVERY)
# Использует те же флаги F103_* (Cortex-M3), т.к. QEMU эмулирует STM32F100RB
# Скрипт линкера: qemu/fw/stm32vldiscovery.ld (для платы с 128K flash)
function Build-Qemu {
    Build-Target -Name "qemu" `
        -CFLAGS $F103_CFLAGS `
        -Sources $CORE_SOURCES `
        -ExtraSources ($QEMU_SOURCES + $LIBOPENCM3_SOURCES) `
        -LDFLAGS ($F103_LDFLAGS + "-Llibopencm3/lib/stm32/f1") `
        -LinkerScript "qemu/fw/stm32vldiscovery.ld" `
        -BinOutput "out/qemu.bin"
}

# Сборка прошивки VW NC03 (Nuvoton NUC131, Cortex-M0)
# Использует флаги NUC131_* (Cortex-M0 + NUC131 BSP)
# Скрипт линкера: gcc_arm.ld из BSP Nuvoton
function Build-VwNc03 {
    Build-Target -Name "vw_nc03" `
        -CFLAGS $NUC131_CFLAGS `
        -Sources $CORE_SOURCES `
        -ExtraSources ($VW_NC03_SOURCES + $NUC131_BSP_SOURCES) `
        -LDFLAGS $NUC131_LDFLAGS `
        -LinkerScript "$NUC131_BSP_DIR/Library/Device/Nuvoton/NUC131/Source/GCC/gcc_arm.ld" `
        -BinOutput "out/vw_nc03.bin"
}

# Очистка артефактов сборки
# Удаляет директорию out/ со всеми объектными файлами, ELF, бинарниками и map-файлами
function Invoke-Clean {
    Write-Host "Cleaning build artifacts..." -ForegroundColor Yellow
    if (Test-Path out) {
        Remove-Item -Recurse -Force out
        Write-Host "Removed out/ directory." -ForegroundColor Green
    } else {
        Write-Host "Nothing to clean." -ForegroundColor Green
    }
}

# Вывод справки по использованию скрипта
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

# ===== Главный блок (точка входа) =====

# Переход в директорию скрипта (для корректных относительных путей)
Push-Location $PSScriptRoot

try {
    switch ($Target) {
        # Очистка: удаление out/ и завершение
        "clean" {
            Invoke-Clean
            exit 0
        }
        # Сборка всех 3 целей последовательно: VW → Volvo → QEMU
        "all" {
            Build-VwNc03
            Build-VolvoOD2
            Build-Qemu
        }
        # Сборка отдельных целей
        "vw_nc03" { Build-VwNc03 }
        "volvo_od2" { Build-VolvoOD2 }
        "qemu" { Build-Qemu }
        # Прошивка VW NC03 через OpenOCD
        # Сначала собирает прошивку, затем вызывает OpenOCD:
        #   init → reset halt → numicro chip_erase → program → verify → reset
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
        # Прошивка Volvo OD2 через OpenOCD (с RDP-разблокировкой)
        # Сначала собирает прошивку, затем вызывает OpenOCD:
        #   init → reset halt → stm32f1x unlock → program → verify → reset
        # Примечание: RDP-разблокировка стирает весь flash!
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
        # Сборка + запуск в QEMU (эмулятор STM32)
        # Параметры QEMU: машина stm32vldiscovery, kernel из .bin, serial→stdio, без GUI
        "run_qemu" {
            Build-Qemu
            if (Get-Command $QEMU -ErrorAction SilentlyContinue) {
                Write-Host "Starting QEMU (stm32vldiscovery)..." -ForegroundColor Yellow
                & $QEMU -M stm32vldiscovery -kernel out/qemu.bin -serial stdio -display none
            } else {
                Write-Host "$QEMU not found in PATH." -ForegroundColor Yellow
            }
        }
        # Неизвестная цель — показать справку
        default {
            Show-Help
        }
    }
}
finally {
    # Восстановление исходной директории (выполняется всегда, даже при ошибке)
    Pop-Location
}
