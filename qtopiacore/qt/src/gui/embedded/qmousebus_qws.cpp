/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qmousebus_qws.h"

#ifndef QT_NO_QWS_MOUSE_BUS

#include "qwindowsystem_qws.h"
#include "qsocketnotifier.h"

#include "qapplication.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

/*
 * bus mouse driver (a.k.a. Logitech busmouse)
 */

class QWSBusMouseHandlerPrivate : public QObject
{
    Q_OBJECT
public:
    QWSBusMouseHandlerPrivate(QWSBusMouseHandler *h, const QString &driver, const QString &device);
    ~QWSBusMouseHandlerPrivate();

    void suspend();
    void resume();

private slots:
    void readMouseData();

protected:
    enum { mouseBufSize = 128 };
    QWSBusMouseHandler *handler;
    QSocketNotifier *mouseNotifier;
    int mouseFD;
    int mouseIdx;
    int obstate;
    uchar mouseBuf[mouseBufSize];
};

QWSBusMouseHandler::QWSBusMouseHandler(const QString &driver, const QString &device)
    : QWSMouseHandler(driver, device)
{
    d = new QWSBusMouseHandlerPrivate(this, driver, device);
}

QWSBusMouseHandler::~QWSBusMouseHandler()
{
    delete d;
}

void QWSBusMouseHandler::suspend()
{
    d->suspend();
}

void QWSBusMouseHandler::resume()
{
    d->resume();
}


QWSBusMouseHandlerPrivate::QWSBusMouseHandlerPrivate(QWSBusMouseHandler *h,
    const QString &, const QString &device)
    : handler(h)

{
    QString mouseDev = device;
    if (mouseDev.isEmpty())
        mouseDev = QLatin1String("/dev/mouse");
    obstate = -1;
    mouseFD = -1;
    mouseFD = open(mouseDev.toLocal8Bit(), O_RDWR | O_NDELAY);
    if (mouseFD < 0)
        mouseFD = open(mouseDev.toLocal8Bit(), O_RDONLY | O_NDELAY);
    if (mouseFD < 0)
        qDebug("Cannot open %s (%s)", qPrintable(mouseDev), strerror(errno));

    // Clear pending input
    tcflush(mouseFD,TCIFLUSH);
    usleep(50000);

    char buf[100];                                // busmouse driver will not read if bufsize < 3,  YYD
    while (read(mouseFD, buf, 100) > 0) { }  // eat unwanted replies

    mouseIdx = 0;

    mouseNotifier = new QSocketNotifier(mouseFD, QSocketNotifier::Read, this);
    connect(mouseNotifier, SIGNAL(activated(int)),this, SLOT(readMouseData()));
}

QWSBusMouseHandlerPrivate::~QWSBusMouseHandlerPrivate()
{
    if (mouseFD >= 0) {
        tcflush(mouseFD,TCIFLUSH);            // yyd.
        close(mouseFD);
    }
}


void QWSBusMouseHandlerPrivate::suspend()
{
    mouseNotifier->setEnabled(false);
}


void QWSBusMouseHandlerPrivate::resume()
{
    mouseIdx = 0;
    obstate = -1;
    mouseNotifier->setEnabled(true);
}

void QWSBusMouseHandlerPrivate::readMouseData()
{
    int n;
    // It'll only read 3 bytes a time and return all other buffer zeroed, thus cause protocol errors
    for (;;) {
        if (mouseBufSize - mouseIdx < 3)
            break;
        n = read(mouseFD, mouseBuf+mouseIdx, 3);
        if (n != 3)
            break;
        mouseIdx += 3;
    }

    static const int accel_limit = 5;
    static const int accel = 2;

    int idx = 0;
    int bstate = 0;
    int dx = 0, dy = 0;
    bool sendEvent = false;
    int tdx = 0, tdy = 0;

    while (mouseIdx-idx >= 3) {
#if 0 // debug
        qDebug("Got mouse data");
#endif
        uchar *mb = mouseBuf+idx;
        bstate = 0;
        dx = 0;
        dy = 0;
        sendEvent = false;
        if (((mb[0] & 0x04)))
            bstate |= Qt::LeftButton;
        if (((mb[0] & 0x01)))
            bstate |= Qt::RightButton;

        dx=(signed char)mb[1];
        dy=(signed char)mb[2];
        sendEvent=true;

        if (sendEvent) {
            if (qAbs(dx) > accel_limit || qAbs(dy) > accel_limit) {
                dx *= accel;
                dy *= accel;
            }
            tdx += dx;
            tdy += dy;
            if (bstate != obstate) {
                QPoint pos = handler->pos() + QPoint(tdx,-tdy);
                handler->limitToScreen(pos);
                handler->mouseChanged(pos,bstate);
                sendEvent = false;
                tdx = 0;
                tdy = 0;
                obstate = bstate;
            }
        }
        idx += 3;
    }
    if (sendEvent) {
        QPoint pos = handler->pos() + QPoint(tdx,-tdy);
        handler->limitToScreen(pos);
        handler->mouseChanged(pos,bstate);
    }

    int surplus = mouseIdx - idx;
    for (int i = 0; i < surplus; i++)
        mouseBuf[i] = mouseBuf[idx+i];
    mouseIdx = surplus;
}

#include "qmousebus_qws.moc"
#endif // QT_NO_QWS_MOUSE_BUS
