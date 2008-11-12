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

#include "qpixmap_p.h"

#include "qwidget.h"
#include "qbitmap.h"
#include "qcolormap.h"
#include "qimage.h"
#include "qmatrix.h"
#include "qapplication.h"
#include "qpainter.h"
#include "qwsdisplay_qws.h"
#include "qscreen_qws.h"
#include "qhash.h"
#include "qdesktopwidget.h"
#include <stdlib.h>
#include <limits.h>

#include <private/qpaintengine_raster_p.h>
#include <private/qwidget_p.h>
#include <private/qbackingstore_p.h>

#include <qdebug.h>

QPixmap QPixmap::grabWindow(WId window, int x, int y, int w, int h)
{
    QWidget *widget = QWidget::find(window);
    if (!widget)
        return QPixmap();

    QRect grabRect = widget->frameGeometry();
    if (w < 0)
        w = widget->width() - x;
    if (h < 0)
        h = widget->height() - y;
    grabRect &= QRect(x, y, w, h).translated(widget->mapToGlobal(QPoint()));

    QPixmap pixmap;

    QScreen *screen = qt_screen;
    QDesktopWidget *desktop = QApplication::desktop();
    if (!desktop)
        return QPixmap();
    if (desktop->numScreens() > 1) {
        const int screenNo = desktop->screenNumber(widget);
        if (screenNo != -1)
            screen = qt_screen->subScreens().at(screenNo);
        grabRect = grabRect.translated(-screen->region().boundingRect().topLeft());
    }

    QWSDisplay::grab(false);

    if (screen->pixelFormat() != QImage::Format_Invalid) {
        const QImage img(screen->base(),
                         screen->width(), screen->height(),
                         screen->linestep(), screen->pixelFormat());
        pixmap = fromImage(img.copy(grabRect));
    } else if (screen->base()) {

        // temporary workaraounds for screen depths supported by the linuxfb
        // driver. Should be replaced by some QScreen::toImage() function
        // or something...
        switch (screen->depth()) {
#ifdef QT_QWS_DEPTH_24
        case 24: {
            QImage img(grabRect.width(), grabRect.height(), QImage::Format_RGB32);
            const uchar *src = screen->base()
                               + screen->linestep() * grabRect.y()
                               + sizeof(quint24) * grabRect.x();
            qt_rectconvert<quint32, quint24>(reinterpret_cast<quint32*>(img.bits()),
                                             reinterpret_cast<const quint24*>(src),
                                             0, 0, grabRect.width(), grabRect.height(),
                                             img.bytesPerLine(),
                                             screen->linestep());
            pixmap = fromImage(img);
            break;
        }
#endif // QT_QWS_DEPTH_24
#ifdef QT_QWS_DEPTH_18
        case 18: {
            QImage img(grabRect.width(), grabRect.height(), QImage::Format_RGB32);
            const uchar *src = screen->base()
                               + screen->linestep() * grabRect.y()
                               + sizeof(quint18) * grabRect.x();
            qt_rectconvert<quint32, quint18>(reinterpret_cast<quint32*>(img.bits()),
                                             reinterpret_cast<const quint18*>(src),
                                             0, 0, grabRect.width(), grabRect.height(),
                                             img.bytesPerLine(),
                                             screen->linestep());
            pixmap = fromImage(img);
            break;
        }
#endif // QT_QWS_DEPTH_18
        default:
            qWarning("QPixmap::grabWindow(): Unsupported screen depth %d",
                     screen->depth());
            break;
        }
    } else {
        qWarning("QPixmap::grabWindow(): Unable to copy pixels from framebuffer");
    }

    QWSDisplay::ungrab();

    return pixmap;
}



/*!
    \internal
*/
QRgb * QPixmap::clut() const
{
    return data->image.colorTable().data();
}

/*!
    \internal
*/
int QPixmap::numCols() const
{
    return data->image.numColors();
}

/*!
    \internal
    \since 4.1
*/
const uchar *QPixmap::qwsBits() const
{
    return data->image.bits();
}

/*!
    \internal
    \since 4.1
*/
int QPixmap::qwsBytesPerLine() const
{
    return data->image.bytesPerLine();
}
