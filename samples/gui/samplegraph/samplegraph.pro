message("including $$PWD")

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
        SHV_PROJECT_TOP_BUILDDIR = $$OUT_PWD/../../..
}

isEmpty(LIBSHV_SRC_DIR) {
        LIBSHV_SRC_DIR = $$PWD/../../..
}
message ( SHV_PROJECT_TOP_BUILDDIR == '$$SHV_PROJECT_TOP_BUILDDIR' )
message ( SHV_PROJECT_TOP_SRCDIR == '$$SHV_PROJECT_TOP_SRCDIR' )
message ( LIBSHV_SRC_DIR == '$$LIBSHV_SRC_DIR' )

QT       += core gui widgets

CONFIG += c++11

TEMPLATE = app
TARGET = samplegraph

DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin
unix:LIBDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:LIBDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

LIBS += \
        -L$$LIBDIR \

LIBS += \
    -lnecrolog \
	-lshvchainpack \
	-lshvcore \
	-lshvcoreqt \
	-lshviotqt \
	-lshvvisu \

unix {
        LIBS += \
		        -Wl,-rpath,\'\$\$ORIGIN/../lib\'
}

INCLUDEPATH += \
    $$LIBSHV_SRC_DIR/3rdparty/necrolog/include \
	$$LIBSHV_SRC_DIR/libshvchainpack/include \
	$$LIBSHV_SRC_DIR/libshvcore/include \
	$$LIBSHV_SRC_DIR/libshvcoreqt/include \
	#$$LIBSHV_SRC_DIR/libshviotqt/include \
	$$LIBSHV_SRC_DIR/libshvvisu/include \

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui


