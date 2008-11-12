/****************************************************************************
**
** Copyright (C) 2005-2008 Trolltech ASA. All rights reserved.
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
TRANSLATOR qdesigner_internal::ConnectionModel
*/

#include "signalsloteditorwindow.h"
#include "signalsloteditor_p.h"
#include "signalsloteditor.h"
#include "qdesigner_integration_p.h"

#include <iconloader_p.h>
#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowManagerInterface>

#include <QtCore/QAbstractItemModel>
#include <QtCore/QDebug>
#include <QtGui/QStandardItemModel>
#include <QtGui/QComboBox>
#include <QtGui/QApplication>
#include <QtGui/QItemDelegate>
#include <QtGui/QItemEditorFactory>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QMessageBox>

namespace qdesigner_internal {

/*******************************************************************************
** ConnectionModel
*/

ConnectionModel::ConnectionModel(SignalSlotEditor *editor, QObject *parent)
    : QAbstractItemModel(parent)
{
    m_editor = editor;

    connect(m_editor, SIGNAL(connectionAdded(Connection*)),
            this, SLOT(connectionAdded(Connection*)));
    connect(m_editor, SIGNAL(connectionRemoved(int)),
            this, SLOT(connectionRemoved(int)));
    connect(m_editor, SIGNAL(aboutToRemoveConnection(Connection*)),
            this, SLOT(aboutToRemoveConnection(Connection*)));
    connect(m_editor, SIGNAL(aboutToAddConnection(int)),
            this, SLOT(aboutToAddConnection(int)));
    connect(m_editor, SIGNAL(connectionChanged(Connection*)),
            this, SLOT(connectionChanged(Connection*)));
}

QVariant ConnectionModel::headerData(int section, Qt::Orientation orientation,
                                        int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();

    static const QVariant senderTitle = tr("Sender");
    static const QVariant signalTitle = tr("Signal");
    static const QVariant receiverTitle = tr("Receiver");
    static const QVariant slotTitle = tr("Slot");

    switch (section) {
    case 0:
        return senderTitle;
    case 1:
        return signalTitle;
    case 2:
        return receiverTitle;
    case 3:
        return slotTitle;
    }
    return  QVariant();
}

QModelIndex ConnectionModel::index(int row, int column,
                                    const QModelIndex &parent) const
{
    if (parent.isValid())
        return QModelIndex();
    if (row < 0 || row >= m_editor->connectionCount())
        return QModelIndex();
    return createIndex(row, column);
}

Connection *ConnectionModel::indexToConnection(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    if (index.row() < 0 || index.row() >= m_editor->connectionCount())
        return 0;
    return m_editor->connection(index.row());
}

QModelIndex ConnectionModel::connectionToIndex(Connection *con) const
{
    return createIndex(m_editor->indexOfConnection(con), 0);
}

QModelIndex ConnectionModel::parent(const QModelIndex&) const
{
    return QModelIndex();
}

int ConnectionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_editor->connectionCount();
}

int ConnectionModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 4;
}

QVariant ConnectionModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    if (index.row() < 0 || index.row() >= m_editor->connectionCount()) {
        return QVariant();
    }

    const SignalSlotConnection *con = static_cast<SignalSlotConnection*>(m_editor->connection(index.row()));
    Q_ASSERT(con != 0);

    static const QVariant senderDefault = tr("<sender>");
    static const QVariant signalDefault = tr("<signal>");
    static const QVariant receiverDefault = tr("<receiver>");
    static const QVariant slotDefault = tr("<slot>");

    switch (index.column()) {
        case 0: {
            const QString sender = con->sender();
            if (sender.isEmpty())
                return senderDefault;
            return sender;
        }
        case 1: {
            const QString signal = con->signal();
            if (signal.isEmpty())
                return signalDefault;
            return signal;
        }
        case 2: {
            const QString receiver = con->receiver();
            if (receiver.isEmpty())
                return receiverDefault;
            return receiver;
        }
        case 3: {
            const QString slot = con->slot();
            if (slot.isEmpty())
                return slotDefault;
            return slot;
        }
    }
    return QVariant();
}

bool ConnectionModel::setData(const QModelIndex &index, const QVariant &data, int)
{
    if (!index.isValid())
        return false;
    if (data.type() != QVariant::String)
        return false;

    SignalSlotConnection *con = static_cast<SignalSlotConnection*>(m_editor->connection(index.row()));
    QDesignerFormWindowInterface *form = m_editor->formWindow();

    QString s = data.toString();
    switch (index.column()) {
        case 0: {
            if (!s.isEmpty() && !objectNameList(form).contains(s))
                s.clear();
            m_editor->setSource(con, s);
            break;
        }
        case 1: {
            if (!memberList(form, con->object(CETypes::EndPoint::Source), SignalMember).contains(s))
                s.clear();
            m_editor->setSignal(con, s);
            break;
        }
        case 2: {
            if (!s.isEmpty() && !objectNameList(form).contains(s))
                s.clear();
            m_editor->setTarget(con, s);
            break;
        }
        case 3: {
            if (!memberList(form, con->object(CETypes::EndPoint::Target), SlotMember).contains(s))
                s.clear();
            m_editor->setSlot(con, s);
            break;
        }
    }

    return true;
}

void ConnectionModel::connectionAdded(Connection*)
{
    endInsertRows();
}

void ConnectionModel::connectionRemoved(int)
{
    endRemoveRows();
}

void ConnectionModel::aboutToRemoveConnection(Connection *con)
{
    int idx = m_editor->indexOfConnection(con);
    beginRemoveRows(QModelIndex(), idx, idx);
}

void ConnectionModel::aboutToAddConnection(int idx)
{
    beginInsertRows(QModelIndex(), idx, idx);
}

Qt::ItemFlags ConnectionModel::flags(const QModelIndex&) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

void ConnectionModel::connectionChanged(Connection *con)
{
    const int idx = m_editor->indexOfConnection(con);
    SignalSlotConnection *changedCon = static_cast<SignalSlotConnection*>(m_editor->connection(idx));
    SignalSlotConnection *c = 0;
    for (int i=0; i<m_editor->connectionCount(); ++i) {
        if (i == idx)
            continue;
        c = static_cast<SignalSlotConnection*>(m_editor->connection(i));
        if (c->sender() == changedCon->sender() && c->signal() == changedCon->signal()
            && c->receiver() == changedCon->receiver() && c->slot() == changedCon->slot()) {
                QMessageBox::warning(m_editor->parentWidget(), tr("Signal and Slot Editor"),
                    tr("The connection already exists!<br>SENDER(%1), SIGNAL(%2), RECEIVER(%3), SLOT(%4)")
                    .arg(changedCon->sender())
                    .arg(changedCon->signal())
                    .arg(changedCon->receiver())
                    .arg(changedCon->slot()));
                break;
        }
    }
    emit dataChanged(createIndex(idx, 0), createIndex(idx, 3));
}

void ConnectionModel::updateAll()
{
    emit dataChanged(index(0, 0), index(rowCount(), columnCount()));
}

/*******************************************************************************
** InlineEditor
*/

#define TITLE_ITEM 1

class InlineEditorModel : public QStandardItemModel
{
    Q_OBJECT
public:
    InlineEditorModel(int rows, int cols, QObject *parent = 0);

    void addTitle(const QString &title);
    void addTextList(const QStringList &text_list);
    void addText(const QString &text);
    bool isTitle(int idx) const;

    int findText(const QString &text) const;

    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
};

class InlineEditor : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText USER true)
public:
    InlineEditor(QWidget *parent = 0);
    ~InlineEditor();

    QString text() const;
    void setText(const QString &text);

    void addTitle(const QString &title);
    void addText(const QString &text);
    void addTextList(const QStringList &text_list);

private slots:
    void checkSelection(int idx);

private:
    InlineEditorModel *m_model;
    int m_idx;
};

InlineEditorModel::InlineEditorModel(int rows, int cols, QObject *parent)
    : QStandardItemModel(rows, cols, parent)
{
}

void InlineEditorModel::addTitle(const QString &title)
{
    const int cnt = rowCount();
    insertRows(cnt, 1);
    QModelIndex cat_idx = index(cnt, 0);
    setData(cat_idx, title + QLatin1Char(':'), Qt::DisplayRole);
    setData(cat_idx, TITLE_ITEM, Qt::UserRole);
    QFont font = QApplication::font();
    font.setBold(true);
    setData(cat_idx, font, Qt::FontRole);
}

bool InlineEditorModel::isTitle(int idx) const
{
    if (idx == -1)
        return false;

    return data(index(idx, 0), Qt::UserRole).toInt() == TITLE_ITEM;
}

void InlineEditorModel::addText(const QString &text)
{
    const int cnt = rowCount();
    insertRows(cnt, 1);
    setData(index(cnt, 0), text, Qt::DisplayRole);
}

void InlineEditorModel::addTextList(const QStringList &text_list)
{
    int cnt = rowCount();
    insertRows(cnt, text_list.size());
    foreach (QString text, text_list) {
        QModelIndex text_idx = index(cnt++, 0);
        setData(text_idx, text, Qt::DisplayRole);
    }
}

Qt::ItemFlags InlineEditorModel::flags(const QModelIndex &index) const
{
    if (isTitle(index.row()))
        return Qt::ItemIsEnabled;
    else
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

int InlineEditorModel::findText(const QString &text) const
{
    const int cnt = rowCount();
    for (int i = 0; i < cnt; ++i) {
        QModelIndex idx = index(i, 0);
        if (data(idx, Qt::UserRole).toInt() == TITLE_ITEM)
            continue;
        if (data(idx, Qt::DisplayRole).toString() == text)
            return i;
    }
    return -1;
}

InlineEditor::InlineEditor(QWidget *parent)
    : QComboBox(parent)
{
    setModel(m_model = new InlineEditorModel(0, 4, this));
    setFrame(false);
    m_idx = -1;
    connect(this, SIGNAL(activated(int)), this, SLOT(checkSelection(int)));
}

InlineEditor::~InlineEditor()
{
}

void InlineEditor::checkSelection(int idx)
{
    if (idx == m_idx)
        return;

   if (m_model->isTitle(idx))
        setCurrentIndex(m_idx);
    else
        m_idx = idx;
}

void InlineEditor::addTitle(const QString &title)
{
    m_model->addTitle(title);
}

void InlineEditor::addTextList(const QStringList &text_list)
{
    m_model->addTextList(text_list);
}

void InlineEditor::addText(const QString &text)
{
    m_model->addText(text);
}

QString InlineEditor::text() const
{
    return currentText();
}

void InlineEditor::setText(const QString &text)
{
    m_idx = m_model->findText(text);
    if (m_idx == -1)
        m_idx = 0;
    setCurrentIndex(m_idx);
}

/*******************************************************************************
** ConnectionDelegate
*/

class ConnectionDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    ConnectionDelegate(QWidget *parent = 0);

    void setForm(QDesignerFormWindowInterface *form);

    virtual QWidget *createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const;

private slots:
    void emitCommitData();

private:
    QDesignerFormWindowInterface *m_form;
};

ConnectionDelegate::ConnectionDelegate(QWidget *parent)
    : QItemDelegate(parent)
{
    m_form = 0;

    static QItemEditorFactory *factory = 0;
    if (factory == 0) {
        factory = new QItemEditorFactory;
        QItemEditorCreatorBase *creator
            = new QItemEditorCreator<InlineEditor>("text");
        factory->registerEditor(QVariant::String, creator);
    }

    setItemEditorFactory(factory);
}

void ConnectionDelegate::setForm(QDesignerFormWindowInterface *form)
{
    m_form = form;
}

QWidget *ConnectionDelegate::createEditor(QWidget *parent,
                                                const QStyleOptionViewItem &option,
                                                const QModelIndex &index) const
{
    if (m_form == 0)
        return 0;

    QWidget *w = QItemDelegate::createEditor(parent, option, index);
    InlineEditor *inline_editor = qobject_cast<InlineEditor*>(w);
    Q_ASSERT(inline_editor != 0);
    const QAbstractItemModel *model = index.model();

    QModelIndex obj_name_idx = model->index(index.row(), index.column() <= 1 ? 0 : 2);
    QString obj_name = model->data(obj_name_idx, Qt::DisplayRole).toString();

    if (index.column() == 0 || index.column() == 2) { // object names
        QStringList obj_name_list = objectNameList(m_form);
        obj_name_list.prepend(tr("<object>"));
        inline_editor->addTextList(obj_name_list);
    } else { // signals, slots
        MemberType type = index.column() == 1 ? SignalMember : SlotMember;
        QModelIndex peer_index = model->index(index.row(), type == SignalMember ? 3 : 1);
        QString peer = model->data(peer_index, Qt::DisplayRole).toString();
        ClassList class_list = classList(obj_name, type, peer, m_form);

        inline_editor->addText(type == SignalMember ? tr("<signal>") : tr("<slot>"));
        foreach (const ClassInfo &class_info, class_list) {
            if (class_info.class_name.isEmpty() || class_info.member_list.isEmpty())
                continue;
            inline_editor->addTitle(class_info.class_name);
            inline_editor->addTextList(class_info.member_list);
        }
    }

    connect(inline_editor, SIGNAL(activated(int)), this, SLOT(emitCommitData()));

    return inline_editor;
}

void ConnectionDelegate::emitCommitData()
{
    InlineEditor *editor = qobject_cast<InlineEditor*>(sender());
    emit commitData(editor);
}

/*******************************************************************************
** SignalSlotEditorWindow
*/

SignalSlotEditorWindow::SignalSlotEditorWindow(QDesignerFormEditorInterface *core,
                                                QWidget *parent)
    : QWidget(parent)
{
    m_handling_selection_change = false;

    m_editor = 0;
    m_view = new QTreeView(this);
    m_view->setItemDelegate(new ConnectionDelegate(this));
    m_view->setEditTriggers(QAbstractItemView::DoubleClicked
                                | QAbstractItemView::EditKeyPressed);
    m_view->setRootIsDecorated(false);
    m_view->setTextElideMode (Qt::ElideMiddle);
    connect(m_view, SIGNAL(activated(QModelIndex)), this, SLOT(updateUi()));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_view);

    QHBoxLayout *layout2 = new QHBoxLayout;
    layout2->setMargin(3);
    layout->addLayout(layout2);
    layout2->addStretch();

    m_remove_button = new QToolButton(this);
    m_remove_button->setIcon(createIconSet(QLatin1String("minus.png")));
    connect(m_remove_button, SIGNAL(clicked()), this, SLOT(removeConnection()));
    layout2->addWidget(m_remove_button);

    m_add_button = new QToolButton(this);
    m_add_button->setIcon(createIconSet(QLatin1String("plus.png")));
    connect(m_add_button, SIGNAL(clicked()), this, SLOT(addConnection()));
    layout2->addWidget(m_add_button);

    connect(core->formWindowManager(),
            SIGNAL(activeFormWindowChanged(QDesignerFormWindowInterface*)),
                this, SLOT(setActiveFormWindow(QDesignerFormWindowInterface*)));

    updateUi();

    m_core = core;
}

void SignalSlotEditorWindow::setActiveFormWindow(QDesignerFormWindowInterface *form)
{
    m_view->setModel(0);
    QDesignerIntegration *integration = qobject_cast<QDesignerIntegration *>(m_core->integration());

    if (!m_editor.isNull()) {
        disconnect(m_view->selectionModel(),
                    SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                    this, SLOT(updateEditorSelection(QModelIndex)));
        disconnect(m_editor, SIGNAL(connectionSelected(Connection*)),
                    this, SLOT(updateDialogSelection(Connection*)));
        if (integration) {
            disconnect(integration, SIGNAL(objectNameChanged(QDesignerFormWindowInterface *, QObject *, const QString &)),
                    this, SLOT(objectNameChanged(QDesignerFormWindowInterface *, QObject *, const QString &)));
        }
    }

    m_editor = qFindChild<SignalSlotEditor*>(form);

    if (!m_editor.isNull()) {
        m_view->setModel(m_editor->model());
        ConnectionDelegate *delegate
            = qobject_cast<ConnectionDelegate*>(m_view->itemDelegate());
        if (delegate != 0)
            delegate->setForm(form);

        connect(m_view->selectionModel(),
                SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                this, SLOT(updateEditorSelection(QModelIndex)));
        connect(m_editor, SIGNAL(connectionSelected(Connection*)),
                this, SLOT(updateDialogSelection(Connection*)));
        if (integration) {
            connect(integration, SIGNAL(objectNameChanged(QDesignerFormWindowInterface *, QObject *, const QString &)),
                    this, SLOT(objectNameChanged(QDesignerFormWindowInterface *, QObject *, const QString &)));
        }
    }

    updateUi();
}

void SignalSlotEditorWindow::updateDialogSelection(Connection *con)
{
    if (m_handling_selection_change || m_editor == 0)
        return;

    ConnectionModel *model = qobject_cast<ConnectionModel*>(m_editor->model());
    Q_ASSERT(model != 0);
    QModelIndex index = model->connectionToIndex(con);
    if (index == m_view->currentIndex())
        return;
    m_handling_selection_change = true;
    m_view->setCurrentIndex(index);
    m_handling_selection_change = false;

    updateUi();
}

void SignalSlotEditorWindow::updateEditorSelection(const QModelIndex &index)
{
    if (m_handling_selection_change || m_editor == 0)
        return;

    if (m_editor == 0)
        return;

    ConnectionModel *model = qobject_cast<ConnectionModel*>(m_editor->model());
    Q_ASSERT(model != 0);
    Connection *con = model->indexToConnection(index);
    if (m_editor->selected(con))
        return;
    m_handling_selection_change = true;
    m_editor->selectNone();
    m_editor->setSelected(con, true);
    m_handling_selection_change = false;

    updateUi();
}

void SignalSlotEditorWindow::objectNameChanged(QDesignerFormWindowInterface *, QObject *, const QString &)
{
    if (m_editor && m_editor->model()) {
        ConnectionModel *model = qobject_cast<ConnectionModel*>(m_editor->model());
        model->updateAll();
    }
}

void SignalSlotEditorWindow::addConnection()
{
    if (m_editor.isNull())
        return;

    m_editor->addEmptyConnection();
    updateUi();
}

void SignalSlotEditorWindow::removeConnection()
{
    if (m_editor.isNull())
        return;

    m_editor->deleteSelected();
    updateUi();
}

void SignalSlotEditorWindow::updateUi()
{
    m_add_button->setEnabled(!m_editor.isNull());
    m_remove_button->setEnabled(!m_editor.isNull() && m_view->currentIndex().isValid());
}

} // namespace qdesigner_internal

#include "signalsloteditorwindow.moc"
