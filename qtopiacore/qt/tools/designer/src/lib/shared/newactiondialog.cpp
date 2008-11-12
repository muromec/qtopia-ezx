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

#include "newactiondialog_p.h"
#include "actioneditor_p.h"
#include "findicondialog_p.h"
#include <QtDesigner/abstracticoncache.h>
#include <QtDesigner/abstractformwindow.h>
#include <QtDesigner/abstractformeditor.h>

#include <QtGui/QPushButton>
#include <QtCore/QRegExp>

namespace qdesigner_internal {

NewActionDialog::NewActionDialog(ActionEditor *parent)
    : QDialog(parent, Qt::Sheet),
      m_actionEditor(parent)
{
    ui.setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui.editActionText->setFocus();
    m_auto_update_object_name = true;
    updateButtons();
}

QIcon NewActionDialog::actionIcon() const
{
    return ui.iconButton->icon();
}

void NewActionDialog::setActionData(const QString &text, const QString &name, const QIcon &icon)
{
    ui.editActionText->setText(text);
    ui.editObjectName->setText(name);
    ui.iconButton->setIcon(icon);
    m_auto_update_object_name = false;
    updateButtons();
}

NewActionDialog::~NewActionDialog()
{
}

void NewActionDialog::accept()
{
    QDialog::accept();
}

QString NewActionDialog::actionText() const
{
    return ui.editActionText->text();
}

QString NewActionDialog::actionName() const
{
    return ui.editObjectName->text();
}

void NewActionDialog::on_editActionText_textEdited(const QString &text)
{
    if (text.isEmpty())
        m_auto_update_object_name = true;

    if (m_auto_update_object_name)
        ui.editObjectName->setText(ActionEditor::actionTextToName(text));

    updateButtons();
}

void NewActionDialog::on_editObjectName_textEdited(const QString&)
{
    updateButtons();
    m_auto_update_object_name = false;
}

void NewActionDialog::on_iconButton_clicked()
{
    QDesignerFormWindowInterface *form = m_actionEditor->formWindow();
    QDesignerFormEditorInterface *core = form->core();

    QString file_path;
    QString qrc_path;
    if (!actionIcon().isNull()) {
        file_path = core->iconCache()->iconToFilePath(actionIcon());
        qrc_path = core->iconCache()->iconToQrcPath(actionIcon());
    }

    FindIconDialog dialog(form, this);
    dialog.setPaths(qrc_path, file_path);

    if (!dialog.exec())
        return;

    file_path = dialog.filePath();
    qrc_path = dialog.qrcPath();

    if (file_path.isEmpty())
        return;

    QIcon icon = core->iconCache()->nameToIcon(file_path, qrc_path);
    ui.iconButton->setIcon(icon);
    updateButtons();
}

void NewActionDialog::on_removeIconButton_clicked()
{
    ui.iconButton->setIcon(QIcon());
    updateButtons();
}

void NewActionDialog::updateButtons()
{
    QPushButton *okButton = ui.buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(!actionText().isEmpty() && !actionName().isEmpty());
    ui.removeIconButton->setEnabled(!ui.iconButton->icon().isNull());
}

} // namespace qdesigner_internal
