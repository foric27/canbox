PREFIX = arm-none-eabi
# Build tools — override for Windows/alternative paths
CC ?= $(PREFIX)-gcc
OBJCOPY ?= $(PREFIX)-objcopy
AR ?= $(PREFIX)-ar

# OpenOCD paths — override for your system
OPENOCD_NUVOTON ?= /opt/openocd-nuvoton/bin/openocd
OPENOCD_STM32  ?= openocd
QEMU_STM32     ?= /opt/qemu-stm32/bin/qemu-system-arm

# Clean command — cross-platform: works in MSYS2/Git Bash
# On pure Windows cmd/pwsh without make, use: .\build.ps1 clean

COMMON_INCLUDES=-Iinclude -I.

VPATH = src
COMMON_CFLAGS=-Os -std=gnu99 -fno-common -ffunction-sections -fdata-sections -Wstrict-prototypes -Wundef -Wextra -Wshadow -Wredundant-decls #-Waddress-of-packed-member
COMMON_LDFLAGS=--static -lc -lm -Wl,--cref -Wl,--gc-sections #-Wl,--print-gc-sections

PHONY:all

all: out/vw_nc03.bin out/volvo_od2.bin out/qemu.bin

include Makefile_stm32f1
include Makefile_volvo_od2
include Makefile_vw_nc03
include Makefile_qemu

clean:
	$(RM) -rf out/

