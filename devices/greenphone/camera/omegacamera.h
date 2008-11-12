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

#ifndef __QTOPIA_CAMERA_OMEGACAMERA_H
#define __QTOPIA_CAMERA_OMEGACAMERA_H

#include "videocapturedevice.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/videodev.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

#define GREENPHONE 

namespace camera
{

class OmegaCamera : public VideoCaptureDevice
{
public:
    OmegaCamera();
    ~OmegaCamera();

    bool hasCamera() const;
    void getCameraImage( QImage& img, bool copy = false);

    QList<QSize> photoSizes() const;
    QList<QSize> videoSizes() const;

    QSize recommendedPhotoSize() const;
    QSize recommendedVideoSize() const;
    QSize recommendedPreviewSize() const;

    QSize captureSize() const;
    void setCaptureSize( QSize size );

    uint refocusDelay() const;
    int minimumFramePeriod() const;

    int getFD() { return fd; }
private:
    int fd;
    int width, height;
    struct video_capability caps;
    struct video_mbuf mbuf;
    unsigned char *frames;
    quint16*    m_imageBuf;
    int currentFrame;

    void setupCamera( QSize size );
    void shutdown();
};


}   // ns camera

#endif  // __QTOPIA_CAMERA_OMEGACAMERA_H
