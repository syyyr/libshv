TEMPLATE = app

QT -= core gui

TARGET = test-c

CHAINPACK_SRC_DIR = $$PWD/../../../../libshvchainpack

INCLUDEPATH += $${CHAINPACK_SRC_DIR}/c

SOURCES += \
    $${CHAINPACK_SRC_DIR}/c/ccpon.c \
    $${TARGET}.c \


