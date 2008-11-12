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

#ifndef QPIXMAP_P_H
#define QPIXMAP_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtGui/qpixmap.h"
#if defined(Q_WS_X11)
#include "QtGui/qx11info_x11.h"
#endif

#if defined(Q_WS_WIN)
#include "qt_windows.h"
#ifndef QT_NO_DIRECT3D
#include <d3d9.h>
#endif
#endif

#if defined(Q_WS_WIN) || defined(Q_WS_QWS)

struct QPixmapData { // internal pixmap data
    QPixmapData() : count(1) { }
    ~QPixmapData(){};
    void ref() { ++count; }
    bool deref() { return !--count; }
    int count;
    int detach_no;
    QImage image;
    QPixmap::Type type;
#if !defined(QT_NO_DIRECT3D) && defined(Q_WS_WIN)
    IDirect3DTexture9 *texture;
#endif

    QImage createBitmapImage(int w, int h);
};

#else // non raster

struct QPixmapData { // internal pixmap data
    QPixmapData() : count(1) { }
    ~QPixmapData();

    void ref() { ++count; }
    bool deref() { return !--count; }
    int count;
    QPixmap::Type type;

    int w, h;
    short d;
    uint uninit:1;
    int ser_no;
    int detach_no;
#if !defined(Q_WS_X11) && !defined(Q_WS_MAC)
    QBitmap *mask;
#endif
#if defined(Q_WS_X11)
    QX11Info xinfo;
    Qt::HANDLE x11_mask;
    Qt::HANDLE picture;
    Qt::HANDLE mask_picture;
    Qt::HANDLE hd2; // sorted in the default display depth
    Qt::HANDLE x11ConvertToDefaultDepth();
#ifndef QT_NO_XRENDER
    void convertToARGB32();
#endif
#elif defined(Q_WS_MAC)
    uint has_alpha : 1, has_mask : 1;
    void macSetHasAlpha(bool b);
    void macGetAlphaChannel(QPixmap *, bool asMask) const;
    void macSetAlphaChannel(const QPixmap *, bool asMask);
    quint32 *pixels;
    uint nbytes;
    QRectF cg_mask_rect;
    CGImageRef cg_data, cg_mask;
#ifdef Q_WS_MAC32
    GWorldPtr qd_data, qd_alpha;
    void macQDDisposeAlpha();
    void macQDUpdateAlpha();
#endif
#endif
    QPaintEngine *paintEngine;
#if !defined(Q_WS_MAC)
    Qt::HANDLE hd;
#endif
#ifdef Q_WS_X11
    QBitmap mask_to_bitmap(int screen) const;
    static Qt::HANDLE bitmap_to_mask(const QBitmap &, int screen);
#endif
    static int allocCell(const QPixmap *p);
    static void freeCell(QPixmapData *data, bool terminate = false);
};

#endif // Q_WS_WIN

#ifdef Q_WS_WIN
QPixmap convertHIconToPixmap( const HICON icon);
QPixmap loadIconFromShell32( int resourceId, int size );
#endif

#  define QT_XFORM_TYPE_MSBFIRST 0
#  define QT_XFORM_TYPE_LSBFIRST 1
#  if defined(Q_WS_WIN)
#    define QT_XFORM_TYPE_WINDOWSPIXMAP 2
#  endif
extern bool qt_xForm_helper(const QTransform&, int, int, int, uchar*, int, int, int, const uchar*, int, int, int);

#endif // QPIXMAP_P_H
