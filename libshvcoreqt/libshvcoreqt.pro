message("including $$PWD")

QT -= gui

CONFIG += C++11
CONFIG += hide_symbols

TEMPLATE = lib
TARGET = shvcoreqt

include( ../subproject_integration.pri )

unix:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

message ( DESTDIR: $$DESTDIR )

android {
DEFINES += ANDROID_BUILD
}

DEFINES += SHVCOREQT_BUILD_DLL

INCLUDEPATH += \
    $$SHV_PROJECT_TOP_SRCDIR/3rdparty/necrolog/include \
	$$PWD/../libshvcore/include \
	$$PWD/../libshvchainpack/include \

LIBS += \
	-L$$DESTDIR

android {
LIBS += -lnecrolog_$${QT_ARCH} \
		-lshvcore_$${QT_ARCH} \
		-lshvchainpack_$${QT_ARCH}
}
else {
	LIBS += -lnecrolog \
			-lshvcore \
			-lshvchainpack
}

include($$PWD/src/src.pri)
