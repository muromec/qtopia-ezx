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

#include "audiovolumemanager.h"
#include "mediakeyservice.h"
#include "mediaservicestask.h"


/*!
    \class MediaServicesTask
    \ingroup QtopiaServer::Task

    \brief The MediaServicesTask class provides a task that manages services related to Media in Qtopia

    This class is used to manage media related facilities in Qtopia. It watches
    media key events and forwards them to the appropriate party.

    This class is part of the Qtopia server and cannot be used by other Qtopia applications.
*/


/*!
    \internal
*/

MediaServicesTask::MediaServicesTask()
{
    m_avm = new AudioVolumeManager();
    m_mks = new MediaKeyService(m_avm);
    connect(m_mks, SIGNAL(volumeChanged(bool)),
            this, SIGNAL(volumeChanged(bool)));
}

/*!
    \internal
*/

MediaServicesTask::~MediaServicesTask()
{
    delete m_mks;
    delete m_avm;
}

/*!
    \internal
    Connect key management with volume management
*/

void MediaServicesTask::setVolume(bool up)
{
    m_mks->setVolume(up);
}

/*!
    \fn MediaServicesTask::volumeChanged(bool)
    \internal
*/

QTOPIA_TASK(MediaServicesTask, MediaServicesTask);
QTOPIA_TASK_PROVIDES(MediaServicesTask, MediaServicesTask);

