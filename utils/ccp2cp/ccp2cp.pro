TEMPLATE = app

QT -= core widgets

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
	SHV_PROJECT_TOP_BUILDDIR=$$shadowed($$PWD)/..
}
message ( SHV_PROJECT_TOP_BUILDDIR: '$$SHV_PROJECT_TOP_BUILDDIR' )

DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin
unix:LIBDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:LIBDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

LIBS += \
#    -L$$LIBDIR \
#    -lnecrolog \
#    -lshvchainpack \
   #-lshvcore \

unix {
#    LIBS += \
#        -Wl,-rpath,\'\$\$ORIGIN/../lib\'
}

NECROLOG_SRC_DIR = ../../3rdparty/necrolog/libnecrolog
CCPCP_SRC_DIR = ../../libshvchainpack/c

INCLUDEPATH += \
	$$NECROLOG_SRC_DIR \
	$$CCPCP_SRC_DIR \

SOURCES += \
	main.cpp \
	$$NECROLOG_SRC_DIR/necrolog.cpp \
	$$CCPCP_SRC_DIR/ccpon.c \
	$$CCPCP_SRC_DIR/cchainpack.c \
    ../../libshvchainpack/c/ccpcp.c


HEADERS += \
	$$NECROLOG_SRC_DIR/necrolog.h \
	$$CCPCP_SRC_DIR/ccpcp.h \
	$$CCPCP_SRC_DIR/ccpon.h \
	$$CCPCP_SRC_DIR/cchainpack.h \

