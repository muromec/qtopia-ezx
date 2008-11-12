qtopia_project(qtopia lib)
TARGET=qdsync_common
CONFIG+=no_tr
# Packaged by tools/qdsync/app
CONFIG+=no_pkg

include(common.pri)
PREFIX=QTOPIADESKTOP
resolve_include()

HEADERS+=\
    qtopia4sync.h\

SOURCES+=\
    qtopia4sync.cpp\

idep(LIBS+=-l$$TARGET)
idep(INCLUDEPATH+=$$PWD $$PWD/..,INCLUDEPATH)
