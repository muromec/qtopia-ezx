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

#ifndef __QTOPIA_CAMERA_VIDEOCAPTURE_H
#define __QTOPIA_CAMERA_VIDEOCAPTURE_H

#include <qsize.h>
#include <qlist.h>
#include <qimage.h>


class VideoCapture
{
public:
    virtual ~VideoCapture() {}

    virtual bool hasCamera() const = 0;
    virtual void getCameraImage(QImage& img, bool copy=false) = 0;

    virtual QList<QSize> photoSizes() const = 0;
    virtual QList<QSize> videoSizes() const = 0;

    virtual QSize recommendedPhotoSize() const = 0;
    virtual QSize recommendedVideoSize() const = 0;
    virtual QSize recommendedPreviewSize() const = 0;

    virtual QSize captureSize() const = 0;
    virtual void setCaptureSize( QSize size ) = 0;

    virtual uint refocusDelay() const = 0;
    virtual int minimumFramePeriod() const = 0;

    static VideoCapture* instance();
};


#endif  // __QTOPIA_CAMERA_VIDEOCAPTURE_H

