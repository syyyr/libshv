message("including $$PWD")

QT += gui widgets

CONFIG += C++11
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = shvvisu

include( ../subproject_integration.pri )

unix:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

message ( DESTDIR: $$DESTDIR )

android {
DEFINES += SHV_ANDROID_BUILD
}

DEFINES += SHVVISU_BUILD_DLL

INCLUDEPATH += \
	$$SHV_PROJECT_TOP_SRCDIR/3rdparty/necrolog/include \
	$$PWD/../libshvcore/include \
    $$PWD/../libshvcoreqt/include \
	$$PWD/../libshviotqt/include \
    $$PWD/../libshvchainpack/include \

LIBS += \
    -L$$DESTDIR \
    -lnecrolog \
    -lshvcore \
    -lshvcoreqt \
	-lshviotqt \
    -lshvchainpack

RESOURCES += \
    images/images.qrc \

include($$PWD/src/src.pri)

