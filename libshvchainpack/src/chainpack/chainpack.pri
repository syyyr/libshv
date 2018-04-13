SOURCES += \
    $$PWD/rpc.cpp \
    $$PWD/rpcmessage.cpp \
    $$PWD/rpcvalue.cpp \
    $$PWD/rpcdriver.cpp \
    $$PWD/metatypes.cpp \
    $$PWD/exception.cpp \
    $$PWD/utils.cpp \
    $$PWD/abstractstreamreader.cpp \
    $$PWD/abstractstreamwriter.cpp \
    $$PWD/cponwriter.cpp \
    $$PWD/cponreader.cpp \
    $$PWD/chainpackwriter.cpp \
    $$PWD/cpon.cpp \
    $$PWD/chainpack.cpp \
    $$PWD/chainpackreader.cpp \
    $$PWD/abstractrpcconnection.cpp \
    $$PWD/metamethod.cpp

HEADERS += \
    $$PWD/rpc.h \
    $$PWD/rpcmessage.h \
    $$PWD/rpcvalue.h \
    $$PWD/rpcdriver.h \
    $$PWD/metatypes.h \
    $$PWD/exception.h \
    $$PWD/utils.h \
    $$PWD/abstractstreamreader.h \
    $$PWD/abstractstreamwriter.h \
    $$PWD/cponwriter.h \
    $$PWD/cponreader.h \
    $$PWD/chainpackwriter.h \
    $$PWD/cpon.h \
    $$PWD/chainpack.h \
    $$PWD/chainpackreader.h \
    $$PWD/abstractrpcconnection.h \
    $$PWD/metamethod.h

unix {
SOURCES += \
    $$PWD/socketrpcdriver.cpp \

HEADERS += \
    $$PWD/socketrpcdriver.h \
}
