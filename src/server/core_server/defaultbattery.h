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

#ifndef _DEFAULTBATTERY_H_
#define _DEFAULTBATTERY_H_

#include <QPowerSourceProvider>
class QHardwareManager;

class DefaultBattery : public QPowerSourceProvider
{
Q_OBJECT
public:
    DefaultBattery(QObject *parent = 0);

private slots:
    void accessoryAdded(const QString &);
    
    void pAvailabilityChanged(QPowerSource::Availability);
    void pChargingChanged(bool);
    void pChargeChanged(int);
    void pCapacityChanged(int);
    void pTimeRemainingChanged(int);

private:
    void initPowerSource();
    void syncPowerSource();

    QString m_primary;
    QPowerSource *m_powerSource;
    QHardwareManager *m_accessories;
};

#endif // _DEFAULTBATTERY_H_
