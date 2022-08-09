message("========== project: $$PWD")
include( ../subproject_integration.pri )

QT += network serialport
QT -= gui

with-shvwebsockets {
	message("$$PWD" with websockets)
	QT += websockets
	DEFINES += WITH_SHV_WEBSOCKETS
}

CONFIG += c++17
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = shviotqt

unix:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

message ( DESTDIR: $$DESTDIR )

DEFINES += SHVIOTQT_BUILD_DLL

unix {
	LIBS += \
		-Wl,-rpath,\'\$\$ORIGIN\'
}

android {
	DEFINES += SHV_ANDROID_BUILD
}
wasm {
	DEFINES += SHV_NO_SSL_SOCKET
	#DEFINES += SHV_WASM_BUILD
}

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
