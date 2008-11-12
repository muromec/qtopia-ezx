qtopia_project(qtopia app)
TARGET=bluetooth
CONFIG+=qtopia_main

HEADERS		= dirdeleterdialog.h \
                  mainwindow.h
SOURCES		= mainwindow.cpp \
                  dirdeleterdialog.cpp \
                  main.cpp

desktop.files=$$QTOPIA_DEPOT_PATH/apps/Applications/bluetooth.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=bluetooth*
help.hint=help
INSTALLS+=help

depends(libraries/qtopiacomm)
depends(libraries/qtopiabase)

pkg.desc=A Bluetooth Applications Suite.
pkg.domain=trusted
