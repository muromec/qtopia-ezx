qtopia_project(qtopia core app)
TARGET=qtopia-dbus-launcher
CONFIG+=no_tr

HEADERS		= launcher.h
SOURCES		= main.cpp launcher.cpp 

depends(3rdparty/libraries/qtdbus)

pkg.desc=Interprocess communication client for Qtopia.
#pkg.domain=trusted
pkg.domain=trusted
