/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the Qt Designer of the Qt Toolkit.
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

#include "spacer_widget_p.h"
#include "layoutinfo_p.h"

#include <QtDesigner/abstractformwindow.h>

#include <QtGui/QLayout>
#include <QtGui/QPainter>
#include <QtCore/qdebug.h>

using namespace qdesigner_internal;

Spacer::Spacer(QWidget *parent)
    : QWidget(parent),
      orient(Qt::Vertical), interactive(true), sh(20, 40)
{
    setAttribute(Qt::WA_MouseNoMask);
    m_formWindow = QDesignerFormWindowInterface::findFormWindow(this);

    setSizeType(QSizePolicy::Expanding);
}

void Spacer::paintEvent(QPaintEvent *)
{
    // Only draw spacers when we're editting widgets
    if (m_formWindow != 0 && m_formWindow->currentTool() != 0)
        return;

    QPainter p(this);
    p.setPen(Qt::blue);

    if (orient == Qt::Horizontal) {
        const int dist = 3;
        const int amplitude = qMin(3, height() / 3);
        const int base = height() / 2;
        int i = 0;
        p.setPen(Qt::white);
        for (i = 0; i < width() / 3 +2; ++i)
            p.drawLine(i * dist, base - amplitude, i * dist + dist / 2, base + amplitude);
        p.setPen(Qt::blue);
        for (i = 0; i < width() / 3 +2; ++i)
            p.drawLine(i * dist + dist / 2, base + amplitude, i * dist + dist, base - amplitude);
        int y = height()/2;
        p.drawLine(0, y-10, 0, y+10);
        p.drawLine(width() - 1, y-10, width() - 1, y+10);
    } else {
        const int dist = 3;
        const int amplitude = qMin(3, width() / 3);
        const int base = width() / 2;
        int i = 0;
        p.setPen(Qt::white);
        for (i = 0; i < height() / 3 +2; ++i)
            p.drawLine(base - amplitude, i * dist, base + amplitude,i * dist + dist / 2);
        p.setPen(Qt::blue);
        for (i = 0; i < height() / 3 +2; ++i)
            p.drawLine(base + amplitude, i * dist + dist / 2, base - amplitude, i * dist + dist);
        int x = width()/2;
        p.drawLine(x-10, 0, x+10, 0);
        p.drawLine(x-10, height() - 1, x+10, height() - 1);
    }
}

void Spacer::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    updateMask();

    if (!interactive)
        return;

    if (!parentWidget() || (m_formWindow && LayoutInfo::layoutType(m_formWindow->core(), parentWidget()) == LayoutInfo::NoLayout))
        sh = size();
}

void Spacer::updateMask()
{
    QRegion r(rect());
    if (orient == Qt::Horizontal) {
        const int amplitude = qMin(3, height() / 3);
        const int base = height() / 2;
        r = r.subtract(QRect(1, 0, width() - 2, base - amplitude));
        r = r.subtract(QRect(1, base + amplitude, width() - 2, height() - base - amplitude));
    } else {
        const int amplitude = qMin(3, width() / 3);
        const int base = width() / 2;
        r = r.subtract(QRect(0, 1, base - amplitude, height() - 2));
        r = r.subtract(QRect(base + amplitude, 1, width() - base - amplitude, height() - 2));
    }
    setMask(r);
}

void Spacer::setSizeType(QSizePolicy::Policy t)
{
    QSizePolicy sizeP;
    if (orient == Qt::Vertical)
        sizeP = QSizePolicy(QSizePolicy::Minimum, t);
    else
        sizeP = QSizePolicy(t, QSizePolicy::Minimum);
    setSizePolicy(sizeP);
}


QSizePolicy::Policy Spacer::sizeType() const
{
    if (orient == Qt::Vertical)
        return sizePolicy().verticalPolicy();
    return sizePolicy().horizontalPolicy();
}

Qt::Alignment Spacer::alignment() const
{
    if (orient == Qt::Vertical)
        return Qt::AlignHCenter;
    return Qt::AlignVCenter;
}

QSize Spacer::minimumSize() const
{
    QSize s = QSize(20,20);
    if (sizeType() == QSizePolicy::Expanding)
        if (orient == Qt::Vertical)
            s.rheight() = 0;
        else
            s.rwidth() = 0;
    return s;
}

QSize Spacer::sizeHint() const
{
    return sh;
}

void Spacer::setSizeHint(const QSize &s)
{
    sh = s;

    if (!parentWidget() || (m_formWindow && LayoutInfo::layoutType(m_formWindow->core(), parentWidget()) == LayoutInfo::NoLayout))
        resize(sizeHint());

    updateGeometry();
}

Qt::Orientation Spacer::orientation() const
{
    return orient;
}

void Spacer::setOrientation(Qt::Orientation o)
{
    if (orient == o)
        return;

    QSizePolicy::Policy st = sizeType();
    orient = o;
    setSizeType(st);

    if (interactive) {
        sh = QSize(sh.height(), sh.width());
        if (!parentWidget() || (m_formWindow && LayoutInfo::layoutType(m_formWindow->core(), parentWidget()) == LayoutInfo::NoLayout))
            resize(height(), width());
    }

    updateMask();
    update();
    updateGeometry();
}

