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

/*
 * YOPY buttons driver
 * Contributed by Ron Victorelli (victorrj at icubed.com)
 */

#include "qkbdyopy_qws.h"

#ifndef QT_NO_QWS_KBD_YOPY

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include <linux/kd.h>
#include <linux/fb.h>
#include <linux/yopy_button.h>

extern "C" {
    int getpgid(int);
}

#include <qwidget.h>
#include <qsocketnotifier.h>

class QWSYopyKbPrivate : public QObject
{
    Q_OBJECT
public:
    QWSYopyKbPrivate(QWSYopyKeyboardHandler *h, const QString&);
    virtual ~QWSYopyKbPrivate();

    bool isOpen() { return buttonFD > 0; }

private slots:
    void readKeyboardData();

private:
    QString terminalName;
    int buttonFD;
    struct termios newT, oldT;
    QSocketNotifier *notifier;
    QWSYopyKeyboardHandler *handler;
};

QWSYopyKeyboardHandler::QWSYopyKeyboardHandler(const QString &device)
{
    d = new QWSYopyKbPrivate(this, device);
}

QWSYopyKeyboardHandler::~QWSYopyKeyboardHandler()
{
    delete d;
}

QWSYopyKbPrivate::QWSYopyKbPrivate(QWSYopyKeyboardHandler *h, const QString &device) : handler(h)
{
    terminalName = device.isEmpty()?"/dev/tty1":device.toLatin1().constData();
    buttonFD = -1;
    notifier = 0;

    buttonFD = ::open(terminalName.toLatin1().constData(), O_RDWR | O_NDELAY, 0);
    if (buttonFD < 0) {
        qWarning("Cannot open %s\n", qPrintable(terminalName));
        return;
    } else {

       tcsetpgrp(buttonFD, getpgid(0));

       /* put tty into "straight through" mode.
       */
       if (tcgetattr(buttonFD, &oldT) < 0) {
           qFatal("Linux-kbd: tcgetattr failed");
       }

       newT = oldT;
       newT.c_lflag &= ~(ICANON | ECHO  | ISIG);
       newT.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON);
       newT.c_iflag |= IGNBRK;
       newT.c_cc[VMIN]  = 0;
       newT.c_cc[VTIME] = 0;


       if (tcsetattr(buttonFD, TCSANOW, &newT) < 0) {
           qFatal("Linux-kbd: TCSANOW tcsetattr failed");
       }

       if (ioctl(buttonFD, KDSKBMODE, K_MEDIUMRAW) < 0) {
           qFatal("Linux-kbd: KDSKBMODE tcsetattr failed");
       }

        notifier = new QSocketNotifier(buttonFD, QSocketNotifier::Read, this);
        connect(notifier, SIGNAL(activated(int)),this,
                 SLOT(readKeyboardData()));
    }
}

QWSYopyKbPrivate::~QWSYopyKbPrivate()
{
    if (buttonFD > 0) {
        ::close(buttonFD);
        buttonFD = -1;
    }
}

void QWSYopyKbPrivate::readKeyboardData()
{
    uchar buf[1];
    char c='1';
    int fd;

    int n=read(buttonFD,buf,1);
    if (n<0) {
        qDebug("Keyboard read error %s",strerror(errno));
    } else {
        uint code = buf[0]&YPBUTTON_CODE_MASK;
        bool press = !(buf[0]&0x80);
        // printf("Key=%d/%d/%d\n",buf[1],code,press);
        int k=(-1);
        switch(code) {
          case 39:       k=Qt::Key_Up;     break;
          case 44:       k=Qt::Key_Down;   break;
          case 41:       k=Qt::Key_Left;   break;
          case 42:       k=Qt::Key_Right;  break;
          case 56:       k=Qt::Key_F1;     break; //windows
          case 29:       k=Qt::Key_F2;     break; //cycle
          case 24:       k=Qt::Key_F3;     break; //record
          case 23:       k=Qt::Key_F4;     break; //mp3
          case 4:        k=Qt::Key_F5;     break; // PIMS
          case 1:        k=Qt::Key_Escape; break; // Escape
          case 40:       k=Qt::Key_Up;     break; // prev
          case 45:       k=Qt::Key_Down;   break; // next
          case 35:       if(!press) {
                           fd = open("/proc/sys/pm/sleep",O_RDWR,0);
                           if(fd >= 0) {
                               write(fd,&c,sizeof(c));
                               close(fd);
                               //
                               // Updates all widgets.
                               //
                               QWidgetList list = QApplication::allWidgets();
                               for (int i = 0; i < list.size(); ++i) {
                                   QWidget *w = list.at(i);
                                   w->update();
                               }
                           }
                         }
                         break;

          default: k=(-1); break;
        }

        if (k >= 0) {
            handler->processKeyEvent(0, k, 0, press, false);
        }
    }
}

#include "qkbdyopy_qws.moc"

#endif // QT_NO_QWS_KBD_YOPY

