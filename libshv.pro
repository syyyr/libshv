TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    libshvchainpack \
    libshvcore \
    libshvcoreqt \

CONFIG(debug, debug|release) {
SUBDIRS += \
    tests \
}

!beaglebone {
SUBDIRS += \
    libshvgui \
}
