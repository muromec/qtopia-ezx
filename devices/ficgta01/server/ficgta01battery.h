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

#ifndef _FICGTA01BATTERY_H_
#define _FICGTA01BATTERY_H_

#include <QObject>
#include <QSocketNotifier>

class QPowerSourceProvider;
class QValueSpaceItem;

class Ficgta01Battery : public QObject
{
Q_OBJECT
public:
    Ficgta01Battery(QObject *parent = 0);

protected:
    virtual void timerEvent(QTimerEvent *);

private:
    int percentCharge;
    bool charging;
    bool cableEnabled;

    QPowerSourceProvider *ac;
    QPowerSourceProvider *backup;
    QPowerSourceProvider *battery;

    int getBatteryLevel();
    bool batteryIsFull();
    QValueSpaceItem *vsUsbCable;
    
private Q_SLOTS:
    
    void updateFicStatus();

    void cableChanged();
    void checkChargeState();
    
};

#endif // _FICGTA01BATTERY_H_
