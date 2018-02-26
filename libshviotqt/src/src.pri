HEADERS += \
	$$PWD/shviotqtglobal.h \
    $$PWD/shvnodetree.h \
    $$PWD/shvnode.h

SOURCES += \
    $$PWD/shvnodetree.cpp \
    $$PWD/shvnode.cpp

include ($$PWD/chainpack/chainpack.pri)
include ($$PWD/client/client.pri)
include ($$PWD/server/server.pri)

