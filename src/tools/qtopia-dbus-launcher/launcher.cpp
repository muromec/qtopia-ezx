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

#include "launcher.h"

#include <qtdbus/qdbusconnection.h>
#include <qtdbus/qdbusmessage.h>

#include <QCoreApplication>
#include <QDebug>

#include <stdio.h>
#include <stdlib.h>

Launcher::Launcher(QObject *parent) : QObject(parent)
{
}

Launcher::~Launcher()
{

}

void Launcher::startup(const QString &app)
{
    QDBusConnection dbc = QDBus::sessionBus();

    if (!dbc.isConnected()) {
        fprintf(stderr, "QtopiaLauncher: Could not connect to DBUS bus\n");
        exit(1);
    }

    bool r = dbc.connect(QString(),                           // Service
                         "/DBusLauncher",                     // Path
                         "com.trolltech.qtopia.DBusLauncher", // Interface
                         "launched",                          // Name
                         this,
                         SLOT(handleSignalReceived(QString,QDBusMessage)));

    QDBusMessage message =
            QDBusMessage::methodCall("com.trolltech.qtopia.DBusLauncher",
                                     "/DBusLauncher",
                                     "com.trolltech.qtopia.DBusLauncher",
                                     "launch", dbc);
    message << app;

    bool ret = dbc.send(message);
    if (!ret)
        fprintf(stderr, "Unable to send message: %s", dbc.lastError().message().toAscii().constData());

    m_app = app;
}

void Launcher::handleSignalReceived(const QString &app, const QDBusMessage &msg)
{
    if (app == m_app) {
        QCoreApplication::quit();
    }
}
