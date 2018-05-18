HEADERS += \
	$$PWD/../3rdparty/necrolog/necrolog.h \
	$$PWD/shvcoreglobal.h \
    $$PWD/string.h \
    $$PWD/utils.h \
    $$PWD/assert.h \
    $$PWD/log.h \
    $$PWD/exception.h \
    $$PWD/stringview.h

SOURCES += \
    $$PWD/string.cpp \
    $$PWD/utils.cpp \
    $$PWD/exception.cpp \
    $$PWD/stringview.cpp


include ($$PWD/utils/utils.pri)

