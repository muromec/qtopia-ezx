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

#ifndef QX11INFO_X11_H
#define QX11INFO_X11_H

#include <QtCore/qnamespace.h>

QT_BEGIN_HEADER

QT_MODULE(Gui)

struct QX11InfoData;
class QPaintDevice;
class QApplicationPrivate;
class QX11InfoPrivate;

typedef struct _XDisplay Display;

class Q_GUI_EXPORT QX11Info
{
public:
    QX11Info();
    ~QX11Info();
    QX11Info(const QX11Info &other);
    QX11Info &operator=(const QX11Info &other);

    static Display *display();
    static const char *appClass();
    int screen() const;
    int depth() const;
    int cells() const;
    Qt::HANDLE colormap() const;
    bool defaultColormap() const;
    void *visual() const;
    bool defaultVisual() const;

    static int appScreen();
    static int appDepth(int screen = -1);
    static int appCells(int screen = -1);
    static Qt::HANDLE appColormap(int screen = -1);
    static void *appVisual(int screen = -1);
    static Qt::HANDLE appRootWindow(int screen = -1);
    static bool appDefaultColormap(int screen = -1);
    static bool appDefaultVisual(int screen = -1);
    static int appDpiX(int screen = -1);
    static int appDpiY(int screen = -1);
    static void setAppDpiX(int screen, int dpi);
    static void setAppDpiY(int screen, int dpi);
    static unsigned long appTime();
    static unsigned long appUserTime();
    static void setAppTime(unsigned long time);
    static void setAppUserTime(unsigned long time);

protected:
    void copyX11Data(const QPaintDevice *);
    void cloneX11Data(const QPaintDevice *);
    void setX11Data(const QX11InfoData *);
    QX11InfoData* getX11Data(bool def = false) const;

    QX11InfoData *x11data;

    friend class QX11PaintEngine;
    friend class QPixmap;
    friend class QWidget;
    friend class QWidgetPrivate;
    friend class QGLWidget;
    friend void qt_init(QApplicationPrivate *priv, int, Display *display, Qt::HANDLE visual,
                        Qt::HANDLE colormap);
    friend void qt_cleanup();
};

QT_END_HEADER

#endif // QX11INFO_X11_H
