TEMPLATE = lib

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols
contains(QT_CONFIG, debug):contains(QT_CONFIG, release):CONFIG += debug_and_release build_all
win32|mac:!macx-xcode:CONFIG += debug_and_release
contains(QT_CONFIG, embedded):CONFIG += embedded
!contains(CONFIG, static) {
	CONFIG += dll
	DEFINES += QT_SHARED
}

TARGET = QtDBus

CONFIG += create_prl link_pkgconfig
DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII QDBUS_MAKEDLL DBUS_API_SUBJECT_TO_CHANGE
QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS
LIBS += $$QT_LIBS_DBUS
QT = core xml

INCLUDEPATH += .

MOC_DIR         = tmp
DESTDIR = ../../../lib
DLLDESTDIR = ../../../bin
isEmpty(QT_MAJOR_VERSION) {
   VERSION=4.3.0
} else {
   VERSION=$${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION}.$${QT_PATCH_VERSION}
}
QMAKE_TARGET_COMPANY = Trolltech ASA
QMAKE_TARGET_PRODUCT = QtDBus
QMAKE_TARGET_DESCRIPTION = Qt DBus module
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2006 Trolltech ASA


unix {
   CONFIG += create_libtool create_pc explicitlib
   QMAKE_PKGCONFIG_LIBDIR = $$[QT_INSTALL_LIBS]
   QMAKE_PKGCONFIG_INCDIR = $$[QT_INSTALL_HEADERS]
   QMAKE_PKGCONFIG_DESCRIPTION = Qt DBus module
   QMAKE_PKGCONFIG_DESTDIR = pkgconfig
   QMAKE_PKGCONFIG_NAME = QtDBus
}

win32 {
   LIBS += -lws2_32 -ladvapi32 -lnetapi32 -luser32
   CONFIG(debug, debug|release):LIBS += -ldbus-1d
   else:LIBS += -ldbus-1
}

#load up the headers info
CONFIG += qt_install_headers
HEADERS_PRI = $$QT_BUILD_TREE/include/QtDBus/headers.pri
include($$HEADERS_PRI)|clear(HEADERS_PRI)

#mac frameworks
mac:!static:contains(QT_CONFIG, qt_framework) {
   QMAKE_FRAMEWORK_BUNDLE_NAME = $$TARGET
   CONFIG += lib_bundle qt_no_framework_direct_includes qt_framework
   CONFIG(debug, debug|release) {
      !build_pass:CONFIG += build_all
   } else { #release
      !debug_and_release|build_pass {
	  CONFIG -= qt_install_headers #no need to install these as well
	  FRAMEWORK_HEADERS.version = Versions
	  FRAMEWORK_HEADERS.files = $$SYNCQT.HEADER_FILES $$SYNCQT.HEADER_CLASSES
      	  FRAMEWORK_HEADERS.path = Headers
      }
      QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS
   }
}

TARGET = $$qtLibraryTarget($$TARGET) #done towards the end (after framework)

# Input

target.path=$$[QT_INSTALL_LIBS]

qt_install_headers {
    qtdbus_headers.files = $$SYNCQT.HEADER_FILES $$SYNCQT.HEADER_CLASSES
    qtdbus_headers.path = $$[QT_INSTALL_HEADERS]/QtDBus
    INSTALLS        += qtdbus_headers
}

INSTALL_HEADERS = $$PUB_HEADERS
targ_headers.files = $$INSTALL_HEADERS
targ_headers.path = $$[QT_INSTALL_HEADERS]/QtDBus
targ_pkgconfig.path = $$[QT_INSTALL_LIBS]/pkgconfig
target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target targ_headers targ_pkgconfig
win32 {
    dlltarget.path=$$[QT_INSTALL_BINS]
    INSTALLS += dlltarget
}

PUB_HEADERS =  qdbusargument.h \
	       qdbusconnectioninterface.h \
	       qdbusmacros.h \
	       qdbuserror.h \
	       qdbusextratypes.h \
	       qdbusmessage.h \
	       qdbusserver.h \
	       qdbusconnection.h \
	       qdbusabstractinterface.h \
	       qdbusinterface.h \
	       qdbusabstractadaptor.h \
	       qdbusreply.h \
	       qdbusmetatype.h \
	       qdbuscontext.h

HEADERS += $$PUB_HEADERS \
           qdbusconnection_p.h qdbusmessage_p.h \
           qdbusinterface_p.h qdbusxmlparser_p.h qdbusabstractadaptor_p.h \
           qdbusargument_p.h qdbusutil_p.h qdbusabstractinterface_p.h \
	   qdbuscontext_p.h qdbusthreaddebug_p.h qdbusintegrator_p.h

SOURCES += qdbusconnection.cpp  \
	qdbusconnectioninterface.cpp \
	qdbuserror.cpp		\
	qdbusintegrator.cpp	\
	qdbusmessage.cpp	\
	qdbusserver.cpp		\
	qdbusabstractinterface.cpp \
	qdbusinterface.cpp	\
	qdbusxmlparser.cpp	\
	qdbusutil.cpp		\
	qdbusintrospection.cpp	\
	qdbusabstractadaptor.cpp \
	qdbusthread.cpp \
	qdbusinternalfilters.cpp \
	qdbusmetaobject.cpp	\
	qdbusxmlgenerator.cpp	\
	qdbusmisc.cpp		\
	qdbusargument.cpp	\
	qdbusreply.cpp		\
	qdbusmetatype.cpp	\
	qdbusextratypes.cpp	\
	qdbusmarshaller.cpp	\
	qdbuscontext.cpp
