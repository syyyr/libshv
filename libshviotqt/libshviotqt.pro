message("including $$PWD")

QT += network
QT -= gui

CONFIG += C++11
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = shviotqt

include( ../subproject_integration.pri )

unix:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

message ( DESTDIR: $$DESTDIR )

android {
DEFINES += ANDROID_BUILD
}

DEFINES += SHVIOTQT_BUILD_DLL

INCLUDEPATH += \
    $$SHV_PROJECT_TOP_SRCDIR/3rdparty/necrolog/include \
	$$PWD/../libshvcore/include \
	$$PWD/../libshvcoreqt/include \
	$$PWD/../libshvchainpack/include \

LIBS += \
	-L$$DESTDIR

android: LIBEXT = "_$${QT_ARCH}"
else: LIBEXT = ""

LIBS += -lnecrolog$${LIBEXT} \
		-lshvchainpack$${LIBEXT} \
		-lshvcore$${LIBEXT} \
		-lshvcoreqt$${LIBEXT}

include($$PWD/src/src.pri)
