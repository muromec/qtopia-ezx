!qbuild{
qtopia_project(qtopia lib)
TARGET=vobject
CONFIG+=staticlib no_tr
license(FREEWARE)
QMAKE_CFLAGS+=-fPIC
QMAKE_CXXFLAGS+=-fPIC
}

HEADERS+=\
    vcc_yacc_p.h\
    vobject_p.h\

SOURCES+=\
    vcc_yacc.cpp\
    vobject.cpp\

!qbuild{
idep(INCLUDEPATH+=$$PWD)
idep(LIBS+=-l$$TARGET)
}
