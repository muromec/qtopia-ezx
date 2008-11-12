qtopia_project(qtopia app)
TARGET=appearance
CONFIG+=qtopia_main no_quicklaunch

HEADERS		= appearance.h
SOURCES		= appearance.cpp main.cpp

desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/appearance.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop
help.source=$$QTOPIA_DEPOT_PATH/help
help.files=appearance*
help.hint=help
INSTALLS+=help
pics.files=$$QTOPIA_DEPOT_PATH/pics/appearance/*
pics.path=/pics/appearance
pics.hint=pics
INSTALLS+=pics
captureService.files=$$QTOPIA_DEPOT_PATH/services/Settings/appearance
captureService.path=/services/Settings
INSTALLS+=captureService

pkg.desc=Appearance settings for Qtopia.
pkg.domain=trusted
