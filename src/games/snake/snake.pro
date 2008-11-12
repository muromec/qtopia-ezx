qtopia_project(qtopia app)
TARGET=snake
CONFIG+=qtopia_main no_quicklaunch


HEADERS = snake.h interface.h sprites.h
SOURCES = snake.cpp interface.cpp main.cpp sprites.cpp

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=snake*
help.hint=help
INSTALLS+=help
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Games/snake.desktop
desktop.path=/apps/Games
desktop.hint=desktop
INSTALLS+=desktop
pics.files=$$QTOPIA_DEPOT_PATH/pics/snake/*
pics.path=/pics/snake
pics.hint=pics
INSTALLS+=pics

pkg.desc=Try to keep the snake alive for as long as possible by eating mice and avoiding walls.
pkg.domain=trusted
