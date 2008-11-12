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

#ifndef QT_NO_QWS_QVFB

#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <qvfbhdr.h>
#include <qscreenvfb_qws.h>
#include <qkbdvfb_qws.h>
#include <qmousevfb_qws.h>
#include <qwindowsystem_qws.h>
#include <qsocketnotifier.h>
#include <qapplication.h>
#include <qscreen_qws.h>
#include <qmousedriverfactory_qws.h>
#include <qkbddriverfactory_qws.h>
#include <qdebug.h>

class QVFbScreenPrivate
{
public:
    QVFbScreenPrivate();
    ~QVFbScreenPrivate();

    bool success;
    unsigned char *shmrgn;
    QVFbHeader *hdr;
    QWSMouseHandler *mouse;
#ifndef QT_NO_QWS_KEYBOARD
    QWSKeyboardHandler *keyboard;
#endif
};

QVFbScreenPrivate::QVFbScreenPrivate()
    : mouse(0)

{
#ifndef QT_NO_QWS_KEYBOARD
    keyboard = 0;
#endif
}

QVFbScreenPrivate::~QVFbScreenPrivate()
{
    delete mouse;
#ifndef QT_NO_QWS_KEYBOARD
    delete keyboard;
#endif
}

/*!
    \internal

    \class QVFbScreen
    \ingroup qws

    \brief The QVFbScreen class implements a screen driver for the
    virtual framebuffer.

    Note that this class is only available in \l {Qtopia Core}.
    Custom screen drivers can be added by subclassing the
    QScreenDriverPlugin class, using the QScreenDriverFactory class to
    dynamically load the driver into the application, but there should
    only be one screen object per application.

    The Qtopia Core platform provides a \l {The Virtual
    Framebuffer}{virtual framebuffer} for development and debugging;
    the virtual framebuffer allows Qtopia Core programs to be
    developed on a desktop machine, without switching between consoles
    and X11.

    \sa QScreen, QScreenDriverPlugin, {Running Applications}
*/

/*!
    \fn bool QVFbScreen::connect(const QString & displaySpec)
    \reimp
*/

/*!
    \fn void QVFbScreen::disconnect()
    \reimp
*/

/*!
    \fn bool QVFbScreen::initDevice()
    \reimp
*/

/*!
    \fn void QVFbScreen::restore()
    \reimp
*/

/*!
    \fn void QVFbScreen::save()
    \reimp
*/

/*!
    \fn void QVFbScreen::setDirty(const QRect & r)
    \reimp
*/

/*!
    \fn void QVFbScreen::setMode(int nw, int nh, int nd)
    \reimp
*/

/*!
    \fn void QVFbScreen::shutdownDevice()
    \reimp
*/

/*!
    \fn QVFbScreen::QVFbScreen(int displayId)

    Constructs a QVNCScreen object. The \a displayId argument
    identifies the Qtopia Core server to connect to.
*/
QVFbScreen::QVFbScreen(int display_id)
    : QScreen(display_id), d_ptr(new QVFbScreenPrivate)
{
    d_ptr->shmrgn = 0;
    d_ptr->hdr = 0;
    data = 0;
}

/*!
    Destroys this QVFbScreen object.
*/
QVFbScreen::~QVFbScreen()
{
    delete d_ptr;
}

bool QVFbScreen::connect(const QString &displaySpec)
{
    QStringList displayArgs = displaySpec.split(QLatin1Char(':'));
    if (displayArgs.contains(QLatin1String("Gray")))
        grayscale = true;

    key_t key = ftok(QByteArray(QT_VFB_MOUSE_PIPE).replace("%1", QByteArray::number(displayId)), 'b');

    if (key == -1)
        return false;

    int shmId = shmget(key, 0, 0);
    if (shmId != -1)
        d_ptr->shmrgn = (unsigned char *)shmat(shmId, 0, 0);
    else
        return false;

    if ((long)d_ptr->shmrgn == -1 || d_ptr->shmrgn == 0) {
        qDebug("No shmrgn %ld", (long)d_ptr->shmrgn);
        return false;
    }

    d_ptr->hdr = (QVFbHeader *)d_ptr->shmrgn;
    data = d_ptr->shmrgn + d_ptr->hdr->dataoffset;

    dw = w = d_ptr->hdr->width;
    dh = h = d_ptr->hdr->height;
    d = d_ptr->hdr->depth;

    switch (d) {
    case 1:
        setPixelFormat(QImage::Format_Mono);
        break;
    case 8:
        setPixelFormat(QImage::Format_Indexed8);
        break;
    case 16:
        setPixelFormat(QImage::Format_RGB16);
        break;
    case 32:
        setPixelFormat(QImage::Format_ARGB32_Premultiplied);
        break;
    }

    lstep = d_ptr->hdr->linestep;

    // Handle display physical size spec.
    int dimIdxW = -1;
    int dimIdxH = -1;
    for (int i = 0; i < displayArgs.size(); ++i) {
        if (displayArgs.at(i).startsWith(QLatin1String("mmWidth"))) {
            dimIdxW = i;
            break;
        }
    }
    for (int i = 0; i < displayArgs.size(); ++i) {
        if (displayArgs.at(i).startsWith(QLatin1String("mmHeight"))) {
            dimIdxH = i;
            break;
        }
    }
    if (dimIdxW >= 0) {
        bool ok;
        int pos = 7;
        if (displayArgs.at(dimIdxW).at(pos) == QLatin1Char('='))
            ++pos;
        int pw = displayArgs.at(dimIdxW).mid(pos).toInt(&ok);
        if (ok) {
            physWidth = pw;
            if (dimIdxH < 0)
                physHeight = dh*physWidth/dw;
        }
    }
    if (dimIdxH >= 0) {
        bool ok;
        int pos = 8;
        if (displayArgs.at(dimIdxH).at(pos) == QLatin1Char('='))
            ++pos;
        int ph = displayArgs.at(dimIdxH).mid(pos).toInt(&ok);
        if (ok) {
            physHeight = ph;
            if (dimIdxW < 0)
                physWidth = dw*physHeight/dh;
        }
    }
    if (dimIdxW < 0 && dimIdxH < 0) {
        const int dpi = 72;
        physWidth = qRound(dw * 25.4 / dpi);
        physHeight = qRound(dh * 25.4 / dpi);
    }

    qDebug("Connected to VFB server %s: %d x %d x %d %dx%dmm (%dx%ddpi)", displaySpec.toLatin1().data(),
        w, h, d, physWidth, physHeight, qRound(dw*25.4/physWidth), qRound(dh*25.4/physHeight) );

    size = lstep * h;
    mapsize = size;
    screencols = d_ptr->hdr->numcols;
    memcpy(screenclut, d_ptr->hdr->clut, sizeof(QRgb) * screencols);

    if (qApp->type() == QApplication::GuiServer) {
        const QString mouseDev = QString(QLatin1String(QT_VFB_MOUSE_PIPE))
                                 .arg(displayId);
        d_ptr->mouse = QMouseDriverFactory::create(QLatin1String("QVFbMouse"),
                                                   mouseDev);
        qwsServer->setDefaultMouse("None");
        if (d_ptr->mouse)
            d_ptr->mouse->setScreen(this);

        const QString keyboardDev = QString(QLatin1String(QT_VFB_KEYBOARD_PIPE))
                                    .arg(displayId);
#ifndef QT_NO_QWS_KEYBOARD
        d_ptr->keyboard = QKbdDriverFactory::create(QLatin1String("QVFbKbd"),
                                                    keyboardDev);
        qwsServer->setDefaultKeyboard("None");
#endif

        if (d_ptr->hdr->dataoffset >= (int)sizeof(QVFbHeader))
            d_ptr->hdr->serverVersion = QT_VERSION;
    }

    return true;
}

void QVFbScreen::disconnect()
{
    if ((long)d_ptr->shmrgn != -1 && d_ptr->shmrgn) {
        if (qApp->type() == QApplication::GuiServer && d_ptr->hdr->dataoffset >= (int)sizeof(QVFbHeader)) {
            d_ptr->hdr->serverVersion = 0;
        }
        shmdt((char*)d_ptr->shmrgn);
    }
}

bool QVFbScreen::initDevice()
{
    if(d==8) {
        screencols=256;
        if (grayscale) {
            // Build grayscale palette
            for(int loopc=0;loopc<256;loopc++) {
                screenclut[loopc]=qRgb(loopc,loopc,loopc);
            }
        } else {
            // 6x6x6 216 color cube
            int idx = 0;
            for(int ir = 0x0; ir <= 0xff; ir+=0x33) {
                for(int ig = 0x0; ig <= 0xff; ig+=0x33) {
                    for(int ib = 0x0; ib <= 0xff; ib+=0x33) {
                        screenclut[idx]=qRgb(ir, ig, ib);
                        idx++;
                    }
                }
            }
            screencols=idx;
        }
        memcpy(d_ptr->hdr->clut, screenclut, sizeof(QRgb) * screencols);
        d_ptr->hdr->numcols = screencols;
    } else if (d == 4) {
        int val = 0;
        for (int idx = 0; idx < 16; idx++, val += 17) {
            screenclut[idx] = qRgb(val, val, val);
        }
        screencols = 16;
        memcpy(d_ptr->hdr->clut, screenclut, sizeof(QRgb) * screencols);
        d_ptr->hdr->numcols = screencols;
    } else if (d == 1) {
        screencols = 2;
        screenclut[1] = qRgb(0xff, 0xff, 0xff);
        screenclut[0] = qRgb(0, 0, 0);
        memcpy(d_ptr->hdr->clut, screenclut, sizeof(QRgb) * screencols);
        d_ptr->hdr->numcols = screencols;
    }

#ifndef QT_NO_QWS_CURSOR
    QScreenCursor::initSoftwareCursor();
#endif
    return true;
}

void QVFbScreen::shutdownDevice()
{
}

void QVFbScreen::setMode(int ,int ,int)
{
}

// save the state of the graphics card
// This is needed so that e.g. we can restore the palette when switching
// between linux virtual consoles.
void QVFbScreen::save()
{
    // nothing to do.
}

// restore the state of the graphics card.
void QVFbScreen::restore()
{
}
void QVFbScreen::setDirty(const QRect& rect)
{
    const QRect r = rect.translated(-offset());
    d_ptr->hdr->dirty = true;
    d_ptr->hdr->update = d_ptr->hdr->update.united(r);
}



#endif // QT_NO_QWS_QVFB

