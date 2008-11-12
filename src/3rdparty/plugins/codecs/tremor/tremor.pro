qtopia_project(qtopia plugin)
TARGET=tremorplugin
CONFIG+=no_tr
license(FREEWARE)

HEADERS = \
        oggplugin.h \
        oggdecoder.h

SOURCES = \
        oggplugin.cpp \
        oggdecoder.cpp

depends(3rdparty/libraries/tremor)
depends(libraries/qtopiamedia)
