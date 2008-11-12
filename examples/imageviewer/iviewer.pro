qtopia_project(qtopia app) 

TARGET = iviewer
CONFIG+=qtopia_main no_singleexec no_quicklaunch no_tr

HEADERS += iviewer.h 
SOURCES += main.cpp iviewer.cpp
SOURCES += listscreen.cpp
HEADERS += listscreen.h
SOURCES += imagescreen.cpp
HEADERS += imagescreen.h
SOURCES += inputdialog.cpp
HEADERS += inputdialog.h
SOURCES += infoscreen.cpp
HEADERS += infoscreen.h

desktop.files=imageviewer.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop

# Package information (used for make packages)
pkg.name=iviewer
pkg.desc=Image Viewer Application
#pkg.version=1.0.0-1
#pkg.maintainer=Trolltech (www.trolltech.com)
#pkg.license=Commercial
pkg.domain=trusted
