qtopia_project(qtopia app)
depends(libraries/qtopia)
TARGET=homescreen
CONFIG+=qtopia_main no_quicklaunch

HEADERS		= homescreen.h
SOURCES		= homescreen.cpp main.cpp

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=homescreen*
help.hint=help
INSTALLS+=help
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/homescreen.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop
pics.files=$$QTOPIA_DEPOT_PATH/pics/homescreen/*
pics.path=/pics/homescreen
pics.hint=pics
INSTALLS+=pics

pkg.desc=Homescreen settings dialog for Qtopia.
pkg.domain=trusted
