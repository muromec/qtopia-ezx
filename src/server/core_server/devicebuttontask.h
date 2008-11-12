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

#ifndef _DEVICEBUTTONTASK_H_
#define _DEVICEBUTTONTASK_H_

#include "qtopiainputevents.h"
#include "qtopiaserverapplication.h"
class QDeviceButton;
class PressHoldGate;
class QValueSpaceItem;
class DeviceButtonTask : public QObject, public QtopiaKeyboardFilter
{
Q_OBJECT
public:
    DeviceButtonTask();

    virtual bool filter(int unicode, int keycode, int modifiers, bool press,
                        bool autoRepeat);

signals:
    void activated(int keyCode, bool held, bool isPressed);

private slots:
    void doActivate(int keyCode, bool held, bool isPressed);

private:
    bool keyLocked();

    QValueSpaceItem* vs;
    PressHoldGate*   ph;
};

QTOPIA_TASK_INTERFACE(DeviceButtonTask);
#endif // _DEVICEBUTTONTASK_H_
