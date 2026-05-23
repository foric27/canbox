# qt/ — Desktop Emulator (qcanbox)

## OVERVIEW
Qt/C++ desktop application that emulates the CAN box firmware. Links same `canbox.c` + `car.c` sources, stubs hardware functions, provides GUI for manual testing.

## STRUCTURE
```
qt/
├── qcanbox.pro          # qmake project file
├── main.cpp             # Qt entry + hw stubs (extern "C")
├── main.h               # QMainWindow subclass
├── qcar.c / qcar.h      # Virtual car state (manual button control)
├── wdg_com.cpp / .h     # Serial port widget (send/receive USART packets)
├── wdg_dbg.cpp / .h     # Debug log widget
├── log_levels.h         # Qt debug log rules
├── ui/                  # .ui designer forms
│   ├── main.ui          # Main window layout
│   ├── dbg.ui           # Debug panel
│   └── com.ui           # Serial port panel
├── images/              # UI assets
├── sg.ico               # Windows app icon
├── qcanbox.qrc          # Qt resource file
└── win32/               # Prebuilt qcanbox.exe
```

## WHERE TO LOOK
| Task | Location | Notes |
|------|----------|-------|
| Change UI layout | `ui/*.ui` | Open in Qt Designer |
| Add serial protocol command | `main.cpp:slt_msg()` → `canbox_rx_process()` | USART RX → canbox protocol |
| Change emulated car state | `qcar.c:qcar_state[]` + GUI buttons in `main.cpp` | Manual toggle via UI buttons |
| Stub a new hw function | `main.cpp` `extern "C"` block | All hw stubs return dummy values |
| Build on Windows | `cd qt && qmake qcanbox.pro && make` | Statically linked via qmake config |
| Prebuilt binary | `win32/qcanbox.exe` | Committed to git |

## CONVENTIONS
- **Defines `QCAR`** macro — adds `e_car_qcar` to `e_car_t` enum in `conf.h`
- All `hw_*` functions are **stubbed** in `main.cpp` `extern "C"` block:
  - `hw_usart_get()` → returns `NULL`
  - `hw_can_get_mscan()` → returns `NULL`
  - `hw_usart_write()` → delegates to `main_t::usart_write()` → emits Qt signal
  - Module boundary: Qt `.pro` file links against `canbox.c` and `car.c` from root
- Build: Qt 5/6, modules: `core gui widgets serialport`
- Windows: static link via `-static -static-libgcc`, stripped via `post_link`
- Car state: controlled by GUI buttons, read via `qcar_state[]` array
- Timer: Qt `QTimer` drives `car_process(1)` + `canbox_process()` every 500ms

## ANTI-PATTERNS
- **DO NOT** add code that requires actual hardware — Qt emulator is for protocol testing only
- **DO NOT** link against Qt in firmware builds — Qt code is separate application
