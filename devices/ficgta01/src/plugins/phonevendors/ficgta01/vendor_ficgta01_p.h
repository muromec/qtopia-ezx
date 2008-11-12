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

#ifndef	VENDOR_FICGTA01_P_H
#define	VENDOR_FICGTA01_P_H

#include <qmodemservice.h>
#include <qmodemcall.h>
#include <qmodemcallprovider.h>
#include <qmodemsimtoolkit.h>
#include <qmodemphonebook.h>
#include <qmodempinmanager.h>
#include <qmodempreferrednetworkoperators.h>
#include <qbandselection.h>
#include <qvibrateaccessory.h>
#include <qmodemnetworkregistration.h>

#include <qmodemsiminfo.h>

#include <qmodemcallvolume.h>

#include <alsa/asoundlib.h>

class Ficgta01CallProvider : public QModemCallProvider
{
    Q_OBJECT
public:
    Ficgta01CallProvider( QModemService *service );
    ~Ficgta01CallProvider();

protected:
    QModemCallProvider::AtdBehavior atdBehavior() const;
    void abortDial( uint modemIdentifier, QPhoneCall::Scope scope );

private slots:
    void cpiNotification( const QString& msg );
    void cnapNotification( const QString& msg );
};

class Ficgta01SimToolkit : public QModemSimToolkit
{
    Q_OBJECT
public:
    Ficgta01SimToolkit( QModemService *service );
    ~Ficgta01SimToolkit();

public slots:
    void initialize();
    void begin();
    void sendResponse( const QSimTerminalResponse& resp );
    void sendEnvelope( const QSimEnvelope& env );

private slots:
    void sataNotification( const QString& msg );
    void satnNotification( const QString& msg );

private:
    QSimCommand lastCommand;
    QByteArray lastCommandBytes;
    QSimCommand mainMenu;
    QByteArray mainMenuBytes;
    bool lastResponseWasExit;
};

class Ficgta01PhoneBook : public QModemPhoneBook
{
    Q_OBJECT
public:
    Ficgta01PhoneBook( QModemService *service );
    ~Ficgta01PhoneBook();

protected:
    bool hasModemPhoneBookCache() const;
    bool hasEmptyPhoneBookIndex() const;


private slots:
    void cstatNotification( const QString& msg );

private:
    QModemService *service;
};

class Ficgta01PinManager : public QModemPinManager
{
    Q_OBJECT
public:
    Ficgta01PinManager( QModemService *service );
    ~Ficgta01PinManager();

protected:
    bool emptyPinIsReady() const;

};

class Ficgta01BandSelection : public QBandSelection
{
    Q_OBJECT
public:
    Ficgta01BandSelection( QModemService *service );
    ~Ficgta01BandSelection();

public slots:
    void requestBand();
    void requestBands();
    void setBand( QBandSelection::BandMode mode, const QString& value );

private slots:
    void bandQuery( bool ok, const QAtResult& result );
    void bandList( bool ok, const QAtResult& result );
    void bandSet( bool ok, const QAtResult& result );

private:
    QModemService *service;
};

class Ficgta01ModemService : public QModemService
{
    Q_OBJECT
public:
    Ficgta01ModemService
        ( const QString& service, QSerialIODeviceMultiplexer *mux,
          QObject *parent = 0 );
    ~Ficgta01ModemService();

    void initialize();

private slots:
    void csq( const QString& msg );
    void firstCsqQuery();

    void ctzv( const QString& msg );
    void configureDone( bool ok );
    void reset();
    void suspend();
    void wake();
    void sendSuspendDone();
    void mcsqOff();
    void mcsqOn();
    
};

class  Ficgta01VibrateAccessory : public QVibrateAccessoryProvider
{
    Q_OBJECT
public:
     Ficgta01VibrateAccessory( QModemService *service );
    ~ Ficgta01VibrateAccessory();

public slots:
    void setVibrateNow( const bool value );
    void setVibrateOnRing( const bool value );
};



class Ficgta01CallVolume : public QModemCallVolume
{
      Q_OBJECT
public:
    
explicit Ficgta01CallVolume( Ficgta01ModemService *service);
~Ficgta01CallVolume();

public slots:
    void setSpeakerVolume( int volume );
    void setMicrophoneVolume( int volume );
    void setSpeakerVolumeRange(int,int);
    void setMicVolumeRange(int,int);

protected:
    bool hasDelayedInit() const;
    
private:
    Ficgta01ModemService *service;


};

class Ficgta01SimInfoPrivate;

class Ficgta01SimInfo : public QSimInfo
{
    Q_OBJECT
public:
    Ficgta01SimInfo(Ficgta01ModemService *service );
    ~Ficgta01SimInfo();
    
protected slots:
    void simInserted();
    void simRemoved();

private slots:
    void requestIdentity();
    void cimi( bool ok, const QAtResult& result );
    void serviceItemPosted( const QString& item );

private:
    Ficgta01SimInfoPrivate *d;

    static QString extractIdentity( const QString& content );
};

class Ficgta01PreferredNetworkOperators : public QModemPreferredNetworkOperators
{
    Q_OBJECT
public:
    explicit Ficgta01PreferredNetworkOperators( QModemService *service );
    ~Ficgta01PreferredNetworkOperators();
};

class Ficgta01ModemNetworkRegistration : public QModemNetworkRegistration
{
    Q_OBJECT
public:
    explicit Ficgta01ModemNetworkRegistration( QModemService *service );

protected:
 virtual QString setCurrentOperatorCommand
        ( QTelephony::OperatorMode mode, const QString& id,
          const QString& technology );
};


#endif
