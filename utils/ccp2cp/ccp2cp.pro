message("========== project: $$PWD")
include( ../../subproject_integration.pri )

TEMPLATE = app

QT -= core widgets gui

QMAKE_CFLAGS += -std=gnu11

DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

CCPCP_SRC_DIR = ../../libshvchainpack/c

INCLUDEPATH += \
	$$CCPCP_SRC_DIR \

SOURCES += \
	main.c \
    $$CCPCP_SRC_DIR/ccpcp.c \
	$$CCPCP_SRC_DIR/ccpon.c \
	$$CCPCP_SRC_DIR/cchainpack.c \
	$$CCPCP_SRC_DIR/ccpcp_convert.c \


HEADERS += \
	$$CCPCP_SRC_DIR/ccpcp.h \
	$$CCPCP_SRC_DIR/ccpon.h \
	$$CCPCP_SRC_DIR/cchainpack.h \
	$$CCPCP_SRC_DIR/ccpcp_convert.h \

