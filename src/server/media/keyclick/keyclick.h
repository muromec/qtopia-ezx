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

#ifndef _KEYCLICK_H_
#define _KEYCLICK_H_

#include "qtopiainputevents.h"
#include <QObject>

class KeyClick : public QObject, public QtopiaKeyboardFilter
{
Q_OBJECT
public:
    KeyClick();
    virtual ~KeyClick();

protected:
    virtual bool filter(int unicode, int keycode, int modifiers,
                        bool press, bool autoRepeat);
    virtual void keyClick(int unicode, int keycode, int modifiers,
                          bool press, bool repeat) = 0;

private slots:
    void rereadVolume();

private:
    bool m_clickenabled;
};

#endif // _KEYCLICK_H_
