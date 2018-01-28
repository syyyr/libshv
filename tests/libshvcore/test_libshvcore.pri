include ( $$PWD/../test.pri )

QT -= gui

INCLUDEPATH += \
	$$PWD/../../libshvcore/include

win32:LIB_DIR = $$DESTDIR
else:LIB_DIR = $$SHV_PROJECT_TOP_BUILDDIR/lib

message (LIB_DIR $$LIB_DIR)
message (DESTDIR $$DESTDIR)

LIBS += \
    -L$$LIB_DIR \
    -lnecrolog \
    -lshvcore \

unix {
    LIBS += \
        -Wl,-rpath,\'$${LIB_DIR}\'
}

