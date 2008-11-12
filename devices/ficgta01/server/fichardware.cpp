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

#ifdef QT_QWS_FICGTA01

#include "fichardware.h"

#include <QSocketNotifier>
#include <QTimer>
#include <QLabel>
#include <QDesktopWidget>
#include <QProcess>
#include <QtopiaIpcAdaptor>

#include <qcontentset.h>
#include <qtopiaapplication.h>
#include <qtopialog.h>
#include <qtopiaipcadaptor.h>

#include <qbootsourceaccessory.h>
#include <qtopiaipcenvelope.h>

#include <qtopiaserverapplication.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <linux/input.h>

#include <sys/ioctl.h>

QTOPIA_TASK(Ficgta01Hardware, Ficgta01Hardware);

Ficgta01Hardware::Ficgta01Hardware()
    : vsoPortableHandsfree("/Hardware/Accessories/PortableHandsfree"),
      vsoUsbCable("/Hardware/UsbGadget"),
      vsoNeoHardware("/Hardware/Neo")
{
    adaptor = new QtopiaIpcAdaptor("QPE/NeoHardware");

    qLog(Hardware) << "ficgta01hardware";

    vsoPortableHandsfree.setAttribute("Present", false);
    vsoPortableHandsfree.sync();

// Handle Audio State Changes
    audioMgr = new QtopiaIpcAdaptor("QPE/AudioStateManager", this);


    QtopiaIpcAdaptor::connect(adaptor, MESSAGE(headphonesInserted(bool)),
                              this, SLOT(headphonesInserted(bool)));

    QtopiaIpcAdaptor::connect(adaptor, MESSAGE(cableConnected(bool)),
                              this, SLOT(cableConnected(bool)));
    findHardwareVersion();
}

Ficgta01Hardware::~Ficgta01Hardware()
{
}

void Ficgta01Hardware::findHardwareVersion()
{
    QFile cpuinfo( "/proc/cpuinfo");
    QString inStr;
    cpuinfo.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&cpuinfo);
    QString line;
    do {
        line  = in.readLine();
        if (line.contains("Hardware") ){
            QStringList token = line.split(":");
            inStr = token.at(1).simplified();
        }
    } while (!line.isNull());

    cpuinfo.close();
    qLog(Hardware)<<"Neo"<< inStr;

    vsoNeoHardware.setAttribute("Device", inStr);
    vsoNeoHardware.sync();
}

void Ficgta01Hardware::headphonesInserted(bool b)
{
    qLog(Hardware)<< __PRETTY_FUNCTION__ << b;
    vsoPortableHandsfree.setAttribute("Present", b);
    vsoPortableHandsfree.sync();
    if (b) {
        QByteArray mode("Headphone");
        audioMgr->send("setProfile(QByteArray)", mode);
    } else {
        QByteArray mode("MediaSpeaker");
        audioMgr->send("setProfile(QByteArray)", mode);
    }


}

void Ficgta01Hardware::cableConnected(bool b)
{
    qLog(Hardware)<< __PRETTY_FUNCTION__ << b;
    vsoUsbCable.setAttribute("cableConnected", b);
    vsoUsbCable.sync();
}

void Ficgta01Hardware::shutdownRequested()
{
    qLog(PowerManagement)<< __PRETTY_FUNCTION__;

    QFile powerFile;
    QFile btPower;

    if ( QFileInfo("/sys/bus/platform/devices/gta01-pm-gsm.0/power_on").exists()) {
//ficgta01
        powerFile.setFileName("/sys/bus/platform/devices/gta01-pm-gsm.0/power_on");
        btPower.setFileName("/sys/bus/platform/devices/gta01-pm-bt.0/power_on");
    } else {
//ficgta02
        powerFile.setFileName("/sys/bus/platform/devices/neo1973-pm-gsm.0/power_on");
        btPower.setFileName("/sys/bus/platform/devices/neo1973-pm-bt.0/power_on");
    }

    if( !powerFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning()<<"File not opened";
    } else {
        QTextStream out(&powerFile);
        out << "0";
        powerFile.close();
    }

        if( !btPower.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning()<<"File not opened";
    } else {
        QTextStream out(&btPower);
        out <<  "0";
        powerFile.close();
    }


    QtopiaServerApplication::instance()->shutdown(QtopiaServerApplication::ShutdownSystem);
}

#endif // QT_QWS_Ficgta01

