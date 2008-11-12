qtopia_project(qtopia app)
TARGET=logging
CONFIG+=qtopia_main no_quicklaunch

HEADERS		= loggingedit.h loggingview.h
SOURCES		= loggingedit.cpp loggingview.cpp main.cpp

TRANSLATABLES += loggingedit.cpp loggingview.cpp

desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/logging.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop
# TODO Logging can't install help when there are no help files to install
# Either this code should be removed or the missing help files should be added
#help.source=$$QTOPIA_DEPOT_PATH/help
#help.files=logging*
#help.hint=help
#INSTALLS+=help
pics.files=pics/*
pics.path=/pics/logging
pics.hint=pics
INSTALLS+=pics

pkg.desc=Logging settings dialog for Qtopia.
pkg.domain=trusted
