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

#include "qdockwidget_container.h"

#include <QtCore/qdebug.h>

#include <QtGui/QDockWidget>
#include <QtGui/QMenuBar>
#include <QtGui/QToolBar>
#include <QtGui/QStatusBar>

using namespace qdesigner_internal;

QDockWidgetContainer::QDockWidgetContainer(QDockWidget *widget, QObject *parent)
    : QObject(parent),
      m_dockWidget(widget)
{
    Q_ASSERT(m_dockWidget->widget() == 0);
}

int QDockWidgetContainer::count() const
{
    return m_dockWidget->widget() ? 1 : 0;
}

QWidget *QDockWidgetContainer::widget(int index) const
{
    Q_UNUSED(index);

    return m_dockWidget->widget();
}

int QDockWidgetContainer::currentIndex() const
{
    return m_dockWidget->widget() ? 0 : -1;
}

void QDockWidgetContainer::setCurrentIndex(int index)
{
    Q_UNUSED(index);
}

void QDockWidgetContainer::addWidget(QWidget *widget)
{
    Q_ASSERT(m_dockWidget->widget() == 0);

    widget->setParent(m_dockWidget);
    m_dockWidget->setWidget(widget);
}

void QDockWidgetContainer::insertWidget(int index, QWidget *widget)
{
    Q_UNUSED(index);

    addWidget(widget);
}

void QDockWidgetContainer::remove(int index)
{
    Q_UNUSED(index);
}

QDockWidgetContainerFactory::QDockWidgetContainerFactory(QExtensionManager *parent)
    : QExtensionFactory(parent)
{
}

QObject *QDockWidgetContainerFactory::createExtension(QObject *object, const QString &iid, QObject *parent) const
{
    if (iid != Q_TYPEID(QDesignerContainerExtension))
        return 0;

    if (QDockWidget *w = qobject_cast<QDockWidget*>(object))
        return new QDockWidgetContainer(w, parent);

    return 0;
}

