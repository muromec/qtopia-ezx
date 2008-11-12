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
#include "qapplication_p.h"
#include "qbitmap.h"
#include "qcursor.h"
#include "qdesktopwidget.h"
#include "qevent.h"
#include "qimage.h"
#include "qlayout.h"
#include "qlibrary.h"
#include "qpainter.h"
#include "qstack.h"
#include "qt_windows.h"
#include "qwidget.h"
#include "qwidget_p.h"
#include "private/qbackingstore_p.h"

#ifndef QT_NO_DIRECT3D
#include "private/qpaintengine_d3d_p.h"
#endif

#include <qdebug.h>

#include <private/qapplication_p.h>
#include <private/qwininputcontext_p.h>
#include <private/qpaintengine_raster_p.h>

typedef BOOL    (WINAPI *PtrSetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
static PtrSetLayeredWindowAttributes ptrSetLayeredWindowAttributes = 0;
#define Q_WS_EX_LAYERED           0x00080000 // copied from WS_EX_LAYERED in winuser.h
#define Q_LWA_ALPHA               0x00000002 // copied from LWA_ALPHA in winuser.h

#ifdef Q_OS_TEMP
#include "sip.h"
#endif

#if defined(QT_NON_COMMERCIAL)
#include "qnc_win.h"
#endif

#if !defined(WS_EX_TOOLWINDOW)
#define WS_EX_TOOLWINDOW 0x00000080
#endif

#if !defined(GWLP_WNDPROC)
#define GWLP_WNDPROC GWL_WNDPROC
#endif

//#define TABLET_DEBUG
#define PACKETDATA  (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_TANGENT_PRESSURE \
                     | PK_ORIENTATION | PK_CURSOR | PK_Z)
#define PACKETMODE  0
#include <wintab.h>
#include <pktdef.h>

typedef HCTX        (API *PtrWTOpen)(HWND, LPLOGCONTEXT, BOOL);
typedef BOOL        (API *PtrWTClose)(HCTX);
typedef UINT        (API *PtrWTInfo)(UINT, UINT, LPVOID);
typedef BOOL        (API *PtrWTEnable)(HCTX, BOOL);
typedef BOOL        (API *PtrWTOverlap)(HCTX, BOOL);
typedef int        (API *PtrWTPacketsGet)(HCTX, int, LPVOID);
typedef BOOL        (API *PtrWTGet)(HCTX, LPLOGCONTEXT);
typedef int     (API *PtrWTQueueSizeGet)(HCTX);
typedef BOOL    (API *PtrWTQueueSizeSet)(HCTX, int);

static PtrWTOpen ptrWTOpen = 0;
static PtrWTClose ptrWTClose = 0;
static PtrWTInfo ptrWTInfo = 0;
static PtrWTQueueSizeGet ptrWTQueueSizeGet = 0;
static PtrWTQueueSizeSet ptrWTQueueSizeSet = 0;
static void init_wintab_functions();
static void qt_tablet_init();
static void qt_tablet_cleanup();
extern HCTX qt_tablet_context;
extern bool qt_tablet_tilt_support;

static QWidget *qt_tablet_widget = 0;

extern bool qt_is_gui_used;
static void init_wintab_functions()
{
    if (!qt_is_gui_used)
        return;
    QLibrary library(QLatin1String("wintab32"));
    QT_WA({
        ptrWTOpen = (PtrWTOpen)library.resolve("WTOpenW");
        ptrWTInfo = (PtrWTInfo)library.resolve("WTInfoW");
    } , {
        ptrWTOpen = (PtrWTOpen)library.resolve("WTOpenA");
        ptrWTInfo = (PtrWTInfo)library.resolve("WTInfoA");
    });

    ptrWTClose = (PtrWTClose)library.resolve("WTClose");
    ptrWTQueueSizeGet = (PtrWTQueueSizeGet)library.resolve("WTQueueSizeGet");
    ptrWTQueueSizeSet = (PtrWTQueueSizeSet)library.resolve("WTQueueSizeSet");
}

static void qt_tablet_init()
{
    static bool firstTime = true;
    if (!firstTime)
        return;
    firstTime = false;
    qt_tablet_widget = new QWidget(0);
    qt_tablet_widget->createWinId();
    qt_tablet_widget->setObjectName(QLatin1String("Qt internal tablet widget"));
    LOGCONTEXT lcMine;
    qAddPostRoutine(qt_tablet_cleanup);
    struct tagAXIS tpOri[3];
    init_wintab_functions();
    if (ptrWTInfo && ptrWTOpen && ptrWTQueueSizeGet && ptrWTQueueSizeSet) {
        // make sure we have WinTab
        if (!ptrWTInfo(0, 0, NULL)) {
#ifdef TABLET_DEBUG
            qWarning("QWidget: Wintab services not available");
#endif
            return;
        }

        // some tablets don't support tilt, check if it is possible,
        qt_tablet_tilt_support = ptrWTInfo(WTI_DEVICES, DVC_ORIENTATION, &tpOri);
        if (qt_tablet_tilt_support) {
            // check for azimuth and altitude
            qt_tablet_tilt_support = tpOri[0].axResolution && tpOri[1].axResolution;
        }
        // build our context from the default context
        ptrWTInfo(WTI_DEFSYSCTX, 0, &lcMine);
        // Go for the raw coordinates, the tablet event will return good stuff
        lcMine.lcOptions |= CXO_MESSAGES | CXO_CSRMESSAGES;
        lcMine.lcPktData = PACKETDATA;
        lcMine.lcPktMode = PACKETMODE;
        lcMine.lcMoveMask = PACKETDATA;
        lcMine.lcOutOrgX = 0;
        lcMine.lcOutExtX = lcMine.lcInExtX;
        lcMine.lcOutOrgY = 0;
        lcMine.lcOutExtY = -lcMine.lcInExtY;
        qt_tablet_context = ptrWTOpen(qt_tablet_widget->winId(), &lcMine, true);
#ifdef TABLET_DEBUG
        qDebug("Tablet is %p", qt_tablet_context);
#endif
        if (!qt_tablet_context) {
#ifdef TABLET_DEBUG
            qWarning("QWidget: Failed to open the tablet");
#endif
            return;
        }
        // Set the size of the Packet Queue to the correct size...
        int currSize = ptrWTQueueSizeGet(qt_tablet_context);
        if (!ptrWTQueueSizeSet(qt_tablet_context, QT_TABLET_NPACKETQSIZE)) {
            // Ideally one might want to use a smaller
            // multiple, but for now, since we managed to destroy
            // the existing Q with the previous call, set it back
            // to the other size, which should work.  If not,
            // there will be trouble.
            if (!ptrWTQueueSizeSet(qt_tablet_context, currSize)) {
                Q_ASSERT_X(0, "Qt::Internal", "There is no packet queue for"
                         " the tablet. The tablet will not work");
            }
        }
    }
}

static void qt_tablet_cleanup()
{
    if (ptrWTClose)
        ptrWTClose(qt_tablet_context);
    delete qt_tablet_widget;
    qt_tablet_widget = 0;
}

const QString qt_reg_winclass(QWidget *w);                // defined in qapplication_win.cpp
void            qt_olednd_unregister(QWidget* widget, QOleDropTarget *dst); // dnd_win
QOleDropTarget* qt_olednd_register(QWidget* widget);

extern bool qt_nograb();
extern HRGN qt_win_bitmapToRegion(const QBitmap& bitmap);

static QWidget *mouseGrb    = 0;
static QCursor *mouseGrbCur = 0;
static QWidget *keyboardGrb = 0;
static HHOOK   journalRec  = 0;

extern "C" LRESULT CALLBACK QtWndProc(HWND, UINT, WPARAM, LPARAM);

#define XCOORD_MAX 16383
#define WRECT_MAX 16383

/*****************************************************************************
  QWidget member functions
 *****************************************************************************/


void QWidgetPrivate::create_sys(WId window, bool initializeWindow, bool destroyOldWindow)
{
    Q_Q(QWidget);
    static int sw = -1, sh = -1;

    Qt::WindowType type = q->windowType();
    Qt::WindowFlags flags = data.window_flags;

    bool topLevel = (flags & Qt::Window);
    bool popup = (type == Qt::Popup);
    bool dialog = (type == Qt::Dialog
                   || type == Qt::Sheet
                   || (flags & Qt::MSWindowsFixedSizeDialogHint));
    bool desktop = (type == Qt::Desktop);
    bool tool = (type == Qt::Tool || type == Qt::Drawer);

    HINSTANCE appinst  = qWinAppInst();
    HWND parentw, destroyw = 0;
    WId id;

    QString windowClassName = qt_reg_winclass(q);

    if (!window)                                // always initialize
        initializeWindow = true;

    if (popup)
        flags |= Qt::WindowStaysOnTopHint; // a popup stays on top

    if (flags & (Qt::WindowMinMaxButtonsHint | Qt::WindowContextHelpButtonHint)) {
        flags |= Qt::WindowSystemMenuHint;
        flags |= Qt::WindowTitleHint;
        flags &= ~Qt::FramelessWindowHint;
    }

    if (sw < 0) {                                // get the (primary) screen size
        sw = GetSystemMetrics(SM_CXSCREEN);
        sh = GetSystemMetrics(SM_CYSCREEN);
    }

    if (desktop) {                                // desktop widget
        popup = false;                                // force this flags off
#ifndef Q_OS_TEMP
        if (QSysInfo::WindowsVersion != QSysInfo::WV_NT && QSysInfo::WindowsVersion != QSysInfo::WV_95)
            data.crect.setRect(GetSystemMetrics(76 /* SM_XVIRTUALSCREEN  */), GetSystemMetrics(77 /* SM_YVIRTUALSCREEN  */),
                           GetSystemMetrics(78 /* SM_CXVIRTUALSCREEN */), GetSystemMetrics(79 /* SM_CYVIRTUALSCREEN */));
        else
#endif
            data.crect.setRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    }

    parentw = q->parentWidget() ? q->parentWidget()->internalWinId() : 0;

#ifdef UNICODE
    QString title;
    const TCHAR *ttitle = 0;
#endif
    QByteArray title95;
    int style = WS_CHILD;
    int exsty = WS_EX_NOPARENTNOTIFY;

    if (window) {
        style = GetWindowLongA(window, GWL_STYLE);
        if (!style)
            qErrnoWarning("QWidget::create: GetWindowLong failed");
        topLevel = false; // #### needed for some IE plugins??
    } else if (popup || (type == Qt::ToolTip) || (type == Qt::SplashScreen)) {
        style = WS_POPUP;
    } else if (topLevel && !desktop) {
        if (flags & Qt::FramelessWindowHint)
            style = WS_POPUP;                // no border
        else if (flags & Qt::WindowTitleHint)
            style = WS_OVERLAPPED;
        else
            style = 0;
    }
    if (!desktop) {
        // if (!testAttribute(Qt::WA_PaintUnclipped))
        // ### Commented out for now as it causes some problems, but
        // this should be correct anyway, so dig some more into this
#ifndef Q_FLATTEN_EXPOSE
        style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN ;
#endif
        if (topLevel) {
            if ((type == Qt::Window || dialog || tool)) {
                if (!(flags & Qt::FramelessWindowHint)) {
                    if (!(flags & Qt::MSWindowsFixedSizeDialogHint)) {
                        style |= WS_THICKFRAME;
                        if(!(flags &
                            ( Qt::WindowSystemMenuHint
                            | Qt::WindowTitleHint
                            | Qt::WindowMinMaxButtonsHint
                            | Qt::WindowContextHelpButtonHint)))
                            style |= WS_POPUP;
                    } else {
                        style |= WS_POPUP | WS_DLGFRAME;
                    }
                }
                if (flags & Qt::WindowTitleHint)
                    style |= WS_CAPTION;
                if (flags & Qt::WindowSystemMenuHint)
                    style |= WS_SYSMENU;
                if (flags & Qt::WindowMinimizeButtonHint)
                    style |= WS_MINIMIZEBOX;
                if (flags & Qt::WindowMaximizeButtonHint)
                    style |= WS_MAXIMIZEBOX;
                if (tool)
                    exsty |= WS_EX_TOOLWINDOW;
                if (flags & Qt::WindowContextHelpButtonHint)
                    exsty |= WS_EX_CONTEXTHELP;
            } else {
                 exsty |= WS_EX_TOOLWINDOW;
            }
        }
    }

    if (flags & Qt::WindowTitleHint) {
        QT_WA({
            title = q->isWindow() ? qAppName() : q->objectName();
            ttitle = (TCHAR*)title.utf16();
        } , {
            title95 = q->isWindow() ? qAppName().toLocal8Bit() : q->objectName().toLatin1();
        });
    }

    // The Qt::WA_WState_Created flag is checked by translateConfigEvent() in
    // qapplication_win.cpp. We switch it off temporarily to avoid move
    // and resize events during creationt
    q->setAttribute(Qt::WA_WState_Created, false);

    if (window) {                                // override the old window
        if (destroyOldWindow)
            destroyw = data.winid;
        id = window;
        setWinId(window);
        LONG res = SetWindowLongA(window, GWL_STYLE, style);
        if (!res)
            qErrnoWarning("QWidget::create: Failed to set window style");
#ifdef _WIN64
        res = SetWindowLongPtrA( window, GWLP_WNDPROC, (LONG_PTR)QtWndProc );
#else
        res = SetWindowLongA( window, GWL_WNDPROC, (LONG)QtWndProc );
#endif
        if (!res)
            qErrnoWarning("QWidget::create: Failed to set window procedure");
    } else if (desktop) {                        // desktop widget
#ifndef Q_OS_TEMP
        id = GetDesktopWindow();
//         QWidget *otherDesktop = QWidget::find(id);        // is there another desktop?
//         if (otherDesktop && otherDesktop->testWFlags(Qt::WPaintDesktop)) {
//             otherDesktop->d_func()->setWinId(0);        // remove id from widget mapper
//             d->setWinId(id);                     // make sure otherDesktop is
//             otherDesktop->d_func()->setWinId(id);       //   found first
//         } else {
            setWinId(id);
//         }
#endif
    } else if (topLevel) {                       // create top-level widget
        if (popup)
            parentw = 0;

#ifdef Q_OS_TEMP

        const TCHAR *cname = windowClassName.utf16();

        id = CreateWindowEx(exsty, cname, ttitle, style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parentw, 0, appinst, 0);
#else
        const bool wasMoved = q->testAttribute(Qt::WA_Moved);
        int x = wasMoved ? data.crect.left() : CW_USEDEFAULT;
        int y = wasMoved ? data.crect.top() : CW_USEDEFAULT;
        int w = CW_USEDEFAULT;
        int h = CW_USEDEFAULT;

        // Adjust for framestrut when needed
        RECT rect = {0,0,0,0};
        if (AdjustWindowRectEx(&rect, style & ~WS_OVERLAPPED, FALSE, exsty)) {
            QTLWExtra *td = maybeTopData();
            if (wasMoved && (td && !td->posFromMove)) {
                x = data.crect.x() + rect.left;
                y = data.crect.y() + rect.top;
            }

            if (q->testAttribute(Qt::WA_Resized)) {
                w = data.crect.width() + (rect.right - rect.left);
                h = data.crect.height() + (rect.bottom - rect.top);
            }
        }

        //update position & initial size of POPUP window
        if (topLevel && initializeWindow && (style & WS_POPUP)) {
            if (!q->testAttribute(Qt::WA_Resized)) {
                w = sw/2;
                h = 4*sh/10;
            }
            if (!wasMoved) {
                x = sw/2 - w/2;
                y = sh/2 - h/2;
            }
        }

        QT_WA({
            const TCHAR *cname = (TCHAR*)windowClassName.utf16();
            id = CreateWindowEx(exsty, cname, ttitle, style,
                                x, y, w, h,
                                parentw, 0, appinst, 0);
        } , {
            id = CreateWindowExA(exsty, windowClassName.toLatin1(), title95, style,
                                 x, y, w, h,
                                 parentw, 0, appinst, 0);
        });

#endif

        if (!id)
            qErrnoWarning("QWidget::create: Failed to create window");
        setWinId(id);
        if ((flags & Qt::WindowStaysOnTopHint) || (type == Qt::ToolTip))
            SetWindowPos(id, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    } else {                                        // create child widget
        QT_WA({
            const TCHAR *cname = (TCHAR*)windowClassName.utf16();
            id = CreateWindowEx(exsty, cname, ttitle, style,
                                data.crect.left(), data.crect.top(), data.crect.width(), data.crect.height(),
                            parentw, NULL, appinst, NULL);
        } , {
            id = CreateWindowExA(exsty, windowClassName.toLatin1(), title95, style,
                                 data.crect.left(), data.crect.top(), data.crect.width(), data.crect.height(),
                            parentw, NULL, appinst, NULL);
        });
        if (!id)
            qErrnoWarning("QWidget::create: Failed to create window");
        SetWindowPos(id, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        setWinId(id);
    }

    if (desktop) {
        q->setAttribute(Qt::WA_WState_Visible);
    } else if (topLevel){
        RECT  cr;
        GetClientRect(id, &cr);
        // one cannot trust cr.left and cr.top, use a correction POINT instead
        POINT pt;
        pt.x = 0;
        pt.y = 0;
        ClientToScreen(id, &pt);

        if (data.crect.width() == 0 || data.crect.height() == 0) {
            data.crect = QRect(pt.x, pt.y, data.crect.width(), data.crect.height());
        } else {
            data.crect = QRect(QPoint(pt.x, pt.y),
                               QPoint(pt.x + cr.right - 1, pt.y + cr.bottom - 1));
        }

        if (data.fstrut_dirty) {
            // be nice to activeqt
            updateFrameStrut();
        }
    }

    q->setAttribute(Qt::WA_WState_Created);                // accept move/resize events
    hd = 0;                                        // no display context

    if (window) {                                // got window from outside
        if (IsWindowVisible(window))
            q->setAttribute(Qt::WA_WState_Visible);
        else
            q->setAttribute(Qt::WA_WState_Visible, false);
    }

    if (extra && !extra->mask.isEmpty())
        setMask_sys(extra->mask);

#if defined(QT_NON_COMMERCIAL)
    QT_NC_WIDGET_CREATE
#endif

    if (q->hasFocus() && q->testAttribute(Qt::WA_InputMethodEnabled))
        q->inputContext()->setFocusWidget(q);

    if (destroyw) {
        DestroyWindow(destroyw);
    }

    QWinInputContext::enable(q, q->testAttribute(Qt::WA_InputMethodEnabled) & q->isEnabled());
    if (q != qt_tablet_widget && QWidgetPrivate::mapper)
        qt_tablet_init();

    if (q->testAttribute(Qt::WA_DropSiteRegistered))
        registerDropSite(true);

    if (maybeTopData() && maybeTopData()->opacity != 255)
        q->setWindowOpacity(maybeTopData()->opacity/255.);

    if (topLevel && (data.crect.width() == 0 || data.crect.height() == 0)) {
        q->setAttribute(Qt::WA_OutsideWSRange, true);
    }
}


void QWidget::destroy(bool destroyWindow, bool destroySubWindows)
{
    Q_D(QWidget);
    if (!isWindow() && parentWidget())
        parentWidget()->d_func()->invalidateBuffer(geometry());
    d->deactivateWidgetCleanup();
    if (testAttribute(Qt::WA_WState_Created)) {
        setAttribute(Qt::WA_WState_Created, false);
        for(int i = 0; i < d->children.size(); ++i) { // destroy all widget children
            register QObject *obj = d->children.at(i);
            if (obj->isWidgetType())
                ((QWidget*)obj)->destroy(destroySubWindows,
                                         destroySubWindows);
        }
        if (mouseGrb == this)
            releaseMouse();
        if (keyboardGrb == this)
            releaseKeyboard();
        if (testAttribute(Qt::WA_ShowModal))                // just be sure we leave modal
            QApplicationPrivate::leaveModal(this);
        else if ((windowType() == Qt::Popup))
            qApp->d_func()->closePopup(this);
        if (destroyWindow && !(windowType() == Qt::Desktop)) {
            DestroyWindow(internalWinId());
        }
        d->setWinId(0);
    }
}

void QWidgetPrivate::reparentChildren()
{
    Q_Q(QWidget);
    QObjectList chlist = q->children();
    for(int i = 0; i < chlist.size(); ++i) { // reparent children
        QObject *obj = chlist.at(i);
        if (obj->isWidgetType()) {
            QWidget *w = (QWidget *)obj;
            if ((w->windowType() == Qt::Popup)) {
                ;
            } else if (w->isWindow()) {
                bool showIt = w->isVisible();
                QPoint old_pos = w->pos();
                w->setParent(q, w->windowFlags());
                w->move(old_pos);
                if (showIt)
                    w->show();
            } else {
                w->d_func()->invalidateBuffer(w->rect());
                SetParent(w->internalWinId(), q->internalWinId());
                w->d_func()->reparentChildren();
            }
        }
    }
}

void QWidgetPrivate::setParent_sys(QWidget *parent, Qt::WindowFlags f)
{
    Q_Q(QWidget);
    bool wasCreated = q->testAttribute(Qt::WA_WState_Created);
    if (q->isVisible() && q->parentWidget() && parent != q->parentWidget())
        q->parentWidget()->d_func()->invalidateBuffer(q->geometry());

    WId old_winid = data.winid;
    // hide and reparent our own window away. Otherwise we might get
    // destroyed when emitting the child remove event below. See QWorkspace.
    if (q->isVisible()) {
        ShowWindow(data.winid, SW_HIDE);
        SetParent(data.winid, 0);
    }

    if (q->testAttribute(Qt::WA_DropSiteRegistered))
        q->setAttribute(Qt::WA_DropSiteRegistered, false); // ole dnd unregister (we will register again below)

    if ((q->windowType() == Qt::Desktop))
        old_winid = 0;
    setWinId(0);

    QObjectPrivate::setParent_helper(parent);
    bool explicitlyHidden = q->testAttribute(Qt::WA_WState_Hidden) && q->testAttribute(Qt::WA_WState_ExplicitShowHide);

    data.window_flags = f;
    data.fstrut_dirty = true;
    q->setAttribute(Qt::WA_WState_Created, false);
    q->setAttribute(Qt::WA_WState_Visible, false);
    q->setAttribute(Qt::WA_WState_Hidden, false);
    adjustFlags(data.window_flags, q);
    // keep compatibility with previous versions, we need to preserve the created state
    // (but we recreate the winId for the widget being reparented, again for compatibility)
    if (wasCreated || (!q->isWindow() && parent->testAttribute(Qt::WA_WState_Created)))
        createWinId();
    if (q->isWindow() || (!parent || parent->isVisible()) || explicitlyHidden)
        q->setAttribute(Qt::WA_WState_Hidden);
    q->setAttribute(Qt::WA_WState_ExplicitShowHide, explicitlyHidden);

    if (wasCreated) {
        reparentChildren();
    }

    if (extra && !extra->mask.isEmpty()) {
        QRegion r = extra->mask;
        extra->mask = QRegion();
        q->setMask(r);
    }
    if (extra && extra->topextra && !extra->topextra->caption.isEmpty())
        setWindowTitle_helper(extra->topextra->caption);
    if (old_winid)
        DestroyWindow(old_winid);

    if (q->testAttribute(Qt::WA_AcceptDrops)
        || (!q->isWindow() && q->parentWidget() && q->parentWidget()->testAttribute(Qt::WA_DropSiteRegistered)))
        q->setAttribute(Qt::WA_DropSiteRegistered, true);


#ifdef Q_OS_TEMP
    // Show borderless toplevel windows in tasklist & NavBar
    if (!parent) {
        QString txt = windowTitle().isEmpty()?qAppName():windowTitle();
        SetWindowText(internalWinId(), (TCHAR*)txt.utf16());
    }
#endif
    invalidateBuffer(q->rect());
}


QPoint QWidget::mapToGlobal(const QPoint &pos) const
{
    Q_D(const QWidget);
    if (!isVisible() || window()->isMinimized() || !testAttribute(Qt::WA_WState_Created)) {
        QPoint toGlobal = mapTo(window(), pos) + window()->pos();
        // Adjust for window decorations
        toGlobal += window()->geometry().topLeft() - window()->frameGeometry().topLeft();
        return toGlobal;
    }
    POINT p;
    QPoint tmp = d->mapToWS(pos);
    p.x = tmp.x();
    p.y = tmp.y();
    ClientToScreen(internalWinId(), &p);
    return QPoint(p.x, p.y);
}

QPoint QWidget::mapFromGlobal(const QPoint &pos) const
{
    Q_D(const QWidget);
    if (!isVisible() || window()->isMinimized() || !testAttribute(Qt::WA_WState_Created)) {
        QPoint fromGlobal = mapFrom(window(), pos - window()->pos());
        // Adjust for window decorations
        fromGlobal -= window()->geometry().topLeft() - window()->frameGeometry().topLeft();
        return fromGlobal;
    }
    POINT p;
    p.x = pos.x();
    p.y = pos.y();
    ScreenToClient(internalWinId(), &p);
    return d->mapFromWS(QPoint(p.x, p.y));
}

void QWidgetPrivate::updateSystemBackground() {}

extern void qt_win_set_cursor(QWidget *, const QCursor &); // qapplication_win.cpp

void QWidgetPrivate::setCursor_sys(const QCursor &cursor)
{
    Q_UNUSED(cursor);
    Q_Q(QWidget);
    qt_win_set_cursor(q, q->cursor());
}

void QWidgetPrivate::unsetCursor_sys()
{
    Q_Q(QWidget);
    qt_win_set_cursor(q, q->cursor());
}

void QWidgetPrivate::setWindowTitle_sys(const QString &caption)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    QT_WA({
        SetWindowText(q->internalWinId(), (TCHAR*)caption.utf16());
    } , {
        SetWindowTextA(q->internalWinId(), caption.toLocal8Bit());
    });
}

/*
  Create an icon mask the way Windows wants it using CreateBitmap.
*/

HBITMAP qt_createIconMask(const QBitmap &bitmap)
{
    QImage bm = bitmap.toImage().convertToFormat(QImage::Format_Mono);
    int w = bm.width();
    int h = bm.height();
    int bpl = ((w+15)/16)*2;                        // bpl, 16 bit alignment
    uchar *bits = new uchar[bpl*h];
    bm.invertPixels();
    for (int y=0; y<h; y++)
        memcpy(bits+y*bpl, bm.scanLine(y), bpl);
    HBITMAP hbm = CreateBitmap(w, h, 1, 1, bits);
    delete [] bits;
    return hbm;
}

HICON qt_createIcon(QIcon icon, int xSize, int ySize, QPixmap **cache)
{
    HICON result = 0;
    if (!icon.isNull()) { // valid icon
        QSize size = icon.actualSize(QSize(xSize, ySize));
        QPixmap pm = icon.pixmap(size);
        if (pm.isNull())
            return 0;

        QBitmap mask = pm.mask();
        if (mask.isNull()) {
            mask = QBitmap(pm.size());
            mask.fill(Qt::color1);
        }

        HBITMAP im = qt_createIconMask(mask);
        ICONINFO ii;
        ii.fIcon    = true;
        ii.hbmMask  = im;
        ii.hbmColor = pm.toWinHBITMAP(QPixmap::PremultipliedAlpha);
        ii.xHotspot = 0;
        ii.yHotspot = 0;
        result = CreateIconIndirect(&ii);

        if (cache) {
            delete *cache;
            *cache = new QPixmap(pm);;
        }
        DeleteObject(ii.hbmColor);
        DeleteObject(im);
    }
    return result;
}

void QWidgetPrivate::setWindowIcon_sys(bool forceReset)
{
    Q_Q(QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created))
        return;
    QTLWExtra* x = topData();
    if (x->iconPixmap && !forceReset)
        // already been set
        return;

    if (x->winIconBig) {
        DestroyIcon(x->winIconBig);
        x->winIconBig = 0;
    }
    if (x->winIconSmall) {
        DestroyIcon(x->winIconSmall);
        x->winIconSmall = 0;
    }

    x->winIconSmall = qt_createIcon(q->windowIcon(),
                                    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                    &(x->iconPixmap));
    x->winIconBig = qt_createIcon(q->windowIcon(),
                                  GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
                                  &(x->iconPixmap));
    if (x->winIconBig) {
        SendMessageA(q->internalWinId(), WM_SETICON, 0 /* ICON_SMALL */, (LPARAM)x->winIconSmall);
        SendMessageA(q->internalWinId(), WM_SETICON, 1 /* ICON_BIG */, (LPARAM)x->winIconBig);
    } else {
        SendMessageA(q->internalWinId(), WM_SETICON, 0 /* ICON_SMALL */, (LPARAM)x->winIconSmall);
        SendMessageA(q->internalWinId(), WM_SETICON, 1 /* ICON_BIG */, (LPARAM)x->winIconSmall);
    }
}


void QWidgetPrivate::setWindowIconText_sys(const QString &iconText)
{
    Q_UNUSED(iconText);
}


QCursor *qt_grab_cursor()
{
    return mouseGrbCur;
}

// The procedure does nothing, but is required for mousegrabbing to work
LRESULT CALLBACK qJournalRecordProc(int nCode, WPARAM wParam, LPARAM lParam)
{
#ifndef Q_OS_TEMP
    return CallNextHookEx(journalRec, nCode, wParam, lParam);
#else
    return 0;
#endif
}

/* Works only as long as pointer is inside the application's window,
   which is good enough for QDockWidget.

   Doesn't call SetWindowsHookExA() - this function causes a system-wide
   freeze if any other app on the system installs a hook and fails to
   process events. */
void QWidgetPrivate::grabMouseWhileInWindow()
{
    Q_Q(QWidget);
    if (!qt_nograb()) {
        if (mouseGrb)
            mouseGrb->releaseMouse();
        Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
        SetCapture(q->internalWinId());
        mouseGrb = q;
    }
}

void QWidget::grabMouse()
{
    if (!qt_nograb()) {
        if (mouseGrb)
            mouseGrb->releaseMouse();
#ifndef Q_OS_TEMP
        journalRec = SetWindowsHookExA(WH_JOURNALRECORD, (HOOKPROC)qJournalRecordProc, GetModuleHandleA(0), 0);
#endif
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
        SetCapture(internalWinId());
        mouseGrb = this;
    }
}

void QWidget::grabMouse(const QCursor &cursor)
{
    if (!qt_nograb()) {
        if (mouseGrb)
            mouseGrb->releaseMouse();
#ifndef Q_OS_TEMP
        journalRec = SetWindowsHookExA(WH_JOURNALRECORD, (HOOKPROC)qJournalRecordProc, GetModuleHandleA(0), 0);
#endif
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
        SetCapture(internalWinId());
        mouseGrbCur = new QCursor(cursor);
        SetCursor(mouseGrbCur->handle());
        mouseGrb = this;
    }
}

void QWidget::releaseMouse()
{
    if (!qt_nograb() && mouseGrb == this) {
        ReleaseCapture();
        if (journalRec) {
#ifndef Q_OS_TEMP
            UnhookWindowsHookEx(journalRec);
#endif
            journalRec = 0;
        }
        if (mouseGrbCur) {
            delete mouseGrbCur;
            mouseGrbCur = 0;
        }
        mouseGrb = 0;
    }
}

void QWidget::grabKeyboard()
{
    if (!qt_nograb()) {
        if (keyboardGrb)
            keyboardGrb->releaseKeyboard();
        keyboardGrb = this;
    }
}

void QWidget::releaseKeyboard()
{
    if (!qt_nograb() && keyboardGrb == this)
        keyboardGrb = 0;
}


QWidget *QWidget::mouseGrabber()
{
    return mouseGrb;
}

QWidget *QWidget::keyboardGrabber()
{
    return keyboardGrb;
}

void QWidget::activateWindow()
{
    window()->createWinId();
    SetForegroundWindow(window()->internalWinId());
}

extern UINT WM_QT_REPAINT;

void QWidgetPrivate::dirtyWidget_sys(const QRegion &rgn)
{
    Q_Q(QWidget);
    if (!rgn.isEmpty()) {
        Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
        QRegion wrgn = rgn;
        if (data.wrect.isValid())
            wrgn.translate(-data.wrect.topLeft());
#if defined(Q_WIN_USE_QT_UPDATE_EVENT)
        dirtyOnScreen += rgn;
        QApplication::postEvent(q, new QEvent(QEvent::UpdateRequest), Qt::LowEventPriority);
        return;
#endif
        InvalidateRgn(q->internalWinId(), wrgn.handle(), FALSE);
        // check if this is the first call to dirty a previously clean widget
        if (!q->testAttribute(Qt::WA_PendingUpdate)) {
            q->setAttribute(Qt::WA_PendingUpdate);
            QT_WA( {
                PostMessageW(q->internalWinId(), WM_QT_REPAINT, 0, 0);
            }, {
                PostMessageA(q->internalWinId(), WM_QT_REPAINT, 0, 0);
            } );
        }
    }
}

void QWidgetPrivate::cleanWidget_sys(const QRegion& rgn)
{
    Q_Q(QWidget);
#if defined(Q_WIN_USE_QT_UPDATE_EVENT)
    dirtyOnScreen -= rgn;
#endif
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    ValidateRgn(q->internalWinId(),rgn.handle());
}

void QWidget::setWindowState(Qt::WindowStates newstate)
{
    Q_D(QWidget);
    Qt::WindowStates oldstate = windowState();
    if (oldstate == newstate)
        return;

    int max = SW_MAXIMIZE;
    int min = SW_MINIMIZE;
    int normal = SW_SHOWNOACTIVATE;
    if (newstate & Qt::WindowActive) {
        max = SW_SHOWMAXIMIZED;
        min = SW_SHOWMINIMIZED;
        normal = SW_SHOWNORMAL;
    }

    if (isWindow()) {
        createWinId();
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));

        // Ensure the initial size is valid, since we store it as normalGeometry below.
        if (!testAttribute(Qt::WA_Resized) && !isVisible())
            adjustSize();

        if ((oldstate & Qt::WindowMaximized) != (newstate & Qt::WindowMaximized)) {
            if (newstate & Qt::WindowMaximized && !(oldstate & Qt::WindowFullScreen))
                d->topData()->normalGeometry = geometry();
            if (isVisible() && !(newstate & Qt::WindowMinimized)) {
                ShowWindow(internalWinId(), (newstate & Qt::WindowMaximized) ? max : normal);
                if (!(newstate & Qt::WindowFullScreen)) {
                    QRect r = d->topData()->normalGeometry;
                    if (!(newstate & Qt::WindowMaximized) && r.width() >= 0) {
                        if (pos() != r.topLeft() || size() !=r.size()) {
                            d->topData()->normalGeometry = QRect(0,0,-1,-1);
                            setGeometry(r);
                        }
                    }
                } else {
                    d->updateFrameStrut();
                }
            }
        }

        if ((oldstate & Qt::WindowFullScreen) != (newstate & Qt::WindowFullScreen)) {
            if (newstate & Qt::WindowFullScreen) {
                if (d->topData()->normalGeometry.width() < 0 && !(oldstate & Qt::WindowMaximized))
                    d->topData()->normalGeometry = geometry();
                d->topData()->savedFlags = GetWindowLongA(internalWinId(), GWL_STYLE);
#ifndef Q_FLATTEN_EXPOSE
                UINT style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP;
#else
                UINT style = WS_POPUP;
#endif
                if (isVisible())
                    style |= WS_VISIBLE;
                SetWindowLongA(internalWinId(), GWL_STYLE, style);
                QRect r = qApp->desktop()->screenGeometry(this);
                UINT swpf = SWP_FRAMECHANGED;
                if (newstate & Qt::WindowActive)
                    swpf |= SWP_NOACTIVATE;

                SetWindowPos(internalWinId(), HWND_TOP, r.left(), r.top(), r.width(), r.height(), swpf);
                d->updateFrameStrut();
            } else {
                UINT style = d->topData()->savedFlags;
                if (isVisible())
                    style |= WS_VISIBLE;
                SetWindowLongA(internalWinId(), GWL_STYLE, style);

                UINT swpf = SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE;
                if (newstate & Qt::WindowActive)
                    swpf |= SWP_NOACTIVATE;
                SetWindowPos(internalWinId(), 0, 0, 0, 0, 0, swpf);
                d->updateFrameStrut();

                // preserve maximized state
                if (isVisible())
                    ShowWindow(internalWinId(), (newstate & Qt::WindowMaximized) ? max : normal);

                if (!(newstate & Qt::WindowMaximized)) {
                    QRect r = d->topData()->normalGeometry;
                    d->topData()->normalGeometry = QRect(0,0,-1,-1);
                    if (r.isValid())
                        setGeometry(r);
                }
            }
        }

        if ((oldstate & Qt::WindowMinimized) != (newstate & Qt::WindowMinimized)) {
            if (isVisible())
                ShowWindow(internalWinId(), (newstate & Qt::WindowMinimized) ? min :
                                    (newstate & Qt::WindowMaximized) ? max : normal);
        }
    }


    data->window_state = newstate;

    QWindowStateChangeEvent e(oldstate);
    QApplication::sendEvent(this, &e);
}


/*
  \internal
  Platform-specific part of QWidget::hide().
*/

void QWidgetPrivate::hide_sys()
{
    Q_Q(QWidget);
    deactivateWidgetCleanup();
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (q->windowFlags() & Qt::Popup)
        ShowWindow(q->internalWinId(), SW_HIDE);
    else
        SetWindowPos(q->internalWinId(),0, 0,0,0,0, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);
    if (q->isWindow()) {
        if (QWidgetBackingStore *bs = maybeBackingStore())
            bs->releaseBuffer();
    } else {
        invalidateBuffer(q->rect());
    }
    q->setAttribute(Qt::WA_Mapped, false);
}


#ifndef Q_OS_TEMP // ------------------------------------------------

/*
  \internal
  Platform-specific part of QWidget::show().
*/
void QWidgetPrivate::show_sys()
{
    Q_Q(QWidget);
#if defined(QT_NON_COMMERCIAL)
    QT_NC_SHOW_WINDOW
#endif
    if (q->testAttribute(Qt::WA_OutsideWSRange))
        return;
    q->setAttribute(Qt::WA_Mapped);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));

    int sm = SW_SHOWNORMAL;
    bool fakedMaximize = false;
    if (q->isWindow()) {
        if (q->isMinimized())
            sm = SW_SHOWMINIMIZED;
        else if (q->isMaximized()) {
            sm = SW_SHOWMAXIMIZED;
            // Windows will not behave correctly when we try to maximize a window which does not
            // have minimize nor maximize buttons in the window frame. Windows would then ignore
            // non-available geometry, and rather maximize the widget to the full screen, minus the
            // window frame (caption). So, we do a trick here, by adding a maximize button before
            // maximizing the widget, and then remove the maximize button afterwards.
            Qt::WindowFlags &flags = data.window_flags;
            if (flags & Qt::WindowTitleHint &&
                !(flags & (Qt::WindowMinMaxButtonsHint | Qt::FramelessWindowHint))) {
                fakedMaximize = TRUE;
                int style = GetWindowLong(q->internalWinId(), GWL_STYLE);
                SetWindowLong(q->internalWinId(), GWL_STYLE, style | WS_MAXIMIZEBOX);
            }
        }
    }
    if ((q->windowType() == Qt::Popup) || (q->windowType() == Qt::ToolTip) || (q->windowType() == Qt::Tool))
        sm = SW_SHOWNOACTIVATE;

    ShowWindow(q->internalWinId(), sm);

    if (fakedMaximize) {
        int style = GetWindowLong(q->internalWinId(), GWL_STYLE);
        SetWindowLong(q->internalWinId(), GWL_STYLE, style & ~WS_MAXIMIZEBOX);
        SetWindowPos(q->internalWinId(), 0, 0, 0, 0, 0,
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER
                     | SWP_FRAMECHANGED);
    }

    if (IsIconic(q->internalWinId()))
        data.window_state |= Qt::WindowMinimized;
    if (IsZoomed(q->internalWinId()))
        data.window_state |= Qt::WindowMaximized;

    // synchronously repaint windows, i.e. splashscreen
    // child widgets get the paint event through the
    // backingstore
    if (q->isWindow())
        UpdateWindow(q->internalWinId());
    invalidateBuffer(q->rect());
}

#else // Q_OS_TEMP --------------------------------------------------
# if defined(WIN32_PLATFORM_PSPC) && (WIN32_PLATFORM_PSPC < 310)
#  define SHFS_SHOWTASKBAR            0x0001
#  define SHFS_SHOWSIPBUTTON          0x0004
   extern "C" BOOL __stdcall SHFullScreen(HWND hwndRequester, DWORD dwState);
# else
#  include <aygshell.h>
# endif

void QWidget::show_sys()
{
    Q_D(QWidget);
    if (testAttribute(Qt::WA_OutsideWSRange))
        return;
    setAttribute(Qt::WA_Mapped);
    uint sm = SW_SHOW;
    if (isWindow()) {
        switch (d->topData()->showMode) {
        case 1:
            sm = SW_HIDE;
            break;
        case 2:
            {
                int scrnum = qApp->desktop()->screenNumber(this);
                setGeometry(qApp->desktop()->availableGeometry(scrnum));
            }
            // Fall-through
        default:
            sm = SW_SHOW;
            break;
        }
        d->topData()->showMode = 0; // reset
    }

    if ((windowType() == Qt::Tool) || (windowType() == Qt::Popup) || windowType() == Qt::ToolTip)
        sm = SW_SHOWNOACTIVATE;

    ShowWindow(internalWinId(), sm);
    if (isWindow() && sm == SW_SHOW)
        SetForegroundWindow(internalWinId());

    // synchronously repaint windows, i.e. splashscreen
    // child widgets get the paint event through the
    // backingstore
    if (q->isWindow())
        UpdateWindow(internalWinId());
    else
        invalidateBuffer(q->rect());
}

#endif // Q_OS_TEMP -------------------------------------------------

void QWidgetPrivate::setFocus_sys()
{
    Q_Q(QWidget);
    if (q->testAttribute(Qt::WA_WState_Created) && q->window()->windowType() != Qt::Popup)
        SetFocus(q->internalWinId());
}

void QWidgetPrivate::raise_sys()
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    SetWindowPos(q->internalWinId(), HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    if(!q->isWindow())
        invalidateBuffer(q->rect());
}

void QWidgetPrivate::lower_sys()
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    SetWindowPos(q->internalWinId(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    invalidateBuffer(q->rect());
}

void QWidgetPrivate::stackUnder_sys(QWidget* w)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    SetWindowPos(q->internalWinId(), w->internalWinId() , 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    invalidateBuffer(q->rect());
}


/*
  Helper function for non-toplevel widgets. Helps to map Qt's 32bit
  coordinate system to Windpws's 16bit coordinate system.

  This code is duplicated from the X11 code, so any changes there
  should also (most likely) be reflected here.

  (In all comments below: s/X/Windows/g)
 */

void QWidgetPrivate::setWSGeometry(bool dontShow)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));

    /*
      There are up to four different coordinate systems here:
      Qt coordinate system for this widget.
      X coordinate system for this widget (relative to wrect).
      Qt coordinate system for parent
      X coordinate system for parent (relative to parent's wrect).
     */
    QRect validRange(-XCOORD_MAX,-XCOORD_MAX, 2*XCOORD_MAX, 2*XCOORD_MAX);
    QRect wrectRange(-WRECT_MAX,-WRECT_MAX, 2*WRECT_MAX, 2*WRECT_MAX);
    QRect wrect;
    //xrect is the X geometry of my X widget. (starts out in  parent's Qt coord sys, and ends up in parent's X coord sys)
    QRect xrect = data.crect;

    QRect parentWRect = q->parentWidget()->data->wrect;

    if (parentWRect.isValid()) {
        // parent is clipped, and we have to clip to the same limit as parent
        if (!parentWRect.contains(xrect)) {
            xrect &= parentWRect;
            wrect = xrect;
            //translate from parent's to my Qt coord sys
            wrect.translate(-data.crect.topLeft());
        }
        //translate from parent's Qt coords to parent's X coords
        xrect.translate(-parentWRect.topLeft());

    } else {
        // parent is not clipped, we may or may not have to clip

        if (data.wrect.isValid() && QRect(QPoint(),data.crect.size()).contains(data.wrect)) {
            // This is where the main optimization is: we are already
            // clipped, and if our clip is still valid, we can just
            // move our window, and do not need to move or clip
            // children

            QRect vrect = xrect & q->parentWidget()->rect();
            vrect.translate(-data.crect.topLeft()); //the part of me that's visible through parent, in my Qt coords
            if (data.wrect.contains(vrect)) {
                xrect = data.wrect;
                xrect.translate(data.crect.topLeft());
                MoveWindow(q->internalWinId(), xrect.x(), xrect.y(), xrect.width(), xrect.height(), true);
                return;
            }
        }

        if (!validRange.contains(xrect)) {
            // we are too big, and must clip
            xrect &=wrectRange;
            wrect = xrect;
            wrect.translate(-data.crect.topLeft());
            //parent's X coord system is equal to parent's Qt coord
            //sys, so we don't need to map xrect.
        }

    }


    // unmap if we are outside the valid window system coord system
    bool outsideRange = !xrect.isValid();
    bool mapWindow = false;
    if (q->testAttribute(Qt::WA_OutsideWSRange) != outsideRange) {
        q->setAttribute(Qt::WA_OutsideWSRange, outsideRange);
        if (outsideRange) {
            ShowWindow(q->internalWinId(), SW_HIDE);
            q->setAttribute(Qt::WA_Mapped, false);
        } else if (!q->isHidden()) {
            mapWindow = true;
        }
    }

    if (outsideRange)
        return;

    bool jump = (data.wrect != wrect);
    data.wrect = wrect;

    // and now recursively for all children...
    for (int i = 0; i < children.size(); ++i) {
        QObject *object = children.at(i);
        if (object->isWidgetType()) {
            QWidget *w = static_cast<QWidget *>(object);
            if (!w->isWindow() && w->testAttribute(Qt::WA_WState_Created))
                w->d_func()->setWSGeometry();
        }
    }

    // move ourselves to the new position and map (if necessary) after
    // the movement. Rationale: moving unmapped windows is much faster
    // than moving mapped windows
    MoveWindow(q->internalWinId(), xrect.x(), xrect.y(), xrect.width(), xrect.height(), !jump);
    if (mapWindow && !dontShow) {
        q->setAttribute(Qt::WA_Mapped);
        ShowWindow(q->internalWinId(), SW_SHOWNOACTIVATE);
    }

    if (jump)
        InvalidateRect(q->internalWinId(), 0, false);

}

//
// The internal qWinRequestConfig, defined in qapplication_win.cpp, stores move,
// resize and setGeometry requests for a widget that is already
// processing a config event. The purpose is to avoid recursion.
//
void qWinRequestConfig(WId, int, int, int, int, int);

void QWidgetPrivate::setGeometry_sys(int x, int y, int w, int h, bool isMove)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (extra) {                                // any size restrictions?
        w = qMin(w,extra->maxw);
        h = qMin(h,extra->maxh);
        w = qMax(w,extra->minw);
        h = qMax(h,extra->minh);
    }
    if (q->isWindow())
        topData()->normalGeometry = QRect(0, 0, -1, -1);

    QSize  oldSize(q->size());
    QPoint oldPos(q->pos());

    if (!q->isWindow())
        isMove = (data.crect.topLeft() != QPoint(x, y));
    bool isResize = w != oldSize.width() || h != oldSize.height();

    if (!isMove && !isResize)
        return;

    if (isResize && !q->testAttribute(Qt::WA_StaticContents))
        ValidateRgn(q->internalWinId(), 0);

    if (isResize)
        data.window_state &= ~Qt::WindowMaximized;

    if (data.window_state & Qt::WindowFullScreen) {
        QTLWExtra *top = topData();

        if (q->isWindow()) {
            // We need to update these flags when we remove the full screen state
            // or the frame will not be updated
            UINT style = top->savedFlags;
            if (q->isVisible())
                style |= WS_VISIBLE;
            SetWindowLongA(q->internalWinId(), GWL_STYLE, style);

            UINT swpf = SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE;
            if (data.window_state & Qt::WindowActive)
                swpf |= SWP_NOACTIVATE;
            SetWindowPos(q->internalWinId(), 0, 0, 0, 0, 0, swpf);
            updateFrameStrut();
        }
        data.window_state &= ~Qt::WindowFullScreen;
        topData()->savedFlags = 0;
    }

    if (q->testAttribute(Qt::WA_WState_ConfigPending)) {        // processing config event
        qWinRequestConfig(q->internalWinId(), isMove ? 2 : 1, x, y, w, h);
    } else {
        q->setAttribute(Qt::WA_WState_ConfigPending);
        if (q->windowType() == Qt::Desktop) {
            data.crect.setRect(x, y, w, h);
        } else if (q->isWindow()) {
            QRect fs(frameStrut());
            if (extra) {
                fs.setLeft(x - fs.left());
                fs.setTop(y - fs.top());
                fs.setRight((x + w - 1) + fs.right());
                fs.setBottom((y + h - 1) + fs.bottom());
            }
            if (w == 0 || h == 0) {
                q->setAttribute(Qt::WA_OutsideWSRange, true);
                if (q->isVisible() && q->testAttribute(Qt::WA_Mapped))
                    hide_sys();
                data.crect = QRect(x, y, w, h);
            } else if (q->isVisible() && q->testAttribute(Qt::WA_OutsideWSRange)) {
                q->setAttribute(Qt::WA_OutsideWSRange, false);

                // put the window in its place and show it
                MoveWindow(q->internalWinId(), fs.x(), fs.y(), fs.width(), fs.height(), true);
                RECT rect;
                GetClientRect(q->internalWinId(), &rect);
                data.crect.setRect(x, y, rect.right - rect.left, rect.bottom - rect.top);

                show_sys();
            } else {
                // If the window is hidden and in maximized state or minimized, instead of moving the
                // window, set the normal position of the window.
                WINDOWPLACEMENT wndpl;
                GetWindowPlacement(q->internalWinId(), &wndpl);
                if ((wndpl.showCmd == SW_MAXIMIZE && !IsWindowVisible(q->internalWinId())) || wndpl.showCmd == SW_SHOWMINIMIZED) {
                    RECT normal = {fs.x(), fs.y(), fs.x()+fs.width(), fs.y()+fs.height()};
                    wndpl.rcNormalPosition = normal;
                    wndpl.showCmd = wndpl.showCmd == SW_SHOWMINIMIZED ? SW_SHOWMINIMIZED : SW_HIDE;
                    SetWindowPlacement(q->internalWinId(), &wndpl);
                } else {
                    MoveWindow(q->internalWinId(), fs.x(), fs.y(), fs.width(), fs.height(), true);
                }
                if (!q->isVisible())
                    InvalidateRect(q->internalWinId(), 0, FALSE);
                RECT rect;
                GetClientRect(q->internalWinId(), &rect);
                data.crect.setRect(x, y, rect.right - rect.left, rect.bottom - rect.top);
            }
        } else {
            QRect oldGeom(data.crect);
            data.crect.setRect(x, y, w, h);
            if (q->isVisible()) {
                if (!isResize) {
                    moveRect(QRect(oldPos, oldSize), x - oldGeom.x(), y - oldGeom.y());
                } else {
                    invalidateBuffer(q->rect());
                    QRegion oldRegion(QRect(oldPos, oldSize));
                    if (!q->mask().isEmpty())
                        oldRegion &= q->mask().translated(oldPos);
                    q->parentWidget()->d_func()->invalidateBuffer(oldRegion);
                }
            }
            if (q->testAttribute(Qt::WA_WState_Created))
                setWSGeometry();
        }
        q->setAttribute(Qt::WA_WState_ConfigPending, false);
    }

    if (q->isWindow() && q->isVisible() && isResize) {
        invalidateBuffer(q->rect()); //after the resize
    }

    // Process events immediately rather than in translateConfigEvent to
    // avoid windows message process delay.
    if (q->isVisible()) {
        if (isMove && q->pos() != oldPos) {
            QMoveEvent e(q->pos(), oldPos);
            QApplication::sendEvent(q, &e);
        }
        if (isResize) {
            QResizeEvent e(q->size(), oldSize);
            QApplication::sendEvent(q, &e);
        }
    } else {
        if (isMove && q->pos() != oldPos)
            q->setAttribute(Qt::WA_PendingMoveEvent, true);
        if (isResize)
            q->setAttribute(Qt::WA_PendingResizeEvent, true);
    }
}



void QWidgetPrivate::setConstraints_sys()
{
}

void QWidgetPrivate::scroll_sys(int dx, int dy)
{
    Q_Q(QWidget);
    scrollChildren(dx, dy);

    if (!QWidgetBackingStore::paintOnScreen(q)) {
        scrollRect(q->rect(), dx, dy);
    } else {
        UINT flags = SW_INVALIDATE;
        if (!q->testAttribute(Qt::WA_OpaquePaintEvent))
            flags |= SW_ERASE;
        Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
        ScrollWindowEx(q->internalWinId(), dx, dy, 0, 0, 0, 0, flags);
        UpdateWindow(q->internalWinId());
    }
}

void QWidgetPrivate::scroll_sys(int dx, int dy, const QRect &r)
{
    Q_Q(QWidget);

    if (!QWidgetBackingStore::paintOnScreen(q)) {
        scrollRect(r, dx, dy);
    } else {
        RECT wr;
        wr.top = r.top();
        wr.left = r.left();
        wr.bottom = r.bottom()+1;
        wr.right = r.right()+1;

        UINT flags = SW_INVALIDATE;
        if (!q->testAttribute(Qt::WA_OpaquePaintEvent))
            flags |= SW_ERASE;
        Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
        ScrollWindowEx(q->internalWinId(), dx, dy, &wr, &wr, 0, 0, flags);
        UpdateWindow(q->internalWinId());
    }
}

extern Q_GUI_EXPORT HDC qt_win_display_dc();

int QWidget::metric(PaintDeviceMetric m) const
{
    Q_D(const QWidget);
    int val;
    if (m == PdmWidth) {
        val = data->crect.width();
    } else if (m == PdmHeight) {
        val = data->crect.height();
    } else {
        HDC gdc = qt_win_display_dc();
        switch (m) {
        case PdmDpiX:
        case PdmPhysicalDpiX:
            val = GetDeviceCaps(gdc, LOGPIXELSX);
            break;
        case PdmDpiY:
        case PdmPhysicalDpiY:
            val = GetDeviceCaps(gdc, LOGPIXELSY);
            break;
        case PdmWidthMM:
            val = data->crect.width()
                    * GetDeviceCaps(gdc, HORZSIZE)
                    / GetDeviceCaps(gdc, HORZRES);
            break;
        case PdmHeightMM:
            val = data->crect.height()
                    * GetDeviceCaps(gdc, VERTSIZE)
                    / GetDeviceCaps(gdc, VERTRES);
            break;
        case PdmNumColors:
            if (GetDeviceCaps(gdc, RASTERCAPS) & RC_PALETTE)
                val = GetDeviceCaps(gdc, SIZEPALETTE);
            else {
                int bpp = GetDeviceCaps((HDC)d->hd, BITSPIXEL);
                if(bpp==32)
                    val = INT_MAX;
                else if(bpp<=8)
                    val = GetDeviceCaps((HDC)d->hd, NUMCOLORS);
                else
                    val = 1 << (bpp * GetDeviceCaps((HDC)d->hd, PLANES));
            }
            break;
        case PdmDepth:
            val = GetDeviceCaps(gdc, BITSPIXEL);
            break;
        default:
            val = 0;
            qWarning("QWidget::metric: Invalid metric command");
        }
    }
    return val;
}

void QWidgetPrivate::createSysExtra()
{
    extra->dropTarget = 0;
#ifndef QT_NO_DIRECT3D
    extra->had_auto_fill_bg = 0;
    extra->had_paint_on_screen = 0;
    extra->had_no_system_bg = 0;
#endif
}

void QWidgetPrivate::deleteSysExtra()
{
}

void QWidgetPrivate::createTLSysExtra()
{
    extra->topextra->winIconSmall = 0;
    extra->topextra->winIconBig = 0;
    extra->topextra->backingStore = 0;
}

void QWidgetPrivate::deleteTLSysExtra()
{
    if (extra->topextra->winIconSmall)
        DestroyIcon(extra->topextra->winIconSmall);
    if (extra->topextra->winIconBig)
        DestroyIcon(extra->topextra->winIconBig);
    delete extra->topextra->backingStore;
    extra->topextra->backingStore = 0;
}

void QWidgetPrivate::registerDropSite(bool on)
{
    Q_Q(QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created))
        return;
    // Enablement is defined by d->extra->dropTarget != 0.
    if (on) {
        // Turn on.
        createExtra();
        QWExtra *extra = extraData();
        if (!extra->dropTarget)
            extra->dropTarget = qt_olednd_register(q);
    } else {
        // Turn off.
        QWExtra *extra = extraData();
        if (extra && extra->dropTarget) {
            qt_olednd_unregister(q, extra->dropTarget);
            extra->dropTarget = 0;
        }
    }
}

void QWidget::setMask(const QRegion &region)
{
    Q_D(QWidget);
    d->createExtra();
    if (region == d->extra->mask)
        return;
#ifndef QT_NO_BACKINGSTORE
    QRegion parentR;
    if (!isWindow())
        parentR = d->extra->mask.isEmpty() ? QRegion(rect()) : d->extra->mask ;
#endif

    d->extra->mask = region;
    if (!testAttribute(Qt::WA_WState_Created))
        return;

    d->setMask_sys(region);

#ifndef QT_NO_BACKINGSTORE
    if (isVisible()) {
        if (!isWindow()) {
            parentR += d->extra->mask;
            parentWidget()->update(parentR.translated(geometry().topLeft()));
        }
        if (!testAttribute(Qt::WA_PaintOnScreen))
            update();
    }
#endif
}

void QWidgetPrivate::setMask_sys(const QRegion &region)
{
    // from qregion_win.cpp
    extern HRGN qt_tryCreateRegion(QRegion::RegionType type, int left, int top, int right, int bottom);
    
    Q_Q(QWidget);
    // Since SetWindowRegion takes ownership, and we need to translate,
    // we take a copy.
    HRGN wr = qt_tryCreateRegion(QRegion::Rectangle, 0,0,0,0);
    CombineRgn(wr, region.handle(), 0, RGN_COPY);

    QPoint offset = (q->isWindow()
                     ? frameStrut().topLeft()
                     : QPoint(0, 0));
    OffsetRgn(wr, offset.x(), offset.y());

    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    SetWindowRgn(data.winid, wr, true);
}

void QWidget::setMask(const QBitmap &bitmap)
{
    QRegion region(bitmap);
    setMask(region);
}

void QWidget::clearMask()
{
    Q_D(QWidget);
    if (!d->extra || d->extra->mask.isEmpty())
        return;

    if(QWExtra *extra = d->extraData())
        extra->mask = QRegion();
    if (testAttribute(Qt::WA_WState_Created))
        SetWindowRgn(internalWinId(), 0, true);
}

void QWidgetPrivate::updateFrameStrut()
{
    Q_Q(QWidget);

    if (!q->testAttribute(Qt::WA_WState_Created))
        return;

    RECT rect = {0,0,0,0};

    QTLWExtra *top = topData();
    uint exstyle = QT_WA_INLINE(GetWindowLongW(q->internalWinId(), GWL_EXSTYLE),
                                GetWindowLongA(q->internalWinId(), GWL_EXSTYLE));
    uint style = QT_WA_INLINE(GetWindowLongW(q->internalWinId(), GWL_STYLE),
                              GetWindowLongA(q->internalWinId(), GWL_STYLE));

    if (AdjustWindowRectEx(&rect, style & ~WS_OVERLAPPED, FALSE, exstyle)) {
        top->frameStrut.setCoords(-rect.left, -rect.top, rect.right, rect.bottom);
        data.fstrut_dirty = false;
    }
}

void QWidget::setWindowOpacity(qreal level)
{
    Q_D(QWidget);
    if(!isWindow())
        return;

    static bool function_resolved = false;
    if (!function_resolved) {
        ptrSetLayeredWindowAttributes =
            (PtrSetLayeredWindowAttributes) QLibrary::resolve(QLatin1String("user32"),
                                                              "SetLayeredWindowAttributes");
        function_resolved = true;
    }

    if (!ptrSetLayeredWindowAttributes)
        return;

    level = qBound(0.0, level, 1.0);
    d->topData()->opacity = (uchar)(level * 255);
    if (!testAttribute(Qt::WA_WState_Created))
        return;
    int wl = GetWindowLongA(internalWinId(), GWL_EXSTYLE);

    if (level != 1.0) {
        if ((wl&Q_WS_EX_LAYERED) == 0)
            SetWindowLongA(internalWinId(), GWL_EXSTYLE, wl|Q_WS_EX_LAYERED);
    } else if (wl&Q_WS_EX_LAYERED) {
        SetWindowLongA(internalWinId(), GWL_EXSTYLE, wl & ~Q_WS_EX_LAYERED);
    }

    bool exposeTrick = (level != 1.0) && (d->topData()->opacity == 1.0) && isVisible();
    if (exposeTrick)
        d->hide_sys(); // Work around Windows (non-)expose bug
    (*ptrSetLayeredWindowAttributes)(internalWinId(), 0, (int)(level * 255), Q_LWA_ALPHA);
    if (exposeTrick)
        d->show_sys(); // Work around Windows (non-)expose bug
}

qreal QWidget::windowOpacity() const
{
    Q_D(const QWidget);
    return (isWindow() && d->maybeTopData()) ? d->maybeTopData()->opacity / 255. : 1.0;
}

class QGlobalRasterPaintEngine: public QRasterPaintEngine
{
public:
    inline QGlobalRasterPaintEngine() : QRasterPaintEngine() { setFlushOnEnd(false); }
};
Q_GLOBAL_STATIC(QGlobalRasterPaintEngine, globalRasterPaintEngine)

#ifndef QT_NO_DIRECT3D
class QGlobal3DEngine
{
public:
    QDirect3DPaintEngine *pointer;
    bool destroyed;

    inline QGlobal3DEngine()
        : pointer(0), destroyed(false)
    { }

    inline ~QGlobal3DEngine()
    {
        delete pointer;
        pointer = 0;
        destroyed = true;
    }
};

static void cleanup_d3d_engine();

QDirect3DPaintEngine *qt_d3dEngine()
{
    static QGlobal3DEngine this_d3dEngine;
    if (!this_d3dEngine.pointer && !this_d3dEngine.destroyed) {
        QDirect3DPaintEngine *x = new QDirect3DPaintEngine;
        if (!q_atomic_test_and_set_ptr(&this_d3dEngine.pointer, 0, x))
            delete x;
        qAddPostRoutine(cleanup_d3d_engine);
    }
    return this_d3dEngine.pointer;
}

static void cleanup_d3d_engine() {
    qt_d3dEngine()->cleanup();
}

#endif

QPaintEngine *QWidget::paintEngine() const
{
#ifndef QT_NO_DIRECT3D
    if ((qApp->testAttribute(Qt::AA_MSWindowsUseDirect3DByDefault)
         || testAttribute(Qt::WA_MSWindowsUseDirect3D))
        && qt_d3dEngine()->hasDirect3DSupport())
    {
        QDirect3DPaintEngine *engine = qt_d3dEngine();
        if (qApp->testAttribute(Qt::AA_MSWindowsUseDirect3DByDefault))
            engine->setFlushOnEnd(false);
        else
            engine->setFlushOnEnd(true);
        return engine;
    }
#endif
    Q_D(const QWidget);
    QPaintEngine *globalEngine = globalRasterPaintEngine();
    if (globalEngine->isActive()) {
        if (d->extraPaintEngine)
            return d->extraPaintEngine;
        QRasterPaintEngine *engine = new QRasterPaintEngine();
        engine->setFlushOnEnd(false);
        const_cast<QWidget *>(this)->d_func()->extraPaintEngine = engine;
        return d->extraPaintEngine;
    }
    return globalEngine;
}

void QWidgetPrivate::setModal_sys()
{
}
