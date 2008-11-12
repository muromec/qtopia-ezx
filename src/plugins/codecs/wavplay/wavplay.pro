qtopia_project(qtopia plugin)
TARGET=wavplay
CONFIG+=no_tr

HEADERS = \
        wavplugin.h \
        wavdecoder.h

SOURCES = \
        wavplugin.cpp \
        wavdecoder.cpp

depends(libraries/qtopiamedia)



