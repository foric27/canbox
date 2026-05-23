# cars/ — Car-Specific CAN Message Handlers

## OVERVIEW
Per-car `.c` files defining CAN message filter tables and parsing callbacks. Included into `car.c` via `#include`, NOT compiled separately.

## STRUCTURE
```
cars/
├── anymsg.c            # Generic — forwards ALL CAN IDs
├── lr2_2007my.c        # Land Rover Freelander 2 (2007 model year)
├── lr2_2013my.c        # Land Rover Freelander 2 (2013 model year)
├── xc90_2007my.c       # Volvo XC90 (2007 model year)
├── skoda_fabia.c       # Skoda Fabia (2006 model year)
├── q3_2015.c           # Audi Q3 (2015 model year)
└── toyota_premio_26x.c # Toyota Premio (260 series)
```

## WHERE TO LOOK
| Task | Location | Notes |
|------|----------|-------|
| Add new car | Create `cars/newcar.c` + add `#define USE_*` in `car.c` | Follow existing car file pattern |
| Add CAN message to existing car | Edit handler table in `cars/lr2_2007my.c` etc. | `msg_desc_t` array: id, timeout_ms, period?, 0, handler |
| Understand CAN IDs for a car | Read comment blocks above handler tables | Each car file self-documents its CAN messages |
| Debug car data | Use interactive debug mode (`10 × 'O'` on USART) | Shows live `carstate` values |

## CONVENTIONS
- Each car file provides a **static** `msg_desc_t` array (e.g., `lr2_2007my_ms[]`)
- Array name convention: `{carname}_ms[]`
- Handler name convention: `{carname}_ms_{id:x}_handler(const uint8_t * msg, struct msg_desc_t * desc)`
- All handlers and arrays are `static` (file-local, no collisions when #included together)
- Handlers write directly to global `carstate` struct (defined in `car.c`)
- Timeout logic: `if (is_timeout(desc)) { carstate.field = STATE_UNDEF; return; }`
- Bit extraction: raw `msg[N]` indexing with hex masks (`msg[2] & 0x80`)
- Scaling: `scale(value, in_min, in_max, out_min, out_max)` — defined in `car.c`
- No `#include` of other car files — each file is self-contained
- All symbols must be `static` — car files are `#include`d in same translation unit

## ANTI-PATTERNS
- **DO NOT** declare non-static symbols — all 7 car files share one translation unit
- **DO NOT** hardcode bit positions without documenting the CAN message format
- **DO NOT** assume specific message periods — use `is_timeout()` for robustness
- **DO NOT** forget `STATE_UNDEF` in timeout handlers — stale data is worse than no data
