#-------------------------------------------------
#
# Unit Test Project of Pencil2D
#
#-------------------------------------------------

! include( ../common.pri ) { error( Could not find the common.pri file! ) }

QT += core widgets gui xml xmlpatterns multimedia svg testlib

TEMPLATE = app

TARGET = tests

CONFIG   += console
CONFIG   -= app_bundle

MOC_DIR = .moc
OBJECTS_DIR = .obj

INCLUDEPATH += \
    ../core_lib/src/graphics \
    ../core_lib/src/graphics/bitmap \
    ../core_lib/src/graphics/vector \
    ../core_lib/src/interface \
    ../core_lib/src/structure \
    ../core_lib/src/tool \
    ../core_lib/src/util \
    ../core_lib/ui \
    ../core_lib/src/managers

HEADERS += \
    src/catch.hpp

SOURCES += \
    src/main.cpp \
    src/test_layer.cpp \
    src/test_layermanager.cpp \
    src/test_object.cpp \
    src/test_filemanager.cpp \
    src/test_bitmapimage.cpp \
    src/test_viewmanager.cpp

# --- CoreLib ---
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../core_lib/release/ -lcore_lib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../core_lib/debug/ -lcore_lib
else:unix: LIBS += -L$$OUT_PWD/../core_lib/ -lcore_lib

INCLUDEPATH += $$PWD/../core_lib/src

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../core_lib/release/libcore_lib.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../core_lib/debug/libcore_lib.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../core_lib/release/core_lib.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../core_lib/debug/core_lib.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../core_lib/libcore_lib.a

INCLUDEPATH += $$PWD/../3rdlib/qtmypaint/json-c \
               $$PWD/../3rdlib/qtmypaint/libmypaint \
               $$PWD/../3rdlib/qtmypaint/src

# --- qtmypaint ---
win32:CONFIG(release, debug|release): LIBS += $$OUT_PWD\..\3rdlib\qtmypaint\src\release\libQTMyPaint.dll
else:win32:CONFIG(debug, debug|release): LIBS += $$OUT_PWD\..\3rdlib\qtmypaint\src\debug\libQTMyPaint.dll
else:!macx:unix: LIBS += -L$$OUT_PWD/../3rdlib/qtmypaint/src/ -lQTMyPaint \
                           $$OUT_PWD/../3rdlib/qtmypaint/src/libQTMyPaint.so

macx: LIBS += $$OUT_PWD/../3rdlib/qtmypaint/src/libQTMyPaint.dylib

INCLUDEPATH += ../3rdlib/qtmypaint
DEPENDPATH += ../3rdlib/qtmypaint

# Install: move libraries to their respective folders
macx {
    libraries.path = $$OUT_PWD
    libraries.files = $$OUT_PWD/../3rdlib/qtmypaint/src/*.dylib
}else:unix {
    libraries.path = $$OUT_PWD
    libraries.files = $$OUT_PWD/../3rdlib/qtmypaint/src/*.so
}else:win32:CONFIG(release, debug|release) {
    libraries.path = $$OUT_PWD/release/
    libraries.files = $$OUT_PWD/../3rdlib/qtmypaint/src/*.dll
}else:win32:CONFIG(debug, release|debug) {
    libraries.path = $$OUT_PWD/debug/
    libraries.files = $$OUT_PWD/../3rdlib/qtmypaint/src/*.dll
}

INSTALLS += libraries

INCLUDEPATH += $$PWD/../3rdlib/qtmypaint
DEPENDPATH += $$PWD/../3rdlib/qtmypaint

macx: LIBS += -framework AppKit
