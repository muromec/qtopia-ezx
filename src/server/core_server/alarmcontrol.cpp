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

#include "alarmcontrol.h"
#include <QSettings>

/*!
  \class AlarmControl
  \ingroup QtopiaServer
  \brief The AlarmControl class maintains information about the daily alarm.
  
  This class is part of the Qtopia server and cannot be used by other Qtopia applications.
 */

/*!
  Returns the AlarmControl instance.
  */
AlarmControl *AlarmControl::instance()
{
    static AlarmControl * control = 0;
    if(!control)
        control = new AlarmControl;
    return control;
}

AlarmControl::AlarmControl()
: alarmOn(false), alarmValueSpace("/UI/DailyAlarm"),
  alarmChannel("QPE/AlarmServer")
{
    connect(&alarmChannel, SIGNAL(received(QString,QByteArray)),
            this, SLOT(alarmMessage(QString,QByteArray)));


    QSettings clockCfg("Trolltech","Clock");
    clockCfg.beginGroup( "Daily Alarm" );
    bool alarm = clockCfg.value("Enabled", false).toBool();
    alarmOn = !alarm;
    alarmEnabled(alarm);
}

void AlarmControl::alarmEnabled(bool on)
{
    if(on != alarmOn) {
        alarmOn = on;
        alarmValueSpace.setAttribute("", alarmOn);
        emit alarmStateChanged(alarmOn);
    }
}

/*!
  Returns true if the alarm is on, otherwise false.
  */
bool AlarmControl::alarmState() const
{
    return alarmOn;
}

/*!
  \fn void AlarmControl::alarmStateChanged(bool newState)

  Emitted whenever the alarm state changes to \a newState.
  */

void AlarmControl::alarmMessage(const QString& message, const QByteArray &data)
{
    if ( message == "dailyAlarmEnabled(bool)" ) {
        QDataStream stream( data );
        bool enabled;
        stream >> enabled;
        alarmEnabled(enabled);
    }
}

