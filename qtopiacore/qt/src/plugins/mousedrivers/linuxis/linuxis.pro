TARGET = linuxismousehandler
include(../../qpluginbase.pri)

target.path = $$[QT_INSTALL_PLUGINS]/mousedrivers
INSTALLS += target

HEADERS = linuxismousedriverplugin.h linuxismousehandler.h
SOURCES = linuxismousedriverplugin.cpp linuxismousehandler.cpp

