qtopia_project(embedded qtopia core plugin)
depends(libraries/qtopia)
TARGET = perftestqvfbscreen

CONFIG+=no_tr

HEADERS = perftestqvfbscreendriverplugin.h perftestqvfbscreen.h
SOURCES = perftestqvfbscreendriverplugin.cpp perftestqvfbscreen.cpp

