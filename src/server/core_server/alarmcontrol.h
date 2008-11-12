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

#ifndef _ALARMCONTROL_H_
#define _ALARMCONTROL_H_

#include <QObject>
#include <qvaluespace.h>
#include <qtopiaipcenvelope.h>

class AlarmControl : public QObject
{
Q_OBJECT
public:
    static AlarmControl *instance();

    bool alarmState() const;

signals:
    void alarmStateChanged(bool);

private slots:
    void alarmMessage(const QString& message, const QByteArray&);

private:
    void alarmEnabled(bool);
    bool alarmOn;
    QValueSpaceObject alarmValueSpace;
    QtopiaChannel alarmChannel;
    AlarmControl();
};

#endif // _ALARMCONTROL_H_

