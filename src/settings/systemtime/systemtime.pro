qtopia_project(qtopia app)
TARGET=systemtime
CONFIG+=qtopia_main

HEADERS		= settime.h
SOURCES		= settime.cpp main.cpp

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=systemtime*
help.hint=help
INSTALLS+=help
timeservice.files+=$$QTOPIA_DEPOT_PATH/services/Time/systemtime
timeservice.path=/services/Time
INSTALLS+=timeservice
dateservice.files+=$$QTOPIA_DEPOT_PATH/services/Date/systemtime
dateservice.path=/services/Date
INSTALLS+=dateservice
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/systemtime.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop
pics.files=$$QTOPIA_DEPOT_PATH/pics/systemtime/*
pics.path=/pics/systemtime
pics.hint=pics
INSTALLS+=pics

pkg.desc=Date/time setting dialog for Qtopia.
pkg.domain=trusted
