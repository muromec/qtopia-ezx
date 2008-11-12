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

#include "default_layoutdecoration.h"
#include "qlayout_widget_p.h"
#include "qdesigner_widget_p.h"

#include <QtDesigner/QDesignerMetaDataBaseItemInterface>
#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormEditorInterface>

#include <QtGui/QGridLayout>
#include <QtCore/qdebug.h>

namespace qdesigner_internal {

// ---- QDesignerLayoutDecoration ----
QDesignerLayoutDecoration::QDesignerLayoutDecoration(QLayoutWidget *widget, QObject *parent)
    : QObject(parent),
      m_layoutSupport(widget->support())
{
    Q_ASSERT(m_layoutSupport);
}

QDesignerLayoutDecoration::QDesignerLayoutDecoration(QDesignerFormWindowInterface *formWindow, QWidget *widget, QObject *parent)
    : QObject(parent),
      m_layoutSupport(new QLayoutSupport(formWindow, widget, this))
{
    Q_ASSERT(m_layoutSupport);
}

QDesignerLayoutDecoration::~QDesignerLayoutDecoration()
{
}

QList<QWidget*> QDesignerLayoutDecoration::widgets(QLayout *layout) const
{
    return m_layoutSupport->widgets(layout);
}

QRect QDesignerLayoutDecoration::itemInfo(int index) const
{
    return m_layoutSupport->itemInfo(index);
}

int QDesignerLayoutDecoration::indexOf(QWidget *widget) const
{
    return m_layoutSupport->indexOf(widget);
}

int QDesignerLayoutDecoration::indexOf(QLayoutItem *item) const
{
    return m_layoutSupport->indexOf(item);
}

QDesignerLayoutDecoration::InsertMode QDesignerLayoutDecoration::currentInsertMode() const
{
    return m_layoutSupport->currentInsertMode();
}

void QDesignerLayoutDecoration::insertWidget(QWidget *widget, const QPair<int, int> &cell)
{
    m_layoutSupport->insertWidget(widget, cell);
}

void QDesignerLayoutDecoration::removeWidget(QWidget *widget)
{
    m_layoutSupport->removeWidget(widget);
}

void QDesignerLayoutDecoration::insertRow(int row)
{
    m_layoutSupport->insertRow(row);
}

void QDesignerLayoutDecoration::insertColumn(int column)
{
    m_layoutSupport->insertColumn(column);
}

void QDesignerLayoutDecoration::simplify()
{
    m_layoutSupport->simplifyLayout();
}

int QDesignerLayoutDecoration::currentIndex() const
{
    return m_layoutSupport->currentIndex();
}

QPair<int, int> QDesignerLayoutDecoration::currentCell() const
{
    return m_layoutSupport->currentCell();
}

int QDesignerLayoutDecoration::findItemAt(const QPoint &pos) const
{
    return m_layoutSupport->findItemAt(pos);
}

int QDesignerLayoutDecoration::findItemAt(int row, int column) const
{
    return m_layoutSupport->findItemAt(row, column);
}

void QDesignerLayoutDecoration::adjustIndicator(const QPoint &pos, int index)
{
    m_layoutSupport->adjustIndicator(pos, index);
}

// ---- QDesignerLayoutDecorationFactory ----
QDesignerLayoutDecorationFactory::QDesignerLayoutDecorationFactory(QExtensionManager *parent)
    : QExtensionFactory(parent)
{
}

QObject *QDesignerLayoutDecorationFactory::createExtension(QObject *object, const QString &iid, QObject *parent) const
{
    if (iid != Q_TYPEID(QDesignerLayoutDecorationExtension))
        return 0;

    if (QLayoutWidget *widget = qobject_cast<QLayoutWidget*>(object)) {
        return new QDesignerLayoutDecoration(widget, parent);
    } else if (QWidget *widget = qobject_cast<QWidget*>(object)) {
        if (QDesignerFormWindowInterface *fw = QDesignerFormWindowInterface::findFormWindow(widget)) {
            QDesignerMetaDataBaseItemInterface *item = fw->core()->metaDataBase()->item(widget->layout());
            return item ? new QDesignerLayoutDecoration(fw, widget, parent) : 0;
        }
    }

    return 0;
}
}
