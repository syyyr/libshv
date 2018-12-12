HEADERS += \
	$$PWD/shviotqtglobal.h \
    $$PWD/utils.h

SOURCES += \
    $$PWD/utils.cpp

include ($$PWD/rpc/rpc.pri)
include ($$PWD/node/node.pri)
include ($$PWD/utils/utils.pri)

