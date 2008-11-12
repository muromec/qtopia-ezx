TARGET	 = qgfxtransformed
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR	 = $$QT_BUILD_TREE/plugins/gfxdrivers

DEFINES	+= QT_QWS_TRANSFORMED

HEADERS		= ../../../../include/Qt/qscreentransformed_qws.h
SOURCES		= main.cpp \
		  ../../../../src/gui/embedded/qscreentransformed_qws.cpp


target.path=$$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS += target
