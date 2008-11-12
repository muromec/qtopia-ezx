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
TRANSLATOR qdesigner_internal::Sentinel
*/

#include "qdesigner_toolbar_p.h"
#include "qdesigner_command_p.h"
#include "actionrepository_p.h"
#include "actionprovider_p.h"
#include "qdesigner_utils_p.h"
#include "qdesigner_objectinspector_p.h"

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerPropertyEditorInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <actionprovider_p.h>
#include <QtDesigner/QExtensionManager>
#include <QtDesigner/QDesignerWidgetFactoryInterface>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QToolButton>
#include <QtGui/QToolBar>
#include <QtGui/QMenu>
#include <QtGui/qevent.h>
#include <QtGui/private/qtoolbarlayout_p.h>

Q_DECLARE_METATYPE(QAction*)

namespace {
    typedef QList<QAction*> ActionList;
}

namespace qdesigner_internal {
// ------------------- ToolBarEventFilter
void ToolBarEventFilter::install(QToolBar *tb)
{
    ToolBarEventFilter *tf = new ToolBarEventFilter(tb);
    tb->installEventFilter(tf);
    tb->setAcceptDrops(true); // ### fake
}

ToolBarEventFilter::ToolBarEventFilter(QToolBar *tb) :
    QObject(tb),
    m_toolBar(tb),
    m_sentinel(0)
{
    QWidget *w = new QWidget(tb);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    w->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    w->setFocusPolicy(Qt::NoFocus);
    m_sentinel = tb->addWidget(w);
    tb->addAction(m_sentinel);
}

bool ToolBarEventFilter::eventFilter (QObject *watched, QEvent *event)
{
    if (watched != m_toolBar)
        return QObject::eventFilter (watched, event);

    switch (event->type()) {
    case QEvent::ChildAdded: {
        // Children should not interact with the mouse
        const QChildEvent *ce = static_cast<const QChildEvent *>(event);
        if (QWidget *w = qobject_cast<QWidget *>(ce->child())) {
            w->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            w->setFocusPolicy(Qt::NoFocus);
        }
    }
        break;
    case QEvent::ContextMenu:
        return handleContextMenuEvent(static_cast<QContextMenuEvent*>(event));
    case QEvent::DragEnter:
    case QEvent::DragMove:
        return handleDragEnterMoveEvent(static_cast<QDragMoveEvent *>(event));
    case QEvent::DragLeave:
        return handleDragLeaveEvent(static_cast<QDragLeaveEvent *>(event));
    case QEvent::Drop:
        return handleDropEvent(static_cast<QDropEvent *>(event));
    case QEvent::MouseButtonPress:
        return handleMousePressEvent(static_cast<QMouseEvent*>(event));
    case QEvent::MouseButtonRelease:
        return handleMouseReleaseEvent(static_cast<QMouseEvent*>(event));
    case QEvent::MouseMove:
        return handleMouseMoveEvent(static_cast<QMouseEvent*>(event));
    default:
        break;
    }
    return QObject::eventFilter (watched, event);
}

int  ToolBarEventFilter::findAction(const QPoint &pos) const
{
    // Default to sentinel action if out of geometry
    const int index = actionIndexAt(m_toolBar, pos);
    return index == -1 ? 0 : index;
}

bool ToolBarEventFilter::handleContextMenuEvent(QContextMenuEvent * event )
{
    event->accept();

    const int index = findAction(m_toolBar->mapFromGlobal(event->globalPos()));
    QAction *action = m_toolBar->actions().at(index);
    QVariant itemData;
    qVariantSetValue(itemData, action);

    QMenu menu(0);

    QAction *newSeperatorAct = menu.addAction(tr("Insert Separator"));
    newSeperatorAct->setData(itemData);
    connect(newSeperatorAct, SIGNAL(triggered()), this, SLOT(slotInsertSeparator()));
    menu.addSeparator();

    if (action && action != m_sentinel) {
        QAction *a = menu.addAction(tr("Remove action '%1'").arg(action->objectName()));
        a->setData(itemData);
        connect(a, SIGNAL(triggered()), this, SLOT(slotRemoveSelectedAction()));
    }

    QAction *remove_toolbar = menu.addAction(tr("Remove Toolbar '%1'").arg(m_toolBar->objectName()));
    connect(remove_toolbar, SIGNAL(triggered()), this, SLOT(slotRemoveToolBar()));

    menu.exec(event->globalPos());
    return true;
}

void ToolBarEventFilter::slotRemoveSelectedAction()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;

    QAction *a = qvariant_cast<QAction*>(action->data());
    Q_ASSERT(a != 0);

    QDesignerFormWindowInterface *fw = formWindow();
    Q_ASSERT(fw);

    const ActionList actions = m_toolBar->actions();
    const int pos = actions.indexOf(a);
    QAction *action_before = 0;
    if (pos != -1 && actions.count() > pos + 1)
        action_before = actions.at(pos + 1);

    RemoveActionFromCommand *cmd = new RemoveActionFromCommand(fw);
    cmd->init(m_toolBar, a, action_before);
    fw->commandHistory()->push(cmd);
}

void ToolBarEventFilter::slotRemoveToolBar()
{
    QDesignerFormWindowInterface *fw = formWindow();
    Q_ASSERT(fw);
    DeleteToolBarCommand *cmd = new DeleteToolBarCommand(fw);
    cmd->init(m_toolBar);
    fw->commandHistory()->push(cmd);
}

void ToolBarEventFilter::slotInsertSeparator()
{
    QDesignerFormWindowInterface *fw = formWindow();
    QAction *theSender = qobject_cast<QAction*>(sender());
    QAction *previous = qvariant_cast<QAction *>(theSender->data());
    fw->beginCommand(tr("Insert Separator"));
    QAction *action = createAction(fw, QLatin1String("separator"), true);
    InsertActionIntoCommand *cmd = new InsertActionIntoCommand(fw);
    cmd->init(m_toolBar, action, previous);
    fw->commandHistory()->push(cmd);
    fw->endCommand();
}

QDesignerFormWindowInterface *ToolBarEventFilter::formWindow() const
{
    return QDesignerFormWindowInterface::findFormWindow(m_toolBar);
}

QAction *ToolBarEventFilter::createAction(QDesignerFormWindowInterface *fw, const QString &objectName, bool separator)
{
    QAction *action = new QAction(fw);
    fw->core()->widgetFactory()->initialize(action);
    if (separator)
        action->setSeparator(true);

    action->setObjectName(objectName);
    fw->ensureUniqueObjectName(action);

    qdesigner_internal::AddActionCommand *cmd = new  qdesigner_internal::AddActionCommand(fw);
    cmd->init(action);
    fw->commandHistory()->push(cmd);

    return action;
}

void ToolBarEventFilter::adjustDragIndicator(const QPoint &pos)
{
    if (QDesignerFormWindowInterface *fw = formWindow()) {
        QDesignerFormEditorInterface *core = fw->core();
        if (QDesignerActionProviderExtension *a = qt_extension<QDesignerActionProviderExtension*>(core->extensionManager(), m_toolBar))
            a->adjustIndicator(pos);
    }
}

void ToolBarEventFilter::hideDragIndicator()
{
    adjustDragIndicator(QPoint(-1, -1));
}

void ToolBarEventFilter::adjustSpecialActions(QToolBar *tb)
{
    if (ToolBarEventFilter *tef = qFindChild<ToolBarEventFilter *>(tb))
        tef->positionSentinel();
}

void ToolBarEventFilter::positionSentinel()
{
    // Make sure dummy is last
    m_toolBar->removeAction(m_sentinel);
    m_toolBar->addAction(m_sentinel);
}

bool ToolBarEventFilter::handleMousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || withinHandleArea(m_toolBar, event->pos()))
        return false;

    if (QDesignerFormWindowInterface *fw = formWindow()) {
        QDesignerFormEditorInterface *core = fw->core();
        // Keep selection in sync
        fw->clearSelection(false);
        if (QDesignerObjectInspector *oi = qobject_cast<QDesignerObjectInspector *>(core->objectInspector())) {
            oi->clearSelection();
            oi->selectObject(m_toolBar);
        }
        core->propertyEditor()->setObject(m_toolBar);
    }
    m_startPosition = m_toolBar->mapFromGlobal(event->globalPos());
    event->accept();
    return true;
}

bool ToolBarEventFilter::handleMouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || m_startPosition.isNull() || withinHandleArea(m_toolBar, event->pos()))
        return false;

    // Accept the event, otherwise, form window selection will trigger
    m_startPosition = QPoint();
    event->accept();
    return true;
}

bool ToolBarEventFilter::handleMouseMoveEvent(QMouseEvent *event)
{
    if (m_startPosition.isNull() || withinHandleArea(m_toolBar, event->pos()))
        return false;

    const QPoint pos = m_toolBar->mapFromGlobal(event->globalPos());
    if ((pos - m_startPosition).manhattanLength() > qApp->startDragDistance()) {
        startDrag(m_startPosition, event->modifiers());
        m_startPosition = QPoint();
        event->accept();
        return true;
    }
    return false;
}

bool ToolBarEventFilter::handleDragEnterMoveEvent(QDragMoveEvent *event)
{
    const ActionRepositoryMimeData *d = qobject_cast<const ActionRepositoryMimeData*>(event->mimeData());
    if (!d)
        return false;

    if (d->actionList().isEmpty()) {
        event->ignore();
        hideDragIndicator();
        return true;
    }

    QAction *action = d->actionList().first();
    if (!action || action->menu() || m_toolBar->actions().contains(action) || !Utils::isObjectAncestorOf(formWindow()->mainContainer(), action)) {
        event->ignore();
        hideDragIndicator();
        return true;
    }

    d->accept(event);
    adjustDragIndicator(event->pos());
    return true;
}

bool ToolBarEventFilter::handleDragLeaveEvent(QDragLeaveEvent *)
{
    hideDragIndicator();
    return false;
}

bool ToolBarEventFilter::handleDropEvent(QDropEvent *event)
{
    const ActionRepositoryMimeData *d = qobject_cast<const ActionRepositoryMimeData*>(event->mimeData());
    if (!d)
        return false;

    if (d->actionList().isEmpty()) {
        event->ignore();
        hideDragIndicator();
        return true;
    }

    QAction *action = d->actionList().first();

    const ActionList actions = m_toolBar->actions();
    if (!action || actions.contains(action)) {
        event->ignore();
        hideDragIndicator();
        return true;
    }

    event->acceptProposedAction();

    int index = findAction(event->pos());
    index = qMin(index, actions.count() - 1);

    QDesignerFormWindowInterface *fw = formWindow();
    InsertActionIntoCommand *cmd = new InsertActionIntoCommand(fw);
    cmd->init(m_toolBar, action, actions.at(index));
    fw->commandHistory()->push(cmd);
    hideDragIndicator();
    return true;
}

void ToolBarEventFilter::startDrag(const QPoint &pos, Qt::KeyboardModifiers modifiers)
{
    const int index = findAction(pos);
    const ActionList actions = m_toolBar->actions();
    if (index == actions.count() - 1)
        return;

    QAction *action = actions.at(index);
    QDesignerFormWindowInterface *fw = formWindow();

    const Qt::DropAction dropAction = (modifiers & Qt::ControlModifier) ? Qt::CopyAction : Qt::MoveAction;
    if (dropAction == Qt::MoveAction) {
        RemoveActionFromCommand *cmd = new RemoveActionFromCommand(fw);
        cmd->init(m_toolBar, action, actions.at(index + 1));
        fw->commandHistory()->push(cmd);
    }

    QDrag *drag = new QDrag(m_toolBar);
    drag->setPixmap(ActionRepositoryMimeData::actionDragPixmap( action));
    drag->setMimeData(new ActionRepositoryMimeData(action, dropAction));

    if (drag->start(dropAction) == Qt::IgnoreAction) {
        hideDragIndicator();
        if (dropAction == Qt::MoveAction) {
            QAction *previous = m_toolBar->actions().at(index);
            InsertActionIntoCommand *cmd = new InsertActionIntoCommand(fw);
            cmd->init(m_toolBar, action, previous);
            fw->commandHistory()->push(cmd);
        }
    }
}

int ToolBarEventFilter::actionIndexAt(const QToolBar *tb, const QPoint &pos)
{
    const ActionList actionList = tb->actions();
    if (const  int size = actionList.size()) {
        for (int  i = 0; i < size; i++) {
            QRect g = tb->actionGeometry(actionList.at(i));
            g.setTopLeft(QPoint(0, 0));
            if (g.contains(pos))
                return i;
        }
    }
    return -1;
}

QAction *ToolBarEventFilter::actionAt(const QToolBar *tb, const QPoint &pos)
{
    const int index = actionIndexAt(tb, pos);
    if (index == -1)
        return 0;
    return tb->actions().at(index);
}

bool ToolBarEventFilter::withinHandleArea(const QToolBar *tb, const QPoint &pos)
{
    if (QToolBarLayout *tbl = qobject_cast<QToolBarLayout *>(tb->layout()))
        return tbl->handleRect().contains(pos);
    return false;
}

// ------------------- SentinelAction

SentinelAction::SentinelAction(QWidget *widget)
    : QAction(widget)
{
    setSeparator(true);
}

SentinelAction::~SentinelAction()
{
}

// ------------------- Sentinel
Sentinel::Sentinel(QWidget *widget)
    : QToolButton(widget)
{
    setObjectName(QString::fromUtf8("__qt__passive_new"));
    setText(QLatin1String(">>"));
    setToolButtonStyle(Qt::ToolButtonTextOnly);
    setToolTip(tr("New Tool Bar"));
}

Sentinel::~Sentinel()
{
}
}
