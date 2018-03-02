HEADERS += \
	$$PWD/shviotqtglobal.h \
    $$PWD/shvnodetree.h \
    $$PWD/shvnode.h

SOURCES += \
    $$PWD/shvnodetree.cpp \
    $$PWD/shvnode.cpp

include ($$PWD/rpc/rpc.pri)
# include ($$PWD/client/client.pri)

