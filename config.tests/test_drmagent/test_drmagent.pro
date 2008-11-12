TEMPLATE=app
CONFIG-=qt
SOURCES=main.cpp
include(../locate_drmagent.pri)
LIBS+=$$DRMAGENT -lpthread
