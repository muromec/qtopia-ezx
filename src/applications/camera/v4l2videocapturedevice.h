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

#ifndef __QTOPIA_V4L2VIDEDOCAPTUREDEVICE_H
#define __QTOPIA_V4L2VIDEDOCAPTUREDEVICE_H

#include <qsize.h>
#include <qlist.h>
#include <qmap.h>

#include "videocapturedevice.h"

namespace camera
{

class FormatConverter;

class V4L2VideoCaptureDevice : public VideoCaptureDevice
{
public:

    V4L2VideoCaptureDevice(int fd);
    ~V4L2VideoCaptureDevice();

    bool hasCamera() const;
    void getCameraImage(QImage& img, bool copy = false);

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
    void calcPhotoSizes();
    void beginCapture();
    void endCapture();

    bool enumerateFormats();

    int                 m_fd;
    int                 m_imageBufferLength;
    unsigned char       *m_imageBuffer;

    QSize               m_size;
    QImage              *m_currentImage;
    QMap<QSize, unsigned int>   m_imageTypes;
    FormatConverter     *m_converter;
    QList<v4l2_fmtdesc> m_supportedFormats;


};

}   // ns camera

#endif  // __QTOPIA_V4L2VIDEDOCAPTUREDEVICE_H

