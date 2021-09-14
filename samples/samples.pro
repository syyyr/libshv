TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
	cli \

qtHaveModule(gui) {
SUBDIRS += \
    gui \
}


