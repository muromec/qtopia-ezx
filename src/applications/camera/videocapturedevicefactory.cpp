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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/videodev.h>
#include <custom.h>

#include "videocapturedevicefactory.h"
#include "videocapturedevice.h"
#ifdef QTOPIA_HAVE_V4L2
#include "v4l2videocapturedevice.h"
#include "nodevice.h"
#endif
#include "v4l1videocapturedevice.h"
#ifdef _CAMERA_DEBUG
#include "dummyvideocapturedevice.h"
#endif

namespace camera
{

VideoCaptureDevice* VideoCaptureDeviceFactory::createVideoCaptureDevice()
{
    VideoCaptureDevice* rc;

#ifdef QTOPIA_CAMERA_DEBUG
    rc = new DummyVideoCaptureDevice;
#else

#ifdef QTOPIA_HAVE_V4L2

    int fd;

    if ((fd = open(V4L_VIDEO_DEVICE, O_RDWR)) != -1)
    {
        v4l2_capability     capability;

        if (ioctl(fd, VIDIOC_QUERYCAP, &capability) == -1)
        {
//            if (errno == EINVAL)      // standard says this is what happens, but a (some) driver(s?) don't do this
//            {   // Failed v4l2 ioctl, assume v4l1
                close(fd);  // see below
                rc = new V4L1VideoCaptureDevice;
//            }
        }
        else
        {
            rc = new V4L2VideoCaptureDevice(fd);
        }

//        close(fd);    // XXX: asymmetric, problem with a driver which
                        // will not allow a second open after the closee
    }
    else
    {
        rc = new NoDevice;
    }

#else
    rc = new V4L1VideoCaptureDevice;
#endif  // QTOPIA_HAVE_V4L2
#endif  // QTOPIA_CAMERA_DEBUG

    return rc;
}

}   // ns camera

