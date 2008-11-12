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

#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>

#include "qlibrary.h"
#include "qtopiaapplication.h"

#include "qtopiaipcenvelope.h"
#include "qtopiaservices.h"

#include "qalarmserver_p.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

struct timerEventItem {
    uint UTCtime;
    QString channel, message;
    int data;
    bool operator==( const timerEventItem &right ) const
    {
        return ( UTCtime == right.UTCtime
                 && channel == right.channel
                 && message == right.message
                 && data == right.data );
    }
};

class TimerReceiverObject : public QObject
{
    Q_OBJECT
public:
    TimerReceiverObject() : timerId(0) { }
    ~TimerReceiverObject() { }

    void resetTimer();
    void setTimerEventItem();
    void deleteTimer();
    void killTimers() { if (timerId) killTimer(timerId); timerId = 0; }

protected:
    void timerEvent( QTimerEvent *te );
private:
    QString atfilename;
    int timerId;
};

TimerReceiverObject *timerEventReceiver = NULL;
QList<timerEventItem*> timerEventList;
timerEventItem *nearestTimerEvent = NULL;

static AlarmServerService *alarmServer = 0;

// set the timer to go off on the next event in the list
void setNearestTimerEvent()
{
    nearestTimerEvent = NULL;
    QList<timerEventItem*>::const_iterator it = timerEventList.begin();
    if ( it != timerEventList.end() )
        nearestTimerEvent = *it;
    for ( ; it != timerEventList.end(); ++it ) {
        if ( (*it)->UTCtime < nearestTimerEvent->UTCtime )
            nearestTimerEvent = *it;
    }
    if (nearestTimerEvent)
        timerEventReceiver->resetTimer();
    else
        timerEventReceiver->deleteTimer();
}


//store current state to file
//Simple implementation. Should run on a timer.

static void saveState()
{
    QString savefilename = Qtopia::applicationFileName( "AlarmServer", "saveFile" );
    if ( timerEventList.isEmpty() ) {
        unlink( savefilename.toAscii().constData() );
        return;
    }

    QFile savefile(savefilename+".new");
    if ( savefile.open(QIODevice::WriteOnly) ) {
        QDataStream ds( &savefile );

        //save
        timerEventItem *item;
        foreach (item, timerEventList) {
            ds << (quint32)item->UTCtime;
            ds << item->channel;
            ds << item->message;
            ds << item->data;
        }

        savefile.close();
        unlink( savefilename.toAscii().constData() );
        QDir d; d.rename(savefilename+".new",savefilename);
    }
}

/*
  Sets up the alarm server. Restoring to previous state (session management).
 */
QTOPIA_EXPORT void Qtopia_initAlarmServer()
{
    // read autosave file and put events in timerEventList
    QString savefilename = Qtopia::applicationFileName( "AlarmServer", "saveFile" );

    QFile savefile(savefilename);
    if ( savefile.open(QIODevice::ReadOnly) ) {
        QDataStream ds( &savefile );
        while ( !ds.atEnd() ) {
            timerEventItem *newTimerEventItem = new timerEventItem;
            quint32 UTCtime;
            ds >> UTCtime;
            newTimerEventItem->UTCtime = UTCtime;
            ds >> newTimerEventItem->channel;
            ds >> newTimerEventItem->message;
            ds >> newTimerEventItem->data;
            timerEventList.append( newTimerEventItem );
        }
        savefile.close();
        if (!timerEventReceiver)
            timerEventReceiver = new TimerReceiverObject;
        setNearestTimerEvent();
    }
    if ( !alarmServer )
        alarmServer = new AlarmServerService;
}

static const char* atdir = "/var/spool/at/";

static bool triggerAtd( bool writeHWClock = false )
{
    QFile trigger(QString(atdir) + "trigger"); // No tr
    if ( trigger.open(QIODevice::WriteOnly) ) {

        const char* data =
// ### removed this define as there's no way of updating the hardware clock if our atdaemon doesn't do it.
// Should probably revise this into a more general solution though.
        //custom atd only writes HW Clock if we write a 'W'
            ( writeHWClock ) ? "W\n" : "\n";
        int len = strlen(data);
        int total_written = trigger.write(data,len);
        if ( total_written != len ) {
            QMessageBox::critical( 0, qApp->translate( "AlarmServer",  "Out of Space" ),
                                   qApp->translate( "AlarmServer", "<qt>Unable to schedule alarm. Free some memory and try again.</qt>" ) );
            trigger.close();
            QFile::remove( trigger.fileName() );
            return false;
        }
        return true;
    }
    return false;
}

void TimerReceiverObject::deleteTimer()
{
    if ( !atfilename.isEmpty() ) {
        unlink( atfilename.toAscii().constData() );
        atfilename = QString();
        triggerAtd( false );
    }
}

void TimerReceiverObject::resetTimer()
{
    const int maxsecs = 2147000;
    int total_written;
    QDateTime nearest;
    nearest.setTime_t(nearestTimerEvent->UTCtime);
    QDateTime now = QDateTime::currentDateTime();
    if ( nearest < now )
        nearest = now;
    int secs = now.secsTo(nearest);
    if ( secs > maxsecs ) {
        // too far for millisecond timing
        secs = maxsecs;
    }

    // System timer (needed so that we wake from deep sleep),
    // from the Epoch in seconds.
    //
    int at_secs = nearest.toTime_t();
    QString fn = atdir + QString::number(at_secs) + "."
                 + QString::number(::getpid());
    if ( fn != atfilename ) {
        QFile atfile(fn+".new");
        if ( atfile.open(QIODevice::WriteOnly) ) {
            // just wake up and delete the at file
            QString cmd = "#!/bin/sh\nrm " + fn;
            total_written = atfile.write(cmd.toLatin1(),cmd.length());
            if ( total_written != int(cmd.length()) ) {
                QMessageBox::critical( 0, tr("Out of Space"),
                                       tr("<qt>Unable to schedule alarm. "
                                          "Please free up space and try again</qt>"));
                atfile.close();
                QFile::remove( atfile.fileName() );
                return;
            }
            atfile.close();
            unlink( atfilename.toAscii().constData() );
            QDir d; d.rename(fn+".new",fn);
            chmod(fn.toLatin1(),0755);
            atfilename = fn;
            triggerAtd( false );
        } else {
            qWarning("Cannot open atd file %s",(const char *)fn.toLatin1());
        }
    }
    // Qt timers (does the actual alarm)
    // from now in milliseconds
    //
    static bool startup = true;
    if (secs < 5 && startup)   // To delay the alarm when Qtopia first starts.
        secs = 5;
    timerId = startTimer( 1000 * secs + 500 );
    startup = false;
}

void TimerReceiverObject::timerEvent( QTimerEvent * )
{
    bool needSave = false;
    if (timerId){
        killTimer(timerId);
        timerId = 0;
    }
    if (nearestTimerEvent) {
        if ( nearestTimerEvent->UTCtime
             <= QDateTime::currentDateTime().toTime_t() ) {
            QDateTime time;
            time.setTime_t(nearestTimerEvent->UTCtime);
            QString channel = nearestTimerEvent->channel;
            if ( !channel.contains( QChar('/') ) ) {
                QtopiaServiceRequest e( channel, nearestTimerEvent->message );
                e << time << nearestTimerEvent->data;
                e.send();

            } else {
                QtopiaIpcEnvelope e( channel, nearestTimerEvent->message );
                e << time << nearestTimerEvent->data;
            }

            timerEventList.removeAll(nearestTimerEvent);
            delete nearestTimerEvent;
            nearestTimerEvent = 0;
            needSave = true;
        }
        setNearestTimerEvent();
    } else {
        resetTimer();
    }
    if ( needSave )
        saveState();
}

/*!
  Schedules an alarm to go off at (or soon after) time \a when. When
  the alarm goes off, the \l {Qtopia IPC Layer}{Qtopia IPC} \a message will
  be sent to \a channel, with \a data as a parameter.

  If \a channel does not contain the \c{/} character, it will be interpreted
  as a QCop service name.  For example, specifying \c{Calendar} as \a channel
  will direct the alarm to the configured calendar program.

  Note that these alarms fire regardless of whether the application that
  created them is running, indeed for some hardware, even if the device
  is not powered on.

  If this function is called with exactly the same data as a previous
  call the subsequent call is ignored, so there is only ever one alarm
  with a given set of parameters.

  \sa deleteAlarm()
*/
void Qtopia::addAlarm ( QDateTime when, const QString& channel,
                             const QString& message, int data)
{
    if ( qApp->type() == QApplication::GuiServer ) {
        bool needSave = false;
        // Here we are the server so either it has been directly called from
        // within the server or it has been sent to us from a client via QCop
        if (!timerEventReceiver)
            timerEventReceiver = new TimerReceiverObject;

        timerEventItem *newTimerEventItem = new timerEventItem;
        newTimerEventItem->UTCtime = when.toTime_t();
        newTimerEventItem->channel = channel;
        newTimerEventItem->message = message;
        newTimerEventItem->data = data;
        // explore the case of already having the event in here...
        foreach (timerEventItem *item, timerEventList) {
            if ( *item == *newTimerEventItem ) {
                delete newTimerEventItem;
                return;
            }
        }
        // if we made it here, it is okay to add the item...
        timerEventList.append( newTimerEventItem );
        needSave = true;
        // quicker than using setNearestTimerEvent()
        if ( nearestTimerEvent ) {
            if (newTimerEventItem->UTCtime < nearestTimerEvent->UTCtime) {
                nearestTimerEvent = newTimerEventItem;
                timerEventReceiver->killTimers();
                timerEventReceiver->resetTimer();
            }
        } else {
            nearestTimerEvent = newTimerEventItem;
            timerEventReceiver->resetTimer();
        }
        if ( needSave )
            saveState();
    } else {
        QtopiaIpcEnvelope e( "QPE/AlarmServer", "addAlarm(QDateTime,QString,QString,int)" );
        e << when << channel << message << data;
    }
}

/*!
  Deletes previously scheduled alarms which match \a when, \a channel,
  \a message, and \a data.

  Passing null values for \a when, \a channel, or for the \l {Qtopia IPC Layer}{Qtopia IPC} \a message, acts as a wildcard meaning "any".
  Similarly, passing -1 for \a data indicates "any".

  If there is no matching alarm, nothing happens.

  \sa addAlarm()

*/
void Qtopia::deleteAlarm (QDateTime when, const QString& channel, const QString& message, int data)
{
    if ( qApp->type() == QApplication::GuiServer) {
        bool needSave = false;
        if ( timerEventReceiver ) {
            timerEventReceiver->killTimers();

            // iterate over the list of events
            QMutableListIterator<timerEventItem*> it( timerEventList );
            uint deleteTime = when.toTime_t();
            bool updatenearest = false;
            while ( it.hasNext() ) {
                timerEventItem *event = it.next();
                // if its a match, delete it
                if ( ( event->UTCtime == deleteTime || when.isNull() )
                    && ( channel.isNull() || event->channel == channel )
                    && ( message.isNull() || event->message == message )
                    && ( data==-1 || event->data == data ) )
                {
                    // if it's first, then we need to update the timer
                    if ( event == nearestTimerEvent ) {
                        updatenearest = true;
                        nearestTimerEvent = 0;
                    }
                    it.remove();
                    delete event;
                    needSave = true;
                }
            }

            if ( updatenearest )
                setNearestTimerEvent();
            else if ( nearestTimerEvent )
                timerEventReceiver->resetTimer();
        }
        if ( needSave )
            saveState();
    } else {
        QtopiaIpcEnvelope e( "QPE/AlarmServer", "deleteAlarm(QDateTime,QString,QString,int)" );
        e << when << channel << message << data;
    }
}

/*!
  Writes the system clock to the hardware clock.
*/
void Qtopia::writeHWClock()
{
    if ( !triggerAtd( true ) ) {
        // atd not running? set it ourselves
        system("/sbin/hwclock -w"); // ##### UTC?
    }
}

/*!
    \service AlarmServerService AlarmServer
    \brief Provides the Qtopia AlarmServer service.

    The \i AlarmServer service is used by Qtopia applications to add and
    delete alarm events.  It is normally accessed through Qtopia::addAlarm()
    and Qtopia::deleteAlarm().

    Messages for the alarm server are sent on the \c{QPE/AlarmServer}
    QCop channel.  The alarm server is started automatically by the
    Qtopia server application.

    \sa Qtopia::addAlarm(), Qtopia::deleteAlarm()
*/

/*!
  \fn AlarmServerService::AlarmServerService( QObject *parent = 0 )

  Create an AlarmServerService instance with the passed \a parent.
*/

/*!
  \fn AlarmServerService::~AlarmServerService()

  Destroy the AlarmServerService instance.
*/

/*!
  Schedules an alarm to go off at (or soon after) time \a when. When
  the alarm goes off, the \l {Qtopia IPC Layer}{Qtopia IPC} message \a msg will
  be sent to \a channel, with \a data as a parameter.

  This slot corresponds to the QCop message
  \c{addAlarm(QDateTime,QString,QString,int)} on the
  \c{QPE/AlarmServer} channel.

  \sa Qtopia::addAlarm()
*/
void AlarmServerService::addAlarm
    ( QDateTime when, const QString& channel, const QString& msg, int data )
{
    Qtopia::addAlarm( when, channel, msg, data );
}

/*!
  Deletes previously scheduled alarms which match \a when, \a channel,
  \a msg, and \a data.

  This slot corresponds to the QCop message
  \c{deleteAlarm(QDateTime,QString,QString,int)} on the
  \c{QPE/AlarmServer} channel.

  \sa Qtopia::deleteAlarm()
*/
void AlarmServerService::deleteAlarm
    ( QDateTime when, const QString& channel, const QString& msg, int data )
{
    Qtopia::deleteAlarm( when, channel, msg, data );
}

/*!
  Message that indicates when the clock application turns the daily alarm
  on or off, according to \a flag.

  This slot corresponds to the QCop message \c{dailyAlarmEnabled(bool)} on
  the \c{QPE/AlarmServer} channel.
*/
void AlarmServerService::dailyAlarmEnabled( bool )
{
    // This is handled in the Qtopia server, not here.  We place
    // this here for documentation purposes only.
}

#include "qalarmserver.moc"
