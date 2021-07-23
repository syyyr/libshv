TEMPLATE = subdirs
CONFIG += ordered

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
	SHV_PROJECT_TOP_BUILDDIR=$$shadowed($$PWD)/..
}

SUBDIRS += \
    necrolog/libnecrolog \


