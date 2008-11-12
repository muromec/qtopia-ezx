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

#include "qdesigner_tabwidget_p.h"
#include "qdesigner_command_p.h"
#include "qdesigner_propertycommand_p.h"
#include "promotiontaskmenu_p.h"

#include <QtDesigner/QDesignerFormWindowInterface>

#include <QtGui/QApplication>
#include <QtGui/QTabBar>
#include <QtGui/QAction>
#include <QtGui/QMouseEvent>
#include <QtGui/QMenu>
#include <QtGui/QLabel>

#include <QtCore/qdebug.h>

namespace qdesigner_internal {
// Store tab widget as drag source
class MyMimeData : public QMimeData
{
    Q_OBJECT
public:
    MyMimeData(const QDesignerTabWidget *tab) : m_tab(tab) {}
    static bool fromMyTab(const QMimeData *mimeData, const QDesignerTabWidget *tab) {
        if (!mimeData)
            return false;
        const MyMimeData *m = qobject_cast<const MyMimeData *>(mimeData);
        return m &&  m->m_tab ==  tab;
    }
private:
    const QDesignerTabWidget *m_tab;
};

} // namespace qdesigner_internal

QDesignerTabWidget::QDesignerTabWidget(QWidget *parent) :
    QTabWidget(parent),
    m_dropIndicator(0),
    m_dragPage(0),
    m_mousePressed(false),
    m_actionDeletePage(new QAction(tr("Delete"),  this)),
    m_actionInsertPage(new QAction(tr("Before Current Page"), this)),
    m_actionInsertPageAfter(new QAction(tr("After Current Page"), this)),
    m_pagePromotionTaskMenu(new qdesigner_internal::PromotionTaskMenu(0, qdesigner_internal::PromotionTaskMenu::ModeSingleWidget, this))
{
    tabBar()->setAcceptDrops(true);
    tabBar()->installEventFilter(this);

    connect(m_actionInsertPage, SIGNAL(triggered()), this, SLOT(addPage()));
    connect(m_actionInsertPageAfter, SIGNAL(triggered()), this, SLOT(addPageAfter()));
    connect(m_actionDeletePage, SIGNAL(triggered()), this, SLOT(removeCurrentPage()));
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(slotCurrentChanged(int)));
}

QDesignerTabWidget::~QDesignerTabWidget()
{
}

QString QDesignerTabWidget::currentTabName() const
{
    return currentWidget()
        ? currentWidget()->objectName()
        : QString();
}

void QDesignerTabWidget::setCurrentTabName(const QString &tabName)
{
    if (QWidget *w = currentWidget())
        w->setObjectName(tabName);
}

QString QDesignerTabWidget::currentTabText() const
{
    return tabText(currentIndex());
}

void QDesignerTabWidget::setCurrentTabText(const QString &tabText)
{
    setTabText(currentIndex(), tabText);
}

QString QDesignerTabWidget::currentTabToolTip() const
{
    return tabToolTip(currentIndex());
}

void QDesignerTabWidget::setCurrentTabToolTip(const QString &tabToolTip)
{
    setTabToolTip(currentIndex(), tabToolTip);
}

QIcon QDesignerTabWidget::currentTabIcon() const
{
    return tabIcon(currentIndex());
}

void QDesignerTabWidget::setCurrentTabIcon(const QIcon &tabIcon)
{
    setTabIcon(currentIndex(), tabIcon);
}

bool QDesignerTabWidget::eventFilter(QObject *o, QEvent *e)
{
    if (o != tabBar())
        return false;

    QDesignerFormWindowInterface *fw = formWindow();
    if (!fw)
        return false;

    switch (e->type()) {
    case QEvent::MouseButtonDblClick:
        break;
    case QEvent::MouseButtonPress: {
        QMouseEvent *mev = static_cast<QMouseEvent*>(e);
        if (mev->button() & Qt::LeftButton) {
            m_mousePressed = true;
            m_pressPoint = mev->pos();

            for (int i = 0; i < tabBar()->count(); ++i) {
                if (tabBar()->tabRect(i).contains(m_pressPoint)) {
                    if (i != tabBar()->currentIndex()) {
                        qdesigner_internal::SetPropertyCommand *cmd = new qdesigner_internal::SetPropertyCommand(fw);
                        cmd->init(this, QLatin1String("currentIndex"), i);
                        fw->commandHistory()->push(cmd);
                    }
                    break;
                }
            }
        }
    } break;

    case QEvent::MouseButtonRelease:
        m_mousePressed = false;
        break;

    case QEvent::MouseMove: {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(e);
        if (m_mousePressed && canMove(mouseEvent)) {
            m_mousePressed = false;
            QDrag *drg = new QDrag(this);
            drg->setMimeData(new qdesigner_internal::MyMimeData(this));

            m_dragIndex = currentIndex();
            m_dragPage = currentWidget();
            m_dragLabel = currentTabText();
            m_dragIcon = currentTabIcon();
            if (m_dragIcon.isNull()) {
                QLabel *label = new QLabel(m_dragLabel);
                label->adjustSize();
                drg->setPixmap(QPixmap::grabWidget(label));
                label->deleteLater();
            } else {
                drg->setPixmap(m_dragIcon.pixmap(22, 22));
            }

            removeTab(m_dragIndex);

            const Qt::DropActions dropAction = drg->start(Qt::MoveAction);

            if (dropAction == Qt::IgnoreAction) {
                // abort
                insertTab(m_dragIndex, m_dragPage, m_dragIcon, m_dragLabel);
                setCurrentIndex(m_dragIndex);
            }

            if (m_dropIndicator)
                m_dropIndicator->hide();
        }
    } break;

    case QEvent::DragLeave: {
        if (m_dropIndicator)
            m_dropIndicator->hide();
    } break;

    case QEvent::DragEnter:
    case QEvent::DragMove: {
        QDragMoveEvent *de = static_cast<QDragMoveEvent*>(e);
        if (!qdesigner_internal::MyMimeData::fromMyTab(de->mimeData(), this))
            return false;

        if (de->proposedAction() == Qt::MoveAction)
            de->acceptProposedAction();
        else {
            de->setDropAction(Qt::MoveAction);
            de->accept();
        }

        QRect rect;
        const int index = pageFromPosition(de->pos(), rect);

        if (!m_dropIndicator) {
            m_dropIndicator = new QWidget(this);
            QPalette p = m_dropIndicator->palette();
            p.setColor(backgroundRole(), Qt::red);
            m_dropIndicator->setPalette(p);
        }

        QPoint pos;
        if (index == count())
            pos = tabBar()->mapToParent(QPoint(rect.x() + rect.width(), rect.y()));
        else
            pos = tabBar()->mapToParent(QPoint(rect.x(), rect.y()));

        m_dropIndicator->setGeometry(pos.x(), pos.y() , 3, rect.height());
        m_dropIndicator->show();
    } break;

    case QEvent::Drop: {
        QDropEvent *de = static_cast<QDropEvent*>(e);
        if (!qdesigner_internal::MyMimeData::fromMyTab(de->mimeData(), this))
            return false;
        de->acceptProposedAction();
        de->accept();

        QRect rect;
        const int newIndex = pageFromPosition(de->pos(), rect);

        qdesigner_internal::MoveTabPageCommand *cmd = new qdesigner_internal::MoveTabPageCommand(fw);
        insertTab(m_dragIndex, m_dragPage, m_dragIcon, m_dragLabel);
        cmd->init(this, m_dragPage, m_dragIcon, m_dragLabel, m_dragIndex, newIndex);
        fw->commandHistory()->push(cmd);
    } break;

    default:
        break;
    }

    return false;
}

void QDesignerTabWidget::removeCurrentPage()
{
    if (!currentWidget())
        return;

    if (QDesignerFormWindowInterface *fw = formWindow()) {
        qdesigner_internal::DeleteTabPageCommand *cmd = new qdesigner_internal::DeleteTabPageCommand(fw);
        cmd->init(this);
        fw->commandHistory()->push(cmd);
    }
}

void QDesignerTabWidget::addPage()
{
    if (QDesignerFormWindowInterface *fw = formWindow()) {
        qdesigner_internal::AddTabPageCommand *cmd = new qdesigner_internal::AddTabPageCommand(fw);
        cmd->init(this, qdesigner_internal::AddTabPageCommand::InsertBefore);
        fw->commandHistory()->push(cmd);
    }
}

void QDesignerTabWidget::addPageAfter()
{
    if (QDesignerFormWindowInterface *fw = formWindow()) {
        qdesigner_internal::AddTabPageCommand *cmd = new qdesigner_internal::AddTabPageCommand(fw);
        cmd->init(this, qdesigner_internal::AddTabPageCommand::InsertAfter);
        fw->commandHistory()->push(cmd);
    }
}

bool QDesignerTabWidget::canMove(QMouseEvent *e) const
{
    const QPoint pt = m_pressPoint - e->pos();
    return pt.manhattanLength() > QApplication::startDragDistance();
}

void QDesignerTabWidget::slotCurrentChanged(int index)
{
    if (widget(index)) {
        if (QDesignerFormWindowInterface *fw = formWindow()) {
            fw->clearSelection();
            fw->selectWidget(this, true);
        }
    }
}

QDesignerFormWindowInterface *QDesignerTabWidget::formWindow() const
{
    return QDesignerFormWindowInterface::findFormWindow(const_cast<QDesignerTabWidget*>(this));
}

void QDesignerTabWidget::tabInserted(int index)
{
    QTabWidget::tabInserted(index);

    if (m_actionDeletePage)
        m_actionDeletePage->setEnabled(count() > 1);
}

void QDesignerTabWidget::tabRemoved(int index)
{
    QTabWidget::tabRemoved(index);

    if (m_actionDeletePage)
        m_actionDeletePage->setEnabled(count() > 1);
}

// Get page from mouse position. Default to new page if in right half of last page?
int QDesignerTabWidget::pageFromPosition(const QPoint &pos, QRect &rect) const {
    int index = 0;
    for (; index < count(); index++) {
        const QRect rc = tabBar()->tabRect(index);
        if (rc.contains(pos)) {
            rect = rc;
            break;
        }
    }

    if (index == count() -1) {
        QRect rect2 = rect;
        rect2.setLeft(rect2.left() + rect2.width() / 2);
        if (rect2.contains(pos))
            index++;
    }
    return index;
}

QMenu *QDesignerTabWidget::addContextMenuActions(QMenu *popup)
{
    QMenu *pageMenu = 0;
    if (count()) {
        const QString pageSubMenuLabel = tr("Page %1 of %2").arg(currentIndex() + 1).arg(count());
        pageMenu = popup->addMenu(pageSubMenuLabel);
        pageMenu->addAction(m_actionDeletePage);
        // Set up promotion menu for current widget.
        if (QWidget *page =  currentWidget ()) {
            m_pagePromotionTaskMenu->setWidget(page);
            m_pagePromotionTaskMenu->addActions(QDesignerFormWindowInterface::findFormWindow(this), 
                                                qdesigner_internal::PromotionTaskMenu::SuppressGlobalEdit, 
                                                pageMenu);
        }
    }

    QMenu *insertPageMenu = popup->addMenu(tr("Insert Page"));
    insertPageMenu->addAction(m_actionInsertPageAfter);
    insertPageMenu->addAction(m_actionInsertPage);
    popup->addSeparator();
    return pageMenu;
}

#include "qdesigner_tabwidget.moc" // required for MyMimeData
