TARGET = json-c
TEMPLATE = lib
CONFIG += staticlib

SUBDIRS = config/win32 \
          config/macxunix

HEADERS += arraylist.h \
           bits.h \
           debug.h \
           json.h \
           json_c_version.h \
           json_inttypes.h \
           json_object.h \
           json_object_iterator.h \
           json_object_private.h \
           json_tokener.h \
           json_util.h \
           linkhash.h \
           math_compat.h \
           printbuf.h \
           random_seed.h \
           json_config.h \
           config.h

# HACK: Copy config.h that fits the respective platform, created by configure in another build
macx:unix: system($$QMAKE_COPY $${PWD}/config/macxunix/config.h $${PWD}/ $$escape_expand(\\n))
win32: system($$QMAKE_COPY $${PWD}\\config\\win32\\config.h $${PWD}\\ $$escape_expand(\\n))
macx:unix: QMAKE_CLEAN += -r $${PWD}/config.h
win32: QMAKE_CLEAN += -r $${PWD}\\config.h

SOURCES += arraylist.c \
           debug.c \
           json_c_version.c \
           json_object.c \
           json_object_iterator.c \
           json_tokener.c \
           json_util.c \
           libjson.c \
           linkhash.c \
           printbuf.c \
           random_seed.c

# for C files, we need to allow C99 mode.
QMAKE_CFLAGS += -std=c99
QMAKE_CFLAGS += -D_XOPEN_SOURCE=600
