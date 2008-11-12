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

#include "qdesigner_stackedbox_p.h"
#include "qdesigner_command_p.h"
#include "qdesigner_propertycommand_p.h"
#include "orderdialog_p.h"
#include "promotiontaskmenu_p.h"

#include <QtDesigner/QDesignerFormWindowInterface>

#include <QtGui/QToolButton>
#include <QtGui/QAction>
#include <QtGui/qevent.h>
#include <QtGui/QMenu>
#include <QtCore/qdebug.h>

namespace {
     QToolButton *createToolButton(QWidget *parent, Qt::ArrowType at, const QString &name) {
         QToolButton *rc =  new QToolButton();
         rc->setAttribute(Qt::WA_NoChildEventsForParent, true);
         rc->setParent(parent);
         rc->setObjectName(name);
         rc->setArrowType(at);
         rc->setAutoRaise(true);
         rc->setContextMenuPolicy(Qt::PreventContextMenu);
         rc->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
         rc->setFixedSize(QSize(15, 15));
         return rc;
     }
}

QDesignerStackedWidget::QDesignerStackedWidget(QWidget *parent) :
    QStackedWidget(parent),
    m_prev(createToolButton(this, Qt::LeftArrow,  QLatin1String("__qt__passive_prev"))),
    m_next(createToolButton(this, Qt::RightArrow, QLatin1String("__qt__passive_next"))),
    m_actionPreviousPage(new QAction(tr("Previous Page"), this)),
    m_actionNextPage(new QAction(tr("Next Page"), this)),
    m_actionDeletePage(new QAction(tr("Delete"), this)),
    m_actionInsertPage(new QAction(tr("Before Current Page"), this)),
    m_actionInsertPageAfter(new QAction(tr("After Current Page"), this)),
    m_actionChangePageOrder(new QAction(tr("Change Page Order..."), this)),
    m_pagePromotionTaskMenu(new qdesigner_internal::PromotionTaskMenu(0, qdesigner_internal::PromotionTaskMenu::ModeSingleWidget, this))
{
    connect(m_prev, SIGNAL(clicked()), this, SLOT(prevPage()));
    connect(m_next, SIGNAL(clicked()), this, SLOT(nextPage()));

    updateButtons();

    connect(m_actionPreviousPage, SIGNAL(triggered()), this, SLOT(prevPage()));
    connect(m_actionNextPage, SIGNAL(triggered()), this, SLOT(nextPage()));
    connect(m_actionDeletePage, SIGNAL(triggered()), this, SLOT(removeCurrentPage()));
    connect(m_actionInsertPage, SIGNAL(triggered()), this, SLOT(addPage()));
    connect(m_actionInsertPageAfter, SIGNAL(triggered()), this, SLOT(addPageAfter()));
    connect(m_actionChangePageOrder, SIGNAL(triggered()), this, SLOT(changeOrder()));

    connect(this, SIGNAL(currentChanged(int)), this, SLOT(slotCurrentChanged(int)));
}

void QDesignerStackedWidget::removeCurrentPage()
{
    if (currentIndex() == -1)
        return;

    if (QDesignerFormWindowInterface *fw = QDesignerFormWindowInterface::findFormWindow(this)) {
        qdesigner_internal::DeleteStackedWidgetPageCommand *cmd = new qdesigner_internal::DeleteStackedWidgetPageCommand(fw);
        cmd->init(this);
        fw->commandHistory()->push(cmd);
    }
}

void QDesignerStackedWidget::changeOrder()
{
    QDesignerFormWindowInterface *fw = QDesignerFormWindowInterface::findFormWindow(this);

    if (!fw)
        return;

    qdesigner_internal::OrderDialog dlg(fw, fw);

    QList<QWidget*> wList;
    for(int i=0; i<count(); ++i) {
        wList.append(widget(i));
    }
    dlg.setPageList(&wList);

    if (dlg.exec() == QDialog::Accepted)
    {
        fw->beginCommand(tr("Change Page Order"));
        for(int i=0; i<wList.count(); ++i) {
            if (wList.at(i) == widget(i))
                continue;
            qdesigner_internal::MoveStackedWidgetCommand *cmd = new qdesigner_internal::MoveStackedWidgetCommand(fw);
            cmd->init(this, wList.at(i), i);
            fw->commandHistory()->push(cmd);
        }
        fw->endCommand();
    }
}

void QDesignerStackedWidget::addPage()
{
    if (QDesignerFormWindowInterface *fw = QDesignerFormWindowInterface::findFormWindow(this)) {
        qdesigner_internal::AddStackedWidgetPageCommand *cmd = new qdesigner_internal::AddStackedWidgetPageCommand(fw);
        cmd->init(this, qdesigner_internal::AddStackedWidgetPageCommand::InsertBefore);
        fw->commandHistory()->push(cmd);
    }
}

void QDesignerStackedWidget::addPageAfter()
{
    if (QDesignerFormWindowInterface *fw = QDesignerFormWindowInterface::findFormWindow(this)) {
        qdesigner_internal::AddStackedWidgetPageCommand *cmd = new qdesigner_internal::AddStackedWidgetPageCommand(fw);
        cmd->init(this, qdesigner_internal::AddStackedWidgetPageCommand::InsertAfter);
        fw->commandHistory()->push(cmd);
    }
}

void QDesignerStackedWidget::updateButtons()
{
    if (m_prev) {
        m_prev->move(width() - 31, 1);
        m_prev->show();
        m_prev->raise();
    }

    if (m_next) {
        m_next->move(width() - 16, 1);
        m_next->show();
        m_next->raise();
    }
}

void QDesignerStackedWidget::prevPage()
{
    if (count() > 1) {
        int newIndex = currentIndex() - 1;
        if (newIndex < 0)
            newIndex = count() - 1;
        gotoPage(newIndex);
    }
}

void QDesignerStackedWidget::nextPage()
{
    if (count() > 1)
        gotoPage((currentIndex() + 1) % count());
}

bool QDesignerStackedWidget::event(QEvent *e)
{
    if (e->type() == QEvent::LayoutRequest) {
        if (m_actionDeletePage)
            m_actionDeletePage->setEnabled(count() > 1);
        updateButtons();
    }

    return QStackedWidget::event(e);
}

void QDesignerStackedWidget::childEvent(QChildEvent *e)
{
    QStackedWidget::childEvent(e);
    updateButtons();
}

void QDesignerStackedWidget::resizeEvent(QResizeEvent *e)
{
    QStackedWidget::resizeEvent(e);
    updateButtons();
}

void QDesignerStackedWidget::showEvent(QShowEvent *e)
{
    QStackedWidget::showEvent(e);
    updateButtons();
}

QString QDesignerStackedWidget::currentPageName() const
{
    if (currentIndex() == -1)
        return QString();

    return widget(currentIndex())->objectName();
}

void QDesignerStackedWidget::setCurrentPageName(const QString &pageName)
{
    if (currentIndex() == -1)
        return;

    if (QWidget *w = widget(currentIndex())) {
        w->setObjectName(pageName);
    }
}

void QDesignerStackedWidget::slotCurrentChanged(int index)
{
    if (widget(index)) {
        if (QDesignerFormWindowInterface *fw = QDesignerFormWindowInterface::findFormWindow(this)) {
            fw->clearSelection();
            fw->selectWidget(this, true);
        }
    }
}

void QDesignerStackedWidget::gotoPage(int page) {
    // Are we on a form or in a preview?
    if (QDesignerFormWindowInterface *fw = QDesignerFormWindowInterface::findFormWindow(this)) {
        qdesigner_internal::SetPropertyCommand *cmd = new  qdesigner_internal::SetPropertyCommand(fw);
        cmd->init(this, QLatin1String("currentIndex"), page);
        fw->commandHistory()->push(cmd);
        fw->emitSelectionChanged(); // Magically prevent an endless loop triggered by auto-repeat.
    } else {
        setCurrentIndex(page);
    }
    updateButtons();
}

QMenu *QDesignerStackedWidget::addContextMenuActions(QMenu *popup) 
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
    popup->addAction(m_actionNextPage);
    popup->addAction(m_actionPreviousPage);
    if (count() > 1) {
        popup->addAction(m_actionChangePageOrder);
    }
    popup->addSeparator();
    return pageMenu;
}
