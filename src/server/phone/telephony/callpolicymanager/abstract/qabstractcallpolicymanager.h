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

#ifndef _QABSTRACTCALLPOLICYMANAGER_H_
#define _QABSTRACTCALLPOLICYMANAGER_H_

#include <QObject>
#include <qtelephonynamespace.h>
#include "qtopiaserverapplication.h"

class QAbstractCallPolicyManager : public QObject
{
    Q_OBJECT
public:
    QAbstractCallPolicyManager(QObject *parent = 0) : QObject(parent) {}

    enum CallHandling
    {
        CannotHandle,
        CanHandle,
        MustHandle,
        NeverHandle
    };

    virtual QString callType() const = 0;
    virtual QString trCallType() const = 0;
    virtual QString callTypeIcon() const = 0;
    virtual QTelephony::RegistrationState registrationState() const = 0;
    virtual QAbstractCallPolicyManager::CallHandling handling(const QString& number) = 0;
    virtual bool isAvailable(const QString& number) = 0;
    virtual QString registrationMessage() const = 0;
    virtual QString registrationIcon() const = 0;

signals:
    void registrationChanged(QTelephony::RegistrationState state);
};

QTOPIA_TASK_INTERFACE(QAbstractCallPolicyManager);

#endif // _QABSTRACTCALLPOLICYMANAGER_H_
