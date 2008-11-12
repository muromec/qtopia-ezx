qtopia_project(qtopia plugin)
TARGET=nokiaaudiohardware

HEADERS		=  nokiaaudioplugin.h
SOURCES	        =  nokiaaudioplugin.cpp

depends(libraries/qtopiaaudio)
depends(libraries/qtopiacomm)
