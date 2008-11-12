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

#ifndef __QTOPIA_V4L1VIDEDOCAPTUREDEVICE_H
#define __QTOPIA_V4L1VIDEDOCAPTUREDEVICE_H

#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <linux/videodev.h>

#include "videocapturedevice.h"

namespace camera
{

class V4L1VideoCaptureDevice : public VideoCaptureDevice
{
public:

    V4L1VideoCaptureDevice();
    ~V4L1VideoCaptureDevice();

    bool hasCamera() const;
    void getCameraImage( QImage& img, bool copy=FALSE );

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

    int                 fd;
    int                 width, height;
    video_capability    caps;
    video_mbuf          mbuf;
    unsigned char       *frames;
    int                 currentFrame;

    void setupCamera( QSize size );
    void shutdown();
};

}   // ns camera

#endif  // __QTOPIA_V4L1VIDEDOCAPTUREDEVICE_H


