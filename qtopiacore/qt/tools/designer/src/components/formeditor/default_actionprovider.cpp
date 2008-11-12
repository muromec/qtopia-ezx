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

#include "default_actionprovider.h"
#include "invisible_widget_p.h"
#include "qdesigner_toolbar_p.h"

#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QToolBar>
#include <QtGui/QApplication>

#include <QtCore/qdebug.h>

using namespace qdesigner_internal;

QDesignerActionProvider::QDesignerActionProvider(QWidget *widget, QObject *parent)
    : QObject(parent),
      m_widget(widget)
{
    Q_ASSERT(m_widget != 0);

    m_indicator = new InvisibleWidget(m_widget);
    m_indicator->setAutoFillBackground(true);
    m_indicator->setBackgroundRole(QPalette::Window);

    QPalette p;
    p.setColor(m_indicator->backgroundRole(), Qt::red);
    m_indicator->setPalette(p);
    m_indicator->hide();
}

QDesignerActionProvider::~QDesignerActionProvider()
{
}

QRect QDesignerActionProvider::actionGeometry(QAction *action) const
{
    if (QMenuBar *menuBar = qobject_cast<QMenuBar*>(m_widget))
        return menuBar->actionGeometry(action);
    else if (QMenu *menu = qobject_cast<QMenu*>(m_widget))
        return menu->actionGeometry(action);
    else if (QToolBar *toolBar = qobject_cast<QToolBar*>(m_widget))
        return toolBar->actionGeometry(action);

    Q_ASSERT(0);
    return QRect();
}

QAction *QDesignerActionProvider::actionAt(const QPoint &pos) const
{
    if (QMenuBar *menuBar = qobject_cast<QMenuBar*>(m_widget))
        return menuBar->actionAt(pos);
    else if (QMenu *menu = qobject_cast<QMenu*>(m_widget))
        return menu->actionAt(pos);
    else if (QToolBar *toolBar = qobject_cast<QToolBar*>(m_widget))
        return ToolBarEventFilter::actionAt(toolBar, pos);

    Q_ASSERT(0);
    return 0;
}

void QDesignerActionProvider::adjustIndicator(const QPoint &pos)
{
    if (pos == QPoint(-1, -1)) {
        m_indicator->hide();
        return;
    }

    if (QAction *action = actionAt(pos)) {
        QRect g = actionGeometry(action);

        if (orientation() == Qt::Horizontal) {
            if (QApplication::layoutDirection() == Qt::LeftToRight)
                g.setRight(g.left() + 1);
            else
                g.setLeft(g.right() - 1);
        } else {
            g.setHeight(2);
        }

        m_indicator->setGeometry(g);

        QPalette p = m_indicator->palette();
        if (p.color(m_indicator->backgroundRole()) != Qt::red) {
            p.setColor(m_indicator->backgroundRole(), Qt::red);
            m_indicator->setPalette(p);
        }

        m_indicator->show();
        m_indicator->raise();
    } else {
        m_indicator->hide();
    }
}

Qt::Orientation QDesignerActionProvider::orientation() const
{
    if (QToolBar *toolBar = qobject_cast<QToolBar*>(m_widget)) {
        return toolBar->orientation();
    } else if (QMenuBar *menuBar = qobject_cast<QMenuBar*>(m_widget)) {
        Q_UNUSED(menuBar);
        return Qt::Horizontal;
    }

    return Qt::Vertical;
}


// ---- QDesignerActionProviderFactory ----
QDesignerActionProviderFactory::QDesignerActionProviderFactory(QExtensionManager *parent)
    : QExtensionFactory(parent)
{
}

QObject *QDesignerActionProviderFactory::createExtension(QObject *object, const QString &iid, QObject *parent) const
{
    if (iid != Q_TYPEID(QDesignerActionProviderExtension))
        return 0;

    if (qobject_cast<QMenu*>(object)
            || qobject_cast<QMenuBar*>(object)
            || qobject_cast<QToolBar*>(object))
        return new QDesignerActionProvider(qobject_cast<QWidget*>(object), parent);

    return 0;
}
