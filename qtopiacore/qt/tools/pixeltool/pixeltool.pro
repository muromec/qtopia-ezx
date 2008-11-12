TEMPLATE = app
CONFIG  += qt assistant warn_on
QT += network

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

DESTDIR     = ../../bin

DEPENDPATH += .
INCLUDEPATH += .
TARGET = pixeltool

# Input
SOURCES += main.cpp qpixeltool.cpp
HEADERS += qpixeltool.h

target.path=$$[QT_INSTALL_BINS]
INSTALLS += target
