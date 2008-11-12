qtopia_project(qtopia plugin)
TARGET=desktopaudiohardware

HEADERS		=  desktopaudioplugin.h
SOURCES	        =  desktopaudioplugin.cpp

depends(libraries/qtopiaaudio)
depends(libraries/qtopiacomm)
