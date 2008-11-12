qtopia_project(qtopia app)
TARGET=minesweep
CONFIG+=qtopia_main no_quicklaunch

HEADERS		= minefield.h \
                  minesweep.h
SOURCES		= minefield.cpp \
		  minesweep.cpp \
		  main.cpp

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=minesweep.html
help.hint=help
INSTALLS+=help
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Games/minesweep.desktop
desktop.path=/apps/Games
desktop.hint=desktop
INSTALLS+=desktop
pics.files=$$QTOPIA_DEPOT_PATH/pics/minesweep/*
pics.path=/pics/minesweep
pics.hint=pics
INSTALLS+=pics

pkg.description=A minesweeper game for Qtopia
pkg.domain=trusted
