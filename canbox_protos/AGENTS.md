# canbox_protos/ — протоколы canbox (Android-протоколы)

Файлы в этой директории — это модули протоколов, которые `#include`-ятся в `src/canbox.c`.
Каждый файл соответствует одному протоколу Android-интерфейса (`enum e_canbox_t`).

## Структура

```
canbox_protos/
  canbox_raise_vw_pq.c       → Raise VW(PQ)   — e_cb_raise_vw_pq
  canbox_raise_vw_mqb.c      → Raise VW(MQB)  — e_cb_raise_vw_mqb
  canbox_od_bmw_nbt_evo.c    → Raise Oudi BMW — e_cb_od_bmw_nbt_evo
  canbox_hiworld_vw_mqb.c    → HiWorld VW(MQB)— e_cb_hiworld_vw_mqb
```

## Конвенции

- **Все функции static** — файл компилируется в контексте `canbox.c`, не должен экспортировать символы
- **Нет `#include`** — все зависимости (заголовки, типы) предоставляются `canbox.c` через порядок `#include`
- **Имена функций**: `<prefix>_process()` для основного цикла (250ms), `<prefix>_park_process()` для парктроника (100ms)
- **Общие Raise-функции** (`canbox_raise_vw_radar_process`, `canbox_raise_vw_wheel_process`, `canbox_raise_vw_mqb_door_process`) живут в `canbox.c` ядре
- **Вызов функций send**: Raise использует `snd_canbox_msg()`, HiWorld — `snd_canbox_hiworld_msg()`

## Выбор на этапе компиляции

Аналогично `cars/*.c`: `#define USE_*` в `canbox.c` управляет `#ifdef` + `#include`.
Все протоколы включены по умолчанию. Для отключения — закомментировать `#define`.
