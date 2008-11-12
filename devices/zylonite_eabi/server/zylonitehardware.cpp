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

#ifdef QT_QWS_ZYLONITEA1

#include "zylonitehardware.h"

#include <QSocketNotifier>
#include <QTimer>
#include <QLabel>
#include <QDesktopWidget>
#include <QProcess>

#include <qcontentset.h>
#include <qtopiaapplication.h>
#include <qtopialog.h>
#include <qtopiaipcadaptor.h>
//TODO
//#include <qspeakerphoneaccessory.h>
#include <qbootsourceaccessory.h>
#include <qtopiaipcenvelope.h>

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

QTOPIA_TASK(ZyloniteHardware, ZyloniteHardware);

ZyloniteHardware::ZyloniteHardware()
      :vsoPortableHandsfree("/Hardware/Accessories/PortableHandsfree")
{
    // TODO
    //StandardDeviceFeatures::disableBatteryMonitor();

    /*
    detectFd = ::open("/dev/input/event0", O_RDONLY | O_NDELAY, 0);
    if (detectFd >= 0) {
        qLog(Hardware) << "Opened keypad as detect input";
        m_notifyDetect = new QSocketNotifier(detectFd, QSocketNotifier::Read, this);
        connect(m_notifyDetect, SIGNAL(activated(int)), this, SLOT(readDetectData()));
    } else {
        qWarning("Cannot open log for detect (%s)", strerror(errno));
    }
    */
    // TODO
    speakerPhone = 0;
    //        new QSpeakerPhoneAccessoryProvider( "zylonite", this );
}

ZyloniteHardware::~ZyloniteHardware()
{
    /*
    if (detectFd >= 0) {
        ::close(detectFd);
        detectFd = -1;
    }
    */
}

void ZyloniteHardware::readDetectData()
{
    detect_device_t detectData;
/*
    int n = read(detectFd, &detectData, sizeof(detectData));
    if(n !=16)
      return;

    if((detectData.type==0) && (detectData.code==0) && (detectData.value==0)) 
        return;
    if(detectData.type==1) return;
    qWarning("event: type=%03d code=%03d value=%03d",detectData.type, 
              detectData.code,detectData.value);
              */
 /* 
    if((detectData.type==5) && (detectData.code==2) && (detectData.value==1)) {
        qLog(Hardware) << "Headset plugged in";
	vsoPortableHandsfree.setAttribute("Present", true);
        system("amixer set \'Speaker Function\' Off");
        return;  
    } 
    if((detectData.type==5) && (detectData.code==2) && (detectData.value==0)) {
        qLog(Hardware) << "Headset unplugged";
	vsoPortableHandsfree.setAttribute("Present", false);
        system("amixer set \'Speaker Function\' On");
        return;  
    } 
    if((detectData.type==5) && (detectData.code==1) && (detectData.value==1)) {
        qLog(Hardware) << "Screen closed";
        return;  
    }
    if((detectData.type==5) && (detectData.code==1) && (detectData.value==0)) {
        qLog(Hardware) << "Screen opened";
        return;  
    }
    */
}

void ZyloniteHardware::mountSD()
{
    QFile partitions("/proc/partitions");
    if (!partitions.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    sdCardDevice.clear();

    QList<QByteArray> lines = partitions.readAll().split('\n');
    for (int i = 2; i < lines.count(); i++) {
        QStringList fields = QString(lines.at(i)).split(' ', QString::SkipEmptyParts);

        if (fields.count() == 4) {
            if (sdCardDevice.isEmpty() && fields.at(3) == "blk0p") {
                sdCardDevice = "/dev/mss/" + fields.at(3);
            } else if (fields.at(3).startsWith("blk0p")) {
                sdCardDevice = "/dev/mss/" + fields.at(3);
                break;
            }

        }
    }

    if (sdCardDevice.isEmpty())
        return;

    if (!mountProc)
        mountProc = new QProcess(this);

    if (mountProc->state() != QProcess::NotRunning)
        return;

    connect(mountProc, SIGNAL(finished(int,QProcess::ExitStatus)),
            SLOT(fsckFinished(int,QProcess::ExitStatus)));

    QStringList arguments;
    arguments << "-a" << sdCardDevice;

    qLog(Hardware) << "Checking filesystem on" << sdCardDevice;
    mountProc->start("fsck", arguments);

    qLog(Hardware) << "Checking filesystem on" << sdCardDevice;
    mountProc->start("fsck", arguments);
}

void ZyloniteHardware::unmountSD()
{
    if (!mountProc)
        mountProc = new QProcess(this);

    if (mountProc->state() != QProcess::NotRunning) {
        qLog(Hardware) << "Previous (u)mount command failed to finished";
        mountProc->kill();
    }

    connect(mountProc, SIGNAL(finished(int,QProcess::ExitStatus)),
            SLOT(mountFinished(int,QProcess::ExitStatus)));

    qLog(Hardware) << "Unmounting /media/sd";
    mountProc->start("umount -l /media/sd");
}

void ZyloniteHardware::fsckFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode != 0)
        qLog(Hardware) << "Filesystem errors detected on" << sdCardDevice;

    disconnect(mountProc, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(fsckFinished(int,QProcess::ExitStatus)));

    connect(mountProc, SIGNAL(finished(int,QProcess::ExitStatus)),
            SLOT(mountFinished(int,QProcess::ExitStatus)));

    QStringList arguments;
    arguments << sdCardDevice << "/media/sd";

    qLog(Hardware) << "Mounting" << sdCardDevice << "on /media/sd";
    mountProc->start("mount", arguments);
}

void ZyloniteHardware::mountFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode != 0)
        qLog(Hardware) << "Failed to (u)mount" << sdCardDevice << "on /media/sd";

    mountProc->deleteLater();
    mountProc = NULL;

    QtopiaIpcEnvelope msg("QPE/Card", "mtabChanged()");
}

void ZyloniteHardware::shutdownRequested()
{
    QtopiaServerApplication::instance()->shutdown(QtopiaServerApplication::ShutdownSystem);
}
#endif // QT_QWS_ZYLONITEA1
