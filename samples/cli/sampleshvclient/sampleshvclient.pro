message("========== project: $$PWD")
include( ../../../subproject_integration.pri )

QT += core network
QT -= gui
CONFIG += c++11

TEMPLATE = app
TARGET = sampleshvclient

DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

INCLUDEPATH += \
	$$SHV_PROJECT_TOP_SRCDIR/3rdparty/necrolog/include \
	$$LIBSHV_SRC_DIR/libshvchainpack/include \
	$$LIBSHV_SRC_DIR/libshvcore/include \
	$$LIBSHV_SRC_DIR/libshvcoreqt/include \
	$$LIBSHV_SRC_DIR/libshviotqt/include \
	$$LIBSHV_SRC_DIR/libshvbroker/include \

LIBDIR = $$DESTDIR
unix: LIBDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib

LIBS += \
        -L$$LIBDIR \

LIBS += \
    -lnecrolog \
    -lshvchainpack \
    -lshvcore \
    -lshvcoreqt \
    -lshviotqt \

unix {
        LIBS += \
                -Wl,-rpath,\'\$\$ORIGIN/../lib\'
}

SHV_TOP_SRCDIR = $$PWD/..

RESOURCES += \
        #shvbroker.qrc \


TRANSLATIONS += \
#        ../eyassrv.pl_PL.ts \

include (src/src.pri)


