qtopia_project(qtopia plugin)
TARGET=exif

CONFIG+=no_tr

HEADERS		=  exifcontentplugin.h \
                   ifd.h

SOURCES	        =  exifcontentplugin.cpp \
                   ifd.cpp

depends(libraries/qtopia)