qtopia_project(external lib)
license(BSD GPL_COMPATIBLE)
TARGET		=   tar
VERSION		=   1.1.2
CONFIG+=staticlib
equals(arch,x86_64):QMAKE_CFLAGS+=-fPIC
CONFIG -= warn_on

HEADERS		=   libtar.h libtar_listhash.h
SOURCES		=   append.c \
    block.c \
    decode.c \
    encode.c \
    extract.c \
    handle.c \
    libtar_hash.c \
    libtar_list.c \
    output.c \
    strlcat.c \
    strlcpy.c \
    strmode.c \
    util.c \
    wrapper.c

DEFINES    +=  HAVE_LCHOWN HAVE_STRFTIME

pkg.desc=tar library

# FIXME "make syncqtopia"
dep(INCLUDEPATH+=$$PWD)
idep(LIBS+=-l$$TARGET)
