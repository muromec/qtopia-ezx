qtopia_project(qtopia app)
TARGET=quicklauncher
CONFIG+=no_tr
CONFIG+=singleexec_main

requires(!equals(LAUNCH_METHOD,normal))
depends(libraries/qtopiapim)

equals(LAUNCH_METHOD,quicklaunch) {
    HEADERS+=quicklaunch.h
    SOURCES+=quicklaunch.cpp
} else {
    HEADERS+=quicklaunchforked.h
    SOURCES+=quicklaunchforked.cpp
    DEFINES+=QUICKLAUNCHER_FORKED
}

equals(QTOPIA_SETPROC_METHOD,prctl):DEFINES+=QTOPIA_SETPROC_PRCTL
equals(QTOPIA_SETPROC_METHOD,argv0):DEFINES+=QTOPIA_SETPROC_ARGV0

SOURCES+=main.cpp

pkg.desc=Quicklauncher stub for quicklaunch enabled applications.
pkg.domain=trusted
