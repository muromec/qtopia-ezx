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

#include <private/qhiviewwidget_mac_p.h>
#include <qdebug.h>

extern HIViewRef qt_mac_hiview_for(const QWidget *w); //qwidget_mac.cpp
extern HIViewRef qt_mac_hiview_for(WindowPtr w); //qwidget_mac.cpp

QHIViewWidget::QHIViewWidget(WindowRef windowref, bool createSubs, QWidget *parent, Qt::WindowFlags flags)
    : QWidget(parent, flags)
{
    create((WId)qt_mac_hiview_for(windowref), false);
    Rect rect;
    GetWindowBounds(windowref, kWindowContentRgn, &rect);
    setGeometry(QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top));
    if(createSubs)
        createQWidgetsFromHIViews();
}

QHIViewWidget::~QHIViewWidget()
{
}

QHIViewWidget::QHIViewWidget(HIViewRef hiviewref, bool createSubs, QWidget *parent, Qt::WindowFlags flags)
    : QWidget(parent, flags)
{
    create((WId)hiviewref, false);
    setVisible(HIViewIsVisible(hiviewref));
    if(createSubs)
        createQWidgetsFromHIViews();
}

void QHIViewWidget::createQWidgetsFromHIViews()
{
    // Nicely walk through and make HIViewWidget out of all the children
    addViews_recursive(HIViewGetFirstSubview(qt_mac_hiview_for(this)), this);
}

void QHIViewWidget::addViews_recursive(HIViewRef child, QWidget *parent)
{
    if (!child)
        return;
    QWidget *widget = QWidget::find(WId(child));
    if(!widget)
        widget = new QHIViewWidget(child, false, parent);
    addViews_recursive(HIViewGetFirstSubview(child), widget);
    addViews_recursive(HIViewGetNextView(child), parent);
}
