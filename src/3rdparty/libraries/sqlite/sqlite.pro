build_qtopia_sqlite {
qtopia_project(external lib)
license(FREEWARE)
TARGET=qtopia-sqlite
CONFIG+=syncqtopia
CONFIG(release,debug|release):DEFINES*=NDEBUG
DEFINES+=SQLITE_OMIT_LOAD_EXTENSION
# QMAKE_CFLAGS+=-O3
# QMAKE_CXXFLAGS+=-O3

sourcedir=$$QTE_DEPOT_PATH/src/3rdparty/sqlite
INCLUDEPATH += $$sourcedir
VPATH += $$sourcedir

SOURCES += sqlite3.c

HEADERS+= sqlite3.h

# I'd do a depend but we only want defines, not symbols
INCLUDEPATH+=$$QTDIR/include/QtCore

sdk_headers.files=$$HEADERS
sdk_headers.path=/include/sqlite
sdk_headers.hint=non_qt_headers
INSTALLS+=sdk_headers

idep(LIBS+=-l$$TARGET)
qt_inc(sqlite)
} else {
qtopia_project(stub)
idep(LIBS+=-lsqlite3)
}
