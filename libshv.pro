TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    3rdparty \
    libshvchainpack \
    libshvcore \
    libshvcoreqt \
    libshviotqt \
    libshvvisu \
    utils \

CONFIG(debug, debug|release) {
SUBDIRS += \
    tests \
}
