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
    $$PWD/chainpackreader.cpp \
    $$PWD/chainpack.cpp \
    $$PWD/chainpackwriter.cpp \
    $$PWD/abstractstreamtokenizer.cpp \
    $$PWD/cpontokenizer.cpp \
    $$PWD/chainpacktokenizer.cpp

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
    $$PWD/chainpackreader.h \
    $$PWD/chainpack.h \
    $$PWD/chainpackwriter.h \
    $$PWD/abstractstreamtokenizer.h \
    $$PWD/cpontokenizer.h \
    $$PWD/chainpacktokenizer.h

unix {
SOURCES += \
    $$PWD/socketrpcdriver.cpp \

HEADERS += \
    $$PWD/socketrpcdriver.h \
}
