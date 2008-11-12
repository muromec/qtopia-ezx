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

/*
TRANSLATOR qdesigner_internal::ObjectInspector
*/

#include "objectinspector.h"
#include "formwindow.h"

// sdk
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerTaskMenuExtension>
#include <QtDesigner/QExtensionManager>
#include <QtDesigner/QDesignerFormWindowCursorInterface>
#include <QtDesigner/QDesignerFormWindowManagerInterface>
#include <QtDesigner/QDesignerWidgetDataBaseInterface>
#include <QtDesigner/QDesignerContainerExtension>
#include <QtDesigner/QDesignerMetaDataBaseInterface>
#include <QtDesigner/QDesignerPropertyEditorInterface>
// shared
#include <formwindowbase_p.h>
#include <tree_widget_p.h>
#include <qdesigner_dnditem_p.h>

// Qt
#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QApplication>
#include <QtGui/QHeaderView>
#include <QtGui/QScrollBar>
#include <QtGui/QItemDelegate>
#include <QtGui/QPainter>
#include <QtGui/QVBoxLayout>
#include <QtGui/QItemSelectionModel>
#include <QtGui/qevent.h>

#include <QtCore/QStack>
#include <QtCore/QPair>
#include <QtCore/qdebug.h>


static inline QObject *objectOfItem(QTreeWidgetItem *item) {
    return qvariant_cast<QObject *>(item->data(0, 1000));
}

namespace {
    // Event filter to be installed on the treeview viewport
    // that suppresses a range selection by dragging
    // which does not really work due to the need to maintain
    // a consistent selection
    class ObjectInspectorEventFilter :public QObject {
    public:
        ObjectInspectorEventFilter(QObject *parent) : QObject(parent) {}

        bool eventFilter(QObject * /*watched*/, QEvent *event )  {
            if (event->type() == QEvent::MouseMove) {
                event->ignore();
                return true;
            }
            return false;
        }
    };
}

namespace qdesigner_internal {

ObjectInspector::ObjectInspector(QDesignerFormEditorInterface *core, QWidget *parent)
    : QDesignerObjectInspector(parent),
      m_core(core),
      m_treeWidget(new TreeWidget(this))
{
    m_treeWidget->viewport()->installEventFilter(new ObjectInspectorEventFilter(this));
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);

    vbox->addWidget(m_treeWidget);

    m_treeWidget->setColumnCount(2);
    m_treeWidget->headerItem()->setText(0, tr("Object"));
    m_treeWidget->headerItem()->setText(1, tr("Class"));

    m_treeWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_treeWidget->header()->setResizeMode(1, QHeaderView::Stretch);
    m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeWidget->setTextElideMode (Qt::ElideMiddle);

    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_treeWidget, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(slotPopupContextMenu(QPoint)));

    connect(m_treeWidget->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotSelectionChanged(QItemSelection,QItemSelection)));

    connect(m_treeWidget->header(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(slotHeaderDoubleClicked(int)));
    setAcceptDrops(true);
}

ObjectInspector::~ObjectInspector()
{
}

QDesignerFormEditorInterface *ObjectInspector::core() const
{
    return m_core;
}

void ObjectInspector::slotPopupContextMenu(const QPoint &pos)
{
    if (m_formWindow == 0 || m_formWindow->currentTool() != 0)
        return;

    QTreeWidgetItem *item = m_treeWidget->itemAt(pos);
    if (!item)
        return;

    QObject *object = objectOfItem(item);
    if (!object)
        return;

    QMenu *menu = 0;
    if (object->isWidgetType()) {
        if (qdesigner_internal::FormWindowBase *fwb = qobject_cast<qdesigner_internal::FormWindowBase*>(m_formWindow))
            menu = fwb->initializePopupMenu(qobject_cast<QWidget *>(object));
    } else {
        // Pull extension for non-widget
        QDesignerTaskMenuExtension *taskMenu = qt_extension<QDesignerTaskMenuExtension*>(core()->extensionManager(), object);
        if (taskMenu) {
            QList<QAction*> actions = taskMenu->taskActions();
            if (!actions.isEmpty()) {
                menu = new QMenu(this);
                menu->addActions(actions);
            }
        }
    }

    if (menu) {
        menu->exec(m_treeWidget->viewport()->mapToGlobal(pos));
        delete menu;
    }
}

bool ObjectInspector::sortEntry(const QObject *a, const QObject *b)
{
    return a->objectName() < b->objectName();
}

namespace {
    enum SelectionType {
        NoSelection,
        // A QObject that has a meta database entry
        QObjectSelection,
        // Unmanaged widget, menu bar or the like
        UnmanagedWidgetSelection,
        // A widget managed by the form window
        ManagedWidgetSelection };
}

static inline SelectionType selectionType(const QDesignerFormWindowInterface *fw, QObject *o)
{
    if (!o->isWidgetType())
        return fw->core()->metaDataBase()->item(o) ?  QObjectSelection : NoSelection;
    return fw->isManaged(qobject_cast<QWidget *>(o)) ? ManagedWidgetSelection :  UnmanagedWidgetSelection;
}

ObjectInspector::PreviousSelection ObjectInspector::previousSelection(QDesignerFormWindowInterface *fw,
                                                                      bool formWindowChanged) const
{
    PreviousSelection rc;
    const QDesignerFormWindowCursorInterface* cursor = fw->cursor();
    const QWidget *current = cursor->current();
    const bool currentIsMainContainer = current == fw || current == fw->mainContainer();
    const int selectedWidgetCount = cursor->selectedWidgetCount();
    // If the selection is current main container only, check previously selected
    // non-managed widgets or objects
    if (currentIsMainContainer && selectedWidgetCount <= 1 && !formWindowChanged) {
        typedef  QList<QTreeWidgetItem*> ItemList;
        const ItemList selection = m_treeWidget->selectedItems ();
        if (!selection.empty()) {
            const ItemList::const_iterator cend = selection.constEnd();
            for (ItemList::const_iterator it = selection.constBegin(); it != cend; ++it)
                if (QObject *o = objectOfItem(*it))
                    switch (selectionType(fw, o)) {
                    case QObjectSelection:
                    case UnmanagedWidgetSelection:
                        rc.insert(o);
                        break;
                    default:
                        break;
                    }
        }
    }

    // None - Add widget selection
    if (rc.empty())
        for (int i = 0; i < selectedWidgetCount; i++)
            rc.insert(cursor->selectedWidget(i));
    return rc;
}

void ObjectInspector::setFormWindow(QDesignerFormWindowInterface *fwi)
{
    FormWindowBase *fw = qobject_cast<FormWindowBase *>(fwi);
    const bool formWindowChanged = m_formWindow != fw;
    const bool resizeToColumn = formWindowChanged;
    m_formWindow = fw;

    const int oldWidth = m_treeWidget->columnWidth(0);
    const int xoffset = m_treeWidget->horizontalScrollBar()->value();
    const int yoffset = m_treeWidget->verticalScrollBar()->value();

    if (formWindowChanged)
        m_formFakeDropTarget = 0;

    const bool selectionBlocked = m_treeWidget->selectionModel()->blockSignals(true);
    if (!fw || !fw->mainContainer()) {
        m_treeWidget->clear();
        m_treeWidget->selectionModel()->blockSignals(selectionBlocked);
        return;
    }

    // maintain selection
    const PreviousSelection oldSelection = previousSelection(fw, formWindowChanged);
    m_treeWidget->clear();

    const QDesignerWidgetDataBaseInterface *db = fw->core()->widgetDataBase();

    m_treeWidget->setUpdatesEnabled(false);

    typedef QPair<QTreeWidgetItem*, QObject*> ItemObjectPair;
    QStack<ItemObjectPair> workingList;
    QObject *rootObject = fw->mainContainer();
    workingList.append(qMakePair(new QTreeWidgetItem(m_treeWidget), rootObject));

    // remember the selection and apply later
    typedef QVector<QTreeWidgetItem*> SelectionList;
    SelectionList selectionList;

    const QString qLayoutWidget = QLatin1String("QLayoutWidget");
    const QString designerPrefix = QLatin1String("QDesigner");
    static const QString noName = tr("<noname>");
    static const QString separator =  tr("separator");

    while (!workingList.isEmpty()) {
        QTreeWidgetItem *item = workingList.top().first;
        QObject *object = workingList.top().second;
        workingList.pop();

        const bool isWidget = object->isWidgetType();

        // MainWindow can be current, but not explicitly be selected.
        if (oldSelection.contains(object))
            selectionList.push_back(item);

        QString className = QLatin1String(object->metaObject()->className());
        if (QDesignerWidgetDataBaseItemInterface *widgetItem = db->item(db->indexOfObject(object, true))) {
            className = widgetItem->name();

            if (isWidget && className == qLayoutWidget
                    && static_cast<QWidget*>(object)->layout()) {
                className = QLatin1String(static_cast<QWidget*>(object)->layout()->metaObject()->className());
            }

            item->setIcon(0, widgetItem->icon());
        }

        if (className.startsWith(designerPrefix))
            className.remove(1, designerPrefix.size() - 1);

        item->setText(1, className);
        item->setToolTip(1, className);
        item->setData(0, 1000, qVariantFromValue(object));

        QString objectName = object->objectName();
        if (objectName.isEmpty())
            objectName = noName;

        if (const QAction *act = qobject_cast<const QAction*>(object)) { // separator is reserved
            if (act->isSeparator()) {
                objectName = separator;
            }
            item->setIcon(0, act->icon());
        }

        item->setText(0, objectName);
        item->setToolTip(0, objectName);

        if (QDesignerContainerExtension *c = qt_extension<QDesignerContainerExtension*>(fw->core()->extensionManager(), object)) {
            for (int i=0; i<c->count(); ++i) {
                QObject *page = c->widget(i);
                Q_ASSERT(page != 0);

                QTreeWidgetItem *pageItem = new QTreeWidgetItem(item);
                workingList.append(qMakePair(pageItem, page));
            }
        } else {
            QList<QObject*> children = object->children();
            qSort(children.begin(), children.end(), ObjectInspector::sortEntry);

            foreach (QObject *child, children) {
                QWidget *widget = qobject_cast<QWidget*>(child);
                if (!widget || !fw->isManaged(widget))
                    continue;

                QTreeWidgetItem *childItem = new QTreeWidgetItem(item);
                workingList.append(qMakePair(childItem, child));
            }

            if (QWidget *widget = qobject_cast<QWidget*>(object)) {
                QList<QAction*> actions = widget->actions();
                foreach (QAction *action, actions) {
                    if (!fw->core()->metaDataBase()->item(action))
                        continue;

                    QObject *obj = action;
                    if (action->menu())
                        obj = action->menu();

                    QTreeWidgetItem *childItem = new QTreeWidgetItem(item);
                    workingList.append(qMakePair(childItem, obj));
                }
            }
        }

        m_treeWidget->expandItem(item);
    }

    m_treeWidget->horizontalScrollBar()->setValue(xoffset);
    m_treeWidget->verticalScrollBar()->setValue(yoffset);

    switch (selectionList.size()) {
    case 0:
        break;
    case 1:
        m_treeWidget->scrollToItem(selectionList[0]);
        m_treeWidget->setCurrentItem(selectionList[0]);
        break;
    default:
        foreach (QTreeWidgetItem* item, selectionList) {
            item->setSelected(true);
        }
        m_treeWidget->scrollToItem(selectionList[0]);
        break;
    }

    m_treeWidget->setUpdatesEnabled(true);
    m_treeWidget->update();

    if (resizeToColumn) {
        m_treeWidget->resizeColumnToContents(0);
    } else {
        m_treeWidget->setColumnWidth(0, oldWidth);
    }
    m_treeWidget->selectionModel()->blockSignals(selectionBlocked);
}

void ObjectInspector::showContainersCurrentPage(QWidget *widget)
{
    if (!widget)
        return;

    FormWindow *fw = FormWindow::findFormWindow(widget);
    if (!fw)
        return;

    QWidget *w = widget->parentWidget();
    while (1) {
        if (fw->isMainContainer(w))
            return;

        if (!w)
            return;

        QDesignerContainerExtension *c = qt_extension<QDesignerContainerExtension*>(core()->extensionManager(), w);
        if (c && !c->widget(c->currentIndex())->isAncestorOf(widget)) {
            for (int i = 0; i < c->count(); i++)
                if (c->widget(i)->isAncestorOf(widget)) {
                    c->setCurrentIndex(i);
                    break;
                }
        }
        w = w->parentWidget();
    }
}

void ObjectInspector::slotSelectionChanged(const QItemSelection & /*selected*/, const QItemSelection &/*deselected*/)
{
    if (!m_formWindow)
        return;

    Selection selection;
    getSelection(selection);

    if (!selection.m_cursorSelection.isEmpty())
        showContainersCurrentPage(selection.m_cursorSelection.last());
    if (!selection.m_selectedObjects.isEmpty())
        showContainersCurrentPage(qobject_cast<QWidget *>(selection.m_selectedObjects[0]));

    m_formWindow->clearSelection(false);

    if (!selection.m_cursorSelection.empty()) {

        // This will trigger an update
        foreach (QWidget* widget, selection.m_cursorSelection) {
            m_formWindow->selectWidget(widget);
        }
    } else {
        if (!selection.m_selectedObjects.empty()) {
            // refresh at least the property editor
            core()->propertyEditor()->setObject(selection.m_selectedObjects[0]);
            core()->propertyEditor()->setEnabled(selection.m_selectedObjects.size());
        }
    }
    QMetaObject::invokeMethod(m_formWindow->core()->formWindowManager(), "slotUpdateActions");
}

void ObjectInspector::getSelection(Selection &s) const
{
    s.clear();

    if (!m_formWindow)
        return;

    const QList<QTreeWidgetItem*> items = m_treeWidget->selectedItems();
    if (items.empty())
        return;

    // sort objects
    foreach (QTreeWidgetItem *item, items)
        if (QObject *object = objectOfItem(item))
            switch (selectionType(m_formWindow, object)) {
            case NoSelection:
                break;
            case QObjectSelection:
            case UnmanagedWidgetSelection:
                // It is actually possible to select an action
                // twice if it is in a menu bar and in a tool bar.
                if (!s.m_selectedObjects.contains(object))
                    s.m_selectedObjects.push_back(object);
                break;
            case ManagedWidgetSelection:
                s.m_cursorSelection.push_back(qobject_cast<QWidget *>(object));
                break;
            }
}

void ObjectInspector::findRecursion(QTreeWidgetItem *item, QObject *o,  ItemList &matchList)
{
    if (objectOfItem(item) == o)
        matchList += item;

    if (const int cc = item->childCount())
        for (int i = 0;i < cc; i++)
            findRecursion(item->child(i), o, matchList);
}

ObjectInspector::ItemList ObjectInspector::findItemsOfObject(QObject *o) const
{
    ItemList rc;
    if (const int tlc = m_treeWidget->topLevelItemCount())
        for (int i = 0;i < tlc; i++)
            findRecursion(m_treeWidget->topLevelItem (i), o, rc);
    return rc;
}

bool ObjectInspector::selectObject(QObject *o)
{
    if (!core()->metaDataBase()->item(o))
        return false;

    const ItemList items = findItemsOfObject(o);
    if (items.empty())
        return false;

    // Change in selection?
    const  ItemList currentSelectedItems = m_treeWidget->selectedItems();
    if (!currentSelectedItems.empty() && currentSelectedItems.toSet() == items.toSet()) {
        return true;
    }
    // do select and update
    m_treeWidget->clearSelection();
    const ItemList::const_iterator cend = items.constEnd();
    for (ItemList::const_iterator it = items.constBegin(); it != cend; ++it )  {
        (*it)->setSelected(true);
    }
    m_treeWidget->scrollToItem(items.first());
    return true;
}

void ObjectInspector::clearSelection()
{
    m_treeWidget->clearSelection();
}

void ObjectInspector::slotHeaderDoubleClicked(int column)
{
    m_treeWidget->resizeColumnToContents(column);
}

QWidget *ObjectInspector::managedWidgetAt(const QPoint &global_mouse_pos)
{
    if (!m_formWindow)
        return 0;

    const  QPoint pos = m_treeWidget->viewport()->mapFromGlobal(global_mouse_pos);
    QTreeWidgetItem *item = m_treeWidget->itemAt(pos);
    if (!item)
        return 0;

    QObject *o = objectOfItem(item);
    if (!o->isWidgetType())
        return 0;
    QWidget *rc = qobject_cast<QWidget *>(o);
    if (!m_formWindow->isManaged(rc))
        return 0;
    return rc;
}

void ObjectInspector::mainContainerChanged()
{
    // Invalidate references to objects kept in items
    if (sender() == m_formWindow)
        setFormWindow(0);
}

void  ObjectInspector::dragEnterEvent (QDragEnterEvent * event)
{
    handleDragEnterMoveEvent(event, true);
}

void  ObjectInspector::dragMoveEvent(QDragMoveEvent * event)
{
    handleDragEnterMoveEvent(event, false);
}

void  ObjectInspector::dragLeaveEvent(QDragLeaveEvent * /* event*/)
{
    restoreDropHighlighting();
}

// Return an offset for dropping.
// Position the dropped widget with form grid offset to avoid overlapping unless we
// drop on a layout. Position doesn't matter there
// and it enables us to drop on a squeezed layout widget

static inline QPoint dropPointOffset(const FormWindowBase *fw, const QWidget *dropTarget)
{
    if (!dropTarget || dropTarget->layout())
        return QPoint(0, 0);
    return QPoint(fw->designerGrid().deltaX(), fw->designerGrid().deltaY());
}

void  ObjectInspector::dropEvent (QDropEvent * event)
{
    if (!m_formWindow || !m_formFakeDropTarget) {
        event->ignore();
        return;
    }

    const QDesignerMimeData *mimeData =  qobject_cast<const QDesignerMimeData *>(event->mimeData());
    if (!mimeData) {
        event->ignore();
        return;
    }
    const QPoint fakeGlobalDropFormPos = m_formFakeDropTarget->mapToGlobal(dropPointOffset(m_formWindow , m_formFakeDropTarget));
    mimeData->moveDecoration(fakeGlobalDropFormPos + mimeData->hotSpot());
    if (!m_formWindow->dropWidgets(mimeData->items(), m_formFakeDropTarget, fakeGlobalDropFormPos)) {
        event->ignore();
        return;
    }
    mimeData->acceptEvent(event);
}

void ObjectInspector::handleDragEnterMoveEvent(QDragMoveEvent * event, bool isDragEnter)
{
    if (!m_formWindow) {
        event->ignore();
        return;
    }

    const QDesignerMimeData *mimeData =  qobject_cast<const QDesignerMimeData *>(event->mimeData());
    if (!mimeData) {
        event->ignore();
        return;
    }

    QWidget *dropTarget = 0;
    QPoint fakeDropTargetOffset = QPoint(0, 0);
    if (QWidget *managedWidget = managedWidgetAt(mapToGlobal(event->pos()))) {
        fakeDropTargetOffset = dropPointOffset(m_formWindow, managedWidget);
        // pretend we drag over the managed widget on the form
        const QPoint fakeFormPos = m_formWindow->mapFromGlobal(managedWidget->mapToGlobal(fakeDropTargetOffset));
        const FormWindowBase::WidgetUnderMouseMode wum = mimeData->items().size() == 1 ? FormWindowBase::FindSingleSelectionDropTarget : FormWindowBase::FindMultiSelectionDropTarget;
        dropTarget = m_formWindow->widgetUnderMouse(fakeFormPos, wum);
    }

    if (m_formFakeDropTarget && dropTarget != m_formFakeDropTarget)
        m_formWindow->highlightWidget(m_formFakeDropTarget, fakeDropTargetOffset, FormWindow::Restore);

    m_formFakeDropTarget =  dropTarget;
    if (m_formFakeDropTarget)
        m_formWindow->highlightWidget(m_formFakeDropTarget, fakeDropTargetOffset, FormWindow::Highlight);

    // Do not refuse enter even if the area is not droppable
    if (isDragEnter || m_formFakeDropTarget)
        mimeData->acceptEvent(event);
    else
        event->ignore();
}

void ObjectInspector::restoreDropHighlighting()
{
    if (m_formFakeDropTarget) {
        if (m_formWindow) {
            m_formWindow->highlightWidget(m_formFakeDropTarget, QPoint(5, 5), FormWindow::Restore);
        }
        m_formFakeDropTarget = 0;
    }
}
}
