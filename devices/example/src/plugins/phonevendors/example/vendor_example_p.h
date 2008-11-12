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

#ifndef VENDOR_EXAMPLE_P_H
#define VENDOR_EXAMPLE_P_H

#include <qmodemservice.h>
#include <qmodemcall.h>
#include <qmodemcallprovider.h>

class QValueSpaceObject;
class QTimer;


class ExampleCallProvider : public QModemCallProvider
{
    Q_OBJECT
public:
    ExampleCallProvider( QModemService *service );
    ~ExampleCallProvider();

protected:
    QModemCallProvider::AtdBehavior atdBehavior() const;
    QString putOnHoldCommand() const;  
};

class ExampleModemService : public QModemService
{
    Q_OBJECT
public:
    ExampleModemService
        ( const QString& service, QSerialIODeviceMultiplexer *mux,
          QObject *parent = 0 );
    ~ExampleModemService();

    void initialize();

private slots:
    void signalStrength( const QString& msg );
    void reset();
};

#endif
