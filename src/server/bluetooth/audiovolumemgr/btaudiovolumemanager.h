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
#ifndef __BTAUDIOVOLUMEMANAGER_P_H__
#define __BTAUDIOVOLUMEMANAGER_P_H__

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qtopia API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QObject>

class BluetoothAudioVolumeControl;
class QBluetoothAddress;

class BtAudioVolumeManager : public QObject
{
    Q_OBJECT

public:
    BtAudioVolumeManager(const QString &service, QObject *parent = 0);
    ~BtAudioVolumeManager();

private slots:
    void serviceAdded(const QString &service);
    void serviceRemoved(const QString &service);

    void audioGatewayConnected(bool success, const QString &msg);
    void audioGatewayDisconnected();
    void audioDeviceConnected(const QBluetoothAddress &addr);
    void audioStateChanged();

private:
    void createVolumeControl();
    void removeVolumeControl();

    QString m_service;
    BluetoothAudioVolumeControl *m_volumeControl;
};


#endif
