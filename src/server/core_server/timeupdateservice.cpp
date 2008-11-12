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

#include "timeupdateservice.h"
#include <QtopiaApplication>
#include <QValueSpace>
#include <QtopiaIpcEnvelope>
#include <QtopiaService>
#include <QTimeZone>
#include "qtopiaserverapplication.h"

#include <sys/time.h>


TimeUpdater::TimeUpdater(int timeoffset, const QTimeZone& tz, bool chtime, bool chzone)
    : offset(timeoffset), zone(tz), updatetime(chtime), updatezone(chzone)
{
    prompt = 0;
}

void TimeUpdater::userCommit(int r)
{
    timer.stop();
    if (r == QMessageBox::Yes)
        commitUpdate();
}

void TimeUpdater::ask()
{
    delete prompt;
    if (!updatetime && !updatezone) {
        prompt = 0;
        return;
    }
    prompt = new QMessageBox(QMessageBox::Question, tr("Time Changed"), QString(), QMessageBox::Yes | QMessageBox::No);
    connect(prompt, SIGNAL(finished(int)), this, SLOT(userCommit(int)));
    if (updatetime)
        timer.start(1000,this);
    else
        timer.stop();
    timerEvent(0);
    prompt->show();
}
void TimeUpdater::timerEvent(QTimerEvent*)
{
    if (!updatetime) {
        if (!updatezone)
            return;
        prompt->setText(tr("<qt>The network time zone is %1. Set this new time zone?</qt>").arg(zone.name()));
    } else {
        QDateTime lt = zone.fromTime_t(time(0)+offset);
        QDateTime now = QDateTime::currentDateTime();
        QString localtime;
        if ( now.date() == lt.date() )
            localtime = QTimeString::localHMS(lt.time(),QTimeString::Long);
        else
            localtime = QTimeString::localYMDHMS(lt,QTimeString::Long);
        if (updatezone)
            localtime += " (" + zone.name() + ")";

        // XXX DEBUG
        //localtime += " " + QString::number(offset) + "s";

        prompt->setText(tr("<qt>The network time is %1. Set this new time?</qt>").arg(localtime));
    }
}

void TimeUpdater::commitUpdate()
{
    emit changeSystemTime(updatetime ? time(0)+offset : 0, zone.id());
}

/*!
    \service TimeUpdateService TimeUpdate
    \brief Provides the Qtopia TimeUpdate service.

    The \i TimeUpdate service monitors time and timezone data from sources
    such as the modem and updates the system time and timezone accordingly.

    The following settings control the automatic update:

    Trolltech/locale/Location/TimezoneAuto

    Trolltech/locale/Location/TimeAuto

    Trolltech/locale/Location/TimezoneAutoPrompt

    Trolltech/locale/Location/TimeAutoPrompt
*/

/*! \internal */
TimeUpdateService::TimeUpdateService()
    : QtopiaAbstractService("TimeUpdate", 0),
      lasttz(-1),externalTimeTimeStamp(0), externalTime(0), externalDstOffset(0),
      updater(0)
{
    publishAll();
}


/*! \internal */
void TimeUpdateService::changeSystemTime(uint newutc, QString newtz)
{
    // This should be the only place in Qtopia that does this...

    QSettings lconfig("Trolltech","locale");
    if (newtz != lconfig.value("Location/Timezone")) {
        lconfig.setValue("Location/Timezone",newtz);
        setenv( "TZ", newtz.toLatin1().constData(), 1 );
#if defined(QTOPIA_ZONEINFO_PATH)
        QString filename = QTOPIA_ZONEINFO_PATH + newtz;
#else
        QString filename = "/usr/share/zoneinfo/" + newtz;
#endif
        QString cmd = "cp -f " + filename + " /etc/localtime";
        system(cmd.toLocal8Bit().constData());
    }
    if (newutc) {
        if ( externalTimeTimeStamp ) {
            int deltatime = newutc - ::time(0);
            externalTimeTimeStamp += deltatime;
        }
        struct timeval myTv;
        myTv.tv_sec = newutc;
        myTv.tv_usec = 0;
        ::settimeofday( &myTv, 0 );
        Qtopia::writeHWClock();
    }
    // set the time (from system) and zone (given to their TZ) for everyone else...
    QtopiaIpcEnvelope setTimeZone( "QPE/System", "timeChange(QString)" );
    setTimeZone << newtz;
}

/*!
  Records an external time update from a source identified by \a sourceid. The recorded time is
  \a time (a UNIX time_t measuring UTC seconds since the start of 1970), in the given \a time_zone
  (the number of minutes east of GMT), with \a dstoffset minutes of daylight savings adjustment
  already applied to the \a time_zone.

  This time is recorded and may be used immediately or in the future upon user request.

  The \a sourceid is currently not used, so only one source should be enabled.
*/
void TimeUpdateService::storeExternalSource(QString sourceid,uint time,int time_zone,int dstoffset)
{
    // Allowing multiple sources would require changing the UI
    // to allow the user to select a source or sources to use,
    // possibly prioritized, and would require this class to
    // distinguish between different sources according to the user's wishes.
    //
    Q_UNUSED(sourceid); // ignored, just one supported

    if ( time ) {
        externalTimeTimeStamp = ::time(0);
        externalTime = time;
        externalDstOffset = dstoffset;
    }

    if ( time_zone != lasttz ) {
        lasttz = time_zone;
        externalTimeZone = QTimeZone::findFromMinutesEast(QDateTime::currentDateTime(),time_zone,externalDstOffset!=0).id();
    }

    updateFromExternalSources();
}


/*! \internal */
void TimeUpdateService::updateFromExternalSources()
{
    QSettings lconfig("Trolltech","locale");

    bool autotz = lconfig.value("Location/TimezoneAuto").toBool();
    bool autotm = lconfig.value("Location/TimeAuto").toBool();

    if ( !autotz && !autotm )
        return;

    bool ask_tz = lconfig.value("Location/TimezoneAutoPrompt").toBool();
    bool ask_time = lconfig.value("Location/TimeAutoPrompt").toBool();

    updateFromExternalSources(autotz, autotm, ask_tz, ask_time);
}

/*!
  Analyzes the most recent external time information and updates the
  time zone if \a autotz, and the time if \a autotm, prompting
  respectively if \a ask_tz and \a ask_time. Only a single prompt
  is ever used, if at all.

  To avoid annoying the user, changes of less than 60 seconds are
  applied without prompting if \a autotm is true.
*/
void TimeUpdateService::updateFromExternalSources(bool autotz, bool autotm, bool ask_tz, bool ask_time)
{
    // Somewhat arbitrary.
    //
    // Too low annoys the user.
    // Too high may cause confusion (eg. files on system dated after now).
    const int maxtimejitter = 60;

    bool tzchanged = autotz && externalTimeZone != getenv("TZ");
    bool tchanged = autotm;

    if (!tzchanged && !tchanged || externalTimeZone.isEmpty())
        return;

    QTimeZone zone(externalTimeZone.toLatin1());

    delete updater;
    updater = new TimeUpdater(externalTime-externalTimeTimeStamp, zone, tchanged, tzchanged);
    connect(updater,SIGNAL(changeSystemTime(uint,QString)),
               this,SLOT(changeSystemTime(uint,QString)));

    if ( ask_time && abs(externalTime-externalTimeTimeStamp) < maxtimejitter )
        ask_time = false;

    if ( ask_tz && tzchanged || ask_time && tchanged )
        updater->ask();
    else
        updater->commitUpdate();
}

QTOPIA_TASK(TimeUpdateService, TimeUpdateService);

