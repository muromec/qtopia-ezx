qtopia_project(external lib)
license(LGPL)
TARGET=openobex
CONFIG-=warn_on

DEFINES+=HAVE_CONFIG_H
phone:enable_bluetooth:DEFINES+=HAVE_BLUETOOTH=1

HEADERS = config.h \
    btobex.h \
    inobex.h \
    irda.h \
    irda_wrap.h \
    irobex.h \
    databuffer.h \
    openobex/obex.h \
    obex_client.h \
    obex_connect.h \
    openobex/obex_const.h \
    obex_header.h \
    obex_main.h \
    obex_object.h \
    obex_server.h \
    obex_transport.h


SOURCES = btobex.c \
    inobex.c \
    irobex.c \
    databuffer.c \
    obex.c \
    obex_client.c \
    obex_connect.c \
    obex_header.c \
    obex_main.c \
    obex_object.c \
    obex_server.c \
    obex_transport.c

win32:SOURCES += win32compat.c

# FIXME "make syncqtopia"
dep(INCLUDEPATH+=$$PWD)
idep(LIBS+=-l$$TARGET)

