qtopia_project(qtopia app)
TARGET=btsettings
CONFIG+=qtopia_main

FORMS =       remotedeviceinfo.ui

HEADERS	    = btsettings.h \
              btsettings_p.h \
              settingsdisplay.h \
              mydevicesdisplay.h \
              localservicesdialog.h \
              pairingagent.h \
              remotedeviceinfodialog.h

SOURCES	    = btsettings.cpp \
              settingsdisplay.cpp \
              mydevicesdisplay.cpp \
              localservicesdialog.cpp \
              pairingagent.cpp \
              remotedeviceinfodialog.cpp \
              main.cpp

depends(libraries/qtopiacomm)

desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/btsettings.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop
help.source=$$QTOPIA_DEPOT_PATH/help
help.files=btsettings*
help.hint=help
INSTALLS+=help
pics.files=$$QTOPIA_DEPOT_PATH/pics/btsettings/*
pics.path=/pics/btsettings
pics.hint=pics
INSTALLS+=pics
settings.files=$$QTOPIA_DEPOT_PATH/etc/default/Trolltech/BluetoothKnownHeadsets.conf
settings.path=/etc/default/Trolltech
INSTALLS+=settings

pkg.desc=Bluetooth Settings
pkg.domain=trusted
