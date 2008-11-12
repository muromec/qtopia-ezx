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

#ifndef QDECORATIONDEFAULT_QWS_H
#define QDECORATIONDEFAULT_QWS_H

#include <QtGui/qdecoration_qws.h>

QT_BEGIN_HEADER

QT_MODULE(Gui)

#if !defined(QT_NO_QWS_DECORATION_DEFAULT) || defined(QT_PLUGIN)

#define CORNER_GRAB 16
#define BORDER_WIDTH  4
#define BOTTOM_BORDER_WIDTH BORDER_WIDTH

class Q_GUI_EXPORT QDecorationDefault : public QDecoration
{
public:
    QDecorationDefault();
    virtual ~QDecorationDefault();

    virtual QRegion region(const QWidget *widget, const QRect &rect, int decorationRegion = All);
    virtual bool paint(QPainter *painter, const QWidget *widget, int decorationRegion = All,
                       DecorationState state = Normal);

protected:
    virtual int titleBarHeight(const QWidget *widget);

    virtual void paintButton(QPainter *painter, const QWidget *widget, int buttonRegion,
                             DecorationState state, const QPalette &pal);
    virtual QPixmap pixmapFor(const QWidget *widget, int decorationRegion, int &xoff, int &yoff);
    virtual const char **xpmForRegion(int region);

    int menu_width;
    int help_width;
    int close_width;
    int minimize_width;
    int maximize_width;
    int normalize_width;

private:
    static QPixmap *staticHelpPixmap;
    static QPixmap *staticMenuPixmap;
    static QPixmap *staticClosePixmap;
    static QPixmap *staticMinimizePixmap;
    static QPixmap *staticMaximizePixmap;
    static QPixmap *staticNormalizePixmap;

};

#endif // QT_NO_QWS_DECORATION_DEFAULT
QT_END_HEADER

#endif // QDECORATIONDEFAULT_QWS_H
