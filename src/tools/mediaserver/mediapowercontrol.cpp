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

#include "mediapowercontrol.h"

#include "QTimer"
#include <qtopialog.h>

namespace mediaserver
{

/*!
    \class mediaserver::MediaPowerControl
    \internal
*/

MediaPowerControl::MediaPowerControl(QObject* parent):
    QObject(parent),
    m_powerConstraint(QtopiaApplication::Enable)
{
}

MediaPowerControl::~MediaPowerControl()
{
}

void MediaPowerControl::activeSessionCount(int activeSessions)
{
    m_activePlayingSessions = activeSessions;

    if(m_activePlayingSessions == 0) {
      QTimer::singleShot(2000, this, SLOT(recheck()));
    } else if (m_powerConstraint != QtopiaApplication::DisableSuspend) {
      m_powerConstraint = QtopiaApplication::DisableSuspend;
      QtopiaApplication::setPowerConstraint(m_powerConstraint);
    }

}

void MediaPowerControl::recheck()
{

  if (m_activePlayingSessions != 0)
    return;

  m_powerConstraint = QtopiaApplication::Enable;
  QtopiaApplication::setPowerConstraint(m_powerConstraint);
}
} // ns mediaserver
