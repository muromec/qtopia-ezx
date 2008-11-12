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

// Uncomment the next line to enable the MIT Shared Memory extension
//
// WARNING:  This has some problems:
//
//    1. Consumes a 800x600 pixmap
//    2. Qt does not handle the ShmCompletion message, so you will
//        get strange effects if you xForm() repeatedly.
//
// #define QT_MITSHM

#if defined(Q_OS_WIN32) && defined(QT_MITSHM)
#undef QT_MITSHM
#endif

#include "qplatformdefs.h"

#include "qdebug.h"
#include "qiodevice.h"
#include "qpixmap_p.h"
#include "qbitmap.h"
#include "qcolormap.h"
#include "qimage.h"
#include "qmatrix.h"
#include "qapplication.h"
#include <private/qpaintengine_x11_p.h>
#include <private/qt_x11_p.h>
#include "qx11info_x11.h"
#include <private/qdrawhelper_p.h>

#include <stdlib.h>

#if defined(Q_CC_MIPS)
#  define for if(0){}else for
#endif

// For thread-safety:
//   image->data does not belong to X11, so we must free it ourselves.

inline static void qSafeXDestroyImage(XImage *x)
{
    if (x->data) {
        free(x->data);
        x->data = 0;
    }
    XDestroyImage(x);
}

QBitmap QPixmapData::mask_to_bitmap(int screen) const
{
    if (!x11_mask)
        return QBitmap();
    QPixmap::x11SetDefaultScreen(screen);
    QBitmap bm(w, h);
    GC gc = XCreateGC(X11->display, bm.data->hd, 0, 0);
    XCopyArea(X11->display, x11_mask, bm.data->hd, gc, 0, 0, bm.data->w, bm.data->h, 0, 0);
    XFreeGC(X11->display, gc);
    return bm;
}

Qt::HANDLE QPixmapData::bitmap_to_mask(const QBitmap &bitmap, int screen)
{
    if (bitmap.isNull())
        return 0;
    QBitmap bm = bitmap;
    bm.x11SetScreen(screen);

    Pixmap mask = XCreatePixmap(X11->display, RootWindow(X11->display, screen),
                                bm.data->w, bm.data->h, 1);
    GC gc = XCreateGC(X11->display, mask, 0, 0);
    XCopyArea(X11->display, bm.data->hd, mask, gc, 0, 0, bm.data->w, bm.data->h, 0, 0);
    XFreeGC(X11->display, gc);
    return mask;
}


/*****************************************************************************
  MIT Shared Memory Extension support: makes xForm noticeably (~20%) faster.
 *****************************************************************************/

#if defined(QT_MITSHM)

static bool               xshminit = false;
static XShmSegmentInfo xshminfo;
static XImage              *xshmimg = 0;
static Pixmap               xshmpm  = 0;

static void qt_cleanup_mitshm()
{
    if (xshmimg == 0)
        return;
    Display *dpy = QX11Info::appDisplay();
    if (xshmpm) {
        XFreePixmap(dpy, xshmpm);
        xshmpm = 0;
    }
    XShmDetach(dpy, &xshminfo); xshmimg->data = 0;
    qSafeXDestroyImage(xshmimg); xshmimg = 0;
    shmdt(xshminfo.shmaddr);
    shmctl(xshminfo.shmid, IPC_RMID, 0);
}

static bool qt_create_mitshm_buffer(const QPaintDevice* dev, int w, int h)
{
    static int major, minor;
    static Bool pixmaps_ok;
    Display *dpy = dev->data->xinfo->display();
    int dd         = dev->x11Depth();
    Visual *vis         = (Visual*)dev->x11Visual();

    if (xshminit) {
        qt_cleanup_mitshm();
    } else {
        if (!XShmQueryVersion(dpy, &major, &minor, &pixmaps_ok))
            return false;                        // MIT Shm not supported
        qAddPostRoutine(qt_cleanup_mitshm);
        xshminit = true;
    }

    xshmimg = XShmCreateImage(dpy, vis, dd, ZPixmap, 0, &xshminfo, w, h);
    if (!xshmimg)
        return false;

    bool ok;
    xshminfo.shmid = shmget(IPC_PRIVATE,
                             xshmimg->bytes_per_line * xshmimg->height,
                             IPC_CREAT | 0777);
    ok = xshminfo.shmid != -1;
    if (ok) {
        xshmimg->data = (char*)shmat(xshminfo.shmid, 0, 0);
        xshminfo.shmaddr = xshmimg->data;
        ok = (xshminfo.shmaddr != (char*)-1);
    }
    xshminfo.readOnly = false;
    if (ok)
        ok = XShmAttach(dpy, &xshminfo);
    if (!ok) {
        qSafeXDestroyImage(xshmimg);
        xshmimg = 0;
        if (xshminfo.shmaddr)
            shmdt(xshminfo.shmaddr);
        if (xshminfo.shmid != -1)
            shmctl(xshminfo.shmid, IPC_RMID, 0);
        return false;
    }
    if (pixmaps_ok)
        xshmpm = XShmCreatePixmap(dpy, DefaultRootWindow(dpy), xshmimg->data,
                                   &xshminfo, w, h, dd);

    return true;
}

#else

// If extern, need a dummy.
//
// static bool qt_create_mitshm_buffer(QPaintDevice*, int, int)
// {
//     return false;
// }

#endif // QT_MITSHM


/*****************************************************************************
  Internal functions
 *****************************************************************************/

extern const uchar *qt_get_bitflip_array();                // defined in qimage.cpp

// Returns position of highest bit set or -1 if none
static int highest_bit(uint v)
{
    int i;
    uint b = (uint)1 << 31;
    for (i=31; ((b & v) == 0) && i>=0;         i--)
        b >>= 1;
    return i;
}

// Returns position of lowest set bit in 'v' as an integer (0-31), or -1
static int lowest_bit(uint v)
{
    int i;
    ulong lb;
    lb = 1;
    for (i=0; ((v & lb) == 0) && i<32;  i++, lb<<=1);
    return i==32 ? -1 : i;
}

// Counts the number of bits set in 'v'
static uint n_bits(uint v)
{
    int i = 0;
    while (v) {
        v = v & (v - 1);
        i++;
    }
    return i;
}

static uint *red_scale_table   = 0;
static uint *green_scale_table = 0;
static uint *blue_scale_table  = 0;

static void cleanup_scale_tables()
{
    delete[] red_scale_table;
    delete[] green_scale_table;
    delete[] blue_scale_table;
}

/*
  Could do smart bitshifting, but the "obvious" algorithm only works for
  nBits >= 4. This is more robust.
*/
static void build_scale_table(uint **table, uint nBits)
{
    if (nBits > 7) {
        qWarning("build_scale_table: internal error, nBits = %i", nBits);
        return;
    }
    if (!*table) {
        static bool firstTable = true;
        if (firstTable) {
            qAddPostRoutine(cleanup_scale_tables);
            firstTable = false;
        }
        *table = new uint[256];
    }
    int   maxVal   = (1 << nBits) - 1;
    int   valShift = 8 - nBits;
    int i;
    for(i = 0 ; i < maxVal + 1 ; i++)
        (*table)[i << valShift] = i*255/maxVal;
}

static int defaultScreen = -1;

/*****************************************************************************
  QPixmap member functions
 *****************************************************************************/

static int qt_pixmap_serial = 0;
int Q_GUI_EXPORT qt_x11_preferred_pixmap_depth = 0;

/*!
  \internal
  Initializes the pixmap data.
*/
void QPixmap::init(int w, int h, Type type)
{
    if (!qApp) {
        qFatal("QPixmap: Must construct a QApplication before a QPaintDevice");
        return;
    }

    if (qApp->type() == QApplication::Tty) {
        qWarning("QPixmap: Cannot create a QPixmap when no GUI is being used");
    }

    data = new QPixmapData;
    memset(data, 0, sizeof(QPixmapData));
    data->type = type;
    data->count  = 1;
    data->uninit = true;
    data->ser_no = ++qt_pixmap_serial;
    data->detach_no = 0;
    data->picture = 0;
    data->x11_mask = 0;
    data->mask_picture = 0;

    if (defaultScreen >= 0 && defaultScreen != data->xinfo.screen()) {
        QX11InfoData* xd = data->xinfo.getX11Data(true);
        xd->screen = defaultScreen;
        xd->depth = QX11Info::appDepth(xd->screen);
        xd->cells = QX11Info::appCells(xd->screen);
        xd->colormap = QX11Info::appColormap(xd->screen);
        xd->defaultColormap = QX11Info::appDefaultColormap(xd->screen);
        xd->visual = (Visual *)QX11Info::appVisual(xd->screen);
        xd->defaultVisual = QX11Info::appDefaultVisual(xd->screen);
        data->xinfo.setX11Data(xd);
    }

    int dd = data->xinfo.depth();

    if (qt_x11_preferred_pixmap_depth)
        dd = qt_x11_preferred_pixmap_depth;

    bool make_null = w == 0 || h == 0;                // create null pixmap
    data->d = (type == PixmapType) ? dd : 1;
    if (make_null || w < 0 || h < 0 || data->d == 0) {
        data->hd = 0;
        data->picture = 0;
        if (!make_null)
            qWarning("QPixmap: Invalid pixmap parameters");
        return;
    }
    data->w = w;
    data->h = h;
    data->hd = (Qt::HANDLE)XCreatePixmap(X11->display,
                                         RootWindow(X11->display,
                                                    data->xinfo.screen()),
                                         w, h, data->d);
#ifndef QT_NO_XRENDER
    if (X11->use_xrender) {
        XRenderPictFormat *format = data->d == 1
                                    ? XRenderFindStandardFormat(X11->display, PictStandardA1)
                                    : XRenderFindVisualFormat(X11->display, (Visual *) data->xinfo.visual());
        data->picture = XRenderCreatePicture(X11->display, data->hd, format, 0, 0);
    }
#endif // QT_NO_XRENDER
}

QPixmapData::~QPixmapData()
{
    if (!qApp)
        return;
    if (x11_mask) {
#ifndef QT_NO_XRENDER
        if (mask_picture)
            XRenderFreePicture(X11->display, mask_picture);
        mask_picture = 0;
#endif
        XFreePixmap(X11->display, x11_mask);
        x11_mask = 0;
    }
    if (hd) {

#ifndef QT_NO_XRENDER
        if (picture) {
            XRenderFreePicture(X11->display, picture);
            picture = 0;
        }
#endif // QT_NO_XRENDER

        if (hd2) {
            XFreePixmap(xinfo.display(), hd2);
            hd2 = 0;
        }
        XFreePixmap(xinfo.display(), hd);
        hd = 0;
    }
    delete paintEngine;
}

typedef void (*_qt_pixmap_cleanup_hook_64)(qint64);
extern _qt_pixmap_cleanup_hook_64 qt_pixmap_cleanup_hook_64;

/*!
    Detaches the pixmap from shared pixmap data.

    A pixmap is automatically detached by Qt whenever its contents are
    about to change. This is done in almost all QPixmap member
    functions that modify the pixmap (fill(), fromImage(),
    load(), etc.), and in QPainter::begin() on a pixmap.

    There are two exceptions in which detach() must be called
    explicitly, that is when calling the handle() or the
    x11PictureHandle() function (only available on X11). Otherwise,
    any modifications done using system calls, will be performed on
    the shared data.

    The detach() function returns immediately if there is just a
    single reference or if the pixmap has not been initialized yet.
*/
void QPixmap::detach()
{
    if (qt_pixmap_cleanup_hook_64 && data->count == 1)
        qt_pixmap_cleanup_hook_64(cacheKey());

    if (data->count != 1)
        *this = copy();
    ++data->detach_no;
    data->uninit = false;

    // reset the cache data
    if (data->hd2) {
        XFreePixmap(X11->display, data->hd2);
        data->hd2 = 0;
    }
}


/*!
    Returns the default pixmap depth used by the application.

    \sa depth(), {QPixmap#Pixmap Information}{Pixmap Information}
*/

int QPixmap::defaultDepth()
{
    return QX11Info::appDepth();
}

/*!
    Fills the pixmap with the given  \a fillColor.

    \sa {QPixmap#Pixmap Transformations}{Pixmap Transformations}
*/

void QPixmap::fill(const QColor &fillColor)
{
    if (isNull())
        return;
    if (fillColor.alpha() != 255) {
#ifndef QT_NO_XRENDER
        if (data->picture && data->d == 32) {
            detach();
            ::Picture src  = X11->getSolidFill(data->xinfo.screen(), fillColor);
            XRenderComposite(X11->display, PictOpSrc, src, 0, data->picture,
                             0, 0, width(), height(),
                             0, 0, width(), height());
        } else
#endif
        {
            QImage im(width(), height(), QImage::Format_ARGB32_Premultiplied);
            im.fill(PREMUL(fillColor.rgba()));
            *this = QPixmap::fromImage(im);
        }
        return;
    } else {
        detach();
    }
    GC gc = XCreateGC(X11->display, data->hd, 0, 0);
    if (depth() == 1) {
        XSetForeground(X11->display, gc, qGray(fillColor.rgb()) > 127 ? 0 : 1);
    } else if (X11->use_xrender && data->d >= 24) {
        XSetForeground(X11->display, gc, fillColor.rgba());
    } else {
        XSetForeground(X11->display, gc,
                       QColormap::instance(data->xinfo.screen()).pixel(fillColor));
    }
    XFillRectangle(X11->display, data->hd, gc, 0, 0, width(), height());
    XFreeGC(X11->display, gc);
}

/*!
    Returns the alpha channel of the pixmap as a new grayscale QPixmap in which
    each pixel's red, green, and blue values are given the alpha value of the
    original pixmap. The color depth of the returned pixmap is the system depth
    on X11 and 8-bit on Windows and Mac OS X.

    You can use this function while debugging
    to get a visible image of the alpha channel. If the pixmap doesn't have an
    alpha channel, i.e., the alpha channel's value for all pixels equals
    0xff), a null pixmap is returned. You can check this with the \c isNull()
    function.

    We show an example:

    \quotefromfile snippets/alphachannel.cpp
    \skipto /pixmap =/
    \printuntil /update/

    \image alphachannelimage.png The pixmap and channelImage QPixmaps

    \sa setAlphaChannel(), {QPixmap#Pixmap Information}{Pixmap
    Information}
*/

QPixmap QPixmap::alphaChannel() const
{
    if (!hasAlphaChannel())
        return QPixmap();
    QImage im(toImage());
    return fromImage(im.alphaChannel(), Qt::OrderedDither);
}

/*!
    \fn void QPixmap::setAlphaChannel(const QPixmap &alphaChannel)

    Sets the alpha channel of this pixmap to the given \a alphaChannel
    by converting the \a alphaChannel into 32 bit and using the
    intensity of the RGB pixel values.

    The effect of this function is undefined when the pixmap is being
    painted on.

    \sa alphaChannel(), {QPixmap#Pixmap Transformations}{Pixmap
    Transformations}
 */
void QPixmap::setAlphaChannel(const QPixmap &alpha)
{
    if (paintingActive()) {
        qWarning("QPixmap::setAlphaChannel: Should not set alpha channel while pixmap is being painted on");
    }

    if (alpha.isNull())
        return;

    if (width() != alpha.width() && height() != alpha.height()) {
        qWarning("QPixmap::setAlphaChannel: The pixmap and the alpha channel pixmap must have the same size");
        return;
    }
    QImage im(toImage());
    im.setAlphaChannel(alpha.toImage());
    *this = fromImage(im, Qt::OrderedDither | Qt::OrderedAlphaDither);
}


/*!
    \fn QBitmap QPixmap::mask() const

    Returns the mask, or a null bitmap if no mask has been set.

    \sa setMask(), {QPixmap#Pixmap Information}{Pixmap Information}
*/
QBitmap QPixmap::mask() const
{
    QBitmap mask;
#ifndef QT_NO_XRENDER
    if (data->picture && data->d == 32) {
        // #### slow - there must be a better way..
        mask = QBitmap::fromImage(toImage().createAlphaMask());
    } else
#endif
    if (depth() == 1) {
        mask = *this;
    } else {
        mask = data->mask_to_bitmap(data->xinfo.screen());
    }
    return mask;
}


/*!
    Sets a mask bitmap.

    The \a newmask bitmap defines the clip mask for this pixmap. Every
    pixel in \a newmask corresponds to a pixel in this pixmap. Pixel
    value 1 means opaque and pixel value 0 means transparent. The mask
    must have the same size as this pixmap.

    \warning Setting the mask on a pixmap will cause any alpha channel
    data to be cleared. For example:
    \quotefromfile snippets/image/image.cpp
    \skipto MASK
    \skipto QPixmap
    \printuntil setMask
    Now, alpha and alphacopy are visually different.

    Setting a null mask resets the mask.

    The effect of this function is undefined when the pixmap is being
    painted on.

    \sa mask(), {QPixmap#Pixmap Transformations}{Pixmap
    Transformations}, QBitmap
*/
void QPixmap::setMask(const QBitmap &newmask)
{
    if (paintingActive()) {
        qWarning("QPixmap::setMask: Should not set mask while pixmap is being painted on");
    }

    if (data == newmask.data)
        // trying to selfmask
        return;

    if (newmask.isNull()) { // clear mask
        detach();
#ifndef QT_NO_XRENDER
        if (data->picture && data->d == 32) {
            QPixmap pixmap;
            if (data->type == QPixmap::BitmapType)
                pixmap = QBitmap(data->w, data->h);
            else
                pixmap = QPixmap(data->w, data->h);
            pixmap.fill(Qt::black);
            XRenderComposite(X11->display, PictOpOver,
                             data->picture, 0,
                             pixmap.data->picture, 0, 0, 0, 0, 0, 0, data->w, data->h);
            *this = pixmap;
        } else
#endif
            if (data->x11_mask) {
#ifndef QT_NO_XRENDER
                if (data->picture) {
                    XRenderPictureAttributes attrs;
                    attrs.alpha_map = 0;
                    XRenderChangePicture(X11->display, data->picture, CPAlphaMap, &attrs);
                }
                if (data->mask_picture)
                    XRenderFreePicture(X11->display, data->mask_picture);
                data->mask_picture = 0;
#endif
                XFreePixmap(X11->display, data->x11_mask);
                data->x11_mask = 0;
            }
        return;
    }

    if (newmask.width() != width() || newmask.height() != height()) {
        qWarning("QPixmap::setMask: The pixmap and the mask must have the same size");
        return;
    }

    detach();

#ifndef QT_NO_XRENDER
    if (data->picture && data->d == 32) {
        XRenderComposite(X11->display, PictOpSrc,
                         data->picture, newmask.x11PictureHandle(),
                         data->picture, 0, 0, 0, 0, 0, 0, data->w, data->h);
    } else
#endif
        if (depth() == 1) {
            XGCValues vals;
            vals.function = GXand;
            GC gc = XCreateGC(X11->display, data->hd, GCFunction, &vals);
            XCopyArea(X11->display, newmask.handle(), data->hd, gc, 0, 0, width(), height(), 0, 0);
            XFreeGC(X11->display, gc);
        } else {
            // ##### should or the masks together
            if (data->x11_mask) {
                XFreePixmap(X11->display, data->x11_mask);
#ifndef QT_NO_XRENDER
                if (data->mask_picture)
                    XRenderFreePicture(X11->display, data->mask_picture);
#endif
            }
            data->x11_mask = QPixmapData::bitmap_to_mask(newmask, data->xinfo.screen());
#ifndef QT_NO_XRENDER
            if (data->picture) {
                data->mask_picture = XRenderCreatePicture(X11->display, data->x11_mask,
                                                          XRenderFindStandardFormat(X11->display, PictStandardA1), 0, 0);
                XRenderPictureAttributes attrs;
                attrs.alpha_map = data->mask_picture;
                XRenderChangePicture(X11->display, data->picture, CPAlphaMap, &attrs);
            }
#endif
        }
}

/*!
  \reimp
*/

int QPixmap::metric(PaintDeviceMetric m) const
{
    int val;
    if (m == PdmWidth)
        val = width();
    else if (m == PdmHeight) {
        val = height();
    } else {
        Display *dpy = X11->display;
        int scr = data->xinfo.screen();
        switch (m) {
            case PdmDpiX:
            case PdmPhysicalDpiX:
                val = QX11Info::appDpiX(scr);
                break;
            case PdmDpiY:
            case PdmPhysicalDpiY:
                val = QX11Info::appDpiY(scr);
                break;
            case PdmWidthMM:
                val = (DisplayWidthMM(dpy,scr)*width())/
                      DisplayWidth(dpy,scr);
                break;
            case PdmHeightMM:
                val = (DisplayHeightMM(dpy,scr)*height())/
                      DisplayHeight(dpy,scr);
                break;
            case PdmNumColors:
                val = 1 << depth();
                break;
            case PdmDepth:
                val = depth();
                break;
            default:
                val = 0;
                qWarning("QPixmap::metric: Invalid metric command");
        }
    }
    return val;
}

/*!
    Converts the pixmap to a QImage. Returns a null image if the
    conversion fails.

    If the pixmap has 1-bit depth, the returned image will also be 1
    bit deep. If the pixmap has 2- to 8-bit depth, the returned image
    has 8-bit depth. If the pixmap has greater than 8-bit depth, the
    returned image has 32-bit depth.

    Note that for the moment, alpha masks on monochrome images are
    ignored.

    \sa fromImage(), {QImage#Image Formats}{Image Formats}
*/

QImage QPixmap::toImage() const
{
    if (isNull())
        return QImage(); // null image

    int            w  = width();
    int            h  = height();
    int            d  = depth();
    Visual *visual = (Visual *) data->xinfo.visual();
    bool    trucol = (visual->c_class >= TrueColor) && d > 1;

    QImage::Format format = QImage::Format_Mono;
    if (d > 1 && d <= 8) {
        d = 8;
        format = QImage::Format_Indexed8;
    }
    // we could run into the situation where d == 8 AND trucol is true, which can
    // cause problems when converting to and from images.  in this case, always treat
    // the depth as 32...
    if (d > 8 || trucol) {
        d = 32;
        format = QImage::Format_RGB32;
    }

    XImage *xi = XGetImage(X11->display, data->hd, 0, 0, w, h, AllPlanes,
                           (d == 1) ? XYPixmap : ZPixmap);

    Q_CHECK_PTR(xi);
    if (!xi)
        return QImage();

    if (data->picture && data->d == 32) {
        QImage image(data->w, data->h, QImage::Format_ARGB32_Premultiplied);
        memcpy(image.bits(), xi->data, xi->bytes_per_line * xi->height);

        // we may have to swap the byte order
        if ((QSysInfo::ByteOrder == QSysInfo::LittleEndian && xi->byte_order == MSBFirst)
            || (QSysInfo::ByteOrder == QSysInfo::BigEndian))
        {
            for (int i=0; i < image.height(); i++) {
                uint *p = (uint*) image.scanLine(i);
                uint *end = p + image.width();
                if ((xi->byte_order == LSBFirst && QSysInfo::ByteOrder == QSysInfo::BigEndian)
                    || (xi->byte_order == MSBFirst && QSysInfo::ByteOrder == QSysInfo::LittleEndian)) {
                    while (p < end) {
                        *p = ((*p << 24) & 0xff000000) | ((*p << 8) & 0x00ff0000)
                             | ((*p >> 8) & 0x0000ff00) | ((*p >> 24) & 0x000000ff);
                        p++;
                    }
                } else if (xi->byte_order == MSBFirst && QSysInfo::ByteOrder == QSysInfo::BigEndian) {
                    while (p < end) {
                        *p = ((*p << 16) & 0x00ff0000) | ((*p >> 16) & 0x000000ff)
                             | ((*p ) & 0xff00ff00);
                        p++;
                    }
                }
            }
        }

        // throw away image data
        qSafeXDestroyImage(xi);

        return image;
    }

    if (d == 1 && xi->bitmap_bit_order == LSBFirst)
        format = QImage::Format_MonoLSB;
    if (data->x11_mask && format == QImage::Format_RGB32)
        format = QImage::Format_ARGB32;

    QImage image(w, h, format);
    if (image.isNull())                        // could not create image
        return image;

    QImage alpha;
    if (data->x11_mask) {
        alpha = mask().toImage();
    }
    bool ale = alpha.format() == QImage::Format_MonoLSB;

    if (trucol) {                                // truecolor
        const uint red_mask         = (uint)visual->red_mask;
        const uint green_mask         = (uint)visual->green_mask;
        const uint blue_mask         = (uint)visual->blue_mask;
        const int  red_shift         = highest_bit(red_mask)   - 7;
        const int  green_shift = highest_bit(green_mask) - 7;
        const int  blue_shift         = highest_bit(blue_mask)  - 7;

        const uint red_bits    = n_bits(red_mask);
        const uint green_bits  = n_bits(green_mask);
        const uint blue_bits   = n_bits(blue_mask);

        static uint red_table_bits   = 0;
        static uint green_table_bits = 0;
        static uint blue_table_bits  = 0;

        if (red_bits < 8 && red_table_bits != red_bits) {
            build_scale_table(&red_scale_table, red_bits);
            red_table_bits = red_bits;
        }
        if (blue_bits < 8 && blue_table_bits != blue_bits) {
            build_scale_table(&blue_scale_table, blue_bits);
            blue_table_bits = blue_bits;
        }
        if (green_bits < 8 && green_table_bits != green_bits) {
            build_scale_table(&green_scale_table, green_bits);
            green_table_bits = green_bits;
        }

        int  r, g, b;

        QRgb  *dst;
        uchar *src;
        uint   pixel;
        int    bppc = xi->bits_per_pixel;

        if (bppc > 8 && xi->byte_order == LSBFirst)
            bppc++;

        for (int y = 0; y < h; y++) {
            uchar* asrc = data->x11_mask ? alpha.scanLine(y) : 0;
            dst = (QRgb *)image.scanLine(y);
            src = (uchar *)xi->data + xi->bytes_per_line*y;
            for (int x = 0; x < w; x++) {
                switch (bppc) {
                case 8:
                    pixel = *src++;
                    break;
                case 16:                        // 16 bit MSB
                    pixel = src[1] | (uint)src[0] << 8;
                    src += 2;
                    break;
                case 17:                        // 16 bit LSB
                    pixel = src[0] | (uint)src[1] << 8;
                    src += 2;
                    break;
                case 24:                        // 24 bit MSB
                    pixel = src[2] | (uint)src[1] << 8 | (uint)src[0] << 16;
                    src += 3;
                    break;
                case 25:                        // 24 bit LSB
                    pixel = src[0] | (uint)src[1] << 8 | (uint)src[2] << 16;
                    src += 3;
                    break;
                case 32:                        // 32 bit MSB
                    pixel = src[3] | (uint)src[2] << 8 | (uint)src[1] << 16 | (uint)src[0] << 24;
                    src += 4;
                    break;
                case 33:                        // 32 bit LSB
                    pixel = src[0] | (uint)src[1] << 8 | (uint)src[2] << 16 | (uint)src[3] << 24;
                    src += 4;
                    break;
                default:                        // should not really happen
                    x = w;                        // leave loop
                    y = h;
                    pixel = 0;                // eliminate compiler warning
                    qWarning("QPixmap::convertToImage: Invalid depth %d", bppc);
                }
                if (red_shift > 0)
                    r = (pixel & red_mask) >> red_shift;
                else
                    r = (pixel & red_mask) << -red_shift;
                if (green_shift > 0)
                    g = (pixel & green_mask) >> green_shift;
                else
                    g = (pixel & green_mask) << -green_shift;
                if (blue_shift > 0)
                    b = (pixel & blue_mask) >> blue_shift;
                else
                    b = (pixel & blue_mask) << -blue_shift;

                if (red_bits < 8)
                    r = red_scale_table[r];
                if (green_bits < 8)
                    g = green_scale_table[g];
                if (blue_bits < 8)
                    b = blue_scale_table[b];

                if (data->x11_mask) {
                    if (ale) {
                        *dst++ = (asrc[x >> 3] & (1 << (x & 7))) ? qRgba(r, g, b, 0xff) : 0;
                    } else {
                        *dst++ = (asrc[x >> 3] & (0x80 >> (x & 7))) ? qRgba(r, g, b, 0xff) : 0;
                    }
                } else {
                    *dst++ = qRgb(r, g, b);
                }
            }
        }
    } else if (xi->bits_per_pixel == d) {        // compatible depth
        char *xidata = xi->data;                // copy each scanline
        int bpl = qMin(image.bytesPerLine(),xi->bytes_per_line);
        for (int y=0; y<h; y++) {
            memcpy(image.scanLine(y), xidata, bpl);
            xidata += xi->bytes_per_line;
        }
    } else {
        /* Typically 2 or 4 bits display depth */
        qWarning("QPixmap::convertToImage: Display not supported (bpp=%d)",
                 xi->bits_per_pixel);
        return QImage();
    }

    if (d == 1) {                                // bitmap
        image.setNumColors(2);
        image.setColor(0, qRgb(255,255,255));
        image.setColor(1, qRgb(0,0,0));
    } else if (!trucol) {                        // pixmap with colormap
        register uchar *p;
        uchar *end;
        uchar  use[256];                        // pixel-in-use table
        uchar  pix[256];                        // pixel translation table
        int    ncols, bpl;
        memset(use, 0, 256);
        memset(pix, 0, 256);
        bpl = image.bytesPerLine();

        if (data->x11_mask) {                                // which pixels are used?
            for (int i = 0; i < h; i++) {
                uchar* asrc = alpha.scanLine(i);
                p = image.scanLine(i);
                if (ale) {
                    for (int x = 0; x < w; x++) {
                        if (asrc[x >> 3] & (1 << (x & 7)))
                            use[*p] = 1;
                        ++p;
                    }
                } else {
                    for (int x = 0; x < w; x++) {
                        if (asrc[x >> 3] & (0x80 >> (x & 7)))
                            use[*p] = 1;
                        ++p;
                    }
                }
            }
        } else {
            for (int i = 0; i < h; i++) {
                p = image.scanLine(i);
                end = p + bpl;
                while (p < end)
                    use[*p++] = 1;
            }
        }
        ncols = 0;
        for (int i = 0; i < 256; i++) {                // build translation table
            if (use[i])
                pix[i] = ncols++;
        }
        for (int i = 0; i < h; i++) {                        // translate pixels
            p = image.scanLine(i);
            end = p + bpl;
            while (p < end) {
                *p = pix[*p];
                p++;
            }
        }
        if (data->x11_mask) {
            int trans;
            if (ncols < 256) {
                trans = ncols++;
                image.setNumColors(ncols);        // create color table
                image.setColor(trans, 0x00000000);
            } else {
                image.setNumColors(ncols);        // create color table
                // oh dear... no spare "transparent" pixel.
                // use first pixel in image (as good as any).
                trans = image.scanLine(0)[0];
            }
            for (int i = 0; i < h; i++) {
                uchar* asrc = alpha.scanLine(i);
                p = image.scanLine(i);
                if (ale) {
                    for (int x = 0; x < w; x++) {
                        if (!(asrc[x >> 3] & (1 << (x & 7))))
                            *p = trans;
                        ++p;
                    }
                } else {
                    for (int x = 0; x < w; x++) {
                        if (!(asrc[x >> 3] & (1 << (7 -(x & 7)))))
                            *p = trans;
                        ++p;
                    }
                }
            }
        } else {
            image.setNumColors(ncols);        // create color table
        }
        QVector<QColor> colors = QColormap::instance(data->xinfo.screen()).colormap();
        int j = 0;
        for (int i=0; i<colors.size(); i++) {                // translate pixels
            if (use[i])
                image.setColor(j++, 0xff000000 | colors.at(i).rgb());
        }
    }

    qSafeXDestroyImage(xi);

    return image;
}


/*!
    \fn QPixmap QPixmap::fromImage(const QImage &image, Qt::ImageConversionFlags flags)

    Converts the given \a image to a pixmap using the specified \a
    flags to control the conversion.  The \a flags argument is a
    bitwise-OR of the \l{Qt::ImageConversionFlags}. Passing 0 for \a
    flags sets all the default options.

    In case of monochrome and 8-bit images, the image is first
    converted to a 32-bit pixmap and then filled with the colors in
    the color table. If this is too expensive an operation, you can
    use QBitmap::fromImage() instead.

    \sa toImage(), {QPixmap#Pixmap Conversion}{Pixmap Conversion}
*/

QPixmap QPixmap::fromImage(const QImage &img, Qt::ImageConversionFlags flags)
{
    QPixmap pixmap;
    if (img.isNull()) {
        qWarning("QPixmap::fromImage: Cannot convert a null image");
        return pixmap;
    }

    QImage  image = img;
    const int         w   = image.width();
    const int         h   = image.height();
    int         d   = image.depth();
    const int         dd  = X11->use_xrender && img.hasAlphaChannel() ? 32 : pixmap.data->xinfo.depth();
    bool force_mono = (dd == 1 || (flags & Qt::ColorMode_Mask) == Qt::MonoOnly);

    if (uint(w) >= 32768 || uint(h) >= 32768)
        return QPixmap();

    // must be monochrome
    if (force_mono) {
        if (d != 1) {
            // dither
            image = image.convertToFormat(QImage::Format_MonoLSB, flags);
            d = 1;
        }
    } else {                                        // can be both
        bool conv8 = false;
        if (d > 8 && dd <= 8) {                // convert to 8 bit
            if ((flags & Qt::DitherMode_Mask) == Qt::AutoDither)
                flags = (flags & ~Qt::DitherMode_Mask)
                        | Qt::PreferDither;
            conv8 = true;
        } else if ((flags & Qt::ColorMode_Mask) == Qt::ColorOnly) {
            conv8 = (d == 1);                        // native depth wanted
        } else if (d == 1) {
            if (image.numColors() == 2) {
                QRgb c0 = image.color(0);        // Auto: convert to best
                QRgb c1 = image.color(1);
                conv8 = qMin(c0,c1) != qRgb(0,0,0) || qMax(c0,c1) != qRgb(255,255,255);
            } else {
                // eg. 1-color monochrome images (they do exist).
                conv8 = true;
            }
        }
        if (conv8) {
            image = image.convertToFormat(QImage::Format_Indexed8, flags);
            d = 8;
        }
    }

    if (d == 1 || d == 16) {
        QImage im = image.convertToFormat(QImage::Format_RGB32, flags);
        return fromImage(im);
    }

    Display *dpy   = X11->display;
    Visual *visual = (Visual *) pixmap.data->xinfo.visual();
    XImage *xi           = 0;
    bool    trucol = (visual->c_class >= TrueColor);
    int     nbytes = image.numBytes();
    uchar  *newbits= 0;

#ifndef QT_NO_XRENDER
    if (X11->use_xrender && image.hasAlphaChannel()) {
        const QImage &cimage = image;

        pixmap.data->w = w;
        pixmap.data->h = h;
        pixmap.data->d = 32;

        pixmap.data->hd =
            (Qt::HANDLE)XCreatePixmap(dpy, RootWindow(dpy, pixmap.data->xinfo.screen()),
                                      w, h, pixmap.data->d);

        pixmap.data->picture = XRenderCreatePicture(X11->display, pixmap.data->hd,
                                                    XRenderFindStandardFormat(X11->display, PictStandardARGB32), 0, 0);

        xi = XCreateImage(dpy, visual, pixmap.data->d, ZPixmap, 0, 0, w, h, 32, 0);
        Q_CHECK_PTR(xi);
        newbits = (uchar *)malloc(xi->bytes_per_line*h);
        Q_CHECK_PTR(newbits);
        xi->data = (char *)newbits;

        switch(cimage.format()) {
        case QImage::Format_Indexed8: {
            QVector<QRgb> colorTable = cimage.colorTable();
            uint *xidata = (uint *)xi->data;
            for (int y = 0; y < h; ++y) {
                const uchar *p = cimage.scanLine(y);
                for (int x = 0; x < w; ++x) {
                    const QRgb rgb = colorTable[p[x]];
                    const int a = qAlpha(rgb);
                    if (a == 0xff)
                        *xidata = rgb;
                    else
                        // RENDER expects premultiplied alpha
                        *xidata = qRgba(qt_div_255(qRed(rgb) * a),
                                        qt_div_255(qGreen(rgb) * a),
                                        qt_div_255(qBlue(rgb) * a),
                                        a);
                    ++xidata;
                }
            }
        }
            break;
        case QImage::Format_RGB32: {
            uint *xidata = (uint *)xi->data;
            for (int y = 0; y < h; ++y) {
                const QRgb *p = (const QRgb *) cimage.scanLine(y);
                for (int x = 0; x < w; ++x)
                    *xidata++ = p[x] | 0xff000000;
            }
        }
            break;
        case QImage::Format_ARGB32: {
            uint *xidata = (uint *)xi->data;
            for (int y = 0; y < h; ++y) {
                const QRgb *p = (const QRgb *) cimage.scanLine(y);
                for (int x = 0; x < w; ++x) {
                    const QRgb rgb = p[x];
                    const int a = qAlpha(rgb);
                    if (a == 0xff)
                        *xidata = rgb;
                    else
                        // RENDER expects premultiplied alpha
                        *xidata = qRgba(qt_div_255(qRed(rgb) * a),
                                        qt_div_255(qGreen(rgb) * a),
                                        qt_div_255(qBlue(rgb) * a),
                                        a);
                    ++xidata;
                }
            }

        }
            break;
        case QImage::Format_ARGB32_Premultiplied: {
            uint *xidata = (uint *)xi->data;
            for (int y = 0; y < h; ++y) {
                const QRgb *p = (const QRgb *) cimage.scanLine(y);
                memcpy(xidata, p, w*sizeof(QRgb));
                xidata += w;
            }
        }
            break;
        default:
            Q_ASSERT(false);
        }

        if ((xi->byte_order == MSBFirst) != (QSysInfo::ByteOrder == QSysInfo::BigEndian)) {
            uint *xidata = (uint *)xi->data;
            uint *xiend = xidata + w*h;
            while (xidata < xiend) {
                *xidata = (*xidata >> 24)
                          | ((*xidata >> 8) & 0xff00)
                          | ((*xidata << 8) & 0xff0000)
                          | (*xidata << 24);
                ++xidata;
            }
        }

        GC gc = XCreateGC(dpy, pixmap.data->hd, 0, 0);
        XPutImage(dpy, pixmap.data->hd, gc, xi, 0, 0, 0, 0, w, h);
        XFreeGC(dpy, gc);

        qSafeXDestroyImage(xi);

        return pixmap;
    }
#endif // QT_NO_XRENDER

    if (trucol) {                                // truecolor display
        if (image.format() == QImage::Format_ARGB32_Premultiplied)
            image = image.convertToFormat(QImage::Format_ARGB32);

        const QImage &cimage = image;
        QRgb  pix[256];                                // pixel translation table
        const bool  d8 = (d == 8);
        const uint  red_mask          = (uint)visual->red_mask;
        const uint  green_mask  = (uint)visual->green_mask;
        const uint  blue_mask          = (uint)visual->blue_mask;
        const int   red_shift          = highest_bit(red_mask)   - 7;
        const int   green_shift = highest_bit(green_mask) - 7;
        const int   blue_shift  = highest_bit(blue_mask)  - 7;
        const uint  rbits = highest_bit(red_mask) - lowest_bit(red_mask) + 1;
        const uint  gbits = highest_bit(green_mask) - lowest_bit(green_mask) + 1;
        const uint  bbits = highest_bit(blue_mask) - lowest_bit(blue_mask) + 1;

        if (d8) {                                // setup pixel translation
            QVector<QRgb> ctable = cimage.colorTable();
            for (int i=0; i < cimage.numColors(); i++) {
                int r = qRed  (ctable[i]);
                int g = qGreen(ctable[i]);
                int b = qBlue (ctable[i]);
                r = red_shift        > 0 ? r << red_shift   : r >> -red_shift;
                g = green_shift > 0 ? g << green_shift : g >> -green_shift;
                b = blue_shift        > 0 ? b << blue_shift  : b >> -blue_shift;
                pix[i] = (b & blue_mask) | (g & green_mask) | (r & red_mask)
                         | ~(blue_mask | green_mask | red_mask);
            }
        }

        xi = XCreateImage(dpy, visual, dd, ZPixmap, 0, 0, w, h, 32, 0);
        Q_CHECK_PTR(xi);
        newbits = (uchar *)malloc(xi->bytes_per_line*h);
        Q_CHECK_PTR(newbits);
        if (!newbits)                                // no memory
            return QPixmap();
        int    bppc = xi->bits_per_pixel;

        bool contig_bits = n_bits(red_mask) == rbits &&
                           n_bits(green_mask) == gbits &&
                           n_bits(blue_mask) == bbits;
        bool dither_tc =
            // Want it?
            (flags & Qt::Dither_Mask) != Qt::ThresholdDither &&
            (flags & Qt::DitherMode_Mask) != Qt::AvoidDither &&
            // Need it?
            bppc < 24 && !d8 &&
            // Can do it? (Contiguous bits?)
            contig_bits;

        static bool init=false;
        static int D[16][16];
        if (dither_tc && !init) {
            // I also contributed this code to XV - WWA.
            /*
              The dither matrix, D, is obtained with this formula:

              D2 = [0 2]
              [3 1]


              D2*n = [4*Dn       4*Dn+2*Un]
              [4*Dn+3*Un  4*Dn+1*Un]
            */
            int n,i,j;
            init=1;

            /* Set D2 */
            D[0][0]=0;
            D[1][0]=2;
            D[0][1]=3;
            D[1][1]=1;

            /* Expand using recursive definition given above */
            for (n=2; n<16; n*=2) {
                for (i=0; i<n; i++) {
                    for (j=0; j<n; j++) {
                        D[i][j]*=4;
                        D[i+n][j]=D[i][j]+2;
                        D[i][j+n]=D[i][j]+3;
                        D[i+n][j+n]=D[i][j]+1;
                    }
                }
            }
            init=true;
        }

        enum { BPP8,
               BPP16_565, BPP16_555,
               BPP16_MSB, BPP16_LSB,
               BPP24_888,
               BPP24_MSB, BPP24_LSB,
               BPP32_8888,
               BPP32_MSB, BPP32_LSB
        } mode = BPP8;

        bool same_msb_lsb = (xi->byte_order == MSBFirst) == (QSysInfo::ByteOrder == QSysInfo::BigEndian);

        if(bppc == 8) // 8 bit
            mode = BPP8;
        else if(bppc == 16) { // 16 bit MSB/LSB
            if(red_shift == 8 && green_shift == 3 && blue_shift == -3 && !d8 && same_msb_lsb)
                mode = BPP16_565;
            else if(red_shift == 7 && green_shift == 2 && blue_shift == -3 && !d8 && same_msb_lsb)
                mode = BPP16_555;
            else
                mode = (xi->byte_order == LSBFirst) ? BPP16_LSB : BPP16_MSB;
        } else if(bppc == 24) { // 24 bit MSB/LSB
            if (red_shift == 16 && green_shift == 8 && blue_shift == 0 && !d8 && same_msb_lsb)
                mode = BPP24_888;
            else
                mode = (xi->byte_order == LSBFirst) ? BPP24_LSB : BPP24_MSB;
        } else if(bppc == 32) { // 32 bit MSB/LSB
            if(red_shift == 16 && green_shift == 8 && blue_shift == 0 && !d8 && same_msb_lsb)
                mode = BPP32_8888;
            else
                mode = (xi->byte_order == LSBFirst) ? BPP32_LSB : BPP32_MSB;
        } else
            qFatal("Logic error 3");

#define GET_PIXEL                                                       \
        uint pixel;                                                     \
        if (d8) pixel = pix[*src++];                                    \
        else {                                                          \
            int r = qRed  (*p);                                         \
            int g = qGreen(*p);                                         \
            int b = qBlue (*p++);                                       \
            r = red_shift   > 0                                         \
                ? r << red_shift   : r >> -red_shift;                   \
            g = green_shift > 0                                         \
                ? g << green_shift : g >> -green_shift;                 \
            b = blue_shift  > 0                                         \
                ? b << blue_shift  : b >> -blue_shift;                  \
            pixel = (r & red_mask)|(g & green_mask) | (b & blue_mask)   \
                    | ~(blue_mask | green_mask | red_mask);             \
        }

#define GET_PIXEL_DITHER_TC                                             \
        int r = qRed  (*p);                                             \
        int g = qGreen(*p);                                             \
        int b = qBlue (*p++);                                           \
        const int thres = D[x%16][y%16];                                \
        if (r <= (255-(1<<(8-rbits))) && ((r<<rbits) & 255)             \
            > thres)                                                    \
            r += (1<<(8-rbits));                                        \
        if (g <= (255-(1<<(8-gbits))) && ((g<<gbits) & 255)             \
            > thres)                                                    \
            g += (1<<(8-gbits));                                        \
        if (b <= (255-(1<<(8-bbits))) && ((b<<bbits) & 255)             \
            > thres)                                                    \
            b += (1<<(8-bbits));                                        \
        r = red_shift   > 0                                             \
            ? r << red_shift   : r >> -red_shift;                       \
        g = green_shift > 0                                             \
            ? g << green_shift : g >> -green_shift;                     \
        b = blue_shift  > 0                                             \
            ? b << blue_shift  : b >> -blue_shift;                      \
        uint pixel = (r & red_mask)|(g & green_mask) | (b & blue_mask);

// again, optimized case
// can't be optimized that much :(
#define GET_PIXEL_DITHER_TC_OPT(red_shift,green_shift,blue_shift,red_mask,green_mask,blue_mask, \
                                rbits,gbits,bbits)                      \
        const int thres = D[x%16][y%16];                                \
        int r = qRed  (*p);                                             \
        if (r <= (255-(1<<(8-rbits))) && ((r<<rbits) & 255)             \
            > thres)                                                    \
            r += (1<<(8-rbits));                                        \
        int g = qGreen(*p);                                             \
        if (g <= (255-(1<<(8-gbits))) && ((g<<gbits) & 255)             \
            > thres)                                                    \
            g += (1<<(8-gbits));                                        \
        int b = qBlue (*p++);                                           \
        if (b <= (255-(1<<(8-bbits))) && ((b<<bbits) & 255)             \
            > thres)                                                    \
            b += (1<<(8-bbits));                                        \
        uint pixel = ((r red_shift) & red_mask)                         \
                     | ((g green_shift) & green_mask)                   \
                     | ((b blue_shift) & blue_mask);

#define CYCLE(body)                                             \
        for (int y=0; y<h; y++) {                               \
            const uchar* src = cimage.scanLine(y);              \
            uchar* dst = newbits + xi->bytes_per_line*y;        \
            const QRgb* p = (const QRgb *)src;                  \
            body                                                \
                }

        if (dither_tc) {
            switch (mode) {
            case BPP16_565:
                CYCLE(
                    quint16* dst16 = (quint16*)dst;
                    for (int x=0; x<w; x++) {
                        GET_PIXEL_DITHER_TC_OPT(<<8,<<3,>>3,0xf800,0x7e0,0x1f,5,6,5)
                            *dst16++ = pixel;
                    }
                    )
                    break;
            case BPP16_555:
                CYCLE(
                    quint16* dst16 = (quint16*)dst;
                    for (int x=0; x<w; x++) {
                        GET_PIXEL_DITHER_TC_OPT(<<7,<<2,>>3,0x7c00,0x3e0,0x1f,5,5,5)
                            *dst16++ = pixel;
                    }
                    )
                    break;
            case BPP16_MSB:                        // 16 bit MSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL_DITHER_TC
                            *dst++ = (pixel >> 8);
                        *dst++ = pixel;
                    }
                    )
                    break;
            case BPP16_LSB:                        // 16 bit LSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL_DITHER_TC
                            *dst++ = pixel;
                        *dst++ = pixel >> 8;
                    }
                    )
                    break;
            default:
                qFatal("Logic error");
            }
        } else {
            switch (mode) {
            case BPP8:                        // 8 bit
                CYCLE(
                    Q_UNUSED(p);
                    for (int x=0; x<w; x++)
                        *dst++ = pix[*src++];
                    )
                    break;
            case BPP16_565:
                CYCLE(
                    quint16* dst16 = (quint16*)dst;
                    for (int x = 0; x < w; x++) {
                        *dst16++ = ((*p >> 8) & 0xf800)
                                   | ((*p >> 5) & 0x7e0)
                                   | ((*p >> 3) & 0x1f);
                        ++p;
                    }
                    )
                    break;
            case BPP16_555:
                CYCLE(
                    quint16* dst16 = (quint16*)dst;
                    for (int x=0; x<w; x++) {
                        *dst16++ = ((*p >> 9) & 0x7c00)
                                   | ((*p >> 6) & 0x3e0)
                                   | ((*p >> 3) & 0x1f);
                        ++p;
                    }
                    )
                    break;
            case BPP16_MSB:                        // 16 bit MSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = (pixel >> 8);
                        *dst++ = pixel;
                    }
                    )
                    break;
            case BPP16_LSB:                        // 16 bit LSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel;
                        *dst++ = pixel >> 8;
                    }
                    )
                    break;
            case BPP24_888:                        // 24 bit MSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        *dst++ = qRed  (*p);
                        *dst++ = qGreen(*p);
                        *dst++ = qBlue (*p++);
                    }
                    )
                    break;
            case BPP24_MSB:                        // 24 bit MSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel >> 16;
                        *dst++ = pixel >> 8;
                        *dst++ = pixel;
                    }
                    )
                    break;
            case BPP24_LSB:                        // 24 bit LSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel;
                        *dst++ = pixel >> 8;
                        *dst++ = pixel >> 16;
                    }
                    )
                    break;
            case BPP32_8888:
                CYCLE(
                    memcpy(dst, p, w * 4);
                    )
                    break;
            case BPP32_MSB:                        // 32 bit MSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel >> 24;
                        *dst++ = pixel >> 16;
                        *dst++ = pixel >> 8;
                        *dst++ = pixel;
                    }
                    )
                    break;
            case BPP32_LSB:                        // 32 bit LSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel;
                        *dst++ = pixel >> 8;
                        *dst++ = pixel >> 16;
                        *dst++ = pixel >> 24;
                    }
                    )
                    break;
            default:
                qFatal("Logic error 2");
            }
        }
        xi->data = (char *)newbits;
    }

    if (d == 8 && !trucol) {                        // 8 bit pixmap
        int  pop[256];                                // pixel popularity

        if (image.numColors() == 0)
            image.setNumColors(1);

        const QImage &cimage = image;
        memset(pop, 0, sizeof(int)*256);        // reset popularity array
        for (int i = 0; i < h; i++) {                        // for each scanline...
            const uchar* p = cimage.scanLine(i);
            const uchar *end = p + w;
            while (p < end)                        // compute popularity
                pop[*p++]++;
        }

        newbits = (uchar *)malloc(nbytes);        // copy image into newbits
        Q_CHECK_PTR(newbits);
        if (!newbits)                                // no memory
            return QPixmap();
        uchar* p = newbits;
        memcpy(p, cimage.bits(), nbytes);        // copy image data into newbits

        /*
         * The code below picks the most important colors. It is based on the
         * diversity algorithm, implemented in XV 3.10. XV is (C) by John Bradley.
         */

        struct PIX {                                // pixel sort element
            uchar r,g,b,n;                        // color + pad
            int          use;                                // popularity
            int          index;                        // index in colormap
            int          mindist;
        };
        int ncols = 0;
        for (int i=0; i< cimage.numColors(); i++) { // compute number of colors
            if (pop[i] > 0)
                ncols++;
        }
        for (int i = cimage.numColors(); i < 256; i++) // ignore out-of-range pixels
            pop[i] = 0;

        // works since we make sure above to have at least
        // one color in the image
        if (ncols == 0)
            ncols = 1;

        PIX pixarr[256];                        // pixel array
        PIX pixarr_sorted[256];                        // pixel array (sorted)
        memset(pixarr, 0, ncols*sizeof(PIX));
        PIX *px                   = &pixarr[0];
        int  maxpop = 0;
        int  maxpix = 0;
        uint j = 0;
        QVector<QRgb> ctable = cimage.colorTable();
        for (int i = 0; i < 256; i++) {                // init pixel array
            if (pop[i] > 0) {
                px->r = qRed  (ctable[i]);
                px->g = qGreen(ctable[i]);
                px->b = qBlue (ctable[i]);
                px->n = 0;
                px->use = pop[i];
                if (pop[i] > maxpop) {        // select most popular entry
                    maxpop = pop[i];
                    maxpix = j;
                }
                px->index = i;
                px->mindist = 1000000;
                px++;
                j++;
            }
        }
        pixarr_sorted[0] = pixarr[maxpix];
        pixarr[maxpix].use = 0;

        for (int i = 1; i < ncols; i++) {                // sort pixels
            int minpix = -1, mindist = -1;
            px = &pixarr_sorted[i-1];
            int r = px->r;
            int g = px->g;
            int b = px->b;
            int dist;
            if ((i & 1) || i<10) {                // sort on max distance
                for (int j=0; j<ncols; j++) {
                    px = &pixarr[j];
                    if (px->use) {
                        dist = (px->r - r)*(px->r - r) +
                               (px->g - g)*(px->g - g) +
                               (px->b - b)*(px->b - b);
                        if (px->mindist > dist)
                            px->mindist = dist;
                        if (px->mindist > mindist) {
                            mindist = px->mindist;
                            minpix = j;
                        }
                    }
                }
            } else {                                // sort on max popularity
                for (int j=0; j<ncols; j++) {
                    px = &pixarr[j];
                    if (px->use) {
                        dist = (px->r - r)*(px->r - r) +
                               (px->g - g)*(px->g - g) +
                               (px->b - b)*(px->b - b);
                        if (px->mindist > dist)
                            px->mindist = dist;
                        if (px->use > mindist) {
                            mindist = px->use;
                            minpix = j;
                        }
                    }
                }
            }
            pixarr_sorted[i] = pixarr[minpix];
            pixarr[minpix].use = 0;
        }

        QColormap cmap = QColormap::instance(pixmap.data->xinfo.screen());
        uint pix[256];                                // pixel translation table
        px = &pixarr_sorted[0];
        for (int i = 0; i < ncols; i++) {                // allocate colors
            QColor c(px->r, px->g, px->b);
            pix[px->index] = cmap.pixel(c);
            px++;
        }

        p = newbits;
        for (int i = 0; i < nbytes; i++) {                // translate pixels
            *p = pix[*p];
            p++;
        }
    }

    if (!xi) {                                // X image not created
        xi = XCreateImage(dpy, visual, dd, ZPixmap, 0, 0, w, h, 32, 0);
        if (xi->bits_per_pixel == 16) {        // convert 8 bpp ==> 16 bpp
            ushort *p2;
            int            p2inc = xi->bytes_per_line/sizeof(ushort);
            ushort *newerbits = (ushort *)malloc(xi->bytes_per_line * h);
            Q_CHECK_PTR(newerbits);
            if (!newerbits)                                // no memory
                return QPixmap();
            uchar* p = newbits;
            for (int y = 0; y < h; y++) {                // OOPS: Do right byte order!!
                p2 = newerbits + p2inc*y;
                for (int x = 0; x < w; x++)
                    *p2++ = *p++;
            }
            free(newbits);
            newbits = (uchar *)newerbits;
        } else if (xi->bits_per_pixel != 8) {
            qWarning("QPixmap::fromImage: Display not supported "
                     "(bpp=%d)", xi->bits_per_pixel);
        }
        xi->data = (char *)newbits;
    }

    pixmap.data->hd = (Qt::HANDLE)XCreatePixmap(X11->display,
                                                RootWindow(X11->display, pixmap.data->xinfo.screen()),
                                                w, h, dd);

    GC gc = XCreateGC(dpy, pixmap.data->hd, 0, 0);
    XPutImage(dpy, pixmap.data->hd, gc, xi, 0, 0, 0, 0, w, h);
    XFreeGC(dpy, gc);

    qSafeXDestroyImage(xi);
    pixmap.data->w = w;
    pixmap.data->h = h;
    pixmap.data->d = dd;

#ifndef QT_NO_XRENDER
    if (X11->use_xrender) {
        XRenderPictFormat *format = pixmap.data->d == 1
                                    ? XRenderFindStandardFormat(X11->display, PictStandardA1)
                                    : XRenderFindVisualFormat(X11->display, (Visual *) pixmap.data->xinfo.visual());
        pixmap.data->picture = XRenderCreatePicture(X11->display, pixmap.data->hd, format, 0, 0);
    }
#endif

    if (image.hasAlphaChannel()) {
        QBitmap m = QBitmap::fromImage(image.createAlphaMask(flags));
        pixmap.setMask(m);
    }

    return pixmap;
}


/*!
    \fn QPixmap QPixmap::grabWindow(WId window, int x, int y, int
    width, int height)

    Creates and returns a pixmap constructed by grabbing the contents
    of the given \a window restricted by QRect(\a x, \a y, \a width,
    \a height).

    The arguments (\a{x}, \a{y}) specify the offset in the window,
    whereas (\a{width}, \a{height}) specify the area to be copied.  If
    \a width is negative, the function copies everything to the right
    border of the window. If \a height is negative, the function
    copies everything to the bottom of the window.

    The window system identifier (\c WId) can be retrieved using the
    QWidget::winId() function. The rationale for using a window
    identifier and not a QWidget, is to enable grabbing of windows
    that are not part of the application, window system frames, and so
    on.

    The grabWindow() function grabs pixels from the screen, not from
    the window, i.e. if there is another window partially or entirely
    over the one you grab, you get pixels from the overlying window,
    too. The mouse cursor is generally not grabbed.

    Note on X11that if the given \a window doesn't have the same depth
    as the root window, and another window partially or entirely
    obscures the one you grab, you will \e not get pixels from the
    overlying window.  The contents of the obscured areas in the
    pixmap will be undefined and uninitialized.

    \warning In general, grabbing an area outside the screen is not
    safe. This depends on the underlying window system.

    \sa grabWidget(), {Screenshot Example}
*/

QPixmap QPixmap::grabWindow(WId window, int x, int y, int w, int h)
{
    if (w == 0 || h == 0)
        return QPixmap();

    Display *dpy = X11->display;
    XWindowAttributes window_attr;
    if (! XGetWindowAttributes(dpy, window, &window_attr))
        return QPixmap();

    if (w < 0)
        w = window_attr.width - x;
    if (h < 0)
        h = window_attr.height - y;

    // determine the screen
    int scr;
    for (scr = 0; scr < ScreenCount(dpy); ++scr) {
        if (window_attr.root == RootWindow(dpy, scr))        // found it
            break;
    }
    if (scr >= ScreenCount(dpy))                // sanity check
        return QPixmap();


    // get the depth of the root window
    XWindowAttributes root_attr;
    if (! XGetWindowAttributes(dpy, window_attr.root, &root_attr))
        return QPixmap();

    if (window_attr.depth == root_attr.depth) {
        // if the depth of the specified window and the root window are the
        // same, grab pixels from the root window (so that we get the any
        // overlapping windows and window manager frames)

        // map x and y to the root window
        WId unused;
        if (! XTranslateCoordinates(dpy, window, window_attr.root, x, y,
                                    &x, &y, &unused))
            return QPixmap();

        window = window_attr.root;
        window_attr = root_attr;
    }

    QPixmap pm(w, h);
    pm.data->uninit = false;
    pm.x11SetScreen(scr);

    GC gc = XCreateGC(dpy, pm.handle(), 0, 0);
    XSetSubwindowMode(dpy, gc, IncludeInferiors);
    XCopyArea(dpy, window, pm.handle(), gc, x, y, w, h, 0, 0);
    XFreeGC(dpy, gc);

    return pm;
}

/*!
    Returns a copy of the pixmap that is transformed using the given
    transformation \a matrix and transformation \a mode. The original
    pixmap is not changed.

    The transformation \a matrix is internally adjusted to compensate
    for unwanted translation; i.e. the pixmap produced is the smallest
    pixmap that contains all the transformed points of the original
    pixmap. Use the trueMatrix() function to retrieve the actual
    matrix used for transforming the pixmap.

    This function is slow because it involves transformation to a
    QImage, non-trivial computations and a transformation back to a
    QPixmap.

    \sa trueMatrix(), {QPixmap#Pixmap Transformations}{Pixmap
    Transformations}
*/
QPixmap QPixmap::transformed(const QTransform &matrix, Qt::TransformationMode mode) const
{
    uint   w = 0;
    uint   h = 0;                               // size of target pixmap
    uint   ws, hs;                              // size of source pixmap
    uchar *dptr;                                // data in target pixmap
    uint   dbpl, dbytes;                        // bytes per line/bytes total
    uchar *sptr;                                // data in original pixmap
    int    sbpl;                                // bytes per line in original
    int    bpp;                                 // bits per pixel
    bool   depth1 = depth() == 1;
    Display *dpy = X11->display;

    if (isNull())
        return copy();

    ws = width();
    hs = height();

    QTransform mat(matrix.m11(), matrix.m12(), matrix.m13(),
                   matrix.m21(), matrix.m22(), matrix.m23(),
                   0., 0., 1);
    bool complex_xform = false;
    qreal scaledWidth;
    qreal scaledHeight;

    if (mat.m12() == 0.0F && mat.m21() == 0.0F) {
        if (mat.m11() == 1.0F && mat.m22() == 1.0F) // identity matrix
            return *this;
        scaledHeight = qAbs(mat.m22()) * hs + 0.9999;
        scaledWidth = qAbs(mat.m11()) * ws + 0.9999;
        h = qAbs(int(scaledHeight));
        w = qAbs(int(scaledWidth));
    } else {                                        // rotation or shearing
        QPolygonF a(QRectF(0, 0, ws+1, hs+1));
        a = mat.map(a);
        QRectF r = a.boundingRect().normalized();
        w = int(r.width() + 0.9999);
        h = int(r.height() + 0.9999);
        scaledWidth = w;
        scaledHeight = h;
        complex_xform = true;
    }
    mat = trueMatrix(mat, ws, hs); // true matrix


    bool invertible;
    mat = mat.inverted(&invertible);  // invert matrix

    if (h == 0 || w == 0 || !invertible
        || qAbs(scaledWidth) >= 32768 || qAbs(scaledHeight) >= 32768 )
	// error, return null pixmap
        return QPixmap();

    if (mode == Qt::SmoothTransformation) {
        QImage image = toImage();
        return QPixmap::fromImage(image.transformed(matrix, mode));
    }

#if defined(QT_MITSHM)
    static bool try_once = true;
    if (try_once) {
        try_once = false;
        if (!xshminit)
            qt_create_mitshm_buffer(this, 800, 600);
    }

    bool use_mitshm = xshmimg && !depth1 &&
                      xshmimg->width >= w && xshmimg->height >= h;
#endif
    XImage *xi = XGetImage(X11->display, handle(), 0, 0, ws, hs, AllPlanes,
                           depth1 ? XYPixmap : ZPixmap);

    if (!xi)
        return QPixmap();

    sbpl = xi->bytes_per_line;
    sptr = (uchar *)xi->data;
    bpp         = xi->bits_per_pixel;

    if (depth1)
        dbpl = (w+7)/8;
    else
        dbpl = ((w*bpp+31)/32)*4;
    dbytes = dbpl*h;

#if defined(QT_MITSHM)
    if (use_mitshm) {
        dptr = (uchar *)xshmimg->data;
        uchar fillbyte = bpp == 8 ? white.pixel() : 0xff;
        for (int y=0; y<h; y++)
            memset(dptr + y*xshmimg->bytes_per_line, fillbyte, dbpl);
    } else {
#endif
        dptr = (uchar *)malloc(dbytes);        // create buffer for bits
        Q_CHECK_PTR(dptr);
        if (depth1)                                // fill with zeros
            memset(dptr, 0, dbytes);
        else if (bpp == 8)                        // fill with background color
            memset(dptr, WhitePixel(X11->display, data->xinfo.screen()), dbytes);
        else
            memset(dptr, 0, dbytes);
#if defined(QT_MITSHM)
    }
#endif

    // #define QT_DEBUG_XIMAGE
#if defined(QT_DEBUG_XIMAGE)
    qDebug("----IMAGE--INFO--------------");
    qDebug("width............. %d", xi->width);
    qDebug("height............ %d", xi->height);
    qDebug("xoffset........... %d", xi->xoffset);
    qDebug("format............ %d", xi->format);
    qDebug("byte order........ %d", xi->byte_order);
    qDebug("bitmap unit....... %d", xi->bitmap_unit);
    qDebug("bitmap bit order.. %d", xi->bitmap_bit_order);
    qDebug("depth............. %d", xi->depth);
    qDebug("bytes per line.... %d", xi->bytes_per_line);
    qDebug("bits per pixel.... %d", xi->bits_per_pixel);
#endif

    int type;
    if (xi->bitmap_bit_order == MSBFirst)
        type = QT_XFORM_TYPE_MSBFIRST;
    else
        type = QT_XFORM_TYPE_LSBFIRST;
    int        xbpl, p_inc;
    if (depth1) {
        xbpl  = (w+7)/8;
        p_inc = dbpl - xbpl;
    } else {
        xbpl  = (w*bpp)/8;
        p_inc = dbpl - xbpl;
#if defined(QT_MITSHM)
        if (use_mitshm)
            p_inc = xshmimg->bytes_per_line - xbpl;
#endif
    }

    if (!qt_xForm_helper(mat, xi->xoffset, type, bpp, dptr, xbpl, p_inc, h, sptr, sbpl, ws, hs)){
        qWarning("QPixmap::transform: display not supported (bpp=%d)",bpp);
        QPixmap pm;
        return pm;
    }

    qSafeXDestroyImage(xi);

    if (depth1) {                                // mono bitmap
        QBitmap bm = QBitmap::fromData(QSize(w, h), dptr,
                                       BitmapBitOrder(X11->display) == MSBFirst
                                       ? QImage::Format_Mono
                                       : QImage::Format_MonoLSB);
        free(dptr);
        return bm;
    } else {                                        // color pixmap
        QPixmap pm;
        pm.data->uninit = false;
        pm.data->xinfo = data->xinfo;
        pm.data->d = data->d;
        pm.data->w = w;
        pm.data->h = h;
        pm.data->hd = (Qt::HANDLE)XCreatePixmap(X11->display, RootWindow(X11->display, data->xinfo.screen()),
                                                w, h, data->d);
#ifndef QT_NO_XRENDER
        if (X11->use_xrender) {
            XRenderPictFormat *format = pm.data->d == 32
                                        ? XRenderFindStandardFormat(X11->display, PictStandardARGB32)
                                        : XRenderFindVisualFormat(X11->display, (Visual *) pm.data->xinfo.visual());
            pm.data->picture = XRenderCreatePicture(X11->display, pm.data->hd, format, 0, 0);
        }
#endif // QT_NO_XRENDER

        GC gc = XCreateGC(X11->display, pm.data->hd, 0, 0);
#if defined(QT_MITSHM)
        if (use_mitshm) {
            XCopyArea(dpy, xshmpm, pm.data->hd, gc, 0, 0, w, h, 0, 0);
        } else
#endif
        {
            xi = XCreateImage(dpy, (Visual *) pm.data->xinfo.visual(), pm.data->d,
                              ZPixmap, 0, (char *)dptr, w, h, 32, 0);
            XPutImage(dpy, pm.handle(), gc, xi, 0, 0, 0, 0, w, h);
            qSafeXDestroyImage(xi);
        }
        XFreeGC(X11->display, gc);

        if (data->x11_mask) { // xform mask, too
            pm.setMask(data->mask_to_bitmap(data->xinfo.screen()).transformed(matrix));
        } else if (data->d != 32 && complex_xform) { // need a mask!
            QBitmap mask(data->w, data->h);
            mask.fill(Qt::color1);
            pm.setMask(mask.transformed(matrix));
        }
        return pm;
    }
}

/*!
  \overload

  This convenience function loads the \a matrix into a
  QTransform and calls the overloaded function.
 */
QPixmap QPixmap::transformed(const QMatrix &matrix, Qt::TransformationMode mode) const
{
    return transformed(QTransform(matrix), mode);
}


/*!
  \internal
*/
int QPixmap::x11SetDefaultScreen(int screen)
{
    int old = defaultScreen;
    defaultScreen = screen;
    return old;
}

/*!
  \internal
*/
void QPixmap::x11SetScreen(int screen)
{
    if (paintingActive()) {
        qWarning("QPixmap::x11SetScreen(): Cannot change screens during painting");
        return;
    }

    if (screen < 0)
        screen = QX11Info::appScreen();

    if (screen == data->xinfo.screen())
        return; // nothing to do

    if (isNull()) {
        QX11InfoData* xd = data->xinfo.getX11Data(true);
        xd->screen = screen;
        xd->depth = QX11Info::appDepth(screen);
        xd->cells = QX11Info::appCells(screen);
        xd->colormap = QX11Info::appColormap(screen);
        xd->defaultColormap = QX11Info::appDefaultColormap(screen);
        xd->visual = (Visual *)QX11Info::appVisual(screen);
        xd->defaultVisual = QX11Info::appDefaultVisual(screen);
        data->xinfo.setX11Data(xd);
        return;
    }
#if 0
    qDebug("QPixmap::x11SetScreen for %p from %d to %d. Size is %d/%d", data, data->xinfo.screen(), screen, width(), height());
#endif

    QImage img = toImage();
    x11SetDefaultScreen(screen);
    if (img.depth() == 1)
        (*this) = QBitmap::fromImage(img);
    else
        (*this) = fromImage(img);
}

/*!
    Returns true if this pixmap has an alpha channel, \e or has a
    mask, otherwise returns false.

    \sa hasAlphaChannel(), alphaChannel(), mask()
*/
bool QPixmap::hasAlpha() const
{
    return data->d == 32 || data->x11_mask;
}

/*!
    Returns true if the pixmap has a format that respects the alpha
    channel, otherwise returns false.

    \sa alphaChannel(), hasAlpha()
*/
bool QPixmap::hasAlphaChannel() const
{
    return data->d == 32;
}

/*!
    Returns information about the configuration of the X display used to display
    the widget.

    \warning This function is only available on X11.

    \sa {QPixmap#Pixmap Information}{Pixmap Information}
*/
const QX11Info &QPixmap::x11Info() const
{
    return data->xinfo;
}

QPaintEngine *QPixmap::paintEngine() const
{
    if (!data->paintEngine)
        data->paintEngine = new QX11PaintEngine();
    return data->paintEngine;
}

/*!
    Returns the X11 Picture handle of the pixmap for XRender
    support.

    This function will return 0 if XRender support is not compiled
    into Qt, if the XRender extension is not supported on the X11
    display, or if the handle could not be created. Use of this
    function is not portable.

    \warning This function is only available on X11.

    \sa {QPixmap#Pixmap Information}{Pixmap Information}
*/

Qt::HANDLE QPixmap::x11PictureHandle() const
{
#ifndef QT_NO_XRENDER
    return data->picture;
#else
    return 0;
#endif // QT_NO_XRENDER
}

Qt::HANDLE QPixmapData::x11ConvertToDefaultDepth()
{
#ifndef QT_NO_XRENDER
    if (d == xinfo.depth() || !X11->use_xrender)
        return hd;
    if (!hd2) {
        hd2 = XCreatePixmap(xinfo.display(), hd, w, h, xinfo.depth());
        XRenderPictFormat *format = XRenderFindVisualFormat(xinfo.display(),
                                                            (Visual*) xinfo.visual());
        Picture pic = XRenderCreatePicture(xinfo.display(), hd2, format, 0, 0);
        XRenderComposite(xinfo.display(), PictOpSrc, picture,
                         XNone, pic, 0, 0, 0, 0, 0, 0, w, h);
        XRenderFreePicture(xinfo.display(), pic);
    }
    return hd2;
#else
    return hd;
#endif
}

QPixmap QPixmap::copy(const QRect &rect) const
{
    if (isNull())
        return QPixmap();

    if (data->type == BitmapType)
        return QBitmap::fromImage(toImage().copy(rect));

    QPixmap pm;
    QSize s = rect.isNull() ? size() : rect.size();

    pm.data->uninit = false;
    pm.data->xinfo = data->xinfo;
    pm.data->d = data->d;
    pm.data->w = s.width();
    pm.data->h = s.height();
    pm.data->hd = (Qt::HANDLE)XCreatePixmap(X11->display, RootWindow(X11->display, data->xinfo.screen()),
                                            s.width(), s.height(), data->d);
#ifndef QT_NO_XRENDER
    if (X11->use_xrender) {
        XRenderPictFormat *format = pm.data->d == 32
                                    ? XRenderFindStandardFormat(X11->display, PictStandardARGB32)
                                    : XRenderFindVisualFormat(X11->display, (Visual *) pm.data->xinfo.visual());
        pm.data->picture = XRenderCreatePicture(X11->display, pm.data->hd, format, 0, 0);
    }
#endif // QT_NO_XRENDER
    if (data->x11_mask) {
        pm.data->x11_mask = XCreatePixmap(X11->display, pm.data->hd, pm.data->w, pm.data->h, 1);
#ifndef QT_NO_XRENDER
        if (X11->use_xrender) {
            pm.data->mask_picture = XRenderCreatePicture(X11->display, pm.data->x11_mask,
                                                         XRenderFindStandardFormat(X11->display, PictStandardA1), 0, 0);
            XRenderPictureAttributes attrs;
            attrs.alpha_map = data->mask_picture;
            XRenderChangePicture(X11->display, data->picture, CPAlphaMap, &attrs);
        }
#endif
    }

#if !defined(QT_NO_XRENDER)
    if (data->picture && data->d == 32) {
        XRenderComposite(X11->display, PictOpSrc,
                         data->picture, 0, pm.data->picture,
                         rect.x(), rect.y(), 0, 0, 0, 0,
                         pm.data->w, pm.data->h);
    } else
#endif
    {
        GC gc = XCreateGC(X11->display, pm.data->hd, 0, 0);
        XCopyArea(X11->display, data->hd, pm.data->hd, gc,
                  rect.x(), rect.y(), pm.width(), pm.height(),
                  0, 0);
        if (data->x11_mask) {
            GC monogc = XCreateGC(X11->display, pm.data->x11_mask, 0, 0);
            XCopyArea(X11->display, data->x11_mask, pm.data->x11_mask, monogc,
                      rect.x(), rect.y(), pm.data->w, pm.data->h,
                      0, 0);
            XFreeGC(X11->display, monogc);
        }
        XFreeGC(X11->display, gc);
    }

    return pm;
}


#if !defined(QT_NO_XRENDER)
void QPixmapData::convertToARGB32()
{
    if (!X11->use_xrender)
        return;

    // Q_ASSERT(count == 1);

    Pixmap pm = XCreatePixmap(X11->display, RootWindow(X11->display, xinfo.screen()),
                              w, h, 32);
    Picture p = XRenderCreatePicture(X11->display, pm,
                                     XRenderFindStandardFormat(X11->display, PictStandardARGB32), 0, 0);
    XRenderComposite(X11->display, PictOpSrc, picture, 0, p, 0, 0, 0, 0, 0, 0, w, h);
    XRenderFreePicture(X11->display, picture);
    XFreePixmap(X11->display, hd);
    if (x11_mask) {
        XFreePixmap(X11->display, x11_mask);
        if (mask_picture)
            XRenderFreePicture(X11->display, mask_picture);
        x11_mask = 0;
        mask_picture = 0;
    }
    hd = pm;
    picture = p;
    d = 32;
}
#endif
