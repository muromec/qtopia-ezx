qtopia_project(qtopia app) # see buildsystem.html for more project keywords
TARGET=notesdemo
CONFIG+=qtopia_main no_singleexec no_quicklaunch no_tr

HEADERS=notesdemo.h
SOURCES=main.cpp notesdemo.cpp

desktop.files=notesdemo.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop

pics.files=pics/*
pics.path=/pics/notesdemo
pics.hint=pics
INSTALLS+=pics

help.source=help
help.files=notesdemo.html
help.hint=help
INSTALLS+=help


pkg.name=notesdemo
pkg.desc=NotesDemo Application
pkg.version=1.0.0-1
pkg.maintainer=Trolltech (www.trolltech.com)
pkg.license=Commercial
pkg.domain=trusted
