QT += core gui multimedia
greaterThan(QT_MAJOR_VERSION, 4) : QT += widgets

QMAKE_CFLAGS_RELEASE -= -O
QMAKE_CFLAGS_RELEASE -= -O1
QMAKE_CFLAGS_RELEASE *= -O2
QMAKE_CFLAGS_RELEASE -= -O3

CONFIG += c++11

TARGET = CorgiDS
CONFIG += console
#CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    ../src/cartridge.cpp \
    ../src/cp15.cpp \
    ../src/cpu.cpp \
    ../src/cpuinstrs.cpp \
    ../src/dma.cpp \
    ../src/firmware.cpp \
    ../src/instrthumb.cpp \
    ../src/qt/main.cpp \
    ../src/rtc.cpp \
    ../src/spi.cpp \
    ../src/timers.cpp \
    ../src/emulator.cpp \
    ../src/qt/emuwindow.cpp \
    ../src/qt/configwindow.cpp \
    ../src/config.cpp \
    ../src/gpu.cpp \
    ../src/arm9rw.cpp \
    ../src/arm7rw.cpp \
    ../src/ipc.cpp \
    ../src/spu.cpp \
    ../src/wifi.cpp \
    ../src/touchscreen.cpp \
    ../src/qt/debugwindow.cpp \
    ../src/disasm_arm.cpp \
    ../src/gpueng.cpp \
    ../src/gpu3d.cpp \
    ../src/armtable.cpp \
    ../src/qt/emuthread.cpp \
    ../src/bios.cpp \
    ../src/qt/audiodevice.cpp \
    ../src/disasm_thumb.cpp \
    ../src/debugger.cpp \
    ../src/slot2.cpp \
    ../src/gba/gbarw.cpp \
    ../src/gba/gbadma.cpp \
    ../src/gba/gbagpu.cpp

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

HEADERS += \
    ../src/cartridge.hpp \
    ../src/cp15.hpp \
    ../src/cpu.hpp \
    ../src/cpuinstrs.hpp \
    ../src/dma.hpp \
    ../src/emulator.hpp \
    ../src/firmware.hpp \
    ../src/instrtable.h \
    ../src/interrupts.hpp \
    ../src/memconsts.h \
    ../src/rtc.hpp \
    ../src/spi.hpp \
    ../src/timers.hpp \
    ../src/qt/emuwindow.hpp \
    ../src/qt/configwindow.hpp \
    ../src/config.hpp \
    ../src/gpu.hpp \
    ../src/ipc.hpp \
    ../src/spu.hpp \
    ../src/wifi.hpp \
    ../src/touchscreen.hpp \
    ../src/qt/debugwindow.hpp \
    ../src/disassembler.hpp \
    ../src/gpueng.hpp \
    ../src/gpu3d.hpp \
    ../src/qt/emuthread.hpp \
    ../src/bios.hpp \
    ../src/qt/audiodevice.hpp \
    ../src/debugger.hpp \
    ../src/slot2.hpp \
    ../src/gba/gbadma.hpp

FORMS += \
    ../src/qt/configwindow.ui \
    ../src/qt/debugwindow.ui
