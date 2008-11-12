TARGET	 = qgfxvnc
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR	 = $$QT_BUILD_TREE/plugins/gfxdrivers

DEFINES	+= QT_QWS_VNC

HEADERS = ../../../../src/gui/embedded/qscreenvnc_qws.h \
          ../../../../src/gui/embedded/qscreenvnc_p.h

SOURCES		= main.cpp \
		  ../../../../src/gui/embedded/qscreenvnc_qws.cpp


target.path += $$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS 	+= target
