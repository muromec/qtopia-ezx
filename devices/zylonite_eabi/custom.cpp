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

QTOPIA_EXPORT int qpe_sysBrightnessSteps()
{
    return 255;
}

QTOPIA_EXPORT void qpe_setBrightness(int b)
{
    char cmd[80];
    if(b==1) b=10;
    else b=b/10;
    sprintf(cmd,"backlight.sh %d",b);
    system(cmd);
}
