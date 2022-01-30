HEADERS += \
    $$PWD/brokertcpserver.h \
	$$PWD/ssl_common.h \
    $$PWD/clientconnectiononbroker.h \
    $$PWD/commonrpcclienthandle.h \
    $$PWD/masterbrokerconnection.h

SOURCES += \
    $$PWD/brokertcpserver.cpp \
	$$PWD/ssl_common.cpp \
    $$PWD/clientconnectiononbroker.cpp \
    $$PWD/commonrpcclienthandle.cpp \
    $$PWD/masterbrokerconnection.cpp

with-shvwebsockets {
HEADERS += \
    $$PWD/websocketserver.h

SOURCES += \
    $$PWD/websocketserver.cpp
}
