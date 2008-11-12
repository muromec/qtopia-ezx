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

#include <qwindowsystem_qws.h>

#include <QtopiaIpcAdaptor>
#include <QtopiaIpcEnvelope>
#include <QtopiaServiceRequest>

#include "systemsuspend.h"

#include <qvaluespace.h>

class ZyloniteSuspend : public SystemSuspendHandler
{
public:
    ZyloniteSuspend();
    virtual bool canSuspend() const;
    virtual bool suspend();
    virtual bool wake();
};

QTOPIA_DEMAND_TASK(ZyloniteSuspend, ZyloniteSuspend);
QTOPIA_TASK_PROVIDES(ZyloniteSuspend, SystemSuspendHandler);

ZyloniteSuspend::ZyloniteSuspend()
{
}

bool ZyloniteSuspend::canSuspend() const
{
    QValueSpaceItem battery("/Accessories/Battery");
    return !battery.value("Charging", false).toBool();
}

bool ZyloniteSuspend::suspend()
{
    system("apm --suspend");
    return true;
}

bool ZyloniteSuspend::wake()
{
    QWSServer::screenSaverActivate( false );
    {
        QtopiaIpcEnvelope("QPE/Card", "mtabChanged()" ); // might have changed while asleep
        QtopiaServiceRequest e("QtopiaPowerManager", "setBacklight(int)");
        e << -3; // Force on
        e.send();
    }

    return true;
}

