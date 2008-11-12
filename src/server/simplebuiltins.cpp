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

#include "applicationlauncher.h"
#include <QtopiaChannel>
#include <QtopiaServiceRequest>

// "taskmanager" builtin
static QWidget *taskmanager()
{
    QtopiaServiceRequest( "TaskManager", "showRunningTasks()" ).send();
    return 0;
}
QTOPIA_SIMPLE_BUILTIN(taskmanager, taskmanager);

#ifdef QTOPIA_PHONEUI
// "callhistory" builtin
#include <phone/homescreencontrol.h>
static QWidget *callhistory()
{
    if (HomeScreenControl::instance()->homeScreen())
        HomeScreenControl::instance()->homeScreen()->showCallHistory(false, QString());
    return 0;
}
QTOPIA_SIMPLE_BUILTIN(callhistory, callhistory);
#endif

// "shutdown" builtin
static QWidget *shutdown()
{
    QtopiaChannel::send( "QPE/System", "shutdown()" );
    return 0;
}
QTOPIA_SIMPLE_BUILTIN(shutdown, shutdown);

#ifdef QTOPIA_CELL

// "simapp" builtin
#include "../applications/simapp/simapp.h"
static QWidget *simapp()
{
    SimApp *s = SimApp::instance();
    return s;
}
QTOPIA_SIMPLE_BUILTIN(simapp, simapp);

#endif

