message("========== project: $$PWD")
include( ../subproject_integration.pri )

QT -= gui

CONFIG += c++17
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = shvcoreqt

unix:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

message ( DESTDIR: $$DESTDIR )

unix {
	LIBS += \
		-Wl,-rpath,\'\$\$ORIGIN\'
}

android {
DEFINES += SHV_ANDROID_BUILD
}

DEFINES += SHVCOREQT_BUILD_DLL

INCLUDEPATH += \
    $$SHV_PROJECT_TOP_SRCDIR/3rdparty/necrolog/include \
	$$PWD/../libshvcore/include \
	$$PWD/../libshvchainpack/include \

LIBS += \
	-L$$DESTDIR

android: LIBEXT = "_$${QT_ARCH}"
else: LIBEXT = ""

LIBS += -lnecrolog$${LIBEXT} \
		-lshvcore$${LIBEXT} \
		-lshvchainpack$${LIBEXT}

include($$PWD/src/src.pri)
