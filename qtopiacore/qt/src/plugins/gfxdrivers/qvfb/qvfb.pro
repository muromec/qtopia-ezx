TARGET	 = qscreenvfb
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR	 = $$QT_BUILD_TREE/plugins/gfxdrivers

DEFINES	+= QT_QWS_QVFB

HEADERS		= ../../../../include/Qt/qscreenvfb_qws.h
SOURCES		= main.cpp \
		  ../../../../src/gui/embedded/qscreenvfb_qws.cpp


target.path += $$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS += target
