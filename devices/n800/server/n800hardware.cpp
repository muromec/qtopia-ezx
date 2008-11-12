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

#ifdef QT_QWS_N800

#include "n800hardware.h"

#include <QSocketNotifier>
#include <QTimer>
#include <QLabel>
#include <QDesktopWidget>
#include <QProcess>
#include <QFile>
#include <QtGlobal>

#include <qcontentset.h>
#include <qtopiaapplication.h>
#include <qtopialog.h>
#include <qbootsourceaccessory.h>
#include <qtopiaipcenvelope.h>
#include <qpowersource.h>

#include <qtopiaserverapplication.h>
#include <standarddevicefeatures.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <sys/ioctl.h>

QTOPIA_TASK(N800Hardware, N800Hardware);

N800Hardware::N800Hardware()
    : charging(false), percent(-1), chargeId(0),
      vsoPortableHandsfree("/Hardware/Accessories/PortableHandsfree"), mountProc(NULL)
{

    bootSource = new QBootSourceAccessoryProvider( "N800", this );

    batterySource = new QPowerSourceProvider(QPowerSource::Battery, "N800Battery", this);
    batterySource->setAvailability(QPowerSource::Available);

    connect(batterySource, SIGNAL(chargingChanged(bool)),
            this, SLOT(chargingChanged(bool)));
    connect(batterySource, SIGNAL(chargeChanged(int)),
            this, SLOT(chargeChanged(int)));

    wallSource = new QPowerSourceProvider(QPowerSource::Wall, "N800Charger", this);

//    QTimer::singleShot(30*1000, this, SLOT(delayedRead()));
}

N800Hardware::~N800Hardware()
{
    if (detectFd >= 0) {
        ::close(detectFd);
        detectFd = -1;
    }
}

void N800Hardware::chargingChanged(bool charging)
{
    if (charging)
        setLeds(101);
    else
        setLeds(batterySource->charge());
}

void N800Hardware::chargeChanged(int charge)
{
    if (!batterySource->charging())
        setLeds(charge);
}

void N800Hardware::setLeds(int charge)
{
/*
  int ledFd = ::open("/dev/omega_chgled", O_RDWR);
    if (ledFd < 0) {
        perror("Failed to open /dev/omega_chgled");
        return;
    }

    if (charge == 101) {
        // red on
        ::ioctl(ledFd, SET_GPO0_CTRL, 0);
        ::ioctl(ledFd, SET_GPO1_CTRL, 7);
    } else if (percent > 75) {
        // red flashing slow
        ::ioctl(ledFd, ENABLE_LED1_IN_ACT, 0);
        ::ioctl(ledFd, SET_LED1_BLINK_TIME, 7);
        ::ioctl(ledFd, SET_LED1_LIGHT_TIME, 1);
        ::ioctl(ledFd, SET_GPO0_CTRL, 0);
        ::ioctl(ledFd, SET_GPO1_CTRL, 1);
    } else if (percent > 50) {
        // red flashing slow
        ::ioctl(ledFd, ENABLE_LED1_IN_ACT, 0);
        ::ioctl(ledFd, SET_LED1_BLINK_TIME, 5);
        ::ioctl(ledFd, SET_LED1_LIGHT_TIME, 1);
        ::ioctl(ledFd, SET_GPO0_CTRL, 0);
        ::ioctl(ledFd, SET_GPO1_CTRL, 1);
    } else if (percent > 25) {
        // red flashing fast
        ::ioctl(ledFd, ENABLE_LED1_IN_ACT, 0);
        ::ioctl(ledFd, SET_LED1_BLINK_TIME, 3);
        ::ioctl(ledFd, SET_LED1_LIGHT_TIME, 1);
        ::ioctl(ledFd, SET_GPO0_CTRL, 0);
        ::ioctl(ledFd, SET_GPO1_CTRL, 1);
    } else if (percent > 0) {
        // red flashing fast
        ::ioctl(ledFd, ENABLE_LED1_IN_ACT, 0);
        ::ioctl(ledFd, SET_LED1_BLINK_TIME, 1);
        ::ioctl(ledFd, SET_LED1_LIGHT_TIME, 1);
        ::ioctl(ledFd, SET_GPO0_CTRL, 0);
        ::ioctl(ledFd, SET_GPO1_CTRL, 1);
    } else if (percent == 0) {
        // red flashing very fast
        ::ioctl(ledFd, ENABLE_LED1_IN_ACT, 0);
        ::ioctl(ledFd, SET_LED1_BLINK_TIME, 0);
        ::ioctl(ledFd, SET_LED1_LIGHT_TIME, 4);
        ::ioctl(ledFd, SET_GPO0_CTRL, 0);
        ::ioctl(ledFd, SET_GPO1_CTRL, 1);
    } else {
        // all off
        ::ioctl(ledFd, SET_GPO0_CTRL, 0);
        ::ioctl(ledFd, SET_GPO1_CTRL, 0);
    }

    if (ledFd >= 0)
        ::close(ledFd);
*/
}

void N800Hardware::shutdownRequested()
{
    QtopiaServerApplication::instance()->shutdown(QtopiaServerApplication::ShutdownSystem);
}


#endif // QT_QWS_N800
