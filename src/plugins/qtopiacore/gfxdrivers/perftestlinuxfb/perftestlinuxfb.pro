qtopia_project(embedded qtopia core plugin)
depends(libraries/qtopia)
TARGET = perftestlinuxfbscreen

CONFIG+=no_tr

HEADERS = perftestlinuxfbscreendriverplugin.h perftestlinuxfbscreen.h
SOURCES = perftestlinuxfbscreendriverplugin.cpp perftestlinuxfbscreen.cpp

