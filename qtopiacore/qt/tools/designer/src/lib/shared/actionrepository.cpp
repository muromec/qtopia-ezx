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

#include "actionrepository_p.h"
#include "resourcemimedata_p.h"

#include <QtGui/QDrag>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QToolButton>
#include <QtGui/QPixmap>
#include <QtGui/QAction>
#include <QtGui/qevent.h>

Q_DECLARE_METATYPE(QAction*)
Q_DECLARE_METATYPE(QListWidgetItem*)

static inline QAction *actionOfItem(const QListWidgetItem* item)
{
    return qvariant_cast<QAction*>(item->data(qdesigner_internal::ActionRepository::ActionRole));
}

namespace qdesigner_internal {

ActionRepository::ActionRepository(QWidget *parent)
    : QListWidget(parent)
{
    setViewMode(IconMode);
    setMovement(Static);
    setResizeMode(Adjust);
    setIconSize(QSize(24, 24));
    setSpacing(iconSize().width() / 3);
    setTextElideMode(Qt::ElideMiddle);

    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode (DragDrop);
}

void ActionRepository::startDrag(Qt::DropActions supportedActions)
{
    if (!selectionModel())
        return;

    const QModelIndexList indexes = selectionModel()->selectedIndexes();

    if (indexes.count() > 0) {
        QDrag *drag = new QDrag(this);
        if (QListWidgetItem *litem = itemFromIndex(indexes.front()))
            if (QAction *action = actionOfItem(litem))
                 drag->setPixmap(ActionRepositoryMimeData::actionDragPixmap(action));
        drag->setMimeData(model()->mimeData(indexes));
        drag->start(supportedActions);
    }
}

bool ActionRepository::event (QEvent * event )
{
    if (movement() != Static)
        return  QListWidget::event(event);

    // Static movement turns off drag handling, unfortunately.
    // We need to dispatch ourselves.
    switch (event->type()) {
    case QEvent::DragEnter:
        dragEnterEvent(static_cast<QDragEnterEvent *>(event));
        return true;
    case QEvent::DragMove:
        dragMoveEvent(static_cast<QDragMoveEvent *>(event));
        return true;
    case QEvent::Drop:
        dropEvent(static_cast<QDropEvent *>(event));
        return true;
    default:
        break;
    }
    return QListWidget::event(event);
}


void ActionRepository::dragEnterEvent(QDragEnterEvent *event)
{
    // We can not override the mime types of the model,
    // so we do our own checking
    if (ResourceMimeData::isResourceMimeData(event->mimeData(), ResourceMimeData::Image))
        event->acceptProposedAction();
    else
        event->ignore();
}

void  ActionRepository::dragMoveEvent(QDragMoveEvent *event)
{
    if (ResourceMimeData::isResourceMimeData(event->mimeData(), ResourceMimeData::Image))
        event->acceptProposedAction();
    else
        event->ignore();
}

bool ActionRepository::dropMimeData(int index, const QMimeData * data, Qt::DropAction action )
{
    if (action != Qt::CopyAction)
        return false;

    QListWidgetItem *droppedItem = item(index);
    if (!droppedItem)
        return false;

    ResourceMimeData md;
    if (!md.fromMimeData(data) || md.type() != ResourceMimeData::Image)
        return false;

    emit resourceImageDropped(md, actionOfItem(droppedItem));
    return true;
}

QMimeData *ActionRepository::mimeData(const QList<QListWidgetItem*> items) const
{
    ActionRepositoryMimeData::ActionList actionList;

    foreach (QListWidgetItem *item, items)
         actionList += actionOfItem(item);

    return new ActionRepositoryMimeData(actionList, Qt::CopyAction);
}

void ActionRepository::filter(const QString &text)
{
    const QSet<QListWidgetItem*> visibleItems = QSet<QListWidgetItem*>::fromList(findItems(text, Qt::MatchContains));
    for (int index=0; index<count(); ++index) {
        QListWidgetItem *i = item(index);
        setItemHidden(i, !visibleItems.contains(i));
    }
}

void ActionRepository::focusInEvent(QFocusEvent *event)
{
    QListWidget::focusInEvent(event);
    if (currentItem()) {
        emit currentItemChanged(currentItem(), currentItem());
    }
}

void ActionRepository::contextMenuEvent(QContextMenuEvent *event)
{
    emit contextMenuRequested(event, itemAt(event->pos()));
}

// ----------     ActionRepositoryMimeData
ActionRepositoryMimeData::ActionRepositoryMimeData(QAction *a, Qt::DropAction dropAction) :
    m_dropAction(dropAction)
{
    m_actionList += a;
}

ActionRepositoryMimeData::ActionRepositoryMimeData(const ActionList &al, Qt::DropAction dropAction) :
    m_dropAction(dropAction),
    m_actionList(al)
{
}

QStringList ActionRepositoryMimeData::formats() const
{
    return QStringList(QLatin1String("action-repository/actions"));
}

QPixmap  ActionRepositoryMimeData::actionDragPixmap(const QAction *action)
{

    // Try to find a suitable pixmap. Grab either widget or icon.
    const QIcon icon = action->icon();
    if (!icon.isNull())
        return icon.pixmap(QSize(22, 22));

    foreach (QWidget *w, action->associatedWidgets())
        if (QToolButton *tb = qobject_cast<QToolButton *>(w))
            return QPixmap::grabWidget(tb);

    // Create a QToolButton
    QToolButton *tb = new QToolButton;
    tb->setText(action->text());
    tb->adjustSize();
    tb->setToolButtonStyle (Qt::ToolButtonTextOnly);
    const QPixmap rc = QPixmap::grabWidget(tb);
    tb->deleteLater();
    return rc;
}

void ActionRepositoryMimeData::accept(QDragMoveEvent *event) const
{
    if (event->proposedAction() == m_dropAction) {
        event->acceptProposedAction();
    } else {
        event->setDropAction(m_dropAction);
        event->accept();
    }
}
} // namespace qdesigner_internal
