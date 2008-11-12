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

#include <QValueSpaceItem>
#include <qtopianamespace.h>
#include <qwindowsystem_qws.h>
#include "custom.h"

#include "include/ipmc.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define _LCDCTRL_IOCTL_BRIGHTNESS 4
#define PM_DISPLAY_ON           10
#define PM_DISPLAY_OFF          11
#define KPBL_ON                 1
#define KPBL_OFF                2


QTOPIA_EXPORT int qpe_sysBrightnessSteps()
{
    return 11;
}

QTOPIA_EXPORT void qpe_setBrightness(int b)
{
    int lcdFd = ::open("/dev/lcdctrl", O_RDWR);
    if(lcdFd >= 0) {
        ::ioctl(lcdFd, _LCDCTRL_IOCTL_BRIGHTNESS, 10*b/255);
        ::close(lcdFd);
    }  

    int ipmcFd = ::open("/dev/ipmc", O_RDWR);
    if (ipmcFd >= 0) {
        if (b == 0) {
            ::ioctl(ipmcFd, IPMC_IOCTL_SEND_PMCOMM, PM_DISPLAY_OFF);
        } else if (b == 1) {
            ::ioctl(ipmcFd, IPMC_IOCTL_SEND_PMCOMM, PM_DISPLAY_ON);

            int kpblFd = ::open("/dev/omega_kpbl", O_RDWR);
            if (kpblFd >= 0) {
                ::ioctl(kpblFd, KPBL_OFF, 0);
                ::close(kpblFd);
            }
        } else {
            ::ioctl(ipmcFd, IPMC_IOCTL_SEND_PMCOMM, PM_DISPLAY_ON);
        }

        ::close(ipmcFd);
    }
}

