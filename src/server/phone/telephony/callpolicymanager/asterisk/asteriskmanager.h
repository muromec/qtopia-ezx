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

#ifndef _ASTERISKMANAGER_H_
#define _ASTERISKMANAGER_H_

#include <QObject>
#include <qnetworkregistration.h>
#include <qcommservicemanager.h>
#include "qabstractcallpolicymanager.h"

class AsteriskManagerPrivate;

class AsteriskManager : public QAbstractCallPolicyManager
{
    Q_OBJECT
public:
    AsteriskManager( QObject *parent=0 );
    ~AsteriskManager();

    QString callType() const;
    QString trCallType() const;
    QString callTypeIcon() const;
    QTelephony::RegistrationState registrationState() const;
    QAbstractCallPolicyManager::CallHandling handling(const QString& number);
    bool isAvailable(const QString& number);
    QString registrationMessage() const;
    QString registrationIcon() const;

private slots:
    void registrationStateChanged();
    void servicesChanged();

private:
    AsteriskManagerPrivate *d;

    void serviceStarted();
    void serviceStopped();
};

QTOPIA_TASK_INTERFACE(AsteriskManager);

#endif // _ASTERISKMANAGER_H_
