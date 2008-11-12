qtopia_project(qtopia app)
TARGET=worldtime
CONFIG+=qtopia_main no_quicklaunch

HEADERS		= worldtime.h cityinfo.h
SOURCES		= worldtime.cpp main.cpp cityinfo.cpp

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=worldtime*
help.hint=help
INSTALLS+=help
pics.files=$$QTOPIA_DEPOT_PATH/pics/worldtime/*
pics.path=/pics/worldtime
pics.hint=pics
INSTALLS+=pics
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/worldtime.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop

pkg.desc=The time-zone manager for Qtopia.
pkg.domain=trusted
