qtopia_project(qtopia plugin)
TARGET=ficgta01multiplex

CONFIG+=no_tr

HEADERS		=  ficgta01multiplexer.h
SOURCES	        =  ficgta01multiplexer.cpp

requires(enable_modem)
depends(libraries/qtopiacomm/serial)
