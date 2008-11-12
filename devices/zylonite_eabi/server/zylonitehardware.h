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

#ifndef ZYLONITEHARDWARE_H
#define ZYLONITEHARDWARE_H

#ifdef QT_QWS_ZYLONITEA1

#include <QObject>
#include <QProcess>

#include <qvaluespace.h>

class QSocketNotifier;
class QtopiaIpcAdaptor;
class QSpeakerPhoneAccessoryProvider;

class ZyloniteHardware : public QObject
{
    Q_OBJECT

public:
    ZyloniteHardware();
    ~ZyloniteHardware();

private:
    QValueSpaceObject vsoPortableHandsfree;

    QSocketNotifier *m_notifyDetect;
    int detectFd;

    QProcess *mountProc;
    QString sdCardDevice;

    QSpeakerPhoneAccessoryProvider *speakerPhone;

private slots:
    void readDetectData();
    void shutdownRequested();

    void mountSD();
    void unmountSD();
    void fsckFinished(int, QProcess::ExitStatus);
    void mountFinished(int, QProcess::ExitStatus);
};

#endif // QT_QWS_ZYLONITEA1

#endif // ZYLONITEHARDWARE_H
