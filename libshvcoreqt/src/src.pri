HEADERS += \
	$$PWD/shvcoreqtglobal.h \
	$$PWD/log.h \
	$$PWD/exception.h \
	$$PWD/utils.h

SOURCES += \
	$$PWD/log.cpp \
	$$PWD/exception.cpp \
	$$PWD/utils.cpp

include ($$PWD/utils/utils.pri)
include ($$PWD/data/data.pri)


