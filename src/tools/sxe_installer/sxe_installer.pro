qtopia_project(desktop app)
TARGET=sxe_installer
CONFIG+=no_install no_tr

DEFINES+=SXE_INSTALLER

SOURCES	= main.cpp

VPATH+=$$QTE_DEPOT_PATH/src/gui/embedded
INCLUDEPATH+=$$QTE_DEPOT_PATH/src/gui/embedded

### Also needs functionality from the package manager
VPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiasecurity
INCLUDEPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiasecurity

### Also needs qLog
VPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiabase
INCLUDEPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiabase
INCLUDEPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiail/framework

SOURCES	+= keyfiler.cpp qpackageregistry.cpp qsxepolicy.cpp qtopialog.cpp qlog.cpp qtopiasxe.cpp qtopianamespace.cpp
HEADERS += qtopiasxe.h keyfiler_p.h qpackageregistry.h qsxepolicy.h qtopialog.h qlog.h qtopianamespace.h
