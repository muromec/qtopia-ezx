qtopia_project(qtopia plugin)
TARGET=wavecommultiplex

CONFIG+=no_tr

HEADERS		=  wavecommultiplexer.h
SOURCES	        =  wavecommultiplexer.cpp

depends(libraries/qtopiacomm/serial)

