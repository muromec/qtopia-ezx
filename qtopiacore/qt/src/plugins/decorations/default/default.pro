TARGET	 = qdecorationdefault
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR	 = $$QT_BUILD_TREE/plugins/decorations

HEADERS		= ../../../gui/embedded/qdecorationdefault_qws.h
SOURCES		= main.cpp \
		  ../../../gui/embedded/qdecorationdefault_qws.cpp

target.path += $$[QT_INSTALL_PLUGINS]/decorations
INSTALLS += target
