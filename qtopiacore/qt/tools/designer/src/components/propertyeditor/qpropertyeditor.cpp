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

#include "qpropertyeditor.h"
#include "qpropertyeditor_model_p.h"
#include "qpropertyeditor_delegate_p.h"

#include <resourcemimedata_p.h>

#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowManagerInterface>

#include <QtGui/QHeaderView>
#include <QtGui/QApplication>
#include <QtGui/QPainter>
#include <QtGui/QScrollBar>
#include <QtGui/qevent.h>
#include <qdebug.h>

namespace qdesigner_internal {

Q_GLOBAL_STATIC_WITH_ARGS(PropertyCollection, dummy_collection, (QLatin1String("<empty>")))

QPropertyEditor::QPropertyEditor(QWidget *parent)    :
    QTreeView(parent),
    m_model(new QPropertyEditorModel(this)),
    m_itemDelegate(new QPropertyEditorDelegate(this))
{
    connect(m_itemDelegate, SIGNAL(editorOpened()), this, SIGNAL(editorOpened()));
    connect(m_itemDelegate, SIGNAL(editorClosed()), this, SIGNAL(editorClosed()));
    setModel(m_model);
    setItemDelegate(m_itemDelegate);
    setTextElideMode (Qt::ElideMiddle);

    connect(header(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(headerDoubleClicked(int)));

    connect(m_itemDelegate, SIGNAL(resetProperty(const QString &)), m_model, SIGNAL(resetProperty(const QString &)));
    setInitialInput(0);

    setAlternatingRowColors(true);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    connect(this, SIGNAL(activated(QModelIndex)),
            this, SLOT(expand(QModelIndex)));

    connect(m_model, SIGNAL(propertyChanged(IProperty*)),
            this, SIGNAL(propertyChanged(IProperty*)));

    setContextMenuPolicy(Qt::CustomContextMenu);
    setAcceptDrops(true);
}

QPropertyEditor::~QPropertyEditor()
{
}

bool QPropertyEditor::isReadOnly() const
{
    return m_itemDelegate->isReadOnly();
}

void QPropertyEditor::setReadOnly(bool readOnly)
{
    m_itemDelegate->setReadOnly(readOnly);
}

void QPropertyEditor::setInitialInput(IProperty *initialInput)
{
    const int oldColumnWidth  = columnWidth(0);

    QScrollBar *sb = verticalScrollBar();

    const int position = sb->value();
    const bool resizeToColumn = !m_model->initialInput() || m_model->initialInput() == dummy_collection();

    if (!initialInput)
        initialInput = dummy_collection();

    m_model->setInitialInput(initialInput);

    setSelectionMode(QTreeView::SingleSelection);
    setSelectionBehavior(QTreeView::SelectRows);
    setRootIsDecorated(true);

    setEditTriggers(QAbstractItemView::CurrentChanged|QAbstractItemView::SelectedClicked);
    setRootIndex(m_model->indexOf(initialInput));

    if (resizeToColumn) {
        resizeColumnToContents(0);
    } else {
        setColumnWidth (0, oldColumnWidth);
    }
    sb->setValue(position);
}

IProperty *QPropertyEditor::initialInput() const
{
    return m_model->initialInput();
}

void QPropertyEditor::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
    // designer fights the style it uses. :(
    static const bool mac_style = QApplication::style()->inherits("QMacStyle");
    static const int windows_deco_size = 9;

    QStyleOptionViewItem opt = viewOptions();

    IProperty *property = static_cast<const QPropertyEditorModel*>(model())->privateData(index);
    if (index.column() == 0 && property && property->changed()) {
        opt.font.setBold(true);
    }

    if (property && property->isSeparator()) {
        painter->fillRect(rect, opt.palette.dark());
    }

    if (model()->hasChildren(index)) {
        opt.state |= QStyle::State_Children;

        QRect primitive(rect.left(), rect.top(), indentation(), rect.height());

        if (!mac_style) {
            primitive.moveLeft(primitive.left() + (primitive.width() - windows_deco_size)/2);
            primitive.moveTop(primitive.top() + (primitive.height() - windows_deco_size)/2);
            primitive.setWidth(windows_deco_size);
            primitive.setHeight(windows_deco_size);
        }

        opt.rect = primitive;

        if (isExpanded(index))
            opt.state |= QStyle::State_Open;
        style()->drawPrimitive(QStyle::PE_IndicatorBranch, &opt, painter, this);
    }
    const QPen savedPen = painter->pen();
    const QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &opt));
    painter->setPen(QPen(color));
    painter->drawLine(rect.x(), rect.bottom(), rect.right(), rect.bottom());
    painter->setPen(savedPen);
}

void QPropertyEditor::keyPressEvent(QKeyEvent *ev)
{
/*    QApplication::syncX();*/
    QTreeView::keyPressEvent(ev);
}

QStyleOptionViewItem QPropertyEditor::viewOptions() const
{
    QStyleOptionViewItem option = QTreeView::viewOptions();
    option.showDecorationSelected = true;
    return option;
}

void QPropertyEditor::focusInEvent(QFocusEvent *event)
{
    QAbstractScrollArea::focusInEvent(event);
    viewport()->update();
}

void QPropertyEditor::headerDoubleClicked(int column)
{
    resizeColumnToContents(column);
}

void  QPropertyEditor::dragEnterEvent(QDragEnterEvent *event)
{
    if (!isReadOnly() && ResourceMimeData::isResourceMimeData(event->mimeData(), ResourceMimeData::Image))
        event->acceptProposedAction();
    else
        event->ignore();
}

void  QPropertyEditor::dragMoveEvent(QDragMoveEvent *event)
{
    if (!isReadOnly() && ResourceMimeData::isResourceMimeData(event->mimeData(), ResourceMimeData::Image))
        event->acceptProposedAction();
    else
        event->ignore();
}

void QPropertyEditor::dropEvent ( QDropEvent * event )
{
    bool accept = false;
    do {
        if (isReadOnly())
            break;

        const QModelIndex index = indexAt(event->pos());
        if (!index.isValid())
            break;

        ResourceMimeData md;
        if (!md.fromMimeData(event->mimeData()) || md.type() != ResourceMimeData::Image)
            break;

        accept = m_model->resourceImageDropped(index, md);
    } while (false);

    if ( accept) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

}

