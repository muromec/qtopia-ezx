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
#include "qevent.h"
#include "private/qeventdispatcher_win_p.h"
#include "qeventloop.h"
#include "qclipboard.h"
#include "qcursor.h"
#include "qdatetime.h"
#include "qpointer.h"
#include "qhash.h"
#include "qlibrary.h"
#include "qmetaobject.h"
#include "qmime.h"
#include "qpainter.h"
#include "qpixmapcache.h"
#include "qsessionmanager.h"
#include "qstyle.h"
#include "qwhatsthis.h" // ######## dependency
#include "qwidget.h"
#include "qcolormap.h"
#include "qlayout.h"
#include "qtooltip.h"
#include "qt_windows.h"
#if defined(QT_NON_COMMERCIAL)
#include "qnc_win.h"
#endif
#include "private/qwininputcontext_p.h"
#include "private/qcursor_p.h"
#include "private/qmath_p.h"
#include "private/qapplication_p.h"
#include "private/qbackingstore_p.h"
#include "qdebug.h"
#include <private/qkeymapper_p.h>

#ifndef QT_NO_THREAD
#include "qmutex.h"
#endif

#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"

#include <oleacc.h>
#ifndef WM_GETOBJECT
#define WM_GETOBJECT                    0x003D
#endif

extern IAccessible *qt_createWindowsAccessible(QAccessibleInterface *object);
#endif // QT_NO_ACCESSIBILITY

#if WINVER >= 0x0600
#include <winuser.h>
#else
#include <winable.h>
#endif

#ifndef FLASHW_STOP
typedef struct {
    UINT  cbSize;
    HWND  hwnd;
    DWORD dwFlags;
    UINT  uCount;
    DWORD dwTimeout;
} FLASHWINFO, *PFLASHWINFO;
#define FLASHW_STOP         0
#define FLASHW_CAPTION      0x00000001
#define FLASHW_TRAY         0x00000002
#define FLASHW_ALL          (FLASHW_CAPTION | FLASHW_TRAY)
#define FLASHW_TIMER        0x00000004
#define FLASHW_TIMERNOFG    0x0000000C
#endif /* FLASHW_STOP */
typedef BOOL (WINAPI *PtrFlashWindowEx)(PFLASHWINFO pfwi);
static PtrFlashWindowEx pFlashWindowEx = 0;

#include <windowsx.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>

#ifdef Q_OS_TEMP
#include <sipapi.h>
#endif

#define PACKETDATA  (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_TANGENT_PRESSURE \
                     | PK_ORIENTATION | PK_CURSOR | PK_Z)
#define PACKETMODE  0

extern bool qt_tabletChokeMouse;

#include <wintab.h>
#ifndef CSR_TYPE
#define CSR_TYPE 20 // Some old Wacom wintab.h may not provide this constant.
#endif
#include <pktdef.h>

typedef HCTX (API *PtrWTOpen)(HWND, LPLOGCONTEXT, BOOL);
typedef BOOL (API *PtrWTClose)(HCTX);
typedef UINT (API *PtrWTInfo)(UINT, UINT, LPVOID);
typedef BOOL (API *PtrWTEnable)(HCTX, BOOL);
typedef BOOL (API *PtrWTOverlap)(HCTX, BOOL);
typedef int  (API *PtrWTPacketsGet)(HCTX, int, LPVOID);
typedef BOOL (API *PtrWTGet)(HCTX, LPLOGCONTEXT);
typedef int  (API *PtrWTQueueSizeGet)(HCTX);
typedef BOOL (API *PtrWTQueueSizeSet)(HCTX, int);

static PtrWTInfo ptrWTInfo = 0;
static PtrWTEnable ptrWTEnable = 0;
static PtrWTOverlap ptrWTOverlap = 0;
static PtrWTPacketsGet ptrWTPacketsGet = 0;
static PtrWTGet ptrWTGet = 0;

static PACKET localPacketBuf[QT_TABLET_NPACKETQSIZE];  // our own tablet packet queue.
HCTX qt_tablet_context;  // the hardware context for the tablet (like a window handle)
bool qt_tablet_tilt_support;
static void tabletInit(UINT wActiveCsr, HCTX hTab);
static void initWinTabFunctions();        // resolve the WINTAB api functions

typedef QHash<UINT, QTabletDeviceData> QTabletCursorInfo;
Q_GLOBAL_STATIC(QTabletCursorInfo, tCursorInfo)
QTabletDeviceData currentTabletPointer;

// from qregion_win.cpp
extern HRGN qt_tryCreateRegion(QRegion::RegionType type, int left, int top, int right, int bottom);

Q_CORE_EXPORT bool winPeekMessage(MSG* msg, HWND hWnd, UINT wMsgFilterMin,
                            UINT wMsgFilterMax, UINT wRemoveMsg);
Q_CORE_EXPORT bool winPostMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#if defined(__CYGWIN32__)
#define __INSIDE_CYGWIN32__
#include <mywinsock.h>
#endif

// support for on-the-fly changes of the XP theme engine
#ifndef WM_THEMECHANGED
#define WM_THEMECHANGED                 0x031A
#endif
#ifndef COLOR_MENUHILIGHT
#define COLOR_MENUHILIGHT                29
#define COLOR_MENUBAR                        30
#endif

// support for xbuttons
#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN                  0x020B
#define WM_XBUTTONUP                    0x020C
#define WM_XBUTTONDBLCLK                0x020D
#define GET_KEYSTATE_WPARAM(wParam)     (LOWORD(wParam))
#define GET_XBUTTON_WPARAM(wParam)      (HIWORD(wParam))
#define XBUTTON1      0x0001
#define XBUTTON2      0x0002
#define MK_XBUTTON1         0x0020
#define MK_XBUTTON2         0x0040
#endif

// support for multi-media-keys on ME/2000/XP
#ifndef WM_APPCOMMAND
#define WM_APPCOMMAND                   0x0319

#define FAPPCOMMAND_MOUSE 0x8000
#define FAPPCOMMAND_KEY   0
#define FAPPCOMMAND_OEM   0x1000
#define FAPPCOMMAND_MASK  0xF000
#define GET_APPCOMMAND_LPARAM(lParam) ((short)(HIWORD(lParam) & ~FAPPCOMMAND_MASK))
#define GET_DEVICE_LPARAM(lParam)     ((WORD)(HIWORD(lParam) & FAPPCOMMAND_MASK))
#define GET_MOUSEORKEY_LPARAM         GET_DEVICE_LPARAM
#define GET_FLAGS_LPARAM(lParam)      (LOWORD(lParam))
#define GET_KEYSTATE_LPARAM(lParam)   GET_FLAGS_LPARAM(lParam)

#define APPCOMMAND_BROWSER_BACKWARD       1
#define APPCOMMAND_BROWSER_FORWARD        2
#define APPCOMMAND_BROWSER_REFRESH        3
#define APPCOMMAND_BROWSER_STOP           4
#define APPCOMMAND_BROWSER_SEARCH         5
#define APPCOMMAND_BROWSER_FAVORITES      6
#define APPCOMMAND_BROWSER_HOME           7
#define APPCOMMAND_VOLUME_MUTE            8
#define APPCOMMAND_VOLUME_DOWN            9
#define APPCOMMAND_VOLUME_UP              10
#define APPCOMMAND_MEDIA_NEXTTRACK        11
#define APPCOMMAND_MEDIA_PREVIOUSTRACK    12
#define APPCOMMAND_MEDIA_STOP             13
#define APPCOMMAND_MEDIA_PLAY_PAUSE       14
#define APPCOMMAND_LAUNCH_MAIL            15
#define APPCOMMAND_LAUNCH_MEDIA_SELECT    16
#define APPCOMMAND_LAUNCH_APP1            17
#define APPCOMMAND_LAUNCH_APP2            18
#define APPCOMMAND_BASS_DOWN              19
#define APPCOMMAND_BASS_BOOST             20
#define APPCOMMAND_BASS_UP                21
#define APPCOMMAND_TREBLE_DOWN            22
#define APPCOMMAND_TREBLE_UP              23

// New commands from Windows XP (some even Sp1)
#ifndef APPCOMMAND_MICROPHONE_VOLUME_MUTE
#define APPCOMMAND_MICROPHONE_VOLUME_MUTE 24
#define APPCOMMAND_MICROPHONE_VOLUME_DOWN 25
#define APPCOMMAND_MICROPHONE_VOLUME_UP   26
#define APPCOMMAND_HELP                   27
#define APPCOMMAND_FIND                   28
#define APPCOMMAND_NEW                    29
#define APPCOMMAND_OPEN                   30
#define APPCOMMAND_CLOSE                  31
#define APPCOMMAND_SAVE                   32
#define APPCOMMAND_PRINT                  33
#define APPCOMMAND_UNDO                   34
#define APPCOMMAND_REDO                   35
#define APPCOMMAND_COPY                   36
#define APPCOMMAND_CUT                    37
#define APPCOMMAND_PASTE                  38
#define APPCOMMAND_REPLY_TO_MAIL          39
#define APPCOMMAND_FORWARD_MAIL           40
#define APPCOMMAND_SEND_MAIL              41
#define APPCOMMAND_SPELL_CHECK            42
#define APPCOMMAND_DICTATE_OR_COMMAND_CONTROL_TOGGLE    43
#define APPCOMMAND_MIC_ON_OFF_TOGGLE      44
#define APPCOMMAND_CORRECTION_LIST        45
#define APPCOMMAND_MEDIA_PLAY             46
#define APPCOMMAND_MEDIA_PAUSE            47
#define APPCOMMAND_MEDIA_RECORD           48
#define APPCOMMAND_MEDIA_FAST_FORWARD     49
#define APPCOMMAND_MEDIA_REWIND           50
#define APPCOMMAND_MEDIA_CHANNEL_UP       51
#define APPCOMMAND_MEDIA_CHANNEL_DOWN     52
#endif // APPCOMMAND_MICROPHONE_VOLUME_MUTE

#endif // WM_APPCOMMAND

UINT WM_QT_REPAINT = 0;
static UINT WM95_MOUSEWHEEL = 0;

#if (_WIN32_WINNT < 0x0400)
// This struct is defined in winuser.h if the _WIN32_WINNT >= 0x0400 -- in the
// other cases we have to define it on our own.
typedef struct tagTRACKMOUSEEVENT {
    DWORD cbSize;
    DWORD dwFlags;
    HWND  hwndTrack;
    DWORD dwHoverTime;
} TRACKMOUSEEVENT, *LPTRACKMOUSEEVENT;
#endif
#ifndef WM_MOUSELEAVE
#define WM_MOUSELEAVE                   0x02A3
#endif

#include "private/qwidget_p.h"

static int translateButtonState(int s, int type, int button);

// ##### get rid of this!
QRgb qt_colorref2qrgb(COLORREF col)
{
    return qRgb(GetRValue(col),GetGValue(col),GetBValue(col));
}


/*****************************************************************************
  Internal variables and functions
 *****************************************************************************/

extern void qt_syncBackingStore(QRegion rgn, QWidget *widget);
extern void qt_syncBackingStore(QRegion rgn, QWidget *widget, bool);
extern Q_CORE_EXPORT char      appName[];
extern Q_CORE_EXPORT char      appFileName[];
extern Q_CORE_EXPORT HINSTANCE appInst;                        // handle to app instance
extern Q_CORE_EXPORT HINSTANCE appPrevInst;                        // handle to prev app instance
extern Q_CORE_EXPORT int appCmdShow;                                // main window show command
static HWND         curWin                = 0;                // current window
static HDC         displayDC        = 0;                // display device context
#ifdef Q_OS_TEMP
static UINT         appUniqueID        = 0;                // application id
#endif

// Session management
static bool        sm_blockUserInput    = false;
static bool        sm_smActive             = false;
extern QSessionManager* qt_session_manager_self;
static bool        sm_cancel;

static bool replayPopupMouseEvent = false; // replay handling when popups close

// ignore the next release event if return from a modal widget
Q_GUI_EXPORT bool qt_win_ignoreNextMouseReleaseEvent = false;

#if defined(QT_DEBUG)
static bool        appNoGrab        = false;        // mouse/keyboard grabbing
#endif

static bool        app_do_modal           = false;        // modal mode
extern QWidgetList *qt_modal_stack;
extern QDesktopWidget *qt_desktopWidget;
static QWidget *popupButtonFocus   = 0;
static bool        qt_try_modal(QWidget *, MSG *, int& ret);

QWidget               *qt_button_down = 0;                // widget got last button-down

static HWND        autoCaptureWnd = 0;
static void        setAutoCapture(HWND);                // automatic capture
static void        releaseAutoCapture();

static void     unregWinClasses();

extern QCursor *qt_grab_cursor();

#if defined(Q_WS_WIN)
#define __export
#endif

extern "C" LRESULT CALLBACK QtWndProc(HWND, UINT, WPARAM, LPARAM);

class QETWidget : public QWidget                // event translator widget
{
public:
    QWExtra    *xtra() { return d_func()->extraData(); }
    QTLWExtra  *topData() { return d_func()->topData(); }
    QWidgetData *dataPtr() { return data; }
    QRect frameStrut() const { return d_func()->frameStrut(); }
    bool        winEvent(MSG *m, long *r)        { return QWidget::winEvent(m, r); }
    void        markFrameStrutDirty()        { data->fstrut_dirty = 1; }
    bool        translateMouseEvent(const MSG &msg);
    bool        translateWheelEvent(const MSG &msg);
    bool        translatePaintEvent(const MSG &msg);
    bool        translateConfigEvent(const MSG &msg);
    bool        translateCloseEvent(const MSG &msg);
    bool        translateTabletEvent(const MSG &msg, PACKET *localPacketBuf, int numPackets);
    void        repolishStyle(QStyle &style);
    inline void showChildren(bool spontaneous) { d_func()->showChildren(spontaneous); }
    inline void hideChildren(bool spontaneous) { d_func()->hideChildren(spontaneous); }
    inline uint testWindowState(uint teststate){ return dataPtr()->window_state & teststate; }
};


extern QFont qt_LOGFONTtoQFont(LOGFONT& lf,bool scale);

static void qt_set_windows_color_resources()
{
    // Do the color settings
    QPalette pal;
    pal.setColor(QPalette::WindowText,
                 QColor(qt_colorref2qrgb(GetSysColor(COLOR_WINDOWTEXT))));
    pal.setColor(QPalette::Button,
                 QColor(qt_colorref2qrgb(GetSysColor(COLOR_BTNFACE))));
    pal.setColor(QPalette::Light,
                 QColor(qt_colorref2qrgb(GetSysColor(COLOR_BTNHIGHLIGHT))));
    pal.setColor(QPalette::Dark,
                 QColor(qt_colorref2qrgb(GetSysColor(COLOR_BTNSHADOW))));
    pal.setColor(QPalette::Mid, pal.button().color().darker(150));
    pal.setColor(QPalette::Text,
                 QColor(qt_colorref2qrgb(GetSysColor(COLOR_WINDOWTEXT))));
    pal.setColor(QPalette::BrightText,
                 QColor(qt_colorref2qrgb(GetSysColor(COLOR_BTNHIGHLIGHT))));
    pal.setColor(QPalette::Base,
                 QColor(qt_colorref2qrgb(GetSysColor(COLOR_WINDOW))));
    pal.setColor(QPalette::Window,
                 QColor(qt_colorref2qrgb(GetSysColor(COLOR_BTNFACE))));
    pal.setColor(QPalette::ButtonText,
                 QColor(qt_colorref2qrgb(GetSysColor(COLOR_BTNTEXT))));
    pal.setColor(QPalette::Midlight,
                 QColor(qt_colorref2qrgb(GetSysColor(COLOR_3DLIGHT))));
    pal.setColor(QPalette::Shadow,
                 QColor(qt_colorref2qrgb(GetSysColor(COLOR_3DDKSHADOW))));
    pal.setColor(QPalette::Highlight,
                 QColor(qt_colorref2qrgb(GetSysColor(COLOR_HIGHLIGHT))));
    pal.setColor(QPalette::HighlightedText,
                 QColor(qt_colorref2qrgb(GetSysColor(COLOR_HIGHLIGHTTEXT))));
    // ### hardcoded until I find out how to get it from the system settings.
    pal.setColor(QPalette::Link, Qt::blue);
    pal.setColor(QPalette::LinkVisited, Qt::magenta);

    pal.setColor(QPalette::Inactive, QPalette::Button, pal.button().color());
    pal.setColor(QPalette::Inactive, QPalette::Window, pal.background().color());
    pal.setColor(QPalette::Inactive, QPalette::Light, pal.light().color());
    pal.setColor(QPalette::Inactive, QPalette::Dark, pal.dark().color());

    if (QSysInfo::WindowsVersion != QSysInfo::WV_NT && QSysInfo::WindowsVersion != QSysInfo::WV_95) {
        if (pal.midlight() == pal.button())
            pal.setColor(QPalette::Midlight, pal.button().color().lighter(110));
        if (pal.background() != pal.base()) {
            pal.setColor(QPalette::Inactive, QPalette::Highlight, pal.color(QPalette::Inactive, QPalette::Window));
            pal.setColor(QPalette::Inactive, QPalette::HighlightedText, pal.color(QPalette::Inactive, QPalette::Text));
        }
    }

    const QColor bg = pal.background().color();
    const QColor fg = pal.foreground().color(), btn = pal.button().color();
    QColor disabled((fg.red()+btn.red())/2,(fg.green()+btn.green())/2,
                     (fg.blue()+btn.blue())/2);
    pal.setColorGroup(QPalette::Disabled, pal.foreground(), pal.button(), pal.light(),
        pal.dark(), pal.mid(), pal.text(), pal.brightText(), pal.base(), pal.background() );
    pal.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    pal.setColor(QPalette::Disabled, QPalette::Text, disabled);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    pal.setColor(QPalette::Disabled, QPalette::Highlight,
                  QColor(qt_colorref2qrgb(GetSysColor(COLOR_HIGHLIGHT))));
    pal.setColor(QPalette::Disabled, QPalette::HighlightedText,
                  QColor(qt_colorref2qrgb(GetSysColor(COLOR_HIGHLIGHTTEXT))));
    pal.setColor(QPalette::Disabled, QPalette::Base, bg);

    QApplicationPrivate::setSystemPalette(pal);

    QApplicationPrivate::initializeWidgetPaletteHash();

    QColor ttip(qt_colorref2qrgb(GetSysColor(COLOR_INFOBK)));

    QColor ttipText(qt_colorref2qrgb(GetSysColor(COLOR_INFOTEXT)));
    {
        QPalette tiplabel(pal);
        tiplabel.setColor(QPalette::All, QPalette::Button, ttip);
        tiplabel.setColor(QPalette::All, QPalette::Window, ttip);
        tiplabel.setColor(QPalette::All, QPalette::Text, ttipText);
        tiplabel.setColor(QPalette::All, QPalette::WindowText, ttipText);
        tiplabel.setColor(QPalette::All, QPalette::ButtonText, ttipText);
        tiplabel.setColor(QPalette::All, QPalette::Button, ttip);
        tiplabel.setColor(QPalette::All, QPalette::Window, ttip);
        tiplabel.setColor(QPalette::All, QPalette::Text, ttipText);
        tiplabel.setColor(QPalette::All, QPalette::WindowText, ttipText);
        tiplabel.setColor(QPalette::All, QPalette::ButtonText, ttipText);
        const QColor fg = tiplabel.foreground().color(), btn = tiplabel.button().color();
        QColor disabled((fg.red()+btn.red())/2,(fg.green()+btn.green())/2,
                         (fg.blue()+btn.blue())/2);
        tiplabel.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
        tiplabel.setColor(QPalette::Disabled, QPalette::Text, disabled);
        tiplabel.setColor(QPalette::Disabled, QPalette::Base, Qt::white);
        tiplabel.setColor(QPalette::Disabled, QPalette::BrightText, Qt::white);
        QToolTip::setPalette(tiplabel);
    }
}

static void qt_set_windows_resources()
{
    if (QApplication::type() != QApplication::Tty)
        (void) QApplication::style(); // trigger creation of application style
#ifndef Q_OS_TEMP
    QFont menuFont;
    QFont messageFont;
    QFont statusFont;
    QFont titleFont;
    QFont smallTitleFont;
    QFont iconTitleFont;
    QT_WA({
        NONCLIENTMETRICS ncm;
        ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICS, lfMessageFont) + sizeof(LOGFONTW);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize , &ncm, 0);
        menuFont = qt_LOGFONTtoQFont(ncm.lfMenuFont,true);
        messageFont = qt_LOGFONTtoQFont(ncm.lfMessageFont,true);
        statusFont = qt_LOGFONTtoQFont(ncm.lfStatusFont,true);
        titleFont = qt_LOGFONTtoQFont(ncm.lfCaptionFont,true);
        smallTitleFont = qt_LOGFONTtoQFont(ncm.lfSmCaptionFont,true);
        LOGFONTW lfIconTitleFont;
        SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lfIconTitleFont), &lfIconTitleFont, 0);
        iconTitleFont = qt_LOGFONTtoQFont(lfIconTitleFont,true);
    } , {
        // A version
        NONCLIENTMETRICSA ncm;
        ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICSA, lfMessageFont) + sizeof(LOGFONTA);
        SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
        menuFont = qt_LOGFONTtoQFont((LOGFONT&)ncm.lfMenuFont,true);
        messageFont = qt_LOGFONTtoQFont((LOGFONT&)ncm.lfMessageFont,true);
        statusFont = qt_LOGFONTtoQFont((LOGFONT&)ncm.lfStatusFont,true);
        titleFont = qt_LOGFONTtoQFont((LOGFONT&)ncm.lfCaptionFont,true);
        smallTitleFont = qt_LOGFONTtoQFont((LOGFONT&)ncm.lfSmCaptionFont,true);
        LOGFONTA lfIconTitleFont;
        SystemParametersInfoA(SPI_GETICONTITLELOGFONT, sizeof(lfIconTitleFont), &lfIconTitleFont, 0);
        iconTitleFont = qt_LOGFONTtoQFont((LOGFONT&)lfIconTitleFont,true);
    });

    QApplication::setFont(menuFont, "QMenu");
    QApplication::setFont(menuFont, "QMenuBar");
    QApplication::setFont(messageFont, "QMessageBox");
    QApplication::setFont(statusFont, "QTipLabel");
    QApplication::setFont(statusFont, "QStatusBar");
    QApplication::setFont(titleFont, "Q3TitleBar");
    QApplication::setFont(titleFont, "QWorkspaceTitleBar");
    
    if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA && QSysInfo::WindowsVersion < QSysInfo::WV_NT_based) {
        QApplication::setFont(iconTitleFont, "QAbstractItemView");
        QApplication::setFont(iconTitleFont, "QDockWidgetTitle");
    } else {
        //Preserve existing behavior for non-vista platforms in 4.3
        QApplication::setFont(smallTitleFont, "QDockWidgetTitle");
    }

#else
    LOGFONT lf;
    HGDIOBJ stockFont = GetStockObject(SYSTEM_FONT);
    GetObject(stockFont, sizeof(lf), &lf);
    QApplicationPrivate::setSystemFont(qt_LOGFONTtoQFont(lf, true));
#endif// Q_OS_TEMP
    qt_set_windows_color_resources();
}

void QApplicationPrivate::initializeWidgetPaletteHash()
{
    QPalette pal = *QApplicationPrivate::sys_pal;
    QColor menuCol(qt_colorref2qrgb(GetSysColor(COLOR_MENU)));
    QColor menuText(qt_colorref2qrgb(GetSysColor(COLOR_MENUTEXT)));
    BOOL isFlat = 0;
    if ((QSysInfo::WindowsVersion >= QSysInfo::WV_XP
        && QSysInfo::WindowsVersion < QSysInfo::WV_NT_based))
        SystemParametersInfo(0x1022 /*SPI_GETFLATMENU*/, 0, &isFlat, 0);
    QPalette menu(pal);
    // we might need a special color group for the menu.
    menu.setColor(QPalette::Active, QPalette::Button, menuCol);
    menu.setColor(QPalette::Active, QPalette::Text, menuText);
    menu.setColor(QPalette::Active, QPalette::WindowText, menuText);
    menu.setColor(QPalette::Active, QPalette::ButtonText, menuText);
    const QColor fg = menu.foreground().color(), btn = menu.button().color();
    QColor disabled(qt_colorref2qrgb(GetSysColor(COLOR_GRAYTEXT)));
    menu.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    menu.setColor(QPalette::Disabled, QPalette::Text, disabled);
    menu.setColor(QPalette::Disabled, QPalette::Highlight,
                    QColor(qt_colorref2qrgb(GetSysColor(
                                            (QSysInfo::WindowsVersion >= QSysInfo::WV_XP
                                            && QSysInfo::WindowsVersion < QSysInfo::WV_NT_based)
                                            && isFlat ? COLOR_MENUHILIGHT
                                                        : COLOR_HIGHLIGHT))));
    menu.setColor(QPalette::Disabled, QPalette::HighlightedText, disabled);
    menu.setColor(QPalette::Disabled, QPalette::Button,
                    menu.color(QPalette::Active, QPalette::Button));
    menu.setColor(QPalette::Inactive, QPalette::Button,
                    menu.color(QPalette::Active, QPalette::Button));
    menu.setColor(QPalette::Inactive, QPalette::Text,
                    menu.color(QPalette::Active, QPalette::Text));
    menu.setColor(QPalette::Inactive, QPalette::WindowText,
                    menu.color(QPalette::Active, QPalette::WindowText));
    menu.setColor(QPalette::Inactive, QPalette::ButtonText,
                    menu.color(QPalette::Active, QPalette::ButtonText));
    menu.setColor(QPalette::Inactive, QPalette::Highlight,
                    menu.color(QPalette::Active, QPalette::Highlight));
    menu.setColor(QPalette::Inactive, QPalette::HighlightedText,
                    menu.color(QPalette::Active, QPalette::HighlightedText));

    if (QSysInfo::WindowsVersion != QSysInfo::WV_NT && QSysInfo::WindowsVersion != QSysInfo::WV_95)
        menu.setColor(QPalette::Inactive, QPalette::ButtonText,
                        pal.color(QPalette::Inactive, QPalette::Dark));
    QApplication::setPalette(menu, "QMenu");

    if ((QSysInfo::WindowsVersion >= QSysInfo::WV_XP
        && QSysInfo::WindowsVersion < QSysInfo::WV_NT_based) && isFlat) {
        QColor menubar(qt_colorref2qrgb(GetSysColor(COLOR_MENUBAR)));
        menu.setColor(QPalette::Active, QPalette::Button, menubar);
        menu.setColor(QPalette::Disabled, QPalette::Button, menubar);
        menu.setColor(QPalette::Inactive, QPalette::Button, menubar);
    }
    QApplication::setPalette(menu, "QMenuBar");
}

/*****************************************************************************
  qt_init() - initializes Qt for Windows
 *****************************************************************************/

void qt_init(QApplicationPrivate *priv, int)
{

    int argc = priv->argc;
    char **argv = priv->argv;
    int i, j;

  // Get command line params

    j = argc ? 1 : 0;
    for (i=1; i<argc; i++) {
        if (argv[i] && *argv[i] != '-') {
            argv[j++] = argv[i];
            continue;
        }
#if defined(QT_DEBUG)
        if (qstrcmp(argv[i], "-nograb") == 0)
            appNoGrab = !appNoGrab;
        else
#endif // QT_DEBUG
        if (qstrcmp(argv[i], "-direct3d") == 0)
            QApplication::setAttribute(Qt::AA_MSWindowsUseDirect3DByDefault);
        else
            argv[j++] = argv[i];
    }
    if(j < priv->argc) {
        priv->argv[j] = 0;
        priv->argc = j;
    }

    // Get the application name/instance if qWinMain() was not invoked
#ifndef Q_OS_TEMP
    // No message boxes but important ones
    SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
#endif

    if (appInst == 0) {
        QT_WA({
            appInst = GetModuleHandle(0);
        }, {
            appInst = GetModuleHandleA(0);
        });
    }

#ifndef Q_OS_TEMP
    // Initialize OLE/COM
    //         S_OK means success and S_FALSE means that it has already
    //         been initialized
    HRESULT r;
    r = OleInitialize(0);
    if (r != S_OK && r != S_FALSE) {
        qWarning("Qt: Could not initialize OLE (error %x)", (unsigned int)r);
    }
#endif

    // Misc. initialization
#if defined(QT_DEBUG)
    GdiSetBatchLimit(1);
#endif

    // initialize key mapper
    QKeyMapper::changeKeyboard();

    QColormap::initialize();
    QFont::initialize();
    QCursorData::initialize();
    qApp->setObjectName(QLatin1String(appName));

    // default font
    HFONT hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    QFont f(QLatin1String("MS Sans Serif"),8);
    int result = 0;
    QT_WA({
            LOGFONT lf;
            if (result = GetObject(hfont, sizeof(lf), &lf))
                f = qt_LOGFONTtoQFont((LOGFONT&)lf,true);
        } , {
              LOGFONTA lf;
              if (result = GetObjectA(hfont, sizeof(lf), &lf))
                  f = qt_LOGFONTtoQFont((LOGFONT&)lf,true);
          });
    if (result
        && QSysInfo::WindowsVersion >= QSysInfo::WV_2000
        && QSysInfo::WindowsVersion <= QSysInfo::WV_NT_based
        && f.family() == QLatin1String("MS Shell Dlg"))
        f.setFamily(QLatin1String("MS Shell Dlg 2"));
    QApplicationPrivate::setSystemFont(f);

    // QFont::locale_init();  ### Uncomment when it does something on Windows

    if (QApplication::desktopSettingsAware())
        qt_set_windows_resources();

    QT_WA({
        WM95_MOUSEWHEEL = RegisterWindowMessage(L"MSWHEEL_ROLLMSG");
        WM_QT_REPAINT = RegisterWindowMessageW(L"WM_QT_REPAINT");
    } , {
        WM95_MOUSEWHEEL = RegisterWindowMessageA("MSWHEEL_ROLLMSG");
        WM_QT_REPAINT = RegisterWindowMessageA("WM_QT_REPAINT");
    });
    initWinTabFunctions();
    QApplicationPrivate::inputContext = new QWinInputContext(0);
}

/*****************************************************************************
  qt_cleanup() - cleans up when the application is finished
 *****************************************************************************/

void qt_cleanup()
{
    unregWinClasses();
    QPixmapCache::clear();

    QCursorData::cleanup();
    QFont::cleanup();
    QColormap::cleanup();
    if (displayDC) {
        ReleaseDC(0, displayDC);
        displayDC = 0;
    }

    delete QApplicationPrivate::inputContext;
    QApplicationPrivate::inputContext = 0;

#ifndef Q_OS_TEMP
  // Deinitialize OLE/COM
    OleUninitialize();
#endif
}


/*****************************************************************************
  Platform specific global and internal functions
 *****************************************************************************/

Q_GUI_EXPORT int qWinAppCmdShow()                        // get main window show command
{
    return appCmdShow;
}


Q_GUI_EXPORT HDC qt_win_display_dc()                        // get display DC
{
    if (!displayDC)
        displayDC = GetDC(0);
    return displayDC;
}

bool qt_nograb()                                // application no-grab option
{
#if defined(QT_DEBUG)
    return appNoGrab;
#else
    return false;
#endif
}

typedef QHash<QString, int> WinClassNameHash;
Q_GLOBAL_STATIC(WinClassNameHash, winclassNames)

const QString qt_reg_winclass(QWidget *w)        // register window class
{
    int flags = w->windowFlags();
    int type = flags & Qt::WindowType_Mask;

    uint style;
    bool icon;
    QString cname;
    if (flags & Qt::MSWindowsOwnDC) {
        cname = QLatin1String("QWidgetOwnDC");
        style = CS_DBLCLKS;
#ifndef Q_OS_TEMP
        style |= CS_OWNDC;
#endif
        icon  = true;
    } else if (type == Qt::Tool || type == Qt::ToolTip){
        style = CS_DBLCLKS;
        if (w->inherits("QTipLabel") || w->inherits("QAlphaWidget")) {
            if ((QSysInfo::WindowsVersion >= QSysInfo::WV_XP
                && QSysInfo::WindowsVersion < QSysInfo::WV_NT_based)) {
                style |= 0x00020000;                // CS_DROPSHADOW
            }
            cname = QLatin1String("QToolTip");
        } else {
            cname = QLatin1String("QTool");
        }
#ifndef Q_OS_TEMP
        style |= CS_SAVEBITS;
#endif
        icon = false;
    } else if (type == Qt::Popup) {
        cname = QLatin1String("QPopup");
        style = CS_DBLCLKS;
#ifndef Q_OS_TEMP
        style |= CS_SAVEBITS;
#endif
        if ((QSysInfo::WindowsVersion >= QSysInfo::WV_XP
            && QSysInfo::WindowsVersion < QSysInfo::WV_NT_based))
            style |= 0x00020000;                // CS_DROPSHADOW
        icon = false;
    } else {
        cname = QLatin1String("QWidget");
        style = CS_DBLCLKS;
        icon  = true;
    }

#ifdef Q_OS_TEMP
    // We need to register the classes with the
    // unique ID on WinCE to make sure we can
    // move the windows to the front when starting
    // a second instance.
    cname = QString::number(appUniqueID);
#endif

    // since multiple Qt versions can be used in one process
    // each one has to have window class names with a unique name
    // The first instance gets the unmodified name; if the class
    // has already been registered by another instance of Qt then
    // add an instance-specific ID, the address of the window proc.
    static int classExists = -1;

    if (classExists == -1) {
        QT_WA({
            WNDCLASS wcinfo;
            classExists = GetClassInfo((HINSTANCE)qWinAppInst(), (TCHAR*)cname.utf16(), &wcinfo);
            classExists = classExists && wcinfo.lpfnWndProc != QtWndProc;
        }, {
            WNDCLASSA wcinfo;
            classExists = GetClassInfoA((HINSTANCE)qWinAppInst(), cname.toLatin1(), &wcinfo);
            classExists = classExists && wcinfo.lpfnWndProc != QtWndProc;
        });
    }

    if (classExists)
        cname += QString::number((uint)QtWndProc);

    if (winclassNames()->contains(cname))        // already registered in our list
        return cname;

    ATOM atom;
#ifndef Q_OS_TEMP
    QT_WA({
        WNDCLASS wc;
        wc.style        = style;
        wc.lpfnWndProc        = (WNDPROC)QtWndProc;
        wc.cbClsExtra        = 0;
        wc.cbWndExtra        = 0;
        wc.hInstance        = (HINSTANCE)qWinAppInst();
        if (icon) {
            wc.hIcon = LoadIcon(appInst, L"IDI_ICON1");
            if (!wc.hIcon)
                wc.hIcon = LoadIcon(0, IDI_APPLICATION);
        } else {
            wc.hIcon = 0;
        }
        wc.hCursor        = 0;
        wc.hbrBackground= 0;
        wc.lpszMenuName        = 0;
        wc.lpszClassName= (TCHAR*)cname.utf16();
        atom = RegisterClass(&wc);
    } , {
        WNDCLASSA wc;
        wc.style        = style;
        wc.lpfnWndProc        = (WNDPROC)QtWndProc;
        wc.cbClsExtra        = 0;
        wc.cbWndExtra        = 0;
        wc.hInstance        = (HINSTANCE)qWinAppInst();
        if (icon) {
            wc.hIcon = LoadIconA(appInst, (char*)"IDI_ICON1");
            if (!wc.hIcon)
                wc.hIcon = LoadIconA(0, (char*)IDI_APPLICATION);
        } else {
            wc.hIcon = 0;
        }
        wc.hCursor        = 0;
        wc.hbrBackground= 0;
        wc.lpszMenuName        = 0;
          QByteArray tempArray = cname.toLatin1();
        wc.lpszClassName= tempArray;
        atom = RegisterClassA(&wc);
    });
#else
        WNDCLASS wc;
        wc.style        = style;
        wc.lpfnWndProc        = (WNDPROC)QtWndProc;
        wc.cbClsExtra        = 0;
        wc.cbWndExtra        = 0;
        wc.hInstance        = (HINSTANCE)qWinAppInst();
        if (icon) {
            wc.hIcon = LoadIcon(appInst, L"IDI_ICON1");
//            if (!wc.hIcon)
//                wc.hIcon = LoadIcon(0, IDI_APPLICATION);
        } else {
            wc.hIcon = 0;
        }
        wc.hCursor        = 0;
        wc.hbrBackground= 0;
        wc.lpszMenuName        = 0;
        wc.lpszClassName= (TCHAR*)cname.utf16();
        atom = RegisterClass(&wc);
#endif

#ifndef QT_NO_DEBUG
    if (!atom)
        qErrnoWarning("QApplication::regClass: Registering window class failed.");
#endif

    winclassNames()->insert(cname, 1);
    return cname;
}

static void unregWinClasses()
{
    WinClassNameHash *hash = winclassNames();
    QHash<QString, int>::ConstIterator it = hash->constBegin();
    while (it != hash->constEnd()) {
        QT_WA({
            UnregisterClass((TCHAR*)it.key().utf16(), (HINSTANCE)qWinAppInst());
        } , {
            UnregisterClassA(it.key().toLatin1(), (HINSTANCE)qWinAppInst());
        });
        ++it;
    }
    hash->clear();
}


/*****************************************************************************
  Safe configuration (move,resize,setGeometry) mechanism to avoid
  recursion when processing messages.
 *****************************************************************************/

struct QWinConfigRequest {
    WId         id;                                        // widget to be configured
    int         req;                                        // 0=move, 1=resize, 2=setGeo
    int         x, y, w, h;                                // request parameters
};

static QList<QWinConfigRequest*> *configRequests = 0;

void qWinRequestConfig(WId id, int req, int x, int y, int w, int h)
{
    if (!configRequests)                        // create queue
        configRequests = new QList<QWinConfigRequest*>;
    QWinConfigRequest *r = new QWinConfigRequest;
    r->id = id;                                        // create new request
    r->req = req;
    r->x = x;
    r->y = y;
    r->w = w;
    r->h = h;
    configRequests->append(r);                // store request in queue
}

Q_GUI_EXPORT void qWinProcessConfigRequests()                // perform requests in queue
{
    if (!configRequests)
        return;
    QWinConfigRequest *r;
    for (;;) {
        if (configRequests->isEmpty())
            break;
        r = configRequests->takeLast();
        QWidget *w = QWidget::find(r->id);
        QRect rect(r->x, r->y, r->w, r->h);
        int req = r->req;
        delete r;

        if ( w ) {                              // widget exists
            if (w->testAttribute(Qt::WA_WState_ConfigPending))
                return;                         // biting our tail
            if (req == 0)
                w->move(rect.topLeft());
            else if (req == 1)
                w->resize(rect.size());
            else
                w->setGeometry(rect);
        }
    }
    delete configRequests;
    configRequests = 0;
}


/*****************************************************************************
    GUI event dispatcher
 *****************************************************************************/

class QGuiEventDispatcherWin32 : public QEventDispatcherWin32
{
    Q_DECLARE_PRIVATE(QEventDispatcherWin32)
public:
    QGuiEventDispatcherWin32(QObject *parent = 0);
    bool processEvents(QEventLoop::ProcessEventsFlags flags);
};

QGuiEventDispatcherWin32::QGuiEventDispatcherWin32(QObject *parent)
    : QEventDispatcherWin32(parent)
{
    createInternalHwnd();
}

bool QGuiEventDispatcherWin32::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    if (!QEventDispatcherWin32::processEvents(flags))
        return false;

    if (configRequests)                        // any pending configs?
        qWinProcessConfigRequests();

    return true;
}

void QApplicationPrivate::createEventDispatcher()
{
    Q_Q(QApplication);
    if (q->type() != QApplication::Tty)
        eventDispatcher = new QGuiEventDispatcherWin32(q);
    else
        eventDispatcher = new QEventDispatcherWin32(q);
}

/*****************************************************************************
  Platform specific QApplication members
 *****************************************************************************/

#ifdef QT3_SUPPORT
void QApplication::setMainWidget(QWidget *mainWidget)
{
    QApplicationPrivate::main_widget = mainWidget;
    if (QApplicationPrivate::main_widget && windowIcon().isNull()
        && QApplicationPrivate::main_widget->testAttribute(Qt::WA_SetWindowIcon))
        setWindowIcon(QApplicationPrivate::main_widget->windowIcon());
}
#endif

#ifndef QT_NO_CURSOR

/*****************************************************************************
  QApplication cursor stack
 *****************************************************************************/

void QApplication::setOverrideCursor(const QCursor &cursor)
{
    qApp->d_func()->cursor_list.prepend(cursor);
    SetCursor(qApp->d_func()->cursor_list.first().handle());
}

void QApplication::restoreOverrideCursor()
{
    if (qApp->d_func()->cursor_list.isEmpty())
        return;
    qApp->d_func()->cursor_list.removeFirst();

    if (!qApp->d_func()->cursor_list.isEmpty()) {
        SetCursor(qApp->d_func()->cursor_list.first().handle());
    } else {
        QWidget *w = QWidget::find(curWin);
        if (w)
            SetCursor(w->cursor().handle());
        else
            SetCursor(QCursor(Qt::ArrowCursor).handle());
    }
}

#endif

/*
  Internal function called from QWidget::setCursor()
*/

void qt_win_set_cursor(QWidget *w, const QCursor& /* c */)
{
    if (!curWin)
        return;
    QWidget* cW = QWidget::find(curWin);
    if (!cW || cW->window() != w->window() ||
         !cW->isVisible() || !cW->underMouse() || QApplication::overrideCursor())
        return;

    SetCursor(cW->cursor().handle());
}



/*****************************************************************************
  Routines to find a Qt widget from a screen position
 *****************************************************************************/

QWidget *QApplication::topLevelAt(const QPoint &pos)
{
    POINT p;
    HWND  win;
    QWidget *w;
    p.x = pos.x();
    p.y = pos.y();
    win = WindowFromPoint(p);
    if (!win)
        return 0;

    w = QWidget::find(win);
    while (!w && win) {
        win = GetParent(win);
        w = QWidget::find(win);
    }
    return w ? w->window() : 0;
}

void QApplication::beep()
{
    MessageBeep(MB_OK);
}

static void alert_widget(QWidget *widget, int duration)
{
    bool stopFlash = duration < 0;

    if (!pFlashWindowEx) {
        QLibrary themeLib(QLatin1String("user32"));
        pFlashWindowEx  = (PtrFlashWindowEx)themeLib.resolve("FlashWindowEx");
    }

    if (pFlashWindowEx && widget && (!widget->isActiveWindow() || stopFlash)) {
        DWORD timeOut = GetCaretBlinkTime();
        if (timeOut <= 0)
            timeOut = 250;

        UINT flashCount;
        if (duration == 0)
            flashCount = 10;
        else
            flashCount = duration/timeOut;

        FLASHWINFO info;
        info.cbSize = sizeof(info);
        info.hwnd = widget->window()->winId();
        info.dwFlags = stopFlash ? FLASHW_STOP : FLASHW_TRAY;
        info.dwTimeout = timeOut;
        info.uCount = flashCount;

        pFlashWindowEx(&info);
    }
}

void QApplication::alert(QWidget *widget, int duration)
{
    if (!QApplicationPrivate::checkInstance("alert"))
        return;

    if (widget) {
        alert_widget(widget, duration);
    } else {
        const QWidgetList toplevels(topLevelWidgets());
        for (int i = 0; i < toplevels.count(); ++i) {
            QWidget *topLevel = toplevels.at(i);
            alert_widget(topLevel, duration);
        }
    }
}

QString QApplicationPrivate::appName() const
{
    return QCoreApplicationPrivate::appName();
}


/*****************************************************************************
  Main event loop
 *****************************************************************************/

extern uint qGlobalPostedEventsCount();

/*!
    \internal
    \since 4.1

    If \a gotFocus is true, \a widget will become the active window.
    Otherwise the active window is reset to 0.
*/
void QApplication::winFocus(QWidget *widget, bool gotFocus)
{
    if (d_func()->inPopupMode()) // some delayed focus event to ignore
        return;
    if (gotFocus) {
        setActiveWindow(widget);
        if (QApplicationPrivate::active_window
            && (QApplicationPrivate::active_window->windowType() == Qt::Dialog)) {
            // raise the entire application, not just the dialog
            QWidget* mw = QApplicationPrivate::active_window;
            while(mw->parentWidget() && (mw->windowType() == Qt::Dialog))
                mw = mw->parentWidget()->window();
            if (mw->testAttribute(Qt::WA_WState_Created) && mw != QApplicationPrivate::active_window)
                SetWindowPos(mw->internalWinId(), HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
        }
    } else {
        setActiveWindow(0);
    }
}


//
// QtWndProc() receives all messages from the main event loop
//

static bool inLoop = false;
static int inputcharset = CP_ACP;

#define RETURN(x) { inLoop=false;return x; }

bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event)
{
    return QCoreApplication::sendSpontaneousEvent(receiver, event);
}

static bool qt_is_translatable_mouse_event(UINT message)
{
    return (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST ||
                message >= WM_XBUTTONDOWN && message <= WM_XBUTTONDBLCLK)
            && message != WM_MOUSEWHEEL

            || message >= WM_NCMOUSEMOVE && message <= WM_NCMBUTTONDBLCLK;
}

extern "C"
LRESULT CALLBACK QtWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    bool result = true;
    QEvent::Type evt_type = QEvent::None;
    QETWidget *widget = 0;

        // there is no need to process pakcets from tablet unless
        // it is actually on the tablet, a flag to let us know...
        int nPackets;        // the number of packets we get from the queue

    long res = 0;
    if (!qApp)                                // unstable app state
        goto do_default;

#if 0
    // make sure we update widgets also when the user resizes
    if (inLoop && qApp->loopLevel())
        qApp->sendPostedEvents(0, QEvent::Paint);
#endif

    inLoop = true;

    MSG msg;
    msg.hwnd = hwnd;                                // create MSG structure
    msg.message = message;                        // time and pt fields ignored
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.pt.x = GET_X_LPARAM(lParam);
    msg.pt.y = GET_Y_LPARAM(lParam);
    // If it's a non-client-area message the coords are screen coords, otherwise they are
    // client coords.
    if (message < WM_NCMOUSEMOVE || message > WM_NCMBUTTONDBLCLK)
        ClientToScreen(msg.hwnd, &msg.pt);

    /*
    // sometimes the autograb is not released, so the clickevent is sent
    // to the wrong window. We ignore this for now, because it doesn't
    // cause any problems.
    if (msg.message == WM_LBUTTONDOWN || msg.message == WM_RBUTTONDOWN || msg.message == WM_MBUTTONDOWN) {
        HWND handle = WindowFromPoint(msg.pt);
        if (msg.hwnd != handle) {
            msg.hwnd = handle;
            hwnd = handle;
        }
    }
    */

#if defined(QT_NON_COMMERCIAL)
    QT_NC_WNDPROC
#endif

    // send through app filter
    if (qApp->filterEvent(&msg, &res))
        return res;

    switch (message) {
#ifndef Q_OS_TEMP
    case WM_QUERYENDSESSION: {
        if (sm_smActive) // bogus message from windows
            RETURN(true);

        sm_smActive = true;
        sm_blockUserInput = true; // prevent user-interaction outside interaction windows
        sm_cancel = false;
        if (qt_session_manager_self)
            qApp->commitData(*qt_session_manager_self);
        if (lParam & ENDSESSION_LOGOFF) {
            _flushall();
        }
        RETURN(!sm_cancel);
    }
    case WM_ENDSESSION: {
        sm_smActive = false;
        sm_blockUserInput = false;
        bool endsession = (bool) wParam;

        if (endsession) {
            // since the process will be killed immediately quit() has no real effect
            int index = QApplication::staticMetaObject.indexOfSignal("aboutToQuit()");
            qApp->qt_metacall(QMetaObject::InvokeMetaMethod, index,0);
            qApp->quit();
        }

        RETURN(0);
    }
    case WM_DISPLAYCHANGE:
        if (qApp->type() == QApplication::Tty)
            break;
        if (qt_desktopWidget) {
            qt_desktopWidget->move(GetSystemMetrics(76), GetSystemMetrics(77));
            QSize sz(GetSystemMetrics(78), GetSystemMetrics(79));
            if (sz == qt_desktopWidget->size()) {
                 // a screen resized without changing size of the virtual desktop
                QResizeEvent rs(sz, qt_desktopWidget->size());
                QApplication::sendEvent(qt_desktopWidget, &rs);
            } else {
                qt_desktopWidget->resize(sz);
            }
        }
        break;
#endif

    case WM_SETTINGCHANGE:
#ifdef Q_OS_TEMP
        // CE SIP hide/show
        if (wParam == SPI_SETSIPINFO) {
            QResizeEvent re(QSize(0, 0), QSize(0, 0)); // Calculated by QDesktopWidget
            QApplication::sendEvent(qt_desktopWidget, &re);
            break;
        }
#endif
        // ignore spurious XP message when user logs in again after locking
        if (qApp->type() == QApplication::Tty)
            break;
        if (QApplication::desktopSettingsAware() && wParam != SPI_SETWORKAREA) {
            widget = (QETWidget*)QWidget::find(hwnd);
            if (widget) {
                if (wParam == SPI_SETNONCLIENTMETRICS)
                    widget->markFrameStrutDirty();
            }
        }
        else if (qt_desktopWidget && wParam == SPI_SETWORKAREA) {
            qt_desktopWidget->move(GetSystemMetrics(76), GetSystemMetrics(77));
            QSize sz(GetSystemMetrics(78), GetSystemMetrics(79));
            if (sz == qt_desktopWidget->size()) {
                 // a screen resized without changing size of the virtual desktop
                QResizeEvent rs(sz, qt_desktopWidget->size());
                QApplication::sendEvent(qt_desktopWidget, &rs);
            } else {
                qt_desktopWidget->resize(sz);
            }
        }

        break;
    case WM_SYSCOLORCHANGE:
        if (qApp->type() == QApplication::Tty)
            break;
        if (QApplication::desktopSettingsAware()) {
            widget = (QETWidget*)QWidget::find(hwnd);
            if (widget && !widget->parentWidget())
                qt_set_windows_color_resources();
        }
        break;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_XBUTTONDOWN:
        if (qt_win_ignoreNextMouseReleaseEvent)
            qt_win_ignoreNextMouseReleaseEvent = false;
        break;

    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_XBUTTONUP:
        if (qt_win_ignoreNextMouseReleaseEvent) {
            qt_win_ignoreNextMouseReleaseEvent = false;
            if (qt_button_down && qt_button_down->internalWinId() == autoCaptureWnd) {
                releaseAutoCapture();
                qt_button_down = 0;
            }

            RETURN(0);
        }
        break;

    default:
        break;
    }

    if (!widget)
        widget = (QETWidget*)QWidget::find(hwnd);
    if (!widget)                                // don't know this widget
        goto do_default;

    if (app_do_modal)        {                        // modal event handling
        int ret = 0;
        if (!qt_try_modal(widget, &msg, ret))
            RETURN(ret);
    }

    res = 0;
    if (widget->winEvent(&msg, &res))                // send through widget filter
        RETURN(res);

    if (qt_is_translatable_mouse_event(message)) {
        if (qApp->activePopupWidget() != 0) { // in popup mode
            POINT curPos = msg.pt;
            QWidget* w = QApplication::widgetAt(curPos.x, curPos.y);
            if (w)
                widget = (QETWidget*)w;
        }

        if (!qt_tabletChokeMouse) {
            result = widget->translateMouseEvent(msg);        // mouse event
        } else {
            // Sometimes we only get a WM_MOUSEMOVE message
            // and sometimes we get both a WM_MOUSEMOVE and
            // a WM_LBUTTONDOWN/UP, this creates a spurious mouse
            // press/release event, using the winPeekMessage
            // will help us fix this.  This leaves us with a
            // question:
            //    This effectively kills using the mouse AND the
            //    tablet simultaneously, well creates wacky input.
            //    Is this going to be a problem? (probably not)
            bool next_is_button = false;
            bool is_mouse_move = (message == WM_MOUSEMOVE);
            if (is_mouse_move) {
                MSG msg1;
                if (winPeekMessage(&msg1, msg.hwnd, WM_MOUSEFIRST,
                                    WM_MOUSELAST, PM_NOREMOVE))
                    next_is_button = (msg1.message == WM_LBUTTONUP
                                       || msg1.message == WM_LBUTTONDOWN);
            }
            if (!is_mouse_move || (is_mouse_move && !next_is_button))
                qt_tabletChokeMouse = false;
        }
    } else if (message == WM_QT_REPAINT) {
        result = widget->translatePaintEvent(msg);
    } else if (message == WM95_MOUSEWHEEL) {
        result = widget->translateWheelEvent(msg);
    } else {
        switch (message) {
        case WM_KEYDOWN:                        // keyboard event
        case WM_SYSKEYDOWN:
            qt_keymapper_private()->updateKeyMap(msg);
            // fall-through intended
        case WM_KEYUP:
        case WM_SYSKEYUP:
        case WM_IME_CHAR:
        case WM_IME_KEYDOWN:
        case WM_CHAR: {
            MSG msg1;
            bool anyMsg = winPeekMessage(&msg1, msg.hwnd, 0, 0, PM_NOREMOVE);
            if (anyMsg && msg1.message == WM_DEADCHAR) {
                result = true; // consume event since there is a dead char next
                break;
            }
            QWidget *g = QWidget::keyboardGrabber();
            if (g)
                widget = (QETWidget*)g;
            else if (QApplication::activePopupWidget())
                widget = (QETWidget*)QApplication::activePopupWidget()->focusWidget()
                       ? (QETWidget*)QApplication::activePopupWidget()->focusWidget()
                       : (QETWidget*)QApplication::activePopupWidget();
            else if (qApp->focusWidget())
                widget = (QETWidget*)QApplication::focusWidget();
            else if (!widget || widget->internalWinId() == GetFocus()) // We faked the message to go to exactly that widget.
                widget = (QETWidget*)widget->window();
            if (widget->isEnabled())
                result = sm_blockUserInput
                            ? true
                            : qt_keymapper_private()->translateKeyEvent(widget, msg, g != 0);
            break;
        }
        case WM_SYSCHAR:
            result = true;                        // consume event
            break;

        case WM_MOUSEWHEEL:
            result = widget->translateWheelEvent(msg);
            break;

        case WM_APPCOMMAND:
            {
                uint cmd = GET_APPCOMMAND_LPARAM(lParam);
                uint uDevice = GET_DEVICE_LPARAM(lParam);
                uint dwKeys = GET_KEYSTATE_LPARAM(lParam);

                int state = translateButtonState(dwKeys, QEvent::KeyPress, 0);

                switch (uDevice) {
                case FAPPCOMMAND_KEY:
                    {
                        int key = 0;

                        switch(cmd) {
                        case APPCOMMAND_BASS_BOOST:
                            key = Qt::Key_BassBoost;
                            break;
                        case APPCOMMAND_BASS_DOWN:
                            key = Qt::Key_BassDown;
                            break;
                        case APPCOMMAND_BASS_UP:
                            key = Qt::Key_BassUp;
                            break;
                        case APPCOMMAND_BROWSER_BACKWARD:
                            key = Qt::Key_Back;
                            break;
                        case APPCOMMAND_BROWSER_FAVORITES:
                            key = Qt::Key_Favorites;
                            break;
                        case APPCOMMAND_BROWSER_FORWARD:
                            key = Qt::Key_Forward;
                            break;
                        case APPCOMMAND_BROWSER_HOME:
                            key = Qt::Key_HomePage;
                            break;
                        case APPCOMMAND_BROWSER_REFRESH:
                            key = Qt::Key_Refresh;
                            break;
                        case APPCOMMAND_BROWSER_SEARCH:
                            key = Qt::Key_Search;
                            break;
                        case APPCOMMAND_BROWSER_STOP:
                            key = Qt::Key_Stop;
                            break;
                        case APPCOMMAND_LAUNCH_APP1:
                            key = Qt::Key_Launch0;
                            break;
                        case APPCOMMAND_LAUNCH_APP2:
                            key = Qt::Key_Launch1;
                            break;
                        case APPCOMMAND_LAUNCH_MAIL:
                            key = Qt::Key_LaunchMail;
                            break;
                        case APPCOMMAND_LAUNCH_MEDIA_SELECT:
                            key = Qt::Key_LaunchMedia;
                            break;
                        case APPCOMMAND_MEDIA_NEXTTRACK:
                            key = Qt::Key_MediaNext;
                            break;
                        case APPCOMMAND_MEDIA_PLAY_PAUSE:
                            key = Qt::Key_MediaPlay;
                            break;
                        case APPCOMMAND_MEDIA_PREVIOUSTRACK:
                            key = Qt::Key_MediaPrevious;
                            break;
                        case APPCOMMAND_MEDIA_STOP:
                            key = Qt::Key_MediaStop;
                            break;
                        case APPCOMMAND_TREBLE_DOWN:
                            key = Qt::Key_TrebleDown;
                            break;
                        case APPCOMMAND_TREBLE_UP:
                            key = Qt::Key_TrebleUp;
                            break;
                        case APPCOMMAND_VOLUME_DOWN:
                            key = Qt::Key_VolumeDown;
                            break;
                        case APPCOMMAND_VOLUME_MUTE:
                            key = Qt::Key_VolumeMute;
                            break;
                        case APPCOMMAND_VOLUME_UP:
                            key = Qt::Key_VolumeUp;
                            break;
                        // Commands new in Windows XP
                        case APPCOMMAND_HELP:
                            key = Qt::Key_Help;
                            break;
                        case APPCOMMAND_FIND:
                            key = Qt::Key_Search;
                            break;
                        case APPCOMMAND_PRINT:
                            key = Qt::Key_Print;
                            break;
                        case APPCOMMAND_MEDIA_PLAY:
                            key = Qt::Key_MediaPlay;
                            break;
                        default:
                            break;
                        }
                        if (key) {
                            bool res = false;
                            QWidget *g = QWidget::keyboardGrabber();
                            if (g)
                                widget = (QETWidget*)g;
                            else if (qApp->focusWidget())
                                widget = (QETWidget*)qApp->focusWidget();
                            else
                                widget = (QETWidget*)widget->window();
                            if (widget->isEnabled()) {
                                res = QKeyMapper::sendKeyEvent(widget, g != 0, QEvent::KeyPress, key,
                                                               Qt::KeyboardModifier(state),
                                                               QString(), false, 0, 0, 0, 0);
                            }
                            if (res)
                                return true;
                        }
                    }
                    break;

                default:
                    break;
                }

                result = false;
            }
            break;

        case WM_NCHITTEST:
            if (widget->isWindow()) {
                QPoint pos = widget->mapFromGlobal(QPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
                // don't show resize-cursors for fixed-size widgets
                QRect fs = widget->frameStrut();
                if (!widget->isMinimized()) {
                    if (widget->minimumWidth() == widget->maximumWidth() && (pos.x() < 0 || pos.x() >= widget->width()))
                        break;
                    if (widget->minimumHeight() == widget->maximumHeight() && (pos.y() < -(fs.top() - fs.left()) || pos.y() >= widget->height()))
                        break;
                }
            }

            result = false;
            break;

        case WM_SYSCOMMAND: {
#ifndef Q_OS_TEMP
            bool window_state_change = false;
            Qt::WindowStates oldstate = Qt::WindowStates(widget->dataPtr()->window_state);
            // MSDN:In WM_SYSCOMMAND messages, the four low-order bits of the wParam parameter are
            // used internally by the system. To obtain the correct result when testing the value of
            // wParam, an application must combine the value 0xFFF0 with the wParam value by using
            // the bitwise AND operator.
            switch(0xfff0 & wParam) {
            case SC_CONTEXTHELP:
#ifndef QT_NO_WHATSTHIS
                QWhatsThis::enterWhatsThisMode();
#endif
                QT_WA({
                    DefWindowProc(hwnd, WM_NCPAINT, 1, 0);
                } , {
                    DefWindowProcA(hwnd, WM_NCPAINT, 1, 0);
                });
                break;
#if defined(QT_NON_COMMERCIAL)
                QT_NC_SYSCOMMAND
#endif
            case SC_MINIMIZE:
                window_state_change = true;
                widget->dataPtr()->window_state |= Qt::WindowMinimized;
                if (widget->isVisible()) {
                    QHideEvent e;
                    qt_sendSpontaneousEvent(widget, &e);
                    widget->hideChildren(true);
                }
                result = false;
                break;
            case SC_MAXIMIZE:
                if(widget->isWindow())
                    widget->topData()->normalGeometry = widget->geometry();
            case SC_RESTORE:
                window_state_change = true;
                if ((0xfff0 & wParam) == SC_MAXIMIZE)
                    widget->dataPtr()->window_state |= Qt::WindowMaximized;
                else if (!widget->isMinimized())
                    widget->dataPtr()->window_state &= ~Qt::WindowMaximized;

                if (widget->isMinimized()) {
                    widget->dataPtr()->window_state &= ~Qt::WindowMinimized;
                    widget->showChildren(true);
                    QShowEvent e;
                    qt_sendSpontaneousEvent(widget, &e);
                }
                result = false;
                break;
            default:
                result = false;
                break;
            }

            if (window_state_change) {
                QWindowStateChangeEvent e(oldstate);
                qt_sendSpontaneousEvent(widget, &e);
            }
#endif

            break;
        }

        case WM_SETTINGCHANGE:
            if ( qApp->type() == QApplication::Tty )
                break;

            if (!msg.wParam) {
                QString area = QT_WA_INLINE(QString::fromUtf16((unsigned short *)msg.lParam),
                                             QString::fromLocal8Bit((char*)msg.lParam));
                if (area == QLatin1String("intl"))
                    QApplication::postEvent(widget, new QEvent(QEvent::LocaleChange));
            }
            break;

        case WM_PAINT:                                // paint event
        case WM_ERASEBKGND:                        // erase window background
            result = widget->translatePaintEvent(msg);
            break;

        case WM_MOVE:                                // move window
        case WM_SIZE:                                // resize window
            result = widget->translateConfigEvent(msg);
            break;

        case WM_ACTIVATEAPP:
            if (wParam == FALSE)
                QApplication::setActiveWindow(0);
            break;

        case WM_ACTIVATE:
            if ( qApp->type() == QApplication::Tty )
                break;

            if (ptrWTOverlap && ptrWTEnable) {
                // cooperate with other tablet applications, but when
                // we get focus, I want to use the tablet...
                if (qt_tablet_context && GET_WM_ACTIVATE_STATE(wParam, lParam)) {
                    if (ptrWTEnable(qt_tablet_context, true))
                        ptrWTOverlap(qt_tablet_context, true);
                }
            }
            if (QApplication::activePopupWidget() && LOWORD(wParam) == WA_INACTIVE &&
                QWidget::find((HWND)lParam) == 0) {
                // Another application was activated while our popups are open,
                // then close all popups.  In case some popup refuses to close,
                // we give up after 1024 attempts (to avoid an infinite loop).
                int maxiter = 1024;
                QWidget *popup;
                while ((popup=QApplication::activePopupWidget()) && maxiter--)
                    popup->close();
            }

            if (LOWORD(wParam) != WA_INACTIVE) {
                // WM_ACTIVATEAPP handles the "true" false case, as this is only when the application
                // loses focus. Doing it here would result in the widget getting focus to not know
                // where it got it from; it would simply get a 0 value as the old focus widget.
                qApp->winFocus(widget, true);
                // reset any window alert flashes
                alert_widget(widget, -1);
            }

            // Windows tries to activate a modally blocked window.
            // This happens when restoring an application after "Show Desktop"
            if (app_do_modal && LOWORD(wParam) == WA_ACTIVE) {
                QWidget *top = 0;
                if (!QApplicationPrivate::tryModalHelper(widget, &top) && top && widget != top && top->isVisible())
                    top->activateWindow();
            }
            break;

#ifndef Q_OS_TEMP
            case WM_MOUSEACTIVATE:
                if (widget->window()->windowType() == Qt::Tool) {
                    QWidget *w = widget;
                    if (!w->window()->focusWidget()) {
                        while (w && (w->focusPolicy() & Qt::ClickFocus) == 0) {
                            if (w->isWindow()) {
                                QWidget *fw = w;
                                while ((fw = fw->nextInFocusChain()) != w && fw->focusPolicy() == Qt::NoFocus)
                                    ;
                                if (fw != w)
                                   break;
                                QWidget *pw = w->parentWidget();
                                while (pw) {
                                    pw = pw->window();
                                    if (pw && pw->isVisible() && pw->focusWidget()) {
                                        Q_ASSERT(pw->testAttribute(Qt::WA_WState_Created));
                                        SetWindowPos(pw->internalWinId(), HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
                                        break;
                                    }
                                    pw = pw->parentWidget();
                                }
                                RETURN(MA_NOACTIVATE);
                            }
                            w = w->parentWidget();
                        }
                    }
                }
                RETURN(MA_ACTIVATE);
                break;
#endif
            case WM_SHOWWINDOW:
#ifndef Q_OS_TEMP
                if (lParam == SW_PARENTOPENING) {
                    if (widget->testAttribute(Qt::WA_WState_Hidden))
                        RETURN(0);
                }
#endif
                if (widget->isWindow() && widget->testAttribute(Qt::WA_WState_Visible)
                    && !widget->testWindowState(Qt::WindowMinimized)) {
                    if (lParam == SW_PARENTOPENING) {
                        QShowEvent e;
                        qt_sendSpontaneousEvent(widget, &e);
                        widget->showChildren(true);
                    } else if (lParam == SW_PARENTCLOSING) {
                        QHideEvent e;
                        qt_sendSpontaneousEvent(widget, &e);
                        widget->hideChildren(true);
                    }
                }
                if  (!wParam && autoCaptureWnd == widget->internalWinId())
                    releaseAutoCapture();
                result = false;
                break;

        case WM_PALETTECHANGED:                        // our window changed palette
            if (QColormap::hPal() && (WId)wParam == widget->internalWinId())
                RETURN(0);                        // otherwise: FALL THROUGH!
            // FALL THROUGH
        case WM_QUERYNEWPALETTE:                // realize own palette
            if (QColormap::hPal()) {
                Q_ASSERT(widget->testAttribute(Qt::WA_WState_Created));
                HDC hdc = GetDC(widget->internalWinId());
                HPALETTE hpalOld = SelectPalette(hdc, QColormap::hPal(), FALSE);
                uint n = RealizePalette(hdc);
                if (n)
                    InvalidateRect(widget->internalWinId(), 0, TRUE);
                SelectPalette(hdc, hpalOld, TRUE);
                RealizePalette(hdc);
                ReleaseDC(widget->internalWinId(), hdc);
                RETURN(n);
            }
            break;
        case WM_CLOSE:                                // close window
            widget->translateCloseEvent(msg);
            RETURN(0);                                // always handled

        case WM_DESTROY:                        // destroy window
            if (hwnd == curWin) {
                QEvent leave(QEvent::Leave);
                QApplication::sendEvent(widget, &leave);
                curWin = 0;
            }
            if (widget == popupButtonFocus)
                popupButtonFocus = 0;
            result = false;
            break;

#ifndef Q_OS_TEMP
        case WM_WINDOWPOSCHANGING:
            {
                result = false;
                if (widget->isWindow()
                    && widget->layout()
                    && widget->layout()->hasHeightForWidth()) {
                    WINDOWPOS *winPos = (WINDOWPOS *)lParam;
                    QRect fs = widget->frameStrut();
                    QRect rect = widget->geometry();
                    QRect newRect = QRect(winPos->x + fs.left(),
                                          winPos->y + fs.top(),
                                          winPos->cx - fs.left() - fs.right(),
                                          winPos->cy - fs.top() - fs.bottom());

                    QSize newSize = QLayout::closestAcceptableSize(widget, newRect.size());

                    int dh = newSize.height() - newRect.height();
                    int dw = newSize.width() - newRect.width();
                    if (!dw && ! dh)
                        break; // Size OK

                    if (rect.y() != newRect.y()) {
                        newRect.setTop(newRect.top() - dh);
                    } else {
                        newRect.setBottom(newRect.bottom() + dh);
                    }

                    if (rect.x() != newRect.x()) {
                        newRect.setLeft(newRect.left() - dw);
                    } else {
                        newRect.setRight(newRect.right() + dw);
                    }

                    winPos->x = newRect.x() - fs.left();
                    winPos->y = newRect.y() - fs.top();
                    winPos->cx = newRect.width() + fs.left() + fs.right();
                    winPos->cy = newRect.height() + fs.top() + fs.bottom();

                    RETURN(0);
                }
            }
            break;

        case WM_GETMINMAXINFO:
            if (widget->xtra()) {
                MINMAXINFO *mmi = (MINMAXINFO *)lParam;
                QWExtra *x = widget->xtra();
                QRect fs = widget->frameStrut();
                if ( x->minw > 0 )
                    mmi->ptMinTrackSize.x = x->minw + fs.right() + fs.left();
                if ( x->minh > 0 )
                    mmi->ptMinTrackSize.y = x->minh + fs.top() + fs.bottom();
                if ( x->maxw < QWIDGETSIZE_MAX ) {
                    mmi->ptMaxTrackSize.x = x->maxw + fs.right() + fs.left();
                    // windows with title bar have an implicit size limit of 112 pixels
                    if (widget->windowFlags() & Qt::WindowTitleHint)
                        mmi->ptMaxTrackSize.x = qMax<long>(mmi->ptMaxTrackSize.x, 112);
                }
                if ( x->maxh < QWIDGETSIZE_MAX )
                    mmi->ptMaxTrackSize.y = x->maxh + fs.top() + fs.bottom();
                RETURN(0);
            }
            break;

            case WM_CONTEXTMENU:
            {
                // it's not VK_APPS or Shift+F10, but a click in the NC area
                if (lParam != (int)0xffffffff) {
                    result = false;
                    break;
                }

                QWidget *fw = QWidget::keyboardGrabber();
                if (!fw) {
                    if (qApp->activePopupWidget())
                        fw = (qApp->activePopupWidget()->focusWidget()
                                                  ? qApp->activePopupWidget()->focusWidget()
                                                  : qApp->activePopupWidget());
                    else if (qApp->focusWidget())
                        fw = qApp->focusWidget();
                    else if (widget)
                        fw = widget->window();
                }
                if (fw && fw->isEnabled()) {
                    QPoint pos = fw->inputMethodQuery(Qt::ImMicroFocus).toRect().center();
                    QContextMenuEvent e(QContextMenuEvent::Keyboard, pos, fw->mapToGlobal(pos));
                    result = qt_sendSpontaneousEvent(fw, &e);
                }
            }
            break;
#endif

        case WM_IME_STARTCOMPOSITION:
        case WM_IME_ENDCOMPOSITION:
        case WM_IME_COMPOSITION: {
            QWidget *fw = qApp->focusWidget();
            QWinInputContext *im = fw ? qobject_cast<QWinInputContext *>(fw->inputContext()) : 0;
            if (fw && im) {
                if(message == WM_IME_STARTCOMPOSITION)
                    result = im->startComposition();
                else if (message == WM_IME_ENDCOMPOSITION)
                    result = im->endComposition();
                else if (message == WM_IME_COMPOSITION)
                    result = im->composition(lParam);
            }
            break;
        }

#ifndef Q_OS_TEMP
        case WM_CHANGECBCHAIN:
        case WM_DRAWCLIPBOARD:
        case WM_RENDERFORMAT:
        case WM_RENDERALLFORMATS:
        case WM_DESTROYCLIPBOARD:
            if (qt_clipboard) {
                QClipboardEvent e(reinterpret_cast<QEventPrivate *>(&msg));
                qt_sendSpontaneousEvent(qt_clipboard, &e);
                RETURN(0);
            }
            result = false;
            break;
#endif
#ifndef QT_NO_ACCESSIBILITY
        case WM_GETOBJECT:
            {
                // Ignoring all requests while starting up
                if (qApp->startingUp() || qApp->closingDown() || (DWORD)lParam != OBJID_CLIENT) {
                    result = false;
                    break;
                }

                typedef LRESULT (WINAPI *PtrLresultFromObject)(REFIID, WPARAM, LPUNKNOWN);
                static PtrLresultFromObject ptrLresultFromObject = 0;
                static bool oleaccChecked = false;

                if (!oleaccChecked) {
                    oleaccChecked = true;
                    ptrLresultFromObject = (PtrLresultFromObject)QLibrary::resolve(QLatin1String("oleacc.dll"), "LresultFromObject");
                }
                if (ptrLresultFromObject) {
                    QAccessibleInterface *acc = QAccessible::queryAccessibleInterface(widget);
                    if (!acc) {
                        result = false;
                        break;
                    }

                    // and get an instance of the IAccessibile implementation
                    IAccessible *iface = qt_createWindowsAccessible(acc);
                    res = ptrLresultFromObject(IID_IAccessible, wParam, iface);  // ref == 2
                    iface->Release(); // the client will release the object again, and then it will destroy itself

                    if (res > 0)
                        RETURN(res);
                }
            }
            result = false;
            break;
        case WM_GETTEXT:
            if (!widget->isWindow()) {
                int ret = 0;
                QAccessibleInterface *acc = QAccessible::queryAccessibleInterface(widget);
                if (acc) {
                    QString text = acc->text(QAccessible::Name, 0);
                    if (text.isEmpty())
                        text = widget->objectName();
                    ret = qMin<int>(wParam - 1, text.size());
                    text.resize(ret);
                    QT_WA({
                        memcpy((void *)lParam, text.utf16(), (text.size() + 1) * 2);
                    }, {
                        memcpy((void *)lParam, text.toLocal8Bit().data(), text.size() + 1);
                    });
                    delete acc;
                }
                if (!ret) {
                    result = false;
                    break;
                }
                RETURN(ret);
            }
            result = false;
            break;
#endif
        case WT_PACKET:
            if (ptrWTPacketsGet) {
                if ((nPackets = ptrWTPacketsGet(qt_tablet_context, QT_TABLET_NPACKETQSIZE, &localPacketBuf))) {
                    result = widget->translateTabletEvent(msg, localPacketBuf, nPackets);
                }
            }
            break;
        case WT_PROXIMITY:
            if (ptrWTPacketsGet) {
                bool enteredProximity = LOWORD(lParam) != 0;
                PACKET proximityBuffer[QT_TABLET_NPACKETQSIZE];
                int totalPacks = ptrWTPacketsGet(qt_tablet_context, QT_TABLET_NPACKETQSIZE, proximityBuffer);
                if (totalPacks > 0 && enteredProximity) {
                    uint currentCursor = proximityBuffer[0].pkCursor;
                    if (!tCursorInfo()->contains(currentCursor))
                        tabletInit(currentCursor, qt_tablet_context);
                    currentTabletPointer = tCursorInfo()->value(currentCursor);
                }
                qt_tabletChokeMouse = false;
                QTabletEvent tabletProximity(enteredProximity ? QEvent::TabletEnterProximity
                                                              : QEvent::TabletLeaveProximity,
                                             QPoint(), QPoint(), QPointF(), currentTabletPointer.currentDevice, currentTabletPointer.currentPointerType, 0, 0,
                                             0, 0, 0, 0, 0, currentTabletPointer.llId);
                QApplication::sendEvent(qApp, &tabletProximity);
            }
            break;
        case WM_KILLFOCUS:
            if (!QWidget::find((HWND)wParam)) { // we don't get focus, so unset it now
                if (!widget->hasFocus()) // work around Windows bug after minimizing/restoring
                    widget = (QETWidget*)qApp->focusWidget();
                HWND focus = ::GetFocus();
                if (!widget || (focus && ::IsChild(widget->internalWinId(), focus))) {
                    result = false;
                } else {
                    widget->clearFocus();
                    result = true;
                }
            } else {
                result = false;
            }
            break;

        case WM_THEMECHANGED:
            if ((widget->windowType() == Qt::Desktop) || !qApp || qApp->closingDown()
                                                         || qApp->type() == QApplication::Tty)
                break;

            if (widget->testAttribute(Qt::WA_WState_Polished))
                qApp->style()->unpolish(widget);

            if (widget->testAttribute(Qt::WA_WState_Polished))
                qApp->style()->polish(widget);
            widget->repolishStyle(*qApp->style());
            if (widget->isVisible())
                widget->update();
            break;

#ifndef Q_OS_TEMP
        case WM_INPUTLANGCHANGE: {
            char info[7];
            if (!GetLocaleInfoA(MAKELCID(lParam, SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, info, 6)) {
                inputcharset = CP_ACP;
            } else {
                inputcharset = QString::fromLatin1(info).toInt();
            }
            QKeyMapper::changeKeyboard();
            break;
        }
#else
        case WM_COMMAND:
            result = (wParam == 0x1);
            if (result)
                QApplication::postEvent(widget, new QEvent(QEvent::OkRequest));
            break;
        case WM_HELP:
            QApplication::postEvent(widget, new QEvent(QEvent::HelpRequest));
            result = true;
            break;
#endif

        case WM_MOUSELEAVE:
            // We receive a mouse leave for curWin, meaning
            // the mouse was moved outside our widgets
            if (widget->internalWinId() == curWin) {
                bool dispatch = !widget->underMouse();
                // hasMouse is updated when dispatching enter/leave,
                // so test if it is actually up-to-date
                if (!dispatch) {
                    QRect geom = widget->geometry();
                    if (widget->parentWidget() && !widget->isWindow()) {
                        QPoint gp = widget->parentWidget()->mapToGlobal(widget->pos());
                        geom.setX(gp.x());
                        geom.setY(gp.y());
                    }
                    QPoint cpos = QCursor::pos();
                    dispatch = !geom.contains(cpos);
                    if ( !dispatch) {
                        QWidget *hittest = QApplication::widgetAt(cpos);
                        dispatch = !hittest || hittest->internalWinId() != curWin;
                    }
                    if (!dispatch) {
                        HRGN hrgn = qt_tryCreateRegion(QRegion::Rectangle, 0,0,0,0);
                        if (GetWindowRgn(curWin, hrgn) != ERROR) {
                            QPoint lcpos = widget->mapFromGlobal(cpos);
                            dispatch = !PtInRegion(hrgn, lcpos.x(), lcpos.y());
                        }
                        DeleteObject(hrgn);
                    }
                }
                if (dispatch) {
                    QApplicationPrivate::dispatchEnterLeave(0, QWidget::find((WId)curWin));
                    curWin = 0;
                }
            }
            break;

        case WM_CANCELMODE:
            {
                // this goes through QMenuBar's event filter
                QEvent e(QEvent::ActivationChange);
                QApplication::sendEvent(qApp, &e);
            }
            break;

        default:
            result = false;                        // event was not processed
            break;
        }
    }

    if (evt_type != QEvent::None) {                // simple event
        QEvent e(evt_type);
        result = qt_sendSpontaneousEvent(widget, &e);
    }

    if (result)
        RETURN(false);

do_default:
    RETURN(QWinInputContext::DefWindowProc(hwnd,message,wParam,lParam))
}


/*****************************************************************************
  Modal widgets; We have implemented our own modal widget mechanism
  to get total control.
  A modal widget without a parent becomes application-modal.
  A modal widget with a parent becomes modal to its parent and grandparents..

  QApplicationPrivate::enterModal()
        Enters modal state
        Arguments:
            QWidget *widget        A modal widget

  QApplicationPrivate::leaveModal()
        Leaves modal state for a widget
        Arguments:
            QWidget *widget        A modal widget
 *****************************************************************************/

bool QApplicationPrivate::modalState()
{
    return app_do_modal;
}

void QApplicationPrivate::enterModal_sys(QWidget *widget)
{
    if (!qt_modal_stack)
        qt_modal_stack = new QWidgetList;

    releaseAutoCapture();
    QApplicationPrivate::dispatchEnterLeave(0, QWidget::find((WId)curWin));
    qt_modal_stack->insert(0, widget);
    app_do_modal = true;
    curWin = 0;
    qt_button_down = 0;
    qt_win_ignoreNextMouseReleaseEvent = false;
}

void QApplicationPrivate::leaveModal_sys(QWidget *widget)
{
    if (qt_modal_stack && qt_modal_stack->removeAll(widget)) {
        if (qt_modal_stack->isEmpty()) {
            delete qt_modal_stack;
            qt_modal_stack = 0;
            QPoint p(QCursor::pos());
            app_do_modal = false; // necessary, we may get recursively into qt_try_modal below
            QWidget* w = QApplication::widgetAt(p.x(), p.y());
            QApplicationPrivate::dispatchEnterLeave(w, QWidget::find(curWin)); // send synthetic enter event
            curWin = w? w->internalWinId() : 0;
        }
        qt_win_ignoreNextMouseReleaseEvent = true;
    }
    app_do_modal = qt_modal_stack != 0;
}

bool qt_try_modal(QWidget *widget, MSG *msg, int& ret)
{
    QWidget * top = 0;

    if (QApplicationPrivate::tryModalHelper(widget, &top))
        return true;

    int type = msg->message;

    bool block_event = false;
#ifndef Q_OS_TEMP
    if (type != WM_NCHITTEST)
#endif
        if ((type >= WM_MOUSEFIRST && type <= WM_MOUSELAST) ||
             type == WM_MOUSEWHEEL || type == (int)WM95_MOUSEWHEEL ||
             type == WM_MOUSELEAVE ||
             (type >= WM_KEYFIRST && type <= WM_KEYLAST)
#ifndef Q_OS_TEMP
            || type == WM_NCMOUSEMOVE
#endif
         ) {
            if (type == WM_MOUSEMOVE
#ifndef Q_OS_TEMP
                 || type == WM_NCMOUSEMOVE
#endif
            ) {
                QCursor *c = qt_grab_cursor();
                if (!c)
                    c = QApplication::overrideCursor();
                if (c)                                // application cursor defined
                    SetCursor(c->handle());
                else
                    SetCursor(QCursor(Qt::ArrowCursor).handle());
            }
            block_event = true;
        } else if (type == WM_CLOSE) {
            block_event = true;
        }
#ifndef Q_OS_TEMP
    else if (type == WM_MOUSEACTIVATE || type == WM_NCLBUTTONDOWN){
        if (!top->isActiveWindow()) {
            top->activateWindow();
        } else {
            QApplication::beep();
        }
        block_event = true;
        ret = MA_NOACTIVATEANDEAT;
    } else if (type == WM_SYSCOMMAND) {
        if (!(msg->wParam == SC_RESTORE && widget->isMinimized()))
            block_event = true;
    }
#endif

    return !block_event;
}


/*****************************************************************************
  Popup widget mechanism

  openPopup()
        Adds a widget to the list of popup widgets
        Arguments:
            QWidget *widget        The popup widget to be added

  closePopup()
        Removes a widget from the list of popup widgets
        Arguments:
            QWidget *widget        The popup widget to be removed
 *****************************************************************************/

void QApplicationPrivate::openPopup(QWidget *popup)
{
    if (!QApplicationPrivate::popupWidgets)
        QApplicationPrivate::popupWidgets = new QWidgetList;
    QApplicationPrivate::popupWidgets->append(popup);
    if (!popup->isEnabled())
        return;

    if (QApplicationPrivate::popupWidgets->count() == 1 && !qt_nograb()) {
        Q_ASSERT(popup->testAttribute(Qt::WA_WState_Created));
        setAutoCapture(popup->internalWinId());        // grab mouse/keyboard
    }
    // Popups are not focus-handled by the window system (the first
    // popup grabbed the keyboard), so we have to do that manually: A
    // new popup gets the focus
    if (popup->focusWidget()) {
        popup->focusWidget()->setFocus(Qt::PopupFocusReason);
    } else if (QApplicationPrivate::popupWidgets->count() == 1) { // this was the first popup
        if (QWidget *fw = q_func()->focusWidget()) {
            QFocusEvent e(QEvent::FocusOut, Qt::PopupFocusReason);
            q_func()->sendEvent(fw, &e);
        }
    }
}

void QApplicationPrivate::closePopup(QWidget *popup)
{
    if (!QApplicationPrivate::popupWidgets)
        return;
    QApplicationPrivate::popupWidgets->removeAll(popup);
    POINT curPos;
    GetCursorPos(&curPos);

    if (QApplicationPrivate::popupWidgets->isEmpty()) { // this was the last popup
        delete QApplicationPrivate::popupWidgets;
        QApplicationPrivate::popupWidgets = 0;
        replayPopupMouseEvent = (!popup->geometry().contains(QPoint(curPos.x, curPos.y))
                                && !popup->testAttribute(Qt::WA_NoMouseReplay));
        if (!popup->isEnabled())
            return;
        if (!qt_nograb())                        // grabbing not disabled
            releaseAutoCapture();
        QWidget *fw = QApplicationPrivate::active_window ? QApplicationPrivate::active_window->focusWidget()
            : q_func()->focusWidget();
        if (fw) {
            if (fw != q_func()->focusWidget()) {
                fw->setFocus(Qt::PopupFocusReason);
            } else {
                QFocusEvent e(QEvent::FocusIn, Qt::PopupFocusReason);
                q_func()->sendEvent(fw, &e);
            }
        }
    } else {
        // Popups are not focus-handled by the window system (the
        // first popup grabbed the keyboard), so we have to do that
        // manually: A popup was closed, so the previous popup gets
        // the focus.
        QWidget* aw = QApplicationPrivate::popupWidgets->last();
        if (QApplicationPrivate::popupWidgets->count() == 1) {
            Q_ASSERT(aw->testAttribute(Qt::WA_WState_Created));
            setAutoCapture(aw->internalWinId());
        }
        if (QWidget *fw = aw->focusWidget())
            fw->setFocus(Qt::PopupFocusReason);
    }
}




/*****************************************************************************
  Event translation; translates Windows events to Qt events
 *****************************************************************************/

//
// Auto-capturing for mouse press and mouse release
//

static void setAutoCapture(HWND h)
{
    if (autoCaptureWnd)
        releaseAutoCapture();
    autoCaptureWnd = h;
    SetCapture(h);
}

static void releaseAutoCapture()
{
    if (autoCaptureWnd) {
        ReleaseCapture();
        autoCaptureWnd = 0;
    }
}


//
// Mouse event translation
//
// Non-client mouse messages are not translated
//

static ushort mouseTbl[] = {
    WM_MOUSEMOVE,        QEvent::MouseMove,                0,
    WM_LBUTTONDOWN,      QEvent::MouseButtonPress,        Qt::LeftButton,
    WM_LBUTTONUP,        QEvent::MouseButtonRelease,      Qt::LeftButton,
    WM_LBUTTONDBLCLK,    QEvent::MouseButtonDblClick,     Qt::LeftButton,
    WM_RBUTTONDOWN,      QEvent::MouseButtonPress,        Qt::RightButton,
    WM_RBUTTONUP,        QEvent::MouseButtonRelease,      Qt::RightButton,
    WM_RBUTTONDBLCLK,    QEvent::MouseButtonDblClick,     Qt::RightButton,
    WM_MBUTTONDOWN,      QEvent::MouseButtonPress,        Qt::MidButton,
    WM_MBUTTONUP,        QEvent::MouseButtonRelease,      Qt::MidButton,
    WM_MBUTTONDBLCLK,    QEvent::MouseButtonDblClick,     Qt::MidButton,
    // use XButton1 for now, the real X button is decided later
    WM_XBUTTONDOWN,      QEvent::MouseButtonPress,        Qt::XButton1,
    WM_XBUTTONUP,        QEvent::MouseButtonRelease,      Qt::XButton1,
    WM_XBUTTONDBLCLK,    QEvent::MouseButtonDblClick,     Qt::XButton1,

    WM_NCMOUSEMOVE,      QEvent::NonClientAreaMouseMove,           0,
    WM_NCLBUTTONDOWN,    QEvent::NonClientAreaMouseButtonPress,    Qt::LeftButton,
    WM_NCLBUTTONUP,      QEvent::NonClientAreaMouseButtonRelease,  Qt::LeftButton,
    WM_NCLBUTTONDBLCLK,  QEvent::NonClientAreaMouseButtonDblClick, Qt::LeftButton,
    WM_NCRBUTTONDOWN,    QEvent::NonClientAreaMouseButtonPress,    Qt::RightButton,
    WM_NCRBUTTONUP,      QEvent::NonClientAreaMouseButtonRelease,  Qt::RightButton,
    WM_NCRBUTTONDBLCLK,  QEvent::NonClientAreaMouseButtonDblClick, Qt::RightButton,
    WM_NCMBUTTONDOWN,    QEvent::NonClientAreaMouseButtonPress,    Qt::MidButton,
    WM_NCMBUTTONUP,      QEvent::NonClientAreaMouseButtonRelease,  Qt::MidButton,
    WM_NCMBUTTONDBLCLK,  QEvent::NonClientAreaMouseButtonDblClick, Qt::MidButton,

    0,                        0,                                0
};

static int translateButtonState(int s, int type, int button)
{
    Q_UNUSED(type);
    Q_UNUSED(button);
    int bst = 0;
    if (s & MK_LBUTTON)
        bst |= Qt::LeftButton;
    if (s & MK_MBUTTON)
        bst |= Qt::MidButton;
    if (s & MK_RBUTTON)
        bst |= Qt::RightButton;
    if (s & MK_SHIFT)
        bst |= Qt::ShiftModifier;
    if (s & MK_CONTROL)
        bst |= Qt::ControlModifier;

    if (s & MK_XBUTTON1)
        bst |= Qt::XButton1;
    if (s & MK_XBUTTON2)
        bst |= Qt::XButton2;

    if (GetKeyState(VK_MENU) < 0)
        bst |= Qt::AltModifier;

    if ((GetKeyState(VK_LWIN) < 0) ||
         (GetKeyState(VK_RWIN) < 0))
        bst |= Qt::MetaModifier;

    return bst;
}

void qt_win_eatMouseMove()
{
    // after closing a windows dialog with a double click (i.e. open a file)
    // the message queue still contains a dubious WM_MOUSEMOVE message where
    // the left button is reported to be down (wParam != 0).
    // remove all those messages (usually 1) and post the last one with a
    // reset button state

    MSG msg = {0, 0, 0, 0, 0, 0, 0};
    QT_WA( {
        while (PeekMessage(&msg, 0, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE))
            ;
        if (msg.message == WM_MOUSEMOVE)
            PostMessage(msg.hwnd, msg.message, 0, msg.lParam);
    }, {
        MSG msg;
        msg.message = 0;
        while (PeekMessageA(&msg, 0, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE))
            ;
        if (msg.message == WM_MOUSEMOVE)
            PostMessageA(msg.hwnd, msg.message, 0, msg.lParam);
    } );
}

// In DnD, the mouse release event never appears, so the
// mouse button state machine must be manually reset
/*! \internal */
void QApplication::winMouseButtonUp()
{
    qt_button_down = 0;
    releaseAutoCapture();
}

void QETWidget::repolishStyle(QStyle &)
{
    QEvent e(QEvent::StyleChange);
    QApplication::sendEvent(this, &e);
}

bool QETWidget::translateMouseEvent(const MSG &msg)
{
    static QPoint pos;
    static POINT gpos={-1,-1};
    QEvent::Type type;                                // event parameters
    int           button;
    int           state;
    int           i;

    if (sm_blockUserInput) //block user interaction during session management
        return true;

    // Compress mouse move events
    if (msg.message == WM_MOUSEMOVE) {
        MSG mouseMsg;
        while (winPeekMessage(&mouseMsg, msg.hwnd, WM_MOUSEFIRST,
                WM_MOUSELAST, PM_NOREMOVE)) {
            if (mouseMsg.message == WM_MOUSEMOVE) {
#define PEEKMESSAGE_IS_BROKEN 1
#ifdef PEEKMESSAGE_IS_BROKEN
                // Since the Windows PeekMessage() function doesn't
                // correctly return the wParam for WM_MOUSEMOVE events
                // if there is a key release event in the queue
                // _before_ the mouse event, we have to also consider
                // key release events (kls 2003-05-13):
                MSG keyMsg;
                bool done = false;
                while (winPeekMessage(&keyMsg, 0, WM_KEYFIRST, WM_KEYLAST,
                        PM_NOREMOVE)) {
                    if (keyMsg.time < mouseMsg.time) {
                        if ((keyMsg.lParam & 0xC0000000) == 0x40000000) {
                            winPeekMessage(&keyMsg, 0, keyMsg.message,
                                            keyMsg.message, PM_REMOVE);
                        } else {
                            done = true;
                            break;
                        }
                    } else {
                        break; // no key event before the WM_MOUSEMOVE event
                    }
                }
                if (done)
                    break;
#else
                // Actually the following 'if' should work instead of
                // the above key event checking, but apparently
                // PeekMessage() is broken :-(
                if (mouseMsg.wParam != msg.wParam)
                    break; // leave the message in the queue because
                           // the key state has changed
#endif
                MSG *msgPtr = (MSG *)(&msg);
                // Update the passed in MSG structure with the
                // most recent one.
                msgPtr->lParam = mouseMsg.lParam;
                msgPtr->wParam = mouseMsg.wParam;
                msgPtr->pt = mouseMsg.pt;
                // Remove the mouse move message
                winPeekMessage(&mouseMsg, msg.hwnd, WM_MOUSEMOVE,
                                WM_MOUSEMOVE, PM_REMOVE);
            } else {
                break; // there was no more WM_MOUSEMOVE event
            }
        }
    }

    for (i=0; (UINT)mouseTbl[i] != msg.message && mouseTbl[i]; i += 3)
        ;
    if (!mouseTbl[i])
        return false;
    type   = (QEvent::Type)mouseTbl[++i];        // event type
    button = mouseTbl[++i];                        // which button
    if (button == Qt::XButton1) {
        switch(GET_XBUTTON_WPARAM(msg.wParam)) {
        case XBUTTON1:
            button = Qt::XButton1;
            break;
        case XBUTTON2:
            button = Qt::XButton2;
            break;
        }
    }
    state  = translateButtonState(msg.wParam, type, button); // button state

    if (type == QEvent::MouseMove || type == QEvent::NonClientAreaMouseMove) {
        if (!(state & Qt::MouseButtonMask))
            qt_button_down = 0;

        QCursor *c = qt_grab_cursor();
        if (!c)
            c = QApplication::overrideCursor();
        if (c)                                // application cursor defined
            SetCursor(c->handle());
        else if (type != QEvent::NonClientAreaMouseMove) {
            QWidget *w = this; // use  widget cursor if widget is enabled
            while (!w->isWindow() && !w->isEnabled())
                w = w->parentWidget();
            SetCursor(w->cursor().handle());
        }

        HWND id = internalWinId();
        if (type == QEvent::NonClientAreaMouseMove)
            id = 0;

        if (curWin != id) {                // new current window
            QApplicationPrivate::dispatchEnterLeave(id == 0 ? 0 : this, QWidget::find(curWin));
            curWin = id;
#ifndef Q_OS_TEMP

            if (curWin != 0) {
                static bool trackMouseEventLookup = false;
                typedef BOOL (WINAPI *PtrTrackMouseEvent)(LPTRACKMOUSEEVENT);
                static PtrTrackMouseEvent ptrTrackMouseEvent = 0;
                if (!trackMouseEventLookup) {
                    trackMouseEventLookup = true;
                    ptrTrackMouseEvent = (PtrTrackMouseEvent)QLibrary::resolve(QLatin1String("comctl32"), "_TrackMouseEvent");
                }
                if (ptrTrackMouseEvent && !qApp->d_func()->inPopupMode()) {
                    // We always have to set the tracking, since
                    // Windows detects more leaves than we do..
                    TRACKMOUSEEVENT tme;
                    tme.cbSize = sizeof(TRACKMOUSEEVENT);
                    tme.dwFlags = 0x00000002;    // TME_LEAVE
                    tme.hwndTrack = curWin;      // Track on window receiving msgs
                    tme.dwHoverTime = (DWORD)-1; // HOVER_DEFAULT
                    ptrTrackMouseEvent(&tme);
                }
            }
#endif // Q_OS_TEMP
        }

        POINT curPos = msg.pt;
        if (curPos.x == gpos.x && curPos.y == gpos.y)
            return true;                        // same global position
        gpos = curPos;

        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
        ScreenToClient(internalWinId(), &curPos);

        pos.rx() = curPos.x;
        pos.ry() = curPos.y;
        pos = d_func()->mapFromWS(pos);
    } else {
        gpos = msg.pt;
        pos = mapFromGlobal(QPoint(gpos.x, gpos.y));

        if (type == QEvent::MouseButtonPress || type == QEvent::MouseButtonDblClick) {        // mouse button pressed
            QWidget *tlw = window();
            qt_button_down = tlw->childAt(mapTo(tlw, pos));
            if (!qt_button_down)
                qt_button_down = this;
        }
    }

    bool res = false;

    bool nonClientAreaEvent = type >= QEvent::NonClientAreaMouseMove
                                && type <= QEvent::NonClientAreaMouseButtonDblClick;

    if (qApp->d_func()->inPopupMode()) {                        // in popup mode

        if (nonClientAreaEvent)
            return false;

        replayPopupMouseEvent = false;
        QWidget* activePopupWidget = qApp->activePopupWidget();
        QWidget *target = activePopupWidget;

        if (target != this) {
            if ((windowType() == Qt::Popup) && rect().contains(pos) && 0)
                target = this;
            else                                // send to last popup
                pos = target->mapFromGlobal(QPoint(gpos.x, gpos.y));
        }
        QWidget *popupChild = target->childAt(pos);
        bool releaseAfter = false;
        switch (type) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonDblClick:
                popupButtonFocus = popupChild;
                break;
            case QEvent::MouseButtonRelease:
                releaseAfter = true;
                break;
            default:
                break;                                // nothing for mouse move
        }

        QPoint globalPos(gpos.x, gpos.y);
        if (target->isEnabled()) {
            if (popupButtonFocus) {
                target = popupButtonFocus;
            } else if (popupChild) {
                // forward mouse events to the popup child. mouse move events
                // are only forwarded to popup children that enable mouse tracking.
                if (type != QEvent::MouseMove || popupChild->hasMouseTracking())
                    target = popupChild;
            }

            pos = target->mapFromGlobal(globalPos);
                QMouseEvent e(type, pos, globalPos,
                            Qt::MouseButton(button),
                            Qt::MouseButtons(state & Qt::MouseButtonMask),
                            Qt::KeyboardModifiers(state & Qt::KeyboardModifierMask));
                res = QApplication::sendSpontaneousEvent(target, &e);
            res = res && e.isAccepted();
        } else {
            // close disabled popups when a mouse button is pressed or released
            switch (type) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonDblClick:
            case QEvent::MouseButtonRelease:
                target->close();
                break;
            default:
                break;
            }
        }

        if (releaseAfter) {
            popupButtonFocus = 0;
            qt_button_down = 0;
        }

        if (type == QEvent::MouseButtonPress
            && qApp->activePopupWidget() != activePopupWidget
            && replayPopupMouseEvent) {
            // the popup dissappeared. Replay the event
            QWidget* w = QApplication::widgetAt(gpos.x, gpos.y);
            if (w && !QApplicationPrivate::isBlockedByModal(w)) {
                Q_ASSERT(w->testAttribute(Qt::WA_WState_Created));
                if (QWidget::mouseGrabber() == 0)
                    setAutoCapture(w->internalWinId());
                if (!w->isActiveWindow())
                    w->activateWindow();
                POINT widgetpt = gpos;
                ScreenToClient(w->internalWinId(), &widgetpt);
                LPARAM lParam = MAKELPARAM(widgetpt.x, widgetpt.y);
                winPostMessage(w->internalWinId(), msg.message, msg.wParam, lParam);
            }
        } else if (type == QEvent::MouseButtonRelease && button == Qt::RightButton
                && qApp->activePopupWidget() == activePopupWidget) {
            // popup still alive and received right-button-release
            QContextMenuEvent e2(QContextMenuEvent::Mouse, pos, globalPos);
            bool res2 = QApplication::sendSpontaneousEvent( target, &e2 );
            if (!res) // RMB not accepted
                res = res2 && e2.isAccepted();
        }
    } else {                                        // not popup mode
        int bs = state & Qt::MouseButtonMask;
        if ((type == QEvent::MouseButtonPress ||
              type == QEvent::MouseButtonDblClick) && bs == button) {
            Q_ASSERT(testAttribute(Qt::WA_WState_Created));
            if (QWidget::mouseGrabber() == 0)
                setAutoCapture(internalWinId());
        } else if (type == QEvent::MouseButtonRelease && bs == 0) {
            if (QWidget::mouseGrabber() == 0)
                releaseAutoCapture();
        }

        QWidget *widget = this;
        QWidget *w = QWidget::mouseGrabber();

        if (((type == QEvent::MouseMove && bs)
             || (type == QEvent::MouseButtonRelease))
            && !qt_button_down && !w)
            return false; // don't send event

        if (!w)
            w = qt_button_down;
        if (w && w != this) {
            widget = w;
            pos = w->mapFromGlobal(QPoint(gpos.x, gpos.y));
        }

        if (type == QEvent::MouseButtonRelease &&
             (state & Qt::MouseButtonMask) == 0) {
            qt_button_down = 0;
        }
        QMouseEvent e(type, pos, QPoint(gpos.x,gpos.y),
                      Qt::MouseButton(button),
                      Qt::MouseButtons(state & Qt::MouseButtonMask),
                      Qt::KeyboardModifiers(state & Qt::KeyboardModifierMask));
        res = QApplication::sendSpontaneousEvent(widget, &e);
        // non client area events are only informational, you cannot "handle" them
        res = res && e.isAccepted() && !nonClientAreaEvent;
        if (type == QEvent::MouseButtonRelease && button == Qt::RightButton) {
            QContextMenuEvent e2(QContextMenuEvent::Mouse, pos, QPoint(gpos.x,gpos.y));
            bool res2 = QApplication::sendSpontaneousEvent(widget, &e2);
            if (!res)
                res = res2 && e2.isAccepted();
        }

        if (type != QEvent::MouseMove)
            pos.rx() = pos.ry() = -9999;        // init for move compression
    }
    return res;
}

bool QETWidget::translateWheelEvent(const MSG &msg)
{
    int  state = 0;

    if (sm_blockUserInput) // block user interaction during session management
        return true;

    state = translateButtonState(GET_KEYSTATE_WPARAM(msg.wParam), 0, 0);

    int delta;
    if (msg.message == WM_MOUSEWHEEL)
        delta = (short) HIWORD (msg.wParam);
    else
        delta = (int) msg.wParam;

    Qt::Orientation orient = (state&Qt::AltModifier
#if 0 // disabled for now - Trenton's one-wheel mouse makes trouble...
    // "delta" for usual wheels is +-120. +-240 seems to indicate the second wheel
    // see more recent MSDN for WM_MOUSEWHEEL

         || delta == 240 || delta == -240)?Qt::Horizontal:Vertical;
    if (delta == 240 || delta == -240)
        delta /= 2;
#endif
       ) ? Qt::Horizontal : Qt::Vertical;

    QPoint globalPos;

    globalPos.rx() = (short)LOWORD (msg.lParam);
    globalPos.ry() = (short)HIWORD (msg.lParam);


    // if there is a widget under the mouse and it is not shadowed
    // by modality, we send the event to it first
    int ret = 0;
    QWidget* w = QApplication::widgetAt(globalPos);
    if (!w || !qt_try_modal(w, (MSG*)&msg, ret)) {
        //synaptics touchpad shows its own widget at this position
        //so widgetAt() will fail with that HWND, try child of this widget
        w = this->childAt(this->mapFromGlobal(globalPos));
        if (!w)
            w = this;
    }

    // send the event to the widget or its ancestors
    {
        QWidget* popup = qApp->activePopupWidget();
        if (popup && w->window() != popup)
            popup->close();
        QWheelEvent e(w->mapFromGlobal(globalPos), globalPos, delta,
                      Qt::MouseButtons(state & Qt::MouseButtonMask),
                      Qt::KeyboardModifier(state & Qt::KeyboardModifierMask), orient);
        if (QApplication::sendSpontaneousEvent(w, &e))
            return true;
    }

    // send the event to the widget that has the focus or its ancestors, if different
    if (w != qApp->focusWidget() && (w = qApp->focusWidget())) {
        QWidget* popup = qApp->activePopupWidget();
        if (popup && w->window() != popup)
            popup->close();
        QWheelEvent e(w->mapFromGlobal(globalPos), globalPos, delta,
                      Qt::MouseButtons(state & Qt::MouseButtonMask),
                      Qt::KeyboardModifier(state & Qt::KeyboardModifierMask), orient);
        if (QApplication::sendSpontaneousEvent(w, &e))
            return true;
    }
    return false;
}


//
// Windows Wintab to QTabletEvent translation
//

// the following is adapted from the wintab syspress example (public domain)
/* -------------------------------------------------------------------------- */
static void tabletInit(UINT wActiveCsr, HCTX hTab)
{
    /* browse WinTab's many info items to discover pressure handling. */
    if (ptrWTInfo && ptrWTGet) {
        AXIS np;
        LOGCONTEXT lc;
        BYTE wPrsBtn;
        BYTE logBtns[32];
        UINT size;

        /* discover the LOGICAL button generated by the pressure channel. */
        /* get the PHYSICAL button from the cursor category and run it */
        /* through that cursor's button map (usually the identity map). */
        wPrsBtn = (BYTE)-1;
        ptrWTInfo(WTI_CURSORS + wActiveCsr, CSR_NPBUTTON, &wPrsBtn);
        size = ptrWTInfo(WTI_CURSORS + wActiveCsr, CSR_BUTTONMAP, &logBtns);
        if ((UINT)wPrsBtn < size)
            wPrsBtn = logBtns[wPrsBtn];

        /* get the current context for its device variable. */
        ptrWTGet(hTab, &lc);

        /* get the size of the pressure axis. */
        QTabletDeviceData tdd;
        ptrWTInfo(WTI_DEVICES + lc.lcDevice, DVC_NPRESSURE, &np);
        tdd.minPressure = int(np.axMin);
        tdd.maxPressure = int(np.axMax);

        ptrWTInfo(WTI_DEVICES + lc.lcDevice, DVC_TPRESSURE, &np);
        tdd.minTanPressure = int(np.axMin);
        tdd.maxTanPressure = int(np.axMax);

        ptrWTInfo(WTI_DEVICES + lc.lcDevice, DVC_X, &np);
        tdd.minX = int(np.axMin);
        tdd.maxX = int(np.axMax);

        ptrWTInfo(WTI_DEVICES + lc.lcDevice, DVC_Y, &np);
        tdd.minY = int(np.axMin);
        tdd.maxY = int(np.axMax);

        ptrWTInfo(WTI_DEVICES + lc.lcDevice, DVC_Z, &np);
        tdd.minZ = int(np.axMin);
        tdd.maxZ = int(np.axMax);

        int csr_type,
            csr_physid;
        ptrWTInfo(WTI_CURSORS + wActiveCsr, CSR_TYPE, &csr_type);
        ptrWTInfo(WTI_CURSORS + wActiveCsr, CSR_PHYSID, &csr_physid);
        tdd.llId = csr_type & 0x0F06;
        tdd.llId = (tdd.llId << 24) | csr_physid;
        switch (csr_type & 0x0F06) {
        case 0x0802:
            tdd.currentDevice = QTabletEvent::Stylus;
            break;
        case 0x0902:
            tdd.currentDevice = QTabletEvent::Airbrush;
            break;
        case 0x0004:
            tdd.currentDevice = QTabletEvent::FourDMouse;
            break;
        case 0x0006:
            tdd.currentDevice = QTabletEvent::Puck;
            break;
        case 0x0804:
            tdd.currentDevice = QTabletEvent::RotationStylus;
            break;
        default:
            tdd.currentDevice = QTabletEvent::NoDevice;
        }

        switch (wActiveCsr % 3) {
        case 2:
            tdd.currentPointerType = QTabletEvent::Eraser;
            break;
        case 1:
            tdd.currentPointerType = QTabletEvent::Pen;
            break;
        case 0:
            tdd.currentPointerType = QTabletEvent::Cursor;
            break;
        default:
            tdd.currentPointerType = QTabletEvent::UnknownPointer;
        }
        tCursorInfo()->insert(wActiveCsr, tdd);
    }
}

bool QETWidget::translateTabletEvent(const MSG &msg, PACKET *localPacketBuf,
                                      int numPackets)
{
    Q_UNUSED(msg);
    POINT ptNew;
    static DWORD btnNew, btnOld, btnChange;
    qreal prsNew;
    ORIENTATION ort;
    static bool button_pressed = false;
    int i,
        tiltX,
        tiltY;
    bool sendEvent = false;
    QEvent::Type t;
    int z = 0;
    qreal rotation = 0.0;
    qreal tangentialPressure;

    POINT point;
    GetCursorPos(&point);

    // the most common event that we get...
    t = QEvent::TabletMove;
    for (i = 0; i < numPackets; i++) {
        // get the unique ID of the device...
        btnOld = btnNew;
        btnNew = localPacketBuf[i].pkButtons;
        btnChange = btnOld ^ btnNew;

        if (btnNew & btnChange) {
            button_pressed = true;
            t = QEvent::TabletPress;
        }
        ptNew.x = UINT(localPacketBuf[i].pkX);
        ptNew.y = UINT(localPacketBuf[i].pkY);

        z = (currentTabletPointer.currentDevice == QTabletEvent::FourDMouse) ? UINT(localPacketBuf[i].pkZ) : 0;

        prsNew = 0.0;
        QRect desktopArea = QApplication::desktop()->geometry();
        QPointF hiResGlobal = currentTabletPointer.scaleCoord(ptNew.x, ptNew.y, desktopArea.left(),
                                                              desktopArea.width(), desktopArea.top(),
                                                              desktopArea.height());

        // adjust to current on screen cursor pos. (only really needed when tablet is in mouse mode)
        hiResGlobal.setX(hiResGlobal.x() - int(hiResGlobal.x() - .5) + point.x);
        hiResGlobal.setY(hiResGlobal.y() - int(hiResGlobal.y() - .5) + point.y);

        if (btnNew) {
            if (currentTabletPointer.currentPointerType == QTabletEvent::Pen || currentTabletPointer.currentPointerType == QTabletEvent::Eraser)
                prsNew = localPacketBuf[i].pkNormalPressure
                            / qreal(currentTabletPointer.maxPressure
                                    - currentTabletPointer.minPressure);
            else
                prsNew = 0;
        } else if (button_pressed) {
            // One button press, should only give one button release
            t = QEvent::TabletRelease;
            button_pressed = false;
        }
        QPoint globalPos(qRound(hiResGlobal.x()), qRound(hiResGlobal.y()));

        // make sure the tablet event get's sent to the proper widget...
        QWidget *w = QApplication::widgetAt(globalPos);
        if (qt_button_down)
            w = qt_button_down; // Pass it to the thing that's grabbed it.

        if (!w)
            w = this;
        QPoint localPos = w->mapFromGlobal(globalPos);
        if (currentTabletPointer.currentDevice == QTabletEvent::Airbrush) {
            tangentialPressure = localPacketBuf[i].pkTangentPressure
                                / qreal(currentTabletPointer.maxTanPressure
                                        - currentTabletPointer.minTanPressure);
        } else {
            tangentialPressure = 0.0;
        }

        if (!qt_tablet_tilt_support) {
            tiltX = tiltY = 0;
            rotation = 0.0;
        } else {
            ort = localPacketBuf[i].pkOrientation;
            // convert from azimuth and altitude to x tilt and y tilt
            // what follows is the optimized version.  Here are the equations
            // I used to get to this point (in case things change :)
            // X = sin(azimuth) * cos(altitude)
            // Y = cos(azimuth) * cos(altitude)
            // Z = sin(altitude)
            // X Tilt = arctan(X / Z)
            // Y Tilt = arctan(Y / Z)
            double radAzim = (ort.orAzimuth / 10) * (Q_PI / 180);
            //double radAlt = abs(ort.orAltitude / 10) * (Q_PI / 180);
            double tanAlt = tan((abs(ort.orAltitude / 10)) * (Q_PI / 180));

            double degX = atan(sin(radAzim) / tanAlt);
            double degY = atan(cos(radAzim) / tanAlt);
            tiltX = int(degX * (180 / Q_PI));
            tiltY = int(-degY * (180 / Q_PI));
            rotation = ort.orTwist;
        }

        QTabletEvent e(t, localPos, globalPos, hiResGlobal, currentTabletPointer.currentDevice,
                       currentTabletPointer.currentPointerType, prsNew, tiltX, tiltY,
                       tangentialPressure, rotation, z, 0, currentTabletPointer.llId);
        sendEvent = QApplication::sendSpontaneousEvent(w, &e);
    }
    return sendEvent;
}

extern bool qt_is_gui_used;
static void initWinTabFunctions()
{
    if (!qt_is_gui_used)
        return;

    QLibrary library(QLatin1String("wintab32"));
    if (library.load()) {
        QT_WA({
            ptrWTInfo = (PtrWTInfo)library.resolve("WTInfoW");
            ptrWTGet = (PtrWTGet)library.resolve("WTGetW");
        } , {
            ptrWTInfo = (PtrWTInfo)library.resolve("WTInfoA");
            ptrWTGet = (PtrWTGet)library.resolve("WTGetA");
        });

        ptrWTEnable = (PtrWTEnable)library.resolve("WTEnable");
        ptrWTOverlap = (PtrWTEnable)library.resolve("WTOverlap");
        ptrWTPacketsGet = (PtrWTPacketsGet)library.resolve("WTPacketsGet");
    }
}


//
// Paint event translation
//
bool QETWidget::translatePaintEvent(const MSG &msg)
{
    QRegion rgn(0, 0, 1, 1);
    Q_ASSERT(testAttribute(Qt::WA_WState_Created));
    int res = GetUpdateRgn(internalWinId(), rgn.handle(), FALSE);
    if (!GetUpdateRect(internalWinId(), 0, FALSE)  // The update bounding rect is invalid
         || (res == ERROR)
         || (res == NULLREGION)) {
        d_func()->hd = 0;
        setAttribute(Qt::WA_PendingUpdate, false);
        return false;
    }
    if(msg.message == WM_ERASEBKGND) {
        QWidgetBackingStore::blitToScreen(rgn, this);
    } else {
        setAttribute(Qt::WA_PendingUpdate, false);
        PAINTSTRUCT ps;
        d_func()->hd = BeginPaint(internalWinId(), &ps);

        // Mapping region from system to qt (32 bit) coordinate system.
        rgn.translate(data->wrect.topLeft());
        qt_syncBackingStore(rgn, this, (msg.message == WM_QT_REPAINT));
        d_func()->hd = 0;
        EndPaint(internalWinId(), &ps);
    }
    return true;
}

//
// Window move and resize (configure) events
//

bool QETWidget::translateConfigEvent(const MSG &msg)
{
    if (!testAttribute(Qt::WA_WState_Created))                // in QWidget::create()
        return true;
    if (testAttribute(Qt::WA_WState_ConfigPending))
        return true;
    if (!isWindow())
        return true;
    setAttribute(Qt::WA_WState_ConfigPending);                // set config flag
    QRect cr = geometry();
    if (msg.message == WM_SIZE) {                // resize event
        WORD a = LOWORD(msg.lParam);
        WORD b = HIWORD(msg.lParam);
        QSize oldSize = size();
        QSize newSize(a, b);
        cr.setSize(newSize);
        if (msg.wParam != SIZE_MINIMIZED)
            data->crect = cr;
        if (isWindow()) {                        // update title/icon text
            d_func()->createTLExtra();
            // Capture SIZE_MINIMIZED without preceding WM_SYSCOMMAND
            // (like Windows+M)
            if (msg.wParam == SIZE_MINIMIZED && !isMinimized()) {
            data->window_state |= Qt::WindowMinimized;
            if (isVisible()) {
                QHideEvent e;
                QApplication::sendSpontaneousEvent(this, &e);
                hideChildren(true);
            }
        } else if (msg.wParam != SIZE_MINIMIZED && isMinimized()) {
            data->window_state &= ~Qt::WindowMinimized;
            showChildren(true);
            QShowEvent e;
            QApplication::sendSpontaneousEvent(this, &e);
        }
        QString txt;
#ifndef Q_OS_TEMP
        if (IsIconic(internalWinId()) && windowIconText().size())
            txt = windowIconText();
        else
#endif
        txt = windowTitle();
        if (!txt.isEmpty())
            d_func()->setWindowTitle_helper(txt);
    }
    if (msg.wParam != SIZE_MINIMIZED && oldSize != newSize) {
        if (isVisible()) {
            QResizeEvent e(newSize, oldSize);
            QApplication::sendSpontaneousEvent(this, &e);
            if (!testAttribute(Qt::WA_StaticContents)) {
                if(QWidgetBackingStore::paintOnScreen(this))
                    repaint();
                else
                    qt_syncBackingStore(d_func()->clipRect(), this);
            }
        } else {
            QResizeEvent *e = new QResizeEvent(newSize, oldSize);
            QApplication::postEvent(this, e);
        }
    }
} else if (msg.message == WM_MOVE) {        // move event
        int a = (int) (short) LOWORD(msg.lParam);
        int b = (int) (short) HIWORD(msg.lParam);
        QPoint oldPos = geometry().topLeft();
        QPoint newCPos(a, b);
        // Ignore silly Windows move event to wild pos after iconify.
        if (!IsIconic(internalWinId()) && newCPos != oldPos) {
            cr.moveTopLeft(newCPos);
            data->crect = cr;
            if (isVisible()) {
                QMoveEvent e(newCPos, oldPos);  // cpos (client position)
                QApplication::sendSpontaneousEvent(this, &e);
            } else {
                QMoveEvent * e = new QMoveEvent(newCPos, oldPos);
                QApplication::postEvent(this, e);
            }
        }
    }
    setAttribute(Qt::WA_WState_ConfigPending, false);                // clear config flag
    return true;
}


//
// Close window event translation.
//
// This class is a friend of QApplication because it needs to emit the
// lastWindowClosed() signal when the last top level widget is closed.
//

bool QETWidget::translateCloseEvent(const MSG &)
{
    return d_func()->close_helper(QWidgetPrivate::CloseWithSpontaneousEvent);
}


void  QApplication::setCursorFlashTime(int msecs)
{
    SetCaretBlinkTime(msecs / 2);
    QApplicationPrivate::cursor_flash_time = msecs;
}


int QApplication::cursorFlashTime()
{
    int blink = (int)GetCaretBlinkTime();
    if (!blink)
        return QApplicationPrivate::cursor_flash_time;
    if (blink > 0)
        return 2*blink;
    return 0;
}


void QApplication::setDoubleClickInterval(int ms)
{
#ifndef Q_OS_TEMP
    SetDoubleClickTime(ms);
#endif
    QApplicationPrivate::mouse_double_click_time = ms;
}

int QApplication::doubleClickInterval()
{
    int ms = GetDoubleClickTime();
    if (ms != 0)
        return ms;
    return QApplicationPrivate::mouse_double_click_time;
}


void QApplication::setKeyboardInputInterval(int ms)
{
    QApplicationPrivate::keyboard_input_time = ms;
}

int QApplication::keyboardInputInterval()
{
    // FIXME: get from the system
    return QApplicationPrivate::keyboard_input_time;
}


void QApplication::setWheelScrollLines(int n)
{
#ifdef SPI_SETWHEELSCROLLLINES
    if (n < 0)
        n = 0;
    QT_WA({
        SystemParametersInfo(SPI_SETWHEELSCROLLLINES, (uint)n, 0, 0);
    } , {
        SystemParametersInfoA(SPI_SETWHEELSCROLLLINES, (uint)n, 0, 0);
    });
#else
    QApplicationPrivate::wheel_scroll_lines = n;
#endif
}

int QApplication::wheelScrollLines()
{
#ifdef SPI_GETWHEELSCROLLLINES
    uint i = 3;
    QT_WA({
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, sizeof(uint), &i, 0);
    } , {
        SystemParametersInfoA(SPI_GETWHEELSCROLLLINES, sizeof(uint), &i, 0);
    });
    if (i > INT_MAX)
        i = INT_MAX;
    return i;
#else
    return QApplicationPrivate::wheel_scroll_lines;
#endif
}

static bool effect_override = false;

void QApplication::setEffectEnabled(Qt::UIEffect effect, bool enable)
{
    effect_override = true;
    switch (effect) {
    case Qt::UI_AnimateMenu:
        QApplicationPrivate::animate_menu = enable;
        break;
    case Qt::UI_FadeMenu:
        QApplicationPrivate::fade_menu = enable;
        break;
    case Qt::UI_AnimateCombo:
        QApplicationPrivate::animate_combo = enable;
        break;
    case Qt::UI_AnimateTooltip:
        QApplicationPrivate::animate_tooltip = enable;
        break;
    case Qt::UI_FadeTooltip:
        QApplicationPrivate::fade_tooltip = enable;
        break;
    case Qt::UI_AnimateToolBox:
        QApplicationPrivate::animate_toolbox = enable;
        break;
    default:
        QApplicationPrivate::animate_ui = enable;
        break;
    }
}

bool QApplication::isEffectEnabled(Qt::UIEffect effect)
{
    if (QColormap::instance().depth() < 16)
        return false;

    if (!effect_override && desktopSettingsAware()
        && !(QSysInfo::WindowsVersion == QSysInfo::WV_95 || QSysInfo::WindowsVersion == QSysInfo::WV_NT)) {
        // we know that they can be used when we are here
        BOOL enabled = false;
        UINT api;
        switch (effect) {
        case Qt::UI_AnimateMenu:
            api = SPI_GETMENUANIMATION;
            break;
        case Qt::UI_FadeMenu:
            if (QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based)
                return false;
            api = SPI_GETMENUFADE;
            break;
        case Qt::UI_AnimateCombo:
            api = SPI_GETCOMBOBOXANIMATION;
            break;
        case Qt::UI_AnimateTooltip:
            if (QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based)
                api = SPI_GETMENUANIMATION;
            else
                api = SPI_GETTOOLTIPANIMATION;
            break;
        case Qt::UI_FadeTooltip:
            if (QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based)
                return false;
            api = SPI_GETTOOLTIPFADE;
            break;
        default:
            api = SPI_GETUIEFFECTS;
            break;
        }
        QT_WA({
            SystemParametersInfo(api, 0, &enabled, 0);
        } , {
            SystemParametersInfoA(api, 0, &enabled, 0);
        });
        return enabled;
    }

    switch(effect) {
    case Qt::UI_AnimateMenu:
        return QApplicationPrivate::animate_menu;
    case Qt::UI_FadeMenu:
        return QApplicationPrivate::fade_menu;
    case Qt::UI_AnimateCombo:
        return QApplicationPrivate::animate_combo;
    case Qt::UI_AnimateTooltip:
        return QApplicationPrivate::animate_tooltip;
    case Qt::UI_FadeTooltip:
        return QApplicationPrivate::fade_tooltip;
    case Qt::UI_AnimateToolBox:
        return QApplicationPrivate::animate_toolbox;
    default:
        return QApplicationPrivate::animate_ui;
    }
}

bool QSessionManager::allowsInteraction()
{
    sm_blockUserInput = false;
    return true;
}

bool QSessionManager::allowsErrorInteraction()
{
    sm_blockUserInput = false;
    return true;
}

void QSessionManager::release()
{
    if (sm_smActive)
        sm_blockUserInput = true;
}

void QSessionManager::cancel()
{
    sm_cancel = true;
}
