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

#include "qtbrushbutton.h"
#include "qtbrushdialog.h"
#include <QPainter>

#include "qdebug.h"

using namespace qdesigner_internal;

namespace qdesigner_internal {

class QtBrushButtonPrivate
{
    QtBrushButton *q_ptr;
    Q_DECLARE_PUBLIC(QtBrushButton)
public:
    void slotEditBrush();
    QBrush m_brush;
    bool m_backgroundTransparent;
    QDesignerBrushManagerInterface *m_brushManager;
    QtBrushDialog *m_dialog;
};

}

void QtBrushButtonPrivate::slotEditBrush()
{
    QtBrushDialog dlg(q_ptr);
    m_dialog = &dlg;
    dlg.setBrush(m_brush);
    dlg.setBrushManager(m_brushManager);
    q_ptr->connect(&dlg, SIGNAL(textureChooserActivated(QWidget *, const QBrush &)),
            q_ptr, SIGNAL(textureChooserActivated(QWidget *, const QBrush &)));
    if (dlg.exec() != QDialog::Accepted) {
        m_dialog = 0;
        return;
    }
    QBrush brush = dlg.brush();
    if (brush.style() == Qt::TexturePattern) {
        QPixmap pix = brush.texture();
        if (pix.width() == 1 && pix.height() == 1) {
            QImage img = pix.toImage();
            QRgb pixel = img.pixel(0, 0);
            brush = QBrush(QColor::fromRgb(qRed(pixel), qGreen(pixel), qBlue(pixel), qAlpha(pixel)));
        }
    }
    m_brush = brush;
    emit q_ptr->brushChanged(m_brush);
    q_ptr->update();
    m_dialog = 0;
}

///////////////

QtBrushButton::QtBrushButton(QWidget *parent)
    : QToolButton(parent)
{
    d_ptr = new QtBrushButtonPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_backgroundTransparent = true;
    d_ptr->m_brushManager = 0;
    d_ptr->m_dialog = 0;

    connect(this, SIGNAL(clicked()), this, SLOT(slotEditBrush()));
    setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));

    QBrush brush(QColor::fromRgb(162, 197, 17));
    setBrush(brush);
}

QtBrushButton::~QtBrushButton()
{
    delete d_ptr;
}

void QtBrushButton::setBackgroundTransparent(bool transparent)
{
    if (d_ptr->m_backgroundTransparent == transparent)
        return;
    d_ptr->m_backgroundTransparent = transparent;
    update();
}

bool QtBrushButton::backgroundTransparent() const
{
    return d_ptr->m_backgroundTransparent;
}

void QtBrushButton::setBrush(const QBrush &brush)
{
    if (d_ptr->m_brush == brush)
        return;
    d_ptr->m_brush = brush;
    update();
}

QBrush QtBrushButton::brush() const
{
    return d_ptr->m_brush;
}

void QtBrushButton::paintEvent(QPaintEvent *e)
{
    QToolButton::paintEvent(e);
    if (!isEnabled())
        return;

    int corr = 2;
    QRect r = rect().adjusted(corr, corr, -corr, -corr);
    QPixmap pix(r.size());
    int pixSize = 20;
    QPainter p(this);
    QBrush br(d_ptr->m_brush);
    int margin = corr;
    if (!d_ptr->m_backgroundTransparent) {
        QPixmap pm(2 * pixSize, 2 * pixSize);
        QPainter pmp(&pm);
        pmp.fillRect(0, 0, pixSize, pixSize, Qt::lightGray);
        pmp.fillRect(pixSize, pixSize, pixSize, pixSize, Qt::lightGray);
        pmp.fillRect(0, pixSize, pixSize, pixSize, Qt::darkGray);
        pmp.fillRect(pixSize, 0, pixSize, pixSize, Qt::darkGray);

        p.end();
        p.begin(&pix);
        p.setBrushOrigin((r.width() % pixSize + pixSize) / 2, (r.height() % pixSize + pixSize) / 2);

        if (d_ptr->m_brush.style() == Qt::LinearGradientPattern ||
                d_ptr->m_brush.style() == Qt::RadialGradientPattern ||
                d_ptr->m_brush.style() == Qt::ConicalGradientPattern ||
                d_ptr->m_brush.style() == Qt::TexturePattern) {
            p.fillRect(0, 0, r.width(), r.height(), pm);
        } else {
            pmp.fillRect(QRect(0, 0, 2 * pixSize, 2 * pixSize), d_ptr->m_brush);
            br = QBrush(pm);
        }
        margin = 0;
    } else {
        p.translate(corr, corr);
    }
    p.setBrushOrigin(0, 0);
    p.fillRect(0, 0, r.width(), r.height(), br);
    /*
    if (d_ptr->m_brush.style() == Qt::LinearGradientPattern ||
            d_ptr->m_brush.style() == Qt::RadialGradientPattern ||
            d_ptr->m_brush.style() == Qt::ConicalGradientPattern) {
        p.scale(r.width(), r.height());
        p.setBrushOrigin(0, 0);
        p.fillRect(QRectF(0, 0, 1, 1), *(d_ptr->m_brush.gradient()));
    } else if (d_ptr->m_brush.style() == Qt::TexturePattern) {
        p.setBrushOrigin(0, 0);
        p.fillRect(0, 0, r.width(), r.height(), br);
    } else {
        p.fillRect(0, 0, r.width(), r.height(), br);
    }
    */
    if (!d_ptr->m_backgroundTransparent) {
        p.end();
        p.begin(this);
        p.setBrushOrigin(corr, corr);
        p.fillRect(r, pix);
    }
}

void QtBrushButton::setBrushManager(QDesignerBrushManagerInterface *manager)
{
    d_ptr->m_brushManager = manager;
}

void QtBrushButton::setTexture(const QPixmap &texture)
{
    if (d_ptr->m_dialog)
        d_ptr->m_dialog->setBrush(QBrush(texture));
}

#include "moc_qtbrushbutton.cpp"
