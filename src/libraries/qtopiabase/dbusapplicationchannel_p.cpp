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

#include "dbusapplicationchannel_p.h"
#include <qtdbus/qdbusconnection.h>
#include <qtdbus/qdbuserror.h>
#include <qtdbus/qdbusmessage.h>
#include <qtdbus/qdbusconnectioninterface.h>
#include <qtdbus/qdbusabstractadaptor.h>
#include "dbusipccommon_p.h"

#include <qtopialog.h>
#include <QFile>

class DBUSQtopiaApplicationChannelAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.trolltech.qtopia")

public:
    DBUSQtopiaApplicationChannelAdaptor(QObject *parent);
};

DBUSQtopiaApplicationChannelAdaptor::DBUSQtopiaApplicationChannelAdaptor(QObject *parent) :
         QDBusAbstractAdaptor(parent)
{

}

/*!
    \class DBUSQtopiaApplicationChannel
    \brief The DBUSQtopiaApplicationChannel class implements Qtopia application channels over DBUS.

    \internal

    This class is an implementation detail and should not be used.

    It is exported for use by libqtopia and the qpe server.
*/

DBUSQtopiaApplicationChannel::DBUSQtopiaApplicationChannel(const QString &appName,
                                                           QObject *parent) :
        QObject(parent),
        m_serviceName("com.trolltech.qtopia.QPE.Application."),
        m_appName(appName)
{
    m_serviceName.append(appName);
    DBUSQtopiaApplicationChannelAdaptor *adaptor =
            new DBUSQtopiaApplicationChannelAdaptor(this);

    QDBusConnection dbc = QDBus::sessionBus();
    if (!dbc.isConnected()) {
        qWarning() << "Unable to connect to D-BUS:" << dbc.lastError();
        return;
    }

    QDBusConnectionInterface *iface = dbc.interface();

    if (iface->registerService(m_serviceName) ==
        QDBusConnectionInterface::ServiceNotRegistered) {
        qWarning() << "WARNING: Could not register QtopiaApplicationChannel service" << m_serviceName;
        return;
    }

    QString dbusPath("/com/trolltech/qtopia/QPE/Application/");
    dbusPath.append(appName);

    qLog(DBUS) << "Registering applicationChannel service:" << appName << "and Path" << dbusPath;

    bool succ = dbc.registerObject(dbusPath, this, QDBusConnection::ExportSlots);
    if (!succ) {
        qLog(DBUS) << dbc.lastError();
    }
}

/*!
    Destructor.
*/
DBUSQtopiaApplicationChannel::~DBUSQtopiaApplicationChannel()
{
    QDBusConnection dbc = QDBus::sessionBus();
    if (!dbc.isConnected())
        return;

    QDBusConnectionInterface *iface = dbc.interface();

    qLog(DBUS) << "Unregistering from:" << m_serviceName;
    iface->unregisterService(m_serviceName);
}

/*!
    Implement the appMessage.
*/
void DBUSQtopiaApplicationChannel::appMessage(const QString &msg, const QByteArray &data)
{
    QFile f("/tmp/foobar");
    f.open(QIODevice::WriteOnly);
    QDataStream stream(&f);
    stream << "DBUSQtopiaApplicationChannel Received message:" << msg;
    f.close();
    qLog(DBUS) << "DBUSQtopiaApplicationChannel Received message:" << msg;
    emit received(msg, data);
}

#include "dbusapplicationchannel_p.moc"

#endif
