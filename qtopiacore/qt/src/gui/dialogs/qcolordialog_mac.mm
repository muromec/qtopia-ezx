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

#include "qcolordialog.h"
#if !defined(QT_NO_COLORDIALOG) && defined(Q_WS_MAC)
#include <qapplication.h>
#include <private/qapplication_p.h>
#include <qdesktopwidget.h>
#include <private/qt_mac_p.h>
#include <qdebug.h>
#import <AppKit/AppKit.h>

#if !CGFLOAT_DEFINED
typedef float CGFloat;  // Should only not be defined on 32-bit platforms
#endif

@class QNSColorPickerResponder;

@interface QNSColorPickerResponder : NSObject {
    NSColor *mColor;
    BOOL mNeedAlpha;
}
- (id)initWithColor:(NSColor*)color needAlpha:(BOOL)needAlpha;
- (void)dealloc;
- (void)windowDidBecomeKey:(NSNotification *)notification;
- (void)windowDidResignKey:(NSNotification *)notification;
- (QRgb)qtColor;
@end

@implementation QNSColorPickerResponder
- (id)initWithColor:(NSColor*)color needAlpha:(BOOL)needAlpha {
    self = [super init];
    mColor = color;
    mNeedAlpha = needAlpha;
    [mColor retain];
    return self;
}

- (void)dealloc {
    [mColor release];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
    NSColorPanel *panel = [notification object];
    [panel setTitle:(NSString*)(CFStringRef)QCFString(QColorDialog::tr("Select color"))];
    [panel setShowsAlpha:mNeedAlpha];
    [panel setColor:mColor];
    NSEnableScreenUpdates();
}
- (void)windowDidResignKey:(NSNotification *)notification {
    NSColorPanel *panel = [notification object];
    [mColor release];
    mColor = [panel color];
    [mColor retain];
}

- (QRgb)qtColor {
    NSString *colorSpace = [mColor colorSpaceName];
    QColor tmpQColor;
    if (colorSpace == NSDeviceCMYKColorSpace) {
        CGFloat cyan, magenta, yellow, black, alpha;
        [mColor getCyan:&cyan magenta:&magenta yellow:&yellow black:&black alpha:&alpha];
        tmpQColor.setCmykF(cyan, magenta, yellow, black, alpha);
    } else {
        NSColor *tmpColor;
        if (colorSpace == NSCalibratedRGBColorSpace || colorSpace == NSDeviceRGBColorSpace) {
            tmpColor = mColor;
        } else {
            tmpColor = [mColor colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
        }
        CGFloat red, green, blue, alpha;
        [tmpColor getRed:&red green:&green blue:&blue alpha:&alpha];
        tmpQColor.setRgbF(red, green, blue, alpha);
    }
    return tmpQColor.rgba();
}
@end

QRgb macGetRgba(QRgb initial, bool needAlpha, bool *ok, QWidget *parent)
{
    QMacCocoaAutoReleasePool pool;
    NSColorPanel *cocoaColorPanel = [NSColorPanel sharedColorPanel];
    NSColor *nsColor = [NSColor colorWithCalibratedRed:qRed(initial) / 255.
                                                       green:qGreen(initial) / 255.
                                                       blue:qBlue(initial) / 255.
                                                       alpha:qAlpha(initial) / 255.];

    QNSColorPickerResponder *responder = [[QNSColorPickerResponder alloc] initWithColor:nsColor
                                                                          needAlpha:needAlpha];
    [cocoaColorPanel setDelegate:responder];
    static const int sw = 420, sh = 300;
    Point p = { -1, -1 };
    if (parent) {
        parent = parent->window();
        p.h = (parent->x() + (parent->width() / 2)) - (sw / 2);
        p.v = (parent->y() + (parent->height() / 2)) - (sh / 2);
        QRect r = QApplication::desktop()->screenGeometry(QApplication::desktop()->screenNumber(parent));
        const int border = 10;
        if(p.h + sw > r.right())
            p.h -= (p.h + sw) - r.right() + border;
        if(p.v + sh > r.bottom())
            p.v -= (p.v + sh) - r.bottom() + border;
        if(p.h < r.left())
            p.h = r.left() + border;
        if(p.v < r.top())
            p.v = r.top() + border;
    }
    RGBColor rgb, rgbout;
    rgb.red = qRed(initial) * 256;
    rgb.blue = qBlue(initial) * 256;
    rgb.green = qGreen(initial) * 256;
    Str255 title;
    bzero(title, sizeof(Str255));
    Point place;
    place.h = p.h == -1 ? 0 : p.h;
    place.v = p.v == -1 ? 0 : p.v;
    Boolean rval = false;
    {
        QMacBlockingFunction block;
        QWidget modal_widg(parent, Qt::Sheet);
        modal_widg.createWinId();
        QApplicationPrivate::enterModal(&modal_widg);
        rval = GetColor(place, title, &rgb, &rgbout);
        QApplicationPrivate::leaveModal(&modal_widg);
    }
    if (rval)
        initial = [responder qtColor];
    if (ok)
        *ok = rval;
    [cocoaColorPanel setDelegate:nil];
    [responder release];
    return initial;
}

QColor macGetColor(const QColor &initial, QWidget *parent)
{
    QRgb rgb = macGetRgba(initial.rgb(), false, 0, parent);

    QColor ret;
    if(ok)
        ret = QColor(rgb);
    return ret;
}
#endif
