qtopia_project(qtopia plugin)
TARGET=threegpp

CONFIG+=no_tr

HEADERS = threegppcontentplugin.h 
SOURCES = threegppcontentplugin.cpp 

depends(libraries/qtopia)
