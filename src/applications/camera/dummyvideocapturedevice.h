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

#ifndef __QTOPIA_DUMMYVIDEDOCAPTUREDEVICE_H
#define __QTOPIA_DUMMYVIDEDOCAPTUREDEVICE_H

#include "videocapturedevice.h"

namespace camera
{


/*!
    \class DummyVideoCaptureDevice
*/

class DummyVideoCaptureDevice : public VideoCaptureDevice
{
public:

    DummyVideoCaptureDevice();
    ~DummyVideoCaptureDevice();

    bool hasCamera() const;
    void getCameraImage(QImage& img, bool copy = false );

    QList<QSize> photoSizes() const;
    QList<QSize> videoSizes() const;

    QSize recommendedPhotoSize() const;
    QSize recommendedVideoSize() const;
    QSize recommendedPreviewSize() const;

    QSize captureSize() const;
    void setCaptureSize(QSize size);

    uint refocusDelay() const;
    int minimumFramePeriod() const;

    int getFD() { return m_fd; }

private:

    void setupCamera();
    void shutdown();

    int                 m_fd;
    int                 m_imageBufferLength;
    unsigned char       *m_imageBuffer;

    QSize               m_size;
    QImage              *m_currentImage;
};

}   // ns camera

#endif  // __QTOPIA_DUMMYVIDEDOCAPTUREDEVICE_H



