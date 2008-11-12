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


#include <qglobal.h>

#include "qpixmap.h"
#include "qpixmap_p.h"

#include "qbitmap.h"
#include "qimage.h"
#include "qwidget.h"
#include "qpainter.h"
#include "qdatastream.h"
#include "qbuffer.h"
#include "qapplication.h"
#include <private/qwidget_p.h>
#include "qevent.h"
#include "qfile.h"
#include "qfileinfo.h"
#include "qpixmapcache.h"
#include "qdatetime.h"
#include "qimagereader.h"
#include "qimagewriter.h"
#include "qpaintengine.h"

#ifdef Q_WS_MAC
# include "private/qt_mac_p.h"
#endif

#if defined(Q_WS_X11)
# include "qx11info_x11.h"
# include <private/qt_x11_p.h>
#endif

// ### Qt 5: remove
typedef void (*_qt_pixmap_cleanup_hook)(int);
Q_GUI_EXPORT _qt_pixmap_cleanup_hook qt_pixmap_cleanup_hook = 0;

// ### Qt 5: rename
typedef void (*_qt_pixmap_cleanup_hook_64)(qint64);
Q_GUI_EXPORT _qt_pixmap_cleanup_hook_64 qt_pixmap_cleanup_hook_64 = 0;

// ### Qt 5: remove
Q_GUI_EXPORT qint64 qt_pixmap_id(const QPixmap &pixmap)
{
    return pixmap.cacheKey();
}

/*!
    \enum QPixmap::ColorMode

    \compat

    This enum type defines the color modes that exist for converting
    QImage objects to QPixmap.  It is provided here for compatibility
    with earlier versions of Qt.

    Use Qt::ImageConversionFlags instead.

    \value Auto  Select \c Color or \c Mono on a case-by-case basis.
    \value Color Always create colored pixmaps.
    \value Mono  Always create bitmaps.
*/

/*!
    Constructs a null pixmap.

    \sa isNull()
*/

QPixmap::QPixmap()
    : QPaintDevice()
{
    init(0, 0);
}

/*!
    \fn QPixmap::QPixmap(int width, int height)

    Constructs a pixmap with the given \a width and \a height.

    The content of the pixmap is uninitialized.  If either \a width or
    \a height is zero, a null pixmap is constructed.

    \sa isNull()
*/

QPixmap::QPixmap(int w, int h)
    : QPaintDevice()
{
    init(w, h);
}

/*!
    \overload

    Constructs a pixmap of the given \a size.
*/

QPixmap::QPixmap(const QSize &size)
    : QPaintDevice()
{
    init(size.width(), size.height());
}

/*!
  \internal
*/
QPixmap::QPixmap(const QSize &s, Type type)
{
    init(s.width(), s.height(), type);
}

/*!
    Constructs a pixmap from the file with the given \a fileName. If the
    file does not exist or is of an unknown format, the pixmap becomes a
    null pixmap.

    The loader attempts to read the pixmap using the specified \a
    format. If the \a format is not specified (which is the default),
    the loader probes the file for a header to guess the file format.

    The file name can either refer to an actual file on disk or to
    one of the application's embedded resources. See the
    \l{resources.html}{Resource System} overview for details on how
    to embed images and other resource files in the application's
    executable.

    If the image needs to be modified to fit in a lower-resolution
    result (e.g. converting from 32-bit to 8-bit), use the \a
    flags to control the conversion.

    The \a fileName, \a format and \a flags parameters are
    passed on to load(). This means that the data in \a fileName is
    not compiled into the binary. If \a fileName contains a relative
    path (e.g. the filename only) the relevant file must be found
    relative to the runtime working directory.

    \sa {QPixmap#Reading and Writing Image Files}{Reading and Writing
    Image Files}
*/

QPixmap::QPixmap(const QString& fileName, const char *format, Qt::ImageConversionFlags flags)
    : QPaintDevice()
{
    init(0, 0);
    load(fileName, format, flags);
}

/*!
    Constructs a pixmap that is a copy of the given \a pixmap.

    \sa copy()
*/

QPixmap::QPixmap(const QPixmap &pixmap)
    : QPaintDevice()
{
    if (pixmap.paintingActive()) {                // make a deep copy
        data = 0;
        operator=(pixmap.copy());
    } else {
        data = pixmap.data;
        data->ref();
    }
}

/*!
    Constructs a pixmap from the given \a xpm data, which must be a
    valid XPM image.

    Errors are silently ignored.

    Note that it's possible to squeeze the XPM variable a little bit
    by using an unusual declaration:

    \code
        static const char * const start_xpm[]={
            "16 15 8 1",
            "a c #cec6bd",
        ....
    \endcode

    The extra \c const makes the entire definition read-only, which is
    slightly more efficient (for example, when the code is in a shared
    library) and ROMable when the application is to be stored in ROM.
*/

QPixmap::QPixmap(const char * const xpm[])
    : QPaintDevice()
{
    init(0, 0);
    if (!xpm)
        return;

    QImage image(xpm);
    if (!image.isNull()) {
        if (data->type == BitmapType)
            *this =  QBitmap::fromImage(image);
        else
            *this = fromImage(image);
    }
}



/*!
    Destroys the pixmap.
*/

QPixmap::~QPixmap()
{
    deref();
}

/*!
  \internal
*/
int QPixmap::devType() const
{
    return QInternal::Pixmap;
}

/*!
    \fn QPixmap QPixmap::copy(int x, int y, int width, int height) const
    \overload

    Returns a deep copy of the subset of the pixmap that is specified
    by the rectangle QRect( \a x, \a y, \a width, \a height).
*/

/*!
    \fn QPixmap QPixmap::copy(const QRect &rectangle) const

    Returns a deep copy of the subset of the pixmap that is specified
    by the given \a rectangle. For more information on deep copies,
    see the \l {Implicit Data Sharing} documentation.

    If the given \a rectangle is empty, the whole image is copied.

    \sa operator=(), QPixmap(), {QPixmap#Pixmap
    Transformations}{Pixmap Transformations}
*/

#if defined(Q_WS_WIN) || defined(Q_WS_QWS)
QPixmap QPixmap::copy(const QRect &rect) const
{
    QPixmap pm;
    if (data->type == BitmapType)
        pm = QBitmap::fromImage(toImage().copy(rect));
    else
        pm = fromImage(toImage().copy(rect));
    return pm;
}
#endif

/*!
    Assigns the given \a pixmap to this pixmap and returns a reference
    to this pixmap.

    \sa copy(), QPixmap()
*/

QPixmap &QPixmap::operator=(const QPixmap &pixmap)
{
    if (paintingActive()) {
        qWarning("QPixmap::operator=: Cannot assign to pixmap during painting");
        return *this;
    }
    if (pixmap.paintingActive()) {                // make a deep copy
        *this = pixmap.copy();
    } else {
        pixmap.data->ref();                                // avoid 'x = x'
        deref();
        data = pixmap.data;
    }
    return *this;
}

/*!
   Returns the pixmap as a QVariant.
*/
QPixmap::operator QVariant() const
{
    return QVariant(QVariant::Pixmap, this);
}

/*!
    \fn bool QPixmap::operator!() const

    Returns true if this is a null pixmap; otherwise returns false.

    \sa isNull()
*/

/*!
    \fn QPixmap::operator QImage() const

    Returns the pixmap as a QImage.

    Use the toImage() function instead.
*/

/*!
    \fn QMatrix QPixmap::trueMatrix(const QTransform &matrix, int width, int height)

    Returns the actual matrix used for transforming a pixmap with the
    given \a width, \a height and \a matrix.

    When transforming a pixmap using the transformed() function, the
    transformation matrix is internally adjusted to compensate for
    unwanted translation, i.e. transformed() returns the smallest
    pixmap containing all transformed points of the original
    pixmap. This function returns the modified matrix, which maps
    points correctly from the original pixmap into the new pixmap.

    \sa transformed(), {QPixmap#Pixmap Transformations}{Pixmap
    Transformations}
*/
QTransform QPixmap::trueMatrix(const QTransform &m, int w, int h)
{
    return QImage::trueMatrix(m, w, h);
}

/*!
  \overload

  This convenience function loads the matrix \a m into a
  QTransform and calls the overloaded function with the
  QTransform and the width \a w and the height \a h.
 */
QMatrix QPixmap::trueMatrix(const QMatrix &m, int w, int h)
{
    return trueMatrix(QTransform(m), w, h).toAffine();
}


/*!
    \fn bool QPixmap::isQBitmap() const

    Returns true if this is a QBitmap; otherwise returns false.
*/

/*!
    \fn bool QPixmap::isNull() const

    Returns true if this is a null pixmap; otherwise returns false.

    A null pixmap has zero width, zero height and no contents. You
    cannot draw in a null pixmap.
*/
bool QPixmap::isNull() const
{
    return data->w == 0;
}

/*!
    \fn int QPixmap::width() const

    Returns the width of the pixmap.

    \sa size(), {QPixmap#Pixmap Information}{Pixmap Information}
*/
int QPixmap::width() const
{
    return data->w;
}

/*!
    \fn int QPixmap::height() const

    Returns the height of the pixmap.

    \sa size(), {QPixmap#Pixmap Information}{Pixmap Information}
*/
int QPixmap::height() const
{
    return data->h;
}

/*!
    \fn QSize QPixmap::size() const

    Returns the size of the pixmap.

    \sa width(), height(), {QPixmap#Pixmap Information}{Pixmap
    Information}
*/
QSize QPixmap::size() const
{
    return QSize(data->w,data->h);
}

/*!
    \fn QRect QPixmap::rect() const

    Returns the pixmap's enclosing rectangle.

    \sa {QPixmap#Pixmap Information}{Pixmap Information}
*/
QRect QPixmap::rect() const
{
    return QRect(0,0,data->w,data->h);
}

/*!
    \fn int QPixmap::depth() const

    Returns the depth of the pixmap.

    The pixmap depth is also called bits per pixel (bpp) or bit planes
    of a pixmap. A null pixmap has depth 0.

    \sa defaultDepth(), {QPixmap#Pixmap Information}{Pixmap
    Information}
*/
int QPixmap::depth() const
{
    return data->d;
}

/*!
    \fn void QPixmap::resize(const QSize &size)
    \overload
    \compat

    Use the QPixmap constructor that takes a QSize (\a size) instead.

    \oldcode
        pixmap.resize(size);
    \newcode
        pixmap = QPixmap(size);
    \endcode
*/
#ifdef QT3_SUPPORT
void QPixmap::resize_helper(const QSize &s)
{
    int w = s.width();
    int h = s.height();
    if (w < 1 || h < 1) {
        *this = QPixmap();
        return;
    }

    int d = data->d;
    // Create new pixmap
    QPixmap pm(QSize(w, h), d == 1 ? BitmapType : PixmapType);
#ifdef Q_WS_X11
    pm.x11SetScreen(data->xinfo.screen());
#endif // Q_WS_X11
    if (!data->uninit && !isNull()) {                // has existing pixmap
        // Copy old pixmap
        QPainter p(&pm);
        p.drawPixmap(0, 0, *this, 0, 0, qMin(width(), w), qMin(height(), h));
    }
#if defined(Q_WS_MAC)
#ifndef QT_MAC_NO_QUICKDRAW
    if(data->qd_alpha)
        data->macQDUpdateAlpha();
#endif
#elif defined(Q_WS_X11)
    if (data->x11_mask) {
        pm.data->x11_mask = (Qt::HANDLE)XCreatePixmap(X11->display, RootWindow(data->xinfo.display(), data->xinfo.screen()),
                                                      w, h, 1);
        GC gc = XCreateGC(X11->display, pm.data->x11_mask, 0, 0);
        XCopyArea(X11->display, data->x11_mask, pm.data->x11_mask, gc, 0, 0, qMin(width(), w), qMin(height(), h), 0, 0);
        XFreeGC(X11->display, gc);
    }
#else
    if (data->mask) {
        QBitmap m = *data->mask;
        m.resize(w, h);
        pm.setMask(m);
    }
#endif
    *this = pm;
}
#endif

/*!
    \fn void QPixmap::resize(int width, int height)
    \compat

    Use the QPixmap constructor that takes two \c{int}s (\a width and
    \a height) instead.

    \oldcode
        pixmap.resize(10, 20);
    \newcode
        pixmap = QPixmap(10, 20);
    \endcode
*/

/*!
    \fn bool QPixmap::selfMask() const
    \compat

    Returns whether the pixmap is its own mask or not.

    This function is no longer relevant since the concept of self
    masking doesn't exists anymore.
*/

#ifndef QT_NO_IMAGE_HEURISTIC_MASK
/*!
    Creates and returns a heuristic mask for this pixmap.

    The function works by selecting a color from one of the corners
    and then chipping away pixels of that color, starting at all the
    edges.  If \a clipTight is true (the default) the mask is just
    large enough to cover the pixels; otherwise, the mask is larger
    than the data pixels.

    The mask may not be perfect but it should be reasonable, so you
    can do things such as the following:

    \code
        QPixmap myPixmap;
        myPixmap->setMask(myPixmap->createHeuristicMask());
    \endcode

    This function is slow because it involves converting to/from a
    QImage, and non-trivial computations.

    \sa QImage::createHeuristicMask(), createMaskFromColor()
*/
QBitmap QPixmap::createHeuristicMask(bool clipTight) const
{
    QBitmap m = QBitmap::fromImage(toImage().createHeuristicMask(clipTight));
    return m;
}
#endif

/*!
    Creates and returns a mask for this pixmap based on the given \a
    maskColor. If the \a mode is Qt::MaskInColor, all pixels matching the
    maskColor will be opaque. If \a mode is Qt::MaskOutColor, all pixels
    matching the maskColor will be transparent.

    This function is slow because it involves converting to/from a
    QImage.

    \sa createHeuristicMask(), QImage::createMaskFromColor()
*/
QBitmap QPixmap::createMaskFromColor(const QColor &maskColor, Qt::MaskMode mode) const
{
    QImage image = toImage().convertToFormat(QImage::Format_ARGB32);
    return QBitmap::fromImage(image.createMaskFromColor(maskColor.rgba(), mode));
}

/*! \overload

    Creates and returns a mask for this pixmap based on the given \a
    maskColor. Same as calling createMaskFromColor(maskColor,
    Qt::MaskInColor)

    \sa createHeuristicMask(), QImage::createMaskFromColor()
*/
QBitmap QPixmap::createMaskFromColor(const QColor &maskColor) const
{
    return createMaskFromColor(maskColor, Qt::MaskInColor);
}

/*!
    Loads a pixmap from the file with the given \a fileName. Returns
    true if the pixmap was successfully loaded; otherwise returns
    false.

    The loader attempts to read the pixmap using the specified \a
    format. If the \a format is not specified (which is the default),
    the loader probes the file for a header to guess the file format.

    The file name can either refer to an actual file on disk or to one
    of the application's embedded resources. See the
    \l{resources.html}{Resource System} overview for details on how to
    embed pixmaps and other resource files in the application's
    executable.

    If the data needs to be modified to fit in a lower-resolution
    result (e.g. converting from 32-bit to 8-bit), use the \a flags to
    control the conversion.

    Note that QPixmaps are automatically added to the QPixmapCache
    when loaded from a file; the key used is internal and can not
    be acquired.

    \sa loadFromData(), {QPixmap#Reading and Writing Image
    Files}{Reading and Writing Image Files}
*/

bool QPixmap::load(const QString &fileName, const char *format, Qt::ImageConversionFlags flags)
{
    if (fileName.isEmpty())
        return false;

    QFileInfo info(fileName);
    QString key = QLatin1String("qt_pixmap_") + info.absoluteFilePath() + QLatin1Char('_') + info.lastModified().toString() + QLatin1Char('_') +
                  QString::number(info.size()) + QLatin1Char('_') + QString::number(data->type);

    if (QPixmapCache::find(key, *this))
        return true;
    QImage image = QImageReader(fileName, format).read();
    if (image.isNull())
        return false;
    QPixmap pm;
    if (data->type == BitmapType)
        pm = QBitmap::fromImage(image, flags);
    else
        pm = fromImage(image, flags);
    if (!pm.isNull()) {
        *this = pm;
        QPixmapCache::insert(key, *this);
        return true;
    }
    return false;
}

/*!
    \fn bool QPixmap::loadFromData(const uchar *data, uint len, const char *format, Qt::ImageConversionFlags flags)

    Loads a pixmap from the \a len first bytes of the given binary \a
    data.  Returns true if the pixmap was loaded successfully;
    otherwise returns false.

    The loader attempts to read the pixmap using the specified \a
    format. If the \a format is not specified (which is the default),
    the loader probes the file for a header to guess the file format.

    If the data needs to be modified to fit in a lower-resolution
    result (e.g. converting from 32-bit to 8-bit), use the \a flags to
    control the conversion.

    \sa load(), {QPixmap#Reading and Writing Image Files}{Reading and
    Writing Image Files}
*/

bool QPixmap::loadFromData(const uchar *buf, uint len, const char *format, Qt::ImageConversionFlags flags)
{
    QByteArray a = QByteArray::fromRawData(reinterpret_cast<const char *>(buf), len);
    QBuffer b(&a);
    b.open(QIODevice::ReadOnly);

    QImage image = QImageReader(&b, format).read();
    QPixmap pm;
    if (data->type == BitmapType)
        pm = QBitmap::fromImage(image, flags);
    else
        pm = fromImage(image, flags);
    if (!pm.isNull()) {
        *this = pm;
        return true;
    }
    return false;
}

/*!
    \fn bool QPixmap::loadFromData(const QByteArray &data, const char *format, Qt::ImageConversionFlags flags)

    \overload

    Loads a pixmap from the binary \a data using the specified \a
    format and conversion \a flags.
*/


/*!
    Saves the pixmap to the file with the given \a fileName using the
    specified image file \a format and \a quality factor. Returns true
    if successful; otherwise returns false.

    The \a quality factor must be in the range [0,100] or -1. Specify
    0 to obtain small compressed files, 100 for large uncompressed
    files, and -1 to use the default settings.

    If \a format is 0, an image format will be chosen from \a fileName's
    suffix.

    \sa {QPixmap#Reading and Writing Image Files}{Reading and Writing
    Image Files}
*/

bool QPixmap::save(const QString &fileName, const char *format, int quality) const
{
    if (isNull())
        return false;                                // nothing to save
    QImageWriter writer(fileName, format);
    return doImageIO(&writer, quality);
}

/*!
    \overload

    This function writes a QPixmap to the given \a device using the
    specified image file \a format and \a quality factor. This can be
    used, for example, to save a pixmap directly into a QByteArray:

    \quotefromfile snippets/image/image.cpp
    \skipto PIX SAVE
    \skipto QPixmap
    \printuntil save
*/

bool QPixmap::save(QIODevice* device, const char* format, int quality) const
{
    if (isNull())
        return false;                                // nothing to save
    QImageWriter writer(device, format);
    return doImageIO(&writer, quality);
}

/*! \internal
*/

bool QPixmap::doImageIO(QImageWriter *writer, int quality) const
{
    if (quality > 100  || quality < -1)
        qWarning("QPixmap::save: quality out of range [-1,100]");
    if (quality >= 0)
        writer->setQuality(qMin(quality,100));
    return writer->write(toImage());
}


// The implementation (and documentation) of
// QPixmap::fill(const QWidget *, const QPoint &)
// is in qwidget.cpp

/*!
    \fn void QPixmap::fill(const QWidget *widget, int x, int y)
    \overload

    Fills the pixmap with the \a widget's background color or pixmap.
    The given point, (\a x, \a y), defines an offset in widget
    coordinates to which the pixmap's top-left pixel will be mapped
    to.
*/

/*! \obsolete
    Returns a number that identifies the contents of this QPixmap
    object. Distinct QPixmap objects can only have the same serial
    number if they refer to the same contents (but they don't have
    to).

    Use cacheKey() instead.

    \warning The serial number doesn't necessarily change when
    the pixmap is altered. This means that it may be dangerous to use
    it as a cache key. For caching pixmaps, we recommend using the
    QPixmapCache class whenever possible.
*/
int QPixmap::serialNumber() const
{
    if (isNull())
        return 0;
    else
        return data->ser_no;
}

/*!
    Returns a number that identifies this QPixmap. Distinct QPixmap
    objects can only have the same cache key if they refer to the same
    contents.

    The cacheKey() will change when the pixmap is altered.
*/
qint64 QPixmap::cacheKey() const
{
    return (((qint64) data->ser_no) << 32) | ((qint64) (data->detach_no));
}

static void sendResizeEvents(QWidget *target)
{
    QResizeEvent e(target->size(), QSize());
    QApplication::sendEvent(target, &e);

    const QObjectList children = target->children();
    for (int i = 0; i < children.size(); ++i) {
        QWidget *child = static_cast<QWidget*>(children.at(i));
        if (child->isWidgetType() && !child->isWindow() && child->testAttribute(Qt::WA_PendingResizeEvent))
            sendResizeEvents(child);
    }
}

/*!
    \fn QPixmap QPixmap::grabWidget(QWidget * widget, const QRect &rectangle)

    Creates a pixmap and paints the given \a widget, restricted by the
    given \a rectangle, in it. If the \a widget has any children, then
    they are also painted in the appropriate positions.

    If no rectangle is specified (the default) the entire widget is
    painted.

    If \a widget is 0, the specified rectangle doesn't overlap the
    widget's rectangle, or an error occurs, the function will return a
    null QPixmap.  If the rectangle is a superset of the given \a
    widget, the areas outside the \a widget are covered with the
    widget's background.

    This function actually asks \a widget to paint itself (and its
    children to paint themselves) by calling paintEvent() with painter
    redirection turned on. But QPixmap also provides the grabWindow()
    function which is a bit faster grabbing pixels directly off the
    screen. In addition, if there are overlaying windows,
    grabWindow(), unlike grabWidget(), will see them.

    \warning Do not call this function from QWidget::paintEvent().

    \sa grabWindow()
*/

QPixmap QPixmap::grabWidget(QWidget * widget, const QRect &rect)
{
    if (!widget)
        return QPixmap();

    if (widget->testAttribute(Qt::WA_PendingResizeEvent) || !widget->testAttribute(Qt::WA_WState_Created))
        sendResizeEvents(widget);

    QRect r(rect);
    if (r.width() < 0)
        r.setWidth(widget->width() - rect.x());
    if (r.height() < 0)
        r.setHeight(widget->height() - rect.y());

    if (!r.intersects(widget->rect()))
        return QPixmap();

    QPixmap res(r.size());
    widget->render(&res, -r.topLeft(), r,
                   QWidget::DrawWindowBackground | QWidget::DrawChildren | QWidget::IgnoreMask);
    return res;
}

/*!
    \fn QPixmap QPixmap::grabWidget(QWidget *widget, int x, int y, int
    width, int height)

    \overload

    Creates a pixmap and paints the given \a widget, restricted by
    QRect(\a x, \a y, \a width, \a height), in it.

    \warning Do not call this function from QWidget::paintEvent().
*/


#if defined(Q_WS_X11) || defined(Q_WS_QWS)
/*!
    Returns the pixmap's handle to the device context.

    Note that, since QPixmap make use of \l {Implicit Data
    Sharing}{implicit data sharing}, the detach() function must be
    called explicitly to ensure that only \e this pixmap's data is
    modified if the pixmap data is shared.

    \warning This function is X11 specific; using it is non-portable.

    \sa detach()
*/

Qt::HANDLE QPixmap::handle() const
{
    return data->hd;
}
#endif


#ifdef QT3_SUPPORT
static Qt::ImageConversionFlags colorModeToFlags(QPixmap::ColorMode mode)
{
    Qt::ImageConversionFlags flags = Qt::AutoColor;
    switch (mode) {
      case QPixmap::Color:
        flags |= Qt::ColorOnly;
        break;
      case QPixmap::Mono:
        flags |= Qt::MonoOnly;
        break;
      default:
        break;// Nothing.
    }
    return flags;
}

/*!
    Use the constructor that takes a Qt::ImageConversionFlag instead.
*/

QPixmap::QPixmap(const QString& fileName, const char *format, ColorMode mode)
    : QPaintDevice()
{
    init(0, 0);
    load(fileName, format, colorModeToFlags(mode));
}

/*!
    Constructs a pixmap from the QImage \a image.

    Use the static fromImage() function instead.
*/

QPixmap::QPixmap(const QImage& image)
    : QPaintDevice()
{
    init(0, 0);
    if (data->type == BitmapType)
        *this = QBitmap::fromImage(image);
    else
        *this = fromImage(image);
}

/*!
    \overload

    Converts the given \a image to a pixmap that is assigned to this
    pixmap.

    Use the static fromImage() function instead.
*/

QPixmap &QPixmap::operator=(const QImage &image)
{
    if (data->type == BitmapType)
        *this = QBitmap::fromImage(image);
    else
        *this = fromImage(image);
    return *this;
}

/*!
    Use the load() function that takes a Qt::ImageConversionFlag instead.
*/

bool QPixmap::load(const QString &fileName, const char *format, ColorMode mode)
{
    return load(fileName, format, colorModeToFlags(mode));
}

/*!
    Use the loadFromData() function that takes a Qt::ImageConversionFlag instead.
*/

bool QPixmap::loadFromData(const uchar *buf, uint len, const char *format, ColorMode mode)
{
    return loadFromData(buf, len, format, colorModeToFlags(mode));
}

/*!
    Use the static fromImage() function instead.
*/
bool QPixmap::convertFromImage(const QImage &image, ColorMode mode)
{
    if (data->type == BitmapType)
        *this = QBitmap::fromImage(image, colorModeToFlags(mode));
    else
        *this = fromImage(image, colorModeToFlags(mode));
    return !isNull();
}

#endif

/*****************************************************************************
  QPixmap stream functions
 *****************************************************************************/
#if !defined(QT_NO_DATASTREAM)
/*!
    \relates QPixmap

    Writes the given \a pixmap to the the given \a stream as a PNG
    image. Note that writing the stream to a file will not produce a
    valid image file.

    \sa QPixmap::save(), {Format of the QDataStream Operators}
*/

QDataStream &operator<<(QDataStream &stream, const QPixmap &pixmap)
{
    return stream << pixmap.toImage();
}

/*!
    \relates QPixmap

    Reads an image from the given \a stream into the given \a pixmap.

    \sa QPixmap::load(), {Format of the QDataStream Operators}
*/

QDataStream &operator>>(QDataStream &stream, QPixmap &pixmap)
{
    QImage image;
    stream >> image;

    if (image.isNull()) {
        pixmap = QPixmap();
    } else if (image.depth() == 1) {
        pixmap = QBitmap::fromImage(image);
    } else {
        pixmap = QPixmap::fromImage(image);
    }
    return stream;
}

#endif //QT_NO_DATASTREAM

#ifdef QT3_SUPPORT
Q_GUI_EXPORT void copyBlt(QPixmap *dst, int dx, int dy,
                          const QPixmap *src, int sx, int sy, int sw, int sh)
{
    Q_ASSERT_X(dst, "::copyBlt", "Destination pixmap must be non-null");
    Q_ASSERT_X(src, "::copyBlt", "Source pixmap must be non-null");

    if (src->hasAlphaChannel()) {
        if (dst->paintEngine()->hasFeature(QPaintEngine::PorterDuff)) {
            QPainter p(dst);
            p.setCompositionMode(QPainter::CompositionMode_Source);
            p.drawPixmap(dx, dy, *src, sx, sy, sw, sh);
        } else {
            QImage image = dst->toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
            QPainter p(&image);
            p.setCompositionMode(QPainter::CompositionMode_Source);
            p.drawPixmap(dx, dy, *src, sx, sy, sw, sh);
            p.end();
            *dst = QPixmap::fromImage(image);
        }
    } else {
        QPainter p(dst);
        p.drawPixmap(dx, dy, *src, sx, sy, sw, sh);
    }

}
#endif

/*!
    \internal
*/

bool QPixmap::isDetached() const
{
    return data->count == 1;
}

void QPixmap::deref()
{
    if(data && data->deref()) { // Destroy image if last ref
        if (qt_pixmap_cleanup_hook_64)
            qt_pixmap_cleanup_hook_64(cacheKey());
        delete data;
        data = 0;
    }
}

/*!
    \fn QImage QPixmap::convertToImage() const

    Use the toImage() function instead.
*/

/*!
    \fn bool QPixmap::convertFromImage(const QImage &image, Qt::ImageConversionFlags flags)

    Use the static fromImage() function instead.
*/

/*!
    \fn QPixmap QPixmap::xForm(const QMatrix &matrix) const

    Use transformed() instead.
*/

/*!
    \fn QPixmap QPixmap::scaled(int width, int height,
    Qt::AspectRatioMode aspectRatioMode, Qt::TransformationMode
    transformMode) const

    \overload

    Returns a copy of the pixmap scaled to a rectangle with the given
    \a width and \a height according to the given \a aspectRatioMode and
    \a transformMode.

    If either the \a width or the \a height is zero or negative, this
    function returns a null pixmap.
*/

/*!
    \fn QPixmap QPixmap::scaled(const QSize &size, Qt::AspectRatioMode
    aspectRatioMode, Qt::TransformationMode transformMode) const

    Scales the pixmap to the given \a size, using the aspect ratio and
    transformation modes specified by \a aspectRatioMode and \a
    transformMode.

    \image qimage-scaling.png

    \list
    \i If \a aspectRatioMode is Qt::IgnoreAspectRatio, the pixmap
       is scaled to \a size.
    \i If \a aspectRatioMode is Qt::KeepAspectRatio, the pixmap is
       scaled to a rectangle as large as possible inside \a size, preserving the aspect ratio.
    \i If \a aspectRatioMode is Qt::KeepAspectRatioByExpanding,
       the pixmap is scaled to a rectangle as small as possible
       outside \a size, preserving the aspect ratio.
    \endlist

    If the given \a size is empty, this function returns a null
    pixmap.

    \sa isNull(), {QPixmap#Pixmap Transformations}{Pixmap
    Transformations}

*/
QPixmap QPixmap::scaled(const QSize& s, Qt::AspectRatioMode aspectMode, Qt::TransformationMode mode) const
{
    if (isNull()) {
        qWarning("QPixmap::scaled: Pixmap is a null pixmap");
        return QPixmap();
    }
    if (s.isEmpty())
        return QPixmap();

    QSize newSize = size();
    newSize.scale(s, aspectMode);
    if (newSize == size())
        return copy();

    QPixmap pix;
    QTransform wm;
    wm.scale((double)newSize.width() / width(), (double)newSize.height() / height());
    pix = transformed(wm, mode);
    return pix;
}

/*!
    \fn QPixmap QPixmap::scaledToWidth(int width, Qt::TransformationMode
    mode) const

    Returns a scaled copy of the image. The returned image is scaled
    to the given \a width using the specified transformation \a mode.
    The height of the pixmap is automatically calculated so that the
    aspect ratio of the pixmap is preserved.

    If \a width is 0 or negative, a null pixmap is returned.

    \sa isNull(), {QPixmap#Pixmap Transformations}{Pixmap
    Transformations}
*/
QPixmap QPixmap::scaledToWidth(int w, Qt::TransformationMode mode) const
{
    if (isNull()) {
        qWarning("QPixmap::scaleWidth: Pixmap is a null pixmap");
        return copy();
    }
    if (w <= 0)
        return QPixmap();

    QTransform wm;
    double factor = (double) w / width();
    wm.scale(factor, factor);
    return transformed(wm, mode);
}

/*!
    \fn QPixmap QPixmap::scaledToHeight(int height,
    Qt::TransformationMode mode) const

    Returns a scaled copy of the image. The returned image is scaled
    to the given \a height using the specified transformation \a mode.
    The width of the pixmap is automatically calculated so that the
    aspect ratio of the pixmap is preserved.

    If \a height is 0 or negative, a null pixmap is returned.

    \sa isNull(), {QPixmap#Pixmap Transformations}{Pixmap
    Transformations}
*/
QPixmap QPixmap::scaledToHeight(int h, Qt::TransformationMode mode) const
{
    if (isNull()) {
        qWarning("QPixmap::scaleHeight: Pixmap is a null pixmap");
        return copy();
    }
    if (h <= 0)
        return QPixmap();

    QTransform wm;
    double factor = (double) h / height();
    wm.scale(factor, factor);
    return transformed(wm, mode);
}

/*!
    \class QPixmap

    \brief The QPixmap class is an off-screen image representation
    that can be used as a paint device.

    \ingroup multimedia
    \ingroup shared
    \mainclass

    Qt provides four classes for handling image data: QImage, QPixmap,
    QBitmap and QPicture. QImage is designed and optimized for I/O,
    and for direct pixel access and manipulation, while QPixmap is
    designed and optimized for showing images on screen. QBitmap is
    only a convenience class that inherits QPixmap, ensuring a depth
    of 1. The isQBitmap() function returns true if a QPixmap object is
    really a bitmap, otherwise returns false. Finally, the QPicture class is a
    paint device that records and replays QPainter commands.

    A QPixmap can easily be displayed on the screen using QLabel or
    one of QAbstractButton's subclasses (such as QPushButton and
    QToolButton). QLabel has a pixmap property, whereas
    QAbstractButton has an icon property. And because QPixmap is a
    QPaintDevice subclass, QPainter can be used to draw directly onto
    pixmaps.

    In addition to the ordinary constructors, a QPixmap can be
    constructed using the static grabWidget() and grabWindow()
    functions which creates a QPixmap and paints the given widget, or
    window, in it.

    Note that the pixel data in a pixmap is internal and is managed by
    the underlying window system. Pixels can only be accessed through
    QPainter functions or by converting the QPixmap to a QImage.
    Depending on the system, QPixmap is stored using a RGB32 or a
    premultiplied alpha format. If the image has an alpha channel, and
    if the system allows, the preferred format is premultiplied alpha.
    Note also that QPixmap, unlike QImage, may be hardware dependent.
    On X11 and Mac, a QPixmap is stored on the server side while a
    QImage is stored on the client side (on Windows, these two classes
    have an equivalent internal representation, i.e. both QImage and
    QPixmap are stored on the client side and don't use any GDI
    resources).

    There are functions to convert between QImage and
    QPixmap. Typically, the QImage class is used to load an image
    file, optionally manipulating the image data, before the QImage
    object is converted into a QPixmap to be shown on
    screen. Alternatively, if no manipulation is desired, the image
    file can be loaded directly into a QPixmap. On Windows, the
    QPixmap class also supports conversion between \c HBITMAP and
    QPixmap.

    QPixmap provides a collection of functions that can be used to
    obtain a variety of information about the pixmap. In addition,
    there are several functions that enables transformation of the
    pixmap.

    QPixmap objects can be passed around by value since the QPixmap
    class uses implicit data sharing. For more information, see the \l
    {Implicit Data Sharing} documentation. QPixmap objects can also be
    streamed.

    \tableofcontents

    \section1 Reading and Writing Image Files

    QPixmap provides several ways of reading an image file: The file
    can be loaded when constructing the QPixmap object, or by using
    the load() or loadFromData() functions later on. When loading an
    image, the file name can either refer to an actual file on disk or
    to one of the application's embedded resources. See \l{The Qt
    Resource System} overview for details on how to embed images and
    other resource files in the application's executable.

    Simply call the save() function to save a QPixmap object.

    The complete list of supported file formats are available through
    the QImageReader::supportedImageFormats() and
    QImageWriter::supportedImageFormats() functions. New file formats
    can be added as plugins. By default, Qt supports the following
    formats:

    \table
    \header \o Format \o Description                      \o Qt's support
    \row    \o BMP    \o Windows Bitmap                   \o Read/write
    \row    \o GIF    \o Graphic Interchange Format (optional) \o Read
    \row    \o JPG    \o Joint Photographic Experts Group \o Read/write
    \row    \o JPEG   \o Joint Photographic Experts Group \o Read/write
    \row    \o PNG    \o Portable Network Graphics        \o Read/write
    \row    \o PBM    \o Portable Bitmap                  \o Read
    \row    \o PGM    \o Portable Graymap                 \o Read
    \row    \o PPM    \o Portable Pixmap                  \o Read/write
    \row    \o XBM    \o X11 Bitmap                       \o Read/write
    \row    \o XPM    \o X11 Pixmap                       \o Read/write
    \endtable

    (To configure Qt with GIF support, pass \c -qt-gif to the \c
    configure script or check the appropriate option in the graphical
    installer.)

    \section1 Pixmap Information

    QPixmap provides a collection of functions that can be used to
    obtain a variety of information about the pixmap:

    \table
    \header
    \o \o Available Functions
    \row
    \o Geometry
    \o
    The size(), width() and height() functions provide information
    about the pixmap's size. The rect() function returns the image's
    enclosing rectangle.

    \row
    \o Alpha component
    \o

    The hasAlphaChannel() returns true if the pixmap has a format that
    respects the alpha channel, otherwise returns false, while the
    hasAlpha() function returns true if the pixmap has an alpha
    channel \e or a mask (otherwise false).

    The alphaChannel() function returns the alpha channel as a new
    QPixmap object, while the mask() function returns the mask as a
    QBitmap object. The alpha channel and mask can be set using the
    setAlphaChannel() and setMask() functions, respectively.

    \row
    \o Low-level information
    \o

    The depth() function returns the depth of the pixmap. The
    defaultDepth() function returns the default depth, i.e. the depth
    used by the application on the given screen.

    The cacheKey() function returns a number that uniquely
    identifies the contents of the QPixmap object.

    The x11Info() function returns information about the configuration
    of the X display used to display the widget.  The
    x11PictureHandle() function returns the X11 Picture handle of the
    pixmap for XRender support. Note that the two latter functions are
    only available on x11.

    \endtable

    \section1 Pixmap Conversion

    A QPixmap object can be converted into a QImage using the
    toImage() function. Likewise, a QImage can be converted into a
    QPixmap using the fromImage(). If this is too expensive an
    operation, you can use QBitmap::fromImage() instead.

    In addition, on Windows, the QPixmap class supports conversion to
    and from HBitmap: the toWinHBITMAP() function creates a HBITMAP
    equivalent to the QPixmap, based on the given HBitmapFormat, and
    returns the HBITMAP handle. The fromWinHBITMAP() function returns
    a QPixmap that is equivalent to the given bitmap which has the
    specified format.

    \section1 Pixmap Transformations

    QPixmap supports a number of functions for creating a new pixmap
    that is a transformed version of the original: The
    createHeuristicMask() function creates and returns a 1-bpp
    heuristic mask (i.e. a QBitmap) for this pixmap. It works by
    selecting a color from one of the corners and then chipping away
    pixels of that color, starting at all the edges. The
    createMaskFromColor() function creates and returns a mask (i.e. a
    QBitmap) for the pixmap based on a given color.


    The scaled(), scaledToWidth() and scaledToHeight() functions
    return scaled copies of the pixmap, while the copy() function
    creates a QPixmap that is a plain copy of the original one.

    The transformed() function returns a copy of the pixmap that is
    transformed with the given transformation matrix and
    transformation mode: Internally, the transformation matrix is
    adjusted to compensate for unwanted translation,
    i.e. transformed() returns the smallest pixmap containing all
    transformed points of the original pixmap. The static trueMatrix()
    function returns the actual matrix used for transforming the
    pixmap.

    There are also functions for changing attributes of a pixmap.
    in-place: The fill() function fills the entire image with the
    given color, the setMask() function sets a mask bitmap, and the
    setAlphaChannel() function sets the pixmap's alpha channel.

    \sa QBitmap, QImage, QImageReader, QImageWriter
*/


/*!
    \typedef QPixmap::DataPtr
    \internal
*/

/*!
    \fn DataPtr &QPixmap::data_ptr()
    \internal
*/
