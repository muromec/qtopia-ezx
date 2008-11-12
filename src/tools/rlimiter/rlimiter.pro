qtopia_project( external app )

CONFIG-=qt
SOURCES=main.c
TARGET=rlimiter
LIBS=-lc

QMAKE_LIBS_DYNLOAD=
QMAKE_CFLAGS_RELEASE=-Os
QMAKE_LFLAGS_RELEASE=-nodefaultlibs
