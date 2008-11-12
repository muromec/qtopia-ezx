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

#ifndef COMPOSEIM_H
#define COMPOSEIM_H
#include <qwindowsystem_qws.h>
#include <inputmethodinterface.h>

class ComposeIM : public QWSInputMethod
{
public:
    ComposeIM();
    ~ComposeIM();

    void reset();
    bool filter(int unicode, int keycode, int modifiers, 
			    bool isPress, bool autoRepeat);

    void setActive(bool);
    bool active() const { return isActive; }

private:
    QString lastText;
    bool upper;
    bool isActive;
};

#endif
