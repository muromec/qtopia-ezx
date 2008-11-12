qtopia_project(qtopia plugin)
TARGET=multiportmultiplex
requires(enable_modem)

CONFIG+=no_tr

# Installing this example in default Qtopia setups will interfere
# with correct operation of the standard multiplexer plugins.
# Remove this line if you wish to install it for testing.
CONFIG+=no_install

HEADERS		=  multiportmultiplexer.h
SOURCES	        =  multiportmultiplexer.cpp

depends(libraries/qtopiacomm/serial)

