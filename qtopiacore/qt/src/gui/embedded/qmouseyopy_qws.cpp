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

#include "qmouseyopy_qws.h"

#ifndef QT_NO_QWS_MOUSE_YOPY
#include "qwindowsystem_qws.h"
#include "qsocketnotifier.h"
#include "qapplication.h"
#include "qscreen_qws.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

class QWSYopyMouseHandlerPrivate : public QObject
{
    Q_OBJECT
public:
    QWSYopyMouseHandlerPrivate(QWSYopyMouseHandler *h);
    ~QWSYopyMouseHandlerPrivate();

    void suspend();
    void resume();

private slots:
    void readMouseData();

private:
    int mouseFD;
    int prevstate;
    QSocketNotifier *mouseNotifier;
    QWSYopyMouseHandler *handler;
};

QWSYopyMouseHandler::QWSYopyMouseHandler(const QString &driver, const QString &device)
    : QWSMouseHandler(driver, device)
{
    d = new QWSYopyMouseHandlerPrivate(this);
}

QWSYopyMouseHandler::~QWSYopyMouseHandler()
{
    delete d;
}

void QWSYopyMouseHandler::resume()
{
    d->resume();
}

void QWSYopyMouseHandler::suspend()
{
    d->suspend();
}

QWSYopyMouseHandlerPrivate::QWSYopyMouseHandlerPrivate(QWSYopyMouseHandler *h)
    : handler(h)
{
    if ((mouseFD = open("/dev/ts", O_RDONLY)) < 0) {
        qWarning("Cannot open /dev/ts (%s)", strerror(errno));
        return;
    } else {
        sleep(1);
    }
    prevstate=0;
    mouseNotifier = new QSocketNotifier(mouseFD, QSocketNotifier::Read,
                                         this);
    connect(mouseNotifier, SIGNAL(activated(int)),this, SLOT(readMouseData()));
}

QWSYopyMouseHandlerPrivate::~QWSYopyMouseHandlerPrivate()
{
    if (mouseFD >= 0)
        close(mouseFD);
}

#define YOPY_XPOS(d) (d[1]&0x3FF)
#define YOPY_YPOS(d) (d[2]&0x3FF)
#define YOPY_PRES(d) (d[0]&0xFF)
#define YOPY_STAT(d) (d[3]&0x01)

struct YopyTPdata {

  unsigned char status;
  unsigned short xpos;
  unsigned short ypos;

};

void QWSYopyMouseHandlerPrivate::suspend()
{
    mouseNotifier->setEnabled(false);
}


void QWSYopyMouseHandlerPrivate::resume()
{
    prevstate = 0;
    mouseNotifier->setEnabled(true);
}

void QWSYopyMouseHandlerPrivate::readMouseData()
{
    if(!qt_screen)
        return;
    YopyTPdata data;

    unsigned int yopDat[4];

    int ret;

    ret=read(mouseFD,&yopDat,sizeof(yopDat));

    if(ret) {
        data.status= (YOPY_PRES(yopDat)) ? 1 : 0;
        data.xpos=YOPY_XPOS(yopDat);
        data.ypos=YOPY_YPOS(yopDat);
        QPoint q;
        q.setX(data.xpos);
        q.setY(data.ypos);
        if (data.status && !prevstate) {
          handler->mouseChanged(q,Qt::LeftButton);
        } else if(!data.status && prevstate) {
          handler->mouseChanged(q,0);
        }
        prevstate = data.status;
    }
    if(ret<0) {
        qDebug("Error %s",strerror(errno));
    }
}

#include "qmouseyopy_qws.moc"
#endif //QT_NO_QWS_MOUSE_YOPY
