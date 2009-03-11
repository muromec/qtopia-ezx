qtopia_project(qtopia core lib)
TARGET          =   QtDBus
CONFIG+=no_tr

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII QDBUS_MAKEDLL DBUS_API_SUBJECT_TO_CHANGE

depends(libraries/qt/xml)
depends(3rdparty/libraries/dbus)

INCLUDEPATH += $$QTE_DEPOT_PATH/src/dbus
QMAKE_CXXFLAGS+=$$DBUS_CFLAGS

VPATH += $$QTE_DEPOT_PATH/tools/qdbus/src

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
	       qdbuspendingcall.h \
	       qdbuspendingreply.h \
	       qdbuscontext.h

HEADERS += $$PUB_HEADERS \
           qdbusconnection_p.h qdbusmessage_p.h \
           qdbusinterface_p.h qdbusxmlparser_p.h qdbusabstractadaptor_p.h \
           qdbusargument_p.h qdbusutil_p.h qdbusabstractinterface_p.h \
	   qdbuscontext_p.h qdbusthreaddebug_p.h qdbusintegrator_p.h \
	   qdbuspendingcall_p.h qdbus_symbols_p.h

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
	qdbuscontext.cpp        \
	qdbuspendingcall.cpp	\
	qdbuspendingreply.cpp	\
	qdbus_symbols.cpp

sdk_headers.files=$${PUB_HEADERS}
sdk_headers.path=/include/qtdbus
sdk_headers.hint=sdk headers
INSTALLS+=sdk_headers

CONFIG+=qtopia_visibility

idep(LIBS+=-l$$TARGET)
qt_inc($$TARGET)

VERSION=$${QTE_VERSION}
