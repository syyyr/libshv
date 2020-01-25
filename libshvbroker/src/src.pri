
HEADERS += \
    $$PWD/aclmanager.h \
    $$PWD/aclmanagersqlite.h \
    $$PWD/brokeraclnode.h \
    $$PWD/shvbrokerglobal.h \
    $$PWD/appclioptions.h \
    $$PWD/brokerapp.h \
    $$PWD/brokernode.h \
    $$PWD/subscriptionsnode.h \
    $$PWD/clientconnectionnode.h \
    $$PWD/clientshvnode.h \
    $$PWD/tunnelsecretlist.h

SOURCES += \
    $$PWD/aclmanagersqlite.cpp \
    $$PWD/appclioptions.cpp \
    $$PWD/brokeraclnode.cpp \
    $$PWD/brokerapp.cpp \
    $$PWD/brokernode.cpp \
    $$PWD/aclmanager.cpp \
    $$PWD/subscriptionsnode.cpp \
    $$PWD/clientconnectionnode.cpp \
    $$PWD/clientshvnode.cpp \
    $$PWD/tunnelsecretlist.cpp

include ($$PWD/rpc/rpc.pri)




