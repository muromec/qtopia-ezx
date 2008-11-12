qtopia_project(qtopia app)
TARGET=callforwarding
CONFIG+=qtopia_main no_quicklaunch

HEADERS		= callforwarding.h
SOURCES		= callforwarding.cpp main.cpp

depends(libraries/qtopiapim)

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=callforwarding*
help.hint=help
INSTALLS+=help
service.files=$$QTOPIA_DEPOT_PATH/services/CallForwarding/callforwarding
service.path=/services/CallForwarding
INSTALLS+=service
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/callforwarding.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop
pics.files=$$QTOPIA_DEPOT_PATH/pics/callforwarding/*
pics.path=/pics/callforwarding
pics.hint=pics
INSTALLS+=pics
captureService.files=$$QTOPIA_DEPOT_PATH/services/Settings/callforwarding
captureService.path=/services/Settings
INSTALLS+=captureService

pkg.desc=Call Forwarding for Qtopia.
pkg.domain=trusted
