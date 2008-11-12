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
TRANSLATOR qdesigner_internal::BuddyEditor
*/

#include "buddyeditor.h"

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerPropertySheetExtension>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QExtensionManager>

#include <qdesigner_command_p.h>
#include <qdesigner_propertycommand_p.h>
#include <qdesigner_utils_p.h>
#include <qlayout_widget_p.h>
#include <connectionedit_p.h>

#include <QtCore/qdebug.h>
#include <QtGui/QLabel>

namespace {
    bool canBeBuddy(QWidget *w, QDesignerFormWindowInterface *form) {
        if (qobject_cast<const QLayoutWidget*>(w)        || w == form->mainContainer() || w->isHidden())
            return false;

        QExtensionManager *ext = form->core()->extensionManager();
        if (QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(ext, w)) {
            const int index = sheet->indexOf(QLatin1String("focusPolicy"));
            if (index != -1) {
                bool ok = false;
                const Qt::FocusPolicy q = static_cast<Qt::FocusPolicy>(qdesigner_internal::Utils::valueOf(sheet->property(index), &ok));
                return ok && q != Qt::NoFocus;
            }
        }
        return false;
    }

    QString buddy(QLabel *label, QDesignerFormEditorInterface *core) {
        QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(core->extensionManager(), label);
        if (sheet == 0)
            return QString();
        const int prop_idx = sheet->indexOf(QLatin1String("buddy"));
        if (prop_idx == -1)
            return QString();
        return sheet->property(prop_idx).toString();
    }

    typedef QList<QLabel*> LabelList;
}

namespace qdesigner_internal {

/*******************************************************************************
** BuddyEditor
*/

BuddyEditor::BuddyEditor(QDesignerFormWindowInterface *form, QWidget *parent) :
    ConnectionEdit(parent, form),
    m_formWindow(form),
    m_updating(false)
{
}


QWidget *BuddyEditor::widgetAt(const QPoint &pos) const
{
    QWidget *w = ConnectionEdit::widgetAt(pos);

    while (w != 0 && !m_formWindow->isManaged(w))
        w = w->parentWidget();
    if (!w)
        return w;

    if (state() == Editing) {
        QLabel *label = qobject_cast<QLabel*>(w);
        if (label == 0)
            return 0;
        const int cnt = connectionCount();
        for (int i = 0; i < cnt; ++i) {
            Connection *con = connection(i);
            if (con->widget(EndPoint::Source) == w)
                return 0;
        }
    } else {
        if (!canBeBuddy(w, m_formWindow))
            return 0;
    }

    return w;
}

Connection *BuddyEditor::createConnection(QWidget *source, QWidget *destination)
{
    return new Connection(this, source, destination);
}

QDesignerFormWindowInterface *BuddyEditor::formWindow() const
{
    return m_formWindow;
}

void BuddyEditor::updateBackground()
{
    if (m_updating || background() == 0)
        return;
    ConnectionEdit::updateBackground();

    m_updating = true;
    QList<Connection *> newList;
    const LabelList label_list = qFindChildren<QLabel*>(background());
    foreach (QLabel *label, label_list) {
        const QString buddy_name = buddy(label, m_formWindow->core());
        if (buddy_name.isEmpty())
            continue;

        const QList<QWidget *> targets = qFindChildren<QWidget*>(background(), buddy_name);
        if (targets.isEmpty())
            continue;

        QWidget *target = 0;

        QListIterator<QWidget *> it(targets);
        while (it.hasNext()) {
            QWidget *widget = it.next();
            if (widget && !widget->isHidden()) {
                target = widget;
                break;
            }
        }

        if (target == 0)
            continue;

        Connection *con = new Connection(this);
        con->setEndPoint(EndPoint::Source, label, widgetRect(label).center());
        con->setEndPoint(EndPoint::Target, target, widgetRect(target).center());
        newList.append(con);
    }

    QList<Connection *> toRemove;

    const int c = connectionCount();
    for (int i = 0; i < c; i++) {
        Connection *con = connection(i);
        QObject *source = con->object(EndPoint::Source);
        QObject *target = con->object(EndPoint::Target);
        bool found = false;
        QListIterator<Connection *> it(newList);
        while (it.hasNext()) {
            Connection *newConn = it.next();
            if (newConn->object(EndPoint::Source) == source && newConn->object(EndPoint::Target) == target) {
                found = true;
                break;
            }
        }
        if (found == false)
            toRemove.append(con);
    }
    if (!toRemove.isEmpty()) {
        DeleteConnectionsCommand command(this, toRemove);
        command.redo();
        foreach (Connection *con, toRemove) {
            if (m_con_list.contains(con)) {
                m_con_list.removeAll(con);
                delete con;
            }
        }
    }

    QListIterator<Connection *> it(newList);
    while (it.hasNext()) {
        Connection *newConn = it.next();

        bool found = false;
        const int c = connectionCount();
        for (int i = 0; i < c; i++) {
            Connection *con = connection(i);
            if (con->object(EndPoint::Source) == newConn->object(EndPoint::Source) &&
                            con->object(EndPoint::Target) == newConn->object(EndPoint::Target)) {
                found = true;
                break;
            }
        }
        if (found == false) {
            AddConnectionCommand command(this, newConn);
            command.redo();
        } else {
            delete newConn;
        }
    }
    m_updating = false;
}

void BuddyEditor::setBackground(QWidget *background)
{
    clear();
    ConnectionEdit::setBackground(background);

    const LabelList label_list = qFindChildren<QLabel*>(background);
    foreach (QLabel *label, label_list) {
        const QString buddy_name = buddy(label, m_formWindow->core());
        if (buddy_name.isEmpty())
            continue;
        QWidget *target = qFindChild<QWidget*>(background, buddy_name);
        if (target == 0)
            continue;

        Connection *con = new Connection(this);
        con->setEndPoint(EndPoint::Source, label, widgetRect(label).center());
        con->setEndPoint(EndPoint::Target, target, widgetRect(target).center());
        addConnection(con);
    }
}

void BuddyEditor::endConnection(QWidget *target, const QPoint &pos)
{
    Q_ASSERT(m_tmp_con != 0);

    m_tmp_con->setEndPoint(EndPoint::Target, target, pos);

    QWidget *source = m_tmp_con->widget(EndPoint::Source);
    Q_ASSERT(source != 0);
    Q_ASSERT(target != 0);
    setEnabled(false);
    Connection *new_con = createConnection(source, target);
    setEnabled(true);
    if (new_con != 0) {
        new_con->setEndPoint(EndPoint::Source, source, m_tmp_con->endPointPos(EndPoint::Source));
        new_con->setEndPoint(EndPoint::Target, target, m_tmp_con->endPointPos(EndPoint::Target));

        selectNone();
        m_con_list.append(new_con);
        QWidget *source = new_con->widget(EndPoint::Source);
        QWidget *target = new_con->widget(EndPoint::Target);
        if (qobject_cast<QLabel*>(source) == 0) {
            qDebug("BuddyEditor::endConnection(): not a label");
        } else {
            undoStack()->beginMacro(tr("Add buddy"));
            SetPropertyCommand *command = new SetPropertyCommand(formWindow());
            command->init(source, QLatin1String("buddy"), target->objectName());
            undoStack()->push(command);
            undoStack()->endMacro();
        }
        setSelected(new_con, true);
    }

    delete m_tmp_con;
    m_tmp_con = 0;

    findObjectsUnderMouse(mapFromGlobal(QCursor::pos()));
}

void BuddyEditor::widgetRemoved(QWidget *widget)
{
    QList<QWidget*> child_list = qFindChildren<QWidget*>(widget);
    child_list.prepend(widget);

    ConnectionSet remove_set;
    foreach (QWidget *w, child_list) {
        foreach (Connection *con, m_con_list) {
            if (con->widget(EndPoint::Source) == w || con->widget(EndPoint::Target) == w)
                remove_set.insert(con, con);
        }
    }

    if (!remove_set.isEmpty()) {
        undoStack()->beginMacro(tr("Remove buddies"));
        foreach (Connection *con, remove_set) {
            setSelected(con, false);
            con->update();
            QWidget *source = con->widget(EndPoint::Source);
            if (qobject_cast<QLabel*>(source) == 0) {
                qDebug("BuddyConnection::widgetRemoved(): not a label");
            } else {
                ResetPropertyCommand *command = new ResetPropertyCommand(formWindow());
                command->init(source, QLatin1String("buddy"));
                undoStack()->push(command);
            }
            if (m_con_list.contains(con)) {
                m_con_list.removeAll(con);
                delete con;
            }
        }
        undoStack()->endMacro();
    }
}

void BuddyEditor::deleteSelected()
{
    if (m_sel_con_set.isEmpty())
        return;

    if (!m_sel_con_set.isEmpty()) {
        undoStack()->beginMacro(tr("Remove buddies"));
        foreach (Connection *con, m_sel_con_set) {
            setSelected(con, false);
            con->update();
            QWidget *source = con->widget(EndPoint::Source);
            if (qobject_cast<QLabel*>(source) == 0) {
                qDebug("BuddyConnection::deleteSelected(): not a label");
            } else {
                ResetPropertyCommand *command = new ResetPropertyCommand(formWindow());
                command->init(source, QLatin1String("buddy"));
                undoStack()->push(command);
            }
            if (m_con_list.contains(con)) {
                m_con_list.removeAll(con);
                delete con;
            }
        }
        undoStack()->endMacro();
    }
}

}
