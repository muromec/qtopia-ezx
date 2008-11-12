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

#ifndef __DBUSROUTER_H__
#define __DBUSROUTER_H__

#if defined(QTOPIA_DBUS_IPC)
#include "applicationlauncher.h"

class DBUSQtopiaApplicationChannel;

class DBusRouter : public ApplicationIpcRouter
{
    Q_OBJECT
public:
    DBusRouter();
    ~DBusRouter();

    virtual void addRoute(const QString &app, RouteDestination *);
    virtual void remRoute(const QString &app, RouteDestination *);

private slots:
    void applicationMessage( const QString& msg, const QByteArray& data );

private:
    void routeMessage(const QString &, const QString &, const QByteArray &);
    QMultiMap<QString, RouteDestination *> m_routes;
    QMap<QString, DBUSQtopiaApplicationChannel *> m_channels;
};

#endif

#endif // QCOPROUTER_H
