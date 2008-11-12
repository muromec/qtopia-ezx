qtopia_project(qtopia plugin)
TARGET=c3200audiohardware

HEADERS		=  c3200audioplugin.h
SOURCES	        =  c3200audioplugin.cpp

depends(libraries/qtopiaaudio)
