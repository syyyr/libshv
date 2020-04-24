HEADERS += \
    $$PWD/brokertcpserver.h \
    $$PWD/masterbrokerconnection.h \
    $$PWD/commonrpcclienthandle.h \
    $$PWD/serverconnectionbroker.h

SOURCES += \
    $$PWD/brokertcpserver.cpp \
    $$PWD/masterbrokerconnection.cpp \
    $$PWD/commonrpcclienthandle.cpp \
    $$PWD/serverconnectionbroker.cpp

with-shvwebsockets {
HEADERS += \
    $$PWD/websocketserver.h \
    $$PWD/websocket.h

SOURCES += \
    $$PWD/websocketserver.cpp \
    $$PWD/websocket.cpp
}
