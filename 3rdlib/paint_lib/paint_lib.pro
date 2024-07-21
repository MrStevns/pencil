! include( ../../util/common.pri ) { error( Could not find the common.pri file! ) }

QT += core gui

TARGET = paint_lib
TEMPLATE = lib
CONFIG += staticlib

# This seems to be neccesary now since all warnings are interepreted as errors...
CONFIG +=  warn_off
DEFINES += HAVE_JSON_C

QMAKE_CFLAGS += -std=c99
QMAKE_CFLAGS += -D_XOPEN_SOURCE=600
QMAKE_CFLAGS += -Ofast

PRECOMPILED_HEADER = src/paintlib-pch.h

# paint_lib

SOURCES += \
            src/mpconfigfilehandler.cpp \
            src/tile.cpp \
            src/tiledbuffer.cpp \
            src/blitrect.cpp \
            src/mpbrushsettingcategories.cpp

HEADERS += \
            src/mpconfigfilehandler.h \
            src/paintlib-pch.h \
            src/tile.h \
            src/tiledbuffer.h \
            src/tileindex.h \
            src/blitrect.h \
            src/mpbrushutils.h \
            src/brushsetting.h \
            src/mpbrushsettingcategories.h

INCLUDEPATH += src
DEPENDPATH += src

# common_lib

INCLUDEPATH += ../../common_lib/src

# libmypaint
SOURCES += \
            src/libmypaint/mypaint-brush.c \
            src/libmypaint/brushmodes.c \
            src/libmypaint/fifo.c \
            src/libmypaint/helpers.c \
            src/libmypaint/libmypaint.c \
            src/libmypaint/mypaint-brush-settings.c \
            src/libmypaint/mypaint-brush.c \
            src/libmypaint/mypaint-fixed-tiled-surface.c \
            src/libmypaint/mypaint-mapping.c \
            src/libmypaint/mypaint-rectangle.c \
            src/libmypaint/mypaint-surface.c \
            src/libmypaint/mypaint-tiled-surface.c \
            src/libmypaint/mypaint.c \
            src/libmypaint/operationqueue.c \
            src/libmypaint/rng-double.c \
            src/libmypaint/tilemap.c \
            src/libmypaint/utils.c

HEADERS += \
            src/libmypaint/brushmodes.h \
            src/libmypaint/brushsettings-gen.h \
#            src/libmypaint/config.h \
            src/libmypaint/fifo.h \
            src/libmypaint/helpers.h \
            src/libmypaint/mypaint-brush-settings-gen.h \
            src/libmypaint/mypaint-brush-settings.h \
            src/libmypaint/mypaint-brush.h \
            src/libmypaint/mypaint-brush.h \
            src/libmypaint/mypaint-config.h \
            src/libmypaint/mypaint-fixed-tiled-surface.h \
            src/libmypaint/mypaint-glib-compat.h \
            src/libmypaint/mypaint-mapping.h \
            src/libmypaint/mypaint-rectangle.h \
            src/libmypaint/mypaint-surface.h \
            src/libmypaint/mypaint-tiled-surface.h \
            src/libmypaint/mypaint.h \
            src/libmypaint/operationqueue.h \
            src/libmypaint/rng-double.h \
            src/libmypaint/tiled-surface-private.h \
            src/libmypaint/tilemap.h \
            src/libmypaint/utils.h

INCLUDEPATH += src/libmypaint
DEPENDPATH += src/libmypaint

# json-c

jsonlibpath = src/json-c
# HACK: Copy config.h that fits the respective platform, created by configure in another build
exists($$jsonlibpath/config.h) {
    message("config.h moved to correct folder")
} else {
    win32 {
        system(echo "testing win32 ")
        system(echo \"$$jsonlibpath\config\win32\config.h\")
        system(echo $$QMAKE_COPY \"$$jsonlibpath\config\win32\config.h\" \"$$jsonlibpath\config.h\")
        system($$QMAKE_COPY \"$$jsonlibpath\config\win32\config.h\" \"$$jsonlibpath\config.h\")
        QMAKE_CLEAN += -r $$jsonlibpath\config.h
    }
    macx|unix {
        system(echo "testing macx and unix ")
        system($$QMAKE_COPY \"$$jsonlibpath/config/macxunix/config.h\" \"$$jsonlibpath\" $$escape_expand(\\n))
        QMAKE_CLEAN += -r $$jsonlibpath/config.h
    }
}

INCLUDEPATH += src/json-c
DEPENDPATH += src/json-c

HEADERS +=  \
            src/json-c/arraylist.h \
            src/json-c/debug.h \
            src/json-c/json_c_version.h \
            src/json-c/json_inttypes.h \
            src/json-c/json_object_iterator.h \
            src/json-c/json_object_private.h \
            src/json-c/json_object.h \
            src/json-c/json_patch.h \
            src/json-c/json_pointer_private.h \
            src/json-c/json_pointer.h \
            src/json-c/json_tokener.h \
            src/json-c/json_types.h \
            src/json-c/json_util.h \
            src/json-c/json_visit.h \
            src/json-c/linkhash.h \
            src/json-c/math_compat.h \
            src/json-c/printbuf.h \
            src/json-c/random_seed.h \
            src/json-c/snprintf_compat.h \
            src/json-c/strdup_compat.h \
            src/json-c/strerror_override.h \
            src/json-c/vasprintf_compat.h \

SOURCES +=  \
            src/json-c/arraylist.c \
            src/json-c/debug.c \
            src/json-c/json_c_version.c \
            src/json-c/json_object_iterator.c \
            src/json-c/json_object.c \
            src/json-c/json_patch.c \
            src/json-c/json_pointer.c \
            src/json-c/json_tokener.c \
            src/json-c/json_util.c \
            src/json-c/json_visit.c \
            src/json-c/libjson.c \
            src/json-c/linkhash.c \
            src/json-c/printbuf.c \
            src/json-c/random_seed.c \
            src/json-c/strerror_override.c \


# qtmypaint
HEADERS += src/qtmypaint/mpbrush.h \
           src/qtmypaint/mphandler.h \
           src/qtmypaint/mpsurface.h \
           src/qtmypaint/mptile.h

SOURCES += src/qtmypaint/mpbrush.cpp \
           src/qtmypaint/mphandler.cpp \
           src/qtmypaint/mpsurface.cpp \
           src/qtmypaint/mptile.cpp

INCLUDEPATH += src/qtmypaint
DEPENDPATH += src/qtmypaint
