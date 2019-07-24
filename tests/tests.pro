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


INCLUDEPATH += $$PWD/../3rdlib/qtmypaint/src
DEPENDPATH += $$PWD/../3rdlib/qtmypaint/src

# --- libmypaint ---
win32-g++: LIBS += $$PWD/../3rdlib/qtmypaint/libmypaint/libmypaint.dll.a
else:!macx:unix: LIBS += $$PWD/../3rdlib/qtmypaint/libmypaint/libmypaint-1.3.so.0.0.0 \
                         $$PWD/../3rdlib/qtmypaint/libmypaint/libmypaint-1.3.so.0 \
                         $$PWD/../3rdlib/qtmypaint/libmypaint/libmypaint.so
else:macx: LIBS += $$PWD/../3rdlib/qtmypaint/libmypaint/libmypaint.dylib

# Install: move libraries to their respective folders
macx {
    libraries.path = $$OUT_PWD
    libraries.files = $$OUT_PWD/../3rdlib/qtmypaint/src/*.dylib

    system($$QMAKE_COPY $$OUT_PWD/../3rdlib/qtmypaint/src/libQTMyPaint.1.0.0.dylib $${OUT_PWD}/)
    system($$QMAKE_SYMBOLIC_LINK $$OUT_PWD/../3rdlib/qtmypaint/src/libQTMyPaint.1.0.dylib  $${OUT_PWD}/)
    system($$QMAKE_SYMBOLIC_LINK $$OUT_PWD/../3rdlib/qtmypaint/src/libQTMyPaint.1.dylib  $${OUT_PWD}/)
    system($$QMAKE_SYMBOLIC_LINK $$OUT_PWD/../3rdlib/qtmypaint/src/libQTMyPaint.dylib $${OUT_PWD}/)

    system($$QMAKE_COPY $$PWD/../3rdlib/qtmypaint/libmypaint/libmypaint-1.3.0.dylib $${OUT_PWD}/)

#    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
#    QMAKE_RPATHDIR += @executable_path/

#    QMAKE_POST_LINK = install_name_tool -add_rpath @executable_path/ libmypaint-1.3.0.dylib
    QMAKE_POST_LINK = install_name_tool -change /usr/local/lib/libmypaint-1.3.0.dylib $$OUT_PWD/libmypaint-1.3.0.dylib $$OUT_PWD/tests

    exists($$OUT_PWD/*.dylib*) {
        message("found dylib files")
        listme = $$system(ls $$OUT_PWD/*.dylib*)
        message($$listme)
    } else {
        message("found no so files")
    }
}else:unix {

    libraries.path = $$OUT_PWD
    libraries.files = $$OUT_PWD/../3rdlib/qtmypaint/src/*.so*

    system($$QMAKE_COPY $$OUT_PWD/../3rdlib/qtmypaint/src/libQTMyPaint.so.1.0.0 $${OUT_PWD}/)
    system($$QMAKE_SYMBOLIC_LINK $$OUT_PWD/../3rdlib/qtmypaint/src/libQTMyPaint.so.1.0 $${OUT_PWD}/)
    system($$QMAKE_SYMBOLIC_LINK $$OUT_PWD/../3rdlib/qtmypaint/src/libQTMyPaint.so.1 $${OUT_PWD}/)
    system($$QMAKE_SYMBOLIC_LINK $$OUT_PWD/../3rdlib/qtmypaint/src/libQTMyPaint.so $${OUT_PWD}/)

    system($$QMAKE_COPY $$PWD/../3rdlib/qtmypaint/libmypaint/libmypaint-1.3.so.0.0.0 $${OUT_PWD}/)
    system($$QMAKE_SYMBOLIC_LINK $$PWD/../3rdlib/qtmypaint/libmypaint/libmypaint-1.3.so.0 $${OUT_PWD}/)
    system($$QMAKE_SYMBOLIC_LINK $$PWD/../3rdlib/qtmypaint/libmypaint/libmypaint.so $${OUT_PWD}/)

    # suppress the default RPATH if you wish
    QMAKE_LFLAGS_RPATH=
    # add your own with quoting gyrations to make sure $ORIGIN gets to the command line unexpanded
    QMAKE_LFLAGS += "-Wl,-rpath,\'\$$OUT_PWD\'"

    exists($$OUT_PWD/*.so*) {
        message("found so files")
        listme = $$system(ls $$OUT_PWD/*.so*)
        message($$listme)
    } else {
        message("found no so files")
    }
}else:win32:CONFIG(release, debug|release) {
    libraries.path = $$OUT_PWD/release/
    libraries.files = $$OUT_PWD/../3rdlib/qtmypaint/src/*.dll
}else:win32:CONFIG(debug, release|debug) {
    libraries.path = $$OUT_PWD/debug/
    libraries.files = $$OUT_PWD/../3rdlib/qtmypaint/src/*.dll
}

# --- qtmypaint ---
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../3rdlib/qtmypaint/src/release/ -lQTMyPaint
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../3rdlib/qtmypaint/src/debug/ -lQTMyPaint
else:!macx:unix: LIBS += $$OUT_PWD/libQTMyPaint.so.1.0.0 \
                         $$OUT_PWD/libQTMyPaint.so \
                         $$OUT_PWD/libQTMyPaint.so.1 \
                         $$OUT_PWD/libQTMyPaint.so.1.0

else:macx: LIBS += $$OUT_PWD/libQTMyPaint.dylib \
                   $$OUT_PWD/libQTMyPaint.1.dylib \
                   $$OUT_PWD/libQTMyPaint.1.0.dylib \
                   $$OUT_PWD/libQTMyPaint.1.0.0.dylib

#INSTALLS += libraries

INCLUDEPATH += $$PWD/../3rdlib/qtmypaint
DEPENDPATH += $$PWD/../3rdlib/qtmypaint

macx: LIBS += -framework AppKit
