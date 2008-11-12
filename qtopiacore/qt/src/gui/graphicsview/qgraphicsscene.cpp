/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
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

static const int QGRAPHICSSCENE_INDEXTIMER_TIMEOUT = 2000;

/*!
    \class QGraphicsScene
    \brief The QGraphicsScene class provides a surface for managing a large
    number of 2D graphical items.
    \since 4.2
    \ingroup multimedia
    \mainclass

    The class serves as a container for QGraphicsItems. It is used together
    with QGraphicsView for visualizing graphical items, such as lines,
    rectangles, text, or even custom items, on a 2D surface. QGraphicsScene is
    part of \l{The Graphics View Framework}.

    QGraphicsScene also provides functionality that lets you efficiently
    determine both the location of items, and for determining what items are
    visible within an arbitrary area on the scene. With the QGraphicsView
    widget, you can either visualize the whole scene, or zoom in and view only
    parts of the scene.

    Example:

    \code
        QGraphicsScene scene;
        scene.addText("Hello, world!");

        QGraphicsView view(&scene);
        view.show();
    \endcode

    Note that QGraphicsScene has no visual appearance of its own; it only
    manages the items. You need to create a QGraphicsView widget to visualize
    the scene.

    To add items to a scene, you start off by constructing a QGraphicsScene
    object. Then, you have two options: either add your existing QGraphicsItem
    objects by calling addItem(), or you can call one of the convenience
    functions addEllipse(), addLine(), addPath(), addPixmap(), addPolygon(),
    addRect(), or addText(), which all return a pointer to the newly added item.
    The dimensions of the items added with these functions are relative to the
    item's coordinate system, and the items position is initialized to (0,
    0) in the scene.

    You can then visualize the scene using QGraphicsView. When the scene
    changes, (e.g., when an item moves or is transformed) QGraphicsScene emits
    the changed() signal. To remove an item, call removeItem().

    QGraphicsScene uses an indexing algorithm to manage the location of items
    efficiently. By default, a BSP (Binary Space Partitioning) tree is used; an
    algorithm suitable for large scenes where most items remain static (i.e.,
    do not move around). You can choose to disable this index by calling
    setItemIndexMethod(). For more information about the available indexing
    algorithms, see the itemIndexMethod property.

    The scene's bounding rect is set by calling setSceneRect(). Items can be
    placed at any position on the scene, and the size of the scene is by
    default unlimited. The scene rect is used only for internal bookkeeping,
    maintaining the scene's item index. If the scene rect is unset,
    QGraphicsScene will use the bounding area of all items, as returned by
    itemsBoundingRect(), as the scene rect. However, itemsBoundingRect() is a
    relatively time consuming function, as it operates by collecting
    positional information for every item on the scene. Because of this, you
    should always set the scene rect when operating on large scenes.

    One of QGraphicsScene's greatest strengths is its ability to efficiently
    determine the location of items. Even with millions of items on the scene,
    the items() functions can determine the location of an item within few
    milliseconds. There are several overloads to items(): one that finds items
    at a certain position, one that finds items inside or intersecting with a
    polygon or a rectangle, and more. The list of returned items is sorted by
    stacking order, with the topmost item being the first item in the list.
    For convenience, there is also an itemAt() function that returns the
    topmost item at a given position.

    QGraphicsScene maintains selection information for the scene. To select
    items, call setSelectionArea(), and to clear the current selection, call
    clearSelection(). Call selectedItems() to get the list of all selected
    items.

    \section1 Event Handling and Propagation

    Another responsibility that QGraphicsScene has, is to propagate events
    from QGraphicsView. To send an event to a scene, you construct an event
    that inherits QEvent, and then send it using, for example,
    QApplication::sendEvent(). event() is responsible for dispatching
    the event to the individual items. Some common events are handled by
    convenience event handlers. For example, key press events are handled by
    keyPressEvent(), and mouse press events are handled by mousePressEvent().

    Key events are delivered to the \e {focus item}. To set the focus item,
    you can either call setFocusItem(), passing an item that accepts focus, or
    the item itself can call QGraphicsItem::setFocus().  Call focusItem() to
    get the current focus item. For compatibility with widgets, the scene also
    maintains its own focus information. By default, the scene does not have
    focus, and all key events are discarded. If setFocus() is called, or if an
    item on the scene gains focus, the scene automatically gains focus. If the
    scene has focus, hasFocus() will return true, and key events will be
    forwarded to the focus item, if any. If the scene loses focus, (i.e.,
    someone calls clearFocus(),) while an item has focus, the scene will
    maintain its item focus information, and once the scene regains focus, it
    will make sure the last focus item regains focus.

    For mouse-over effects, QGraphicsScene dispatches \e {hover
    events}. If an item accepts hover events (see
    QGraphicsItem::acceptsHoverEvents()), it will receive a \l
    {QEvent::}{GraphicsSceneHoverEnter} event when the mouse enters
    its area. As the mouse continues moving inside the item's area,
    QGraphicsScene will send it \l {QEvent::}{GraphicsSceneHoverMove}
    events. When the mouse leaves the item's area, the item will
    receive a \l {QEvent::}{GraphicsSceneHoverLeave} event.

    All mouse events are delivered to the current \e {mouse grabber}
    item. An item becomes the scene's mouse grabber if it accepts
    mouse events (see QGraphicsItem::acceptedMouseButtons()) and it
    receives a mouse press. It stays the mouse grabber until it
    receives a mouse release when no other mouse buttons are
    pressed. You can call mouseGrabberItem() to determine what item is
    currently grabbing the mouse.

    \sa QGraphicsItem, QGraphicsView
*/

/*!
    \enum QGraphicsScene::SceneLayer
    \since 4.3

    This enum describes the rendering layers in a QGraphicsScene. When
    QGraphicsScene draws the scene contents, it renders each of these layers
    separately, in order.

    Each layer represents a flag that can be OR'ed together when calling
    functions such as invalidate() or QGraphicsView::invalidateScene().

    \value ItemLayer The item layer. QGraphicsScene renders all items are in
    this layer by calling the virtual function drawItems(). The item layer is
    drawn after the background layer, but before the foreground layer.

    \value BackgroundLayer The background layer. QGraphicsScene renders the
    scene's background in this layer by calling the virtual function
    drawBackground(). The background layer is drawn first of all layers.

    \value ForegroundLayer The foreground layer. QGraphicsScene renders the
    scene's foreground in this layer by calling the virtual function
    drawForeground().  The foreground layer is drawn last of all layers.

    \value AllLayers All layers; this value represents a combination of all
    three layers.

    \sa invalidate(), QGraphicsView::invalidateScene()
*/

/*!
    \enum QGraphicsScene::ItemIndexMethod

    This enum describes the indexing algorithms QGraphicsScene provides for
    managing positional information about items on the scene.

    \value BspTreeIndex A Binary Space Partitioning tree is applied. All
    QGraphicsScene's item location algorithms are of an order close to
    logarithmic complexity, by making use of binary search. Adding, moving and
    removing items is logarithmic. This approach is best for static scenes
    (i.e., scenes where most items do not move).

    \value NoIndex No index is applied. Item location is of linear complexity,
    as all items on the scene are searched. Adding, moving and removing items,
    however, is done in constant time. This approach is ideal for dynamic
    scenes, where many items are added, moved or removed continuously.

    \sa setItemIndexMethod(), bspTreeDepth
*/

#include "qgraphicsscene.h"

#ifndef QT_NO_GRAPHICSVIEW

#include "qgraphicsitem.h"
#include "qgraphicsitem_p.h"
#include "qgraphicsscene_p.h"
#include "qgraphicssceneevent.h"
#include "qgraphicsview.h"

#include <private/qobject_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qlist.h>
#include <QtCore/qrect.h>
#include <QtCore/qset.h>
#include <QtCore/qtimer.h>
#include <QtGui/qevent.h>
#include <QtGui/qtransform.h>
#include <QtGui/qmatrix.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpaintengine.h>
#include <QtGui/qpolygon.h>
#include <QtGui/qstyleoption.h>
#include <QtGui/qtooltip.h>
#include <math.h>
#include <qdebug.h>

// QRectF::intersects() returns false always if either the source or target
// rectangle's width or height are 0. This works around that problem.
static QRectF _q_adjustedRect(const QRectF &rect)
{
    static const qreal p = 0.00001;
    QRectF r = rect;
    if (!r.width())
        r.adjust(-p, 0, p, 0);
    if (!r.height())
        r.adjust(0, -p, 0, p);
    return r;
}

/*!
    \internal
*/
QGraphicsScenePrivate::QGraphicsScenePrivate()
    : indexMethod(QGraphicsScene::BspTreeIndex),
      bspTreeDepth(0),
      lastItemCount(0),
      hasSceneRect(false),
      updateAll(false),
      calledEmitUpdated(false),
      selectionChanging(0),
      regenerateIndex(true),
      purgePending(false),
      indexTimerId(0),
      restartIndexTimer(false),
      hasFocus(false),
      focusItem(0),
      lastFocusItem(0),
      mouseGrabberItem(0),
      lastMouseGrabberItem(0),
      dragDropItem(0),
      lastDropAction(Qt::IgnoreAction)
{
}

/*!
    \internal
*/
QList<QGraphicsItem *> QGraphicsScenePrivate::estimateItemsInRect(const QRectF &rect) const
{
    Q_Q(const QGraphicsScene);
    const_cast<QGraphicsScenePrivate *>(this)->purgeRemovedItems();

    if (indexMethod == QGraphicsScene::BspTreeIndex) {
        // ### Only do this once in a while.
        QGraphicsScenePrivate *that = const_cast<QGraphicsScenePrivate *>(this);

        // Get items from BSP tree
        QList<QGraphicsItem *> items = that->bspTree.items(rect);

        // Fill in with any unindexed items
        for (int i = 0; i < unindexedItems.size(); ++i) {
            if (QGraphicsItem *item = unindexedItems.at(i)) {
                QRectF boundingRect = _q_adjustedRect(item->sceneBoundingRect());
                if (!item->d_ptr->itemDiscovered && item->isVisible()
                    && (boundingRect.intersects(rect) || boundingRect.contains(rect))) {
                    item->d_ptr->itemDiscovered = 1;
                    items << item;
                }
            }
        }

        // Reset the discovered state of all discovered items
        for (int i = 0; i < items.size(); ++i)
            items.at(i)->d_func()->itemDiscovered = 0;
        return items;
    }

    QList<QGraphicsItem *> itemsInRect;
    foreach (QGraphicsItem *item, q->items()) {
        QRectF boundingRect = _q_adjustedRect(item->sceneBoundingRect());
        if (item->isVisible() && (boundingRect.intersects(rect) || boundingRect.contains(rect)))
            itemsInRect << item;
    }
    return itemsInRect;
}

/*!
    \internal
*/
void QGraphicsScenePrivate::addToIndex(QGraphicsItem *item)
{
    if (indexMethod == QGraphicsScene::BspTreeIndex) {
        if (item->d_func()->index != -1) {
            bspTree.insertItem(item, item->sceneBoundingRect());
            foreach (QGraphicsItem *child, item->children())
                child->addToIndex();
        } else {
            // The BSP tree is regenerated if the number of items grows to a
            // certain threshold, or if the bounding rect of the graph doubles in
            // size.
            startIndexTimer();
        }
    }
}

/*!
    \internal
*/
void QGraphicsScenePrivate::removeFromIndex(QGraphicsItem *item)
{
    if (indexMethod == QGraphicsScene::BspTreeIndex) {
        int index = item->d_func()->index;
        if (index != -1) {
            bspTree.removeItem(item, item->sceneBoundingRect());
            freeItemIndexes << index;
            indexedItems[index] = 0;
            item->d_func()->index = -1;
            unindexedItems << item;

            foreach (QGraphicsItem *child, item->children())
                child->removeFromIndex();
        }

        startIndexTimer();
    }
}

/*!
    \internal
*/
void QGraphicsScenePrivate::resetIndex()
{
    purgeRemovedItems();
    if (indexMethod == QGraphicsScene::BspTreeIndex) {
        for (int i = 0; i < indexedItems.size(); ++i) {
            if (QGraphicsItem *item = indexedItems.at(i)) {
                item->d_ptr->index = -1;
                unindexedItems << item;
            }
        }
        indexedItems.clear();
        freeItemIndexes.clear();
        regenerateIndex = true;
        startIndexTimer();
    }
}

static inline int intmaxlog(int n)
{
    return  (n > 0 ? qMax(int(::ceil(::log(double(n)) / ::log(double(2)))), 5) : 0);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::_q_generateBspTree()
{
    if (!indexTimerId)
        return;

    Q_Q(QGraphicsScene);
    q->killTimer(indexTimerId);
    indexTimerId = 0;

    purgeRemovedItems();

    // Add unindexedItems to indexedItems
    QRectF unindexedItemsBoundingRect;
    for (int i = 0; i < unindexedItems.size(); ++i) {
        if (QGraphicsItem *item = unindexedItems.at(i)) {
            unindexedItemsBoundingRect |= item->sceneBoundingRect();
            if (!freeItemIndexes.isEmpty()) {
                int freeIndex = freeItemIndexes.takeFirst();
                item->d_func()->index = freeIndex;
                indexedItems[freeIndex] = item;
            } else {
                item->d_func()->index = indexedItems.size();
                indexedItems << item;
            }
        }
    }

    QRectF oldGrowingItemsBoundingRect = growingItemsBoundingRect;
    growingItemsBoundingRect |= unindexedItemsBoundingRect;

    // Determine whether we should regenerate the BSP tree.
    int depth = bspTreeDepth;
    if (depth == 0) {
        int oldDepth = intmaxlog(lastItemCount);
        depth = intmaxlog(indexedItems.size());
        static const int slack = 100;
        if (bspTree.leafCount() == 0 || oldDepth != depth && qAbs(lastItemCount - indexedItems.size()) > slack) {
            // ### Crude algorithm.
            regenerateIndex = true;
        }
    }

    // Regenerate the tree.
    if (regenerateIndex) {
        regenerateIndex = false;
        bspTree.initialize(q->sceneRect(), depth);
        unindexedItems = indexedItems;
        lastItemCount = indexedItems.size();
        q->update();

        // Take this opportunity to reset our largest-item counter for
        // untransformable items. When the items are inserted into the BSP
        // tree, we'll get an accurate calculation.
        largestUntransformableItem = QRectF();
    }

    // Insert all unindexed items into the tree.
    for (int i = 0; i < unindexedItems.size(); ++i) {
        if (QGraphicsItem *item = unindexedItems.at(i)) {
            QRectF rect = item->sceneBoundingRect();
            bspTree.insertItem(item, rect);

            // If the item ignores view transformations, update our
            // largest-item-counter to ensure that the view can accurately
            // discover untransformable items when drawing.
            if (item->d_ptr->itemIsUntransformable()) {
                QGraphicsItem *topmostUntransformable = item;
                while (topmostUntransformable && (topmostUntransformable->d_ptr->ancestorFlags
                                                  & QGraphicsItemPrivate::AncestorIgnoresTransformations)) {
                    topmostUntransformable = topmostUntransformable->parentItem();
                }
                // ### Verify that this is the correct largest untransformable rectangle.
                largestUntransformableItem |= item->mapToItem(topmostUntransformable, item->boundingRect()).boundingRect();
            }
        }
    }
    unindexedItems.clear();

    // Notify scene rect changes.
    if (!hasSceneRect && growingItemsBoundingRect != oldGrowingItemsBoundingRect)
        emit q->sceneRectChanged(growingItemsBoundingRect);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::_q_emitUpdated()
{
    Q_Q(QGraphicsScene);
    calledEmitUpdated = false;
    QList<QRectF> oldUpdatedRects;
    oldUpdatedRects = updateAll ? (QList<QRectF>() << (sceneRect | growingItemsBoundingRect)) : updatedRects;
    updateAll = false;
    updatedRects.clear();
    emit q->changed(oldUpdatedRects);
}

/*!
    \internal

    Updates all items in the pending update list. At this point, the list is
    unlikely to contain partially constructed items.
*/
void QGraphicsScenePrivate::_q_updateLater()
{
    foreach (QGraphicsItem *item, pendingUpdateItems)
        item->update();
    pendingUpdateItems.clear();
}

/*!
    \internal

    Schedules an item for removal. This function leaves some stale indexes
    around in the BSP tree; these will be cleaned up the next time someone
    triggers purgeRemovedItems().

    Note: This function is called from QGraphicsItem's destructor. \a item is
    being destroyed, so we cannot call any pure virtual functions on it (such
    as boundingRect()). Also, it is unnecessary to update the item's own state
    in any way.

    ### Refactoring: This function shares much functionality with removeItem()
*/
void QGraphicsScenePrivate::_q_removeItemLater(QGraphicsItem *item)
{
    Q_Q(QGraphicsScene);

    if (QGraphicsItem *parent = item->d_func()->parent) {
        QVariant variant;
        qVariantSetValue<QGraphicsItem *>(variant, item);
        parent->itemChange(QGraphicsItem::ItemChildRemovedChange, item);
        parent->d_func()->children.removeAll(item);
    }

    int index = item->d_func()->index;
    if (index != -1) {
        // Important: The index is useless until purgeRemovedItems() is
        // called.
        indexedItems[index] = (QGraphicsItem *)0;
        if (!purgePending) {
            purgePending = true;
            q->update();
        }
        removedItems << item;
    } else {
        // Recently added items are purged immediately. unindexedItems() never
        // contains stale items.
        unindexedItems.removeAll(item);
        q->update();
    }

    // Reset the mouse grabber and focus item data.
    if (item == mouseGrabberItem) {
        lastMouseGrabberItem = mouseGrabberItem;
        mouseGrabberItem = 0;
    }
    if (item == focusItem)
        focusItem = 0;
    if (item == lastFocusItem)
        lastFocusItem = 0;

    // Disable selectionChanged() for individual items
    ++selectionChanging;
    int oldSelectedItemsSize = selectedItems.size();

    // Update selected & hovered item bookkeeping
    selectedItems.remove(item);
    hoverItems.removeAll(item);
    pendingUpdateItems.removeAll(item);
    cachedItemsUnderMouse.removeAll(item);

    // Remove all children recursively.
    foreach (QGraphicsItem *child, item->children())
        _q_removeItemLater(child);

    // Reenable selectionChanged() for individual items
    --selectionChanging;
    if (!selectionChanging && selectedItems.size() != oldSelectedItemsSize)
        emit q->selectionChanged();
}

/*!
    \internal

    Removes stale pointers from all data structures.
*/
void QGraphicsScenePrivate::purgeRemovedItems()
{
    Q_Q(QGraphicsScene);

    if (!purgePending && removedItems.isEmpty())
        return;

    // Remove stale items from the BSP tree.
    if (indexMethod != QGraphicsScene::NoIndex)
        bspTree.removeItems(removedItems);

    // Purge this list.
    removedItems.clear();
    freeItemIndexes.clear();
    for (int i = 0; i < indexedItems.size(); ++i) {
        if (!indexedItems.at(i))
            freeItemIndexes << i;
    }
    purgePending = false;

    // No locality info for the items; update the whole scene.
    q->update();
}

/*!
    \internal

    Starts or restarts the timer used for reindexing unindexed items.
*/
void QGraphicsScenePrivate::startIndexTimer()
{
    Q_Q(QGraphicsScene);
    if (indexTimerId) {
        restartIndexTimer = true;
    } else {
        indexTimerId = q->startTimer(QGRAPHICSSCENE_INDEXTIMER_TIMEOUT);
    }
}

/*!
    \internal

    Returns a list of possible mouse grabbers for \a event. The first item in
    the list is the topmost candidate, and the last item is the bottommost
    candidate.
*/
QList<QGraphicsItem *> QGraphicsScenePrivate::possibleMouseGrabbersForEvent(const QList<QGraphicsItem *> &items,
                                                                            QGraphicsSceneMouseEvent *event)
{
    QList<QGraphicsItem *> possibleMouseGrabbers;
    foreach (QGraphicsItem *item, items) {
        if (item->acceptedMouseButtons() & event->button()) {
            if (!item->isEnabled()) {
                // Disabled mouse-accepting items discard mouse events.
                break;
            }
            possibleMouseGrabbers << item;
        }
    }
    return possibleMouseGrabbers;
}

/*!
    Returns all items for the screen position in \a event.
*/
QList<QGraphicsItem *> QGraphicsScenePrivate::itemsAtPosition(const QPoint &screenPos,
                                                              const QPointF &scenePos,
                                                              QWidget *widget) const
{
    Q_Q(const QGraphicsScene);
    QGraphicsView *view = widget ? qobject_cast<QGraphicsView *>(widget->parentWidget()) : 0;
    QList<QGraphicsItem *> items;
    if (view)
        items = view->items(view->viewport()->mapFromGlobal(screenPos));
    else
        items = q->items(scenePos);
    return items;
}

/*!
    \internal
*/
void QGraphicsScenePrivate::storeMouseButtonsForMouseGrabber(QGraphicsSceneMouseEvent *event)
{
    for (int i = 0x1; i <= 0x10; i <<= 1) {
        if (event->buttons() & i) {
            mouseGrabberButtonDownPos.insert(Qt::MouseButton(i),
                                             mouseGrabberItem->d_ptr->genericMapFromScene(event->scenePos(),
                                                                                          event->widget()));
            mouseGrabberButtonDownScenePos.insert(Qt::MouseButton(i), event->scenePos());
            mouseGrabberButtonDownScreenPos.insert(Qt::MouseButton(i), event->screenPos());
        }
    }
}

/*!
    \internal
*/
void QGraphicsScenePrivate::installSceneEventFilter(QGraphicsItem *watched, QGraphicsItem *filter)
{
    sceneEventFilters.insert(watched, filter);
}

/*!
    \internal
*/
bool QGraphicsScenePrivate::painterStateProtection(const QPainter *painter) const
{
    // Detect if painter state protection is disabled.
    QPaintDevice *device = painter->paintEngine()->paintDevice();
    for (int i = 0; i < views.size(); ++i) {
        QWidget *viewport = views.at(i)->viewport();
        if ((QPaintDevice *)viewport == device)
            return !(views.at(i)->optimizationFlags() & QGraphicsView::DontSavePainterState);
    }
    return true;
}

/*!
    \internal
*/
void QGraphicsScenePrivate::removeSceneEventFilter(QGraphicsItem *watched, QGraphicsItem *filter)
{
    if (!sceneEventFilters.contains(watched))
        return;

    QMultiMap<QGraphicsItem *, QGraphicsItem *>::Iterator it = sceneEventFilters.lowerBound(watched);
    QMultiMap<QGraphicsItem *, QGraphicsItem *>::Iterator end = sceneEventFilters.upperBound(watched);
    do {
        if (it.value() == filter)
            it = sceneEventFilters.erase(it);
        else
            ++it;
    } while (it != end);
}

/*!
    \internal
*/
bool QGraphicsScenePrivate::filterEvent(QGraphicsItem *item, QEvent *event)
{
    if (item && !sceneEventFilters.contains(item))
        return false;

    QMultiMap<QGraphicsItem *, QGraphicsItem *>::Iterator it = sceneEventFilters.lowerBound(item);
    QMultiMap<QGraphicsItem *, QGraphicsItem *>::Iterator end = sceneEventFilters.upperBound(item);
    while (it != end) {
        // ### The filterer and filteree might both be deleted.
        if (it.value()->sceneEventFilter(it.key(), event))
            return true;
        ++it;
    }
    return false;
}

/*!
    \internal
*/
bool QGraphicsScenePrivate::sendEvent(QGraphicsItem *item, QEvent *event)
{
    if (filterEvent(item, event))
        return false;
    return item ? item->sceneEvent(event) : false;
}

/*!
    \internal
*/
void QGraphicsScenePrivate::cloneDragDropEvent(QGraphicsSceneDragDropEvent *dest,
                                               QGraphicsSceneDragDropEvent *source)
{
    dest->setWidget(source->widget());
    dest->setPos(source->pos());
    dest->setScenePos(source->scenePos());
    dest->setScreenPos(source->screenPos());
    dest->setButtons(source->buttons());
    dest->setModifiers(source->modifiers());
    dest->setPossibleActions(source->possibleActions());
    dest->setProposedAction(source->proposedAction());
    dest->setDropAction(source->dropAction());
    dest->setSource(source->source());
    dest->setMimeData(source->mimeData());
}

/*!
    \internal
*/
void QGraphicsScenePrivate::sendDragDropEvent(QGraphicsItem *item,
                                              QGraphicsSceneDragDropEvent *dragDropEvent)
{
    dragDropEvent->setPos(item->d_ptr->genericMapFromScene(dragDropEvent->scenePos(), dragDropEvent->widget()));
    sendEvent(item, dragDropEvent);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::sendHoverEvent(QEvent::Type type, QGraphicsItem *item,
                                           QGraphicsSceneHoverEvent *hoverEvent)
{
    QGraphicsSceneHoverEvent event(type);
    event.setWidget(hoverEvent->widget());
    event.setPos(item->d_ptr->genericMapFromScene(hoverEvent->scenePos(), hoverEvent->widget()));
    event.setScenePos(hoverEvent->scenePos());
    event.setScreenPos(hoverEvent->screenPos());
    sendEvent(item, &event);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::sendMouseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (mouseEvent->button() == 0 && mouseEvent->buttons() == 0) {
        // ### This is a temporary fix for until we get proper mouse
        // grab events. mouseGrabberItem should be set to 0 if the
        // view loses mouse grab.
        mouseGrabberItem = 0;
        return;
    }

    for (int i = 0x1; i <= 0x10; i <<= 1) {
        Qt::MouseButton button = Qt::MouseButton(i);
        mouseEvent->setButtonDownPos(button, mouseGrabberButtonDownPos.value(button, mouseGrabberItem->d_ptr->genericMapFromScene(mouseEvent->scenePos(), mouseEvent->widget())));
        mouseEvent->setButtonDownScenePos(button, mouseGrabberButtonDownScenePos.value(button, mouseEvent->scenePos()));
        mouseEvent->setButtonDownScreenPos(button, mouseGrabberButtonDownScreenPos.value(button, mouseEvent->screenPos()));
    }
    mouseEvent->setPos(mouseGrabberItem->d_ptr->genericMapFromScene(mouseEvent->scenePos(), mouseEvent->widget()));
    mouseEvent->setLastPos(mouseGrabberItem->d_ptr->genericMapFromScene(mouseEvent->lastScenePos(), mouseEvent->widget()));
    sendEvent(mouseGrabberItem, mouseEvent);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::mousePressEventHandler(QGraphicsSceneMouseEvent *mouseEvent)
{
    Q_Q(QGraphicsScene);
    if (mouseGrabberItem) {
        // We already have a mouse grabber. This means more than one button is
        // pressed at the same time. This is just delivered like a normal
        // event.
        sendMouseEvent(mouseEvent);
        return;
    }

    // Ignore by default, unless we find a mouse grabber that accepts it.
    mouseEvent->ignore();

    // Start by determining the number of items at the current position.
    // Reuse value from earlier calculations if possible.
    if (cachedItemsUnderMouse.isEmpty()) {
        cachedItemsUnderMouse = itemsAtPosition(mouseEvent->screenPos(),
                                                mouseEvent->scenePos(),
                                                mouseEvent->widget());
    }

    // Set focus on the topmost enabled item that can take focus.
    bool setFocus = false;
    foreach (QGraphicsItem *item, cachedItemsUnderMouse) {
        if (item->isEnabled() && (item->flags() & QGraphicsItem::ItemIsFocusable)) {
            setFocus = true;
            if (item != q->focusItem())
                q->setFocusItem(item, Qt::MouseFocusReason);
            break;
        }
    }

    // If nobody could take focus, we clear it.
    if (!setFocus)
        q->setFocusItem(0, Qt::MouseFocusReason);

    // Find a mouse grabber by sending mouse press events to all mouse grabber
    // candidates one at a time, until the event is accepted. It's accepted by
    // default, so the receiver has to explicitly ignore it for it to pass
    // through.
    foreach (QGraphicsItem *item, possibleMouseGrabbersForEvent(cachedItemsUnderMouse, mouseEvent)) {
        mouseGrabberItem = item;
        mouseEvent->accept();

        if (mouseEvent->type() == QEvent::GraphicsSceneMouseDoubleClick && item != lastMouseGrabberItem) {
            // If this item is different from the item that received the last
            // mouse event, and mouseEvent is a doubleclick event, then the
            // event is converted to a press. Known limitation:
            // Triple-clicking will not generate a doubleclick, though.
            QGraphicsSceneMouseEvent mousePress(QEvent::GraphicsSceneMousePress);
            mousePress.accept();
            mousePress.setButton(mouseEvent->button());
            mousePress.setButtons(mouseEvent->buttons());
            mousePress.setScreenPos(mouseEvent->screenPos());
            mousePress.setScenePos(mouseEvent->scenePos());
            mousePress.setModifiers(mouseEvent->modifiers());
            sendMouseEvent(&mousePress);
            mouseEvent->setAccepted(mousePress.isAccepted());
        } else {
            sendMouseEvent(mouseEvent);
        }
        if (mouseEvent->isAccepted()) {
            if (mouseGrabberItem)
                storeMouseButtonsForMouseGrabber(mouseEvent);
            lastMouseGrabberItem = mouseGrabberItem;
            return;
        }
    }

    // Is the event still ignored? Then the mouse press goes to the scene.
    // Reset the mouse grabber, clear the selection, clear focus, and leave
    // the event ignored so that it can propagate through the originating
    // view.
    if (!mouseEvent->isAccepted()) {
        lastMouseGrabberItem = mouseGrabberItem;

        QGraphicsView *view = mouseEvent->widget() ? qobject_cast<QGraphicsView *>(mouseEvent->widget()->parentWidget()) : 0;
        bool dontClearSelection = view && view->dragMode() == QGraphicsView::ScrollHandDrag;
        if (!dontClearSelection) {
            // Clear the selection if the originating view isn't in scroll
            // hand drag mode. The view will clear the selection if no drag
            // happened.
            q->clearSelection();
        }
    }
}

/*!
    \internal
*/
inline bool qt_closestLeaf(const QGraphicsItem *item1, const QGraphicsItem *item2)
{
    qreal z1 = item1->d_ptr->z;
    qreal z2 = item2->d_ptr->z;
    return z1 != z2 ? z1 > z2 : item1 > item2;
}

/*!
    \internal
*/
bool qt_closestItemFirst(const QGraphicsItem *item1, const QGraphicsItem *item2)
{
    // Siblings? Just check their z-values.
    if (item1->d_ptr->parent == item2->d_ptr->parent)
        return qt_closestLeaf(item1, item2);

    // Find item1's ancestors. If item2 is among them, return true (item1 is
    // above item2).
    QVector<const QGraphicsItem *> ancestors1;
    const QGraphicsItem *parent1 = item1;
    do {
        if (parent1 == item2)
            return true;
        ancestors1.prepend(parent1);
    } while ((parent1 = parent1->d_ptr->parent));

    // Find item2's ancestors. If item1 is among them, return false (item2 is
    // above item1).
    QVector<const QGraphicsItem *> ancestors2;
    const QGraphicsItem *parent2 = item2;
    do {
        if (parent2 == item1)
            return false;
        ancestors2.prepend(parent2);
    } while ((parent2 = parent2->d_ptr->parent));

    // Truncate the largest ancestor list.
    int size1 = ancestors1.size();
    int size2 = ancestors2.size();
    if (size1 > size2) {
        ancestors1.resize(size2);
    } else if (size2 > size1) {
        ancestors2.resize(size1);
    }

    // Compare items from the two ancestors lists and find a match. Then
    // compare item1's and item2's toplevels relative to the common ancestor.
    for (int i = ancestors1.size() - 2; i >= 0; --i) {
        const QGraphicsItem *a1 = ancestors1.at(i);
        const QGraphicsItem *a2 = ancestors2.at(i);
        if (a1 == a2)
            return qt_closestLeaf(ancestors1.at(i + 1), ancestors2.at(i + 1));
    }

    // No common ancestor? Then just compare the items' toplevels directly.
    return qt_closestLeaf(ancestors1.first(), ancestors2.first());
}

/*!
    \internal
*/
void QGraphicsScenePrivate::sortItems(QList<QGraphicsItem *> *itemList)
{
    qSort(itemList->begin(), itemList->end(), qt_closestItemFirst);
}

/*!
    Constructs a QGraphicsScene object. The \a parent parameter is
    passed to QObject's constructor.
*/
QGraphicsScene::QGraphicsScene(QObject *parent)
    : QObject(*new QGraphicsScenePrivate, parent)
{
    update();
}

/*!
    Constructs a QGraphicsScene object, using \a sceneRect for its
    scene rectangle. The \a parent parameter is passed to QObject's
    constructor.

    \sa sceneRect
*/
QGraphicsScene::QGraphicsScene(const QRectF &sceneRect, QObject *parent)
    : QObject(*new QGraphicsScenePrivate, parent)
{
    setSceneRect(sceneRect);
    update();
}

/*!
    Constructs a QGraphicsScene object, using the rectangle specified
    by (\a x, \a y), and the given \a width and \a height for its
    scene rectangle. The \a parent parameter is passed to QObject's
    constructor.

    \sa sceneRect
*/
QGraphicsScene::QGraphicsScene(qreal x, qreal y, qreal width, qreal height, QObject *parent)
    : QObject(*new QGraphicsScenePrivate, parent)
{
    setSceneRect(x, y, width, height);
    update();
}

/*!
    Destroys the QGraphicsScene object.
*/
QGraphicsScene::~QGraphicsScene()
{
    Q_D(QGraphicsScene);
    for (int i = 0; i < d->unindexedItems.size(); ++i) {
        if (QGraphicsItem *item = d->unindexedItems[i]) {
            d->unindexedItems[i] = 0;
            d->removeFromIndex(item);
            item->d_func()->scene = 0;
            delete item;
        }
    }
    for (int i = 0; i < d->indexedItems.size(); ++i) {
        if (QGraphicsItem *item = d->indexedItems[i]) {
            if (!d->removedItems.contains(item)) {
                d->indexedItems[i] = 0;
                d->removeFromIndex(item);
                item->d_func()->scene = 0;
                delete item;
            }
        }
    }

    // Remove this scene from all associated views.
    for (int j = 0; j < d->views.size(); ++j)
        d->views.at(j)->setScene(0);
}

/*!
    \property QGraphicsScene::sceneRect
    \brief the scene rectangle; the bounding rectangle of the scene

    The scene rectangle defines the extent of the scene. It is
    primarily used by QGraphicsView to determine the view's default
    scrollable area, and by QGraphicsScene to manage item indexing.

    If unset, or if set to a null QRectF, sceneRect() will return the largest
    bounding rect of all items on the scene since the scene was created (i.e.,
    a rectangle that grows when items are added to or moved in the scene, but
    never shrinks).

    \sa width(), height(), QGraphicsView::sceneRect
*/
QRectF QGraphicsScene::sceneRect() const
{
    Q_D(const QGraphicsScene);
    const_cast<QGraphicsScenePrivate *>(d)->_q_generateBspTree();
    return d->hasSceneRect ? d->sceneRect : d->growingItemsBoundingRect;
}
void QGraphicsScene::setSceneRect(const QRectF &rect)
{
    Q_D(QGraphicsScene);
    if (rect != d->sceneRect) {
        d->hasSceneRect = !rect.isNull();
        d->sceneRect = rect;
        d->resetIndex();
        emit sceneRectChanged(rect);
    }
}

/*!
     \fn qreal QGraphicsScene::width() const

     This convenience function is equivalent to calling sceneRect().width().

     \sa height()
*/

/*!
     \fn qreal QGraphicsScene::height() const

     This convenience function is equivalent to calling \c sceneRect().height().

     \sa width()
*/

/*!
    Renders the \a source rect from scene into \a target, using \a painter. This
    function is useful for capturing the contents of the scene onto a paint
    device, such as a QImage (e.g., to take a screenshot), or for printing
    with QPrinter. For example:

    \code
        QGraphicsScene scene;
        scene.addItem(...
        ...
        QPrinter printer(QPrinter::HighResolution);
        printer.setPageSize(QPrinter::A4);

        QPainter painter(&printer);
        scene.render(&painter);
    \endcode

    If \a source is a null rect, this function will use sceneRect() to
    determine what to render. If \a target is a null rect, the dimensions of \a
    painter's paint device will be used.

    The source rect contents will be transformed according to \a
    aspectRatioMode to fit into the target rect. By default, the aspect ratio
    is kept, and \a source is scaled to fit in \a target.

    \sa QGraphicsView::render()
*/
void QGraphicsScene::render(QPainter *painter, const QRectF &target, const QRectF &source,
                            Qt::AspectRatioMode aspectRatioMode)
{
    Q_D(QGraphicsScene);

    // Default source rect = scene rect
    QRectF sourceRect = source;
    if (sourceRect.isNull())
        sourceRect = sceneRect();

    // Default target rect = device rect
    QRectF targetRect = target;
    if (targetRect.isNull()) {
        if (painter->device()->devType() == QInternal::Picture)
            targetRect = sourceRect;
        else
            targetRect.setRect(0, 0, painter->device()->width(), painter->device()->height());
    }

    // Find the ideal x / y scaling ratio to fit \a source into \a target.
    qreal xratio = targetRect.width() / sourceRect.width();
    qreal yratio = targetRect.height() / sourceRect.height();

    // Scale according to the aspect ratio mode.
    switch (aspectRatioMode) {
    case Qt::KeepAspectRatio:
        xratio = yratio = qMin(xratio, yratio);
        break;
    case Qt::KeepAspectRatioByExpanding:
        xratio = yratio = qMax(xratio, yratio);
        break;
    case Qt::IgnoreAspectRatio:
        break;
    }

    // Find all items to draw, and reverse the list (we want to draw
    // in reverse order).
    QList<QGraphicsItem *> itemList = items(sourceRect, Qt::IntersectsItemBoundingRect);
    QGraphicsItem **itemArray = new QGraphicsItem *[itemList.size()];
    int numItems = itemList.size();
    for (int i = 0; i < numItems; ++i)
        itemArray[numItems - i - 1] = itemList.at(i);
    itemList.clear();

    painter->save();

    // Transform the painter.
    painter->setClipRect(targetRect);
    painter->translate(targetRect.left(), targetRect.top());
    painter->scale(xratio, yratio);
    painter->translate(-sourceRect.left(), -sourceRect.top());

    // Two unit vectors.
    QLineF v1(0, 0, 1, 0);
    QLineF v2(0, 0, 0, 1);

    // Generate the style options
    QStyleOptionGraphicsItem *styleOptionArray = new QStyleOptionGraphicsItem[numItems];
    for (int i = 0; i < numItems; ++i) {
        QGraphicsItem *item = itemArray[i];

        QStyleOptionGraphicsItem option;
        option.state = QStyle::State_None;
        option.rect = item->boundingRect().toRect();
        if (item->isSelected())
            option.state |= QStyle::State_Selected;
        if (item->isEnabled())
            option.state |= QStyle::State_Enabled;
        if (item->hasFocus())
            option.state |= QStyle::State_HasFocus;
        if (d->hoverItems.contains(item))
            option.state |= QStyle::State_MouseOver;
        if (item == mouseGrabberItem())
            option.state |= QStyle::State_Sunken;

        // Calculate a simple level-of-detail metric.
        QTransform neo = item->sceneTransform() * painter->transform();
        option.levelOfDetail = ::sqrt(double(neo.map(v1).length() * neo.map(v2).length()));
        option.matrix = neo.toAffine(); //### discards perspective

        option.exposedRect = item->boundingRect();
        option.exposedRect &= neo.inverted().mapRect(targetRect);

        styleOptionArray[i] = option;
    }

    // Render the scene.
    drawBackground(painter, sourceRect);
    drawItems(painter, numItems, itemArray, styleOptionArray);
    drawForeground(painter, sourceRect);

    delete [] itemArray;
    delete [] styleOptionArray;

    painter->restore();
}

/*!
    \property QGraphicsScene::itemIndexMethod
    \brief the item indexing method.

    QGraphicsScene applies an indexing algorithm to the scene, to speed up
    item discovery functions like items() and itemAt(). Indexing is most
    efficient for static scenes (i.e., where items don't move around). For
    dynamic scenes, or scenes with many animated items, the index bookkeeping
    can outweight the fast lookup speeds.

    For the common case, the default index method BspTreeIndex works fine.  If
    your scene uses many animations and you are experiencing slowness, you can
    disable indexing by calling \c setItemIndexMethod(NoIndex).

    \sa bspTreeDepth
*/
QGraphicsScene::ItemIndexMethod QGraphicsScene::itemIndexMethod() const
{
    Q_D(const QGraphicsScene);
    return d->indexMethod;
}
void QGraphicsScene::setItemIndexMethod(ItemIndexMethod method)
{
    Q_D(QGraphicsScene);
    d->resetIndex();
    d->indexMethod = method;
}

/*!
    \property QGraphicsScene::bspTreeDepth
    \brief the depth of QGraphicsScene's BSP index tree
    \since 4.3

    This property has no effect when NoIndex is used.

    This value determines the depth of QGraphicsScene's BSP tree. The depth
    directly affects QGraphicsScene's performance and memory usage; the latter
    growing exponentially with the depth of the tree. With an optimal tree
    depth, QGraphicsScene can instantly determine the locality of items, even
    for scenes with thousands or millions of items. This also greatly improves
    rendering performance.

    By default, the value is 0, in which case Qt will guess a reasonable
    default depth based on the size, location and number of items in the
    scene. If these parameters change frequently, however, you may experience
    slowdowns as QGraphicsScene retunes the depth internally. You can avoid
    potential slowdowns by fixating the tree depth through setting this
    property.

    The depth of the tree and the size of the scene rectangle decide the
    granularity of the scene's partitioning. The size of each scene segment is
    determined by the following algorithm:

    \code
        QSizeF segmentSize = sceneRect().size() / pow(2, depth - 1);
    \endcode

    The BSP tree has an optimal size when each segment contains between 0 and
    10 items.

    \sa itemIndexMethod
*/
int QGraphicsScene::bspTreeDepth() const
{
    Q_D(const QGraphicsScene);
    return d->bspTreeDepth;
}
void QGraphicsScene::setBspTreeDepth(int depth)
{
    Q_D(QGraphicsScene);
    if (d->bspTreeDepth == depth)
        return;

    if (depth < 0) {
        qWarning("QGraphicsScene::setBspTreeDepth: invalid depth %d ignored; must be >= 0", depth);
        return;
    }

    d->bspTreeDepth = depth;
    d->resetIndex();
}

/*!
    Calculates and returns the bounding rect of all items on the scene. This
    function works by iterating over all items, and because if this, it can
    be slow for large scenes.

    \sa sceneRect()
*/
QRectF QGraphicsScene::itemsBoundingRect() const
{
    QRectF boundingRect;
    foreach (QGraphicsItem *item, items())
        boundingRect |= item->sceneBoundingRect();
    return boundingRect;
}

/*!
    Returns a list of all items on the scene, in no particular order.

    \sa addItem(), removeItem()
*/
QList<QGraphicsItem *> QGraphicsScene::items() const
{
    Q_D(const QGraphicsScene);
    const_cast<QGraphicsScenePrivate *>(d)->purgeRemovedItems();

    // If freeItemIndexes is empty, we know there are no holes in indexedItems and
    // unindexedItems.
    if (d->freeItemIndexes.isEmpty()) {
        if (d->unindexedItems.isEmpty())
            return d->indexedItems;
        return d->indexedItems + d->unindexedItems;
    }

    // Rebuild the list of items to avoid holes. ### We could also just
    // compress the item lists at this point.
    QList<QGraphicsItem *> itemList;
    foreach (QGraphicsItem *item, d->indexedItems + d->unindexedItems) {
        if (item)
            itemList << item;
    }
    return itemList;
}

/*!
    Returns all visible items at position \a pos in the scene. The items are
    listed in descending Z order (i.e., the first item in the list is the
    top-most item, and the last item is the bottom-most item).

    \sa itemAt()
*/
QList<QGraphicsItem *> QGraphicsScene::items(const QPointF &pos) const
{
    QList<QGraphicsItem *> itemsAtPoint;

    // Find all items within a 1x1 rect area starting at pos. This can be
    // inefficient for scenes that use small coordinates (like unity
    // coordinates), or for detailed graphs. ### The index should support
    // fetching items at a pos to avoid this limitation.
    foreach (QGraphicsItem *item, items(QRectF(pos, QSizeF(1, 1)), Qt::IntersectsItemBoundingRect)) {
        if (item->contains(item->mapFromScene(pos)))
            itemsAtPoint << item;
    }
    return itemsAtPoint;
}

/*!
    \internal

    Inserts \a item into \a result if selectionPath collides with it according
    to \a mode.
*/
static void _qt_pathIntersectsItem(const QPainterPath &selectionPath, QGraphicsItem *item,
                                   Qt::ItemSelectionMode mode, QList<QGraphicsItem *> *result)
{
    if (selectionPath.isEmpty())
        return;

    QPainterPath path;
    if (mode == Qt::ContainsItemShape || mode == Qt::IntersectsItemShape) {
        path = item->mapToScene(item->shape());
    } else {
        path.addPolygon(_q_adjustedRect(item->sceneBoundingRect()));
        path.closeSubpath();
    }

    if (path.isEmpty())
        return;

    bool intersects = selectionPath.intersects(path);
    if (mode == Qt::IntersectsItemShape || mode == Qt::IntersectsItemBoundingRect) {
        if (intersects || selectionPath.contains(path.elementAt(0)) || path.contains(selectionPath.elementAt(0))) {
            *result << item;
        }
    } else if (!intersects && selectionPath.contains(path.elementAt(0))) {
        *result << item;
    }
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsScene::items(const QRectF &rectangle, Qt::ItemSelectionMode mode) const

    \overload

    Returns all visible items that, depending on \a mode, are either inside or
    intersect with the specified \a rectangle.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a rectangle are returned.

    \sa itemAt()
*/
QList<QGraphicsItem *> QGraphicsScene::items(const QRectF &rect, Qt::ItemSelectionMode mode) const
{
    Q_D(const QGraphicsScene);
    QList<QGraphicsItem *> itemsInRect;

    QPainterPath rectPath;
    rectPath.addRect(rect);

    // The index returns a rough estimate of what items are inside the rect.
    // Refine it by iterating through all returned items.
    foreach (QGraphicsItem *item, d->estimateItemsInRect(rect))
        _qt_pathIntersectsItem(rectPath, item, mode, &itemsInRect);

    d->sortItems(&itemsInRect);
    return itemsInRect;
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsScene::items(qreal x, qreal y, qreal w, qreal h, Qt::ItemSelectionMode mode) const
    \since 4.3

    This convenience function is equivalent to calling items(QRectF(\a x, \a y, \a w, \a h), \a mode).
*/

/*!
    \overload

    Returns all visible items that, depending on \a mode, are either inside or
    intersect with the polygon \a polygon.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a polygon are returned.

    \sa itemAt()
*/
QList<QGraphicsItem *> QGraphicsScene::items(const QPolygonF &polygon, Qt::ItemSelectionMode mode) const
{
    Q_D(const QGraphicsScene);
    QList<QGraphicsItem *> itemsInPolygon;

    QPainterPath polyPath;
    polyPath.addPolygon(polygon);
    polyPath.closeSubpath();

    QRectF polyRect = polygon.boundingRect();

    // The index returns a rough estimate of what items are inside the rect.
    // Refine it by iterating through all returned items.
    foreach (QGraphicsItem *item, d->estimateItemsInRect(polygon.boundingRect())) {
        if (mode == Qt::IntersectsItemBoundingRect
            && polygon.containsPoint(item->mapToScene(item->boundingRect().topLeft()),
                                     Qt::OddEvenFill)) {
            itemsInPolygon << item;
        } else if (polyRect.intersects(_q_adjustedRect(item->sceneBoundingRect()))) {
            _qt_pathIntersectsItem(polyPath, item, mode, &itemsInPolygon);
        }
    }

    d->sortItems(&itemsInPolygon);
    return itemsInPolygon;
}

/*!
    \overload

    Returns all visible items that, depending on \a path, are either inside or
    intersect with the path \a path.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a path are returned.

    \sa itemAt()
*/
QList<QGraphicsItem *> QGraphicsScene::items(const QPainterPath &path, Qt::ItemSelectionMode mode) const
{
    Q_D(const QGraphicsScene);
    QList<QGraphicsItem *> tmp;

    // The index returns a rough estimate of what items are inside the rect.
    // Refine it by iterating through all returned items.
    foreach (QGraphicsItem *item, d->estimateItemsInRect(path.controlPointRect()))
        _qt_pathIntersectsItem(path, item, mode, &tmp);

    d->sortItems(&tmp);
    return tmp;
}

/*!
    Returns a list of all items that collide with \a item. Collisions are
    determined by calling QGraphicsItem::collidesWithItem(); the collision
    detection is determined by \a mode. By default, all items whose shape
    intersects \a item or is contained inside \a item's shape are returned.

    The items are returned in descending Z order (i.e., the first item in the
    list is the top-most item, and the last item is the bottom-most item).

    \sa items(), itemAt(), QGraphicsItem::collidesWithItem()
*/
QList<QGraphicsItem *> QGraphicsScene::collidingItems(const QGraphicsItem *item,
                                                      Qt::ItemSelectionMode mode) const
{
    Q_D(const QGraphicsScene);
    if (!item) {
        qWarning("QGraphicsScene::collidingItems: cannot find collisions for null item");
        return QList<QGraphicsItem *>();
    }

    QList<QGraphicsItem *> tmp;
    foreach (QGraphicsItem *itemInVicinity, d->estimateItemsInRect(item->sceneBoundingRect())) {
        if (item != itemInVicinity && item->collidesWithItem(itemInVicinity, mode))
            tmp << itemInVicinity;
    }
    d->sortItems(&tmp);
    return tmp;
}

/*!
    \fn QGraphicsItem *QGraphicsScene::itemAt(const QPointF &position) const

    Returns the topmost visible item at the specified \a position, or 0 if
    there are no items at this position.

    \sa items(), collidingItems()
*/
QGraphicsItem *QGraphicsScene::itemAt(const QPointF &pos) const
{
    QList<QGraphicsItem *> itemsAtPoint = items(pos);
    return itemsAtPoint.isEmpty() ? 0 : itemsAtPoint.first();
}

/*!
    \fn QGraphicsScene::itemAt(qreal x, qreal y) const
    \overload

    Returns the topmost item at the position specified by (\a x, \a
    y), or 0 if there are no items at this position.

    This convenience function is equivalent to calling \c
    {itemAt(QPointF(x, y))}.
*/

/*!
    Returns a list of all currently selected items. The items are
    returned in no particular order.

    \sa setSelectionArea()
*/
QList<QGraphicsItem *> QGraphicsScene::selectedItems() const
{
    Q_D(const QGraphicsScene);

    // Optimization: Lazily removes items that are not selected.
    QGraphicsScene *that = const_cast<QGraphicsScene *>(this);
    QSet<QGraphicsItem *> actuallySelectedSet;
    foreach (QGraphicsItem *item, that->d_func()->selectedItems) {
        if (item->isSelected())
            actuallySelectedSet << item;
    }

    that->d_func()->selectedItems = actuallySelectedSet;

    return d->selectedItems.values();
}

/*!
    Returns the selection area that was previously set with
    setSelectionArea(), or an empty QPainterPath if no selection area has been
    set.

    \sa setSelectionArea()
*/
QPainterPath QGraphicsScene::selectionArea() const
{
    Q_D(const QGraphicsScene);
    return d->selectionArea;
}

/*!
    Sets the selection area to \a path. All items within this area are
    immediately selected, and all items outside are unselected. You can get
    the list of all selected items by calling selectedItems().

    For an item to be selected, it must be marked as \e selectable
    (QGraphicsItem::ItemIsSelectable).

    \sa clearSelection(), selectionArea()
*/
void QGraphicsScene::setSelectionArea(const QPainterPath &path)
{
    setSelectionArea(path, Qt::IntersectsItemShape);
}

/*!
    \overload
    \since 4.3

    Sets the selection area to \a path using \a mode to determine if items are
    included in the selection area.

    \sa clearSelection(), selectionArea()
*/
void QGraphicsScene::setSelectionArea(const QPainterPath &path, Qt::ItemSelectionMode mode)
{
    Q_D(QGraphicsScene);

    // Note: with boolean path operations, we can improve performance here
    // quite a lot by "growing" the old path instead of replacing it. That
    // allows us to only check the intersect area for changes, instead of
    // reevaluating the whole path over again.
    d->selectionArea = path;

    QSet<QGraphicsItem *> unselectItems = d->selectedItems;

    // Disable emitting selectionChanged() for individual items.
    ++d->selectionChanging;
    bool changed = false;

    // Set all items in path to selected.
    foreach (QGraphicsItem *item, items(path, mode)) {
        if (item->flags() & QGraphicsItem::ItemIsSelectable) {
            if (!item->isSelected())
                changed = true;
            unselectItems.remove(item);
            item->setSelected(true);
        }
    }

    // Unselect all items outside path.
    foreach (QGraphicsItem *item, unselectItems) {
        item->setSelected(false);
        changed = true;
    }

    // Reenable emitting selectionChanged() for individual items.
    --d->selectionChanging;

    if (!d->selectionChanging && changed)
        emit selectionChanged();
}

/*!
   Clears the current selection.

   \sa setSelectionArea(), selectedItems()
*/
void QGraphicsScene::clearSelection()
{
    Q_D(QGraphicsScene);

    // Disable emitting selectionChanged
    ++d->selectionChanging;
    bool changed = !d->selectedItems.isEmpty();

    foreach (QGraphicsItem *item, d->selectedItems)
        item->setSelected(false);
    d->selectedItems.clear();

    // Reenable emitting selectionChanged() for individual items.
    --d->selectionChanging;

    if (!d->selectionChanging && changed)
        emit selectionChanged();
}

/*!
    Groups all items in \a items into a new QGraphicsItemGroup, and returns a
    pointer to the group. The group is created with the common ancestor of \a
    items as its parent, and with position (0, 0). The items are all
    reparented to the group, and their positions and transformations are
    mapped to the group. If \a items is empty, this function will return an
    empty top-level QGraphicsItemGroup.

    QGraphicsScene has ownership of the group item; you do not need to delete
    it. To dismantle (ungroup) a group, call destroyItemGroup().

    \sa destroyItemGroup(), QGraphicsItemGroup::addToGroup()
*/
QGraphicsItemGroup *QGraphicsScene::createItemGroup(const QList<QGraphicsItem *> &items)
{
    // Build a list of the first item's ancestors
    QList<QGraphicsItem *> ancestors;
    int n = 0;
    if (!items.isEmpty()) {
        QGraphicsItem *parent = items.at(n++);
        while ((parent = parent->parentItem()))
            ancestors.append(parent);
    }

    // Find the common ancestor for all items
    QGraphicsItem *commonAncestor = 0;
    if (!ancestors.isEmpty()) {
        while (n < items.size()) {
            int commonIndex = -1;
            QGraphicsItem *parent = items.at(n++);
            do {
                int index = ancestors.indexOf(parent, qMax(0, commonIndex));
                if (index != -1) {
                    commonIndex = index;
                    break;
                }
            } while ((parent = parent->parentItem()));

            if (commonIndex == -1) {
                commonAncestor = 0;
                break;
            }

            commonAncestor = ancestors.at(commonIndex);
        }
    }

    // Create a new group at that level
    QGraphicsItemGroup *group = new QGraphicsItemGroup(commonAncestor);
    if (!commonAncestor)
        addItem(group);
    foreach (QGraphicsItem *item, items)
        group->addToGroup(item);
    return group;
}

/*!
    Reparents all items in \a group to \a group's parent item, then removes \a
    group from the scene, and finally deletes it. The items' positions and
    transformations are mapped from the group to the group's parent.

    \sa createItemGroup(), QGraphicsItemGroup::removeFromGroup()
*/
void QGraphicsScene::destroyItemGroup(QGraphicsItemGroup *group)
{
    foreach (QGraphicsItem *item, group->children())
        group->removeFromGroup(item);
    removeItem(group);
    delete group;
}

/*!
    Adds or moves the item \a item and all its childen to the scene.

    If the item is visible (i.e., QGraphicsItem::isVisible() returns
    true), QGraphicsScene will emit changed() once control goes back
    to the event loop.

    If the item is already in a different scene, it will first be removed from
    its old scene, and then added to this scene as a top-level.

    \sa removeItem(), addEllipse(), addLine(), addPath(), addPixmap(),
    addRect(), addText()
*/
void QGraphicsScene::addItem(QGraphicsItem *item)
{
    Q_D(QGraphicsScene);
    if (!item) {
        qWarning("QGraphicsScene::addItem: cannot add null item");
        return;
    }
    if (item->scene() == this) {
        qWarning("QGraphicsScene::addItem: item has already been added to this scene");
        return;
    }

    // Prevent reusing a recently deleted pointer: purge all removed items
    // from our lists.
    d->purgeRemovedItems();

    // Remove this item from its existing scene
    if (item->d_func()->scene)
        item->d_func()->scene->removeItem(item);

    // Detach this item from its parent if the parent's scene is different
    // from this scene.
    if (QGraphicsItem *itemParent = item->parentItem()) {
        if (itemParent->scene() != this) {
            QVariant variant;
            qVariantSetValue<QGraphicsItem *>(variant, item);
            itemParent->itemChange(QGraphicsItem::ItemChildRemovedChange, variant);
            itemParent->d_func()->children.removeAll(item);
            item->d_func()->parent = 0;
        }
    }

    // Add the item to this scene
    item->d_func()->scene = qVariantValue<QGraphicsScene *>(item->itemChange(QGraphicsItem::ItemSceneChange,
                                                                             qVariantFromValue<QGraphicsScene *>(this)));
    if (d->indexMethod != QGraphicsScene::NoIndex) {
        // Indexing requires sceneBoundingRect(), but because \a item might
        // not be completely constructed at this point, we need to store it in
        // a temporary list and schedule an indexing for later.
        d->unindexedItems << item;
        item->d_func()->index = -1;
        d->startIndexTimer();
    } else {
        // No index: We can insert the item directly.
        if (!d->freeItemIndexes.isEmpty()) {
            int newIndex = d->freeItemIndexes.takeLast();
            Q_ASSERT_X(d->indexedItems[newIndex] == 0, "QGraphicsItem::addItem",
                       "An index marked as free was still occupied");
            d->indexedItems[newIndex] = item;
            item->d_func()->index = newIndex;
        } else {
            item->d_func()->index = d->indexedItems.size();
            d->indexedItems << item;
        }
    }

    // Add to list of items that require an update. We cannot assume that the
    // item is fully constructed, so calling item->update() can lead to a pure
    // virtual function call to boundingRect().
    if (!d->updateAll) {
        if (d->pendingUpdateItems.isEmpty())
            QTimer::singleShot(0, this, SLOT(_q_updateLater()));
        d->pendingUpdateItems << item;
    }

    // Disable selectionChanged() for individual items
    ++d->selectionChanging;
    int oldSelectedItemSize = d->selectedItems.size();

    // Update selection lists
    if (item->isSelected())
        d->selectedItems << item;

    // Add all children recursively
    foreach (QGraphicsItem *child, item->children())
        addItem(child);

    // Reenable selectionChanged() for individual items
    --d->selectionChanging;
    if (!d->selectionChanging && d->selectedItems.size() != oldSelectedItemSize)
        emit selectionChanged();
}

/*!
    Creates and adds an ellipse item to the scene, and returns the item
    pointer. The geometry of the ellipse is defined by \a rect, and it's pen
    and brush are initialized to \a pen and \a brush.

    Note that the item's geometry is provided in item coordinates, and its
    position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addLine(), addPath(), addPixmap(), addRect(), addText(), addItem()
*/
QGraphicsEllipseItem *QGraphicsScene::addEllipse(const QRectF &rect, const QPen &pen, const QBrush &brush)
{
    QGraphicsEllipseItem *item = new QGraphicsEllipseItem(rect);
    item->setPen(pen);
    item->setBrush(brush);
    addItem(item);
    return item;
}

/*!
    \fn QGraphicsEllipseItem *QGraphicsScene::addEllipse(qreal x, qreal y, qreal w, qreal h, const QPen &pen, const QBrush &brush)
    \since 4.3

    This convenience function is equivalent to calling addEllipse(QRectF(\a x,
    \a y, \a w, \a h), \a pen, \a brush).
*/

/*!
    Creates and adds a line item to the scene, and returns the item
    pointer. The geometry of the line is defined by \a line, and it's pen
    is initialized to \a pen.

    Note that the item's geometry is provided in item coordinates, and its
    position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addEllipse(), addPath(), addPixmap(), addRect(), addText(), addItem()
*/
QGraphicsLineItem *QGraphicsScene::addLine(const QLineF &line, const QPen &pen)
{
    QGraphicsLineItem *item = new QGraphicsLineItem(line);
    item->setPen(pen);
    addItem(item);
    return item;
}

/*!
    \fn QGraphicsLineItem *QGraphicsScene::addLine(qreal x1, qreal y1, qreal x2, qreal y2, const QPen &pen)
    \since 4.3

    This convenience function is equivalent to calling addLine(QLineF(\a x1,
    \a y1, \a x2, \a y2), \a pen).
*/

/*!
    Creates and adds a path item to the scene, and returns the item
    pointer. The geometry of the path is defined by \a path, and it's pen and
    brush are initialized to \a pen and \a brush.

    Note that the item's geometry is provided in item coordinates, and its
    position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addEllipse(), addLine(), addPixmap(), addRect(), addText(), addItem()
*/
QGraphicsPathItem *QGraphicsScene::addPath(const QPainterPath &path, const QPen &pen, const QBrush &brush)
{
    QGraphicsPathItem *item = new QGraphicsPathItem(path);
    item->setPen(pen);
    item->setBrush(brush);
    addItem(item);
    return item;
}

/*!
    Creates and adds a pixmap item to the scene, and returns the item
    pointer. The pixmap is defined by \a pixmap.

    Note that the item's geometry is provided in item coordinates, and its
    position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addEllipse(), addLine(), addPath(), addRect(), addText(), addItem()
*/
QGraphicsPixmapItem *QGraphicsScene::addPixmap(const QPixmap &pixmap)
{
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmap);
    addItem(item);
    return item;
}

/*!
    Creates and adds a polygon item to the scene, and returns the item
    pointer. The polygon is defined by \a polygon, and it's pen and
    brush are initialized to \a pen and \a brush.

    Note that the item's geometry is provided in item coordinates, and its
    position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addEllipse(), addLine(), addPath(), addRect(), addText(), addItem()
*/
QGraphicsPolygonItem *QGraphicsScene::addPolygon(const QPolygonF &polygon,
                                                 const QPen &pen, const QBrush &brush)
{
    QGraphicsPolygonItem *item = new QGraphicsPolygonItem(polygon);
    item->setPen(pen);
    item->setBrush(brush);
    addItem(item);
    return item;
}

/*!
    Creates and adds a rectangle item to the scene, and returns the item
    pointer. The geometry of the rectangle is defined by \a rect, and it's pen
    and brush are initialized to \a pen and \a brush.

    Note that the item's geometry is provided in item coordinates, and its
    position is initialized to (0, 0). For example, if a QRect(50, 50, 100,
    100) is added, its top-left corner will be at (50, 50) relative to the
    origin in the items coordinate system.

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addEllipse(), addLine(), addPixmap(), addPixmap(), addText(), addItem()
*/
QGraphicsRectItem *QGraphicsScene::addRect(const QRectF &rect, const QPen &pen, const QBrush &brush)
{
    QGraphicsRectItem *item = new QGraphicsRectItem(rect);
    item->setPen(pen);
    item->setBrush(brush);
    addItem(item);
    return item;
}

/*!
    \fn QGraphicsRectItem *QGraphicsScene::addRect(qreal x, qreal y, qreal w, qreal h, const QPen &pen, const QBrush &brush)
    \since 4.3

    This convenience function is equivalent to calling addRect(QRectF(\a x,
    \a y, \a w, \a h), \a pen, \a brush).
*/

/*!
    Creates and adds a text item to the scene, and returns the item
    pointer. The text string is initialized to \a text, and it's font
    is initialized to \a font.

    The item's position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addEllipse(), addLine(), addPixmap(), addPixmap(), addRect(), addItem()
*/
QGraphicsTextItem *QGraphicsScene::addText(const QString &text, const QFont &font)
{
    QGraphicsTextItem *item = new QGraphicsTextItem(text);
    item->setFont(font);
    addItem(item);
    return item;
}

/*!
    Creates and adds a QGraphicsSimpleTextItem to the scene, and returns the
    item pointer. The text string is initialized to \a text, and it's font is
    initialized to \a font.

    The item's position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addEllipse(), addLine(), addPixmap(), addPixmap(), addRect(), addItem()
*/
QGraphicsSimpleTextItem *QGraphicsScene::addSimpleText(const QString &text, const QFont &font)
{
    QGraphicsSimpleTextItem *item = new QGraphicsSimpleTextItem(text);
    item->setFont(font);
    addItem(item);
    return item;
}

/*!
    Removes the item \a item and all its children from the scene.  The
    ownership of \a item is passed on to the caller (i.e.,
    QGraphicsScene will no longer delete \a item when destroyed).

    \sa addItem()
*/
void QGraphicsScene::removeItem(QGraphicsItem *item)
{
    // ### Refactoring: This function shares much functionality with _q_removeItemLater()
    Q_D(QGraphicsScene);
    if (!item) {
        qWarning("QGraphicsScene::removeItem: cannot remove 0-item");
        return;
    }
    if (item->scene() != this) {
        qWarning("QGraphicsScene::removeItem: item %p's scene (%p)"
                 " is different from this scene (%p)",
                 item, item->scene(), this);
        return;
    }

    // Clear its background
    item->update();

    // Note: This will access item's sceneBoundingRect(), which (as this is
    // C++) is why we cannot call removeItem() from QGraphicsItem's
    // destructor.
    d->removeFromIndex(item);

    // Set the item's scene ptr to 0.
    item->d_func()->scene = qVariantValue<QGraphicsScene *>(item->itemChange(QGraphicsItem::ItemSceneChange,
                                                                             qVariantFromValue<QGraphicsScene *>(0)));

    // Detach the item from its parent.
    if (QGraphicsItem *parentItem = item->parentItem()) {
        if (parentItem->scene()) {
            Q_ASSERT_X(parentItem->scene() == this, "QGraphicsScene::removeItem",
                       "Parent item's scene is different from this item's scene");
            item->setParentItem(0);
        }
    }

    // Remove from our item lists.
    int index = item->d_func()->index;
    if (index != -1) {
        d->freeItemIndexes << index;
        d->indexedItems[index] = 0;
    } else {
        d->unindexedItems.removeAll(item);
    }

    // Reset the mouse grabber and focus item data.
    if (item == d->mouseGrabberItem) {
        d->lastMouseGrabberItem = d->mouseGrabberItem;
        d->mouseGrabberItem = 0;
    }
    if (item == d->focusItem)
        d->focusItem = 0;
    if (item == d->lastFocusItem)
        d->lastFocusItem = 0;

    // Disable selectionChanged() for individual items
    ++d->selectionChanging;
    int oldSelectedItemsSize = d->selectedItems.size();

    // Update selected & hovered item bookkeeping
    d->selectedItems.remove(item);
    d->hoverItems.removeAll(item);
    d->pendingUpdateItems.removeAll(item);
    d->cachedItemsUnderMouse.removeAll(item);

    // Remove all children recursively
    foreach (QGraphicsItem *child, item->children())
        removeItem(child);

    // Reenable selectionChanged() for individual items
    --d->selectionChanging;

    if (!d->selectionChanging && d->selectedItems.size() != oldSelectedItemsSize)
        emit selectionChanged();
}

/*!
    Returns the scene's current focus item, or 0 if no item currently has
    focus.

    The focus item receives keyboard input when the scene receives a
    key event.

    \sa setFocusItem(), QGraphicsItem::hasFocus()
*/
QGraphicsItem *QGraphicsScene::focusItem() const
{
    Q_D(const QGraphicsScene);
    return d->focusItem;
}

/*!
    Sets the scene's focus item to \a item, with the focus reason \a
    focusReason, after removing focus from any previous item that may have had
    focus.

    If \a item is 0, or if it either does not accept focus (i.e., it does not
    have the QGraphicsItem::ItemIsFocusable flag enabled), or is not visible
    or not enabled, this function only removes focus from any previous
    focusitem.

    If item is not 0, and the scene does not currently have focus (i.e.,
    hasFocus() returns false), this function will call setFocus()
    automatically.

    \sa focusItem(), hasFocus(), setFocus()
*/
void QGraphicsScene::setFocusItem(QGraphicsItem *item, Qt::FocusReason focusReason)
{
    Q_D(QGraphicsScene);
    if (item == d->focusItem)
        return;
    if (item && (!(item->flags() & QGraphicsItem::ItemIsFocusable)
                 || !item->isVisible() || !item->isEnabled())) {
        item = 0;
    }

    if (item) {
        setFocus(focusReason);
        if (item == d->focusItem)
            return;
    }

    if (d->focusItem) {
        QFocusEvent event(QEvent::FocusOut, focusReason);
        d->lastFocusItem = d->focusItem;
        d->sendEvent(d->focusItem, &event);
        d->focusItem = 0;
    }

    if (item) {
        d->focusItem = item;
        QFocusEvent event(QEvent::FocusIn, focusReason);
        d->sendEvent(item, &event);
    }
}

/*!
    Returns true if the scene has focus; otherwise returns false. If the scene
    has focus, it will will forward key events from QKeyEvent to any item that
    has focus.

    \sa setFocus(), setFocusItem()
*/
bool QGraphicsScene::hasFocus() const
{
    Q_D(const QGraphicsScene);
    return d->hasFocus;
}

/*!
    Sets focus on the scene by sending a QFocusEvent to the scene, passing \a
    focusReason as the reason. If the scene regains focus after having
    previously lost it while an item had focus, the last focus item will
    receive focus with \a focusReason as the reason.

    If the scene already has focus, this function does nothing.

    \sa hasFocus(), clearFocus(), setFocusItem()
*/
void QGraphicsScene::setFocus(Qt::FocusReason focusReason)
{
    Q_D(QGraphicsScene);
    if (d->hasFocus)
        return;
    QFocusEvent event(QEvent::FocusIn, focusReason);
    QCoreApplication::sendEvent(this, &event);
}

/*!
    Clears focus from the scene. If any item has focus when this function is
    called, it will lose focus, and regain focus again once the scene regains
    focus.

    A scene that does not have focus ignores key events.

    \sa hasFocus(), setFocus(), setFocusItem()
*/
void QGraphicsScene::clearFocus()
{
    Q_D(QGraphicsScene);
    if (d->hasFocus) {
        d->hasFocus = false;
        setFocusItem(0, Qt::OtherFocusReason);
    }
}

/*!
    Returns the current mouse grabber item, or 0 if no item is currently
    grabbing the mouse. The mouse grabber item is the item that receives all
    mouse events sent to the scene.

    An item becomes a mouse grabber when it receives and accepts a
    mouse press event, and it stays the mouse grabber until either of
    the following events occur:

    \list
    \o If the item receives a mouse release event when there are no other
    buttons pressed, it loses the mouse grab.
    \o If the item becomes invisible (i.e., someone calls \c {item->setVisible(false))},
    or if it becomes disabled (i.e., someone calls \c {item->setEnabled(false))},
    it loses the mouse grab.
    \o If the item is removed from the scene, it loses the mouse grab.
    \endlist

    If the item loses its mouse grab, the scene will ignore all mouse events
    until a new item grabs the mouse (i.e., until a new item receives a mouse
    press event).
*/
QGraphicsItem *QGraphicsScene::mouseGrabberItem() const
{
    Q_D(const QGraphicsScene);
    return d->mouseGrabberItem;
}

/*!
    \property QGraphicsScene::backgroundBrush
    \brief the background brush of the scene.

    Set this property to changes the scene's background to a different color,
    gradient or texture. The default background brush is Qt::NoBrush. The
    background is drawn before (behind) the items.

    Example:

    \code
        QGraphicsScene scene;
        QGraphicsView view(&scene);
        view.show();

        // a blue background
        scene.setBackgroundBrush(Qt::blue);

        // a gradient background
        QRadialGradient gradient(0, 0, 10);
        gradient.setSpread(QGradient::RepeatSpread);
        scene.setBackgroundBrush(gradient);
    \endcode

    QGraphicsScene::render() calls drawBackground() to draw the scene
    background. For more detailed control over how the background is drawn,
    you can reimplement drawBackground() in a subclass of QGraphicsScene.
*/
QBrush QGraphicsScene::backgroundBrush() const
{
    Q_D(const QGraphicsScene);
    return d->backgroundBrush;
}
void QGraphicsScene::setBackgroundBrush(const QBrush &brush)
{
    Q_D(QGraphicsScene);
    d->backgroundBrush = brush;
    foreach (QGraphicsView *view, d->views) {
        view->resetCachedContent();
        view->viewport()->update();
    }
    update();
}

/*!
    \property QGraphicsScene::foregroundBrush
    \brief the foreground brush of the scene.

    Change this property to set the scene's foreground to a different
    color, gradient or texture.

    The foreground is drawn after (on top of) the items. The default
    foreground brush is Qt::NoBrush ( i.e. the foreground is not
    drawn).

    Example:

    \code
        QGraphicsScene scene;
        QGraphicsView view(&scene);
        view.show();

        // a white semi-transparent foreground
        scene.setForegroundBrush(QColor(255, 255, 255, 127));

        // a grid foreground
        scene.setForegroundBrush(QBrush(Qt::lightGray, Qt::CrossPattern));
    \endcode

    QGraphicsScene::render() calls drawForeground() to draw the scene
    foreground. For more detailed control over how the foreground is
    drawn, you can reimplement the drawForeground() function in a
    QGraphicsScene subclass.
*/
QBrush QGraphicsScene::foregroundBrush() const
{
    Q_D(const QGraphicsScene);
    return d->foregroundBrush;
}
void QGraphicsScene::setForegroundBrush(const QBrush &brush)
{
    Q_D(QGraphicsScene);
    d->foregroundBrush = brush;
    foreach (QGraphicsView *view, views())
        view->viewport()->update();
    update();
}

/*!
    This method is used by input methods to query a set of properties of
    the scene to be able to support complex input method operations as support
    for surrounding text and reconversions.

    The \a query parameter specifies which property is queried.

    \sa QWidget::inputMethodQuery()
*/
QVariant QGraphicsScene::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QGraphicsScene);
    if (!d->focusItem)
        return QVariant();
    const QTransform matrix = d->focusItem->sceneTransform();
    QVariant value = d->focusItem->inputMethodQuery(query);
    if (value.type() == QVariant::RectF)
        value = matrix.mapRect(value.toRectF());
    else if (value.type() == QVariant::PointF)
        value = matrix.map(value.toPointF());
    else if (value.type() == QVariant::Rect)
        value = matrix.mapRect(value.toRect());
    else if (value.type() == QVariant::Point)
        value = matrix.map(value.toPoint());
    return value;
}

/*!
    Schedules a redraw of the area \a rect on the scene.

    \sa sceneRect(), changed()
*/
void QGraphicsScene::update(const QRectF &rect)
{
    Q_D(QGraphicsScene);
    if (d->updateAll)
        return;

    if (rect.isNull()) {
        d->updateAll = true;
        d->updatedRects.clear();
    } else {
        d->updatedRects << rect;
    }
    if (!d->calledEmitUpdated) {
        d->calledEmitUpdated = true;
        QTimer::singleShot(0, this, SLOT(_q_emitUpdated()));
    }
}

/*!
    \fn void QGraphicsScene::update(qreal x, qreal y, qreal w, qreal h)
    \since 4.3

    This convenience function is equivalent to calling update(QRectF(\a x, \a
    y, \a w, \a h));
*/

/*!
    Invalidates and schedules a redraw of the \a layers in \a rect on the
    scene. Any cached content in \a layers is unconditionally invalidated and
    redrawn.

    You can use this function overload to notify QGraphicsScene of changes to
    the background or the foreground of the scene. This function is commonly
    used for scenes with tile-based backgrounds to notify changes when
    QGraphicsView has enabled
    \l{QGraphicsView::CacheBackground}{CacheBackground}.

    Example:

    \code
      QRectF TileScene::rectForTile(int x, int y) const
      {
          // Return the rectangle for the tile at position (x, y).
          return QRectF(x * tileWidth, y * tileHeight, tileWidth, tileHeight);
      }

      void TileScene::setTile(int x, int y, const QPixmap &pixmap)
      {
          // Sets or replaces the tile at position (x, y) with pixmap.
          if (x >= 0 && x < numTilesH && y >= 0 && y < numTilesV) {
              tiles[y][x] = pixmap;
              invalidate(rectForTile(x, y), BackgroundLayer);
          }
      }

      void TileScene::drawBackground(QPainter *painter, const QRectF &exposed)
      {
          // Draws all tiles that intersect the exposed area.
          for (int y = 0; y < numTilesV; ++y) {
              for (int x = 0; x < numTilesH; ++x) {
                  QRectF rect = rectForTile(x, y);
                  if (exposed.intersects(rect))
                      painter->drawPixmap(rect.topLeft(), tiles[y][x]);
              }
          }
      }
    \endcode

    Note that QGraphicsView currently supports background caching only (see
    QGraphicsView::CacheBackground). This function is equivalent to calling
    update() if any layer but BackgroundLayer is passed.

    \sa QGraphicsView::resetCachedContent()
*/
void QGraphicsScene::invalidate(const QRectF &rect, SceneLayers layers)
{
    foreach (QGraphicsView *view, views())
        view->invalidateScene(rect, layers);
    update(rect);
}

/*!
    \fn void QGraphicsScene::invalidate(qreal x, qreal y, qreal w, qreal h, SceneLayers layers)
    \overload
    \since 4.3

    This convenience function is equivalent to calling invalidate(QRectF(\a x, \a
    y, \a w, \a h), \a layers);
*/

/*!
    Returns a list of all the views that display this scene.

    \sa QGraphicsView::scene()
*/
QList <QGraphicsView *> QGraphicsScene::views() const
{
    Q_D(const QGraphicsScene);
    return d->views;
}

/*!
    This slot \e advances the scene by one step, by calling
    QGraphicsItem::advance() for all items on the scene. This is done in two
    phases: in the first phase, all items are notified that the scene is about
    to change, and in the second phase all items are notified that they can
    move. In the first phase, QGraphicsItem::advance() is called passing a
    value of 0 as an argument, and 1 is passed in the second phase.

    \sa QGraphicsItem::advance(), QGraphicsItemAnimation, QTimeLine
*/
void QGraphicsScene::advance()
{
    for (int i = 0; i < 2; ++i) {
        foreach (QGraphicsItem *item, items())
            item->advance(i);
    }
}

/*!
    Processes the event \a event, and dispatches it to the respective
    event handlers.

    In addition to calling the convenience event handlers, this
    function is responsible for converting mouse move events to hover
    events for when there is no mouse grabber item. Hover events are
    delivered directly to items; there is no convenience function for
    them.

    \sa contextMenuEvent(), keyPressEvent(), keyReleaseEvent(),
    mousePressEvent(), mouseMoveEvent(), mouseReleaseEvent(),
    mouseDoubleClickEvent(), focusInEvent(), focusOutEvent()
*/
bool QGraphicsScene::event(QEvent *event)
{
    Q_D(QGraphicsScene);

    switch (event->type()) {
    case QEvent::GraphicsSceneMousePress:
    case QEvent::GraphicsSceneMouseMove:
    case QEvent::GraphicsSceneMouseRelease:
    case QEvent::GraphicsSceneMouseDoubleClick:
    case QEvent::GraphicsSceneHoverEnter:
    case QEvent::GraphicsSceneHoverLeave:
    case QEvent::GraphicsSceneHoverMove:
        // Reset the under-mouse list to ensure that this event gets fresh
        // item-under-mouse data. Be careful about this list; if people delete
        // items from inside event handlers, this list can quickly end up
        // having stale pointers in it. We need to clear it before dispatching
        // events that use it.
        d->cachedItemsUnderMouse.clear();
    default:
        break;
    }

    switch (event->type()) {
    case QEvent::GraphicsSceneDragEnter:
        dragEnterEvent(static_cast<QGraphicsSceneDragDropEvent *>(event));
        break;
    case QEvent::GraphicsSceneDragMove:
        dragMoveEvent(static_cast<QGraphicsSceneDragDropEvent *>(event));
        break;
    case QEvent::GraphicsSceneDragLeave:
        dragLeaveEvent(static_cast<QGraphicsSceneDragDropEvent *>(event));
        break;
    case QEvent::GraphicsSceneDrop:
        dropEvent(static_cast<QGraphicsSceneDragDropEvent *>(event));
        break;
    case QEvent::GraphicsSceneContextMenu:
        contextMenuEvent(static_cast<QGraphicsSceneContextMenuEvent *>(event));
        break;
    case QEvent::KeyPress:
        keyPressEvent(static_cast<QKeyEvent *>(event));
        break;
    case QEvent::KeyRelease:
        keyReleaseEvent(static_cast<QKeyEvent *>(event));
        break;
    case QEvent::GraphicsSceneMouseMove:
        if (d->mouseGrabberItem) {
            mouseMoveEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
        } else {
            QGraphicsSceneMouseEvent *mouseEvent = static_cast<QGraphicsSceneMouseEvent *>(event);
            QGraphicsSceneHoverEvent hover;
            hover.setWidget(mouseEvent->widget());
            hover.setPos(mouseEvent->pos());
            hover.setScenePos(mouseEvent->scenePos());
            hover.setScreenPos(mouseEvent->screenPos());
            if (!d->dispatchHoverEvent(&hover))
                mouseMoveEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
        }
        break;
    case QEvent::GraphicsSceneMousePress:
        mousePressEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
        break;
    case QEvent::GraphicsSceneMouseRelease:
        mouseReleaseEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
        break;
    case QEvent::GraphicsSceneMouseDoubleClick:
        mouseDoubleClickEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
        break;
    case QEvent::GraphicsSceneWheel:
        wheelEvent(static_cast<QGraphicsSceneWheelEvent *>(event));
        break;
    case QEvent::FocusIn:
        focusInEvent(static_cast<QFocusEvent *>(event));
        break;
    case QEvent::FocusOut:
        focusOutEvent(static_cast<QFocusEvent *>(event));
        break;
    case QEvent::GraphicsSceneHoverEnter:
    case QEvent::GraphicsSceneHoverLeave:
    case QEvent::GraphicsSceneHoverMove:
        d->dispatchHoverEvent(static_cast<QGraphicsSceneHoverEvent *>(event));
        break;
    case QEvent::Leave:
        d->leaveScene();
        break;
    case QEvent::GraphicsSceneHelp:
        helpEvent(static_cast<QGraphicsSceneHelpEvent *>(event));
        break;
    case QEvent::InputMethod:
        inputMethodEvent(static_cast<QInputMethodEvent *>(event));
        break;
    case QEvent::Timer:
        if (d->indexTimerId && static_cast<QTimerEvent *>(event)->timerId() == d->indexTimerId) {
            if (d->restartIndexTimer) {
                d->restartIndexTimer = false;
            } else {
                // this call will kill the timer
                d->_q_generateBspTree();
            }
        }
        // Fallthrough intended - support timers in subclasses.
    default:
        return QObject::event(event);
    }
    return true;
}

/*!
    This event handler, for event \a contextMenuEvent, can be reimplemented in
    a subclass to receive context menu events. The default implementation
    forwards the event to the item at the scene position provided by the event.

    \sa QGraphicsItem::contextMenuEvent()
*/
void QGraphicsScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent)
{
    Q_D(QGraphicsScene);
    if (QGraphicsItem *item = itemAt(contextMenuEvent->scenePos())) {
        contextMenuEvent->setPos(item->d_ptr->genericMapFromScene(contextMenuEvent->scenePos(),
                                                                  contextMenuEvent->widget()));
        d->sendEvent(item, contextMenuEvent);
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a subclass
    to receive drag enter events for the scene.

    The default implementation accepts the event and prepares the scene to
    accept drag move events.

    \sa QGraphicsItem::dragEnterEvent(), dragMoveEvent(), dragLeaveEvent(),
    dropEvent()
*/
void QGraphicsScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_D(QGraphicsScene);
    d->dragDropItem = 0;
    d->lastDropAction = Qt::IgnoreAction;
    event->accept();
}

/*!
    This event handler, for event \a event, can be reimplemented in a subclass
    to receive drag move events for the scene.

    \sa QGraphicsItem::dragMoveEvent(), dragEnterEvent(), dragLeaveEvent(),
    dropEvent()
*/
void QGraphicsScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_D(QGraphicsScene);
    event->ignore();

    if (d->mouseGrabberItem) {
        // Mouse grabbers that start drag events lose the mouse grab.
        d->lastMouseGrabberItem = d->mouseGrabberItem;
        d->mouseGrabberItem = 0;
        d->mouseGrabberButtonDownPos.clear();
        d->mouseGrabberButtonDownScenePos.clear();
        d->mouseGrabberButtonDownScreenPos.clear();
    }

    bool eventDelivered = false;

    // Find the topmost enabled items under the cursor. They are all
    // candidates for accepting drag & drop events.
    foreach (QGraphicsItem *item, d->itemsAtPosition(event->screenPos(),
                                                     event->scenePos(),
                                                     event->widget())) {
        if (!item->isEnabled() || !item->acceptDrops())
            continue;

        if (item != d->dragDropItem) {
            // Enter the new drag drop item. If it accepts the event, we send
            // the leave to the parent item.
            QGraphicsSceneDragDropEvent dragEnter(QEvent::GraphicsSceneDragEnter);
            d->cloneDragDropEvent(&dragEnter, event);
            dragEnter.setDropAction(event->proposedAction());
            d->sendDragDropEvent(item, &dragEnter);
            event->setAccepted(dragEnter.isAccepted());
            event->setDropAction(dragEnter.dropAction());
            if (!event->isAccepted()) {
                // Propagate to the item under
                continue;
            }

            d->lastDropAction = event->dropAction();

            if (d->dragDropItem) {
                // Leave the last drag drop item. A perfect implementation
                // would set the position of this event to the point where
                // this event and the last event intersect with the item's
                // shape, but that's not easy to do. :-)
                QGraphicsSceneDragDropEvent dragLeave(QEvent::GraphicsSceneDragLeave);
                d->cloneDragDropEvent(&dragLeave, event);
                d->sendDragDropEvent(d->dragDropItem, &dragLeave);
            }

            // We've got a new drag & drop item
            d->dragDropItem = item;
        }

        // Send the move event.
        event->setDropAction(d->lastDropAction);
        event->accept();
        d->sendDragDropEvent(item, event);
        if (event->isAccepted()) {
            d->lastDropAction = event->dropAction();
            eventDelivered = true;
            break;
        }

        // Propagate
        event->setDropAction(Qt::IgnoreAction);
    }

    if (!eventDelivered) {
        if (d->dragDropItem) {
            // Leave the last drag drop item
            QGraphicsSceneDragDropEvent dragLeave(QEvent::GraphicsSceneDragLeave);
            d->cloneDragDropEvent(&dragLeave, event);
            d->sendDragDropEvent(d->dragDropItem, &dragLeave);
            d->dragDropItem = 0;
        }
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a subclass
    to receive drag leave events for the scene.

    \sa QGraphicsItem::dragLeaveEvent(), dragEnterEvent(), dragMoveEvent(),
    dropEvent()
*/
void QGraphicsScene::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_D(QGraphicsScene);
    if (d->dragDropItem) {
        // Leave the last drag drop item
        d->sendDragDropEvent(d->dragDropItem, event);
        d->dragDropItem = 0;
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a subclass
    to receive drop events for the scene.

    \sa QGraphicsItem::dropEvent(), dragEnterEvent(), dragMoveEvent(),
    dragLeaveEvent()
*/
void QGraphicsScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_UNUSED(event);
    Q_D(QGraphicsScene);
    if (d->dragDropItem) {
        // Drop on the last drag drop item
        d->sendDragDropEvent(d->dragDropItem, event);
        d->dragDropItem = 0;
    }
}

/*!
    This event handler, for event \a focusEvent, can be reimplemented in a
    subclass to receive focus in events.

    The default implementation sets focus on the scene, and then on the last
    focus item.

    \sa QGraphicsItem::focusOutEvent()
*/
void QGraphicsScene::focusInEvent(QFocusEvent *focusEvent)
{
    Q_D(QGraphicsScene);

    d->hasFocus = true;
    if (d->lastFocusItem) {
        // Set focus on the last focus item
        setFocusItem(d->lastFocusItem, focusEvent->reason());
    }
}

/*!
    This event handler, for event \a focusEvent, can be reimplemented in a
    subclass to receive focus out events.

    The default implementation removes focus from any focus item, then removes
    focus from the scene.

    \sa QGraphicsItem::focusInEvent()
*/
void QGraphicsScene::focusOutEvent(QFocusEvent *focusEvent)
{
    Q_D(QGraphicsScene);
    d->hasFocus = false;
    setFocusItem(0, focusEvent->reason());
}

/*!
    This event handler, for event \a helpEvent, can be
    reimplemented in a subclass to receive help events. The events
    are of type QEvent::ToolTip, which are created when a tooltip is
    requested.

    The default implementation shows the tooltip of the topmost
    item, i.e., the item with the highest z-value, at the mouse
    cursor position. If no item has a tooltip set, this function
    does nothing.

   \sa QGraphicsItem::toolTip(), QGraphicsSceneHelpEvent
*/
void QGraphicsScene::helpEvent(QGraphicsSceneHelpEvent *helpEvent)
{
#ifdef QT_NO_TOOLTIP
    Q_UNUSED(helpEvent);
#else
    // Find the first item that does tooltips
    Q_D(QGraphicsScene);
    QList<QGraphicsItem *> itemsAtPos = d->itemsAtPosition(helpEvent->screenPos(),
                                                           helpEvent->scenePos(),
                                                           helpEvent->widget());
    QGraphicsItem *toolTipItem = 0;
    for (int i = 0; i < itemsAtPos.size(); ++i) {
        QGraphicsItem *tmp = itemsAtPos.at(i);
        if (!tmp->toolTip().isEmpty()) {
            toolTipItem = tmp;
            break;
        }
    }

    // Show or hide the tooltip
    QString text;
    QPoint point;
    if (toolTipItem && !toolTipItem->toolTip().isEmpty()) {
        text = toolTipItem->toolTip();
        point = helpEvent->screenPos();
    }
    QToolTip::showText(point, text);
#endif
}

/*!
    This event handler, for event \a hoverEvent, can be reimplemented in a
    subclass to receive hover enter events. The default implementation
    forwards the event to the topmost item that accepts hover events at the
    scene position from the event.

    \sa QGraphicsItem::hoverEvent(), QGraphicsItem::setAcceptsHoverEvents()
*/
bool QGraphicsScenePrivate::dispatchHoverEvent(QGraphicsSceneHoverEvent *hoverEvent)
{
    // Find the first item that accepts hover events, reusing earlier
    // calculated data is possible.
    if (cachedItemsUnderMouse.isEmpty()) {
        cachedItemsUnderMouse = itemsAtPosition(hoverEvent->screenPos(),
                                                hoverEvent->scenePos(),
                                                hoverEvent->widget());
    }

    QGraphicsItem *item = 0;
    for (int i = 0; i < cachedItemsUnderMouse.size(); ++i) {
        QGraphicsItem *tmp = cachedItemsUnderMouse.at(i);
        if (tmp->acceptsHoverEvents()) {
            item = tmp;
            break;
        }
    }

    if (!item) {
        // Send HoverLeave events to all existing hover items, topmost first.
        while (!hoverItems.isEmpty()) {
            QGraphicsItem *lastItem = hoverItems.takeLast();
            if (lastItem->acceptsHoverEvents())
                sendHoverEvent(QEvent::GraphicsSceneHoverLeave, lastItem, hoverEvent);
        }
        return false;
    }

    int itemIndex = hoverItems.indexOf(item);
    if (itemIndex == -1) {
        // Send HoverLeave events to all existing hover items, topmost first.
        while (!hoverItems.isEmpty() && !hoverItems.last()->isAncestorOf(item)) {
            QGraphicsItem *lastItem = hoverItems.takeLast();
            if (lastItem->acceptsHoverEvents())
                sendHoverEvent(QEvent::GraphicsSceneHoverLeave, lastItem, hoverEvent);
        }

        // Item is a child of a known item. Generate enter events for the
        // missing links.
        QList<QGraphicsItem *> parents;
        parents << item;

        QGraphicsItem *parent = item->parentItem();
        while (parent && (hoverItems.isEmpty() || parent != hoverItems.last())) {
            parents.prepend(parent);
            parent = parent->parentItem();
        }
        for (int i = 0; i < parents.size(); ++i) {
            parent = parents.at(i);
            hoverItems << parent;
            sendHoverEvent(QEvent::GraphicsSceneHoverEnter, parent, hoverEvent);
        }
    } else {
        // Known item, generate leave events for any children
        while (hoverItems.size() > itemIndex + 1) {
            QGraphicsItem *child = hoverItems.takeAt(itemIndex + 1);
            sendHoverEvent(QEvent::GraphicsSceneHoverLeave, child, hoverEvent);
        }

        // Generate a move event for the item itself
        sendHoverEvent(QEvent::GraphicsSceneHoverMove, item, hoverEvent);
    }
    return true;
}

/*!
    \internal

    Handles all actions necessary to clean up the scene when the mouse leaves
    the view.
*/
void QGraphicsScenePrivate::leaveScene()
{
    Q_Q(QGraphicsScene);
#ifndef QT_NO_TOOLTIP
    // Remove any tooltips
    QToolTip::showText(QPoint(), QString());
#endif
    // Send HoverLeave events to all existing hover items, topmost first.
    QGraphicsView *senderWidget = qobject_cast<QGraphicsView *>(q->sender());
    QGraphicsSceneHoverEvent hoverEvent;
    hoverEvent.setWidget(senderWidget);

    if (senderWidget) {
        QPoint cursorPos = QCursor::pos();
        hoverEvent.setScenePos(senderWidget->mapToScene(senderWidget->mapFromGlobal(cursorPos)));
        hoverEvent.setScreenPos(cursorPos);
    }

    while (!hoverItems.isEmpty()) {
        QGraphicsItem *lastItem = hoverItems.takeLast();
        if (lastItem->acceptsHoverEvents())
            sendHoverEvent(QEvent::GraphicsSceneHoverLeave, lastItem, &hoverEvent);
    }
}

/*!
    This event handler, for event \a keyEvent, can be reimplemented in a
    subclass to receive keypress events. The default implementation forwards
    the event to current focus item.

    \sa QGraphicsItem::keyPressEvent(), focusItem()
*/
void QGraphicsScene::keyPressEvent(QKeyEvent *keyEvent)
{
    Q_D(QGraphicsScene);
    if (QGraphicsItem *item = focusItem())
        d->sendEvent(item, keyEvent);
    else
        keyEvent->ignore();
}

/*!
    This event handler, for event \a keyEvent, can be reimplemented in a
    subclass to receive key release events. The default implementation
    forwards the event to current focus item.

    \sa QGraphicsItem::keyReleaseEvent(), focusItem()
*/
void QGraphicsScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
    Q_D(QGraphicsScene);
    if (QGraphicsItem *item = focusItem())
        d->sendEvent(item, keyEvent);
    else
        keyEvent->ignore();
}

/*!
    This event handler, for event \a mouseEvent, can be reimplemented
    in a subclass to receive mouse press events for the scene.

    The default implementation depends on the state of the scene. If
    there is a mouse grabber item, then the event is sent to the mouse
    grabber. Otherwise, it is forwarded to the topmost item that
    accepts mouse events at the scene position from the event, and
    that item promptly becomes the mouse grabber item.

    If there is no item at the given position on the scene, the
    selection area is reset, any focus item loses its input focus, and
    the event is then ignored.

    \sa QGraphicsItem::mousePressEvent(),
    QGraphicsItem::setAcceptedMouseButtons()
*/
void QGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    Q_D(QGraphicsScene);
    d->mousePressEventHandler(mouseEvent);
}

/*!
    This event handler, for event \a mouseEvent, can be reimplemented
    in a subclass to receive mouse move events for the scene.

    The default implementation depends on the mouse grabber state. If
    there is a mouse grabber item, the event is sent to the mouse
    grabber; otherwise, the event is ignored.

    \sa QGraphicsItem::mousePressEvent(), QGraphicsItem::mouseReleaseEvent(),
    QGraphicsItem::mouseDoubleClickEvent(), QGraphicsItem::setAcceptedMouseButtons()
*/
void QGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    Q_D(QGraphicsScene);
    if (!d->mouseGrabberItem) {
        mouseEvent->ignore();
        return;
    }

    // Forward the event to the mouse grabber
    d->sendMouseEvent(mouseEvent);
    mouseEvent->accept();
}

/*!
    This event handler, for event \a mouseEvent, can be reimplemented
    in a subclass to receive mouse release events for the scene.

    The default implementation depends on the mouse grabber state.  If
    there is no mouse grabber, the event is ignored.  Otherwise, if
    there is a mouse grabber item, the event is sent to the mouse
    grabber. If this mouse release represents the last pressed button
    on the mouse, the mouse grabber item then loses the mouse grab.

    \sa QGraphicsItem::mousePressEvent(), QGraphicsItem::mouseMoveEvent(),
    QGraphicsItem::mouseDoubleClickEvent(), QGraphicsItem::setAcceptedMouseButtons()
*/
void QGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    Q_D(QGraphicsScene);
    if (!d->mouseGrabberItem) {
        mouseEvent->ignore();
        return;
    }

    // Forward the event to the mouse grabber
    d->sendMouseEvent(mouseEvent);
    mouseEvent->accept();

    // Reset the mouse grabber when the last mouse button has been released.
    if (!mouseEvent->buttons()) {
        d->lastMouseGrabberItem = d->mouseGrabberItem;
        d->mouseGrabberItem = 0;

        // Generate a hoverevent
        QGraphicsSceneHoverEvent hoverEvent;
        hoverEvent.setWidget(mouseEvent->widget());
        hoverEvent.setPos(mouseEvent->pos());
        hoverEvent.setScenePos(mouseEvent->scenePos());
        hoverEvent.setScreenPos(mouseEvent->screenPos());
        QCoreApplication::sendEvent(this, &hoverEvent);
    }
}

/*!
    This event handler, for event \a mouseEvent, can be reimplemented
    in a subclass to receive mouse doubleclick events for the scene.

    If someone doubleclicks on the scene, the scene will first receive
    a mouse press event, followed by a release event (i.e., a click),
    then a doubleclick event, and finally a release event. If the
    doubleclick event is delivered to a different item than the one
    that received the first press and release, it will be delivered as
    a press event. However, tripleclick events are not delivered as
    doubleclick events in this case.

    The default implementation is similar to mousePressEvent().

    \sa QGraphicsItem::mousePressEvent(), QGraphicsItem::mouseMoveEvent(),
    QGraphicsItem::mouseReleaseEvent(), QGraphicsItem::setAcceptedMouseButtons()
*/
void QGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    Q_D(QGraphicsScene);
    d->mousePressEventHandler(mouseEvent);
}

/*!
    This event handler, for event \a wheelEvent, can be reimplemented in a
    subclass to receive mouse wheel events for the scene.

    By default, the event is delivered to the topmost visible item under the
    cursor. If ignored, the event propagates to the item beneath, and again
    until the event is accepted, or it reaches the scene. If no items accept
    the event, it is ignored.

    \sa QGraphicsItem::wheelEvent()
*/
void QGraphicsScene::wheelEvent(QGraphicsSceneWheelEvent *wheelEvent)
{
    Q_D(QGraphicsScene);
    foreach (QGraphicsItem *item, d->itemsAtPosition(wheelEvent->screenPos(),
                                                     wheelEvent->scenePos(),
                                                     wheelEvent->widget())) {
        wheelEvent->setPos(item->d_ptr->genericMapFromScene(wheelEvent->scenePos(),
                                                            wheelEvent->widget()));
        wheelEvent->accept();
        d->sendEvent(item, wheelEvent);
        if (wheelEvent->isAccepted())
            break;
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive input method events for the scene.

    The default implementation forwards the event to the focusItem().
    If no item currently has focus, this function does nothing.

    \sa QGraphicsItem::inputMethodEvent()
*/
void QGraphicsScene::inputMethodEvent(QInputMethodEvent *event)
{
    Q_D(QGraphicsScene);
    if (!d->focusItem)
        return;
    d->sendEvent(d->focusItem, event);
}

/*!
    Draws the background of the scene using \a painter, before any items and
    the foreground are drawn. Reimplement this function to provide a custom
    background for the scene.

    All painting is done in \e scene coordinates. The \a rect
    parameter is the exposed rectangle.

    If all you want is to define a color, texture, or gradient for the
    background, you can call setBackgroundBrush() instead.

    \sa drawForeground(), drawItems()
*/
void QGraphicsScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    Q_D(QGraphicsScene);

    if (d->backgroundBrush.style() != Qt::NoBrush) {
        bool painterStateProtection = d->painterStateProtection(painter);
        if (painterStateProtection)
            painter->save();
        painter->setBrushOrigin(0, 0);
        painter->fillRect(rect, backgroundBrush());
        if (painterStateProtection)
            painter->restore();
    }
}

/*!
    Draws the foreground of the scene using \a painter, after the background
    and all items have been drawn. Reimplement this function to provide a
    custom foreground for the scene.

    All painting is done in \e scene coordinates. The \a rect
    parameter is the exposed rectangle.

    If all you want is to define a color, texture or gradient for the
    foreground, you can call setForegroundBrush() instead.

    \sa drawBackground(), drawItems()
*/
void QGraphicsScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    Q_D(QGraphicsScene);

    if (d->foregroundBrush.style() != Qt::NoBrush) {
        bool painterStateProtection = d->painterStateProtection(painter);
        if (painterStateProtection)
            painter->save();
        painter->setBrushOrigin(0, 0);
        painter->fillRect(rect, foregroundBrush());
        if (painterStateProtection)
            painter->restore();
    }
}

/*!
    Paints the given \a items using the provided \a painter, after the
    background has been drawn, and before the foreground has been
    drawn.  All painting is done in \e scene coordinates. Before
    drawing each item, the painter must be transformed using
    QGraphicsItem::sceneMatrix().

    The \a options parameter is the list of style option objects for
    each item in \a items. The \a numItems parameter is the number of
    items in \a items and options in \a options. The \a widget
    parameter is optional; if specified, it should point to the widget
    that is being painted on.

    The default implementation prepares the painter matrix, and calls
    QGraphicsItem::paint() on all items. Reimplement this function to
    provide custom painting of all items for the scene; gaining
    complete control over how each item is drawn. In some cases this
    can increase drawing performance significantly.

    Example:

    \quotefromfile snippets/graphicssceneadditemsnippet.cpp
    \skipto /::drawItems/
    \printuntil /^\}/

    \sa drawBackground(), drawForeground()
*/
void QGraphicsScene::drawItems(QPainter *painter,
                               int numItems,
                               QGraphicsItem *items[],
                               const QStyleOptionGraphicsItem options[], QWidget *widget)
{
    Q_D(QGraphicsScene);

    // Detect if painter state protection is disabled.
    bool painterStateProtection = d->painterStateProtection(painter);
    QTransform oldTransform = painter->worldTransform();

    for (int i = 0; i < numItems; ++i) {
        // Save painter
        if (painterStateProtection)
            painter->save();

        QGraphicsItem *item = items[i];

        if (item->d_ptr->itemIsUntransformable()) {
            painter->setTransform(item->deviceTransform(painter->worldTransform()), false);
        } else {
            painter->setTransform(item->sceneTransform(), true);
        }

        if (item->flags() & QGraphicsItem::ItemClipsToShape)
            painter->setClipPath(item->shape(), Qt::IntersectClip);
        if (item->d_ptr->ancestorFlags & QGraphicsItemPrivate::AncestorClipsChildren) {
            // Set a clip path on \a painter by walking up the parent item
            // chain of \a item, intersecting clip paths as long as the item's
            // ancestor clips children.
            QGraphicsItem *target = item->parentItem();
            do {
                painter->setClipPath(item->mapFromItem(target, target->shape()), Qt::IntersectClip);
            } while ((target->d_ptr->ancestorFlags & QGraphicsItemPrivate::AncestorClipsChildren)
                     && (target = target->parentItem()));
        }

        item->paint(painter, &options[i], widget);

        // Restore painter
        if (painterStateProtection)
            painter->restore();
        else
            painter->setWorldTransform(oldTransform);
    }
}

/*!
    \fn QGraphicsScene::changed(const QList<QRectF> &region)

    This signal is emitted by QGraphicsScene when control reaches the
    event loop, if the scene content changes. The \a region parameter
    contains a list of scene rectangles that indicate the area that
    has been changed.

    \sa QGraphicsView::updateScene()
*/

/*!
    \fn QGraphicsScene::sceneRectChanged(const QRectF &rect)

    This signal is emitted by QGraphicsScene whenever the scene rect changes.
    The \a rect parameter is the new scene rectangle.

    \sa QGraphicsView::updateSceneRect()
*/

/*!
    \fn QGraphicsScene::selectionChanged()
    \since 4.3

    This signal is emitted by QGraphicsScene whenever the selection
    changes. You can call selectedItems() to get the new list of selected
    items.

    The selection changes whenever an item is selected or unselected, a
    selection area is set, cleared or otherwise changed, if a preselected item
    is added to the scene, or if a selected item is removed from the scene.

    QGraphicsScene emits this signal only once for group selection operations.
    For example, if you set a selection area, select or unselect a
    QGraphicsItemGroup, or if you add or remove from the scene a parent item
    that contains several selected items, selectionChanged() is emitted only
    once after the operation has completed (instead of once for each item).

    \sa setSelectionArea(), selectedItems(), QGraphicsItem::setSelected()
*/

/*!
    \internal

    This private function is called by QGraphicsItem, which is a friend of
    QGraphicsScene. It is used by QGraphicsScene to record the rectangles that
    need updating. It also launches a single-shot timer to ensure that
    updated() will be emitted later.

    The \a item parameter is the item that changed, and \a rect is the
    area of the item that changed given in item coordinates.
*/
void QGraphicsScene::itemUpdated(QGraphicsItem *item, const QRectF &rect)
{
    Q_D(QGraphicsScene);
    QRectF boundingRect = _q_adjustedRect(item->boundingRect());
    if (!rect.isNull())
        boundingRect &= _q_adjustedRect(rect);

    QRectF oldGrowingItemsBoundingRect = d->growingItemsBoundingRect;

    if (item->d_ptr->itemIsUntransformable()) {
        // Update d->largestUntransformableItem by mapping this item's
        // bounding rect back to the topmost untransformable item's
        // untransformed coordinate system (which sort of equals the 1:1
        // coordinate system of an untransformed view).
        QGraphicsItem *parent = item;
        while (parent && (parent->d_ptr->ancestorFlags & QGraphicsItemPrivate::AncestorIgnoresTransformations))
            parent = parent->parentItem();
        d->largestUntransformableItem |= item->mapToItem(parent, item->boundingRect()).boundingRect();

        // Update this item in all views, and compensate for
        // antialiasing. Note: QRect isn't inclusive, so right/bottom need 3
        // "pixels" of compensation.
        foreach (QGraphicsView *view, d->views) {
            QRectF viewportRect = item->deviceTransform(view->viewportTransform()).mapRect(boundingRect);
            update(view->mapToScene(viewportRect.toRect()).boundingRect());
        }
    } else {
        QRectF sceneBoundingRect = item->sceneTransform().mapRect(boundingRect);
        update(sceneBoundingRect);
        d->growingItemsBoundingRect |= sceneBoundingRect;
    }

    if (!d->hasSceneRect && d->growingItemsBoundingRect != oldGrowingItemsBoundingRect)
        emit sceneRectChanged(d->growingItemsBoundingRect);
}

#include "moc_qgraphicsscene.cpp"

#endif // QT_NO_GRAPHICSVIEW
