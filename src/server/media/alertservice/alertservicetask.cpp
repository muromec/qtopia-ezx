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

#include "alertservicetask.h"
#include <QSoundControl>
#include "qtopiaserverapplication.h"

/*!
    \service AlertService Alert
    \brief Provides the Qtopia Alert service.

    The \i Alert service enables applications to sound the audible
    system alert.  Normally an application will use Qtopia::soundAlarm()
    for this.

    The \i Alert service is typically supplied by the AlertServiceTask task in
    the Qtopia server, but the system integrator may change the provider
    of this service if the device has special hardware (e.g. a buzzer or
    vibration system) that can alert the user.

    This class is part of the Qtopia server and cannot be used by other Qtopia applications.
    \sa Qtopia::soundAlarm()
*/

/*!
    \internal
*/
AlertService::AlertService( QObject *parent )
        : QtopiaAbstractService( "Alert", parent )
{
    publishAll();
}

/*!
    \internal
*/
AlertService::~AlertService()
{
}

/*!
    \fn AlertService::soundAlert()

    Sounds the audible system alert (once). This is used for applications such
    as Calendar when it needs to inform the user of an event.

    This slot corresponds to the QCop service message \c{Alert::soundAlert()}.
*/

/*!
  \class AlertServiceTask
  \ingroup QtopiaServer::Task
  \brief The AlertServiceTask class provides a WAV file implementation of the Alert service.

  The AlertServiceTask provides a Qtopia Server Task.  Qtopia Server Tasks are
  documented in full in the QtopiaServerApplication class documentation.

  \table
  \row \o Task Name \o AlertService
  \row \o Interfaces \o None
  \row \o Services \o Alert
  \endtable

  On reception of the Alert::soundAlert() service message the \c {:sound/alarm} system sound 
  is played using the QSoundControl API.

  This class is part of the Qtopia server and cannot be used by other Qtopia applications.
  \sa AlertService, Qtopia::soundAlarm()
 */

/*! \internal */
AlertServiceTask::AlertServiceTask()
: AlertService(0)
{
}

/*! \internal */
void AlertServiceTask::soundAlert()
{
    QSoundControl *soundcontrol =
        new QSoundControl(new QSound(":sound/alarm"));
    soundcontrol->setPriority(QSoundControl::RingTone);
    soundcontrol->sound()->play();

    QObject::connect(soundcontrol, SIGNAL(done()),
                        this, SLOT(playDone()));
}

void AlertServiceTask::playDone()
{
    QSoundControl* soundControl = qobject_cast<QSoundControl*>(sender());

    soundControl->sound()->deleteLater();
    soundControl->deleteLater();
}

QTOPIA_TASK(AlertService, AlertServiceTask);
