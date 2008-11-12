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

#include <qlist.h>
#include <qtopialog.h>
#include <custom.h>

#include "v4l1videocapturedevice.h"

namespace camera
{

bool V4L1VideoCaptureDevice::hasCamera() const
{
    return ( fd != -1 );
}

QSize V4L1VideoCaptureDevice::captureSize() const
{
    return QSize( width, height );
}

uint V4L1VideoCaptureDevice::refocusDelay() const
{
    return 250;
}

int V4L1VideoCaptureDevice::minimumFramePeriod() const
{
    return 40; // milliseconds
}

V4L1VideoCaptureDevice::V4L1VideoCaptureDevice()
{
    setupCamera( QSize( 0, 0 ) );
}

V4L1VideoCaptureDevice::~V4L1VideoCaptureDevice()
{
    shutdown();
}

void V4L1VideoCaptureDevice::setupCamera(QSize size)
{
    // Clear important variables.
    frames = 0;
    currentFrame = 0;
    width = 640;
    height = 480;
    caps.minwidth = width;
    caps.minheight = height;
    caps.maxwidth = width;
    caps.maxheight = height;

    // Open the video device.
    fd = open( V4L_VIDEO_DEVICE, O_RDWR );
    if ( fd == -1 ) {
        qWarning( "Cannot open video device %s: %s", V4L_VIDEO_DEVICE, strerror( errno ) );
        return;
    }

    // Get the device's current capabilities.
    memset( &caps, 0, sizeof( caps ) );
    if ( ioctl( fd, VIDIOCGCAP, &caps ) < 0 ) {
        qWarning( "%s: could not retrieve the video capabilities", V4L_VIDEO_DEVICE );
        close( fd );
        fd = -1;
        return;
    }

    // Change the channel to the first-connected camera, skipping TV inputs.
    // If there are multiple cameras, this may need to be modified.
    int chan;
    struct video_channel chanInfo;
    qLog(Camera) << "Available video capture inputs:";
    for ( chan = 0; chan < caps.channels; ++chan ) {
        chanInfo.channel = chan;
        if ( ioctl( fd, VIDIOCGCHAN, &chanInfo ) >= 0 ) {
            if ( chanInfo.type == VIDEO_TYPE_CAMERA )
                qLog(Camera) << chanInfo.name << "(camera)";
            else if ( chanInfo.type == VIDEO_TYPE_TV )
                qLog(Camera) << chanInfo.name << "(tv)";
            else
                qLog(Camera) << chanInfo.name << "(unknown)";
        }
    }
    for ( chan = 0; chan < caps.channels; ++chan ) {
        chanInfo.channel = chan;
        if ( ioctl( fd, VIDIOCGCHAN, &chanInfo ) >= 0 ) {
            if ( chanInfo.type == VIDEO_TYPE_CAMERA ) {
                qLog(Camera) << "Selecting camera on input" << chanInfo.name;
                if ( ioctl( fd, VIDIOCSCHAN, &chan ) < 0 ) {
                    qLog(Camera) << V4L_VIDEO_DEVICE << ": could not set the channel";
                }
                break;
            }
        }
    }

    // Set the desired picture mode to RGB32.
    struct video_picture pict;
    memset( &pict, 0, sizeof( pict ) );
    ioctl( fd, VIDIOCGPICT, &pict );
    pict.palette = VIDEO_PALETTE_RGB32;
    if ( ioctl( fd, VIDIOCSPICT, &pict ) < 0 ) {
        qWarning( "%s: could not set the picture mode", V4L_VIDEO_DEVICE );
        close( fd );
        fd = -1;
        return;
    }

    // Determine the capture size to use.  Zero indicates "preview mode".
    if ( size.width() == 0 ) {
        size = QSize( caps.minwidth, caps.minheight );
    }

    // Get the current capture window.
    struct video_window wind;
    memset( &wind, 0, sizeof( wind ) );
    ioctl( fd, VIDIOCGWIN, &wind );

    // Adjust the capture size to match the camera's aspect ratio.
    if ( caps.maxwidth > 0 && caps.maxheight > 0 ) {
        if ( size.width() > size.height() ) {
            size = QSize( size.height() * caps.maxwidth / caps.maxheight,
                          size.height() );
        } else {
            size = QSize( size.width(),
                          size.width() * caps.maxheight / caps.maxwidth );
        }
    }

    // Set the new capture window.
    wind.x = 0;
    wind.y = 0;
    wind.width = size.width();
    wind.height = size.height();
    if ( ioctl( fd, VIDIOCSWIN, &wind ) < 0 ) {
        qWarning( "%s: could not set the capture window", V4L_VIDEO_DEVICE );
    }

    // Re-read the capture window, to see what it was adjusted to.
    ioctl( fd, VIDIOCGWIN, &wind );
    width = wind.width;
    height = wind.height;

    // Enable mmap-based access to the camera.
    memset( &mbuf, 0, sizeof( mbuf ) );
    if ( ioctl( fd, VIDIOCGMBUF, &mbuf ) < 0 ) {
        qWarning( "%s: mmap-based camera access is not available", V4L_VIDEO_DEVICE );
        close( fd );
        fd = -1;
        return;
    }

    // Mmap the designated memory region.
    frames = (unsigned char *)mmap( 0, mbuf.size, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, fd, 0 );
    if ( !frames || frames == (unsigned char *)(long)(-1) ) {
        qWarning( "%s: could not mmap the device", V4L_VIDEO_DEVICE );
        close( fd );
        fd = -1;
        return;
    }

    // Start capturing of the first frame.
    struct video_mmap capture;
    currentFrame = 0;
    capture.frame = currentFrame;
    capture.width = width;
    capture.height = height;
    capture.format = VIDEO_PALETTE_RGB32;
    ioctl( fd, VIDIOCMCAPTURE, &capture );
}

void V4L1VideoCaptureDevice::shutdown()
{
    if ( frames != 0 ) {
        munmap( frames, mbuf.size );
        frames = 0;
    }
    if ( fd != -1 ) {
        int flag = 0;
        ioctl( fd, VIDIOCSYNC, 0);
        ioctl( fd, VIDIOCCAPTURE, &flag );
        close( fd );
        fd = -1;
    }
}

void V4L1VideoCaptureDevice::getCameraImage( QImage& img, bool copy )
{
    if ( fd == -1 ) {
        if ( img.isNull() ) {
            img = QImage(width, height, QImage::Format_RGB32);
        }
        return;
    }

    // Start capturing the next frame (we alternate between 0 and 1).
    int frame = currentFrame;
    struct video_mmap capture;
    if ( mbuf.frames > 1 ) {
        currentFrame = !currentFrame;
        capture.frame = currentFrame;
        capture.width = width;
        capture.height = height;
        capture.format = VIDEO_PALETTE_RGB32;
        ioctl( fd, VIDIOCMCAPTURE, &capture );
    }

    // Wait for the current frame to complete.
    ioctl( fd, VIDIOCSYNC, &frame );

    // Create an image that refers directly to the kernel's
    // frame buffer, to avoid having to copy the data.
    if ( !copy ) {
        img = QImage( frames + mbuf.offsets[frame], width, height,
                      QImage::Format_RGB32 );
    } else {
        img = QImage( width, height, QImage::Format_RGB32 );
        memcpy( img.bits(), frames + mbuf.offsets[frame], width * height * 4 );
    }

    // Queue up another frame if the device only supports one at a time.
    if ( mbuf.frames <= 1 ) {
        capture.frame = currentFrame;
        capture.width = width;
        capture.height = height;
        capture.format = VIDEO_PALETTE_RGB32;
        ioctl( fd, VIDIOCMCAPTURE, &capture );
    }
}

QList<QSize> V4L1VideoCaptureDevice::photoSizes() const
{
    QList<QSize> list;
    list.append( QSize( caps.maxwidth, caps.maxheight ) );
    if ( caps.maxwidth != caps.minwidth || caps.maxheight != caps.minheight )
        list.append( QSize( caps.minwidth, caps.minheight ) );
    return list;
}

QList<QSize> V4L1VideoCaptureDevice::videoSizes() const
{
    // We use the same sizes for both.
    return photoSizes();
}

QSize V4L1VideoCaptureDevice::recommendedPhotoSize() const
{
    return QSize( caps.maxwidth, caps.maxheight );
}

QSize V4L1VideoCaptureDevice::recommendedVideoSize() const
{
    return QSize( caps.minwidth, caps.minheight );
}

QSize V4L1VideoCaptureDevice::recommendedPreviewSize() const
{
    return QSize( caps.minwidth, caps.minheight );
}

void V4L1VideoCaptureDevice::setCaptureSize( QSize size )
{
    if ( size.width() != width || size.height() != height ) {
        shutdown();
        setupCamera( size );
    }
}

}   // ns camera
