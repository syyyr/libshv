
HEADERS += \
    $$PWD/shvbrokerglobal.h \
    $$PWD/appclioptions.h \
    $$PWD/brokerapp.h \
    $$PWD/brokernode.h \
    $$PWD/subscriptionsnode.h \
    $$PWD/brokerconfigfilenode.h \
    $$PWD/clientconnectionnode.h \
    $$PWD/clientshvnode.h \
    $$PWD/tunnelsecretlist.h

SOURCES += \
    $$PWD/appclioptions.cpp \
    $$PWD/brokerapp.cpp \
    $$PWD/brokernode.cpp \
    $$PWD/subscriptionsnode.cpp \
    $$PWD/brokerconfigfilenode.cpp \
    $$PWD/clientconnectionnode.cpp \
    $$PWD/clientshvnode.cpp \
    $$PWD/tunnelsecretlist.cpp

include ($$PWD/rpc/rpc.pri)




