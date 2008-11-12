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

#include <custom.h>
#include <stdio.h>
#include <stdlib.h>

QTOPIABASE_EXPORT int qpe_sysBrightnessSteps()
{
    return 255;
}

QTOPIABASE_EXPORT void qpe_setBrightness(int b)
{
    char cmd[80];

    if(b==1) b=10;
    else b=b/10;
    sprintf(cmd,"echo %d>/sys/class/backlight/corgi-bl/brightness",b);
    //qWarning("cmd=%s",cmd);
    system(cmd);
}
