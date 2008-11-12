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

#include "qdesigner_toolbox_p.h"
#include "qdesigner_command_p.h"
#include "orderdialog_p.h"
#include "promotiontaskmenu_p.h"

#include <QtDesigner/QDesignerFormWindowInterface>

#include <QtGui/QAction>
#include <QtGui/QMenu>

QDesignerToolBox::QDesignerToolBox(QWidget *parent) :
    QToolBox(parent),
    m_actionDeletePage(new QAction(tr("Delete Page"), this)),
    m_actionInsertPage(new QAction(tr("Before Current Page"), this)),
    m_actionInsertPageAfter(new QAction(tr("After Current Page"), this)),
    m_actionChangePageOrder(new QAction(tr("Change Page Order..."), this)),
    m_pagePromotionTaskMenu(new qdesigner_internal::PromotionTaskMenu(0, qdesigner_internal::PromotionTaskMenu::ModeSingleWidget, this))
{
    connect(m_actionDeletePage, SIGNAL(triggered()), this, SLOT(removeCurrentPage()));
    connect(m_actionInsertPage, SIGNAL(triggered()), this, SLOT(addPage()));
    connect(m_actionInsertPageAfter, SIGNAL(triggered()), this, SLOT(addPageAfter()));
    connect(m_actionChangePageOrder, SIGNAL(triggered()), this, SLOT(changeOrder()));
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(slotCurrentChanged(int)));
}

QString QDesignerToolBox::currentItemText() const
{
    return itemText(currentIndex());
}

void QDesignerToolBox::setCurrentItemText(const QString &itemText)
{
    setItemText(currentIndex(), itemText);
}

QString QDesignerToolBox::currentItemName() const
{
    if (currentIndex() == -1)
        return QString();

    return widget(currentIndex())->objectName();
}

void QDesignerToolBox::setCurrentItemName(const QString &itemName)
{
    if (currentIndex() == -1)
        return;

    widget(currentIndex())->setObjectName(itemName);
}

QIcon QDesignerToolBox::currentItemIcon() const
{
    return itemIcon(currentIndex());
}

void QDesignerToolBox::setCurrentItemIcon(const QIcon &itemIcon)
{
    setItemIcon(currentIndex(), itemIcon);
}

QString QDesignerToolBox::currentItemToolTip() const
{
    return itemToolTip(currentIndex());
}

void QDesignerToolBox::setCurrentItemToolTip(const QString &itemToolTip)
{
    setItemToolTip(currentIndex(), itemToolTip);
}

void QDesignerToolBox::removeCurrentPage()
{
    if (currentIndex() == -1 || !widget(currentIndex()))
        return;

    if (QDesignerFormWindowInterface *fw = QDesignerFormWindowInterface::findFormWindow(this)) {
        qdesigner_internal::DeleteToolBoxPageCommand *cmd = new qdesigner_internal::DeleteToolBoxPageCommand(fw);
        cmd->init(this);
        fw->commandHistory()->push(cmd);
    }
}

void QDesignerToolBox::addPage()
{
    if (QDesignerFormWindowInterface *fw = QDesignerFormWindowInterface::findFormWindow(this)) {
        qdesigner_internal::AddToolBoxPageCommand *cmd = new qdesigner_internal::AddToolBoxPageCommand(fw);
        cmd->init(this, qdesigner_internal::AddToolBoxPageCommand::InsertBefore);
        fw->commandHistory()->push(cmd);
    }
}

void QDesignerToolBox::changeOrder()
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

    if (dlg.exec() == QDialog::Accepted)   {
        fw->beginCommand(tr("Change Page Order"));
        for(int i=0; i<wList.count(); ++i) {
            if (wList.at(i) == widget(i))
                continue;
            qdesigner_internal::MoveToolBoxPageCommand *cmd = new qdesigner_internal::MoveToolBoxPageCommand(fw);
            cmd->init(this, wList.at(i), i);
            fw->commandHistory()->push(cmd);
        }
        fw->endCommand();
    }
}

void QDesignerToolBox::addPageAfter()
{
    if (QDesignerFormWindowInterface *fw = QDesignerFormWindowInterface::findFormWindow(this)) {
        qdesigner_internal::AddToolBoxPageCommand *cmd = new qdesigner_internal::AddToolBoxPageCommand(fw);
        cmd->init(this, qdesigner_internal::AddToolBoxPageCommand::InsertAfter);
        fw->commandHistory()->push(cmd);
    }
}

void QDesignerToolBox::itemInserted(int index)
{
    if (count() > 1 && widget(index))
        widget(index)->setBackgroundRole(widget(index>0?0:1)->backgroundRole());
}


QPalette::ColorRole QDesignerToolBox::currentItemBackgroundRole() const
{
    return widget(0) ? widget(0)->backgroundRole() : QPalette::Window;
}

void QDesignerToolBox::setCurrentItemBackgroundRole(QPalette::ColorRole role)
{
    for (int i = 0; i < count(); ++i) {
        QWidget *w = widget(i);
        w->setBackgroundRole(role);
        w->update();
    }
}

void QDesignerToolBox::slotCurrentChanged(int index)
{
    if (widget(index)) {
        if (QDesignerFormWindowInterface *fw = QDesignerFormWindowInterface::findFormWindow(this)) {
            fw->clearSelection();
            fw->selectWidget(this, true);
        }
    }
}

QMenu *QDesignerToolBox::addContextMenuActions(QMenu *popup)
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
    if (count() > 1) {
        popup->addAction(m_actionChangePageOrder);
    }
    popup->addSeparator();
    return pageMenu;
}
