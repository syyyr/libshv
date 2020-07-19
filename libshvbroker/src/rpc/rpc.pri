HEADERS += \
    $$PWD/brokerclientserverconnection.h \
    $$PWD/brokertcpserver.h \
    $$PWD/commonrpcclienthandle.h \
    $$PWD/slavebrokerclientconnection.h

SOURCES += \
    $$PWD/brokerclientserverconnection.cpp \
    $$PWD/brokertcpserver.cpp \
    $$PWD/commonrpcclienthandle.cpp \
    $$PWD/slavebrokerclientconnection.cpp

with-shvwebsockets {
HEADERS += \
    $$PWD/websocketserver.h \
    $$PWD/websocket.h

SOURCES += \
    $$PWD/websocketserver.cpp \
    $$PWD/websocket.cpp
}
