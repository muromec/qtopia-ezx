TARGET=getifaddrs
CONFIG -= qt
mac:CONFIG -= app_bundle
QT =
TEMPLATE=app
SOURCES=getifaddrs.cpp
LIBS += $$QMAKE_LIBS_NETWORK

