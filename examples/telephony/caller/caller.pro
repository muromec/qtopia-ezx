qtopia_project(qtopia app) # see buildsystem.html for more project keywords
TARGET=caller
CONFIG+=qtopia_main no_singleexec no_quicklaunch no_tr

FORMS  =caller.ui
HEADERS=caller.h
SOURCES=main.cpp caller.cpp

depends(libraries/qtopiaphone)

pkg.name=caller
pkg.desc=Caller Demo Application
pkg.version=1.0.0-1
pkg.maintainer=Trolltech (www.trolltech.com)
pkg.license=Commercial
pkg.domain=trusted
