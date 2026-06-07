# Префикс кросс-компилятора для ARM Cortex-M (gcc-arm-none-eabi)
# Используется для сборки прошивок под STM32F1 (Cortex-M3) и NUC131 (Cortex-M0)
PREFIX = arm-none-eabi

# Основные утилиты toolchain. Переменные с ?= позволяют переопределить
# извне (например, для Windows или альтернативных путей установки)
CC ?= $(PREFIX)-gcc       # Кросс-компилятор C (GNU ARM Embedded GCC)
OBJCOPY ?= $(PREFIX)-objcopy  # Утилита для конвертации ELF → bin/hex/srec
AR ?= $(PREFIX)-ar        # Архиватор для создания статических библиотек

# Пути к OpenOCD и QEMU — переопределяйте под вашу систему
# OPENOCD_NUVOTON: патченный OpenOCD для прошивки NUC131 (VW NC03)
# OPENOCD_STM32: стандартный OpenOCD для STM32F1 (Volvo OD2)
# QEMU_STM32: патченный QEMU для эмуляции stm32vldiscovery
OPENOCD_NUVOTON ?= /opt/openocd-nuvoton/bin/openocd
OPENOCD_STM32  ?= openocd
QEMU_STM32     ?= /opt/qemu-stm32/bin/qemu-system-arm

# Команда очистки. Работает в MSYS2/Git Bash.
# В чистом Windows (cmd/pwsh) без make используйте: .\build.ps1 clean

# Общие пути include для всех целей сборки
# -Iinclude: заголовки общей прошивки (hw.h, car.h, canbox.h и др.)
# -I.: корень проекта (для относительных путей к cars/*.c, canbox_protos/*.c)
COMMON_INCLUDES=-Iinclude -I.

# VPATH: список директорий для поиска исходников при сборке
# make будет искать .c файлы в src/ если они указаны без пути
VPATH = src

# Общие флаги компиляции C для всех трёх целей (Volvo, VW, QEMU)
# -Os: оптимизация по размеру (критично для микроконтроллеров с малым flash)
# -std=gnu99: стандарт C99 с GNU-расширениями
# -fno-common: запрет неинициализированных глобальных переменных в BSS
# -ffunction-sections, -fdata-sections: каждая функция/данные в отдельной секции
#   (нужно для --gc-sections линковщика — удаление неиспользуемого кода)
# -Wstrict-prototypes: предупреждение о неполных прототипах функций
# -Wundef: предупреждение о неопределённых макросах в #if
# -Wextra: дополнительные предупреждения
# -Wshadow: предупреждение о затенении переменных
# -Wredundant-decls: предупреждение о избыточных объявлениях
# #-Waddress-of-packed-member: отключено, т.к. libopencm3 активно использует packed структуры
COMMON_CFLAGS=-Os -std=gnu99 -fno-common -ffunction-sections -fdata-sections -Wstrict-prototypes -Wundef -Wextra -Wshadow -Wredundant-decls #-Waddress-of-packed-member

# Общие флаги линковщика для всех целей
# --static: статическая линковка (нет libc.so на bare-metal)
# -lc, -lm: стандартная C-библиотека и математическая библиотека
# -Wl,--cref: таблица перекрёстных ссылок в map-файле
# -Wl,--gc-sections: удаление неиспользуемых секций (мёртвый код)
# #-Wl,--print-gc-sections: отладка — выводить какие секции удалены
COMMON_LDFLAGS=--static -lc -lm -Wl,--cref -Wl,--gc-sections #-Wl,--print-gc-sections

# Объявление phony-целей (не соответствуют реальным файлам)
.PHONY: all clean

# Главная цель: собрать прошивки для всех трёх платформ
# out/vw_nc03.bin: VW NC03 (Nuvoton NUC131, Cortex-M0)
# out/volvo_od2.bin: OD-Volvo-02 (STM32F103x8, Cortex-M3)
# out/qemu.bin: эмулятор QEMU (stm32vldiscovery)
all: out/vw_nc03.bin out/volvo_od2.bin out/qemu.bin

# Подключение вспомогательных Makefile с флагами и правилами для каждой платформы:
# Makefile_stm32f1 — общие флаги STM32F1 + объекты libopencm3
# Makefile_volvo_od2 — сборка прошивки Volvo OD2
# Makefile_vw_nc03 — сборка прошивки VW NC03 (NUC131)
# Makefile_qemu — сборка эмулятора QEMU
include Makefile_stm32f1
include Makefile_volvo_od2
include Makefile_vw_nc03
include Makefile_qemu

# Цель очистки: удаление директории out/ со всеми объектными файлами,
# ELF, бинарниками и map-файлами
# $(RM) раскрывается в rm -f (или эквивалент на данной платформе)
clean:
	$(RM) -rf out/
