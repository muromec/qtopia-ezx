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

#ifndef _CELLBROADCASTCONTROL_H_
#define _CELLBROADCASTCONTROL_H_

#include <QObject>
#include "cellmodemmanager.h"
class QString;
class QCellBroadcast;
class QCBSMessage;

class CellBroadcastControl : public QObject
{
Q_OBJECT
public:
    CellBroadcastControl(QObject *parent = 0);

    enum Type { Popup, Background };

    static CellBroadcastControl *instance();

signals:
    void broadcast(CellBroadcastControl::Type, const QString &channel, const QString &text);

private slots:
    void cellBroadcast(const QCBSMessage &);
    void cellBroadcastResult(QTelephony::Result);
    void registrationChanged(QTelephony::RegistrationState);

private:
    void subscribe();
    QCellBroadcast *cb;
    bool firstSubscribe;
    QString cellLocation;
};

#endif // _CELLBROADCASTCONTROL_H_

