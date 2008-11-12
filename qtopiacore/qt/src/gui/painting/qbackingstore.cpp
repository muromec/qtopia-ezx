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
#include <qapplication.h>
#ifdef Q_WS_WIN
# include "qt_windows.h"
# include <private/qpaintengine_raster_p.h>
# ifndef QT_NO_DIRECT3D
#  include <private/qpaintengine_d3d_p.h>
#  include <private/qwindowsurface_d3d_p.h>
# endif
#endif
#include "qbackingstore_p.h"
#include "private/qwidget_p.h"
#include <qdebug.h>
#include <qstack.h>
#include <qevent.h>
#include <qabstractscrollarea.h>
#include "private/qabstractscrollarea_p.h"
#ifdef Q_WS_X11
# include "private/qt_x11_p.h"
#endif

#ifdef Q_WS_QWS
#include <qscreen_qws.h>
#include <qwsdisplay_qws.h>
#include <qapplication.h>
#include <qwsmanager_qws.h>
#include <private/qwsmanager_p.h>
#include <unistd.h>
#endif

#include "qwindowsurface_raster_p.h"
#ifdef Q_WS_X11
#include "qwindowsurface_x11_p.h"
#elif defined(Q_WS_QWS)
#include "qwindowsurface_qws_p.h"
#endif

/*****************************************************************************
  Top Level Window backing store
 *****************************************************************************/

extern bool qt_sendSpontaneousEvent(QObject*, QEvent*); // qapplication_xxx.cpp

#ifndef Q_WS_QWS
static bool qt_enable_backingstore = true;
#endif
#ifdef Q_WS_X11
// for compatibility with Qt 4.0
Q_GUI_EXPORT void qt_x11_set_global_double_buffer(bool enable)
{
    qt_enable_backingstore = enable;
}
#endif

bool QWidgetBackingStore::paintOnScreen(QWidget *w)
{
#if defined(Q_WS_QWS) || defined(Q_WS_MAC)
    Q_UNUSED(w);
    return false;
#elif  defined(QT_NO_BACKINGSTORE)
    Q_UNUSED(w);
    return true;
#else
    if (w && (w->testAttribute(Qt::WA_PaintOnScreen) || !w->isWindow() && w->window()->testAttribute(Qt::WA_PaintOnScreen)))
        return true;

    // sanity check for overlarge toplevels. Better: store at least screen size and move offset.
    if (w && w->isWindow() && (w->width() > 4096  || w->height() > 4096))
        return true;

    static signed char checked_env = -1;
    if(checked_env == -1)
        checked_env = (qgetenv("QT_ONSCREEN_PAINT") == "1") ? 1 : 0;

    return checked_env == 1 || !qt_enable_backingstore;
#endif
}

#ifndef QT_NO_PAINT_DEBUG

#ifdef Q_WS_QWS
static void qt_showYellowThing(QWidget *widget, const QRegion &rgn, int msec, bool)
{
    Q_UNUSED(widget);

    static QWSYellowSurface surface(true);
    surface.setDelay(msec);
    surface.flush(widget, rgn, QPoint());
}

#else
static void qt_showYellowThing(QWidget *widget, const QRegion &toBePainted, int msec, bool unclipped)
{
    //flags to fool painter
    bool paintUnclipped = widget->testAttribute(Qt::WA_PaintUnclipped);
#ifdef Q_WS_WIN
    Q_UNUSED(unclipped);
#else
    if (unclipped && !QWidgetBackingStore::paintOnScreen(widget))
        widget->setAttribute(Qt::WA_PaintUnclipped);
#endif

    bool setFlag = !widget->testAttribute(Qt::WA_WState_InPaintEvent);
    if(setFlag)
        widget->setAttribute(Qt::WA_WState_InPaintEvent);


    static int i = 0;

    //setup the engine
    QPaintEngine *pe = widget->paintEngine();
    if (pe) {
        pe->setSystemClip(toBePainted);
        {
            QPainter p(widget);
            p.setClipRegion(toBePainted);

            switch (i) {
            case 0:
                p.fillRect(widget->rect(), QColor(255,255,0));
                break;
            case 1:
                p.fillRect(widget->rect(), QColor(255,200,55));
                break;
            case 2:
                p.fillRect(widget->rect(), QColor(200,255,55));
                break;
            case 3:
                p.fillRect(widget->rect(), QColor(200,200,0));
                break;
            }
            i = (i+1) & 3;
            p.end();
        }
    }

    if(setFlag)
        widget->setAttribute(Qt::WA_WState_InPaintEvent, false);

    //restore
    widget->setAttribute(Qt::WA_PaintUnclipped, paintUnclipped);

    if (pe) {
        pe->setSystemClip(QRegion());
        //flush
        if (pe->type() == QPaintEngine::Raster) {
            QRasterPaintEngine *rpe = static_cast<QRasterPaintEngine *>(pe);
            rpe->flush(widget, QPoint());
        }
    }

    QApplication::syncX();

#if defined(Q_OS_UNIX)
    ::usleep(1000*msec);
#elif defined(Q_OS_WIN)
    ::Sleep(msec);
#endif

}
#endif

static int test_qt_flushPaint()
{
    static int flush_paint = qgetenv("QT_FLUSH_PAINT").toInt();
    return flush_paint;
}

static bool qt_flushPaint(QWidget *widget, const QRegion &toBePainted)
{
    static int flush_paint = test_qt_flushPaint();
    if (!flush_paint)
        return false;

    qt_showYellowThing(widget, toBePainted, flush_paint * 10, true);

    return true;
}

static void qt_unflushPaint(QWidget *widget, const QRegion &rgn)
{
    if (!QWidgetBackingStore::paintOnScreen(widget))
        QWidgetBackingStore::copyToScreen(widget, rgn);
}

#if !defined(Q_WS_QWS)
static bool qt_flushUpdate(QWidget *widget, const QRegion &rgn)
{
    static int checked_env = -1;
    if(checked_env == -1) {
        checked_env = qgetenv("QT_FLUSH_UPDATE").toInt();
    }

    if (checked_env == 0)
        return false;

    qt_showYellowThing(widget, rgn, checked_env*10, false);

    return true;
}
#endif

#endif // QT_NO_PAINT_DEBUG

void qt_syncBackingStore(QRegion rgn, QWidget *widget, bool recursive)
{
    if (!QWidgetBackingStore::paintOnScreen(widget)) {
        if (QWidgetBackingStore *bs = widget->d_func()->maybeBackingStore())
            bs->cleanRegion(rgn, widget, recursive);
    } else {
        widget->repaint(rgn);
    }
}
void qt_syncBackingStore(QRegion rgn, QWidget *widget)
{
    qt_syncBackingStore(rgn, widget, false);
}

#if !defined(QT_NO_DIRECT3D) && defined(Q_WS_WIN)
extern QDirect3DPaintEngine *qt_d3dEngine();
#endif

QWindowSurface *qt_default_window_surface(QWidget *widget)
{
    QWindowSurface *surface = 0;

#ifdef Q_WS_WIN
#ifndef QT_NO_DIRECT3D
    if (qApp->testAttribute(Qt::AA_MSWindowsUseDirect3DByDefault)
        && (widget->windowOpacity() == 1.0f)
        && qt_d3dEngine()->hasDirect3DSupport())
        surface = new QD3DWindowSurface(widget);
    else
        surface = new QRasterWindowSurface(widget);
#else
    surface = new QRasterWindowSurface(widget);
#endif
#elif defined(Q_WS_X11)
    surface = new QX11WindowSurface(widget);
#elif defined(Q_WS_QWS)
    if (widget->windowType() == Qt::Desktop)
        return 0;
    widget->ensurePolished();
    surface = qt_screen->createSurface(widget);
#else
    Q_UNUSED(widget);
#endif

    // The QWindowSurface constructor will call QWidget::setWindowSurface(),
    // but automatically created surfaces should not be added to the topdata.
#ifdef Q_BACKINGSTORE_SUBSURFACES
    Q_ASSERT(widget->d_func()->topData()->windowSurface == surface);
#endif
    widget->d_func()->topData()->windowSurface = 0;

    return surface;
}

#ifdef Q_WS_WIN

/*
   Used by QETWidget::translatePaintEvent and expects rgn to be in
   windowing system coordinates.
 */

void QWidgetBackingStore::blitToScreen(const QRegion &rgn, QWidget *widget)
{
    QWidget *tlw = widget->window();
    if (!widget->isVisible() || !tlw->testAttribute(Qt::WA_Mapped) || rgn.isEmpty())
        return;

    if (!QWidgetBackingStore::paintOnScreen(widget)) {
        QWidgetBackingStore *bs = tlw->d_func()->topData()->backingStore;

        bs->windowSurface->flush(widget, rgn, widget->mapTo(tlw, QPoint(0, 0)));
    }
}
#endif

#if defined(Q_WS_X11) || (defined(Q_WS_WIN) && defined(Q_WIN_USE_QT_UPDATE_EVENT))
void qt_syncBackingStore(QWidget *widget)
{
    // dirtyOnScreen may get out of sync when widget is scrolled or moved
    widget->d_func()->dirtyOnScreen &= widget->d_func()->clipRect();

    const QRegion dirty =  widget->d_func()->dirtyOnScreen;
    QWidget *tlw = widget->window();
    if (!QWidgetBackingStore::paintOnScreen(widget)) {
        QWidgetBackingStore *bs = tlw->d_func()->topData()->backingStore;
        bs->cleanRegion(dirty, widget);
    } else {
        widget->repaint(dirty);
    }
}
#elif defined(Q_WS_QWS)
void qt_syncBackingStore(QWidget *widget)
{
    QWidget *tlw = widget->window();
    QTLWExtra *topData = tlw->d_func()->topData();

    QWidgetBackingStore *bs = topData->backingStore;
    QWSWindowSurface *surface = 0;
    if(bs)
        surface = static_cast<QWSWindowSurface*>(bs->windowSurface);
    else
        qWarning("request to sync backing store of widget %p, "
                 "which does not have its backing store defined yet",
                 (void*)widget);

    QRegion toClean;
    if (surface)
        toClean = surface->dirtyRegion();

#ifdef Q_BACKINGSTORE_SUBSURFACES
    if (bs) {
        QList<QWindowSurface*> subSurfaces = bs->subSurfaces;
        for (int i = 0; i < subSurfaces.size(); ++i) {
            QWSWindowSurface *s = static_cast<QWSWindowSurface*>(subSurfaces.at(i));
            QPoint offset = s->window()->mapTo(tlw, QPoint());
            toClean += s->dirtyRegion().translated(-offset);
        }
    }
#endif

#ifdef Q_WIDGET_USE_DIRTYLIST
    if (!toClean.isEmpty() || !bs->dirtyWidgets.isEmpty())
#else
    if (!toClean.isEmpty())
#endif
        topData->backingStore->cleanRegion(toClean, tlw);
}
#elif defined(Q_WS_WIN) && !defined(Q_WIN_USE_QT_UPDATE_EVENT)
void qt_syncBackingStore(QWidget *widget)
{
    QWidget *tlw = widget->window();
    QTLWExtra *topData = tlw->d_func()->topData();

    QWidgetBackingStore *bs = topData->backingStore;
    if (!bs)
        return;

    const QRegion toClean = bs->dirty;
#ifdef Q_WIDGET_USE_DIRTYLIST
    if (!toClean.isEmpty() || !bs->dirtyWidgets.isEmpty())
#else
    if (!toClean.isEmpty())
#endif
        topData->backingStore->cleanRegion(toClean, tlw, false);
}
#endif // Q_WS_WIN

/*
   A version of QRect::intersects() that does not normalize the rects.
*/
static inline bool qRectIntersects(const QRect &r1, const QRect &r2)
{
    return (qMax(r1.left(), r2.left()) <= qMin(r1.right(), r2.right()) &&
             qMax(r1.top(), r2.top()) <= qMin(r1.bottom(), r2.bottom()));
}

QWidgetBackingStore::QWidgetBackingStore(QWidget *t) : tlw(t)
{
    windowSurface = tlw->windowSurface();
    if (!windowSurface)
        windowSurface = qt_default_window_surface(t);

#ifdef Q_BACKINGSTORE_SUBSURFACES
    // XXX: hw: workaround to ensure all existing subsurfaces are added to the list
    QList<QObject*> children = t->children();
    for (int i = 0; i < children.size(); ++i) {
        QWidget *child = qobject_cast<QWidget*>(children.at(i));
        if (!child)
            continue;
        QTLWExtra *extra = child->d_func()->maybeTopData();
        if (extra && extra->windowSurface)
            subSurfaces.append(extra->windowSurface);
    }
#endif
}

QWidgetBackingStore::~QWidgetBackingStore()
{
    delete windowSurface;
    windowSurface = 0;
}

/*
  Widget's coordinate system
  move whole rect by dx,dy
  rect must be valid
  doesn't generate any updates
*/
bool QWidgetBackingStore::bltRect(const QRect &rect, int dx, int dy, QWidget *widget)
{
    QPoint pos(widget->mapTo(tlw, rect.topLeft()));

#ifdef Q_WS_QWS
    pos += topLevelOffset();
#endif

    return windowSurface->scroll(QRect(pos, rect.size()), dx, dy);
}


//parent's coordinates; move whole rect; update parent and widget
//assume the screen blt has already been done, so we don't need to refresh that part
void QWidgetPrivate::moveRect(const QRect &rect, int dx, int dy)
{
    Q_Q(QWidget);
    if (!q->isVisible())
        return;

    QWidget *tlw = q->window();
    QTLWExtra* x = tlw->d_func()->topData();


    static int accelEnv = -1;
    if (accelEnv == -1) {
        accelEnv = qgetenv("QT_NO_FAST_MOVE").toInt() == 0;
    }

    QWidget *pw = q->parentWidget();
    QPoint toplevelOffset = pw->mapTo(tlw, QPoint());
    QWidgetPrivate *pd = pw->d_func();
    QRect clipR = pd->clipRect();
#ifdef Q_WS_QWS
    QWidgetBackingStore *wbs = x->backingStore;
    QWSWindowSurface *surface = static_cast<QWSWindowSurface*>(wbs->windowSurface);
    clipR = clipR.intersected(surface->clipRegion().translated(-toplevelOffset).boundingRect());
#endif
    QRect newRect = rect.translated(dx,dy);

    QRect destRect = rect.intersected(clipR);
    if (destRect.isValid())
        destRect = destRect.translated(dx,dy).intersected(clipR);
    QRect sourceRect = destRect.translated(-dx, -dy);

    bool accelerateMove = accelEnv &&  isOpaque() && !isOverlapped(sourceRect)
        && !isOverlapped(destRect);

    if (!accelerateMove) {
        QRegion parentR(rect & clipR);
        if (q->mask().isEmpty()) {
            parentR -= newRect;
        } else {
            // invalidateBuffer() excludes anything outside the mask
            parentR += newRect & clipR;
        }
        pd->invalidateBuffer(parentR);
        invalidateBuffer((newRect & clipR).translated(-data.crect.topLeft()));
    } else {

        QRegion childExpose = newRect & clipR;
        QWidgetBackingStore *wbs = x->backingStore;

        if (sourceRect.isValid()) {
            if (wbs->bltRect(sourceRect, dx, dy, pw))
                childExpose -= destRect;
        }

#ifdef Q_WS_QWS
        QRegion dirty = sourceRect.translated(toplevelOffset);
        if (surface)
            dirty &= surface->dirtyRegion();
        const QRect newDirty = dirty.boundingRect().translated(QPoint(dx,dy) - toplevelOffset);
#else
        QRect newDirty = (wbs->dirty & sourceRect.translated(toplevelOffset)).boundingRect().translated(QPoint(dx,dy) - toplevelOffset);
#endif
        childExpose += newDirty;

        childExpose.translate(-data.crect.topLeft());
        invalidateBuffer(childExpose);

        QRegion parentExpose = rect & clipR;
        parentExpose -= newRect;
        if (!q->mask().isEmpty()) {
            parentExpose += QRegion(newRect) - q->mask().translated(data.crect.topLeft());
        }
        pd->invalidateBuffer(parentExpose);
#ifdef Q_WS_QWS
        //QWS does not have native child widgets: copy everything to screen, just like scrollRect()
//        pd->dirtyWidget_sys(QRegion(sourceRect)+destRect);

        wbs->dirtyOnScreen += sourceRect.translated(toplevelOffset);
        wbs->dirtyOnScreen += destRect.translated(toplevelOffset);
#endif
    }
}

//widget's coordinates; scroll within rect;  only update widget
void QWidgetPrivate::scrollRect(const QRect &rect, int dx, int dy)
{
    Q_Q(QWidget);
    QWidget *tlw = q->window();
    QTLWExtra* x = tlw->d_func()->topData();
    QWidgetBackingStore *wbs = x->backingStore;

    static int accelEnv = -1;
    if (accelEnv == -1) {
        accelEnv = qgetenv("QT_NO_FAST_SCROLL").toInt() == 0;
    }

    bool accelerateScroll = accelEnv &&  isOpaque()  && !isOverlapped(data.crect);

#if defined(Q_WS_QWS)
    QWSWindowSurface *surface;
    surface = static_cast<QWSWindowSurface*>(wbs->windowSurface);

    if (accelerateScroll && !surface->isBuffered()) {
        const QRegion surfaceClip = surface->clipRegion();
        const QRegion outsideClip = QRegion(rect) - surfaceClip;
        if (!outsideClip.isEmpty()) {
            const QVector<QRect> clipped = (surfaceClip & rect).rects();
            if (clipped.size() < 8) {
                for (int i = 0; i < clipped.size(); ++i)
                    scrollRect(clipped.at(i), dx, dy);
                return;
            } else {
                accelerateScroll = false;
            }
        }
    }
#endif // Q_WS_QWS

    if (!accelerateScroll) {
        invalidateBuffer(rect);
    } else {
        const QPoint toplevelOffset = q->mapTo(tlw, QPoint());
#ifdef Q_WS_QWS
        QWSWindowSurface *surface = static_cast<QWSWindowSurface*>(wbs->windowSurface);
        const QRegion clip = surface->clipRegion().translated(-toplevelOffset)
                             & clipRect();
        const QRect scrollRect = rect & clip.boundingRect();
        const QRect destRect = scrollRect.translated(dx, dy)
                               & scrollRect
                               &  clip.boundingRect();
#else
        QRect scrollRect = rect & clipRect();

        QRect destRect = scrollRect.isValid() ? scrollRect.translated(dx,dy).intersected(scrollRect) : QRect();

#endif
        QRect sourceRect = destRect.translated(-dx, -dy);

        QRegion childExpose = scrollRect;
        if (sourceRect.isValid()) {
            if (wbs->bltRect(sourceRect, dx, dy, q))
                childExpose -= destRect;
        }

//        childExpose += (wbs->dirty & sourceRect.translated(toplevelOffset)).boundingRect().translated(QPoint(dx,dy) - toplevelOffset);
#ifdef Q_WS_QWS
        QRegion dirty = sourceRect.translated(toplevelOffset);
        if (surface)
            dirty &= surface->dirtyRegion();
        const QRect newDirty = dirty.boundingRect().translated(QPoint(dx,dy) - toplevelOffset);
#else
        QRect newDirty = (wbs->dirty & sourceRect.translated(toplevelOffset)).boundingRect().translated(QPoint(dx,dy) - toplevelOffset);
#endif
//         qDebug() << "scrollRect" << q << rect << dx << dy << "dirty" << wbs->dirty << "newDirty" << newDirty;
        childExpose += newDirty;
        invalidateBuffer(childExpose);

        // Instead of using native scroll-on-screen, we copy from
        // backingstore, giving only one screen update for each
        // scroll, and a solid appearance
#ifdef Q_WS_QWS
        wbs->dirtyOnScreen += destRect.translated(toplevelOffset);
#else
        dirtyWidget_sys(rect);
#endif
    }
}

void QWidgetBackingStore::dirtyRegion(const QRegion &rgn, QWidget *widget)
{
    QRegion wrgn(rgn);
    Q_ASSERT(widget->window() == tlw);
    if(!widget->isVisible() || !widget->updatesEnabled())
        return;
    wrgn &= widget->d_func()->clipRect();
    if (!widget->mask().isEmpty())
        wrgn &= widget->mask();

#ifndef Q_WS_QWS
#ifdef Q_RATE_LIMIT_PAINTING
    if (refreshInterval > 0) {
        QWidget *timerWidget = widget;
#ifdef Q_FLATTEN_EXPOSE
        if (timerWidget != tlw) {
            timerWidget = tlw;
            wrgn.translate(widget->mapTo(tlw, QPoint(0, 0)));
        }
#endif
        timerWidget->d_func()->dirty += wrgn;
        if (timerWidget->d_func()->timerId == -1)
            timerWidget->d_func()->timerId = timerWidget->startTimer(refreshInterval);
    } else {
        widget->d_func()->dirtyWidget_sys(wrgn);
    }
#else
    widget->d_func()->dirtyWidget_sys(wrgn);
#endif // Q_RATE_LIMIT_PAINTING
#endif
#if defined(Q_RATE_LIMIT_PAINTING) && defined(Q_FLATTEN_EXPOSE)
    // wrgn is already translated otherwise
    if (refreshInterval <= 0)
#endif
    wrgn.translate(widget->mapTo(tlw, QPoint(0, 0)));
#ifndef Q_WS_QWS
    dirty += wrgn;
#endif
#ifdef Q_WS_QWS
    tlw->d_func()->dirtyWidget_sys(wrgn); //optimization: don't translate twice
#endif
}

#ifdef Q_RATE_LIMIT_PAINTING
int QWidgetBackingStore::refreshInterval = 30;

Q_GUI_EXPORT void qt_setMinimumRefreshInterval(int ms)
{
    QWidgetBackingStore::refreshInterval = ms >= 0 ? ms : 0;
}

void QWidgetBackingStore::updateDirtyRegion(QWidget *widget)
{
    if (!widget || widget->d_func()->dirty.isEmpty())
        return;

    if (widget->isVisible() && widget->updatesEnabled())
        widget->d_func()->dirtyWidget_sys(widget->d_func()->dirty);
    widget->d_func()->dirty = QRegion();
}
#endif // Q_RATE_LIMIT_PAINTING


void QWidgetBackingStore::copyToScreen(QWidget *widget, const QRegion &rgn)
{
    QWidget *tlw = widget->window();
    QTLWExtra *topextra = tlw->d_func()->extra->topextra;
    QPoint offset = widget->mapTo(tlw, QPoint());
    topextra->backingStore->copyToScreen(rgn, widget, offset, false);
}

/**
 * Copies the contents of the backingstore into the screen area of \a widget.
 * \a offset is the position of \a widget relative to the top level widget.
 * \a rgn is the region to be updated in \a widget coordinates.
 * \a recursive indicates that the widget should recursivly call copyToScreen
 * for all child widgets.
 */
void QWidgetBackingStore::copyToScreen(const QRegion &rgn, QWidget *widget, const QPoint &offset, bool recursive)
{
    if (rgn.isEmpty())
        return;
    Q_ASSERT(widget->testAttribute(Qt::WA_WState_Created));
#ifdef Q_WS_QWS
    Q_UNUSED(recursive);
     // XXX: hw: this addition should probably be moved to cleanRegion()
    const QRegion toFlush = rgn + dirtyOnScreen;
    windowSurface->flush(widget, toFlush, offset);
    dirtyOnScreen = QRegion();
#else
    if (!QWidgetBackingStore::paintOnScreen(widget)) {
        widget->d_func()->cleanWidget_sys(rgn);

#if defined(Q_WS_WIN) && defined(Q_WIN_USE_QT_UPDATE_EVENT)
        HDC oldHandle = HDC(widget->d_func()->hd);
        if (oldHandle) {
            // New clip is the widget's visible region intersected with the flush region.
            widget->d_func()->hd = Qt::HANDLE(GetDCEx(widget->internalWinId(), rgn.handle(),
                                                      DCX_INTERSECTRGN));
        }
#endif

#ifndef QT_NO_PAINT_DEBUG
        qt_flushUpdate(widget, rgn);
#endif

        QPoint wOffset = widget->data->wrect.topLeft();
        windowSurface->flush(widget, rgn, offset);

#if defined(Q_WS_WIN) && defined(Q_WIN_USE_QT_UPDATE_EVENT)
        // Reset device context.
        if (oldHandle) {
            ReleaseDC(widget->internalWinId(), HDC(widget->d_func()->hd));
            widget->d_func()->hd = Qt::HANDLE(oldHandle);
        }
#endif
    }

#ifdef Q_FLATTEN_EXPOSE
    Q_ASSERT(!recursive);
    // Q_ASSERT(widget->isWindow());
#endif

    if(recursive) {
        const QObjectList children = widget->children();
        for(int i = 0; i < children.size(); ++i) {
            if(QWidget *child = qobject_cast<QWidget*>(children.at(i))) {
                if(!child->isWindow() && child->isVisible()) {
                    if (qRectIntersects(rgn.boundingRect().translated(-child->pos()), child->rect())) {
                        QRegion childRegion(rgn);
                        childRegion.translate(-child->pos());
                        childRegion &= child->d_func()->clipRect();
                        if(!childRegion.isEmpty())
                            copyToScreen(childRegion, child, offset+child->pos(), recursive);
                    }
                }
            }
        }
    }
#endif
}



#ifdef Q_BACKINGSTORE_SUBSURFACES

void QWidgetBackingStore::cleanRegion(const QRegion &rgn, QWidget *widget, bool recursiveCopyToScreen)
{
    if (!widget->isVisible() || !widget->updatesEnabled() || !tlw->testAttribute(Qt::WA_Mapped))
        return;

    if(QWidgetBackingStore::paintOnScreen(widget))
        return;

    QRegion toClean;
#if defined(Q_WS_QWS)
    QRect tlwRect = tlw->frameGeometry();
#else
    QRect tlwRect = tlw->geometry();
#endif

#ifdef Q_WS_QWS
    QList<QWindowSurface*> surfaces = subSurfaces;
    surfaces.prepend(windowSurface);

    for (int i = 0; i < surfaces.size(); ++i) {
        QWindowSurface *windowSurface = surfaces.at(i);
        QWidget *currWidget = windowSurface->window();
        QRect tlwRect;
        if (currWidget->isWindow()) {
            QWSWindowSurface *s = static_cast<QWSWindowSurface*>(windowSurface);
            tlwRect = QRect(currWidget->mapToGlobal(-s->painterOffset()),
                            currWidget->frameGeometry().size());
        } else {
            // XXX: hw: currently subsurfaces are resized in setGeometry_sys
            tlwRect = windowSurface->geometry();
        }
#endif
#ifdef Q_WS_QWS
        if (!static_cast<QWSWindowSurface*>(windowSurface)->isValid()) {
            // this looks strange but it really just releases the surface
            windowSurface->setGeometry(QRect());
            // the old window surface is deleted in setWindowSurface, which is
            // called from QWindowSurface constructor
            windowSurface = qt_default_window_surface(tlw);
        }
#endif
        if (windowSurface->geometry() != tlwRect) {
            if (windowSurface->geometry().size() != tlwRect.size()) {
                toClean = QRect(QPoint(0, 0), tlwRect.size());
                recursiveCopyToScreen = true;
#ifdef Q_WIDGET_USE_DIRTYLIST
                for (int i = 0; i < dirtyWidgets.size(); ++i)
                    dirtyWidgets.at(i)->d_func()->dirty = QRegion();
                dirtyWidgets.clear();
#endif
            }
            windowSurface->setGeometry(tlwRect);
        } else {
#ifdef Q_WS_QWS
            toClean = static_cast<QWSWindowSurface*>(windowSurface)->dirtyRegion();
#else
            toClean = dirty;
#endif
        }
#ifdef Q_WS_QWS
        const QPoint painterOffset = static_cast<QWSWindowSurface*>(windowSurface)->painterOffset();
        if (currWidget->isWindow())
            tlwOffset = painterOffset;
#else
        const QPoint painterOffset(0, 0);
#endif
        // ### move into prerender step

        QRegion toFlush = toClean;
#ifdef Q_WIDGET_USE_DIRTYLIST
        if (!toClean.isEmpty() || !dirtyWidgets.isEmpty()) {
#else
        if (!toClean.isEmpty()) {
#endif
#ifndef Q_WS_QWS
            dirty -= toClean;
#endif
            if (tlw->updatesEnabled()) {
                // Pre render config
                windowSurface->paintDevice()->paintEngine()->setSystemClip(toClean);

                // hw: XXX the toClean region is not correct if !dirtyWidgets.isEmpty()


// Avoid deadlock with QT_FLUSH_PAINT: the server will wait for
// the BackingStore lock, so if we hold that, the server will
// never release the Communication lock that we are waiting for in
// sendSynchronousCommand
#ifndef QT_NO_PAINT_DEBUG
                const bool flushing = (test_qt_flushPaint() > 0);
#else
                const bool flushing = false;
#endif
                if (!flushing)
                    windowSurface->beginPaint(toClean);
                windowSurface->paintDevice()->paintEngine()->setSystemClip(QRegion());

#ifdef Q_WIDGET_USE_DIRTYLIST
                for (int i = 0; i < dirtyWidgets.size(); ++i) {
                    QWidget *w = dirtyWidgets.at(i);
                    const QPoint offset = w->mapTo(tlw, QPoint());
                    const QRegion dirty = w->d_func()->dirty;
                    w->d_func()->drawWidget(w->windowSurface()->paintDevice(), dirty,
                                            painterOffset + offset, 0);
                    toFlush += dirty.translated(offset);
                    w->d_func()->dirty = QRegion();
                }
                dirtyWidgets.clear();
#endif // Q_WIDGET_USE_DIRTYLIST

                if (!toClean.isEmpty()) {
                    currWidget->d_func()->drawWidget(windowSurface->paintDevice(),
                                                     toClean, painterOffset);
                }

                // Drawing the overlay...
                windowSurface->paintDevice()->paintEngine()->setSystemClip(toClean);
                if (!flushing)
                    windowSurface->endPaint(toClean);
                windowSurface->paintDevice()->paintEngine()->setSystemClip(QRegion());
            }
        }

#ifdef Q_WS_QWS
        if (toFlush.isEmpty())
            continue;
        // XXX: hack to make copyToScreen work on the correct windowsurface
        QWindowSurface *oldSurface = this->windowSurface;
        this->windowSurface = windowSurface;
#endif
#if 0
        if (recursiveCopyToScreen) {
            toFlush.translate(widget->mapTo(tlw, QPoint()));
#ifdef Q_WS_QWS
            toFlush.translate(currWidget->mapFrom(tlw, QPoint()));
            copyToScreen(toFlush, currWidget, tlwOffset, recursiveCopyToScreen);
#else
            copyToScreen(toFlush, tlw, tlwOffset, recursiveCopyToScreen);
#endif
        } else {
#ifdef Q_WS_X11
            toFlush += widget->d_func()->dirtyOnScreen;
#endif
#ifdef Q_WS_QWS
            copyToScreen(toFlush, currWidget,
                         currWidget->mapFrom(tlw, widget->mapTo(tlw, QPoint())),
                         false);
#else
            copyToScreen(toFlush, widget, widget->mapTo(tlw, QPoint()), false);
#endif
        }
#else // XXX
        copyToScreen(toFlush & static_cast<QWSWindowSurface*>(windowSurface)->clipRegion(), currWidget, tlwOffset, recursiveCopyToScreen);
#endif
#ifdef Q_WS_QWS
        this->windowSurface = oldSurface;
#endif
    }
}

#else // Q_BACKINGSTORE_SUBSURFACES
void QWidgetBackingStore::cleanRegion(const QRegion &rgn, QWidget *widget, bool recursiveCopyToScreen)
{
    if (!widget->isVisible() || !widget->updatesEnabled() || !tlw->testAttribute(Qt::WA_Mapped))
        return;

    if(QWidgetBackingStore::paintOnScreen(widget))
        return;

    QRegion toClean;
#if defined(Q_WS_QWS)
    QRect tlwRect = tlw->frameGeometry();
#else
    QRect tlwRect = tlw->geometry();
#endif

#ifdef Q_WS_QWS
    if (!static_cast<QWSWindowSurface*>(windowSurface)->isValid()) {
        // this looks strange but it really just releases the surface
        windowSurface->setGeometry(QRect());
        // the old window surface is deleted in setWindowSurface, which is
        // called from QWindowSurface constructor
        windowSurface = qt_default_window_surface(tlw);
    }
    toClean = static_cast<QWSWindowSurface*>(windowSurface)->dirtyRegion();
#else
    toClean = dirty;
#endif

    if (windowSurface->geometry() != tlwRect) {
        if (windowSurface->geometry().size() != tlwRect.size()) {
            toClean = QRect(QPoint(0, 0), tlwRect.size());
            recursiveCopyToScreen = true;
#ifdef Q_WIDGET_USE_DIRTYLIST
            for (int i = 0; i < dirtyWidgets.size(); ++i)
                dirtyWidgets.at(i)->d_func()->dirty = QRegion();
            dirtyWidgets.clear();
#endif
        }
        windowSurface->setGeometry(tlwRect);
    }
#ifdef Q_WS_QWS
    tlwOffset = static_cast<QWSWindowSurface*>(windowSurface)->painterOffset();
#endif
    // ### move into prerender step

    QRegion toFlush = rgn;

#ifdef Q_WIDGET_USE_DIRTYLIST
    if (!toClean.isEmpty() || !dirtyWidgets.isEmpty()) {
#else
    if (!toClean.isEmpty()) {
#endif
#ifndef Q_WS_QWS
        dirty -= toClean;
#endif
        if (tlw->updatesEnabled()) {

            // hw: XXX the toClean region is not correct if !dirtyWidgets.isEmpty()

            // Pre render config
            windowSurface->paintDevice()->paintEngine()->setSystemClip(toClean);

// Avoid deadlock with QT_FLUSH_PAINT: the server will wait for
// the BackingStore lock, so if we hold that, the server will
// never release the Communication lock that we are waiting for in
// sendSynchronousCommand
#ifndef QT_NO_PAINT_DEBUG
            const bool flushing = (test_qt_flushPaint() > 0);
#else
            const bool flushing = false;
#endif
            if (!flushing)
                windowSurface->beginPaint(toClean);
            windowSurface->paintDevice()->paintEngine()->setSystemClip(QRegion());

#ifdef Q_WIDGET_USE_DIRTYLIST
#ifdef Q_WS_QWS
            const QPoint poffset = static_cast<QWSWindowSurface*>(windowSurface)->painterOffset();
#else
            const QPoint poffset(0,0);
#endif
            for (int i = 0; i < dirtyWidgets.size(); ++i) {
                QWidget *w = dirtyWidgets.at(i);
                const QPoint offset = w->mapTo(tlw, QPoint());
                const QRegion dirty = w->d_func()->dirty;
                w->d_func()->drawWidget(windowSurface->paintDevice(), dirty,
                                        poffset + offset, 0);
                toFlush += dirty.translated(offset);
                w->d_func()->dirty = QRegion();
            }
            dirtyWidgets.clear();
#endif // Q_WIDGET_USE_DIRTYLIST

            if (!toClean.isEmpty())
                tlw->d_func()->drawWidget(windowSurface->paintDevice(),
                                          toClean, tlwOffset);

            // Drawing the overlay...
            windowSurface->paintDevice()->paintEngine()->setSystemClip(toClean);
            if (!flushing)
                windowSurface->endPaint(toClean);
            windowSurface->paintDevice()->paintEngine()->setSystemClip(QRegion());
        }
    }

#ifdef Q_FLATTEN_EXPOSE
    // Q_ASSERT(widget->isWindow());
    recursiveCopyToScreen = false;
#endif
    if (recursiveCopyToScreen) {
        toFlush.translate(widget->mapTo(tlw, QPoint()));
        copyToScreen(toFlush, tlw, tlwOffset, recursiveCopyToScreen);
    } else {
#if defined(Q_WS_X11) || (defined(Q_WS_WIN) && defined(Q_WIN_USE_QT_UPDATE_EVENT))
        toFlush += widget->d_func()->dirtyOnScreen;
#endif
        copyToScreen(toFlush, widget, widget->mapTo(tlw, QPoint()), false);
    }
}

#endif // Q_BACKINGSTORE_SUBSURFACES

#ifdef Q_WS_QWS
void QWidgetBackingStore::releaseBuffer()
{
    if (windowSurface)
        windowSurface->setGeometry(QRect());
#ifdef Q_BACKINGSTORE_SUBSURFACES
    for (int i = 0; i < subSurfaces.size(); ++i)
        subSurfaces.at(i)->setGeometry(QRect());
#endif
}
#elif defined(Q_WS_WIN)
void QWidgetBackingStore::releaseBuffer()
{
    windowSurface->setGeometry(QRect());
}
#endif

bool QWidgetBackingStore::isOpaque(const QWidget *widget)
{
    return widget->d_func()->isOpaque();
}


void QWidgetBackingStore::paintSiblingsRecursive(QPaintDevice *pdev, const QObjectList& siblings, int index, const QRegion &rgn,
                                                 const QPoint &offset, int flags
#ifdef Q_BACKINGSTORE_SUBSURFACES
                                                 , const QWindowSurface *currentSurface
#endif
    )
{
    QWidget *w = 0;

    do {
        QWidget *x =  qobject_cast<QWidget*>(siblings.at(index));
        if (x && !x->isWindow() && !x->isHidden() && qRectIntersects(rgn.boundingRect(), x->geometry())) {
#ifdef Q_BACKINGSTORE_SUBSURFACES
            if (x->windowSurface() == currentSurface)
#endif
            {
                w = x;
                break;
            }
        }
        --index;
    } while (index >= 0);


    if (!w)
        return;

    QWExtra *extra = w->d_func()->extraData();

    if (index > 0) {
        QRegion wr = rgn;
        if (isOpaque(w)) {
            if(!extra || extra->mask.isEmpty()) {
                wr -= w->geometry();
            } else {
                wr -= extra->mask.translated(w->pos());
            }
        }
        paintSiblingsRecursive(pdev, siblings, index - 1, wr, offset, flags
#ifdef Q_BACKINGSTORE_SUBSURFACES
                               , currentSurface
#endif
            );
    }
    if(w->updatesEnabled()) {
        QRegion wRegion(rgn & w->geometry());
        wRegion.translate(-w->pos());

        if(extra && !extra->mask.isEmpty())
            wRegion &= extra->mask;
        if(!wRegion.isEmpty())
            w->d_func()->drawWidget(pdev, wRegion, offset+w->pos(), flags);
    }
}

#ifdef Q_WIDGET_USE_DIRTYLIST
void QWidgetBackingStore::removeDirtyWidget(QWidget *w)
{
    if (!w->d_func()->dirty.isEmpty()) {
        dirtyWidgets.removeAll(w);
        w->d_func()->dirty = QRegion();
    }

    const int n = w->children().count();
    for (int i = 0; i < n; ++i) {
        if (QWidget *child = qobject_cast<QWidget*>(w->children().at(i)))
            removeDirtyWidget(child);
    }
}
#endif // Q_WIDGET_USE_DIRTYLIST

void QWidgetPrivate::drawWidget(QPaintDevice *pdev, const QRegion &rgn, const QPoint &offset, int flags)
{
    Q_Q(QWidget);
    if (rgn.isEmpty())
        return;

    const bool asRoot = flags & DrawAsRoot;
    const bool alsoOnScreen = flags & DrawPaintOnScreen;
    const bool recursive = flags & DrawRecursive;
    const bool alsoInvisible = flags & DrawInvisible;

    QRegion toBePainted = rgn;
    if (asRoot && !alsoInvisible)
        toBePainted &= clipRect(); //(rgn & visibleRegion());
#ifndef Q_FLATTEN_EXPOSE
    if (!(flags & DontSubtractOpaqueChildren))
        subtractOpaqueChildren(toBePainted, q->rect(), QPoint());
#endif

    if (!toBePainted.isEmpty()) {
        bool onScreen = QWidgetBackingStore::paintOnScreen(q);
        if (!onScreen || alsoOnScreen) {
            //update the "in paint event" flag
            if (q->testAttribute(Qt::WA_WState_InPaintEvent))
                qWarning("QWidget::repaint: Recursive repaint detected");
            q->setAttribute(Qt::WA_WState_InPaintEvent);

            //clip away the new area
#ifndef QT_NO_PAINT_DEBUG
            bool flushed = qt_flushPaint(q, toBePainted);
#endif
            QPaintEngine *paintEngine = pdev->paintEngine();
            if (paintEngine) {
                QPainter::setRedirected(q, pdev, -offset);

                QRegion wrgn = toBePainted;
                wrgn.translate(offset);
                paintEngine->setSystemRect(q->data->crect);
                paintEngine->setSystemClip(wrgn);

                //paint the background
                if ((asRoot || q->autoFillBackground() || onScreen || q->testAttribute(Qt::WA_StyledBackground))
                    && !q->testAttribute(Qt::WA_OpaquePaintEvent)
                    && !q->testAttribute(Qt::WA_NoSystemBackground)) {

                    QPainter p(q);
                    QRect backgroundRect = toBePainted.boundingRect();

#ifndef QT_NO_SCROLLAREA
                    if (QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea *>(q->parent())) {
                        if (scrollArea->viewport() == q) {
                            QAbstractScrollAreaPrivate *priv = static_cast<QAbstractScrollAreaPrivate *>(static_cast<QWidget *>(scrollArea)->d_ptr);
                            const QPoint offset = priv->contentsOffset();
                            p.translate(-offset);
                            backgroundRect.translate(offset);
                        }
                    }
#endif // QT_NO_SCROLLAREA
                    paintBackground(&p, backgroundRect, asRoot || onScreen);
                }
                if (q->testAttribute(Qt::WA_TintedBackground)
                    && !onScreen && !asRoot && !isOpaque() ) {
                    QPainter p(q);
                    QColor tint = q->palette().window().color();
                    tint.setAlphaF(.6);
                    p.fillRect(toBePainted.boundingRect(), tint);
                }
            }

#if 0
            qDebug() << "painting" << q << "opaque ==" << isOpaque();
            qDebug() << "clipping to" << toBePainted << "location == " << offset
                     << "geometry ==" << QRect(q->mapTo(q->window(), QPoint(0, 0)), q->size());
#endif

            //actually send the paint event
            QPaintEvent e(toBePainted);
            qt_sendSpontaneousEvent(q, &e);

            //restore
            if (paintEngine) {
                paintEngine->setSystemRect(QRect());
                pdev->paintEngine()->setSystemClip(QRegion());
                QPainter::restoreRedirected(q);
            }
            q->setAttribute(Qt::WA_WState_InPaintEvent, false);
            if(!q->testAttribute(Qt::WA_PaintOutsidePaintEvent) && q->paintingActive())
                qWarning("QWidget::repaint: It is dangerous to leave painters active on a widget outside of the PaintEvent");

#ifndef QT_NO_PAINT_DEBUG
            if (flushed)
                qt_unflushPaint(q, toBePainted);
#endif
        } else if(q->isWindow()) {
            if (pdev->paintEngine()) {
                QPainter p(pdev);
                p.setClipRegion(toBePainted);
                const QBrush bg = q->palette().brush(QPalette::Window);
                if (bg.style() == Qt::TexturePattern)
                    p.drawTiledPixmap(q->rect(), bg.texture());
                else
                    p.fillRect(q->rect(), bg);
            }
        }
    }

    if (recursive) {
        const QObjectList children = q->children();
        if (!children.isEmpty()) {
            QWidgetBackingStore::paintSiblingsRecursive(pdev, children, children.size()-1, rgn, offset, flags & ~DrawAsRoot
#ifdef Q_BACKINGSTORE_SUBSURFACES
                                                        , q->windowSurface()
#endif
                );
        }
    }
}

/* cross-platform QWidget code */

void QWidgetPrivate::invalidateBuffer(const QRegion &rgn)
{
    if(qApp && qApp->closingDown())
        return;
    Q_Q(QWidget);
    if (QWidgetBackingStore *bs = maybeBackingStore())
        bs->dirtyRegion(rgn, q);
}

void QWidget::repaint(const QRegion& rgn)
{
    if (testAttribute(Qt::WA_WState_ConfigPending)) {
        update(rgn);
        return;
    }

    if (!isVisible() || !updatesEnabled() || rgn.isEmpty())
        return;
    Q_D(QWidget);
    Q_ASSERT(testAttribute(Qt::WA_WState_Created));
//    qDebug() << "repaint" << this << rgn;
    if (!QWidgetBackingStore::paintOnScreen(this)) {
        if (QWidgetBackingStore *bs = d->maybeBackingStore()) {
            QRegion wrgn(rgn);
            d->subtractOpaqueSiblings(wrgn, QPoint());
            d->subtractOpaqueChildren(wrgn, rect(), QPoint());
            bs->dirtyRegion(wrgn, this);
            bs->cleanRegion(wrgn, this);
        }
    }
#ifndef Q_WS_QWS
// QWS does paint-on-screen in qscreen_qws.cpp
    else {
        d->cleanWidget_sys(rgn);
        //     qDebug() << "QWidget::repaint paintOnScreen" << this << "region" << rgn;
#ifndef QT_NO_PAINT_DEBUG
        qt_flushPaint(this, rgn);
#endif

        QPaintEngine *engine = paintEngine();

        QRegion systemClipRgn(rgn);

        if (engine) {
            if (!data->wrect.topLeft().isNull()) {
                QPainter::setRedirected(this, this, data->wrect.topLeft());
                systemClipRgn.translate(-data->wrect.topLeft());
            }
            engine->setSystemClip(systemClipRgn);
            engine->setSystemRect(data->crect);
        }

        d->drawWidget(this, rgn, QPoint(), QWidgetPrivate::DrawAsRoot | QWidgetPrivate::DrawPaintOnScreen);

#ifdef Q_WS_WIN
        if (engine && engine->type() == QPaintEngine::Raster) {
            bool tmp_dc = !d->hd;
            if (tmp_dc)
                d->hd = GetDC(winId());
            static_cast<QRasterPaintEngine *>(engine)->flush(this, QPoint(0, 0));
            if (tmp_dc) {
                ReleaseDC(winId(), (HDC)d->hd);
                d->hd = 0;
            }
        }
#endif
        if (engine) {
            if (!data->wrect.topLeft().isNull())
                QPainter::restoreRedirected(this);
            engine->setSystemClip(QRegion());
            engine->setSystemRect(QRect());
        }

        if(!testAttribute(Qt::WA_PaintOutsidePaintEvent) && paintingActive())
            qWarning("QWidget::repaint: It is dangerous to leave painters active on a widget outside of the PaintEvent");
    }
#endif //Q_WS_QWS
}

void QWidget::update()
{
    if(!isVisible() || !updatesEnabled())
        return;
    QWidgetBackingStore::updateWidget(this, rect());
}

void QWidget::update(const QRect &r)
{
    if(!isVisible() || !updatesEnabled() || r.isEmpty())
        return;
    QWidgetBackingStore::updateWidget(this, r);
}

void QWidget::update(const QRegion& rgn)
{
    if(!isVisible() || !updatesEnabled() || rgn.isEmpty())
        return;
    QWidgetBackingStore::updateWidget(this, rgn);
}

void QWidgetBackingStore::updateWidget(QWidget *that, const QRegion &rgn)
{
    QWidgetPrivate * const d = that->d_func();
    if (that->testAttribute(Qt::WA_WState_InPaintEvent)) {
        QApplication::postEvent(that, new QUpdateLaterEvent(rgn));
        return;
    }

    QWidgetBackingStore *bs = d->maybeBackingStore();
    if (!bs)
        return;

    QRegion wrgn = rgn & d->clipRect();
#ifndef Q_FLATTEN_EXPOSE
    d->subtractOpaqueSiblings(wrgn, QPoint());
    d->subtractOpaqueChildren(wrgn, that->rect(), QPoint());
#endif

    if (wrgn.isEmpty())
        return;

#ifdef Q_WIDGET_USE_DIRTYLIST
    if (qt_region_strictContains(d->dirty, wrgn.boundingRect()))
        return; // already dirty

    if (d->isOpaque()) {
        // TODO: overlapping non-opaque siblings
        if (bs->dirtyWidgets.isEmpty())
            QApplication::postEvent(that->window(), new QEvent(QEvent::UpdateRequest), Qt::LowEventPriority);
        if (d->dirty.isEmpty())
            bs->dirtyWidgets.append(that);
        d->dirty += wrgn;
        return;
    }
#endif

    bs->dirtyRegion(wrgn, that);
}

