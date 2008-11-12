/****************************************************************************
**
** This file is part of the Qtopia Opensource Edition Package.
**
** Copyright (C) 2008 Trolltech ASA.
**
** Contact: Qt Extended Information (info@qtextended.org)
**
** This file may be used under the terms of the GNU General Public License
** versions 2.0 as published by the Free Software Foundation and appearing
** in the file LICENSE.GPL included in the packaging of this file.
**
** Please review the following information to ensure GNU General Public
** Licensing requirements will be met:
**     http://www.fsf.org/licensing/licenses/info/GPLv2.html.
**
**
****************************************************************************/

#include <qtopiaabstractservice.h>
#include <qmetaobject.h>
#include <qtopialog.h>
#include <QByteArray>

#if defined(QTOPIA_DBUS_IPC)
#include <qdbusconnection.h>
#include <qdbusabstractadaptor.h>
#include <qdbuserror.h>
#include <qdbusconnectioninterface.h>
#include "dbusipccommon_p.h"
#else
#include <qtopiaipcadaptor.h>
#include <qtopiaservices.h>
#endif

/*!
    \class QtopiaAbstractService
    \mainclass
    \brief The QtopiaAbstractService class provides an interface to messages on a QCop service
    which simplifies remote slot invocations

    Service messages in Qtopia are sent with QtopiaServiceRequest.  They consist
    of a service name, a message name, and a list of parameter values.  Qtopia
    dispatches service messages to the applications associated with the service
    name, on the application's \c{QPE/Application/appname} channel, where
    \c{appname} is the application's name.

    Client applications can listen for service messages on the application
    channel directly, using QtopiaChannel, but it is cleaner and less error-prone
    to create an instance of QtopiaAbstractService instead.

    The use of QtopiaAbstractService will be demonstrated using the Qtopia \c{Time}
    service.  This has a single message called \c{editTime()} which asks
    the service to pop up a dialog allowing the user to edit the current time.

    \code
    class TimeService : public QtopiaAbstractService
    {
        Q_OBJECT
    public:
        TimeService( QObject *parent = 0 );

    public slots:
        void editTime();
    };

    TimeService::TimeService( QObject *parent )
        : QtopiaAbstractService( "Time", parent )
    {
        publishAll();
    }
    \endcode

    The call to publishAll() causes all public slots within \c{TimeService}
    to be automatically registered as Service messages.  This can be
    useful if the service has many message types.

    The client can send a request to the service using QtopiaServiceRequest:

    \code
    QtopiaServiceRequest req( "Time", "editTime()" );
    req.send();
    \endcode

    \sa QtopiaService, QtopiaIpcAdaptor, QtopiaIpcEnvelope, QtopiaServiceRequest
    \sa QtopiaChannel

    \ingroup ipc
*/

#if defined(QTOPIA_DBUS_IPC)
class QtopiaDBusAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.trolltech.qtopia")

public:
    QtopiaDBusAdaptor(QObject *parent);
    ~QtopiaDBusAdaptor();
};

QtopiaDBusAdaptor::QtopiaDBusAdaptor(QObject *parent) : QDBusAbstractAdaptor(parent)
{

}

QtopiaDBusAdaptor::~QtopiaDBusAdaptor()
{

}
#endif

#if !defined(QTOPIA_DBUS_IPC)
class ServiceQtopiaIpcAdaptorProxy : public QtopiaIpcAdaptor
{
    Q_OBJECT

public:
    ServiceQtopiaIpcAdaptorProxy(const QString &channel, QObject *parent=0);

    QString memberToMessage( const QByteArray& member );
    QStringList sendChannels( const QString& channel );
    QString receiveChannel( const QString& channel );

    QString m_channel;
};

ServiceQtopiaIpcAdaptorProxy::ServiceQtopiaIpcAdaptorProxy(const QString &channel, QObject *parent) :
        QtopiaIpcAdaptor(channel, parent), m_channel(channel)
{

}

QString ServiceQtopiaIpcAdaptorProxy::memberToMessage( const QByteArray& member )
{
    return m_channel + "::" + QtopiaIpcAdaptor::memberToMessage( member );
}

QStringList ServiceQtopiaIpcAdaptorProxy::sendChannels( const QString& channel )
{
    return QtopiaService::channels( channel );
}

QString ServiceQtopiaIpcAdaptorProxy::receiveChannel( const QString& )
{
    return QString();
}
#endif

class QtopiaAbstractService_Private
{
public:
    QtopiaAbstractService_Private(const QString &service);

#if !defined(QTOPIA_DBUS_IPC)
    QtopiaIpcAdaptor *m_copobject;
#endif

    QString m_service;
    bool m_publishAllCalled;
};

QtopiaAbstractService_Private::QtopiaAbstractService_Private(const QString &service) :
#if !defined(QTOPIA_DBUS_IPC)
        m_copobject(NULL),
#endif
        m_service(service),
        m_publishAllCalled(false)
{
#if !defined(QTOPIA_DBUS_IPC)
    m_copobject = new ServiceQtopiaIpcAdaptorProxy(service);
#endif
}

/*!
    Construct a remote service object for \a service and attach it to \a parent.
*/
QtopiaAbstractService::QtopiaAbstractService( const QString& service, QObject *parent )
    : QObject( parent )
{
    m_data = new QtopiaAbstractService_Private(service);
}

/*!
    Destroy this QCop service handling object.
*/
QtopiaAbstractService::~QtopiaAbstractService()
{
#if defined(QTOPIA_DBUS_IPC)
    QDBusConnection dbc = QDBus::sessionBus();
    if (!dbc.isConnected()) {
        qWarning() << "Unable to connect to D-BUS:" << dbc.lastError();
        return;
    }

    qLog(Services) << "Unregistering service" << m_data->m_service;

    QString path = dbusPathBase;
    path.append(m_data->m_service);
    dbc.unregisterObject(path);

    QDBusConnectionInterface *iface = dbc.interface();
    QString service = dbusInterface;
    service.append(".");
    service.append(m_data->m_service);

    iface->unregisterService(service);
#endif

    if (m_data)
        delete m_data;
}

/*!
    Publishes all slots on this object within subclasses of QtopiaAbstractService.
    This is typically called from a subclass constructor.
*/
void QtopiaAbstractService::publishAll()
{
#if defined(QTOPIA_DBUS_IPC)
    QDBusConnection dbc = QDBus::sessionBus();
    if (!dbc.isConnected()) {
        qWarning() << "Unable to connect to D-BUS:" << dbc.lastError();
        return;
    }

    qLog(Services) << "Registering service" << m_data->m_service;

    QDBusConnectionInterface *iface = dbc.interface();
    QString service = dbusInterface;
    service.append(".");
    service.append(m_data->m_service);
    if (iface->registerService(service) == QDBusConnectionInterface::ServiceNotRegistered) {
        qWarning() << "WARNING: could not request name" << service;
        return;
    }

    new QtopiaDBusAdaptor(this);
    QString path = dbusPathBase;
    path.append(m_data->m_service);
    dbc.registerObject(path, this, QDBusConnection::ExportNonScriptableSlots);
#else
    const QMetaObject *meta = metaObject();
    if ( !m_data->m_publishAllCalled ) {
        int count = meta->methodCount();
        int index = QtopiaAbstractService::staticMetaObject.methodCount();
        for ( ; index < count; ++index ) {
            QMetaMethod method = meta->method( index );
            if ( method.methodType() == QMetaMethod::Slot &&
                 method.access() == QMetaMethod::Public) {
                QByteArray name = method.signature();
                QtopiaIpcAdaptor::connect(m_data->m_copobject, "3" + name, this, "1" + name);
            }
        }
        m_data->m_publishAllCalled = true;
    }
#endif
}

#include "qtopiaabstractservice.moc"
