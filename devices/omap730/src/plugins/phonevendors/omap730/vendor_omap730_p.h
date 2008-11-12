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

#ifndef	VENDOR_OMAP730_P_H
#define	VENDOR_OMAP730_P_H

#include <qmodemservice.h>
#include <qmodemcall.h>
#include <qmodemcallprovider.h>
#include <qmodemsimtoolkit.h>
#include <qmodemphonebook.h>
#include <qmodempinmanager.h>


class Omap730CallProvider : public QModemCallProvider
{
    Q_OBJECT
public:
    Omap730CallProvider( QModemService *service );
    ~Omap730CallProvider();

protected:
    QModemCallProvider::AtdBehavior atdBehavior() const;
    void abortDial( uint modemIdentifier, QPhoneCall::Scope scope );

private slots:
    void cpiNotification( const QString& msg );
    void cnapNotification( const QString& msg );
};

class Omap730SimToolkit : public QModemSimToolkit
{
    Q_OBJECT
public:
    Omap730SimToolkit( QModemService *service );
    ~Omap730SimToolkit();

public slots:
    void initialize();
    void begin();
    void sendResponse( const QSimTerminalResponse& resp );
    void sendEnvelope( const QSimEnvelope& env );

private slots:
    void satiNotification( const QString& msg );
    void satnNotification( const QString& msg );

private:
    QSimCommand lastCommand;
    QByteArray lastCommandBytes;
    QSimCommand mainMenu;
    QByteArray mainMenuBytes;
    bool lastResponseWasExit;
};

class Omap730PhoneBook : public QModemPhoneBook
{
    Q_OBJECT
public:
    Omap730PhoneBook( QModemService *service );
    ~Omap730PhoneBook();

protected:
    bool hasEmptyPhoneBookIndex() const;
};

class Omap730PinManager : public QModemPinManager
{
    Q_OBJECT
public:
    Omap730PinManager( QModemService *service );
    ~Omap730PinManager();

protected:
    bool emptyPinIsReady() const;
};

class Omap730ModemService : public QModemService
{
    Q_OBJECT
public:
    Omap730ModemService
        ( const QString& service, QSerialIODeviceMultiplexer *mux,
          QObject *parent = 0 );
    ~Omap730ModemService();

    void initialize();

private slots:
    void csq( const QString& msg );
    void configureDone( bool ok );
    void reset();
};

#endif
