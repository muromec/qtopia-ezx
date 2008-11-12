qtopia_project(qtopia app)
TARGET=phonenetworks
CONFIG+=qtopia_main no_quicklaunch

HEADERS		= phonenetworks.h \
                  modemnetwork.h \
                  voipnetwork.h
SOURCES		= phonenetworks.cpp main.cpp \
                  modemnetwork.cpp \
                  voipnetwork.cpp

depends(libraries/qtopiapim)
depends(libraries/qtopiaphone)

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=phonenetworks*
help.hint=help
INSTALLS+=help
service.files=$$QTOPIA_DEPOT_PATH/services/CallNetworks/phonenetworks
service.path=/services/CallNetworks
INSTALLS+=service
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/phonenetworks.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop
pics.files=$$QTOPIA_DEPOT_PATH/pics/phonenetworks/*
pics.path=/pics/phonenetworks
pics.hint=pics
INSTALLS+=pics
settings.files=$$QTOPIA_DEPOT_PATH/etc/default/Trolltech/GsmOperatorCountry.conf
settings.path=/etc/default/Trolltech
INSTALLS+=settings

pkg.desc=Phone Network selection for Qtopia.
pkg.domain=trusted
