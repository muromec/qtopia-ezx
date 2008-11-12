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

#include "formwindow_widgetstack.h"
#include <QtDesigner/QDesignerFormWindowToolInterface>

#include <QtGui/QWidget>
#include <QtGui/qevent.h>
#include <QtGui/QAction>
#include <QtGui/QHBoxLayout>

#include <QtCore/qdebug.h>

using namespace qdesigner_internal;

FormWindowWidgetStack::FormWindowWidgetStack(QWidget *parent)
    : QWidget(parent),
      m_current_index(-1)
{
    QHBoxLayout *l = new QHBoxLayout(this);
    l->setMargin(0);
    l->setSpacing(0);
}

FormWindowWidgetStack::~FormWindowWidgetStack()
{
}

int FormWindowWidgetStack::count() const
{
    return m_tools.count();
}

QDesignerFormWindowToolInterface *FormWindowWidgetStack::currentTool() const
{
    return tool(m_current_index);
}

void FormWindowWidgetStack::setCurrentTool(int index)
{
    if (index < 0 || index >= count()) {
        qDebug("FormWindowWidgetStack::setCurrentTool(): invalid index: %d", index);
        return;
    }

    if (index == m_current_index)
        return;

    if (m_current_index != -1)
        m_tools.at(m_current_index)->deactivated();

    if (m_current_index > 0) { // we don't hide the form editor
        QWidget *w = m_tools.at(m_current_index)->editor();
        if (w != 0)
            w->hide();
    }

    m_current_index = index;

    QDesignerFormWindowToolInterface *tool = m_tools.at(m_current_index);
    tool->activated();
    QWidget *w = tool->editor();
    if (w != 0) {
        if (w->rect() != rect())
            w->setGeometry(rect());
        m_tools.at(0)->editor()->raise();
        w->show();
        w->raise();
    }

    emit currentToolChanged(index);
}

void FormWindowWidgetStack::setSenderAsCurrentTool()
{
    QDesignerFormWindowToolInterface *tool = 0;
    QAction *action = qobject_cast<QAction*>(sender());
    if (action == 0) {
        qDebug("FormWindowWidgetStack::setSenderAsCurrentTool(): sender is not a QAction");
        return;
    }

    foreach (QDesignerFormWindowToolInterface *t, m_tools) {
        if (action == t->action()) {
            tool = t;
            break;
        }
    }

    if (tool == 0) {
        qDebug("FormWindowWidgetStack::setSenderAsCurrentTool(): unknown tool");
        return;
    }

    setCurrentTool(tool);
}

int FormWindowWidgetStack::indexOf(QDesignerFormWindowToolInterface *tool) const
{
    return m_tools.indexOf(tool);
}

void FormWindowWidgetStack::setCurrentTool(QDesignerFormWindowToolInterface *tool)
{
    int index = indexOf(tool);
    if (index == -1) {
        qDebug("FormWindowWidgetStack::setCurrentTool(): unknown tool");
        return;
    }

    setCurrentTool(index);
}

void FormWindowWidgetStack::addTool(QDesignerFormWindowToolInterface *tool)
{
    if (QWidget *w = tool->editor()) {
        w->setParent(this);

        if (layout()->isEmpty())
            layout()->addWidget(w);


        if (!m_tools.isEmpty()) // we don't hide the form editor
            w->hide();
    }

    m_tools.append(tool);

    connect(tool->action(), SIGNAL(triggered()), this, SLOT(setSenderAsCurrentTool()));
}

void FormWindowWidgetStack::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    QRect r = QRect(0, 0, event->size().width(), event->size().height());

    // We always resize the widget tool
    QDesignerFormWindowToolInterface *widget_tool = tool(0);
    if (widget_tool != 0 && widget_tool->editor() != 0)
        widget_tool->editor()->setGeometry(r);

    QDesignerFormWindowToolInterface *cur_tool = currentTool();
    if (cur_tool == widget_tool)
        return;

    if (cur_tool != 0 && cur_tool->editor() != 0)
        cur_tool->editor()->setGeometry(r);
}

QDesignerFormWindowToolInterface *FormWindowWidgetStack::tool(int index) const
{
    if (index < 0 || index >= count())
        return 0;

    return m_tools.at(index);
}

int FormWindowWidgetStack::currentIndex() const
{
    return m_current_index;
}

QWidget *FormWindowWidgetStack::defaultEditor() const
{
    if (m_tools.isEmpty())
        return 0;

    return m_tools.at(0)->editor();
}

QSize FormWindowWidgetStack::sizeHint() const
{
    if (QWidget *editor = defaultEditor())
        return editor->sizeHint();

    return QWidget::sizeHint();
}

QSize FormWindowWidgetStack::minimumSizeHint() const
{
    if (QWidget *editor = defaultEditor())
        return editor->minimumSizeHint();

    return QWidget::minimumSizeHint();
}

