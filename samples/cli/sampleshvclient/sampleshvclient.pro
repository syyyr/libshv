message("including $$PWD")

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
		SHV_PROJECT_TOP_BUILDDIR = $$OUT_PWD/../../..
} else {
		message ( SHV_PROJECT_TOP_BUILDDIR is not empty and set to $$SHV_PROJECT_TOP_BUILDDIR )
		message ( This is obviously done in file $$SHV_PROJECT_TOP_SRCDIR/.qmake.conf )
}

isEmpty(SHV_PROJECT_TOP_SRCDIR) {
	SHV_PROJECT_TOP_SRCDIR = $$PWD/../../..
	LIBSHV_SRC_DIR = $$SHV_PROJECT_TOP_SRCDIR
} else {
	LIBSHV_SRC_DIR = $$SHV_PROJECT_TOP_SRCDIR/3rdparty/libshv
	message ( SHV_PROJECT_TOP_SRCDIR is not empty and set to $$SHV_PROJECT_TOP_SRCDIR )
	message ( This is obviously done in file $$SHV_PROJECT_TOP_SRCDIR/.qmake.conf )
}

message ( SHV_PROJECT_TOP_BUILDDIR == '$$SHV_PROJECT_TOP_BUILDDIR' )
message ( SHV_PROJECT_TOP_SRCDIR == '$$SHV_PROJECT_TOP_SRCDIR' )
message ( LIBSHV_SRC_DIR == '$$LIBSHV_SRC_DIR' )

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


