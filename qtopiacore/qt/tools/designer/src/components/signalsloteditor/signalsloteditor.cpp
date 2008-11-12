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

#include "signalsloteditor.h"
#include "signalsloteditor_p.h"

#include <qdesigner_membersheet_p.h>
#include <ui4_p.h>

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QExtensionManager>
#include <QtDesigner/QDesignerMetaDataBaseInterface>
#include <QtDesigner/QDesignerContainerExtension>
#include <QtDesigner/QDesignerLanguageExtension>
#include <QtDesigner/QDesignerFormWindowCursorInterface>
#include <QtDesigner/QDesignerWidgetDataBaseInterface>

#include <QtGui/QAction>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QListWidget>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
#include <QtGui/QApplication>
#include <QtGui/QUndoCommand>
#include <QtGui/QMenu>

#include <QtCore/qdebug.h>

/*******************************************************************************
** Tools
*/

template <typename T>
static void merge(QDesignerFormWindowInterface *form, QStringList *lst, const QList<T> &elts)
{
    QDesignerMetaDataBaseInterface *db = form->core()->metaDataBase();

    foreach (T e, elts) {
        QAction *action = qobject_cast<QAction*>(e);

        if (action && db->item(action->menu())) {
            // good
        } else if (!db->item(e)) {
            // hmm, nothing to do
            continue;
        }

        QString name = e->objectName();

        if (action && action->menu())
            name = action->menu()->objectName();

        if (name.isEmpty())
            continue;

        lst->append(name);
    }
}

static bool signalMatchesSlot(QDesignerFormEditorInterface *core,
                              const QString &signal,
                              const QString &slot)
{    
    QExtensionManager *em = core->extensionManager();
    const QDesignerLanguageExtension *lang = qt_extension<QDesignerLanguageExtension*> (em, core);
    if (lang)
        return lang->signalMatchesSlot(signal, slot);

    return QDesignerMemberSheet::signalMatchesSlot(signal, slot);
}

namespace qdesigner_internal {

QStringList objectNameList(QDesignerFormWindowInterface *form)
{
    QStringList result;
    if (form->mainContainer()) {
        QDesignerContainerExtension *c = qt_extension<QDesignerContainerExtension *>(
                    form->core()->extensionManager(), form->mainContainer());
        if (c) {
            for (int i = 0 ; i < c->count(); i++)
                result.append(c->widget(i)->objectName().trimmed());
        }
    }

    QDesignerFormWindowCursorInterface *cursor = form->cursor();
    for (int i = 0; i < cursor->widgetCount(); ++i) {
        const QString name = cursor->widget(i)->objectName().trimmed();
        if (!name.isEmpty())
            result.append(name);
    }

    if (form->mainContainer()) {
        merge(form, &result, qFindChildren<QAction*>(form->mainContainer()));
    }

    result.sort();

    return result;
}

QStringList memberList(QDesignerFormWindowInterface *form, QObject *object, MemberType member_type)
{
    QStringList result;

    if (object == 0)
        return result;

    QDesignerMemberSheetExtension *members
        = qt_extension<QDesignerMemberSheetExtension*>
                (form->core()->extensionManager(), object);
    Q_ASSERT(members != 0);

    for (int i = 0; i < members->count(); ++i) {
        if (!members->isVisible(i))
            continue;

        if (member_type == SignalMember && !members->isSignal(i))
            continue;

        if (member_type == SlotMember && !members->isSlot(i))
            continue;

        result.append(members->signature(i));
    }

    return result;
}


ClassList classList(const QString &obj_name, MemberType member_type,
                            const QString &peer, QDesignerFormWindowInterface *form)
{
    ClassList result;

    QObject *object = qFindChild<QObject*>(form, obj_name);

    if (object == 0)
        return result;

    QDesignerFormEditorInterface *core = form->core();
    QDesignerMemberSheetExtension *members = qt_extension<QDesignerMemberSheetExtension*>(core->extensionManager(), object);
    Q_ASSERT(members != 0);

    QString class_name;
    QStringList member_list;
    for (int i = members->count() -  1; i >= 0; --i) {
        if (!members->isVisible(i))
            continue;

        if (member_type == SignalMember && !members->isSignal(i))
            continue;

        if (member_type == SlotMember && !members->isSlot(i))
            continue;

        const QString signal = member_type == SignalMember ? members->signature(i) : peer;
        const QString slot = member_type == SignalMember ? peer : members->signature(i);
        if (!signalMatchesSlot(core, signal, slot))
            continue;

       const QString s = members->declaredInClass(i);
        if (s != class_name) {
            if (!member_list.isEmpty())
                result.append(ClassInfo(class_name, member_list));
            class_name = s;
            member_list.clear();
        }
        member_list.append(members->signature(i));
    }
    if (!member_list.isEmpty())
        result.append(ClassInfo(class_name, member_list));

    return result;
}
}

namespace {
/*******************************************************************************
** OldSignalSlotDialog
*/

class OldSignalSlotDialog : public QDialog
{
    Q_OBJECT
public:
    OldSignalSlotDialog(QDesignerFormEditorInterface *core, QWidget *sender, QWidget *receiver, QWidget *parent = 0);

    QString signal() const;
    QString slot() const;

    void setSignalSlot(const QString &signal, const QString &slot);

    bool showAllSignalsSlots() const;
    void setShowAllSignalsSlots(bool showIt);

private slots:
    void selectSignal(QListWidgetItem *item);
    void selectSlot(QListWidgetItem *item);
    void populateSignalList();
    void populateSlotList(const QString &signal = QString());

private:
    QListWidget *m_signal_list, *m_slot_list;
    QDialogButtonBox *m_buttonBox;
    QPushButton *m_ok_button;
    QWidget *m_source, *m_destination;
    QDesignerFormEditorInterface *m_core;
    QCheckBox *m_show_all_checkbox;
};

// ### deprecated
QString realObjectName(QDesignerFormEditorInterface *core, QObject *object)
{
    if (object == 0)
        return QString();

    const QDesignerMetaDataBaseInterface *mdb = core->metaDataBase();
    if (const QDesignerMetaDataBaseItemInterface *item = mdb->item(object))
        return item->name();

    return object->objectName();
}

QString realClassName(QDesignerFormEditorInterface *core, QWidget *widget)
{
    QString class_name = QLatin1String(widget->metaObject()->className());
    const QDesignerWidgetDataBaseInterface *wdb = core->widgetDataBase();
    const int idx = wdb->indexOfObject(widget);
    if (idx != -1)
        class_name = wdb->item(idx)->name();
    return class_name;
}

QString widgetLabel(QDesignerFormEditorInterface *core, QWidget *widget)
{
    return QString::fromUtf8("%1 (%2)")
            .arg(realObjectName(core, widget))
            .arg(realClassName(core, widget));
}

void OldSignalSlotDialog::populateSlotList(const QString &signal)
{
    QString selectedName;
    if (QListWidgetItem * item = m_slot_list->currentItem()) {
        selectedName = item->text();
    }
    m_slot_list->clear();

    const bool show_all = m_show_all_checkbox->isChecked();

    QStringList signatures;

    if (QDesignerMemberSheetExtension *members
            = qt_extension<QDesignerMemberSheetExtension*>
                    (m_core->extensionManager(), m_destination)) {
        for (int i=0; i<members->count(); ++i) {
            if (!members->isVisible(i))
                continue;

            if (!show_all && members->inheritedFromWidget(i))
                continue;

            if (members->isSlot(i)) {
                if (!signalMatchesSlot(m_core, signal, members->signature(i)))
                    continue;

                signatures.append(members->signature(i));
            }
        }
    }

    signatures.sort();

    QListWidgetItem *curr = 0;
    foreach (QString sig, signatures) {
        QListWidgetItem *item = new QListWidgetItem(m_slot_list);
        item->setText(sig);
        if (sig == selectedName)
            curr = item;
    }
    if (curr)
        m_slot_list->setCurrentItem(curr);

    if (m_slot_list->selectedItems().isEmpty())
        m_ok_button->setEnabled(false);
}

void OldSignalSlotDialog::populateSignalList()
{
    QString selectedName;
    if (QListWidgetItem * item = m_signal_list->currentItem()) {
        selectedName = item->text();
    }
    m_signal_list->clear();

    const bool show_all = m_show_all_checkbox->isChecked();

    QStringList signatures;

    if (QDesignerMemberSheetExtension *members = qt_extension<QDesignerMemberSheetExtension*>(m_core->extensionManager(), m_source)) {
        for (int i=0; i<members->count(); ++i) {
            if (!members->isVisible(i))
                continue;

            if (!show_all && members->inheritedFromWidget(i))
                continue;

            if (members->isSignal(i)) {
                signatures.append(members->signature(i));
            }
        }
    }

    signatures.sort();

    QListWidgetItem *curr = 0;
    foreach (QString sig, signatures) {
        QListWidgetItem *item = new QListWidgetItem(m_signal_list);
        item->setText(sig);
        if (!selectedName.isEmpty() && sig == selectedName) {
            curr = item;
        }
    }

    if (curr) {
        m_signal_list->setCurrentItem(curr);
    } else {
        selectedName.clear();
    }

    populateSlotList(selectedName);
    if (!curr) {
        m_slot_list->setEnabled(false);
    }
}

// ### use designer
OldSignalSlotDialog::OldSignalSlotDialog(QDesignerFormEditorInterface *core, QWidget *source, QWidget *destination,
                                         QWidget *parent) :
    QDialog(parent),
    m_signal_list(new QListWidget(this)),
    m_slot_list(new QListWidget(this)),
    m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,Qt::Horizontal, this)),
    m_ok_button(m_buttonBox->button(QDialogButtonBox::Ok)),
    m_source(source),
    m_destination(destination),
    m_core(core),
    m_show_all_checkbox(new QCheckBox(QObject::tr("Show all signals and slots")))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_signal_list->setTextElideMode (Qt::ElideMiddle);
    m_slot_list->setTextElideMode (Qt::ElideMiddle);

    connect(m_signal_list,
                SIGNAL(itemClicked(QListWidgetItem*)),
            this,
                SLOT(selectSignal(QListWidgetItem*)));
    connect(m_slot_list,
                SIGNAL(itemClicked(QListWidgetItem*)),
            this,
                SLOT(selectSlot(QListWidgetItem*)));
    m_slot_list->setEnabled(false);

    m_ok_button->setDefault(true);
    m_ok_button->setEnabled(false);
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect(m_show_all_checkbox, SIGNAL(toggled(bool)), this, SLOT(populateSignalList()));

    QLabel *source_label = new QLabel(this);
    source_label->setText(widgetLabel(core, source));
    QLabel *destination_label = new QLabel(this);
    destination_label->setText(widgetLabel(core, destination));

    QVBoxLayout *l1 = new QVBoxLayout(this);

    QHBoxLayout *l2 = new QHBoxLayout();
    l1->addLayout(l2);

    QVBoxLayout *l3 = new QVBoxLayout();
    l2->addLayout(l3);

    l3->addWidget(source_label);
    l3->addWidget(m_signal_list);

    QVBoxLayout *l4 = new QVBoxLayout();
    l2->addLayout(l4);

    l4->addWidget(destination_label);
    l4->addWidget(m_slot_list);

    l1->addWidget(m_show_all_checkbox);
    l1->addWidget(m_buttonBox);

    setWindowTitle(QObject::tr("Configure Connection"));

    populateSignalList();
}

QListWidgetItem *findItem(const QListWidget &list_widget, const QString &text)
{
    QListWidgetItem *result = 0;
    for (int i = 0; i < list_widget.count(); ++i) {
        QListWidgetItem *item = list_widget.item(i);
        if (item->text() == text) {
            result = item;
            break;
        }
    }
    return result;
}

void OldSignalSlotDialog::setSignalSlot(const QString &signal, const QString &slot)
{
    QListWidgetItem *sig_item = findItem(*m_signal_list, signal);

    if (sig_item == 0) {
        m_show_all_checkbox->setChecked(true);
        sig_item = findItem(*m_signal_list, signal);
    }

    if (sig_item != 0) {
        selectSignal(sig_item);
        QListWidgetItem *slot_item = findItem(*m_slot_list, slot);
        if (slot_item == 0) {
            m_show_all_checkbox->setChecked(true);
            slot_item = findItem(*m_slot_list, slot);
        }
        if (slot_item != 0)
            selectSlot(slot_item);
    }
}

bool OldSignalSlotDialog::showAllSignalsSlots() const
{
    return m_show_all_checkbox->isChecked();
}

void OldSignalSlotDialog::setShowAllSignalsSlots(bool showIt)
{
    m_show_all_checkbox->setChecked(showIt);
}

void OldSignalSlotDialog::selectSignal(QListWidgetItem *item)
{
    if (item == 0) {
        m_signal_list->clearSelection();
        populateSlotList();
        m_slot_list->setEnabled(false);
        m_ok_button->setEnabled(false);
    } else {
        m_signal_list->setCurrentItem(item);
        populateSlotList(item->text());
        m_slot_list->setEnabled(true);
        m_ok_button->setEnabled(!m_slot_list->selectedItems().isEmpty());
    }
}

void OldSignalSlotDialog::selectSlot(QListWidgetItem *item)
{
    if (item == 0) {
        m_slot_list->clearSelection();
    } else {
        m_slot_list->setCurrentItem(item);
    }
    m_ok_button->setEnabled(true);
}

QString OldSignalSlotDialog::signal() const
{
    QList<QListWidgetItem*> item_list = m_signal_list->selectedItems();
    if (item_list.size() != 1)
        return QString();
    return item_list.at(0)->text();
}

QString OldSignalSlotDialog::slot() const
{
    QList<QListWidgetItem*> item_list = m_slot_list->selectedItems();
    if (item_list.size() != 1)
        return QString();
    return item_list.at(0)->text();
}

}
namespace qdesigner_internal {

/*******************************************************************************
** SignalSlotConnection
*/

SignalSlotConnection::SignalSlotConnection(ConnectionEdit *edit, QWidget *source, QWidget *target)
    : Connection(edit, source, target)
{
}

DomConnection *SignalSlotConnection::toUi() const
{
    DomConnection *result = new DomConnection;

    result->setElementSender(sender());
    result->setElementSignal(signal());
    result->setElementReceiver(receiver());
    result->setElementSlot(slot());

    DomConnectionHints *hints = new DomConnectionHints;
    QList<DomConnectionHint*> list;

    QPoint sp = endPointPos(EndPoint::Source);
    QPoint tp = endPointPos(EndPoint::Target);

    DomConnectionHint *hint = new DomConnectionHint;
    hint->setAttributeType(QLatin1String("sourcelabel"));
    hint->setElementX(sp.x());
    hint->setElementY(sp.y());
    list.append(hint);

    hint = new DomConnectionHint;
    hint->setAttributeType(QLatin1String("destinationlabel"));
    hint->setElementX(tp.x());
    hint->setElementY(tp.y());
    list.append(hint);

    hints->setElementHint(list);
    result->setElementHints(hints);

    return result;
}

void SignalSlotConnection::setSignal(const QString &signal)
{
    m_signal = signal;
    setLabel(EndPoint::Source, m_signal);
}

void SignalSlotConnection::setSlot(const QString &slot)
{
    m_slot = slot;
    setLabel(EndPoint::Target, m_slot);
}

QString SignalSlotConnection::sender() const
{
    SignalSlotEditor *edit = qobject_cast<SignalSlotEditor*>(this->edit());
    Q_ASSERT(edit != 0);

    return realObjectName(edit->formWindow()->core(), object(EndPoint::Source));
}

QString SignalSlotConnection::receiver() const
{
    SignalSlotEditor *edit = qobject_cast<SignalSlotEditor*>(this->edit());
    Q_ASSERT(edit != 0);

    return realObjectName(edit->formWindow()->core(), object(EndPoint::Target));
}

void SignalSlotConnection::updateVisibility()
{
    Connection::updateVisibility();
    if (isVisible() && (signal().isEmpty() || slot().isEmpty()))
        setVisible(false);
}

/*******************************************************************************
** Commands
*/

class SetMemberCommand : public QUndoCommand, public CETypes
{
public:
    SetMemberCommand(SignalSlotConnection *con, EndPoint::Type type,
                        const QString &member, SignalSlotEditor *editor);
    virtual void redo();
    virtual void undo();
private:
    const QString m_old_member;
    const QString m_new_member;
    const EndPoint::Type m_type;
    SignalSlotConnection *m_con;
    SignalSlotEditor *m_editor;
};

SetMemberCommand::SetMemberCommand(SignalSlotConnection *con, EndPoint::Type type,
                                   const QString &member, SignalSlotEditor *editor) :
    m_old_member(type == EndPoint::Source ? con->signal() : con->slot()),
    m_new_member(member),
    m_type(type),
    m_con(con),
    m_editor(editor)
{
    if (type == EndPoint::Source)
        setText(QApplication::translate("Command", "Change signal"));
    else
        setText(QApplication::translate("Command", "Change slot"));
}

void SetMemberCommand::redo()
{
    m_con->update();
    if (m_type == EndPoint::Source)
        m_con->setSignal(m_new_member);
    else
        m_con->setSlot(m_new_member);
    m_con->update();
    emit m_editor->connectionChanged(m_con);
}

void SetMemberCommand::undo()
{
    m_con->update();
    if (m_type == EndPoint::Source)
        m_con->setSignal(m_old_member);
    else
        m_con->setSlot(m_old_member);
    m_con->update();
    emit m_editor->connectionChanged(m_con);
}

/*******************************************************************************
** SignalSlotEditor
*/

SignalSlotEditor::SignalSlotEditor(QDesignerFormWindowInterface *form_window, QWidget *parent) :
     ConnectionEdit(parent, form_window),
     m_form_window(form_window),
     m_model(new ConnectionModel(this, this)),
     m_showAllSignalsSlots(false)
{
    connect(this, SIGNAL(widgetActivated(QWidget*)), form_window, SIGNAL(activated(QWidget*)));
}

QAbstractItemModel *SignalSlotEditor::model() const
{
    return m_model;
}

void SignalSlotEditor::modifyConnection(Connection *con)
{
    SignalSlotConnection *sigslot_con = static_cast<SignalSlotConnection*>(con);

    OldSignalSlotDialog dialog(m_form_window->core(),
                               sigslot_con->widget(EndPoint::Source),
                               sigslot_con->widget(EndPoint::Target),
                               m_form_window->core()->topLevel());

    dialog.setSignalSlot(sigslot_con->signal(), sigslot_con->slot());
    dialog.setShowAllSignalsSlots(m_showAllSignalsSlots);

    if (dialog.exec() == QDialog::Accepted) {
        sigslot_con->setSignal(dialog.signal());
        sigslot_con->setSlot(dialog.slot());
    }

    m_showAllSignalsSlots = dialog.showAllSignalsSlots();
}

Connection *SignalSlotEditor::createConnection(QWidget *source, QWidget *destination)
{
    SignalSlotConnection *con = 0;

    Q_ASSERT(source != 0);
    Q_ASSERT(destination != 0);

    OldSignalSlotDialog dialog(m_form_window->core(),
                               source,
                               destination,
                               m_form_window->core()->topLevel());

    dialog.setShowAllSignalsSlots(m_showAllSignalsSlots);

    if (dialog.exec() == QDialog::Accepted) {
        con = new SignalSlotConnection(this, source, destination);
        con->setSignal(dialog.signal());
        con->setSlot(dialog.slot());
    }

    m_showAllSignalsSlots = dialog.showAllSignalsSlots();

    return con;
}

DomConnections *SignalSlotEditor::toUi() const
{
    DomConnections *result = new DomConnections;
    QList<DomConnection*> list;
    for (int i = 0; i < connectionCount(); ++i) {
        const SignalSlotConnection *con = static_cast<const SignalSlotConnection*>(connection(i));
        Q_ASSERT(con != 0);

        // If a widget's parent has been removed, and the parent was not a managed widget
        // (a page in a tab widget), we never get a widgetRemoved(). So we filter out
        // these child widgets here.
        const QObject *source = con->object(EndPoint::Source);
        if (source == 0)
            continue;

        const QObject *target = con->object(EndPoint::Target);
        if (target == 0)
            continue;

        if (const QWidget *sourceWidget = qobject_cast<const QWidget*>(source)) {
            if (!background()->isAncestorOf(sourceWidget))
                continue;
        }

        if (const QWidget *targetWidget = qobject_cast<const QWidget*>(target)) {
            if (!background()->isAncestorOf(targetWidget))
                continue;
        }

        list.append(con->toUi());
    }
    result->setElementConnection(list);
    return result;
}

QObject *SignalSlotEditor::objectByName(QWidget *topLevel, const QString &name) const
{
    if (name.isEmpty())
        return 0;

    Q_ASSERT(topLevel);
    QObject *object = 0;
    if (topLevel->objectName() == name)
        object = topLevel;
    else
        object = qFindChild<QObject*>(topLevel, name);
    const QDesignerMetaDataBaseInterface *mdb = formWindow()->core()->metaDataBase();
    if (mdb->item(object))
        return object;
    return 0;
}

void SignalSlotEditor::fromUi(const DomConnections *connections, QWidget *parent)
{
    if (connections == 0)
        return;

    setBackground(parent);
    clear();
    const QList<DomConnection*> list = connections->elementConnection();
    foreach (const DomConnection *dom_con, list) {
        QObject *source = objectByName(parent, dom_con->elementSender());
        if (source == 0) {
            qDebug("SignalSlotEditor::fromUi(): no source widget called \"%s\"",
                        dom_con->elementSender().toUtf8().constData());
            continue;
        }
        QObject *destination = objectByName(parent, dom_con->elementReceiver());
        if (destination == 0) {
            qDebug("SignalSlotEditor::fromUi(): no destination widget called \"%s\"",
                        dom_con->elementReceiver().toUtf8().constData());
            continue;
        }

        QPoint sp = QPoint(20, 20), tp = QPoint(20, 20);
        const DomConnectionHints *dom_hints = dom_con->elementHints();
        if (dom_hints != 0) {
            QList<DomConnectionHint*> list = dom_hints->elementHint();
            foreach (DomConnectionHint *hint, list) {
                QString attr_type = hint->attributeType();
                QPoint p = QPoint(hint->elementX(), hint->elementY());
                if (attr_type == QLatin1String("sourcelabel"))
                    sp = p;
                else if (attr_type == QLatin1String("destinationlabel"))
                    tp = p;
            }
        }

        SignalSlotConnection *con = new SignalSlotConnection(this);

        con->setEndPoint(EndPoint::Source, source, sp);
        con->setEndPoint(EndPoint::Target, destination, tp);
        con->setSignal(dom_con->elementSignal());
        con->setSlot(dom_con->elementSlot());
        addConnection(con);
    }
}

static bool skipWidget(const QWidget *w)
{
    const QString name = QLatin1String(w->metaObject()->className());
    if (name == QLatin1String("QDesignerWidget"))
        return true;
    if (name == QLatin1String("QLayoutWidget"))
        return true;
    if (name == QLatin1String("qdesigner_internal::FormWindow"))
        return true;
    if (name == QLatin1String("Spacer"))
        return true;
    return false;
}

QWidget *SignalSlotEditor::widgetAt(const QPoint &pos) const
{
    QWidget *widget = ConnectionEdit::widgetAt(pos);

    if (widget == m_form_window->mainContainer())
        return widget;

    for (; widget != 0; widget = widget->parentWidget()) {
        QDesignerMetaDataBaseItemInterface *item = m_form_window->core()->metaDataBase()->item(widget);
        if (item == 0)
            continue;
        if (skipWidget(widget))
            continue;
        break;
    }

    return widget;
}

void SignalSlotEditor::setSignal(SignalSlotConnection *con, const QString &member)
{
    if (member == con->signal())
        return;
    
    m_form_window->beginCommand(QApplication::translate("Command", "Change signal"));
    undoStack()->push(new SetMemberCommand(con, EndPoint::Source, member, this));
    if (!signalMatchesSlot(m_form_window->core(), member, con->slot()))
        undoStack()->push(new SetMemberCommand(con, EndPoint::Target, QString(), this));
    m_form_window->endCommand();
}

void SignalSlotEditor::setSlot(SignalSlotConnection *con, const QString &member)
{
    if (member == con->slot())
        return;

    m_form_window->beginCommand(QApplication::translate("Command", "Change slot"));
    undoStack()->push(new SetMemberCommand(con, EndPoint::Target, member, this));
    if (!signalMatchesSlot(m_form_window->core(), con->signal(), member))
        undoStack()->push(new SetMemberCommand(con, EndPoint::Source, QString(), this));
    m_form_window->endCommand();
}

void SignalSlotEditor::setSource(Connection *_con, const QString &obj_name)
{
    SignalSlotConnection *con = static_cast<SignalSlotConnection*>(_con);

   if (con->sender() == obj_name)
        return;

    m_form_window->beginCommand(QApplication::translate("Command", "Change sender"));
    ConnectionEdit::setSource(con, obj_name);

    QObject *sourceObject = con->object(EndPoint::Source);
    const QStringList member_list = memberList(m_form_window, sourceObject, SignalMember);

    if (!member_list.contains(con->signal()))
        undoStack()->push(new SetMemberCommand(con, EndPoint::Source, QString(), this));

    m_form_window->endCommand();
}

void SignalSlotEditor::setTarget(Connection *_con, const QString &obj_name)
{
    SignalSlotConnection *con = static_cast<SignalSlotConnection*>(_con);

    if (con->receiver() == obj_name)
        return;

    m_form_window->beginCommand(QApplication::translate("Command", "Change receiver"));
    ConnectionEdit::setTarget(con, obj_name);

    QObject *targetObject = con->object(EndPoint::Target);
    QStringList member_list = memberList(m_form_window, targetObject, SlotMember);

    if (!member_list.contains(con->slot()))
        undoStack()->push(new SetMemberCommand(con, EndPoint::Target, QString(), this));

    m_form_window->endCommand();
}

void SignalSlotEditor::addEmptyConnection()
{
    SignalSlotConnection *con = new SignalSlotConnection(this);
    undoStack()->push(new AddConnectionCommand(this, con));
}

} // namespace qdesigner_internal

#include "signalsloteditor.moc"
