qtopia_project(qtopia app)
TARGET=serverwidgets
CONFIG+=qtopia_main

HEADERS		= serverwidgets.h
SOURCES		= serverwidgets.cpp main.cpp

# TODO Server Widgets can't install help when there are no help files to install
# Either this code should be removed or the missing files should be added
#help.source=$$QTOPIA_DEPOT_PATH/help
#help.files=serverwidgets*
#help.hint=help
#INSTALLS+=help
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/serverwidgets.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop
pics.files=$$QTOPIA_DEPOT_PATH/pics/serverwidgets/*
pics.path=/pics/serverwidgets
pics.hint=pics
INSTALLS+=pics

pkg.desc=Changes Qtopia server UI.
pkg.domain=trusted
