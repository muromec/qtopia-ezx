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

#ifndef NOKIAKBDHANDLER_H
#define NOKIAKBDHANDLER_H

#ifdef QT_QWS_NOKIA

#include <QObject>
#include <QWSKeyboardHandler>

#include <termios.h>
#include <linux/kd.h>

class QSocketNotifier;

class NokiaKbdHandler : public QObject, public QWSKeyboardHandler
{
    Q_OBJECT
public:
    NokiaKbdHandler();
    ~NokiaKbdHandler();

private:
    QSocketNotifier *m_notify;
    int  kbdFD;
    struct termios origTermData;
    bool shift;
int numKeyPress;
private Q_SLOTS:
    void readKbdData();

    void handleTtySwitch(int sig);
};

#endif // QT_QWS_NOKIA

#endif // NOKIAKBDHANDLER_H
