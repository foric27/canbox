# canbox

Прошивка для CAN-адаптеров (canbus box), доступных на AliExpress.

## Назначение

CAN-адаптер — устройство, сопрягающее головное устройство (ГУ) на Android с CAN-шиной автомобиля. 

**Возможности:**
- подача питания на ГУ при включении зажигания (ACC)
- включение камеры заднего хода при селекторе R
- управление подсветкой экрана ГУ в зависимости от уровня освещения
- отображение состояния автомобиля на экране ГУ:
  - открытие дверей, капота, багажника
  - показания парктроников
  - положение селектора АКПП
  - одометр, VIN, температура ОЖ, напряжение бортсети
  - климат-контроль

![canbus box](canbus.png)

## Поддерживаемые платформы

| Адаптер | MCU | Ядро | Протокол |
|---------|-----|------|----------|
| OD-Volvo-02 | STM32F103x8 | Cortex-M3 | Raise VW(PQ/MQB), Oudi BMW(NBT), HiWorld VW(MQB) |
| VW-NC03 | Nuvoton NUC131 | Cortex-M0 | CAN-кадры через аппаратный CAN-контроллер |

## Поддерживаемые автомобили

| Модель | CAN-шина |
|--------|----------|
| AnyMsg (универсальный режим) | — |
| Land Rover Freelander 2 (2007 MY) | HS-CAN 500 кбит/с |
| Land Rover Freelander 2 (2013 MY) | HS-CAN 500 кбит/с |
| Volvo XC90 (2007 MY) | HS-CAN 500 кбит/с |
| Skoda Fabia (2006 MY) | 100 кбит/с |
| Audi Q3 (2015 MY, MQB) | HS-CAN 500 кбит/с |
| Toyota Premio (260-series) | — |

## Конфигурирование

Все настройки выполняются **до компиляции** в файле `include/config.h`:

```c
#define CONFIG_CAR_SKODA_FABIA   // выбрать один из 7 автомобилей
#define USE_RAISE_VW_PQ          // выбрать один из 4 протоколов ГУ
#define CONFIG_ILLUM 50          // порог включения подсветки, %
#define CONFIG_REAR_DELAY 1500   // задержка отключения камеры заднего хода, мс
```

После изменения `config.h` пересоберите прошивку командой `make` или `.\build.ps1`.

> Ранее конфигурация хранилась во flash и менялась через терминал. Теперь всё разрешается на этапе компиляции — проще, меньше кода, нет wear-leveling.

## Сборка из исходников

### Linux

```bash
# ARM toolchain
sudo apt install gcc-arm-none-eabi

# Сборка
make                  # все три прошивки: vw_nc03.bin, volvo_od2.bin, qemu.bin
make clean            # удаление артефактов
make run_qemu         # сборка и запуск в QEMU
make flash_volvo_od2  # прошивка OD-Volvo-02
make flash_vw_nc03    # прошивка VW-NC03
```

### Windows (PowerShell)

```powershell
# ARM toolchain — xPack GNU Arm Embedded GCC
# Скачать: https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases
# Добавить в PATH

# Сборка через build.ps1
.\build.ps1              # все три прошивки
.\build.ps1 clean        # удаление артефактов
.\build.ps1 volvo_od2    # сборка только OD-Volvo-02
.\build.ps1 vw_nc03      # сборка только VW-NC03
.\build.ps1 qemu         # сборка только QEMU
.\build.ps1 run_qemu     # сборка и запуск QEMU
```

> **Примечание:** `make` на Windows доступен через Git Bash, MSYS2 или chocolatey (`choco install make`). Если `make` установлен — Makefile тоже работает после настройки путей к OpenOCD/QEMU:
> ```bash
> make OPENOCD_NUVOTON=/c/openocd/bin/openocd OPENOCD_STM32=/c/openocd/bin/openocd
> ```

### Qt-эмулятор

```bash
cd qt && qmake qcanbox.pro && make   # сборка qcanbox
```

## Структура проекта

```
canbox/
├── src/                  # ядро прошивки (7 .c-файлов)
│   ├── main.c            # точка входа, главный цикл (1/5/100/250/1000 мс домены)
│   ├── canbox.c          # диспетчер протоколов ГУ (включает один из canbox_protos/*.c)
│   ├── car.c             # модель данных авто, диспетчер CAN-сообщений
│   ├── hw.c              # инициализация/сон железа
│   ├── ring.c            # кольцевой буфер (USART RX/TX)
│   ├── tick.c            # системный таймер, флаги временных доменов
│   └── sbrk.c            # менеджер кучи (newlib _sbrk_r)
├── include/              # заголовочные файлы API (10 .h-файлов)
│   ├── hw.h, hw_can.h, hw_usart.h, hw_tick.h, hw_clock.h, hw_conf.h
│   ├── canbox.h, car.h, config.h, ring.h
```

## Архитектура

Ядро прошивки в `src/` компилируется **трижды** для трёх аппаратных платформ с разными `hw_*.c`-прослойками:

- **STM32F103** (OD-Volvo-02) — объектные файлы: `.vo`, HAL: libopencm3
- **NUC131** (VW-NC03) — объектные файлы: `.vwo`, HAL: Nuvoton BSP
- **QEMU** (stm32vldiscovery) — объектные файлы: `.qemu`, заглушки в `qemu/fw/`

Дифференциация платформ — через **подстановку файлов** при линковке, не через `#ifdef` (за исключением `_ebss`/`__bss_end__` в `sbrk.c`).

Главный цикл (`src/main.c`) — невытесняющий, флаговый, с доменами:
- **1 мс** — обработка задержки камеры
- **5 мс** — диспетчер CAN-сообщений (`car_process`)
- **100 мс** — отправка данных парктроников
- **250 мс** — отправка состояния авто в ГУ (`canbox_process`) / вывод отладки
- **1000 мс** — детектор неактивности CAN-шины → сон

## Нестандартные технические решения

- Кастомные расширения объектных файлов (`.vo`, `.vwo`, `.qemu`, `.stm32f1`) — предотвращают коллизии имён при компиляции одних и тех же исходников под разные MCU
- `libopencm3` — вендоренная копия, не submodule (для воспроизводимости сборки)
- `cars/*.c` — не компилируются отдельно, а `#include`-ятся в `car.c`
- `canbox_protos/*.c` — не компилируются отдельно, а `#include`-ятся в `canbox.c`; каждый файл полностью самодостаточен
- `flash_volvo_od2` — трёхстадийная разблокировка RDP чипа перед программированием
- `startup_NUC131.o` — собирается из `startup_NUC131.S` (`.../NUC131/Source/GCC/`) по неявному правилу GNU Make
- Бинарные файлы прошивок (`*.bin`) закоммичены в репозиторий для пользователей без toolchain

## Эмуляция и отладка

### QEMU
```bash
make run_qemu
```
Запускает прошивку в QEMU (машина `stm32vldiscovery`). Вывод USART направляется в stdio — можно взаимодействовать с прошивкой через терминал.

![debug info](qemu.png)

### Qt-эмулятор (qcanbox)
Десктопное приложение, линкующее те же исходники `canbox.c` + `car.c`. Позволяет вручную эмулировать состояние автомобиля (двери, селектор, ремень) и наблюдать CAN-пакеты через виртуальный COM-порт.

## Поддерживаемое железо

![od-volvo-02 pcb](volvo_od2/hw/pcb.jpg)
![od-volvo-02 circuit](volvo_od2/hw/sch.jpg)
![vw_nc03 pcb](vw_nc03/hw/pcb.jpg)
![vw_nc03 circuit](vw_nc03/hw/sch.jpg)

---

[Подробнее на Drive2](https://www.drive2.ru/b/599820152787190466/)
