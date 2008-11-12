qtopia_project(qtopia app)
TARGET		= dualdisplaybasic
CONFIG+=qtopia_main no_quicklaunch

HEADERS		= display.h
SOURCES		= main.cpp display.cpp

depends(libraries/qtopiaphone)

pkg.desc=Example phone status display
pkg.domain=trusted
