!qbuild{
qtopia_project(qtopia lib)
TARGET=g711
CONFIG+=staticlib no_tr
license(FREEWARE)
}

HEADERS+=\
    g711.h\

SOURCES+=\
    g711.c\

!qbuild{
idep(INCLUDEPATH+=$$PWD)
idep(LIBS+=-l$$TARGET)
}
