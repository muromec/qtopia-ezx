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

#ifndef FICHARDWARE_H
#define FICHARDWARE_H

#ifdef QT_QWS_FICGTA01

#include <QObject>
#include <QProcess>

#include <qvaluespace.h>
#include <linux/input.h>

class QBootSourceAccessoryProvider;
class QPowerSourceProvider;

class QSocketNotifier;
class QtopiaIpcAdaptor;
class QSpeakerPhoneAccessoryProvider;

class Ficgta01Hardware : public QObject
{
    Q_OBJECT

public:
    Ficgta01Hardware();
    ~Ficgta01Hardware();

private:
     QValueSpaceObject vsoPortableHandsfree;
     QValueSpaceObject vsoUsbCable;
     QValueSpaceObject vsoNeoHardware;
     QtopiaIpcAdaptor *adaptor;

     void findHardwareVersion();

     QtopiaIpcAdaptor         *audioMgr;
//     QAudioStateConfiguration *audioConf;
    
 
private slots:
     void headphonesInserted(bool);
     void cableConnected(bool);
     void shutdownRequested();
};

#endif

#endif
