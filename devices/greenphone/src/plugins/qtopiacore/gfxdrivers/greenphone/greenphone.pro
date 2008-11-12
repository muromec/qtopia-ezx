qtopia_project(embedded qtopia core plugin)
TARGET = greenphonescreen

CONFIG += no_tr

INCLUDEPATH += ../../../../../../../include/
LIBS += -L../../../../../../../lib -lblend
HEADERS += greenphonescreendriverplugin.h greenphonescreen.h
SOURCES += greenphonescreendriverplugin.cpp greenphonescreen.cpp

