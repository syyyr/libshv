include ( $$PWD/../test.pri )

QT -= gui

INCLUDEPATH += \
	$$PWD/../../3rdparty/necrolog/include \
	$$PWD/../../libshvchainpack/include \
	$$PWD/../../libshvcore/include \
	$$PWD/../../libshvcoreqt/include \
	$$PWD/../../libshviotqt/include \

win32:LIB_DIR = $$DESTDIR
else:LIB_DIR = $$SHV_PROJECT_TOP_BUILDDIR/lib

message (INCLUDEPATH $$INCLUDEPATH)
message (LIB_DIR $$LIB_DIR)
message (DESTDIR $$DESTDIR)

LIBS += \
    -L$$LIB_DIR \
    -lnecrolog \
    -lshvcoreqt \
    -lshvchainpack \
    -lshvcore \
    -lshviotqt \

unix {
    LIBS += \
        -Wl,-rpath,\'$${LIB_DIR}\'
}

