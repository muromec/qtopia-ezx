TEMPLATE = lib
CONFIG += plugin
QT += opengl

LIBS += 

TARGET = hybridscreen
target.path = $$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS += target

HEADERS	= hybridscreen.h \
          hybridsurface.h
SOURCES	= hybridscreen.cpp \
          hybridsurface.cpp \
          hybridplugin.cpp

