qtopia_project(desktop app)
TARGET=qdawggen
CONFIG+=no_tr no_install

VPATH           += $${QTOPIA_DEPOT_PATH}/src/libraries/qtopiabase
INCLUDEPATH     += $${QTOPIA_DEPOT_PATH}/src/libraries/qtopiabase
# Dodgy, but get us the Qtopia includes
qt_inc(qtopia)
HEADERS		= qdawg.h qmemoryfile_p.h global.h
SOURCES		= main.cpp qdawg.cpp qmemoryfile.cpp global.cpp

unix:SOURCES    += qmemoryfile_unix.cpp
    
win32:SOURCES    += qmemoryfile_win.cpp
pkg.desc=QDawg file generator
pkg.domain=trusted
