SOURCES += \
    $$PWD/chainpackprotocol.cpp \
    $$PWD/rpcmessage.cpp \
    $$PWD/rpcvalue.cpp \
    $$PWD/rpcdriver.cpp \
    $$PWD/metatypes.cpp \
    $$PWD/cponprotocol.cpp \
    $$PWD/exception.cpp \
    $$PWD/utils.cpp

HEADERS += \
    $$PWD/chainpackprotocol.h \
    $$PWD/rpcmessage.h \
    $$PWD/rpcvalue.h \
    $$PWD/rpcdriver.h \
    $$PWD/metatypes.h \
    $$PWD/cponprotocol.h \
    $$PWD/exception.h \
    $$PWD/utils.h

unix {
SOURCES += \
    $$PWD/socketrpcdriver.cpp \

HEADERS += \
    $$PWD/socketrpcdriver.h \
}
