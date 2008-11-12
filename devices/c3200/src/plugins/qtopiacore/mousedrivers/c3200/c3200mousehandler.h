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

#ifndef C3200MOUSEHANDLER_H
#define C3200MOUSEHANDLER_H

#define TS_SAMPLES   2

#include <QtGui/QWSCalibratedMouseHandler>

#ifdef QT_QWS_C3200

class QSocketNotifier;
class C3200MouseHandler : public QObject, public QWSCalibratedMouseHandler
{
    Q_OBJECT
public:
    C3200MouseHandler();
    ~C3200MouseHandler();

    void suspend();
    void resume();

private:
    void openTs();
    void closeTs();

private:
    bool m_raw : 1;
    int  totX,totY,nX,nY;
    int  sx[TS_SAMPLES+3], sy[TS_SAMPLES+3], ss;
    int  index_x1, index_x2, index_y1, index_y2, min_x, min_y;
    int  mouseFD;
    int  mouseIdx;
    static const int mouseBufSize = 2048;
    uchar mouseBuf[mouseBufSize];
    QPoint oldmouse;
    C3200MouseHandler *handler;
    QSocketNotifier *m_notify;

private Q_SLOTS:
    void readMouseData();
};

#endif // QT_QWS_C3200

#endif // C3200MOUSEHANDLER_H
