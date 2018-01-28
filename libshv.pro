TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    3rdparty \
    libshvchainpack \
    libshvcore \
    libshvcoreqt \

CONFIG(debug, debug|release) {
SUBDIRS += \
    tests \
}

!no-libshv-gui {
SUBDIRS += \
    libshvgui \
}
