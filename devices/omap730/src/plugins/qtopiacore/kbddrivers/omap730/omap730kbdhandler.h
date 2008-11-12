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

#ifndef OMAP730KBDHANDLER_H
#define OMAP730KBDHANDLER_H

#ifdef QT_QWS_OMAP730

#include <QObject>
#include <QWSKeyboardHandler>

class QSocketNotifier;
class Omap730KbdHandler : public QObject, public QWSKeyboardHandler
{
    Q_OBJECT
public:
    Omap730KbdHandler();
    ~Omap730KbdHandler();

private:
    QSocketNotifier *m_notify;
    int  kbdFD;
    bool shift;
    unsigned int prev_key,prev_unicode;

private Q_SLOTS:
    void readKbdData();
};

#endif // QT_QWS_OMAP730

#endif // OMAP730KBDHANDLER_H
