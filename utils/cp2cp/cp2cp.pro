message("========== project: $$PWD")
include( ../../subproject_integration.pri )

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

