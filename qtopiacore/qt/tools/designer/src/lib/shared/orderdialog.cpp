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
TRANSLATOR qdesigner_internal::OrderDialog
*/

#include "orderdialog_p.h"
#include <QtDesigner/QDesignerFormWindowInterface>

using namespace qdesigner_internal;

OrderDialog::OrderDialog(QDesignerFormWindowInterface *form, QWidget *parent)
    : QDialog(parent), m_pages(0)
{
    ui.setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_form = form;

    ui.upButton->setEnabled(false);
    ui.downButton->setEnabled(false);
}

OrderDialog::~OrderDialog()
{
}

void OrderDialog::setPageList(QList<QWidget*> *pages)
{
    m_pages = pages;
    for (int i=0; i<pages->count(); ++i) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(tr("Index %1 (%2)").arg(i).arg(pages->at(i)->objectName()));
        QVariant v;
        qVariantSetValue<QWidget*>(v, pages->at(i));
        item->setData(Qt::UserRole, v);
        ui.pageList->addItem(item);
    }

    if (ui.pageList->count() > 0)
        ui.pageList->setCurrentRow(0);
}

void OrderDialog::accept()
{
    m_pages->clear();
    for (int i=0; i<ui.pageList->count(); ++i)
    {
        if (QWidget *w = qvariant_cast<QWidget*>(ui.pageList->item(i)->data(Qt::UserRole)))
            m_pages->append(w);
    }
    QDialog::accept();
}

void OrderDialog::on_upButton_clicked()
{
    int row = ui.pageList->currentRow();
    if (row <= 0)
        return;

    ui.pageList->insertItem(row - 1, ui.pageList->takeItem(row));
    ui.pageList->setCurrentRow(row - 1);
}

void OrderDialog::on_downButton_clicked()
{
    int row = ui.pageList->currentRow();
    if (row == -1 || row == ui.pageList->count() - 1)
        return;

    ui.pageList->insertItem(row + 1, ui.pageList->takeItem(row));
    ui.pageList->setCurrentRow(row + 1);
}

void OrderDialog::on_pageList_currentRowChanged(int)
{
    ui.upButton->setEnabled(true);
    ui.downButton->setEnabled(true);
}
