TARGET	 = qtslibmousedriver
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR	 = $$QT_BUILD_TREE/plugins/mousedrivers

HEADERS		= ../../../gui/embedded/qmousedriverplugin_qws.h \
		  ../../../gui/embedded/qmousetslib_qws.h
SOURCES		= main.cpp \
		  ../../../gui/embedded/qmousetslib_qws.cpp

target.path += $$[QT_INSTALL_PLUGINS]/mousedrivers
INSTALLS += target

