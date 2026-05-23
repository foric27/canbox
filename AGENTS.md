# canbox Project Rules

## Overview
Firmware for CAN bus adapters (VW NC03 / Volvo OD2) and a QEMU emulation target.
STM32F103x8 (Cortex-M3) + Nuvoton NUC131 (Cortex-M0). Desktop emulator via Qt.

## Structure

```
canbox/
├── src/*.c                # Shared firmware core (8 .c files)
├── include/*.h            # Shared firmware headers (10 .h files)
├── cars/*.c               # Per-car CAN message handlers (included into car.c)
├── volvo_od2/
│   ├── fw/hw_*.c          # STM32F103 hardware layer
│   └── hw/                # KiCad PCB/schematics
├── vw_nc03/
│   ├── fw/hw_*.c          # NUC131 hardware layer
│   └── hw/                # KiCad PCB/schematics
├── qemu/fw/hw_*.c         # QEMU hardware stubs
├── qt/                    # Desktop emulator (C++/Qt)
├── libopencm3/            # Vendored STM32 HAL (not submodule)
├── *.diff                 # Patches: openocd-fix.diff, qemu-7.2.0-fix.diff
└── Makefile*              # Build: root + 4 variant Makefiles
```

**Architecture**: Shared core (`src/main.c`, `src/canbox.c`, `src/car.c`, `src/conf.c`, `src/hw.c`, `src/ring.c`, `src/tick.c`, `src/sbrk.c`) compiled 3× for different platforms. Platform differentiation via `hw_*.c` implementations in each target's `fw/` directory. Headers in `include/`.

## Where to Look

| Task | Location | Notes |
|------|----------|-------|
| Main loop / timer dispatch | `src/main.c` | 1ms, 5ms, 100ms, 250ms, 1000ms timer domains |
| CAN message → Android protocol | `src/canbox.c` | Raise VW(PQ/MQB), Oudi BMW, HiWorld protocols |
| CAN packet parsing (per car) | `cars/*.c` | Included into `src/car.c` via `#include` |
| Configuration read/write | `src/conf.c` | Flash-based, wear-leveling via ring buffer |
| Hardware abstraction API | `include/hw.h`, `include/hw_can.h`, `include/hw_usart.h`, … | Headers in `include/` |
| Platform-specific HAL impl | `{target}/fw/hw_*.c` | STM32F1: libopencm3, NUC131: BSP, QEMU: stubs |
| Qt emulator entry | `qt/main.cpp` | Stubs `hw_usart_get`, `conf_get_car`, `hw_can_*` |
| Systick / timer flags | `src/tick.c` | Generates flag_1ms, flag_5ms, flag_100ms, flag_250ms, flag_1000ms |
| Heap (malloc/sbrk) | `src/sbrk.c` | Custom `_sbrk_r`, `#ifdef STM32F1` for different linker symbols |
| Ring buffer | `src/ring.c` | Used by USART RX/TX |
| Canbox protocol headers | `include/canbox.h` | `canbox_process`, `canbox_park_process`, key callbacks |
| Car state API | `include/car.h` | Doors, radar, illumination, selector, VIN, climate |
| Config schema | `include/conf.h` | `e_car_t` (7 cars), `e_canbox_t` (4 protocols), `MAX_REAR_DELAY` |

## Code Map

| Symbol | Type | Location | Role |
|--------|------|----------|------|
| `main()` | function | `src/main.c:454` | Entry: hw_setup → conf_read → car_init → main loop |
| `carstate` | struct | `src/car.c:58` | Global car state (doors, speed, radar, climate…) |
| `timer` | volatile struct | `src/tick.c:3` | Timer flags: flag_tick, flag_5ms, flag_100ms, flag_250ms, flag_1000ms |
| `conf` | struct | `src/conf.c:35` | Flash-stored config (car, canbox, illum, rear_delay) |
| `car_init()` | function | `src/car.c` | Selects car handler, registers CAN messages from `cars/*.c` |
| `car_process(ms)` | function | `src/car.c` | Dispatches CAN messages with timeout tracking |
| `canbox_process()` | function | `src/canbox.c` | Sends car state to Android via USART |
| `canbox_park_process()` | function | `src/canbox.c` | Sends parking sensor data |
| `hw_setup()` | function | `src/hw.c:18` | Init: clock → GPIO → systick → USART → CAN → conf |
| `hw_sleep()` | function | `src/hw.c:37` | Disable periphs → CPU sleep → re-init on wake |
| `e_car_t` | enum | `include/conf.h:9` | 7 car types + qcar (Qt emulator) |
| `e_canbox_t` | enum | `include/conf.h:24` | 4 Android protocols |
| `key_cb_t` | struct | `include/car.h:54` | SWC key callbacks (volume, prev/next, mode, navi, mute) |

## Conventions (observe strictly — no config files exist)

**Language**: C99 (`-std=gnu99`), minor C++ (Qt emulator only)

**Coding style** (extracted from source, no .clang-format):
- **Indent**: tabs (not spaces)
- **Braces**: K&R — opening `{` on same line for functions, if/for/while
- **Line length**: ~100 chars (soft limit, rarely exceeds 120)
- **Naming**:
  - functions: `snake_case` (`conf_read`, `hw_gpio_setup`)
  - types: `snake_case_t` (`car_state_t`, `msg_can_t`)
  - enums: `e_` prefix (`e_car_t`), values: `e_` + snake_case (`e_car_lr2_2007my`)
  - macros/constants: `UPPER_SNAKE_CASE` (`MAX_REAR_DELAY`, `TICK_HZ`)
  - enum size sentinel: `_nums` / `_nms` suffix (`e_car_nums`, `e_cb_nums`)
- **Header guards**: `#ifndef FILENAME_H` / `#define FILENAME_H` / `#endif`
- **Includes**: stdlib first (`<...>`), then project (`"..."`)
- **Pointer style**: `*` attached to type (`uint8_t * buf`)
- **struct init**: C99 designated initializers (`{.field = value,}`)
- **Null checks**: C-style (`if (!ptr)`, `if (!var)`)
- **Comments**: `//` for single-line, `/* */` for block
- **Attribute macros**: `__attribute__((packed))`, `__attribute__((aligned(N)))`, `#pragma pack(1)` for flash structs

**Compile flags**: `-Os -fno-common -ffunction-sections -fdata-sections -Wall -Wextra -Wstrict-prototypes -Wundef -Wshadow -Wredundant-decls`

## Anti-Patterns (this project)

- **No `malloc` in ISR or time-critical paths** — `sbrk.c` is bare-metal heap, not ISR-safe
- **No blocking in `main()` loop** — all processing is poll-based, 1ms tick granularity
- **No direct register access in shared code** — use `hw_*.h` API; platform code in `{target}/fw/` only
- **No `#ifdef STM32F1` in new code** — use hardware abstraction (`hw_*.h`) instead; existing `#ifdef` in `sbrk.c` is legacy
- **No hardcoded CAN IDs in `car.c`** — IDs belong in `cars/*.c` handler tables
- **No magic numbers in config** — use `conf.h` enums, not raw integers for car/canbox selection
- **No USART direct write outside `main.c` debug** — use `canbox_*_process()` functions for protocol output
- **No `sleep()` / blocking delays** — use `timer.flag_*` state machine instead
- **Never skip `conf_write()` after config change** — config persists only when explicitly saved
- **Platform-specific `.c` files must NOT pull in headers from other platforms**

## Commands

```bash
# Build all firmware targets
make                                    # → vw_nc03.bin + volvo_od2.bin + qemu.bin

# Clean build artifacts
make clean                              # removes *.bin *.elf *.map *.o *.vo *.vwo *.qemu *.d

# Flash (requires patched OpenOCD)
make flash_vw_nc03                      # NUC131 via /opt/openocd-nuvoton/
make flash_volvo_od2                    # STM32F103 with RDP unlock
make test_nuc                           # Read NUC131 registers via OpenOCD

# Emulation
make run_qemu                           # QEMU stm32vldiscovery (requires patched QEMU)

# Qt desktop emulator
cd qt && qmake qcanbox.pro && make     # → qt/release/qcanbox(.exe)
```

**Toolchain**: `arm-none-eabi-gcc` (apt: `gcc-arm-none-eabi`)
**Patched deps**: `openocd-fix.diff` (NUC131 flash), `qemu-7.2.0-fix.diff` (STM32 USART emulation)

## Non-Standard Patterns

- Object file extensions: `.vwo` (NUC131), `.vo` (STM32F1), `.qemu` (QEMU) — prevents name collisions
- `libopencm3` is committed directly (not submodule), compiled via explicit rules in `Makefile_stm32f1`
- Same `src/` `.c` files compiled 3× for different MCUs; platform diff via per-target `hw_*.c` inclusion
- `hw_*.h` headers in `include/`, but implementations in `{target}/fw/` (split HAL pattern)
- `hw_gpio.h` is NOT in `include/` (unlike other `hw_*.h`) — lives only in each target's `fw/`
- `cars/*.c` are NOT compiled separately — they're `#include`-d into `src/car.c`
- NUC131 startup object (`startup_NUC131.o`) is prebuilt, not compiled from source
- `flash_volvo_od2` does chip RDP unlock before programming (3-stage: unlock → erase → program)
- Firmware binaries (`*.bin`) and `qt/win32/qcanbox.exe` are committed to git
- `.gitignore`, `.gitattributes`, `.editorconfig` added — build artifacts excluded, LF enforced, tab style configured
- No CI, no tests

## Notes

- QEMU target uses `stm32vldiscovery` machine (STM32F100RB, 128K flash)
- Volvo OD2 flash unlock requires OpenOCD raw register writes — chip may be RDP-locked
- `STATE_UNDEF` (0xff) is the "no data yet" sentinel in `carstate` — timeout handlers reset fields to this
- Timer domains in main loop are NOT preemptive — they're flag-based, processed in sequence
- `conf_write()` uses wear-leveling: writes to next slot, erases page only when wrapping
- Car files in `cars/` are included (not linked) — must NOT have duplicate static symbols
- Qt emulator defines `QCAR` macro to add `e_car_qcar` to `e_car_t` enum
- HW schematics in `volvo_od2/hw/` and `vw_nc03/hw/` are KiCad, not Eagle/Altium
