qtopia_project(qtopia app)
TARGET=light-and-power
CONFIG+=qtopia_main no_quicklaunch

FORMS	= lightsettingsbase.ui 
TRANSLATABLES = $${FORMS}
HEADERS		= light.h minsecspinbox.h
SOURCES		= light.cpp main.cpp minsecspinbox.cpp

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=light-and-power*
help.hint=help
INSTALLS+=help
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/light-and-power.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop
pics.files=$$QTOPIA_DEPOT_PATH/pics/light-and-power/*
pics.path=/pics/light-and-power
pics.hint=pics
INSTALLS+=pics
captureService.files=$$QTOPIA_DEPOT_PATH/services/Settings/light-and-power
captureService.path=/services/Settings
INSTALLS+=captureService

pkg.desc=Light and Power settings dialog for Qtopia.
pkg.domain=trusted
