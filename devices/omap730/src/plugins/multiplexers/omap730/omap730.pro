qtopia_project(qtopia plugin)
TARGET=omap730multiplex

CONFIG+=no_tr

HEADERS		=  omap730multiplexer.h
SOURCES	        =  omap730multiplexer.cpp

depends(libraries/qtopiacomm)

