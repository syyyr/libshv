message("========== project: $$PWD")
include( ../subproject_integration.pri )

QT -= core gui

CONFIG += C++11
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = shvcore

unix:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

message ( SHV_PROJECT_TOP_SRCDIR: $$SHV_PROJECT_TOP_SRCDIR )
message ( DESTDIR: $$DESTDIR )

unix {
	LIBS += \
		-Wl,-rpath,\'\$\$ORIGIN\'
}

android {
DEFINES += SHV_ANDROID_BUILD
}

DEFINES += SHVCORE_BUILD_DLL

INCLUDEPATH += \
    $$SHV_PROJECT_TOP_SRCDIR/3rdparty/necrolog/include \
	../libshvchainpack/include \

LIBS += \
	-L$$DESTDIR

android: LIBEXT = "_$${QT_ARCH}"
else: LIBEXT = ""

LIBS += \
	-lnecrolog$${LIBEXT} \
	-lshvchainpack$${LIBEXT}

include($$PWD/src/src.pri)
