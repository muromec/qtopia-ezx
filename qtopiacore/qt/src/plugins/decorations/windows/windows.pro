TARGET	 = qdecorationwindows
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR	 = $$QT_BUILD_TREE/plugins/decorations

HEADERS		= ../../../gui/embedded/qdecorationwindows_qws.h
SOURCES		= main.cpp \
		  ../../../gui/embedded/qdecorationwindows_qws.cpp

target.path += $$[QT_INSTALL_PLUGINS]/decorations
INSTALLS += target
