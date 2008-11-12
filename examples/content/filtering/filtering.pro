qtopia_project(qtopia app)
TARGET=filtering
CONFIG+=no_tr

# Input
HEADERS += filterdemo.h
SOURCES += filterdemo.cpp main.cpp

desktop.files=filtering.desktop
desktop.path=/apps/Applications
desktop.trtarget=filtering-nct
desktop.hint=nct desktop
INSTALLS+=desktop

pics.files=pics/*
pics.path=/pics/filtering
pics.hint=pics
INSTALLS+=pics

help.source=help
help.files=filtering.html
help.hint=help
INSTALLS+=help

# SXE permissions required
pkg.domain=trusted
pkg.name=filtering
pkg.desc=This is a command line tool used to demonstrate database access and filtering
pkg.version=1.0.0-1
pkg.maintainer=Trolltech (www.trolltech.com)
pkg.license=GPL
