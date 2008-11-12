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
TRANSLATOR qdesigner_internal::ButtonTaskMenu
*/

#include "button_taskmenu.h"
#include "inplace_editor.h"

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormWindowCursorInterface>

#include <QtGui/QAction>
#include <QtGui/QStyle>
#include <QtGui/QStyleOption>

#include <QtCore/QEvent>
#include <QtCore/QVariant>
#include <QtCore/qdebug.h>

using namespace qdesigner_internal;

ButtonTaskMenu::ButtonTaskMenu(QAbstractButton *button, QObject *parent)
    : QDesignerTaskMenu(button, parent),
      m_button(button),
      m_preferredEditAction(new QAction(tr("Change text..."), this))
{
    connect(m_preferredEditAction, SIGNAL(triggered()), this, SLOT(editText()));
    m_taskActions.append(m_preferredEditAction);

    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    m_taskActions.append(sep);
}

ButtonTaskMenu::~ButtonTaskMenu()
{
}

QAction *ButtonTaskMenu::preferredEditAction() const
{
    return m_preferredEditAction;
}

QList<QAction*> ButtonTaskMenu::taskActions() const
{
    return m_taskActions + QDesignerTaskMenu::taskActions();
}

void ButtonTaskMenu::editText()
{
    m_formWindow = QDesignerFormWindowInterface::findFormWindow(m_button);
    if (!m_formWindow.isNull()) {
        connect(m_formWindow, SIGNAL(selectionChanged()), this, SLOT(updateSelection()));
        Q_ASSERT(m_button->parentWidget() != 0);
        
        QStyleOptionButton opt;
        opt.init(m_button);
        const QRect r = m_button->style()->subElementRect(QStyle::SE_PushButtonContents, &opt, m_button);

        m_editor = new InPlaceEditor(m_button, ValidationMultiLine, m_formWindow,m_button->text(),r);

        connect(m_editor, SIGNAL(textChanged(QString)), this, SLOT(updateText(QString)));
    }
}

void ButtonTaskMenu::editIcon()
{
}

ButtonTaskMenuFactory::ButtonTaskMenuFactory(QExtensionManager *extensionManager)
    : QExtensionFactory(extensionManager)
{
}

QObject *ButtonTaskMenuFactory::createExtension(QObject *object, const QString &iid, QObject *parent) const
{
    if (QAbstractButton *button = qobject_cast<QAbstractButton*>(object)) {
        if (iid == Q_TYPEID(QDesignerTaskMenuExtension)) {
            return new ButtonTaskMenu(button, parent);
        }
    }

    return 0;
}

void ButtonTaskMenu::updateText(const QString &text)
{
    m_formWindow->cursor()->setProperty(QLatin1String("text"), QVariant(text));
}

void ButtonTaskMenu::updateSelection()
{
    if (m_editor)
        m_editor->deleteLater();
}

