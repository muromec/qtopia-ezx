qtopia_project(qtopia plugin)
TARGET=vorbis

CONFIG+=no_tr

HEADERS = vorbiscontentplugin.h 
SOURCES = vorbiscontentplugin.cpp 

depends(libraries/qtopia)
depends(libraries/qtopiamedia)
depends(3rdparty/libraries/tremor)

