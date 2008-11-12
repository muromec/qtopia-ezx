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

#ifndef _E1_BATTERY_H_
#define _E1_BATTERY_H_

#include <QObject>
#include <QValueSpaceItem>

class E1Battery : public QObject
{
Q_OBJECT
public:
    E1Battery(QObject *parent = 0);
    virtual ~E1Battery();

    int charge();

signals:
    void chargeChanged(int);

protected:
    virtual void timerEvent(QTimerEvent *);

private slots:
    void batteryChanged();

private:
    void startcharge();
    void stopcharge();
    int m_timerId;
    QValueSpaceItem m_battery;
    int m_charge;
};

#endif // _E1_BATTERY_H_

