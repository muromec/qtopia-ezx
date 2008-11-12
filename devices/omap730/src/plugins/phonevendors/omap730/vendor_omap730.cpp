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

#include "vendor_omap730_p.h"
#include <qmodemindicators.h>
#include <qatutils.h>

static bool supportsStk = false;

Omap730CallProvider::Omap730CallProvider( QModemService *service )
    : QModemCallProvider( service )
{
    service->primaryAtChat()->registerNotificationType
        ( "%CPI:", this, SLOT(cpiNotification(QString)) );
    service->primaryAtChat()->registerNotificationType
        ( "%CNAP:", this, SLOT(cnapNotification(QString)) );
}

Omap730CallProvider::~Omap730CallProvider()
{
}

QModemCallProvider::AtdBehavior Omap730CallProvider::atdBehavior() const
{
    // When ATD reports OK, it indicates that it is back in command
    // mode and a %CPI notification will indicate when we are connected.
    return AtdOkIsDialingWithStatus;
}

void Omap730CallProvider::abortDial( uint , QPhoneCall::Scope )
{
    // Use the ATH command to abort outgoing calls, instead of AT+CHLD=1.
    atchat()->chat( "ATH" );
}

void Omap730CallProvider::cpiNotification( const QString& msg )
{
    // Call progress notification for the OMAP730 device.
    // %CPI: <id>,<msgType>,<ibt>,<tch>,<dir>,<mode>,<number>,<ton>,<alpha>
    // where <id> is the call identifier, and <msgType> is one of:
    // 0 = SETUP, 1 = DISCONNECT, 2 = ALERT, 3 = PROCEED,
    // 4 = SYNCHRONIZATION, 5 = PROGRESS, 6 = CONNECTED,
    // 7 = RELEASE, 8 = REJECT
    uint posn = 5;
    uint identifier = QAtUtils::parseNumber( msg, posn );
    uint status = QAtUtils::parseNumber( msg, posn );
    QModemCall *call = callForIdentifier( identifier );

    if ( status == 6 && call &&
         ( call->state() == QPhoneCall::Dialing ||
           call->state() == QPhoneCall::Alerting ) ) {

        // This is an indication that a "Dialing" connection
        // is now in the "Connected" state.
        call->setConnected();

    } else if ( ( status == 1 || status == 7 ) && call &&
                ( call->state() == QPhoneCall::Dialing ||
                  call->state() == QPhoneCall::Alerting ) ) {

        // We never managed to connect.
        hangupRemote( call );

    } else if ( status == 2 && call &&
                call->state() == QPhoneCall::Dialing ) {

        // Call is moving from Dialing to Alerting.
        call->setState( QPhoneCall::Alerting );

    } else if ( ( status == 1 || status == 7 ) && call &&
                ( call->state() == QPhoneCall::Connected ||
                  call->state() == QPhoneCall::Hold ) ) {

        // This is an indication that the connection has been lost.
        hangupRemote( call );

    } else if ( ( status == 1 || status == 7 ) && call &&
                call->state() == QPhoneCall::Incoming ) {

        // This is an indication that an incoming call was missed.
        call->setState( QPhoneCall::Missed );

    } else if ( ( status == 2 || status == 4 ) && !call ) {

        // This is a newly waiting call.  Treat it the same as "RING".
        QAtUtils::skipField( msg, posn );
        QAtUtils::skipField( msg, posn );
        QAtUtils::skipField( msg, posn );
        uint mode = QAtUtils::parseNumber( msg, posn );
        QString callType;
        if ( mode == 1 || mode == 6 || mode == 7 )
            callType = "Data";  // No tr
        else if ( mode == 2 || mode == 8 )
            callType = "Fax";   // No tr
        else
            callType = "Voice"; // No tr
        QString number = QAtUtils::nextString( msg, posn );
        uint type = QAtUtils::parseNumber( msg, posn );
        ringing( QAtUtils::decodeNumber( number, type ), callType, identifier );

    }
}

void Omap730CallProvider::cnapNotification( const QString& msg )
{
    // Calling name presentation from the network.
    uint posn = 6;
    QAtUtils::skipField( msg, posn );	    // pres_mode
    QAtUtils::skipField( msg, posn );	    // dcs
    QAtUtils::skipField( msg, posn );	    // name_length
    QString name = QAtUtils::nextString( msg, posn );
    QModemCall *call = incomingCall();
    if ( call )
        call->emitNotification( QPhoneCall::CallingName, name );
}

Omap730SimToolkit::Omap730SimToolkit( QModemService *service )
    : QModemSimToolkit( service )
{
    supportsStk = false;
    lastCommand.setType( QSimCommand::NoCommand );
    mainMenu = lastCommand;
    lastResponseWasExit = false;

    service->primaryAtChat()->registerNotificationType
        ( "%SATI:", this, SLOT(satiNotification(QString)) );
    service->primaryAtChat()->registerNotificationType
	( "%SATN:", this, SLOT(satnNotification(QString)) );
}

Omap730SimToolkit::~Omap730SimToolkit()
{
}

void Omap730SimToolkit::initialize()
{
    // We don't need to do anything here, because SIM toolkit initialization
    // happens during the detection code.
    initializationDone();
}

void Omap730SimToolkit::begin()
{
    if ( !supportsStk ) {

	// SIM toolkit functionality is not available.
	emit beginFailed();

    } else if ( lastCommand.type() == QSimCommand::SetupMenu ) {

	// We just fetched the main menu, so return what we fetched.
	emit command( lastCommand );

    } else if ( mainMenu.type() == QSimCommand::SetupMenu ) {

	// We have a cached main menu from a previous invocation.
	lastCommand = mainMenu;
	lastCommandBytes = mainMenuBytes;
	emit command( mainMenu );

    } else {

	// The SIM toolkit is in an unknown state, so we cannot proceed.
	// If the OMAP730 could perform a proper STK reset, we might have
	// been able to do something.
	emit beginFailed();

    }
}

void Omap730SimToolkit::sendResponse( const QSimTerminalResponse& resp )
{
    if ( resp.command().type() == QSimCommand::SelectItem &&
         resp.result() == QSimTerminalResponse::BackwardMove ) {
        lastResponseWasExit = true;
    } else {
        lastResponseWasExit = false;
    }
    service()->primaryAtChat()->chat
        ( "AT%SATR=\"" + QAtUtils::toHex( resp.toPdu() ) + "\"" );
}

void Omap730SimToolkit::sendEnvelope( const QSimEnvelope& env )
{
    service()->primaryAtChat()->chat
        ( "AT%SATE=\"" + QAtUtils::toHex( env.toPdu() ) + "\"" );
}

void Omap730SimToolkit::satiNotification( const QString& msg )
{
    // SIM toolkit command indication.
    QByteArray bytes = QAtUtils::fromHex( msg.mid(6) );
    if ( bytes.size() > 0 ) {

        lastCommandBytes = bytes;
        lastCommand = QSimCommand::fromPdu( bytes );
        if ( lastCommand.type() == QSimCommand::SetupMenu ) {
            // Cache the main menu, because we won't get it again!
            mainMenuBytes = bytes;
            mainMenu = lastCommand;
        }
        emitCommandAndRespond( lastCommand );

    } else if ( lastResponseWasExit &&
                mainMenu.type() == QSimCommand::SetupMenu ) {

        // We exited from a submenu and we got an empty "%SATI"
        // response.  This is the OMAP730's way of telling us that we
        // now need to display the main menu.  It would have been
        // better if the OMAP730 resent the menu to us itself.
        lastCommandBytes = mainMenuBytes;
        lastCommand = mainMenu;
        emit command( lastCommand );

    }
}

void Omap730SimToolkit::satnNotification( const QString& )
{
    // Nothing to do here at present.  Just ignore the %SATN notifications.
}

Omap730PhoneBook::Omap730PhoneBook( QModemService *service )
    : QModemPhoneBook( service )
{
}

Omap730PhoneBook::~Omap730PhoneBook()
{
}

bool Omap730PhoneBook::hasEmptyPhoneBookIndex() const
{
    return true;
}

Omap730PinManager::Omap730PinManager( QModemService *service )
    : QModemPinManager( service )
{
}

Omap730PinManager::~Omap730PinManager()
{
}

bool Omap730PinManager::emptyPinIsReady() const
{
    return true;
}

Omap730ModemService::Omap730ModemService
        ( const QString& service, QSerialIODeviceMultiplexer *mux,
          QObject *parent )
    : QModemService( service, mux, parent )
{
    connect( this, SIGNAL(resetModem()), this, SLOT(reset()) );

    // Register for "%CSQ" notifications to get signal quality updates.
    primaryAtChat()->registerNotificationType
        ( "%CSQ:", this, SLOT(csq(QString)), true );

    // Turn on SIM toolkit support in the modem.  This must be done
    // very early in the process, to ensure that it happens before
    // the first AT+CFUN command.
    chat( "AT%SATC=1,\"FFFFFFFFFF\"", this, SLOT(configureDone(bool)) );

    // Turn on call progress indications, with phone number information.
    chat( "AT%CPI=2" );
}

Omap730ModemService::~Omap730ModemService()
{
}

void Omap730ModemService::initialize()
{
    if ( !supports<QSimToolkit>() )
        addInterface( new Omap730SimToolkit( this ) );

    if ( !supports<QPhoneBook>() )
        addInterface( new Omap730PhoneBook( this ) );

    if ( !supports<QPinManager>() )
        addInterface( new Omap730PinManager( this ) );

    if ( !callProvider() )
        setCallProvider( new Omap730CallProvider( this ) );

    QModemService::initialize();
}

void Omap730ModemService::csq( const QString& msg )
{
    // Automatic signal quality update from the OMAP730.
    uint posn = 6;
    uint rssi = QAtUtils::parseNumber( msg, posn );
    indicators()->setSignalQuality( (int)rssi, 31 );
}

void Omap730ModemService::configureDone( bool ok )
{
    supportsStk = ok;
}

void Omap730ModemService::reset()
{
    // Turn on dynamic signal quality notifications.
    chat( "AT%CSQ=1" );

    // Turn on "%CNAP" notifications, which supply the caller's
    // name on an call.  Only supported on some networks.
    chat( "AT%CNAP=1" );
}
