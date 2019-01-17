TEMPLATE = app

QT -= core widgets gui

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
	SHV_PROJECT_TOP_BUILDDIR=$$shadowed($$PWD)/..
}
message ( SHV_PROJECT_TOP_BUILDDIR: '$$SHV_PROJECT_TOP_BUILDDIR' )

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
	../../3rdparty/necrolog/include \
	../../libshvchainpack/include \
	#../../libshvcore/include \

SOURCES += \
	main.cpp \

HEADERS += \

