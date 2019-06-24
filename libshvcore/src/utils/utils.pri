unix {
HEADERS += \
    $$PWD/fileshvjournal.h \

SOURCES += \
    $$PWD/fileshvjournal.cpp \
}

HEADERS += \
    $$PWD/crypt.h \
    $$PWD/versioninfo.h \
	$$PWD/clioptions.h \
    $$PWD/shvpath.h \
    $$PWD/shvjournalgetlogparams.h

SOURCES += \
    $$PWD/crypt.cpp \
    $$PWD/versioninfo.cpp \
	$$PWD/clioptions.cpp \
    $$PWD/shvpath.cpp \
    $$PWD/shvjournalgetlogparams.cpp

