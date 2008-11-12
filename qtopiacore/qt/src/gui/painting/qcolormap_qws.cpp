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

#include "qcolormap.h"
#include "qcolor.h"
#include "qpaintdevice.h"
#include "qscreen_qws.h"
#include "qwsdisplay_qws.h"

class QColormapPrivate
{
public:
    inline QColormapPrivate()
        : ref(1), mode(QColormap::Direct), depth(0), numcolors(0)
    { }

    QAtomic ref;

    QColormap::Mode mode;
    int depth;
    int numcolors;
};

static QColormapPrivate *screenMap = 0;

void QColormap::initialize()
{
    screenMap = new QColormapPrivate;

    screenMap->depth = QPaintDevice::qwsDisplay()->depth();
    if (screenMap->depth < 8) {
        screenMap->mode = QColormap::Indexed;
        screenMap->numcolors = 256;
    } else {
        screenMap->mode = QColormap::Direct;
        screenMap->numcolors = -1;
    }
}

void QColormap::cleanup()
{
    delete screenMap;
    screenMap = 0;
}

QColormap QColormap::instance(int /*screen*/)
{
    return QColormap();
}

QColormap::QColormap()
    : d(screenMap)
{ d->ref.ref(); }

QColormap::QColormap(const QColormap &colormap)
    :d (colormap.d)
{ d->ref.ref(); }

QColormap::~QColormap()
{
    if (!d->ref.deref())
        delete d;
}

QColormap::Mode QColormap::mode() const
{ return d->mode; }


int QColormap::depth() const
{ return d->depth; }


int QColormap::size() const
{
    return d->numcolors;
}

uint QColormap::pixel(const QColor &color) const
{
    QRgb rgb = color.rgba();
    if (d->mode == QColormap::Direct) {
        switch(d->depth) {
        case 16:
            return qt_convRgbTo16(rgb);
        case 24:
        case 32:
        {
            const int r = qRed(rgb);
            const int g = qGreen(rgb);
            const int b = qBlue(rgb);
            const int red_shift = 16;
            const int green_shift = 8;
            const int red_mask   = 0xff0000;
            const int green_mask = 0x00ff00;
            const int blue_mask  = 0x0000ff;
            const int tg = g << green_shift;
#ifdef QT_QWS_DEPTH_32_BGR
            if (qt_screen->pixelType() == QScreen::BGRPixel) {
                const int tb = b << red_shift;
                return 0xff000000 | (r & blue_mask) | (tg & green_mask) | (tb & red_mask);
            }
#endif
            const int tr = r << red_shift;
            return 0xff000000 | (b & blue_mask) | (tg & green_mask) | (tr & red_mask);
        }
        }
    }
    return qt_screen->alloc(qRed(rgb), qGreen(rgb), qBlue(rgb));
}

const QColor QColormap::colorAt(uint pixel) const
{
    if (d->mode == Direct) {
        if (d->depth == 16) {
            pixel = qt_conv16ToRgb(pixel);
        }
        const int red_shift = 16;
        const int green_shift = 8;
        const int red_mask   = 0xff0000;
        const int green_mask = 0x00ff00;
        const int blue_mask  = 0x0000ff;
#ifdef QT_QWS_DEPTH_32_BGR
        if (qt_screen->pixelType() == QScreen::BGRPixel) {
            return QColor((pixel & blue_mask),
                          (pixel & green_mask) >> green_shift,
                          (pixel & red_mask) >> red_shift);
        }
#endif
        return QColor((pixel & red_mask) >> red_shift,
                      (pixel & green_mask) >> green_shift,
                      (pixel & blue_mask));
    }
    Q_ASSERT_X(int(pixel) < qt_screen->numCols(), "QColormap::colorAt", "pixel out of bounds of palette");
    return QColor(qt_screen->clut()[pixel]);
}

const QVector<QColor> QColormap::colormap() const
{
    return QVector<QColor>();
}

QColormap &QColormap::operator=(const QColormap &colormap)
{ qAtomicAssign(d, colormap.d); return *this; }

