qtopia_project(qtopia plugin)
TARGET=modifiedgsm0710multiplex
requires(enable_modem)

CONFIG+=no_tr

# Installing this example in default Qtopia setups will interfere
# with correct operation of the standard multiplexer plugins.
# Remove this line if you wish to install it for testing.
CONFIG+=no_install

HEADERS		=  modifiedgsm0710.h
SOURCES	        =  modifiedgsm0710.cpp

depends(libraries/qtopiacomm/serial)

