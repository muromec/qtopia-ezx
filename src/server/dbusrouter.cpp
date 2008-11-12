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

#if defined(QTOPIA_DBUS_IPC)
#include <qtopia/private/dbusapplicationchannel_p.h>
#include <qtdbus/qdbusconnection.h>
#include <qtdbus/qdbuserror.h>
#include <qtdbus/qdbusmessage.h>
#include <qtdbus/qdbusconnectioninterface.h>
#include <qtdbus/qdbusabstractadaptor.h>
#include <qtopia/private/dbusipccommon_p.h>
#include "dbusrouter.h"

#include <QDebug>

class DBusLauncherAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.trolltech.qtopia.DBusLauncher")
public:

    DBusLauncherAdaptor(QObject *parent);
};

DBusLauncherAdaptor::DBusLauncherAdaptor(QObject *parent) : QDBusAbstractAdaptor(parent)
{

}

class DBusLauncherService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.trolltech.qtopia.DBusLauncher")

public:
    DBusLauncherService(QObject *parent = 0);
    ~DBusLauncherService();

public slots:
    Q_SCRIPTABLE void launch(const QString &app);

private slots:
    void terminated(const QString &app, ApplicationTypeLauncher::TerminationReason, bool filtered);
    void applicationStateChanged(const QString &app, ApplicationTypeLauncher::ApplicationState);

private:
    void sendLaunched(const QString &app);
    QSet<QString> m_pending;
};

DBusLauncherService::DBusLauncherService(QObject *parent) : QObject(parent)
{
}

DBusLauncherService::~DBusLauncherService()
{

}

void DBusLauncherService::sendLaunched(const QString &app)
{
    m_pending.remove(app);

    QDBusConnection dbc = QDBus::sessionBus();

    QDBusMessage message = QDBusMessage::signal("/DBusLauncher",
            "com.trolltech.qtopia.DBusLauncher", "launched", dbc);

    message << app;

    bool ret = dbc.send(message);
    if (!ret)
        qWarning("Unable to send message: %s", dbc.lastError().message().toAscii().constData());
}

void DBusLauncherService::applicationStateChanged(const QString &app,
                                 ApplicationTypeLauncher::ApplicationState state)
{
    if (!m_pending.contains(app)) {
        return;
    }

    if (state != ApplicationTypeLauncher::Running) {
        return;
    }

    sendLaunched(app);
}

void DBusLauncherService::terminated(const QString &app,
                    ApplicationTypeLauncher::TerminationReason reason, bool)
{
    if (!m_pending.contains(app)) {
        return;
    }

    sendLaunched(app);
}

void DBusLauncherService::launch(const QString &app)
{
    // Launch route
    ApplicationLauncher *l = qtopiaTask<ApplicationLauncher>();
    if (l) {
        m_pending.insert(app);
        connect(l, SIGNAL(applicationStateChanged(QString,ApplicationTypeLauncher::ApplicationState)),
                this, SLOT(applicationStateChanged(QString,ApplicationTypeLauncher::ApplicationState)));
        connect(l, SIGNAL(applicationTerminated(QString,ApplicationTypeLauncher::TerminationReason,bool)),
                this, SLOT(terminated(QString,ApplicationTypeLauncher::TerminationReason,bool)));

        l->launch(app);
    }
}

/*!
    \class DBusRouter
    \ingroup QtopiaServer::Task
    \brief The DBusRouter launches DBus applications and routes Application Messages
    \internal

    \bold {NOTE:} This class is not used.  It should be considered
    for removal.  The configure option that enables this class
    has been removed in Qtopia 4.2 series.

    For ease of replacability with QCopRouter, the DBusRouter
    task installs itself as an IpcRouter.
  
    This class is part of the Qtopia server and cannot be used by other Qtopia applications.
 */

QTOPIA_TASK(IpcRouter, DBusRouter);
QTOPIA_TASK_PROVIDES(IpcRouter, ApplicationIpcRouter);

/*!
    \internal
*/
DBusRouter::DBusRouter()
{
    DBusLauncherService *service = new DBusLauncherService(this);
    DBusLauncherAdaptor *adaptor = new DBusLauncherAdaptor(service);

    QDBusConnection dbc = QDBus::sessionBus();
    if (!dbc.isConnected()) {
        qWarning() << "Unable to connect to D-BUS:" << dbc.lastError();
        return;
    }

    QDBusConnectionInterface *iface = dbc.interface();

    if (iface->registerService("com.trolltech.qtopia.DBusLauncher") ==
        QDBusConnectionInterface::ServiceNotRegistered) {
        qWarning() << "WARNING: Could not register DBusLauncher service!!!";
        return;
    }

    bool succ = dbc.registerObject("/DBusLauncher", service, QDBusConnection::ExportSlots);
}

/*!
    \internal
*/
DBusRouter::~DBusRouter()
{
    foreach (DBUSQtopiaApplicationChannel *channel, m_channels) {
        delete channel;
    }
}

/*!
    \internal
*/
// Process all messages to QPE/Application/*.
void DBusRouter::applicationMessage
        ( const QString& msg, const QByteArray& data )
{
    QObject *s = sender();

    if (!s)
        return;

    DBUSQtopiaApplicationChannel *channel = qobject_cast<DBUSQtopiaApplicationChannel *>(s);
    if (!channel)
        return;

    routeMessage(channel->appName(), msg, data);
}

/*!
    \internal
*/
void DBusRouter::routeMessage(const QString &dest,
                              const QString &message,
                              const QByteArray &data)
{
    // Launch route
    ApplicationLauncher *l = qtopiaTask<ApplicationLauncher>();
    if(l)
        l->launch(dest);

    QMultiMap<QString, RouteDestination *>::Iterator iter = m_routes.find(dest);
    while(iter != m_routes.end() && iter.key() == dest) {
        (*iter)->routeMessage(dest, message, data);
        ++iter;
    }
}

/*!
    \internal
*/
void DBusRouter::addRoute(const QString &app, RouteDestination *dest)
{
    Q_ASSERT(dest);
    m_routes.insert(app, dest);

    if (!m_channels.contains(app)) {
        DBUSQtopiaApplicationChannel *channel = new DBUSQtopiaApplicationChannel(app, this);
        connect(channel, SIGNAL(received(QString,QByteArray)),
                this, SLOT(applicationMessage(QString,QByteArray)));
        m_channels.insert(app, channel);
    }
}

/*!
    \internal
*/
void DBusRouter::remRoute(const QString &app, RouteDestination *dest)
{
    Q_ASSERT(dest);
    QMultiMap<QString, RouteDestination *>::Iterator iter = m_routes.find(app);
    while(iter != m_routes.end() && iter.key() == app) {
        if(iter.value() == dest) {
            m_routes.erase(iter);
            return;
        }
        ++iter;
    }

    if (!m_routes.contains(app)) {
        QMap<QString, DBUSQtopiaApplicationChannel *>::Iterator iter = m_channels.find(app);
        delete iter.value();
        m_channels.erase(iter);
    }
}

#include "dbusrouter.moc"

#endif
