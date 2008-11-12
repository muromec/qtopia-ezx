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

#include "messagecontrol.h"
#include <qcommservicemanager.h>
#include <QString>
#include <QSettings>
#include <QByteArray>
#include <QDataStream>

/*! \internal */
MessageControl::MessageControl() :
    phoneValueSpace("/Communications/Messages"),
    smsMemFull("/Telephony/Status/SMSMemoryFull"),
#ifdef QTOPIA_CELL
    smsreq(0),
#endif
    messageCountUpdate(0),
    channel("QPE/System"),
    smsCount(0), mmsCount(0), systemCount(0),
    smsIsFull(false), prevSmsMemoryFull(0)
{
    QSettings setting("Trolltech", "qpe");
    setting.beginGroup("Messages");
    smsCount = setting.value("MissedSMSMessages", 0).toInt();
    mmsCount = setting.value("MissedMMSMessages", 0).toInt();
    systemCount = setting.value("MissedSystemMessages", 0).toInt();

    mgr = new QCommServiceManager( this );
    connect( mgr, SIGNAL(servicesChanged()), this,
             SLOT(telephonyServicesChanged()) );
    telephonyServicesChanged(); // Check to see if we already have SMS.

    connect(&channel, SIGNAL(received(QString,QByteArray)),
            this, SLOT(sysMessage(QString,QByteArray)) );

    connect(&smsMemFull, SIGNAL(contentsChanged()),
            this, SLOT(smsMemoryFullChanged()) );
    prevSmsMemoryFull = smsMemFull.value().toInt();

    // React to changes in incoming message count
    messageCountUpdate = new QtopiaIpcAdaptor("QPE/Messages/MessageCountUpdated");
    QtopiaIpcAdaptor::connect(messageCountUpdate, MESSAGE(changeValue()), 
                              this, SLOT(messageCountChanged()));

    doNewCount(false);
    messageCountChanged();
}

void MessageControl::doNewCount(bool write, bool fromSystem, bool notify)
{
    if(write) {
        QSettings setting("Trolltech", "qpe");
        setting.beginGroup( "Messages" );
        setting.setValue( "MissedSMSMessages", smsCount);
        setting.setValue( "MissedMMSMessages", mmsCount);
        setting.setValue( "MissedSystemMessages", systemCount );
    }

    int count = messageCount();
    if (count) {
        // Don't update the NewMessages VS variable until the messages have been received
        if ( !fromSystem )
            emit messageCount(messageCount(), smsFull(), false, notify);
        else
            emit messageCount(messageCount(), false, true, notify);
    } else {
        // We have to do this, since our count reset notification doesn't arrive until after
        // the notification that we should update the message count...
        emit messageCount(0, false, false, notify);
        messageCountChanged();
    }
}

/*! Returns the MessageControl instance. */
MessageControl *MessageControl::instance()
{
    static MessageControl *mc = 0;
    if(!mc)
        mc = new MessageControl;
    return mc;
}

void MessageControl::smsUnreadCountChanged()
{
#ifdef QTOPIA_CELL
    // Report on the number of unread SMS messages.
    int c = (smsreq ? smsreq->unreadCount() : 0);
    smsIsFull = ( smsreq && smsreq->usedMessages() != -1 ?
        ( smsreq->usedMessages() >= smsreq->totalMessages() ) : false );
    if (c != smsCount) {
        smsCount = c;
        if (!smsreq->unreadList().isEmpty())
            smsId = smsreq->unreadList().last();
        doNewCount();
    }
#endif
}

void MessageControl::telephonyServicesChanged()
{
#ifdef QTOPIA_CELL
    // Create the SMS request object if the service has started.
    if ( !smsreq ) {
        if ( mgr->supports<QSMSReader>().size() > 0 ) {
            smsreq = new QSMSReader( QString(), this );
            connect( smsreq, SIGNAL(unreadCountChanged()),
                     this, SLOT(smsUnreadCountChanged()) );

            // Tell the SIM how many new messages we have
            smsreq->setUnreadCount(smsCount);
        }
    }
#endif
}

/*! Returns the number of unread messages */
int MessageControl::messageCount() const
{
    return smsCount + mmsCount + systemCount;
}

/*!
  Returns true if the SMS memory is full.
 */
bool MessageControl::smsFull() const
{
    return smsIsFull;
}

/*!
  Returns the id of the most recently received SMS message
*/
QString MessageControl::lastSmsId() const
{
    return smsId;
}

void MessageControl::sysMessage(const QString& message, const QByteArray &data)
{
    QDataStream stream( data );

    if ( message == "newMmsCount(int)") {
         int count;
         stream >> count;
         if (count != mmsCount) {
             mmsCount = count;
             doNewCount(true,false,false);
         }
    }
    else if( message == "newSystemCount(int)" ){
        int count;
        stream >> count;
        if( count != systemCount ){
            systemCount = count;
            doNewCount( true, true, true);
        }
    }
}

void MessageControl::smsMemoryFullChanged()
{
    // Check for the "message rejected state", so we can report it.
    // The "full" state will be reported via messageCount() when new
    // message arrivals are detected.
    int fullState = smsMemFull.value().toInt();
    if ( fullState != prevSmsMemoryFull ) {
        prevSmsMemoryFull = fullState;

        emit smsMemoryFull( fullState != 0 );
        if ( fullState == 2 )
            emit messageRejected();
    }
}

void MessageControl::messageCountChanged()
{
    // The incoming message count has been updated externally
    // Update the valueSpace variable
    phoneValueSpace.setAttribute("NewMessages", messageCount());
}

