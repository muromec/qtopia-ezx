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


/*!
    \class camera::VideoCaptureDevice
    \brief The VideoCaptureDevice class provides a basic interface for Video Capture Devices such as cameras
    
    This class represents a basic capture device such as a camera on the Linux platorm. It main function is to set up the device for image capture
    and store the most recent captured image in a buffer for retrieval.
    \internal
*/    

/*!
    \fn bool VideoCaptureDevice::hasCamera() const = 0
    Returns true if a camera is detected, false otherwise
    \internal


*/    
  
/*!  
    \fn void VideoCaptureDevice::getCameraImage(QImage& img, bool copy = false) = 0

    Stores a recently captured image frame in \c{img}
    \internal

*/

/*!
    \fn QList<QSize> VideoCaptureDevice::photoSizes() const = 0
    Returns the  available sizes that the device supports in capture mode
    \internal

*/

/*!
    \fn QList<QSize> VideoCaptureDevice::videoSizes() const = 0
     Returns the  available sizes that the device supports in video mode
    \internal

*/ 

/*!
    \fn QSize VideoCaptureDevice::recommendedPhotoSize() const = 0
    Returns the optimal photo size for the device
    \internal

*/

/*!
    \fn QSize VideoCaptureDevice::recommendedVideoSize() const = 0
    Returns the optimal video size for the device
    \internal

*/    
    
/*!
    \fn QSize VideoCaptureDevice::recommendedPreviewSize() const = 0
    Returns the preview size for the device
    \internal

*/    

/*!    
    \fn QSize VideoCaptureDevice::captureSize() const = 0
    Returns the current capture size
    \internal

*/    
    
/*!  
    \fn void VideoCaptureDevice::setCaptureSize(QSize size) = 0
    Sets the  capture size
    \internal

*/
    
/*!
    \fn uint VideoCaptureDevice::refocusDelay() const = 0
    Returns the delay in milliseconds that the device needs to refocus
    \internal

*/

/*!    
    \fn int VideoCaptureDevice::minimumFramePeriod() const = 0
    Returns the rate at which a frame become available in milliseconds
    \internal

*/

/*!
    \fn int VideoCaptureDevice::getFD() = 0
    Returns the device file descriptor
    \internal

*/    
}
