message("========== project: $$PWD")
include( ../../../subproject_integration.pri )

QT -= gui
QT += core network sql

with-shvwebsockets {
	QT += websockets
	DEFINES += WITH_SHV_WEBSOCKETS
}

CONFIG += c++11

TEMPLATE = app
TARGET = sampleshvbroker

DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin
unix:LIBDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:LIBDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

INCLUDEPATH += \
	$$SHV_PROJECT_TOP_SRCDIR/3rdparty/necrolog/include \
	$$LIBSHV_SRC_DIR/libshvchainpack/include \
	$$LIBSHV_SRC_DIR/libshvcore/include \
	$$LIBSHV_SRC_DIR/libshvcoreqt/include \
	$$LIBSHV_SRC_DIR/libshviotqt/include \
	$$LIBSHV_SRC_DIR/libshvbroker/include \

LIBS += \
        -L$$LIBDIR \

LIBS += \
    -lnecrolog \
    -lshvchainpack \
    -lshvcore \
    -lshvcoreqt \
    -lshviotqt \
    -lshvbroker \

unix {
        LIBS += \
                -Wl,-rpath,\'\$\$ORIGIN/../lib\'
}

RESOURCES += \
        #shvbroker.qrc \


TRANSLATIONS += \

include (src/src.pri)


