qtopia_project(qtopia app)
TARGET=iaxsettings
CONFIG+=qtopia_main no_quicklaunch
requires(enable_voip)

FORMS	= iaxsettingsbase.ui 
HEADERS	= iaxsettings.h
SOURCES	= iaxsettings.cpp main.cpp

depends(libraries/qtopiaphone)

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=iaxsettings*
help.hint=help
INSTALLS+=help
desktop.files=$$QTOPIA_DEPOT_PATH/examples/asterisk/desktop/iaxsettings.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop
pics.files=$$QTOPIA_DEPOT_PATH/examples/asterisk/pics/iaxsettings/*
pics.path=/pics/iaxsettings
pics.hint=pics
INSTALLS+=pics

pkg.desc=Asterisk IAX settings dialog for Qtopia.
pkg.domain=trusted
