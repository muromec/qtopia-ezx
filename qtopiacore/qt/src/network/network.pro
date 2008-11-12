TARGET   = QtNetwork
QPRO_PWD = $$PWD
DEFINES += QT_BUILD_NETWORK_LIB
QT = core
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x64000000

include(../qbase.pri)

# Qt network module

PRECOMPILED_HEADER = ../corelib/global/qt_pch.h
HEADERS += qauthenticator.h \
	   qauthenticator_p.h \
	   qftp.h \
           qhttp.h \
           qhostaddress.h \
           qabstractsocketengine_p.h \
           qnativesocketengine_p.h \
           qhttpsocketengine_p.h \
           qsocks5socketengine_p.h \
           qabstractsocket.h \
           qabstractsocket_p.h \
           qtcpsocket.h \
           qudpsocket.h \
           qtcpserver.h \
           qhostinfo.h \
           qhostinfo_p.h \
           qurlinfo.h \
           qnetworkproxy.h \
	   qnetworkinterface.h \
	   qnetworkinterface_p.h

SOURCES	= qauthenticator.cpp \
          qftp.cpp \
          qhttp.cpp \
          qhostaddress.cpp \
          qabstractsocketengine.cpp \
          qnativesocketengine.cpp \
          qhttpsocketengine.cpp \
          qsocks5socketengine.cpp \
          qabstractsocket.cpp \
          qtcpsocket.cpp \
          qudpsocket.cpp \
          qtcpserver.cpp \
          qhostinfo.cpp \
          qurlinfo.cpp \
          qnetworkproxy.cpp \
	  qnetworkinterface.cpp

unix:SOURCES += qhostinfo_unix.cpp qnativesocketengine_unix.cpp qnetworkinterface_unix.cpp
win32:SOURCES += qhostinfo_win.cpp qnativesocketengine_win.cpp qnetworkinterface_win.cpp

# OpenSSL support; compile in QSslSocket.
contains(QT_CONFIG, openssl) {
    include($$QT_SOURCE_TREE/config.tests/unix/openssl/openssl.pri)

    HEADERS += qssl.h \
               qsslcertificate.h \
               qsslcertificate_p.h \
               qsslcipher.h \
               qsslcipher_p.h \
               qsslerror.h \
               qsslkey.h \
               qsslsocket.h \
               qsslsocket_openssl_p.h \
               qsslsocket_openssl_symbols_p.h \
               qsslsocket_p.h
    SOURCES += qssl.cpp \
               qsslcertificate.cpp \
               qsslcipher.cpp \
               qsslerror.cpp \
               qsslkey.cpp \
               qsslsocket.cpp \
               qsslsocket_openssl.cpp \
               qsslsocket_openssl_symbols.cpp

    # Only for static builds, we link against OpenSSL.
    contains(CONFIG, static) {
        win32:LIBS += -lssleay32 -llibeay32
        unix:LIBS += -lssl -lcrypto
    }

    # Include Qt's default CA bundle
    RESOURCES += network.qrc
}

mac:INCLUDEPATH += ../3rdparty/dlcompat #qdns.cpp uses it (on Jaguar)

QMAKE_LIBS += $$QMAKE_LIBS_NETWORK
