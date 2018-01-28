message("including $$PWD")

QT += network
QT -= gui

CONFIG += C++11
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = shvcoreqt

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
	SHV_PROJECT_TOP_BUILDDIR=$$shadowed($$PWD)/..
}
message ( SHV_PROJECT_TOP_BUILDDIR: '$$SHV_PROJECT_TOP_BUILDDIR' )

unix:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

message ( DESTDIR: $$DESTDIR )

DEFINES += SHVCOREQT_BUILD_DLL

INCLUDEPATH += \
	$$PWD/../libshvcore/include \
	$$PWD/../libshvchainpack/include \
	../3rdparty/necrolog/include \

LIBS += \
    -L$$DESTDIR \
    -lnecrolog \
    -lshvcore \
    -lshvchainpack

include($$PWD/src/src.pri)
