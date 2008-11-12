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

#ifndef EXAMPLEKBDHANDLER_H
#define EXAMPLEKBDHANDLER_H

#ifdef QT_QWS_EXAMPLE

#include <QObject>
#include <QWSKeyboardHandler>

class QSocketNotifier;
class ExampleKbdHandler : public QObject, public QWSKeyboardHandler
{
    Q_OBJECT
public:
    ExampleKbdHandler(const QString &device = QString("/dev/input/event0"));
    ~ExampleKbdHandler();

private:
    QSocketNotifier *m_notify;
    int  kbdFd;
    bool shift;

private Q_SLOTS:
    void readKbdData();
};

#endif // QT_QWS_EXAMPLE

#endif // EXAMPLEKBDHANDLER_H
