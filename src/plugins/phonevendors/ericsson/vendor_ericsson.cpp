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

#include "vendor_ericsson_p.h"
#include <qmodemindicators.h>
#include <qatresultparser.h>
#include <qatutils.h>
#include <qvaluespace.h>
#include <QTimer>
#include <QDebug>

EricssonCallProvider::EricssonCallProvider( QModemService *service )
    : QModemCallProvider( service )
{
    service->primaryAtChat()->registerNotificationType
            ( "*ECAV:", this, SLOT(ecavNotification(QString)) );
}

EricssonCallProvider::~EricssonCallProvider()
{
}

QModemCallProvider::AtdBehavior EricssonCallProvider::atdBehavior() const
{
    // When the ATD command says OK, it has returned to command state.
    // We will notify QModemCallProvider when the call actually connects
    // and sends us an *ECAM notification.
    return AtdOkIsDialingWithStatus;
}

void EricssonCallProvider::ecavNotification( const QString& msg )
{
    // Ericsson-style call monitoring notification.  The first
    // two parameters are the call identifier and the status.
    // 0 = IDLE, 1 = CALLING, 2 = CONNECTING, 3 = ACTIVE,
    // 4 = HOLD, 5 = WAITING, 6 = ALERTING, 7 = BUSY.
    uint posn = 6;
    uint identifier = QAtUtils::parseNumber( msg, posn );
    uint status = QAtUtils::parseNumber( msg, posn );
    QModemCall *call = callForIdentifier( identifier );

    if ( status == 3 && call &&
         ( call->state() == QPhoneCall::Dialing ||
           call->state() == QPhoneCall::Alerting ) ) {

        // This is an indication that a "Dialing" connection
        // is now in the "Connected" state.
        call->setConnected();

    } else if ( ( status == 2 || status == 6 ) && call &&
                call->state() == QPhoneCall::Dialing ) {

        // This is an indication that a "Dialing" connection
        // is now in the "Alerting" state.
        //
        // Note: according to some Ericsson specifications, "ALERTING"
        // is only for incoming calls, with "CONNECTING" for outgoing calls.
        // But other Ericsson specifications are vague.  We check for both
        // "CONNECTING" and "ALERTING" just in case.  The QPhoneCall::Dialing
        // check above will ensure that we don't do this by accident for
        // incoming calls.
        call->setState( QPhoneCall::Alerting );

    } else if ( status == 0 && call &&
                ( call->state() == QPhoneCall::Dialing ||
                  call->state() == QPhoneCall::Alerting ) ) {

        // We never managed to connect.
        hangupRemote( call );

    } else if ( status == 0 && call &&
                ( call->state() == QPhoneCall::Connected ||
                  call->state() == QPhoneCall::Hold ) ) {

        // This is an indication that the connection has been lost.
        hangupRemote( call );

    } else if ( status == 0 && call && call->state() == QPhoneCall::Incoming ) {

        // This is an indication that an incoming call was missed.
        call->setState( QPhoneCall::Missed );

    } else if ( status == 5 && !call ) {

        // This is a newly waiting call.  Treat it the same as "RING".
        uint callTypeId = QAtUtils::parseNumber( msg, posn );
        QString callType;
        if ( callTypeId == 0 || callTypeId == 128 )
            callType = "Voice";     // No tr
        else if ( callTypeId == 3 )
            callType = "Fax";       // No tr
        else
            callType = "Data";      // No tr
        QAtUtils::skipField( msg, posn );
        QAtUtils::skipField( msg, posn );
        QString number = QAtUtils::nextString( msg, posn );
        uint type = QAtUtils::parseNumber( msg, posn );
        ringing( QAtUtils::decodeNumber( number, type ),
                 callType, identifier );

    }
}

EricssonModemService::EricssonModemService
        ( const QString& service, QSerialIODeviceMultiplexer *mux,
          QObject *parent, bool trolltechExtensions )
    : QModemService( service, mux, parent )
{
    this->trolltechExtensions = trolltechExtensions;

    // We need to do some extra stuff at reset time.
    connect( this, SIGNAL(resetModem()), this, SLOT(reset()) );

    // Turn on Ericsson-style call monitoring so that we can
    // distinguish between call in progress and call connected.
    primaryAtChat()->chat( "AT*ECAM=1" );

    // Register for signal strength and sms memory full notifications.
    primaryAtChat()->registerNotificationType
        ( "*ECIND: 5,1,", this, SLOT(signalStrength(QString)) );
    primaryAtChat()->registerNotificationType
        ( "*ECIND: 5,5,", this, SLOT(smsMemoryFull(QString)) );
    primaryAtChat()->registerNotificationType
        ( "*TTZ: 2,", this, SLOT(ttzNotification(QString)), true );
}

EricssonModemService::~EricssonModemService()
{
}

void EricssonModemService::initialize()
{
    // Always turn this on for now
    //if ( trolltechExtensions ) {
    {

        // We have a special SIM toolkit implementation that allows us
        // to perform GCF compliance testing.  Start it up now.
        addInterface( new TrolltechSimToolkit( this ) );

        // Band selection.
        addInterface( new TrolltechBandSelection( this ) );

    }

    if ( !callProvider() )
        setCallProvider( new EricssonCallProvider( this ) );

    QModemService::initialize();
}

void EricssonModemService::reset()
{
    // Make sure that "AT*ECAM" is re-enabled after a reset.
    chat( "AT*ECAM=1" );

    // Turn on unsolicited signal strength and SMS memory full indicators.
    chat( "AT*ECIND=2,1,1" );
    chat( "AT*ECIND=2,5,1" );
}

void EricssonModemService::signalStrength( const QString& msg )
{
    int value = msg.mid(12).toInt();
    indicators()->setSignalQuality( value, 5 );
}

void EricssonModemService::smsMemoryFull( const QString& msg )
{
    int value = msg.mid(12).toInt();
    indicators()->setSmsMemoryFull
        ( (QModemIndicators::SmsMemoryFullStatus)value );
}

void EricssonModemService::ttzNotification( const QString& msg )
{
    // Timezone information from the network.
    uint posn = 8;
    QString time = QAtUtils::nextString( msg, posn );
    int dst = ((int)QAtUtils::parseNumber( msg, posn )) * 60;
    int zoneIndex = time.length();
    while ( zoneIndex > 0 && time[zoneIndex - 1] != QChar('-') &&
            time[zoneIndex - 1] != QChar('+') )
        --zoneIndex;
    int zoneOffset;
    if ( zoneIndex > 0 && time[zoneIndex - 1] == QChar('-') ) {
        zoneOffset = time.mid(zoneIndex - 1).toInt() * 15;
    } else if ( zoneIndex > 0 && time[zoneIndex - 1] == QChar('+') ) {
        zoneOffset = time.mid(zoneIndex).toInt() * 15;
    } else {
        // Unknown timezone information.
        return;
    }
    QString timeString;
    if (zoneIndex > 0)
        timeString = time.mid(0, zoneIndex - 1);
    else
        timeString = time;
    QDateTime t = QDateTime::fromString(timeString, "MM/dd/yyyy, HH:mm:ss");
    if ( t.isValid() ) {
        QDateTime utc = QDateTime(t.date(), t.time(), Qt::UTC);
        utc = utc.addSecs(-zoneOffset * 60);
        indicators()->setNetworkTime( utc.toTime_t(), zoneOffset, dst );
    }
}

TrolltechSimToolkit::TrolltechSimToolkit( QModemService *service )
    : QModemSimToolkit( service )
{
    service->primaryAtChat()->registerNotificationType( "*TCMD:", this, SLOT(tcmd(QString)) );
    service->primaryAtChat()->registerNotificationType( "*TCC:", this, SLOT(tcc(QString)) );
}

TrolltechSimToolkit::~TrolltechSimToolkit()
{
}

void TrolltechSimToolkit::initialize()
{
    // Send the "Trolltech SIM Toolkit Begin" command, which will cause the
    // phone simulator to send us the main menu, or to simply ignore us if
    // it does not support SIM toolkit.
    service()->chat( "AT*TSTB" );

    // Initialization is done.
    QModemSimToolkit::initialize();
}

void TrolltechSimToolkit::begin()
{
    // Send the "Trolltech SIM Toolkit Begin" command.
    service()->chat( "AT*TSTB", this, SLOT(tstb(bool)) );
}

void TrolltechSimToolkit::end()
{
    // Send the "Trolltech SIM Toolkit End" command.
    service()->chat( "AT*TSTE" );
}

void TrolltechSimToolkit::tstb( bool ok )
{
    // If the initialization command failed, then bail out.
    // If it succeeded, the phone simulator will send us the main menu next.
    if ( !ok )
        emit beginFailed();
}

void TrolltechSimToolkit::tcmd( const QString& value )
{
    // "*TCMD: size" unsolicited notification indicating the size of the command to fetch.
    uint posn = 6;
    int size = (int)QAtUtils::parseNumber( value, posn );
    fetchCommand( size );
}

void TrolltechSimToolkit::tcc( const QString& value )
{
    // "*TCC: type,data" unsolicited notification for call control events.
    uint posn = 5;
    QSimControlEvent::Type type =
        (QSimControlEvent::Type)QAtUtils::parseNumber( value, posn );
    QByteArray pdu = QAtUtils::fromHex( value.mid( (int)posn ) );
    emit controlEvent( QSimControlEvent::fromPdu( type, pdu ) );
}

TrolltechBandSelection::TrolltechBandSelection
        ( EricssonModemService *service )
    : QBandSelection( service->service(), service, Server )
{
    this->service = service;
}

TrolltechBandSelection::~TrolltechBandSelection()
{
}

void TrolltechBandSelection::requestBand()
{
    service->primaryAtChat()->chat
        ( "AT*TBAND?", this, SLOT(tbandGet(bool,QAtResult)) );
}

void TrolltechBandSelection::requestBands()
{
    service->primaryAtChat()->chat
        ( "AT*TBAND=?", this, SLOT(tbandList(bool,QAtResult)) );
}

void TrolltechBandSelection::setBand
        ( QBandSelection::BandMode mode, const QString& value )
{
    if ( mode == Automatic ) {
        service->primaryAtChat()->chat
            ( "AT*TBAND=0", this, SLOT(tbandSet(bool,QAtResult)) );
    } else {
        service->primaryAtChat()->chat
            ( "AT*TBAND=1,\"" + QAtUtils::quote(value) + "\"",
              this, SLOT(tbandSet(bool,QAtResult)) );
    }
}

void TrolltechBandSelection::tbandGet( bool, const QAtResult& result )
{
    QAtResultParser parser( result );
    if ( parser.next( "*TBAND:" ) ) {
        if (parser.readNumeric() != 0) {
            emit band( Manual, parser.readString() );
        } else {
            emit band( Automatic, QString() );
        }
    } else {
        emit band( Automatic, QString() );
    }
}

void TrolltechBandSelection::tbandSet( bool, const QAtResult& result )
{
    emit setBandResult( (QTelephony::Result)result.resultCode() );
}

void TrolltechBandSelection::tbandList( bool, const QAtResult& result )
{
    QStringList list;
    QAtResultParser parser( result );
    while ( parser.next( "*TBAND:" ) ) {
        QString name = parser.readString();
        if (!name.isEmpty())
            list += name;
    }
    emit bands( list );
}
