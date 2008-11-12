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

#include "qvariant.h"
#include "qdesktopwidget.h"
#include "qapplication.h"
#include "qt_x11_p.h"
#include "qx11info_x11.h"
#include "qwidget_p.h"
#include <limits.h>

// defined in qwidget_x11.cpp
extern int qt_x11_create_desktop_on_screen;

// function to update the workarea of the screen
static bool qt_desktopwidget_workarea_dirty = true;
void qt_desktopwidget_update_workarea()
{
    qt_desktopwidget_workarea_dirty = true;
}


class QSingleDesktopWidget : public QWidget
{
public:
    QSingleDesktopWidget();
    ~QSingleDesktopWidget();
};

QSingleDesktopWidget::QSingleDesktopWidget()
    : QWidget(0, Qt::Desktop)
{
}

QSingleDesktopWidget::~QSingleDesktopWidget()
{
    const QObjectList &childList = children();
    for (int i = childList.size(); i > 0 ;) {
        --i;
        childList.at(i)->setParent(0);
    }
}


class QDesktopWidgetPrivate : public QWidgetPrivate
{
public:
    QDesktopWidgetPrivate();
    ~QDesktopWidgetPrivate();

    void init();

    bool use_xinerama;
    int defaultScreen;
    int screenCount;

    QWidget **screens;
    QRect *rects;
    QRect *workareas;
};

QDesktopWidgetPrivate::QDesktopWidgetPrivate()
    : use_xinerama(false), defaultScreen(0), screenCount(1),
      screens(0), rects(0), workareas(0)
{
}

QDesktopWidgetPrivate::~QDesktopWidgetPrivate()
{
    if (screens) {
        for (int i = 0; i < screenCount; ++i) {
            if (i == defaultScreen) continue;
            delete screens[i];
            screens[i] = 0;
        }

        delete [] screens;
    }

    if (rects)     delete [] rects;
    if (workareas) delete [] workareas;
}

void QDesktopWidgetPrivate::init()
{
    // get the screen count
#ifndef QT_NO_XINERAMA
    XineramaScreenInfo *xinerama_screeninfo = 0;
    int unused;
    use_xinerama = (XineramaQueryExtension(X11->display, &unused, &unused) && XineramaIsActive(X11->display));

    if (use_xinerama) {
        xinerama_screeninfo =
            XineramaQueryScreens(X11->display, &screenCount);
        defaultScreen = 0;
    } else
#endif // QT_NO_XINERAMA
    {
        defaultScreen = DefaultScreen(X11->display);
        screenCount = ScreenCount(X11->display);
    }

    delete [] rects;
    rects     = new QRect[screenCount];
    delete [] workareas;
    workareas = new QRect[screenCount];

    // get the geometry of each screen
    int i, x, y, w, h;
    for (i = 0; i < screenCount; i++) {

#ifndef QT_NO_XINERAMA
        if (use_xinerama) {
            x = xinerama_screeninfo[i].x_org;
            y = xinerama_screeninfo[i].y_org;
            w = xinerama_screeninfo[i].width;
            h = xinerama_screeninfo[i].height;
        } else
#endif // QT_NO_XINERAMA
            {
                x = 0;
                y = 0;
                w = WidthOfScreen(ScreenOfDisplay(X11->display, i));
                h = HeightOfScreen(ScreenOfDisplay(X11->display, i));
            }

        rects[i].setRect(x, y, w, h);
        workareas[i] = QRect();
    }

#ifndef QT_NO_XINERAMA
    if (xinerama_screeninfo)
        XFree(xinerama_screeninfo);
#endif // QT_NO_XINERAMA

}

// the QDesktopWidget itself will be created on the default screen
// as qt_x11_create_desktop_on_screen defaults to -1
QDesktopWidget::QDesktopWidget()
    : QWidget(*new QDesktopWidgetPrivate, 0, Qt::Desktop)
{
    Q_D(QDesktopWidget);
    d->init();
}

QDesktopWidget::~QDesktopWidget()
{
}

bool QDesktopWidget::isVirtualDesktop() const
{
    Q_D(const QDesktopWidget);
    return d->use_xinerama;
}

int QDesktopWidget::primaryScreen() const
{
    Q_D(const QDesktopWidget);
    return d->defaultScreen;
}

int QDesktopWidget::numScreens() const
{
    Q_D(const QDesktopWidget);
    return d->screenCount;
}

QWidget *QDesktopWidget::screen(int screen)
{
    Q_D(QDesktopWidget);
    if (d->use_xinerama)
        return this;

    if (screen < 0 || screen >= d->screenCount)
        screen = d->defaultScreen;

    if (! d->screens) {
        d->screens = new QWidget*[d->screenCount];
        memset(d->screens, 0, d->screenCount * sizeof(QWidget *));
        d->screens[d->defaultScreen] = this;
    }

    if (! d->screens[screen] ||               // not created yet
        ! (d->screens[screen]->windowType() == Qt::Desktop)) { // reparented away
        qt_x11_create_desktop_on_screen = screen;
        d->screens[screen] = new QSingleDesktopWidget;
        qt_x11_create_desktop_on_screen = -1;
    }

    return d->screens[screen];
}

const QRect QDesktopWidget::availableGeometry(int screen) const
{
    Q_D(const QDesktopWidget);
    if (qt_desktopwidget_workarea_dirty) {
        // the workareas are dirty, invalidate them
        for (int i = 0; i < d->screenCount; ++i)
            d->workareas[i] = QRect();
        qt_desktopwidget_workarea_dirty = false;
    }

    if (screen < 0 || screen >= d->screenCount)
        screen = d->defaultScreen;

    if (d->workareas[screen].isValid())
        return d->workareas[screen];

    if (! isVirtualDesktop() && X11->isSupportedByWM(ATOM(_NET_WORKAREA))) {
        Atom ret;
        int format, e;
        unsigned char *data = 0;
        unsigned long nitems, after;

        e = XGetWindowProperty(X11->display,
                                QX11Info::appRootWindow(screen),
                                ATOM(_NET_WORKAREA), 0, 4, False, XA_CARDINAL,
                                &ret, &format, &nitems, &after, &data);

        if (e == Success && ret == XA_CARDINAL &&
            format == 32 && nitems == 4) {
            long *workarea = (long *) data;
            d->workareas[screen].setRect(workarea[0], workarea[1],
                                          workarea[2], workarea[3]);
        } else {
            d->workareas[screen] = screenGeometry(screen);
        }
        if (data)
            XFree(data);
    } else {
        d->workareas[screen] = screenGeometry(screen);
    }

    return d->workareas[screen];
}

const QRect QDesktopWidget::screenGeometry(int screen) const
{
    Q_D(const QDesktopWidget);
    if (screen < 0 || screen >= d->screenCount)
        screen = d->defaultScreen;

    return d->rects[screen];
}

int QDesktopWidget::screenNumber(const QWidget *widget) const
{
    Q_D(const QDesktopWidget);
    if (!widget)
        return d->defaultScreen;

#ifndef QT_NO_XINERAMA
    if (d->use_xinerama) {
        // this is how we do it for xinerama
        QRect frame = widget->frameGeometry();
        if (!widget->isWindow())
            frame.moveTopLeft(widget->mapToGlobal(QPoint(0, 0)));

        int maxSize = -1;
        int maxScreen = -1;

        for (int i = 0; i < d->screenCount; ++i) {
            QRect sect = d->rects[i].intersected(frame);
            int size = sect.width() * sect.height();
            if (size > maxSize && sect.width() > 0 && sect.height() > 0) {
                maxSize = size;
                maxScreen = i;
            }
        }
        return maxScreen;
    }
#endif // QT_NO_XINERAMA

    return widget->x11Info().screen();
}

int QDesktopWidget::screenNumber(const QPoint &point) const
{
    Q_D(const QDesktopWidget);
    int closestScreen = -1;
    int shortestDistance = INT_MAX;
    for (int i = 0; i < d->screenCount; ++i) {
        int thisDistance = d->pointToRect(point, d->rects[i]);
        if (thisDistance < shortestDistance) {
            shortestDistance = thisDistance;
            closestScreen = i;
        }
    }
    return closestScreen;
}

void QDesktopWidget::resizeEvent(QResizeEvent *event)
{
    Q_D(QDesktopWidget);
    d->init();
    qt_desktopwidget_workarea_dirty = true;
    QWidget::resizeEvent(event);
}
