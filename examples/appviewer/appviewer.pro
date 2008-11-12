qtopia_project(qtopia app) # see buildsystem.html for more project keywords
TARGET=appviewer
CONFIG+=qtopia_main no_singleexec no_quicklaunch

HEADERS=appviewer.h
SOURCES=main.cpp appviewer.cpp

desktop.files=appviewer.desktop
desktop.path=/apps/Applications
desktop.trtarget=textviewer-nct
desktop.hint=nct desktop
INSTALLS+=desktop

pkg.name=appviewer
pkg.desc=An Example Program to use the Content system and System paths
pkg.version=1.0.0-1
pkg.maintainer=Trolltech (www.trolltech.com)
pkg.license=GPL
pkg.domain=trusted
