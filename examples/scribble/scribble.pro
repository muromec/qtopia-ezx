qtopia_project(qtopia app)
TARGET=scribble
CONFIG+=qtopia_main no_quicklaunch

HEADERS       = mainwindow.h \
                scribblearea.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                scribblearea.cpp

desktop.files=scribble.desktop
desktop.path=/apps/Applications
desktop.hint=desktop

pics.files=pics/*
pics.path=/pics/scribble
pics.hint=pics

INSTALLS+=desktop pics

pkg.desc=Scribble Application
pkg.domain=trusted

