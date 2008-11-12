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
TRANSLATOR qdesigner_internal::ComboBoxTaskMenu
*/

#include "combobox_taskmenu.h"
#include "inplace_editor.h"
#include "listwidgeteditor.h"
#include <qdesigner_command_p.h>

#include <QtDesigner/QDesignerFormWindowInterface>
    
#include <QtGui/QAction>
#include <QtGui/QStyle>
#include <QtGui/QFontComboBox>
#include <QtGui/QStyleOption>

#include <QtCore/QEvent>
#include <QtCore/QVariant>
#include <QtCore/qdebug.h>

using namespace qdesigner_internal;

ComboBoxTaskMenu::ComboBoxTaskMenu(QComboBox *button, QObject *parent)
    : QDesignerTaskMenu(button, parent),
      m_comboBox(button)
{
    m_editItemsAction = new QAction(this);
    m_editItemsAction->setText(tr("Edit Items..."));
    connect(m_editItemsAction, SIGNAL(triggered()), this, SLOT(editItems()));
    m_taskActions.append(m_editItemsAction);

    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    m_taskActions.append(sep);
}

ComboBoxTaskMenu::~ComboBoxTaskMenu()
{
}

QAction *ComboBoxTaskMenu::preferredEditAction() const
{
    return m_editItemsAction;
}

QList<QAction*> ComboBoxTaskMenu::taskActions() const
{
    return m_taskActions + QDesignerTaskMenu::taskActions();
}

void ComboBoxTaskMenu::editItems()
{
    m_formWindow = QDesignerFormWindowInterface::findFormWindow(m_comboBox);
    if (m_formWindow.isNull())
        return;

    Q_ASSERT(m_comboBox != 0);

    ListWidgetEditor dlg(m_formWindow, m_comboBox->window());
    dlg.fillContentsFromComboBox(m_comboBox);
    if (dlg.exec() == QDialog::Accepted) {
        QList<QPair<QString, QIcon> > items;
        for (int i = 0; i < dlg.count(); i++) {
            items.append(qMakePair<QString, QIcon>(dlg.text(i), dlg.icon(i)));
        }
        ChangeListContentsCommand *cmd = new ChangeListContentsCommand(m_formWindow);
        cmd->init(m_comboBox, items);
        cmd->setText(tr("Change Combobox Contents"));
        m_formWindow->commandHistory()->push(cmd);
    }
}

ComboBoxTaskMenuFactory::ComboBoxTaskMenuFactory(QExtensionManager *extensionManager)
    : QExtensionFactory(extensionManager)
{
}

QObject *ComboBoxTaskMenuFactory::createExtension(QObject *object, const QString &iid, QObject *parent) const
{
    QComboBox *button = qobject_cast<QComboBox*>(object);
    if (button && !qobject_cast<QFontComboBox*>(object)) {
        if (iid == Q_TYPEID(QDesignerTaskMenuExtension)) {
            return new ComboBoxTaskMenu(button, parent);
        }
    }

    return 0;
}

void ComboBoxTaskMenu::updateSelection()
{
    if (m_editor)
        m_editor->deleteLater();
}

