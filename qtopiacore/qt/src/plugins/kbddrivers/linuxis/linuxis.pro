TARGET = linuxiskbdhandler
include(../../qpluginbase.pri)

target.path = $$[QT_INSTALL_PLUGINS]/kbddrivers
INSTALLS += target

CONFIG+=no_tr

HEADERS = linuxiskbddriverplugin.h linuxiskbdhandler.h
SOURCES = linuxiskbddriverplugin.cpp linuxiskbdhandler.cpp
