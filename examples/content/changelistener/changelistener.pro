qtopia_project(qtopia app) # see buildsystem.html for more project keywords
TARGET=changelistener
CONFIG+=qtopia_main no_singleexec no_quicklaunch no_tr

HEADERS=changelistener.h
SOURCES=main.cpp changelistener.cpp

desktop.files=changelistener.desktop
desktop.path=/apps/Applications
desktop.trtarget=notesdemo-nct
desktop.hint=nct desktop
INSTALLS+=desktop

pics.files=pics/*
pics.path=/pics/changelistener
pics.hint=pics
INSTALLS+=pics

help.source=help
help.files=changelistener.html
help.hint=help
INSTALLS+=help


pkg.name=changelistener
pkg.desc=ChangeListener Application
pkg.version=1.0.0-1
pkg.maintainer=Trolltech (www.trolltech.com)
pkg.license=Commercial
pkg.domain=trusted
