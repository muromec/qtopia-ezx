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

#ifndef QMDIAREA_P_H
#define QMDIAREA_P_H

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

#include "qmdiarea.h"
#include "qmdisubwindow.h"

#ifndef QT_NO_MDIAREA

#include <QList>
#include <QRect>
#include <QPoint>
#include <private/qmdisubwindow_p.h>
#include <private/qabstractscrollarea_p.h>

class Rearranger
{
public:
    enum Type {
        RegularTiler,
        SimpleCascader,
        IconTiler
    };

    // Rearranges widgets relative to domain.
    virtual void rearrange(QList<QWidget *> &widgets, const QRect &domain) const = 0;
    virtual Type type() const = 0;
    virtual ~Rearranger() {}
};

class RegularTiler : public Rearranger
{
    // Rearranges widgets according to a regular tiling pattern
    // covering the entire domain.
    // Both positions and sizes may change.
    void rearrange(QList<QWidget *> &widgets, const QRect &domain) const;
    inline Type type() const { return Rearranger::RegularTiler; }
};

class SimpleCascader : public Rearranger
{
    // Rearranges widgets according to a simple, regular cascading pattern.
    // Widgets are resized to minimumSize.
    // Both positions and sizes may change.
    void rearrange(QList<QWidget *> &widgets, const QRect &domain) const;
    inline Type type() const { return Rearranger::SimpleCascader; }
};

class IconTiler : public Rearranger
{
    // Rearranges icons (assumed to be the same size) according to a regular
    // tiling pattern filling up the domain from the bottom.
    // Only positions may change.
    void rearrange(QList<QWidget *> &widgets, const QRect &domain) const;
    inline Type type() const { return Rearranger::IconTiler; }
};

class Placer
{
public:
    // Places the rectangle defined by 'size' relative to 'rects' and 'domain'.
    // Returns the position of the resulting rectangle.
    virtual QPoint place(
        const QSize &size, const QList<QRect> &rects, const QRect &domain) const = 0;
    virtual ~Placer() {}
};

class MinOverlapPlacer : public Placer
{
    QPoint place(const QSize &size, const QList<QRect> &rects, const QRect &domain) const;
    static int accumulatedOverlap(const QRect &source, const QList<QRect> &rects);
    static QRect findMinOverlapRect(const QList<QRect> &source, const QList<QRect> &rects);
    static void getCandidatePlacements(
        const QSize &size, const QList<QRect> &rects, const QRect &domain,
        QList<QRect> &candidates);
    static QPoint findBestPlacement(
        const QRect &domain, const QList<QRect> &rects, QList<QRect> &source);
    static void findNonInsiders(
        const QRect &domain, QList<QRect> &source, QList<QRect> &result);
    static void findMaxOverlappers(
        const QRect &domain, const QList<QRect> &source, QList<QRect> &result);
};

class QMdiAreaPrivate : public QAbstractScrollAreaPrivate
{
    Q_DECLARE_PUBLIC(QMdiArea)
public:
    QMdiAreaPrivate();

    // Variables.
    Rearranger *cascader;
    Rearranger *regularTiler;
    Rearranger *iconTiler;
    Placer *placer;
    QList<Rearranger *> pendingRearrangements;
    QList< QPointer<QMdiSubWindow> > pendingPlacements;
    QList< QPointer<QMdiSubWindow> > childWindows;
    QList<int> indicesToStackedChildren;
    QPointer<QMdiSubWindow> active;
    QPointer<QMdiSubWindow> aboutToBecomeActive;
    QBrush background;
    QMdiArea::AreaOptions options;
    bool ignoreGeometryChange;
    bool ignoreWindowStateChange;
    bool isActivated;
    bool isSubWindowsTiled;
    bool showActiveWindowMaximized;
    bool tileCalledFromResizeEvent;
    bool updatesDisabledByUs;
    int indexToNextWindow;
    int indexToPreviousWindow;
    int resizeTimerId;

    // Slots.
    void _q_deactivateAllWindows(QMdiSubWindow *aboutToActivate = 0);
    void _q_processWindowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState);

    // Functions.
    void appendChild(QMdiSubWindow *child);
    void place(Placer *placer, QMdiSubWindow *child);
    void rearrange(Rearranger *rearranger);
    void arrangeMinimizedSubWindows();
    void activateWindow(QMdiSubWindow *child);
    void activateCurrentWindow();
    void emitWindowActivated(QMdiSubWindow *child);
    void resetActiveWindow(QMdiSubWindow *child = 0);
    void updateActiveWindow(int removedIndex);
    void updateScrollBars();
    void internalRaise(QMdiSubWindow *child) const;
    bool scrollBarsEnabled() const;
    bool lastWindowAboutToBeDestroyed() const;
    void setChildActivationEnabled(bool enable = true, bool onlyNextActivationEvent = false) const;

    // Reimp
    void scrollBarPolicyChanged(Qt::Orientation, Qt::ScrollBarPolicy);
    QList<QMdiSubWindow *> subWindowList(QMdiArea::WindowOrder, bool reversed = false) const;
    void disconnectSubWindow(QObject *subWindow);

    inline void startResizeTimer()
    {
        Q_Q(QMdiArea);
        if (resizeTimerId > 0)
            q->killTimer(resizeTimerId);
        resizeTimerId = q->startTimer(200);
    }

    inline bool windowStaysOnTop(QMdiSubWindow *subWindow) const
    {
        if (!subWindow)
            return false;
        return subWindow->windowFlags() & Qt::WindowStaysOnTopHint;
    }

    inline bool isExplicitlyDeactivated(QMdiSubWindow *subWindow) const
    {
        if (!subWindow)
            return true;
        return subWindow->d_func()->isExplicitlyDeactivated;
    }

    inline void setActive(QMdiSubWindow *subWindow, bool active = true) const
    {
        if (subWindow)
            subWindow->d_func()->setActive(active);
    }
};

#endif // QT_NO_MDIAREA

#endif // QMDIAREA_P_H
