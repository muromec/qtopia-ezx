qtopia_project(qtopia lib)
TARGET=mylib

HEADERS=mylib.h
SOURCES=mylib.cpp

idep(INCLUDEPATH+=$$PWD)
idep(LIBS+=-L$$PWD -l$$TARGET)

