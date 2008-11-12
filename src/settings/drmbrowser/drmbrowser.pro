qtopia_project(qtopia app)
TARGET=drmbrowser
CONFIG+=qtopia_main

HEADERS		= drmbrowser.h

SOURCES		= main.cpp \
                  drmbrowser.cpp

desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/drmbrowser.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=drmbrowser*
help.hint=help
INSTALLS+=help

omadrmagentservice.files=$$QTOPIA_DEPOT_PATH/services/OmaDrmAgent/drmbrowser
omadrmagentservice.path=/services/OmaDrmAgent
INSTALLS+=omadrmagentservice

qdsservice.files=$$QTOPIA_DEPOT_PATH/etc/qds/OmaDrmAgent
qdsservice.path=/etc/qds
INSTALLS+=qdsservice


depends(libraries/qtopia)

phone {
    depends(libraries/qtopiaphone)
}

pkg.desc=DRM information for Qtopia.
pkg.domain=trusted
