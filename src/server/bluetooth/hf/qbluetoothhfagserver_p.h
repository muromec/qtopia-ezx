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

#ifndef __QBLUETOOTHHFAGSERVER_P_H__
#define __QBLUETOOTHHFAGSERVER_P_H__

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

#include <qtopia/comm/qbluetoothaudiogateway.h>
#include <qtopiaglobal.h>

class QBluetoothHandsfreeCommInterface;

class QBluetoothHandsfreeAudioGatewayServer : public QBluetoothAudioGateway
{
    Q_OBJECT

public:
    QBluetoothHandsfreeAudioGatewayServer(QBluetoothHandsfreeCommInterface *parent,
                                        const QString &audioDev,
                                        const QString& service);
    ~QBluetoothHandsfreeAudioGatewayServer();

    using QCommInterface::setValue;

public slots:
    void connect(const QBluetoothAddress &addr, int rfcomm_channel);
    void disconnect();
    void setSpeakerVolume(int volume);
    void setMicrophoneVolume(int volume);
    void releaseAudio();
    void connectAudio();

private:
    QBluetoothHandsfreeCommInterface *m_parent;
};

#endif
