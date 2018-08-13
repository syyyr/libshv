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
CCPON_SRC_DIR = ../../libshvchainpack/c

INCLUDEPATH += \
	$$NECROLOG_SRC_DIR \
	$$CCPON_SRC_DIR \

SOURCES += \
	main.cpp \
	$$NECROLOG_SRC_DIR/necrolog.cpp \
	$$CCPON_SRC_DIR/ccpon.c \


HEADERS += \
	$$NECROLOG_SRC_DIR/necrolog.h \
	$$CCPON_SRC_DIR/ccpon.h \

