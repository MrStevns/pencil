! include( ../util/common.pri ) { error( Could not find the common.pri file! ) }

QT += core

TEMPLATE = lib
CONFIG += staticlib

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/pencilerror.cpp

HEADERS += \
    src/pencildef.h \
    src/pencilerror.h
