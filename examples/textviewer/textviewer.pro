qtopia_project(qtopia app) # see buildsystem.html for more project keywords
TARGET=textviewer
CONFIG+=qtopia_main no_singleexec no_quicklaunch no_tr

HEADERS=textviewer.h
SOURCES=main.cpp textviewer.cpp

desktop.files=textviewer.desktop
desktop.path=/apps/Applications
desktop.trtarget=textviewer-nct
desktop.hint=nct desktop
INSTALLS+=desktop

pkg.name=textviewer
pkg.desc=An Example Program to View Text Files
pkg.version=1.0.0-1
pkg.maintainer=Trolltech (www.trolltech.com)
pkg.license=GPL
pkg.domain=trusted
