qtopia_project(qtopia app)
TARGET=beaming
CONFIG+=qtopia_main no_quicklaunch

HEADERS		= ircontroller.h beaming.h
SOURCES		= ircontroller.cpp beaming.cpp main.cpp

depends(libraries/qtopiacomm)

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=beaming*
help.hint=help
INSTALLS+=help
pics.files=$$QTOPIA_DEPOT_PATH/pics/beaming/*
pics.path=/pics/beaming
pics.hint=pics
INSTALLS+=pics
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/beaming.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop

pkg.desc=Beaming settings dialog for Qtopia.
pkg.domain=trusted
