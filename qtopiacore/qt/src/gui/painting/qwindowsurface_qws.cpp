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

#include "qwindowsurface_qws_p.h"
#include <qwidget.h>
#include <qscreen_qws.h>
#include <qwsmanager_qws.h>
#include <qapplication.h>
#include <qwsdisplay_qws.h>
#include <qrgb.h>
#include <qpaintengine.h>
#include <qdesktopwidget.h>
#include <private/qapplication_p.h>
#include <private/qwsdisplay_qws_p.h>
#include <private/qwidget_p.h>
#include <private/qwsmanager_p.h>
#include <private/qwslock_p.h>
#include <private/qbackingstore_p.h>
#include <stdio.h>

#ifdef Q_BACKINGSTORE_SUBSURFACES

typedef QMap<int, QWSWindowSurface*> SurfaceMap;
Q_GLOBAL_STATIC(SurfaceMap, winIdToSurfaceMap);

QWSWindowSurface* qt_findWindowSurface(int winId)
{
    return winIdToSurfaceMap()->value(winId);
}

static void qt_insertWindowSurface(int winId, QWSWindowSurface *surface)
{
    if (!surface)
        winIdToSurfaceMap()->remove(winId);
    else
        winIdToSurfaceMap()->insert(winId, surface);
}

#endif // Q_BACKINGSTORE_SUBSURFACES

inline bool isWidgetOpaque(const QWidget *w)
{
    return w->d_func()->isOpaque();
}

static inline QScreen *getScreen(const QWidget *w)
{
    const QList<QScreen*> subScreens = qt_screen->subScreens();
    if (subScreens.isEmpty())
        return qt_screen;

    const int screen = QApplication::desktop()->screenNumber(w);

    return qt_screen->subScreens().at(screen < 0 ? 0 : screen);
}


static inline void setImageMetrics(QImage &img, QWidget *window) {
    QScreen *myScreen = getScreen(window);
    if (myScreen) {
        int dpmx = myScreen->width()*1000 / myScreen->physicalWidth();
        int dpmy = myScreen->height()*1000 / myScreen->physicalHeight();
        img.setDotsPerMeterX(dpmx);
        img.setDotsPerMeterY(dpmy);
    }
}

class QWSWindowSurfacePrivate
{
public:
    QWSWindowSurfacePrivate() : flags(0), winId(0) {}

    void setWinId(int id) { winId = id; }

    QWSWindowSurface::SurfaceFlags flags;
    QRegion dirty;
    QRegion clip;
    QRegion clippedDirty; // dirty, but currently outside the clip region

    int winId;
};

// XXX: this should probably be handled in QWidgetPrivate
void QWSWindowSurface::invalidateBuffer()
{
    d_ptr->dirty = QRegion();
    d_ptr->clip = QRegion();
    d_ptr->clippedDirty = QRegion();

    QWidget *win = window();
    if (win) {
#ifdef Q_BACKINGSTORE_SUBSURFACES
        const QPoint offset = -win->mapToGlobal(QPoint());
#else
        const QPoint offset = -win->geometry().topLeft();
#endif
        setDirty(geometry().translated(offset)); // XXX: clip with mask

#ifndef QT_NO_QWS_MANAGER
        QTLWExtra *topextra = win->d_func()->extra->topextra;
        QWSManager *manager = topextra->qwsManager;
        if (manager)
            manager->d_func()->dirtyRegion(QDecoration::All,
                                           QDecoration::Normal);
#endif
    }
}

int QWSWindowSurface::winId() const
{
    // XXX: the widget winId may change during the lifetime of the widget!!!

    const QWidget *win = window();
    if (win && win->isWindow())
        return win->internalWinId();

#ifdef Q_BACKINGSTORE_SUBSURFACES
    if (!d_ptr->winId) {
        QWSWindowSurface *that = const_cast<QWSWindowSurface*>(this);
        QWSDisplay *display = QWSDisplay::instance();
        const int id = display->takeId();
        qt_insertWindowSurface(id, that);
        that->d_ptr->winId = id;

        if (win)
            display->nameRegion(id, win->objectName(), win->windowTitle());
        else
            display->nameRegion(id, QString(), QString());

        display->setAltitude(id, 1, true); // XXX
    }
#endif

    return d_ptr->winId;
}

void QWSWindowSurface::setWinId(int id)
{
    d_ptr->winId = id;
}

/*!
    \class QWSWindowSurface
    \since 4.2
    \ingroup qws
    \preliminary
    \internal

    \brief The QWSWindowSurface class provides the drawing area for top-level
    windows in Qtopia Core.

    Note that this class is only available in Qtopia Core.

    In \l {Qtopia Core}, the default behavior is for each client to
    render its widgets into memory while the server is responsible for
    putting the contents of the memory onto the
    screen. QWSWindowSurface is used by the window system to implement
    the associated memory allocation.

    When a screen update is required, the server runs through all the
    top-level windows that intersect with the region that is about to
    be updated, and ensures that the associated clients have updated
    their memory buffer. Then the server uses the screen driver to
    copy the content of the memory to the screen. To locate the
    relevant parts of memory, the driver is provided with the list of
    top-level windows that intersect with the given region. Associated
    with each of the top-level windows there is a window surface
    representing the drawing area of the window.

    When deriving from the QWSWindowSurface class, e.g., when adding
    an \l {Adding an Accelerated Graphics Driver in Qtopia
    Core}{accelerated graphics driver}, there are several pure virtual
    functions that must be implemented. In addition, QWSWindowSurface
    provides several virtual functions that can be reimplemented to
    customize the drawing process.

    \tableofcontents

    \section1 Pure Virtual Functions

    There are in fact two window surface instances for each top-level
    window; one used by the application when drawing a window, and
    another used by the server application to perform window
    compositioning. Implement the attach() to create the server-side
    representation of the surface. The data() function must be
    implemented to provide the required data.

    Implement the key() function to uniquely identify the surface
    class, and the isValid() function to determine is a surface
    corresponds to a given widget.

    The geometry() function must be implemented to let the window
    system determine the area required by the window surface
    (QWSWindowSurface also provides a corresponding virtual
    setGeometry() function that is called whenever the area necessary
    for the top-level window to be drawn, changes). The image()
    function is called by the window system during window
    compositioning, and must be implemented to return an image of the
    top-level window.

    Finally, the paintDevice() function must be implemented to return
    the appropriate paint device, and the scroll() function must be
    implemented to scroll the given region of the surface the given
    number of pixels.

    \section1 Virtual Functions

    When painting onto the surface, the window system will always call
    the beginPaint() function before any painting operations are
    performed. Likewise the endPaint() function is automatically
    called when the painting is done. Reimplement the painterOffset()
    function to alter the offset that is applied when drawing.

    The window system uses the flush() function to put a given region
    of the widget onto the screen, and the release() function to
    deallocate the screen region corresponding to this window surface.

    \section1 Other Members

    QWSWindowSurface provides the window() function returning a
    pointer to the top-level window the surface is representing. The
    currently visible region of the associated widget can be retrieved
    and set using the clipRegion() and setClipRegion() functions,
    respectively.

    When the window system performs the window compositioning, it uses
    the SurfaceFlag enum describing the surface content. The currently
    set surface flags can be retrieved and altered using the
    surfaceFlags() and setSurfaceFlags() functions. In addition,
    QWSWindowSurface provides the isBuffered(), isOpaque() and
    isRegionReserved() convenience functions.  Use the dirtyRegion()
    function to retrieve the part of the widget that must be
    repainted, and the setDirty() function to ensure that a region is
    repainted.

    \sa {Qtopia Core Architecture#Drawing on Screen}{Qtopia
    Core Architecture}
*/

/*!
    \enum QWSWindowSurface::SurfaceFlag

    This enum is used to describe the window surface's contents.  It
    is used by the screen driver to handle region allocation and
    composition.

    \value RegionReserved The surface contains a reserved area. Once
    allocated, a reserved area can not not be changed by the window
    system, i.e., no other widgets can be drawn on top of this.

    \value Buffered
    The surface is in a memory area which is not part of a framebuffer.
    (A top-level window with QWidget::windowOpacity() other than 1.0 must use
    a buffered surface in order to making blending with the background work.)

    \value Opaque
    The surface contains only opaque pixels.

    \sa surfaceFlags(), setSurfaceFlags()
*/

/*!
    \fn bool QWSWindowSurface::isValid() const
    \since 4.3

    Implement this function to return true if the surface is a valid
    surface for the given top-level \a window; otherwise return
    false.

    \sa window(), key()
*/

/*!
    \fn QString QWSWindowSurface::key() const

    Implement this function to return a string that uniquely
    identifies the class of this surface.

    \sa window(), isValid()
*/

/*!
    \fn QByteArray QWSWindowSurface::permanentState() const
    \since 4.3

    Implement this function to return the data required for creating a
    server-side representation of the surface.

    \sa attach()
*/

/*!
    \fn void QWSWindowSurface::setPermanentState(const QByteArray &data)
    \since 4.3

    Implement this function to attach a server-side surface instance
    to the corresponding client side instance using the given \a
    data. Return true if successful; otherwise return false.

    \sa data()
*/

/*!
    \fn const QImage QWSWindowSurface::image() const

    Implement this function to return an image of the top-level window.

    \sa geometry()
*/

/*!
    \fn bool QWSWindowSurface::isRegionReserved() const

    Returns true if the QWSWindowSurface::RegionReserved is set; otherwise
    returns false.

    \sa surfaceFlags()
*/

/*!
    \fn bool QWSWindowSurface::isBuffered() const

    Returns true if the QWSWindowSurface::Buffered is set; otherwise returns false.

    \sa surfaceFlags()
*/

/*!
    \fn bool QWSWindowSurface::isOpaque() const

    Returns true if the QWSWindowSurface::Opaque is set; otherwise
    returns false.

    \sa surfaceFlags()
*/


/*!
    Constructs an empty surface.
*/
QWSWindowSurface::QWSWindowSurface()
    : QWindowSurface(0), d_ptr(new QWSWindowSurfacePrivate)
{
}

/*!
    Constructs an empty surface for the given top-level \a widget.
*/
QWSWindowSurface::QWSWindowSurface(QWidget *widget)
    : QWindowSurface(widget), d_ptr(new QWSWindowSurfacePrivate)
{
}

QWSWindowSurface::~QWSWindowSurface()
{
    delete d_ptr;
}

/*!
    Returns the offset to be used when painting.

    \sa paintDevice()
*/
QPoint QWSWindowSurface::painterOffset() const
{
    const QWidget *w = window();
    if (!w)
        return QPoint();
    return w->geometry().topLeft() - w->frameGeometry().topLeft();
}

void QWSWindowSurface::beginPaint(const QRegion &)
{
    lock();
}

void QWSWindowSurface::endPaint(const QRegion &)
{
    unlock();
}

// XXX: documentation!!!
QByteArray QWSWindowSurface::transientState() const
{
    return QByteArray();
}

QByteArray QWSWindowSurface::permanentState() const
{
    return QByteArray();
}

void QWSWindowSurface::setTransientState(const QByteArray &state)
{
    Q_UNUSED(state);
}

void QWSWindowSurface::setPermanentState(const QByteArray &state)
{
    Q_UNUSED(state);
}

bool QWSWindowSurface::lock(int timeout)
{
    Q_UNUSED(timeout);
    return true;
}

void QWSWindowSurface::unlock()
{
}

/*!
    Returns the region that must be repainted.

    \sa setDirty()
*/
const QRegion QWSWindowSurface::dirtyRegion() const
{
    return d_ptr->dirty;
}

/*!
    Marks the given \a region as dirty, i.e., altered.

    \sa dirtyRegion()
*/
void QWSWindowSurface::setDirty(const QRegion &region) const
{
    if (region.isEmpty())
        return;

    const bool updatePosted = !d_ptr->dirty.isEmpty();
    d_ptr->dirty += region & d_ptr->clip;
    d_ptr->clippedDirty += (region - d_ptr->clip);

    if (updatePosted)
        return;

    QWidget *win = window();
    if (win && !d_ptr->dirty.isEmpty())
        QApplication::postEvent(win, new QEvent(QEvent::UpdateRequest),
                                Qt::LowEventPriority);
}

/*!
    Returns the region currently visible on the screen.

    \sa setClipRegion()
*/
const QRegion QWSWindowSurface::clipRegion() const
{
    return d_ptr->clip;
}

/*!
    Sets the region currently visible on the screen to be the given \a
    clip region.

    \sa clipRegion()
*/
void QWSWindowSurface::setClipRegion(const QRegion &clip)
{
    if (clip == d_ptr->clip)
        return;

    QRegion expose = (clip - d_ptr->clip);
    d_ptr->clip = clip;

    QWidget *win = window();
#ifndef QT_NO_QWS_MANAGER
    if (win && win->isWindow() && !expose.isEmpty()) {
        QTLWExtra *topextra = win->d_func()->extra->topextra;
        QWSManager *manager = topextra->qwsManager;
        if (manager) {
            const QRegion r = manager->region().translated(
                -win->geometry().topLeft()) & expose;
            if (!r.isEmpty())
                 manager->d_func()->dirtyRegion(QDecoration::All,
                                                QDecoration::Normal, r);
        }
    }
#endif

    QRegion dirtyExpose;
    if (isBuffered()) {
        dirtyExpose = expose & d_ptr->clippedDirty;
        d_ptr->clippedDirty -= expose;
        expose -= dirtyExpose;
    } else {
        dirtyExpose = expose;
    }

    if (!dirtyExpose.isEmpty()) {
        setDirty(dirtyExpose);
        d_ptr->dirty += expose;
    } else if (!expose.isEmpty()) {
        // XXX: prevent flush() from resetting dirty and from flushing too much
        const QRegion oldDirty = d_ptr->dirty;
        d_ptr->dirty = QRegion();
        flush(win, expose, QPoint());
        d_ptr->dirty = oldDirty;
    }
}

/*!
    Returns the surface flags describing the contents of this surface.

    \sa isBuffered(), isOpaque(), isRegionReserved()
*/
QWSWindowSurface::SurfaceFlags QWSWindowSurface::surfaceFlags() const
{
    return d_ptr->flags;
}

/*!
    Sets the surface flags describing the contents of this surface, to
    be the given \a flags.

    \sa surfaceFlags()
*/
void QWSWindowSurface::setSurfaceFlags(SurfaceFlags flags)
{
    d_ptr->flags = flags;
}

void QWSWindowSurface::setGeometry(const QRect &rect)
{
    QRegion mask = rect;

    const QWidget *win = window();
    if (win) {
#ifndef QT_NO_QWS_MANAGER
        if (win->isWindow()) {
            QTLWExtra *topextra = win->d_func()->extra->topextra;
            QWSManager *manager = topextra->qwsManager;

            if (manager) {
                // The frame geometry is the bounding rect of manager->region,
                // which could be too much, so we need to clip.
                mask &= (manager->region() + win->geometry());
            }
        }
#endif

        const QRegion winMask = win->mask();
        if (!winMask.isEmpty())
            mask &= winMask.translated(win->geometry().topLeft());
    }

    setGeometry(rect, mask);
}

void QWSWindowSurface::setGeometry(const QRect &rect, const QRegion &mask)
{
    if (rect == geometry()) // XXX: && mask == prevMask
        return;

    const bool isResize = rect.size() != geometry().size();
    const bool needsRepaint = isResize || !isBuffered();

    QWindowSurface::setGeometry(rect);

    const QRegion region = mask & rect;
    QWidget::qwsDisplay()->requestRegion(winId(), key(), permanentState(),
                                         region);

    if (needsRepaint) {
        const QRegion oldClip = clipRegion();
        const QWidget *win = window();
        bool resetClip = false;

        if (win && isBuffered()) {
            const QPoint topLeft = win->geometry().topLeft();
            resetClip = region.translated(-topLeft) == oldClip;
        }

        invalidateBuffer();

        // Server won't send us a region event if we've requested the same
        // region as we used to have, so we need to set clip region ourselves.
        if (resetClip)
            setClipRegion(oldClip);
    }
}

static inline void flushUpdate(QWidget *widget, const QRegion &region,
                               const QPoint &offset)
{
#ifdef QT_NO_PAINT_DEBUG
    Q_UNUSED(widget);
    Q_UNUSED(region);
    Q_UNUSED(offset);
#else
    static int delay = -1;

    if (delay == -1)
        delay = qgetenv("QT_FLUSH_UPDATE").toInt() * 10;

    if (delay == 0)
        return;

    static QWSYellowSurface surface(true);
    surface.setDelay(delay);
    surface.flush(widget, region, offset);
#endif // QT_NO_PAINT_DEBUG
}

void QWSWindowSurface::flush(QWidget *widget, const QRegion &region,
                             const QPoint &offset)
{
    const QWidget *win = window();
    if (!win)
        return;

    Q_UNUSED(offset);

    const bool opaque = isWidgetOpaque(win);
#ifdef QT_QWS_DISABLE_FLUSHCLIPPING
    QRegion toFlush = region;
#else
    QRegion toFlush = region & d_ptr->clip;
#endif
    const QRegion stillDirty = (d_ptr->dirty - toFlush);

    if (!toFlush.isEmpty()) {
#ifndef QT_NO_QWS_MANAGER
        if (win->isWindow()) {
            QTLWExtra *topextra = win->d_func()->extra->topextra;
            QWSManager *manager = topextra->qwsManager;
            if (manager) {
                QWSManagerPrivate *managerPrivate = manager->d_func();
                if (!managerPrivate->dirtyRegions.isEmpty()) {
                    beginPaint(toFlush);
                    managerPrivate->paint(paintDevice(), toFlush);
                    endPaint(toFlush);
                }
            }
        }
#endif

        flushUpdate(widget, toFlush, QPoint(0, 0));

        toFlush.translate(win->mapToGlobal(QPoint(0, 0)));

        win->qwsDisplay()->repaintRegion(winId(), win->windowFlags(), opaque, toFlush);
    }

    // XXX: hw: this is not correct when painting outside a paint event.
    // The dirty region should be moved into QWidgetBackingstore.
    d_ptr->dirty = QRegion();
    setDirty(stillDirty);
}

/*!
    Move the surface with the given \a offset.

    A subclass may reimplement this function to enable accelerated window move.
    It must return true if the move was successful and no repaint is necessary,
    false otherwise.

    The default implementation updates the QWindowSurface geometry and
    returns true if the surface is buffered; false otherwise.

    This function is called by the window system on the client instance.

    \sa isBuffered()
*/
bool QWSWindowSurface::move(const QPoint &offset)
{
    QWindowSurface::setGeometry(geometry().translated(offset));
    return isBuffered();
}

/*!
    Move the surface with the given \a offset.

    The new visible region after the window move is given by \a newClip
    in screen coordinates.

    A subclass may reimplement this function to enable accelerated window move.
    The returned region indicates the area that still needs to be composed
    on the screen.

    The default implementation updates the QWindowSurface geometry and
    returns the union of the old and new geometry.

    This function is called by the window system on the server instance.
*/
QRegion QWSWindowSurface::move(const QPoint &offset, const QRegion &newClip)
{
    const QRegion oldGeometry = geometry();
    QWindowSurface::setGeometry(geometry().translated(offset));
    return oldGeometry + newClip;
}

static void scroll(const QImage &img, const QRect &rect, const QPoint &point)
{
    uchar *mem = const_cast<uchar*>(img.bits());

    int lineskip = img.bytesPerLine();
    int depth = img.depth() >> 3;

    const QRect r = rect;
    const QPoint p = rect.topLeft() + point;

    const uchar *src;
    uchar *dest;

    if (r.top() < p.y()) {
        src = mem + r.bottom() * lineskip + r.left() * depth;
        dest = mem + (p.y() + r.height() - 1) * lineskip + p.x() * depth;
        lineskip = -lineskip;
    } else {
        src = mem + r.top() * lineskip + r.left() * depth;
        dest = mem + p.y() * lineskip + p.x() * depth;
    }

    const int w = r.width();
    int h = r.height();
    const int bytes = w * depth;
    do {
        ::memmove(dest, src, bytes);
        dest += lineskip;
        src += lineskip;
    } while (--h);
}

bool QWSMemorySurface::lock(int timeout)
{
    Q_UNUSED(timeout);
#ifdef QT_NO_QWS_MULTIPROCESS
    return true;
#else
    if (!memlock)
        return true;
    return memlock->lock(QWSLock::BackingStore);
#endif
}

void QWSMemorySurface::unlock()
{
#ifndef QT_NO_QWS_MULTIPROCESS
    if (memlock)
        memlock->unlock(QWSLock::BackingStore);
#endif
}

QWSMemorySurface::QWSMemorySurface()
    : QWSWindowSurface()
#ifndef QT_NO_QWS_MULTIPROCESS
    , memlock(0)
#endif
{
    setSurfaceFlags(Buffered);
}

QWSMemorySurface::QWSMemorySurface(QWidget *w)
    : QWSWindowSurface(w)
{
    SurfaceFlags flags = Buffered;
    if (isWidgetOpaque(w))
        flags |= Opaque;
    setSurfaceFlags(flags);

#ifndef QT_NO_QWS_MULTIPROCESS
    memlock = QWSDisplay::Data::getClientLock();
#endif
}

QWSMemorySurface::~QWSMemorySurface()
{
}

QPixmap QWSMemorySurface::grabWidget(const QWidget *widget, const QRect &rectangle) const
{
    QPixmap result;

    if (widget->window() != window() || img.isNull())
        return result;

    QRect rect = rectangle.isEmpty() ? widget->rect() : (widget->rect() & rectangle);

    rect.translate(offset(widget));
    rect &= QRect(QPoint(), img.size());

    if (rect.isEmpty())
        return result;

    QImage subimg(img.scanLine(rect.y()) + rect.x() * img.depth() / 8,
                  rect.width(), rect.height(),
                  img.bytesPerLine(), img.format());
    subimg.detach(); //### expensive -- maybe we should have a real SubImage that shares reference count
    result = QPixmap::fromImage(subimg);
    return result;
}


QImage::Format
QWSMemorySurface::preferredImageFormat(const QWidget *widget) const
{
    const bool opaque = isWidgetOpaque(widget);

    if (opaque && getScreen(widget)->depth() <= 16)
        return QImage::Format_RGB16;
    else
        return QImage::Format_ARGB32_Premultiplied;
}

#ifndef QT_NO_QWS_MULTIPROCESS
void QWSMemorySurface::setLock(int lockId)
{
    if (memlock && memlock->id() == lockId)
        return;
    delete memlock;
    memlock = (lockId == -1 ? 0 : new QWSLock(lockId));
    return;
}
#endif // QT_NO_QWS_MULTIPROCESS

bool QWSMemorySurface::isValid() const
{
    if (img == QImage())
        return true;

    const QWidget *win = window();
    if (preferredImageFormat(win) != img.format())
        return false;

    if (isOpaque() != isWidgetOpaque(win)) // XXX: use QWidgetPrivate::isOpaque()
        return false;

    return true;
}

bool QWSMemorySurface::scroll(const QRegion &area, int dx, int dy)
{
    const QVector<QRect> rects = area.rects();
    for (int i = 0; i < rects.size(); ++i)
        ::scroll(img, rects.at(i), QPoint(dx, dy));

    return true;
}

QPoint QWSMemorySurface::painterOffset() const
{
    const QWidget *w = window();
    if (!w)
        return QPoint();

    if (w->mask().isEmpty())
        return QWSWindowSurface::painterOffset();

    const QRegion region = w->mask()
                           & w->frameGeometry().translated(-w->geometry().topLeft());
    return -region.boundingRect().topLeft();
}

QWSLocalMemSurface::QWSLocalMemSurface()
    : QWSMemorySurface(), mem(0), memsize(0)
{
}

QWSLocalMemSurface::QWSLocalMemSurface(QWidget *w)
    : QWSMemorySurface(w), mem(0), memsize(0)
{
}

QWSLocalMemSurface::~QWSLocalMemSurface()
{
    if (memsize)
        delete[] mem;
}

void QWSLocalMemSurface::setGeometry(const QRect &rect)
{
    QSize size = rect.size();

    QWidget *win = window();
    if (win && !win->mask().isEmpty()) {
        const QRegion region = win->mask()
                               & rect.translated(-win->geometry().topLeft());
        size = region.boundingRect().size();
    }

    if (img.size() != size) {
        QImage::Format imageFormat = preferredImageFormat(win);
        const int bytesPerPixel = imageFormat == QImage::Format_RGB16 ? 2 : 4;

        const int bpl = (size.width() * bytesPerPixel + 3) & ~3;
        memsize = bpl * size.height();

        delete[] mem;
        if (memsize == 0) {
            mem = 0;
            img = QImage();
        } else {
            mem = new uchar[memsize];
            img = QImage(mem, size.width(), size.height(), imageFormat);
            setImageMetrics(img, win);
        }
    }

    QWSWindowSurface::setGeometry(rect);
}

QByteArray QWSLocalMemSurface::permanentState() const
{
    QByteArray array;
    array.resize(sizeof(uchar*) + 3 * sizeof(int) +
                 sizeof(SurfaceFlags));

    char *ptr = array.data();

    *reinterpret_cast<uchar**>(ptr) = mem;
    ptr += sizeof(uchar*);

    reinterpret_cast<int*>(ptr)[0] = img.width();
    reinterpret_cast<int*>(ptr)[1] = img.height();
    ptr += 2 * sizeof(int);

    *reinterpret_cast<int *>(ptr) = img.format();
    ptr += sizeof(int);

    *reinterpret_cast<SurfaceFlags*>(ptr) = surfaceFlags();

    return array;
}

void QWSLocalMemSurface::setPermanentState(const QByteArray &data)
{
    int width;
    int height;
    QImage::Format format;
    SurfaceFlags flags;

    const char *ptr = data.constData();

    mem = *reinterpret_cast<uchar* const*>(ptr);
    ptr += sizeof(uchar*);

    width = reinterpret_cast<const int*>(ptr)[0];
    height = reinterpret_cast<const int*>(ptr)[1];
    ptr += 2 * sizeof(int);

    format = QImage::Format(*reinterpret_cast<const int*>(ptr));
    ptr += sizeof(int);

    flags = *reinterpret_cast<const SurfaceFlags*>(ptr);

    QWSMemorySurface::img = QImage(mem, width, height, format);
    setSurfaceFlags(flags);
}

#ifndef QT_NO_QWS_MULTIPROCESS

QWSSharedMemSurface::QWSSharedMemSurface()
    : QWSMemorySurface()
{
}

QWSSharedMemSurface::QWSSharedMemSurface(QWidget *widget)
    : QWSMemorySurface(widget)
{
}

QWSSharedMemSurface::~QWSSharedMemSurface()
{
    // mem.detach() is done automatically by ~QSharedMemory
}

bool QWSSharedMemSurface::setMemory(int memId)
{
    if (mem.id() == memId)
        return true;

    mem.detach();
    if (!mem.attach(memId)) {
        perror("QWSSharedMemSurface: attaching to shared memory");
        qCritical("QWSSharedMemSurface: Error attaching to"
                  " shared memory 0x%x", memId);
        return false;
    }

    return true;
}

void QWSSharedMemSurface::setPermanentState(const QByteArray &data)
{
    int memId;
    int width;
    int height;
    int lockId;
    QImage::Format format;
    SurfaceFlags flags;

    const char *ptr = data.constData();

    memId = reinterpret_cast<const int*>(ptr)[0];
    width = reinterpret_cast<const int*>(ptr)[1];
    height = reinterpret_cast<const int*>(ptr)[2];
    lockId = reinterpret_cast<const int*>(ptr)[3];
    ptr += 4 * sizeof(int);

    format = *reinterpret_cast<const QImage::Format*>(ptr);
    ptr += sizeof(QImage::Format);
    flags = *reinterpret_cast<const SurfaceFlags*>(ptr);

    setSurfaceFlags(flags);
    setMemory(memId);
    setLock(lockId);

    uchar *base = static_cast<uchar*>(mem.address());
    QWSMemorySurface::img = QImage(base, width, height, format);
}

void QWSSharedMemSurface::setGeometry(const QRect &rect)
{
    const QSize size = rect.size();
    if (img.size() != size) {
        QWidget *win = window();
        QImage::Format imageFormat = preferredImageFormat(win);
        const int bytesPerPixel = imageFormat == QImage::Format_RGB16 ? 2 : 4;

        const int bpl = (size.width() * bytesPerPixel + 3) & ~3;
        const int imagesize = bpl * size.height();

        if (imagesize == 0) {
            mem.detach();
            img = QImage();
        } else {
            mem.detach();
            if (!mem.create(imagesize)) {
                perror("QWSSharedMemSurface::setGeometry allocating shared memory");
                qFatal("Error creating shared memory of size %d", imagesize);
            }
            uchar *base = static_cast<uchar*>(mem.address());
            img = QImage(base, size.width(), size.height(), imageFormat);
            setImageMetrics(img, win);
        }
    }

    QWSWindowSurface::setGeometry(rect);
}

QByteArray QWSSharedMemSurface::permanentState() const
{
    QByteArray array;
    array.resize(4 * sizeof(int) + sizeof(QImage::Format) +
                 sizeof(SurfaceFlags));

    char *ptr = array.data();

    reinterpret_cast<int*>(ptr)[0] = mem.id();
    reinterpret_cast<int*>(ptr)[1] = img.width();
    reinterpret_cast<int*>(ptr)[2] = img.height();
    reinterpret_cast<int*>(ptr)[3] = (memlock ? memlock->id() : -1);
    ptr += 4 * sizeof(int);

    *reinterpret_cast<QImage::Format*>(ptr) = img.format();
    ptr += sizeof(QImage::Format);

    *reinterpret_cast<SurfaceFlags*>(ptr) = surfaceFlags();

    return array;
}

#endif // QT_NO_QWS_MULTIPROCESS

#ifndef QT_NO_PAINTONSCREEN

QWSOnScreenSurface::QWSOnScreenSurface(QWidget *w)
    : QWSMemorySurface(w)
{
    attachToScreen(getScreen(w));
    setSurfaceFlags(Opaque);
}

QWSOnScreenSurface::QWSOnScreenSurface()
    : QWSMemorySurface()
{
    setSurfaceFlags(Opaque);
}

void QWSOnScreenSurface::attachToScreen(const QScreen *s)
{
    screen = s;
    uchar *base = screen->base();
    QImage::Format format  = screen->pixelFormat();

    if (format == QImage::Format_Invalid || format == QImage::Format_Indexed8) {
        //### currently we have no paint engine for indexed image formats
        qFatal("QWSOnScreenSurface::attachToScreen(): screen depth %d "
               "not implemented", screen->depth());
        return;
    }
    QWSMemorySurface::img = QImage(base, screen->width(), screen->height(),
                                   screen->linestep(), format );
}

QWSOnScreenSurface::~QWSOnScreenSurface()
{
}

QPoint QWSOnScreenSurface::painterOffset() const
{
    return geometry().topLeft() + QWSWindowSurface::painterOffset();
}

bool QWSOnScreenSurface::isValid() const
{
    const QWidget *win = window();
    if (screen != getScreen(win))
        return false;
    if (img.isNull())
        return false;
    return QScreen::isWidgetPaintOnScreen(win);
}

QByteArray QWSOnScreenSurface::permanentState() const
{
    QByteArray array;
    array.resize(sizeof(int));
    int *ptr = reinterpret_cast<int*>(array.data());
    ptr[0] = QApplication::desktop()->screenNumber(window());
    return array;
}

void QWSOnScreenSurface::setPermanentState(const QByteArray &data)
{
    const int *ptr = reinterpret_cast<const int*>(data.constData());
    const int screenNo = ptr[0];

    QScreen *screen = qt_screen;
    if (screenNo > 0)
        screen = qt_screen->subScreens().at(screenNo);
    attachToScreen(screen);
}

#endif // QT_NO_PAINTONSCREEN

#ifndef QT_NO_PAINT_DEBUG

QWSYellowSurface::QWSYellowSurface(bool isClient)
    : QWSWindowSurface(), delay(10)
{
    if (isClient) {
        setWinId(QWidget::qwsDisplay()->takeId());
        QWidget::qwsDisplay()->nameRegion(winId(),
                                          QLatin1String("Debug flush paint"),
                                          QLatin1String("Silly yellow thing"));
        QWidget::qwsDisplay()->setAltitude(winId(), 1, true);
    }
    setSurfaceFlags(Buffered);
}

QWSYellowSurface::~QWSYellowSurface()
{
}

QByteArray QWSYellowSurface::permanentState() const
{
    QByteArray array;
    array.resize(2 * sizeof(int));

    int *ptr = reinterpret_cast<int*>(array.data());
    ptr[0] = surfaceSize.width();
    ptr[1] = surfaceSize.height();

    return array;
}

void QWSYellowSurface::setPermanentState(const QByteArray &data)
{
    const int *ptr = reinterpret_cast<const int*>(data.constData());

    const int width = ptr[0];
    const int height = ptr[1];

    img = QImage(width, height, QImage::Format_ARGB32);
    img.fill(qRgba(255,255,31,127));
}

void QWSYellowSurface::flush(QWidget *widget, const QRegion &region,
                             const QPoint &offset)
{
    Q_UNUSED(offset);

    QWSDisplay *display = QWidget::qwsDisplay();
    QRegion rgn = region;

    if (widget)
        rgn.translate(widget->mapToGlobal(QPoint(0, 0)));

    surfaceSize = region.boundingRect().size();

    const int id = winId();
    display->requestRegion(id, key(), permanentState(), rgn);
    display->setAltitude(id, 1, true);
    display->repaintRegion(id, 0, false, rgn);

    ::usleep(500 * delay);
    display->requestRegion(id, key(), permanentState(), QRegion());
    ::usleep(500 * delay);
}

#endif // QT_NO_PAINT_DEBUG

#ifndef QT_NO_DIRECTPAINTER

static inline QScreen *getPrimaryScreen()
{
    QScreen *screen = QScreen::instance();
    if (!screen->base()) {
        QList<QScreen*> subScreens = screen->subScreens();
        if (subScreens.size() < 1)
            return 0;
        screen = subScreens.at(0);
    }
    return screen;
}

QWSDirectPainterSurface::QWSDirectPainterSurface(bool isClient,
                                                 QDirectPainter::SurfaceFlag flags)
    : QWSWindowSurface(), flushingRegionEvents(false)
{
    setSurfaceFlags(Opaque);
    synchronous = (flags == QDirectPainter::ReservedSynchronous);

    if (isClient) {
        setWinId(QWidget::qwsDisplay()->takeId());
        QWidget::qwsDisplay()->nameRegion(winId(),
                                          QLatin1String("QDirectPainter reserved space"),
                                          QLatin1String("reserved"));
    } else {
        setWinId(0);
    }

    _screen = QScreen::instance();
    if (!_screen->base()) {
        QList<QScreen*> subScreens = _screen->subScreens();
        if (subScreens.size() < 1)
            _screen = 0;
        else
            _screen = subScreens.at(0);
    }
}

QWSDirectPainterSurface::~QWSDirectPainterSurface()
{
    if (winId() && QWSDisplay::instance()) // make sure not in QApplication destructor
        QWidget::qwsDisplay()->destroyRegion(winId());
}

void QWSDirectPainterSurface::setRegion(const QRegion &region)
{
    QRegion reg = region;

    if (_screen->isTransformed()) {
        const QSize devSize(_screen->deviceWidth(), _screen->deviceHeight());
        reg = _screen->mapFromDevice(region, devSize);
    }

    const int id = winId();
    QWidget::qwsDisplay()->requestRegion(id, key(), permanentState(), reg);
#ifndef QT_NO_QWS_MULTIPROCESS
    if (synchronous)
        QWSDisplay::instance()->d->waitForRegionAck(id);
#endif
}

void QWSDirectPainterSurface::flush(QWidget *, const QRegion &r, const QPoint &)
{
    QWSDisplay::instance()->repaintRegion(winId(), 0, true, r);
}

QByteArray QWSDirectPainterSurface::permanentState() const
{
    QByteArray res;
    if (isRegionReserved())
        res.append( 'r');
    return res;
}

void QWSDirectPainterSurface::setPermanentState(const QByteArray &ba)
{
    if (ba.size() > 0 && ba.at(0) == 'r')
        setReserved();
    setSurfaceFlags(surfaceFlags() | Opaque);
}

void QWSDirectPainterSurface::beginPaint(const QRegion &region)
{
    QWSWindowSurface::beginPaint(region);
#ifndef QT_NO_QWS_MULTIPROCESS
    if (!synchronous) {
        flushingRegionEvents = true;
        QWSDisplay::instance()->d->waitForRegionEvents(winId());
        flushingRegionEvents = false;
    }
#endif
}

bool QWSDirectPainterSurface::lock(int timeout)
{
    Q_UNUSED(timeout);
    if (QApplication::type() == QApplication::GuiClient)
        QWSDisplay::grab(true);
    return true;
}

void QWSDirectPainterSurface::unlock()
{
    if (QApplication::type() == QApplication::GuiClient)
        QWSDisplay::ungrab();
}

#endif // QT_NO_DIRECTPAINTER
