qtopia_project(qtopia app)
TARGET=profileedit
CONFIG+=qtopia_main

HEADERS		= ringprofile.h
SOURCES		= main.cpp ringprofile.cpp

HEADERS += ringtoneeditor.h
SOURCES += ringtoneeditor.cpp

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=profileedit*
help.hint=help
INSTALLS+=help
desktop.files	=$$QTOPIA_DEPOT_PATH/apps/Settings/ringprofile.desktop
desktop.path	=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop
pics.files	=$$QTOPIA_DEPOT_PATH/pics/profileedit/*
pics.path	=/pics/profileedit
pics.hint=pics
INSTALLS+=pics
service.files   =$$QTOPIA_DEPOT_PATH/services/Profiles/profileedit
service.path	=/services/Profiles
INSTALLS+=service
serviceb.files   =$$QTOPIA_DEPOT_PATH/services/SettingsManager/profileedit
serviceb.path	=/services/SettingsManager
INSTALLS+=serviceb

pkg.desc=Phone profile settings for Qtopia.
pkg.domain=trusted
