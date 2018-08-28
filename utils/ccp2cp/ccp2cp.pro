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

CCPCP_SRC_DIR = ../../libshvchainpack/c

INCLUDEPATH += \
	$$CCPCP_SRC_DIR \

SOURCES += \
	main.c \
	$$CCPCP_SRC_DIR/ccpon.c \
	$$CCPCP_SRC_DIR/cchainpack.c \
    $$CCPCP_SRC_DIR/ccpcp.c


HEADERS += \
	$$CCPCP_SRC_DIR/ccpcp.h \
	$$CCPCP_SRC_DIR/ccpon.h \
	$$CCPCP_SRC_DIR/cchainpack.h \

