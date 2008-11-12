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
#include "composeim.h"
#include <QDebug>

ComposeIM::ComposeIM()
    : QWSInputMethod(),
    upper(false), isActive(false)
{
}

ComposeIM::~ComposeIM()
{
}

bool ComposeIM::filter(int unicode, int keycode, int modifiers, 
        bool isPress, bool /* autoRepeat */)
{
    if (!isActive)
        return false;

    if ((keycode > Qt::Key_Z || keycode < Qt::Key_A) && keycode != Qt::Key_Shift) {
        reset();
        return false;
    }

    if (isPress) {
        if (keycode == Qt::Key_Shift) {
            lastText = upper ? lastText.toLower() : lastText.toUpper();
            upper = !upper;
            sendPreeditString(lastText, 0);
        } else {
            if (!lastText.isEmpty())
                sendCommitString(lastText);

            lastText = QChar(unicode);
            upper = (modifiers & Qt::ShiftModifier) == Qt::ShiftModifier;
            sendPreeditString(lastText, 0);
        }
    }
    return true;
}

void ComposeIM::reset()
{
    if (!lastText.isEmpty())
        sendCommitString(lastText);
    upper = false;
    lastText = QString::null;
}

void ComposeIM::setActive(bool b)
{
    if (b && b != isActive) {
        lastText.clear();
        upper = false;
    }
    isActive = b;
}
