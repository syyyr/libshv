HEADERS += \
    $$PWD/brokertcpserver.h \
    $$PWD/clientconnection.h \
    $$PWD/commonrpcclienthandle.h \
    $$PWD/masterbrokerconnection.h

SOURCES += \
    $$PWD/brokertcpserver.cpp \
    $$PWD/clientconnection.cpp \
    $$PWD/commonrpcclienthandle.cpp \
    $$PWD/masterbrokerconnection.cpp

with-shvwebsockets {
HEADERS += \
    $$PWD/websocketserver.h \
    $$PWD/websocket.h

SOURCES += \
    $$PWD/websocketserver.cpp \
    $$PWD/websocket.cpp
}
