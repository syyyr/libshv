message("including $$PWD")

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
		SHV_PROJECT_TOP_BUILDDIR = $$OUT_PWD/../../..
}
isEmpty(SHV_PROJECT_TOP_SRCDIR) {
	SHV_PROJECT_TOP_SRCDIR = $$PWD/../../..
}
LIBSHV_SRC_DIR = $$SHV_PROJECT_TOP_SRCDIR/3rdparty/libshv

message ( SHV_PROJECT_TOP_BUILDDIR == '$$SHV_PROJECT_TOP_BUILDDIR' )
message ( SHV_PROJECT_TOP_SRCDIR == '$$SHV_PROJECT_TOP_SRCDIR' )
message ( LIBSHV_SRC_DIR == '$$LIBSHV_SRC_DIR' )

TEMPLATE = app

QT -= core widgets gui

DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin
unix:LIBDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:LIBDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

LIBS += \
    -L$$LIBDIR \
    -lnecrolog \
    -lshvchainpack \
    #-lshvcore \

unix {
    LIBS += \
        -Wl,-rpath,\'\$\$ORIGIN/../lib\'
}

INCLUDEPATH += \
	$$SHV_PROJECT_TOP_SRCDIR/3rdparty/necrolog/include \
	$$LIBSHV_SRC_DIR/libshvchainpack/include \

SOURCES += \
	main.cpp \

HEADERS += \

