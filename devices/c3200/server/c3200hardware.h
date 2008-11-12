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

#ifndef C3200HARDWARE_H
#define C3200HARDWARE_H

#ifdef QT_QWS_C3200

#include <QObject>
#include <QProcess>

#include <QAudioStateConfiguration>
#include <QAudioStateInfo>
#include <qaudionamespace.h>
#include <QtopiaIpcEnvelope>

#include <qvaluespace.h>

class QSocketNotifier;
class QtopiaIpcAdaptor;
class QPowerSourceProvider;
class QAudioStateConfiguration;

class C3200Hardware : public QObject
{
    Q_OBJECT

public:
    C3200Hardware();
    ~C3200Hardware();

private:
    QValueSpaceObject vsoPortableHandsfree;

    QSocketNotifier *m_notifyDetect;
    int detectFd;

    QPowerSourceProvider *batterySource;
    QPowerSourceProvider *wallSource;

    QtopiaIpcAdaptor *mgr;
    QAudioStateConfiguration *audioConf;

private slots:
    void readDetectData();
    void shutdownRequested();

    void chargingChanged(bool charging);
    void chargeChanged(int charge);

    void availabilityChanged();
    void currentStateChanged(const QAudioStateInfo &state, QAudio::AudioCapability capability);
};

#endif // QT_QWS_C3200

#endif // C3200HARDWARE_H
