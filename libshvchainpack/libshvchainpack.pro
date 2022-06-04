message("========== project: $$PWD")
include( ../subproject_integration.pri )

QT -= core gui

CONFIG += C++17
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = shvchainpack

unix:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

message ( DESTDIR: $$DESTDIR )

android {
DEFINES += SHV_ANDROID_BUILD
}

DEFINES += SHVCHAINPACK_BUILD_DLL

INCLUDEPATH += \
    $$SHV_PROJECT_TOP_SRCDIR/3rdparty/necrolog/include

LIBS += \
	-L$$DESTDIR

android: LIBEXT = "_$${QT_ARCH}"
else: LIBEXT = ""

LIBS += \
	-lnecrolog$${LIBEXT}

include($$PWD/src/src.pri)
include($$PWD/c/c.pri)

