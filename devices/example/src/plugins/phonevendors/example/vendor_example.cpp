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

#include "vendor_example_p.h"
#include <qmodemindicators.h>
#include <qatresultparser.h>
#include <qatutils.h>
#include <qvaluespace.h>
#include <QTimer>

ExampleCallProvider::ExampleCallProvider( QModemService *service )
    : QModemCallProvider( service )
{
}

ExampleCallProvider::~ExampleCallProvider()
{
}

QModemCallProvider::AtdBehavior ExampleCallProvider::atdBehavior() const
{ 
    return AtdOkIsConnect;
}

QString ExampleCallProvider::putOnHoldCommand() const
{
    qWarning("**********ExampleCallProvider::putOnHoldCommand");
    return "AT+CHLD=2";
}

ExampleModemService::ExampleModemService
        ( const QString& service, QSerialIODeviceMultiplexer *mux,
          QObject *parent )
    : QModemService( service, mux, parent )
{
    // We need to do some extra stuff at reset time.
    connect( this, SIGNAL(resetModem()), this, SLOT(reset()) );

    // Register for signal strength notification
    primaryAtChat()->registerNotificationType
        ( "+CSQ:", this, SLOT(signalStrength(QString)), true );

}

ExampleModemService::~ExampleModemService()
{
}

void ExampleModemService::initialize()
{
    if ( !callProvider() )
        setCallProvider( new ExampleCallProvider( this ) );

    QModemService::initialize();
}

void ExampleModemService::reset()
{
    // Make sure that "AT*ECAM" is re-enabled after a reset.
    chat( "AT*ECAM=1" );

    // Turn on unsolicited signal strength indicators.
    chat( "AT*ECIND=2,1,1" );
}

void ExampleModemService::signalStrength( const QString& msg )
{
    int value = msg.mid(12).toInt();
    indicators()->setSignalQuality( value, 5 );
    qWarning("**********ExampleModemService::signalStrength()");
}
