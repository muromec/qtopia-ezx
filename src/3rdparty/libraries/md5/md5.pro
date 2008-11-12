qtopia_project(embedded lib)
license(BSD GPL_COMPATIBLE)
TARGET = md5
QT=core
CONFIG+=no_tr

VPATH+=$$QT_DEPOT_PATH/src/3rdparty/md5
INCLUDEPATH+=$$QT_DEPOT_PATH/src/3rdparty/md5

SOURCES+=md5.cpp md5hash.cpp
HEADERS+=md5.h md5hash.h md5hashglobal.h

dep(INCLUDEPATH+=$$PWD)
idep(LIBS+=-l$$TARGET)
