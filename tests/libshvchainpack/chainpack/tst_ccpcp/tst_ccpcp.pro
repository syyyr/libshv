# project file to edit this test case in Qt creator

QT -= core gui

TARGET = tst_ccpcp

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
	SHV_PROJECT_TOP_BUILDDIR=$$shadowed($$PWD)/..
}
message ( SHV_PROJECT_TOP_BUILDDIR: '$$SHV_PROJECT_TOP_BUILDDIR' )

DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin
message (DESTDIR: $$DESTDIR)

CCPCP_DIR=$$PWD/../../../../libshvchainpack/c

message (CCPCP_DIR: $$CCPCP_DIR)

INCLUDEPATH += $$CCPCP_DIR

HEADERS += \
    $$CCPCP_DIR/ccpcp.h \
    $$CCPCP_DIR/ccpon.h \
    $$CCPCP_DIR/cchainpack.h \
    $$CCPCP_DIR/ccpcp_convert.h \

SOURCES += \
    $${TARGET}.c \
    $$CCPCP_DIR/ccpcp.c \
    $$CCPCP_DIR/ccpon.c \
    $$CCPCP_DIR/cchainpack.c \
    $$CCPCP_DIR/ccpcp_convert.c \


