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

#include "qtbrushwidget.h"
#include <QPainter>

using namespace qdesigner_internal;

namespace qdesigner_internal {

class QtBrushWidgetPrivate
{
    QtBrushWidget *q_ptr;
    Q_DECLARE_PUBLIC(QtBrushWidget)
public:
    QBrush m_brush;
    bool m_backgroundTransparent;
    int m_size;
};

}

///////////////

QtBrushWidget::QtBrushWidget(QWidget *parent)
    : QWidget(parent)
{
    d_ptr = new QtBrushWidgetPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_size = 3;
    d_ptr->m_backgroundTransparent = true;

    setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
}

QtBrushWidget::~QtBrushWidget()
{
    delete d_ptr;
}

QSize QtBrushWidget::sizeHint() const
{
    return QSize(256, 256);
}

QSize QtBrushWidget::minimumSizeHint() const
{
    return QSize(20, 20);
}

int QtBrushWidget::heightForWidth(int w) const
{
    return w;
}

void QtBrushWidget::setBackgroundTransparent(bool transparent)
{
    if (d_ptr->m_backgroundTransparent == transparent)
        return;
    d_ptr->m_backgroundTransparent = transparent;
    update();
}

bool QtBrushWidget::backgroundTransparent() const
{
    return d_ptr->m_backgroundTransparent;
}

void QtBrushWidget::setBrush(const QBrush &brush)
{
    if (d_ptr->m_brush == brush)
        return;
    d_ptr->m_brush = brush;
    update();
}

QBrush QtBrushWidget::brush() const
{
    return d_ptr->m_brush;
}

void QtBrushWidget::setBackgroundSize(int size)
{
    int s = size;
    if (s < 0)
        s = 0;
    else if (s > 8)
        s = 8;
    d_ptr->m_size = s;
    update();
}

void QtBrushWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)

    if (!isEnabled())
        return;

    QPainter p(this);
    QRect r = rect();
    QBrush br = d_ptr->m_brush;
    if (!d_ptr->m_backgroundTransparent) {
        int pixSize = 5 * (1 << d_ptr->m_size);
        QPixmap pm(2 * pixSize, 2 * pixSize);
        QPainter pmp(&pm);
        pmp.fillRect(0, 0, pixSize, pixSize, Qt::lightGray);
        pmp.fillRect(pixSize, pixSize, pixSize, pixSize, Qt::lightGray);
        pmp.fillRect(0, pixSize, pixSize, pixSize, Qt::darkGray);
        pmp.fillRect(pixSize, 0, pixSize, pixSize, Qt::darkGray);
        p.setBrushOrigin((r.width() % pixSize + pixSize) / 2, (r.height() % pixSize + pixSize) / 2);

        if (d_ptr->m_brush.style() == Qt::LinearGradientPattern ||
                d_ptr->m_brush.style() == Qt::RadialGradientPattern ||
                d_ptr->m_brush.style() == Qt::ConicalGradientPattern ||
                d_ptr->m_brush.style() == Qt::TexturePattern) {
            p.fillRect(r, pm);
        } else {
            pmp.fillRect(QRect(0, 0, 2 * pixSize, 2 * pixSize), d_ptr->m_brush);
            br = QBrush(pm);
        }
    }

    p.setBrushOrigin(0, 0);
    p.fillRect(r, br);
    /*
    if (d_ptr->m_brush.style() == Qt::LinearGradientPattern ||
            d_ptr->m_brush.style() == Qt::RadialGradientPattern ||
            d_ptr->m_brush.style() == Qt::ConicalGradientPattern) {
        p.setBrushOrigin(0, 0);
        p.scale(r.width(), r.height());
        p.fillRect(QRect(0, 0, 1, 1), br);
    } else if (d_ptr->m_brush.style() == Qt::TexturePattern) {
        p.setBrushOrigin(0, 0);
        p.fillRect(r, br);
    } else {
        p.fillRect(r, br);
    }
    */
}

