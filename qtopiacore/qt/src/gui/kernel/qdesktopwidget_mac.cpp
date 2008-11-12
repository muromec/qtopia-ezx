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

#include "qapplication.h"
#include "qdesktopwidget.h"
#include <private/qt_mac_p.h>
#include "qwidget_p.h"

/*****************************************************************************
  Externals
 *****************************************************************************/
RgnHandle qt_mac_get_rgn(); //qregion_mac.cpp
void qt_mac_dispose_rgn(RgnHandle r); //qregion_mac.cpp

/*****************************************************************************
  QDesktopWidget member functions
 *****************************************************************************/

class QDesktopWidgetPrivate : public QWidgetPrivate
{
public:
    QDesktopWidgetPrivate();

    int appScreen;
    int screenCount;

    // Icky Icky Icky... we have two different types depending on
    // which version of OS X we are running. They are both 32-bit values though
    // so we can do the casting that needs to be done. Sensitive viewers of
    // this code may want to look away.
    QVector<CGDirectDisplayID> devs;
    QVector<QRect> rects;
    static void readScreenInformation(QVector<CGDirectDisplayID> &devices, QVector<QRect> &screenRects,
                                      int &screenCount);
};

QDesktopWidgetPrivate::QDesktopWidgetPrivate()
{
    appScreen = screenCount = 0;
    readScreenInformation(devs, rects, screenCount);
}

void QDesktopWidgetPrivate::readScreenInformation(QVector<CGDirectDisplayID> &devices,
                                                  QVector<QRect> &screenRects, int &screenCount)
{
    screenCount = 0;
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
    CGDisplayCount cg_count;
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5) {
        CGGetActiveDisplayList(0, 0, &cg_count);
        screenCount = cg_count;
    } else
#endif
    {
        for (GDHandle g = GetMainDevice(); g; g = GetNextDevice(g))
            ++screenCount;
    }
    devices.resize(screenCount);
    screenRects.resize(screenCount);
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5) {
        CGGetActiveDisplayList(screenCount, devices.data(), &cg_count);
        Q_ASSERT(cg_count == (CGDisplayCount)screenCount);
        for (int i = 0; i < screenCount; ++i) {
            CGRect r = CGDisplayBounds(devices.at(i));
            screenRects[i] = QRectF(r.origin.x, r.origin.y, r.size.width, r.size.height).toRect();
        }
    } else
#endif
    {
        int i = 0;
        for (GDHandle g = GetMainDevice(); i < screenCount && g; g = GetNextDevice(g), ++i) {
            devices[i] = CGDirectDisplayID(g);
            Rect r = (*g)->gdRect;
            screenRects[i] = QRect(r.left, r.top, r.right - r.left, r.bottom - r.top);
        }
    }
}

QDesktopWidget::QDesktopWidget()
: QWidget(*new QDesktopWidgetPrivate, 0, Qt::Desktop)
{
    setObjectName(QLatin1String("desktop"));
    setAttribute(Qt::WA_WState_Visible);
}

QDesktopWidget::~QDesktopWidget()
{
}

bool QDesktopWidget::isVirtualDesktop() const
{
    return true;
}

int QDesktopWidget::primaryScreen() const
{
    return d_func()->appScreen;
}

int QDesktopWidget::numScreens() const
{
    return d_func()->screenCount;
}

QWidget *QDesktopWidget::screen(int)
{
    return this;
}

const QRect QDesktopWidget::availableGeometry(int screen) const
{
    Q_D(const QDesktopWidget);
    if(screen < 0 || screen >= d->screenCount)
        screen = d->appScreen;
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5) {
        HIRect r;
        HIWindowGetAvailablePositioningBounds(d->devs[screen], kHICoordSpaceScreenPixel, &r);
        return QRectF(r.origin.x, r.origin.y, r.size.width, r.size.height).toRect();
    }
#endif
    Rect r;
    GetAvailableWindowPositioningBounds(reinterpret_cast<GDHandle>(d->devs[screen]), &r);
    return QRect(r.left, r.top, r.right - r.left, r.bottom - r.top);
}

const QRect QDesktopWidget::screenGeometry(int screen) const
{
    Q_D(const QDesktopWidget);
    if(screen < 0 || screen >= d->screenCount)
        screen = d->appScreen;
    return d->rects[screen];
}

int QDesktopWidget::screenNumber(const QWidget *widget) const
{
    Q_D(const QDesktopWidget);
    if(!widget)
        return d->appScreen;
    QRect frame = widget->frameGeometry();
    if(!widget->isWindow())
        frame.moveTopLeft(widget->mapToGlobal(QPoint(0,0)));
    int maxSize = -1, maxScreen = -1;
    for(int i = 0; i < d->screenCount; ++i) {
        QRect sect = d->rects[i].intersected(frame);
        int size = sect.width() * sect.height();
        if(size > maxSize && sect.width() > 0 && sect.height() > 0) {
            maxSize = size;
            maxScreen = i;
        }
    }
    return maxScreen;
}

int QDesktopWidget::screenNumber(const QPoint &point) const
{
    Q_D(const QDesktopWidget);
    int closestScreen = -1;
    int shortestDistance = INT_MAX;
    for (int i = 0; i < d->screenCount; ++i) {
        int thisDistance = d->pointToRect(point, d->rects.at(i));
        if (thisDistance < shortestDistance) {
            shortestDistance = thisDistance;
            closestScreen = i;
        }
    }
    return closestScreen;
}

void QDesktopWidget::resizeEvent(QResizeEvent *)
{
    Q_D(QDesktopWidget);
    int oldScreenCount = d->screenCount;
    QVector<QRect> oldRects = d->rects;

    QVector<CGDirectDisplayID> newDevs;
    QVector<QRect> newRects;
    int newScreenCount;
    QDesktopWidgetPrivate::readScreenInformation(newDevs, newRects, newScreenCount);
    d->screenCount = newScreenCount;
    d->devs = newDevs;
    d->rects = newRects;
    for (int i = 0; i < newScreenCount; ++i) {
        if (i >= oldScreenCount || newRects.at(i) != oldRects.at(i))
            emit resized(i);
    }
}
