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

#ifdef QT_QWS_C3200

#include "c3200hardware.h"

#include <QSocketNotifier>
#include <QTimer>
#include <QLabel>
#include <QDesktopWidget>
#include <QProcess>

#include <qwindowsystem_qws.h>

#include <qcontentset.h>
#include <qtopiaapplication.h>
#include <qtopialog.h>
#include <qtopiaipcadaptor.h>
#include <qbootsourceaccessory.h>
#include <qtopiaipcenvelope.h>
#include <qpowersource.h>

#include <QAudioStateConfiguration>
#include <QAudioStateInfo>
#include <qaudionamespace.h>
#include <QtopiaIpcEnvelope>

#include <qtopiaserverapplication.h>
#include <standarddevicefeatures.h>
#include <ui/standarddialogs.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <sys/ioctl.h>

struct detect_device_t {
    unsigned int   dummy1;
    unsigned int   dummy2;
    unsigned short type;
    unsigned short code;
    unsigned int   value;
};

QTOPIA_TASK(C3200Hardware, C3200Hardware);

C3200Hardware::C3200Hardware()
      :vsoPortableHandsfree("/Hardware/Accessories/PortableHandsfree")
{
    //StandardDeviceFeatures::disableBatteryMonitor();
    detectFd = ::open("/dev/input/event0", O_RDONLY | O_NDELAY, 0);
    if (detectFd >= 0) {
        qLog(Hardware) << "Opened keypad as detect input";
        m_notifyDetect = new QSocketNotifier(detectFd, QSocketNotifier::Read, this);
        connect(m_notifyDetect, SIGNAL(activated(int)), this, SLOT(readDetectData()));
    } else {
        qWarning("Cannot open log for detect (%s)", strerror(errno));
    }
    batterySource = new QPowerSourceProvider(QPowerSource::Battery, "C3200Battery", this);
    batterySource->setAvailability(QPowerSource::Available);
    connect(batterySource, SIGNAL(chargingChanged(bool)),
            this, SLOT(chargingChanged(bool)));
    connect(batterySource, SIGNAL(chargeChanged(int)),
            this, SLOT(chargeChanged(int)));
    wallSource = new QPowerSourceProvider(QPowerSource::Wall, "C3200Charger", this);

    // Handle Audio State Changes
    mgr = new QtopiaIpcAdaptor("QPE/AudioStateManager", this);
    audioConf = new QAudioStateConfiguration(this);
    connect(audioConf, SIGNAL(currentStateChanged(QAudioStateInfo,QAudio::AudioCapability)),
            this, SLOT(currentStateChanged(QAudioStateInfo,QAudio::AudioCapability)));
    connect(audioConf, SIGNAL(availabilityChanged()),
            this, SLOT(availabilityChanged()));
}

C3200Hardware::~C3200Hardware()
{
    if (detectFd >= 0) {
        ::close(detectFd);
        detectFd = -1;
    }
}

void C3200Hardware::readDetectData()
{
    detect_device_t detectData;

    int n = read(detectFd, &detectData, sizeof(detectData));
    if(n !=16)
      return;

    if((detectData.type==0) && (detectData.code==0) && (detectData.value==0))
        return;
    if(detectData.type==1) return;

    //-------------
    int ac = 0xff; // AC status
    int bs = 0xff; // Battery status
    int bf = 0xff; // Battery flag
    int pc = -1; // Remaining battery (percentage)
    int min = -1; // Remaining battery (minutes)

    FILE *f = fopen("/proc/apm", "r");
    if ( f  ) {
        //I 1.13 1.2 0x02 0x00 0xff 0xff 49% 147 sec
        char u;
        fscanf(f, "%*[^ ] %*d.%*d 0x%*x 0x%x 0x%x 0x%x %d%% %i %c",
                &ac, &bs, &bf, &pc, &min, &u);
        fclose(f);

        int  percent = pc;

        if(ac) {
            wallSource->setAvailability(QPowerSource::Available);
            batterySource->setCharging(true);
            qLog(Hardware) << "Charger cable plugged in";
        } else {
            wallSource->setAvailability(QPowerSource::NotAvailable);
            batterySource->setCharging(false);
            batterySource->setCharge(percent);
            qLog(Hardware) << "Charger cable unplugged";
        }
    } else {
        wallSource->setAvailability(QPowerSource::NotAvailable);
        batterySource->setCharging(false);
        qLog(Hardware) << "/proc/apm failed!";
    }

    //-------------

    qLog(Hardware) << "event: type = " << detectData.type << ", code = " <<
        detectData.code << ", value = " << detectData.value;

    if((detectData.type==5) && (detectData.code==2) && (detectData.value==1)) {
        qLog(Hardware) << "Headset plugged in";
	vsoPortableHandsfree.setAttribute("Present", true);
        QByteArray mode("PhoneSpeaker");
        mgr->send("setProfile(QByteArray)", mode);
        return;
    }
    if((detectData.type==5) && (detectData.code==2) && (detectData.value==0)) {
        qLog(Hardware) << "Headset unplugged";
	vsoPortableHandsfree.setAttribute("Present", false);
        QByteArray mode("MediaSpeaker");
        mgr->send("setProfile(QByteArray)", mode);
        return;
    }
    if((detectData.type==5) && (detectData.code==1) && (detectData.value==1)) {
        qLog(Hardware) << "Screen closed";
        system("echo 1>/sys/class/backlight/corgi-bl/brightness");
        return;
    }
    if((detectData.type==5) && (detectData.code==1) && (detectData.value==0)) {
        qLog(Hardware) << "Screen opened";
        system("echo 10>/sys/class/backlight/corgi-bl/brightness");
        QWSServer::instance()->refresh();
        return;
    }
}
void C3200Hardware::shutdownRequested()
{
    QtopiaServerApplication::instance()->shutdown(QtopiaServerApplication::ShutdownSystem);
}

void C3200Hardware::chargingChanged(bool charging)
{
    qLog(Hardware) << "could set leds here! charging " << charging;
}

void C3200Hardware::chargeChanged(int charge)
{
    qLog(Hardware) << "charge " << charge;
}

void C3200Hardware::availabilityChanged()
{
    qLog(Hardware) << "void C3200Hardware::availabilityChanged()";
}

void C3200Hardware::currentStateChanged(const QAudioStateInfo &state, QAudio::AudioCapability)
{
    qLog(Hardware) << "void C3200Hardware::currentStateChanged()";
}

#endif // QT_QWS_C3200
