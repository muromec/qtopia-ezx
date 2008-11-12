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

#ifndef BT_DIALUP_SERVICE_H
#define BT_DIALUP_SERVICE_H

#include <QObject>
#include <QString>

#include "bluetoothserialportservice.h"
#include <qtopiaserverapplication.h>

class QBluetoothSerialPortService;
class BtDialupServiceTask : public QObject
{
    Q_OBJECT
public:
    BtDialupServiceTask( QObject* parent = 0 );
    ~BtDialupServiceTask();

private:
    QBluetoothSerialPortService* provider;
};

#endif //BT_DIALUP_SERVICE_h
