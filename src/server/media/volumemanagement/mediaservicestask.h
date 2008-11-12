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

#ifndef __QTOPIA_SERVER_MEDIA_MEDIASERVICETASK_H
#define __QTOPIA_SERVER_MEDIA_MEDIASERVICETASK_H

#include "qtopiaserverapplication.h"

class AudioVolumeManager;
class MediaKeyService;

class MediaServicesTask : public QObject
{
    Q_OBJECT

public:
    MediaServicesTask();
    ~MediaServicesTask();
    
public slots:
    void setVolume(bool up);

signals:
    void volumeChanged(bool up);

private:
    AudioVolumeManager* m_avm;
    MediaKeyService*    m_mks;
};
QTOPIA_TASK_INTERFACE(MediaServicesTask);


#endif  // __QTOPIA_SERVER_MEDIA_MEDIASERVICETASK_H

