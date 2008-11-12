qtopia_project(qtopia plugin)
TARGET=greenphonemultiplex

CONFIG+=no_tr

HEADERS		=  greenphonemultiplexer.h
SOURCES	        =  greenphonemultiplexer.cpp

requires(enable_modem)
depends(libraries/qtopiacomm/serial)
