TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    jsmn.cpp \
    catalog.cpp \
    transfer.cpp \
    type.cpp

HEADERS += \
    jsmn.h \
    catalog.h \
    transfer.h \
    type.h

