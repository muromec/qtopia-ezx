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

#ifndef _GSMKEYACTIONS_H_
#define _GSMKEYACTIONS_H_

#include "qtopiaserverapplication.h"
#include "gsmkeyfilter.h"
#include "qtelephonynamespace.h"

class GsmKeyActionsPrivate;
class QDialOptions;
class QAbstractDialerScreen;

class GsmKeyActions : public QObject
{
    Q_OBJECT
public:
    explicit GsmKeyActions( QObject *parent = 0 );
    virtual ~GsmKeyActions();

    void setDialer( QAbstractDialerScreen *dialer );
    bool isGsmNumber( const QString& number );

private slots:
    void imeiRequest();
    void imeiReply( const QString& name, const QString& value );
    void changePassword( const QString& request );
    void changePin( const QString& request );
    void changePin2( const QString& request );
    void unblockPin( const QString& request );
    void unblockPin2( const QString& request );
    void modifyDial( QDialOptions& options, bool& handledAlready );
    void filterKeys( const QString& input, bool& filtered );
    void filterSelect( const QString& input, bool& filtered );
    void testKeys( const QString& input, bool& filterable );
    void callerIdRestriction
        ( GsmKeyFilter::ServiceAction action, const QStringList& args );
    void callForwarding
        ( GsmKeyFilter::ServiceAction action, const QStringList& args );
    void callBarring
        ( GsmKeyFilter::ServiceAction action, const QStringList& args );
    void callerIdPresentation
        ( GsmKeyFilter::ServiceAction action, const QStringList& args );
    void connectedIdPresentation
        ( GsmKeyFilter::ServiceAction action, const QStringList& args );
    void holdOrSwap();
    void releaseAllAcceptIncoming();
    void supplementaryServiceResult( QTelephony::Result result );

private:
    GsmKeyActionsPrivate *d;

    bool checkNewPins( const QString& title, const QStringList& pins );
    void sendServiceToNetwork
        ( GsmKeyFilter::ServiceAction action, const QStringList& args,
          const QString& title = QString() );
};

#endif // _GSMKEYACTIONS_H_
