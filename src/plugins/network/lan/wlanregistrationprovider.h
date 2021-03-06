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

#include <custom.h>

#ifndef NO_WIRELESS_LAN

#include <QWlanRegistration>
#include <QAbstractIpcInterfaceGroup>

class WlanRegistrationInterface;
class WlanRegistrationProvider : public QAbstractIpcInterfaceGroup
{
    Q_OBJECT
public:
    WlanRegistrationProvider( const QString& serviceName, QObject* parent = 0 );
    ~WlanRegistrationProvider();

    void initialize();

    void setAccessPoint( const QString& essid = QString() );
    void notifyClients();

private:
    WlanRegistrationInterface* wri;
    QString servName;
    QString essid;
};

#endif //NO_WIRELESS_LAN
