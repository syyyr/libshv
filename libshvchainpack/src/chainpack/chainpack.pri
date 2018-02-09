SOURCES += \
    $$PWD/rpc.cpp \
    $$PWD/chainpackprotocol.cpp \
    $$PWD/rpcmessage.cpp \
    $$PWD/rpcvalue.cpp \
    $$PWD/rpcdriver.cpp \
    $$PWD/metatypes.cpp \
    $$PWD/exception.cpp \
    $$PWD/utils.cpp \
    $$PWD/abstractstreamreader.cpp \
    $$PWD/abstractstreamwriter.cpp \
    $$PWD/cponwriter.cpp \
    $$PWD/cponreader.cpp

HEADERS += \
    $$PWD/rpc.h \
    $$PWD/chainpackprotocol.h \
    $$PWD/rpcmessage.h \
    $$PWD/rpcvalue.h \
    $$PWD/rpcdriver.h \
    $$PWD/metatypes.h \
    $$PWD/exception.h \
    $$PWD/utils.h \
    $$PWD/abstractstreamreader.h \
    $$PWD/abstractstreamwriter.h \
    $$PWD/cponwriter.h \
    $$PWD/cponreader.h

unix {
SOURCES += \
    $$PWD/socketrpcdriver.cpp \

HEADERS += \
    $$PWD/socketrpcdriver.h \
}
