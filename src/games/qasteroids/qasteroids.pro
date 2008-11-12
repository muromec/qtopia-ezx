qtopia_project(qtopia app)
TARGET=qasteroids
CONFIG+=qtopia_main no_quicklaunch

HEADERS		= ledmeter.h sprites.h toplevel.h view.h
SOURCES		= ledmeter.cpp sprites.cpp toplevel.cpp view.cpp main.cpp

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=qasteroids*
help.hint=help
INSTALLS+=help
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Games/qasteroids.desktop
desktop.path=/apps/Games
desktop.hint=desktop
INSTALLS+=desktop
pics.files=$$QTOPIA_DEPOT_PATH/pics/qasteroids/*
pics.path=/pics/qasteroids
pics.hint=pics
INSTALLS+=pics
sounds.files=$$QTOPIA_DEPOT_PATH/sounds/qasteroids/*
sounds.path=/sounds/qasteroids
INSTALLS+=sounds
pkg.domain=trusted

