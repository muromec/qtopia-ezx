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

#include "layoutinfo_p.h"

#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerContainerExtension>
#include <QtDesigner/QDesignerMetaDataBaseInterface>
#include <QtDesigner/QExtensionManager>

#include <QtGui/QHBoxLayout>
#include <QtGui/QSplitter>

#include <QtCore/QMap>
#include <QtCore/qdebug.h>

namespace qdesigner_internal {

LayoutInfo::Type LayoutInfo::layoutType(QDesignerFormEditorInterface *core, QWidget *w, QLayout *&layout)
{
    layout = 0;

    if (QDesignerContainerExtension *container = qt_extension<QDesignerContainerExtension*>(core->extensionManager(), w))
        w = container->widget(container->currentIndex());

    if (qobject_cast<QSplitter*>(w))
        return static_cast<QSplitter*>(w)->orientation() == Qt::Horizontal ? HBox : VBox;

    if (!w || !w->layout())
        return NoLayout;

    QLayout *lay = w->layout();

    if (lay && core->metaDataBase()->item(lay) == 0) {
        lay = qFindChild<QLayout*>(lay);
    }
    layout = lay;

#ifdef QD_DEBUG
    Q_ASSERT (lay == 0 || core->metaDataBase()->item(lay) != 0);
#endif

    return layoutType(core, lay);
}

/*!
  \overload
*/
LayoutInfo::Type LayoutInfo::layoutType(QDesignerFormEditorInterface *core, QLayout *layout)
{
    Q_UNUSED(core)

    if (qobject_cast<QHBoxLayout*>(layout))
        return HBox;
    else if (qobject_cast<QVBoxLayout*>(layout))
        return VBox;
    else if (qobject_cast<QGridLayout*>(layout))
        return Grid;
    return NoLayout;
}

/*!
  \overload
*/
LayoutInfo::Type LayoutInfo::layoutType(QDesignerFormEditorInterface *core, QWidget *w)
{
    return layoutType(core, w->layout());
}

QWidget *LayoutInfo::layoutParent(QDesignerFormEditorInterface *core, QLayout *layout)
{
    Q_UNUSED(core)

    QObject *o = layout;
    while (o) {
        if (QWidget *widget = qobject_cast<QWidget*>(o))
            return widget;

        o = o->parent();
    }
    return 0;
}

void LayoutInfo::deleteLayout(QDesignerFormEditorInterface *core, QWidget *widget)
{
    if (QDesignerContainerExtension *container = qt_extension<QDesignerContainerExtension*>(core->extensionManager(), widget))
        widget = container->widget(container->currentIndex());

    Q_ASSERT(widget != 0);

    QLayout *layout = managedLayout(core, widget);

    if (layout == 0 || core->metaDataBase()->item(layout) != 0) {
        delete layout;
        return;
    }

    qDebug() << "trying to delete an unmanaged layout:" << "widget:" << widget << "layout:" << layout;
}

void LayoutInfo::cells(QLayout *layout, IntervalList *rows, IntervalList *columns)
{
    QMap<Interval, int> rowDict;
    QMap<Interval, int> columnDict;

    int i = 0;
    while (QLayoutItem *item = layout->itemAt(i)) {
        ++i;

        QRect g = item->geometry();
        columnDict.insert(Interval(g.left(), g.right()), 1);
        rowDict.insert(Interval(g.top(), g.bottom()), 1);
    }

    if (columns)
        *columns = columnDict.keys();

    if (rows)
        *rows = rowDict.keys();
}

bool LayoutInfo::isWidgetLaidout(QDesignerFormEditorInterface *core, QWidget *widget)
{
    Q_UNUSED(core);

    QWidget *parent = widget->parentWidget();

    if (qobject_cast<QSplitter*>(parent)) { // ### generalize
        return true;
    }

    if (parent && parent->layout()) {
        if (parent->layout()->indexOf(widget) != -1)
            return true;

        QList<QLayout*> childLayouts = qFindChildren<QLayout*>(parent->layout());
        foreach (QLayout *childLayout, childLayouts) {
            if (childLayout->indexOf(widget) != -1)
                return true;
        }
    }

    return false;
}

QLayout *LayoutInfo::internalLayout(QWidget *widget)
{
    QLayout *widgetLayout = widget->layout();
    if (widgetLayout && widget->inherits("Q3GroupBox")) {
        if (widgetLayout->count()) {
            widgetLayout = widgetLayout->itemAt(0)->layout();
        } else {
            widgetLayout = 0;
        }
    }
    return widgetLayout;
}


QLayout *LayoutInfo::managedLayout(QDesignerFormEditorInterface *core, QWidget *widget)
{
    if (widget == 0)
        return 0;
    
    QLayout *layout = widget->layout();
    if (!layout)
        return 0;

    return managedLayout(core, layout);    
}

QLayout *LayoutInfo::managedLayout(QDesignerFormEditorInterface *core, QLayout *layout)
{
    QDesignerMetaDataBaseInterface *metaDataBase = core->metaDataBase();

    if (!metaDataBase)
        return layout;
    
    const QDesignerMetaDataBaseItemInterface *item = metaDataBase->item(layout);
    if (item == 0) {
        layout = qFindChild<QLayout*>(layout);
        item = metaDataBase->item(layout);
    }
    if (!item)
        return 0;
    return layout;
}


} // namespace qdesigner_internal
