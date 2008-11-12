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

#ifndef _VOIPMANAGER_H_
#define _VOIPMANAGER_H_

#include <QObject>
#include <qnetworkregistration.h>
#include <qpresence.h>
#include <qcommservicemanager.h>
#include "qabstractcallpolicymanager.h"

class VoIPManagerPrivate;

class VoIPManager : public QAbstractCallPolicyManager
{
    Q_OBJECT
public:
    VoIPManager(QObject *parent=0);
    static VoIPManager * instance();

    QString callType() const;
    QString trCallType() const;
    QString callTypeIcon() const;
    QAbstractCallPolicyManager::CallHandling handling(const QString& number);
    QString registrationMessage() const;
    QString registrationIcon() const;

    QTelephony::RegistrationState registrationState() const;
    QPresence::Status localPresence() const;
    void startMonitoring();
    bool isAvailable(const QString &uri);

signals:
    void localPresenceChanged(QPresence::Status);
    void monitoredPresenceChanged(const QString&, bool available);

private slots:
    void registrationStateChanged();
    void localPresenceChanged();
    void monitoredPresence(const QString&, QPresence::Status);
    void servicesChanged();
    void hideMessageTimeout();

private:
    VoIPManagerPrivate *d;

    void serviceStarted();
    void serviceStopped();
};

QTOPIA_TASK_INTERFACE(VoIPManager);

#endif // _VOIPMANAGER_H_
