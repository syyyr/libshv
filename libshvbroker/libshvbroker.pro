message("========== project: $$PWD")
include( ../subproject_integration.pri )

QT += network sql
QT -= gui

with-shvwebsockets {
    QT += websockets
    DEFINES += WITH_SHV_WEBSOCKETS
}

CONFIG += C++11
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = shvbroker


unix:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

unix {
	LIBS += \
		-Wl,-rpath,\'\$\$ORIGIN\'
}

message ( DESTDIR: $$DESTDIR )

android {
DEFINES += ANDROID_BUILD
}

DEFINES += SHVBROKER_BUILD_DLL

INCLUDEPATH += \
	$$SHV_PROJECT_TOP_SRCDIR/3rdparty/necrolog/include \
	$$PWD/../libshvcore/include \
	$$PWD/../libshvcoreqt/include \
	$$PWD/../libshviotqt/include \
	$$PWD/../libshvchainpack/include \

LIBS += \
    -L$$DESTDIR \
    -lnecrolog \
    -lshvchainpack \
    -lshvcore \
    -lshvcoreqt \
    -lshviotqt \

include($$PWD/src/src.pri)
