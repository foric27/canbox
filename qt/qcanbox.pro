#
# @brief Файл проекта Qt для сборки эмулятора qCanBox
#
# Определяет модули Qt, исходники, заголовки, формы и ресурсы.
# Линкует прошивочный код (src/canbox.c, src/car.c) с Qt-оберткой.
# Поддерживает статическую сборку под Windows.
#

# Подключаемые модули Qt
QT += core gui widgets serialport

# Имя выходного исполняемого файла
TARGET = qcanbox

# Тип сборки — приложение
TEMPLATE = app

# Заголовки и исходники Qt-части
HEADERS = main.h wdg_dbg.h wdg_com.h
SOURCES = main.cpp wdg_dbg.cpp wdg_com.cpp

# Формы Qt Designer
FORMS += ui/main.ui ui/dbg.ui ui/com.ui

# Пути поиска зависимостей и заголовков прошивки
DEPENDPATH += $$PWD/../ $$PWD/../src
INCLUDEPATH += $$PWD/../include $$PWD/../

# Заголовки прошивки (canbox.h, car.h)
HEADERS += $$PWD/../include/canbox.h $$PWD/../include/car.h

# Исходники прошивки + виртуальный автомобиль
SOURCES += $$PWD/../src/canbox.c $$PWD/../src/car.c qcar.c

# Файл ресурсов (иконки UI)
RESOURCES += qcanbox.qrc

# Макросы препроцессора
DEFINES += QT_STATICPLUGIN
DEFINES += QCAR    # Добавляет e_car_qcar в enum e_car_t

# Настройки специфичные для Windows
win32 {
	# Иконка приложения
	RC_ICONS = sg.ico
	# Статическая линковка runtime
	QMAKE_LFLAGS += -static -static-libgcc
	# Удаление символов из release-бинарника
	QMAKE_POST_LINK=$$QMAKE_STRIP release/$(TARGET)
}
