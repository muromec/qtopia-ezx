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

#ifndef _QPHONEPROFILEPROVIDER_H_
#define _QPHONEPROFILEPROVIDER_H_

#include <QObject>

class QString;
class QByteArray;
class QPhoneProfileProviderPrivate;
class QPhoneProfileProvider : public QObject
{
Q_OBJECT
public:
    QPhoneProfileProvider(QObject *parent = 0);
    virtual ~QPhoneProfileProvider();

private slots:
    void appMessage(const QString &msg, const QByteArray &data);
    void scheduleActivation();
    void activeChanged();
    void audioProfileChanged();

private:
    Q_DISABLE_COPY(QPhoneProfileProvider);
    QPhoneProfileProviderPrivate *d;
};

#endif // _QPHONEPROFILEPROVIDER_H_
