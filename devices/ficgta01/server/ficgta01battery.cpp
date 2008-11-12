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

#include "ficgta01battery.h"
#include "qtopiaserverapplication.h"

#include <QPowerSourceProvider>
#include <QTimer>
#include <QFileMonitor>

#include <math.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <QValueSpaceItem>
#include <QtopiaIpcEnvelope>


// /sys/devices/platform/bq27000-battery.0/power_supply/bat

Ficgta01Battery::Ficgta01Battery(QObject *parent)
: QObject(parent),
  ac(0),
  battery(0),
  charging(0),
  percentCharge(0),
  cableEnabled(0),
  vsUsbCable(0)
{

    qWarning()<<"Ficgta01Battery::Ficgta01Battery";

    ac = new QPowerSourceProvider(QPowerSource::Wall, "PrimaryAC", this);
    battery = new QPowerSourceProvider(QPowerSource::Battery, "Ficgta01Battery", this);


    vsUsbCable = new QValueSpaceItem("/Hardware/UsbGadget/cableConnected", this);
    connect( vsUsbCable, SIGNAL(contentsChanged()), SLOT(cableChanged()));

    battery->setAvailability(QPowerSource::Available);

    checkChargeState();

    startTimer(60 * 1000);

    QTimer::singleShot( 10 * 1000, this, SLOT(updateFicStatus()));
}

void Ficgta01Battery::updateFicStatus()
{

    int min = -1; // Remaining battery (minutes)

    // apm on freerunner is borked.

    batteryIsFull();

    if(cableEnabled) {
        ac->setAvailability(QPowerSource::Available);
    } else {
        ac->setAvailability(QPowerSource::NotAvailable);
    }

    battery->setCharge( percentCharge);

    qLog(PowerManagement) << __PRETTY_FUNCTION__ << cableEnabled << percentCharge;

    battery->setCharging( cableEnabled);
    battery->setTimeRemaining(min);
}

/*! \internal */
void Ficgta01Battery::timerEvent(QTimerEvent *)
{
    updateFicStatus();
}

/*! \internal */
void Ficgta01Battery::cableChanged()
{
    bool ch = vsUsbCable->value().toBool();
    qLog(PowerManagement) << __PRETTY_FUNCTION__ << ch;
    cableEnabled = ch;
    charging = ch;

    QTimer::singleShot( 1000, this, SLOT(updateFicStatus()));
}


/*! \internal */
int Ficgta01Battery::getBatteryLevel()
{
    qLog(PowerManagement) << __PRETTY_FUNCTION__;
    int voltage = 0;
    QString batteryVoltage;
    QString inStr;
    if ( QFileInfo("/sys/devices/platform/s3c2440-i2c/i2c-adapter/i2c-0/0-0073").exists()) {
        batteryVoltage = "/sys/devices/platform/s3c2440-i2c/i2c-adapter/i2c-0/0-0073/battvolt";
    } else {
        batteryVoltage = "/sys/devices/platform/s3c2410-i2c/i2c-adapter/i2c-0/0-0008/battvolt";
    }

    QFile battvolt( batteryVoltage);
    battvolt.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&battvolt);
    in >> inStr;
    voltage = inStr.toInt();
    battvolt.close();
    qLog(PowerManagement)<<"voltage"<< inStr;

    // lets use 3400 as empty, for all intensive purposes,
    // 2 minutes left of battery life till neo shuts off might
    // as well be empty

    voltage = voltage - 3400;
    float perc = voltage  / 8;
    percentCharge = round( perc + 0.5);
    percentCharge = qBound<quint16>(0, percentCharge, 100);
    qLog(PowerManagement)<<"Battery volt"<< voltage + 3400 << percentCharge<<"%";

/*
4200 = 100%
4000 = 90%
3600 = 20% << 10 minutes left
3400 = 5% << 2 minutes left
3100 = 0%
*/
    return voltage;
}


/*! \internal */
bool Ficgta01Battery::batteryIsFull()
{
    qLog(PowerManagement) << __PRETTY_FUNCTION__;
    if(getBatteryLevel() + 3400 > 4200)
        return true;
    return false;
}


void Ficgta01Battery::checkChargeState()
{
    qLog(PowerManagement) << __PRETTY_FUNCTION__;
    QString chgState;
    if (QFileInfo("/sys/devices/platform/s3c2440-i2c/i2c-adapter/i2c-0/0-0073/chgstate").exists()) {
        //freerunner
        chgState = "/sys/devices/platform/s3c2440-i2c/i2c-adapter/i2c-0/0-0073/chgstate";
    } else {
        //1973
        chgState = "/sys/devices/platform/s3c2410-i2c/i2c-adapter/i2c-0/0-0008/chgstate";
    }

    QString inStr;
    QFile chgstate( chgState);
    chgstate.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&chgstate);
    in >> inStr;
    chgstate.close();
    qLog(PowerManagement) << inStr;

    if (inStr.contains("enabled")) {
        cableEnabled = true;
    } else {
        cableEnabled = false;
    }

    charging = cableEnabled;
    cableEnabled = cableEnabled;
    battery->setCharging(cableEnabled);

    QtopiaIpcEnvelope e2("QPE/Neo1973Hardware", "cableConnected(bool)");
    e2 << cableEnabled;
    updateFicStatus();
}

QTOPIA_TASK(Ficgta01Battery, Ficgta01Battery);
