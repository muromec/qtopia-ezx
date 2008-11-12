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
TRANSLATOR qdesigner_internal::ListWidgetEditor
*/

#include "listwidgeteditor.h"
#include <findicondialog_p.h>
#include <iconloader_p.h>

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerIconCacheInterface>

#include <QtGui/QComboBox>
#include <QtCore/QDir>
#include <QtCore/qdebug.h>

using namespace qdesigner_internal;

ListWidgetEditor::ListWidgetEditor(QDesignerFormWindowInterface *form, QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    m_form = form;
    ui.deletePixmapItemButton->setIcon(createIconSet(QString::fromUtf8("editdelete.png")));
    ui.deletePixmapItemButton->setEnabled(false);

    QIcon upIcon = createIconSet(QString::fromUtf8("up.png"));
    QIcon downIcon = createIconSet(QString::fromUtf8("down.png"));
    QIcon minusIcon = createIconSet(QString::fromUtf8("minus.png"));
    QIcon plusIcon = createIconSet(QString::fromUtf8("plus.png"));
    ui.moveItemUpButton->setIcon(upIcon);
    ui.moveItemDownButton->setIcon(downIcon);
    ui.newItemButton->setIcon(plusIcon);
    ui.deleteItemButton->setIcon(minusIcon);
}

ListWidgetEditor::~ListWidgetEditor()
{
}

void ListWidgetEditor::fillContentsFromListWidget(QListWidget *listWidget)
{
    setWindowTitle(tr("Edit List Widget"));
    for (int i=0; i<listWidget->count(); ++i) {
        QListWidgetItem *oldItem = listWidget->item(i);
        QListWidgetItem *item = oldItem->clone();
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui.listWidget->addItem(item);
    }

    if (ui.listWidget->count() > 0)
        ui.listWidget->setCurrentRow(0);
    else
        updateEditor();
}

void ListWidgetEditor::fillContentsFromComboBox(QComboBox *comboBox)
{
    setWindowTitle(tr("Edit Combobox"));
    for (int i=0; i<comboBox->count(); ++i) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(comboBox->itemText(i));
        item->setIcon(qVariantValue<QIcon>(comboBox->itemData(i)));
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui.listWidget->addItem(item);
    }

    if (ui.listWidget->count() > 0)
        ui.listWidget->setCurrentRow(0);
    else
        updateEditor();
}

void ListWidgetEditor::on_newItemButton_clicked()
{
    int row = ui.listWidget->currentRow() + 1;

    QListWidgetItem *item = new QListWidgetItem;
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setText(tr("New Item"));
    if (row < ui.listWidget->count())
        ui.listWidget->insertItem(row, item);
    else
        ui.listWidget->addItem(item);

    ui.listWidget->setCurrentItem(item);
    ui.listWidget->editItem(item);
}

void ListWidgetEditor::on_deleteItemButton_clicked()
{
    int row = ui.listWidget->currentRow();

    if (row != -1)
        delete ui.listWidget->takeItem(row);

    if (row == ui.listWidget->count())
        row--;
    if (row < 0)
        updateEditor();
    else
        ui.listWidget->setCurrentRow(row);
}

void ListWidgetEditor::on_moveItemUpButton_clicked()
{
    int row = ui.listWidget->currentRow();
    if (row <= 0)
        return; // nothing to do

    ui.listWidget->insertItem(row - 1, ui.listWidget->takeItem(row));
    ui.listWidget->setCurrentRow(row - 1);
}

void ListWidgetEditor::on_moveItemDownButton_clicked()
{
    int row = ui.listWidget->currentRow();
    if (row == -1 || row == ui.listWidget->count() - 1)
        return; // nothing to do

    ui.listWidget->insertItem(row + 1, ui.listWidget->takeItem(row));
    ui.listWidget->setCurrentRow(row + 1);
}

void ListWidgetEditor::on_listWidget_currentRowChanged(int)
{
    updateEditor();
}

void ListWidgetEditor::on_listWidget_itemChanged(QListWidgetItem *)
{
    updateEditor();
}

void ListWidgetEditor::on_itemTextLineEdit_textEdited(const QString &text)
{
    int currentRow = ui.listWidget->currentRow();
    if (currentRow != -1) {
        QListWidgetItem *item = ui.listWidget->item(currentRow);
        item->setText(text);
    }
}

void ListWidgetEditor::on_deletePixmapItemButton_clicked()
{
    int currentRow = ui.listWidget->currentRow();
    if (currentRow == -1)
        return;
    QListWidgetItem *item = ui.listWidget->item(currentRow);

    item->setIcon(QIcon());
    ui.previewPixmapItemButton->setIcon(QIcon());
    ui.deletePixmapItemButton->setEnabled(false);
}

void ListWidgetEditor::on_previewPixmapItemButton_clicked()
{
    int currentRow = ui.listWidget->currentRow();
    if (currentRow == -1)
        return;
    QListWidgetItem *item = ui.listWidget->item(currentRow);

    FindIconDialog dialog(m_form, this);
    QString file_path;
    QString qrc_path;

    QIcon icon = item->icon();
    if (!icon.isNull()) {
        file_path = m_form->core()->iconCache()->iconToFilePath(icon);
        qrc_path = m_form->core()->iconCache()->iconToQrcPath(icon);
    }

    dialog.setPaths(qrc_path, file_path);
    if (dialog.exec()) {
        file_path = dialog.filePath();
        qrc_path = dialog.qrcPath();
        if (!file_path.isEmpty()) {
            icon = m_form->core()->iconCache()->nameToIcon(file_path, qrc_path);
            item->setIcon(icon);
            ui.previewPixmapItemButton->setIcon(icon);
            ui.deletePixmapItemButton->setEnabled(!icon.isNull());
        }
    }
}

int ListWidgetEditor::count() const
{
    return ui.listWidget->count();
}

QIcon ListWidgetEditor::icon(int row) const
{
    return ui.listWidget->item(row)->icon();
}

QString ListWidgetEditor::text(int row) const
{
    return ui.listWidget->item(row)->text();
}

void ListWidgetEditor::updateEditor()
{
    bool currentItemEnabled = false;

    bool moveRowUpEnabled = false;
    bool moveRowDownEnabled = false;

    QListWidgetItem *item = ui.listWidget->currentItem();
    if (item) {
        currentItemEnabled = true;
        int currentRow = ui.listWidget->currentRow();
        if (currentRow > 0)
            moveRowUpEnabled = true;
        if (currentRow < ui.listWidget->count() - 1)
            moveRowDownEnabled = true;
    }

    ui.moveItemUpButton->setEnabled(moveRowUpEnabled);
    ui.moveItemDownButton->setEnabled(moveRowDownEnabled);
    ui.deleteItemButton->setEnabled(currentItemEnabled);
    ui.textLabel->setEnabled(currentItemEnabled);
    ui.pixmapLabel->setEnabled(currentItemEnabled);
    ui.deletePixmapItemButton->setEnabled(currentItemEnabled);
    ui.previewPixmapItemButton->setEnabled(currentItemEnabled);
    ui.itemTextLineEdit->setEnabled(currentItemEnabled);

    QString itemText;
    QIcon itemIcon;

    if (item) {
        itemText = item->text();
        itemIcon = item->icon();
    }
    int cursorPos = ui.itemTextLineEdit->cursorPosition();
    ui.itemTextLineEdit->setText(itemText);
    ui.itemTextLineEdit->setCursorPosition(cursorPos);
    ui.previewPixmapItemButton->setIcon(itemIcon);
    ui.deletePixmapItemButton->setEnabled(!itemIcon.isNull());
}
