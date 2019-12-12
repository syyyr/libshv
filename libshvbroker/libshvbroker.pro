message("including $$PWD")

QT += network
QT -= gui

with-shvwebsockets {
    QT += websockets
    DEFINES += WITH_SHV_WEBSOCKETS
}

CONFIG += C++11
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = shvbroker

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
	SHV_PROJECT_TOP_BUILDDIR=$$shadowed($$PWD)/..
}
message ( SHV_PROJECT_TOP_BUILDDIR: '$$SHV_PROJECT_TOP_BUILDDIR' )

unix:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

message ( DESTDIR: $$DESTDIR )

android {
DEFINES += ANDROID_BUILD
}

DEFINES += SHVBROKER_BUILD_DLL

INCLUDEPATH += \
	$$PWD/../libshvcore/include \
	$$PWD/../libshvcoreqt/include \
	$$PWD/../libshviotqt/include \
	$$PWD/../libshvchainpack/include \
	$$PWD/../3rdparty/necrolog/include \

LIBS += \
    -L$$DESTDIR \
    -lnecrolog \
    -lshvchainpack \
    -lshvcore \
    -lshvcoreqt \
    -lshviotqt \

include($$PWD/src/src.pri)
