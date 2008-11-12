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

#ifndef BLUETOOTH_TERMINAL_SERVICE
#define BLUETOOTH_TERMINAL_SERVICE

#include <qbluetoothabstractservice.h>
#include <qbluetoothnamespace.h>

class QBluetoothSerialPortServicePrivate;

class QBluetoothSerialPortService : public QBluetoothAbstractService
{
    Q_OBJECT
public:
    QBluetoothSerialPortService( const QString& serviceID,
                                 const QString& serviceName,
                                 const QBluetoothSdpRecord &record,
                                 QObject* parent = 0 );
    ~QBluetoothSerialPortService();

    void start();
    void stop();
    void setSecurityOptions( QBluetooth::SecurityOptions options );

protected slots:
    void newConnection();

private slots:
    void initiateModemEmulator();
    void emulatorStateChanged();

private:
    QBluetoothSerialPortServicePrivate* d;
};

#endif ///BLUETOOTH_TERMINAL_SERVICE
