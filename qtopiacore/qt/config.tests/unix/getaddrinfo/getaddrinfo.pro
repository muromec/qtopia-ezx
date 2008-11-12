SOURCES = getaddrinfotest.cpp
CONFIG -= qt dylib
mac:CONFIG -= app_bundle
solaris-cc*:LIBS += -lsocket -lnsl
