QT += core gui network widgets

TARGET = snftp
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

CONFIG += c++11

SOURCES += \
        crypto.cpp \
        main.cpp \
        mainwidget.cpp \
        startupdialog.cpp

HEADERS += \
        crypto.h \
        mainwidget.h \
        startupdialog.h

FORMS += \
        mainwidget.ui \
        startupdialog.ui

LIBS += -lsodium
