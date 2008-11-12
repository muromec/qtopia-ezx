qtopia_project(qtopia app)
TARGET=language
CONFIG+=qtopia_main no_quicklaunch

FORMS	= languagesettingsbase.ui
HEADERS		= languagesettings.h langmodel.h
SOURCES		= language.cpp main.cpp langmodel.cpp

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=language.html
help.hint=help
INSTALLS+=help
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/language.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop
pics.files=$$QTOPIA_DEPOT_PATH/pics/language/*
pics.path=/pics/language
pics.hint=pics
INSTALLS+=pics

pkg.description=Language settings application
pkg.domain=trusted
