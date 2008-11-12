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

#include "tree_widget_p.h"

#include <QtCore/QPair>
#include <QtCore/QStack>

#include <QtGui/QApplication>
#include <QtGui/QHeaderView>
#include <QtGui/QScrollBar>
#include <QtGui/QItemDelegate>
#include <QtGui/QPainter>
#include <QtGui/QStyle>

namespace qdesigner_internal {

class TreeWidgetDelegate: public QItemDelegate
{
public:
    TreeWidgetDelegate(TreeWidget *treeWidget)
        : QItemDelegate(treeWidget) {}


    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const
    {
        QItemDelegate::paint(painter, option, index);

        QPen savedPen = painter->pen();
        QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &option));
        painter->setPen(QPen(color));

        painter->drawLine(option.rect.x(), option.rect.bottom(),
                            option.rect.right(), option.rect.bottom());

        int right = (option.direction == Qt::LeftToRight) ? option.rect.right() : option.rect.left();
        painter->drawLine(right, option.rect.y(), right, option.rect.bottom());

        painter->setPen(savedPen);
    }

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QItemDelegate::sizeHint(option, index) + QSize(4,4);
    }
};


TreeWidget::TreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    setItemDelegate(new TreeWidgetDelegate(this));

    setAlternatingRowColors(true);
}

TreeWidget::~TreeWidget()
{
}

static int level(QAbstractItemModel *model, const QModelIndex &index)
{
    int result = 0;
    QModelIndex parent = model->parent(index);
    while (parent.isValid()) {
        parent = model->parent(parent);
        ++result;
    }
    return result;
}

void TreeWidget::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
    // designer figts the style it uses. :(
    static bool mac_style 
        = QApplication::style()->inherits("QMacStyle");
    static const int windows_deco_size = 9;

    QStyleOptionViewItem option = viewOptions();

    if (model()->hasChildren(index)) {
        option.state |= QStyle::State_Children;

        const bool reverse = isRightToLeft();
        int indent = level(model(), index)*indentation();
        QRect primitive(reverse ? rect.left() : rect.left() + indent - 2,
                        rect.top(), indentation(), rect.height());

        if (!mac_style) {
            primitive.moveLeft(reverse ? primitive.left()
                               : primitive.left() + (primitive.width() - windows_deco_size)/2);
            primitive.moveTop(primitive.top() + (primitive.height() - windows_deco_size)/2);
            primitive.setWidth(windows_deco_size);
            primitive.setHeight(windows_deco_size);
        }

        option.rect = primitive;

        if (isExpanded(index))
            option.state |= QStyle::State_Open;

        style()->drawPrimitive(QStyle::PE_IndicatorBranch, &option, painter, this);
    }
    QPen savedPen = painter->pen();
    QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &option));
    painter->setPen(QPen(color));
    painter->drawLine(rect.x(), rect.bottom(), rect.right(), rect.bottom());
    painter->setPen(savedPen);
}

} // namespace qdesigner_internal
