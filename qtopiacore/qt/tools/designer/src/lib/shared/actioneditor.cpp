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
TRANSLATOR qdesigner_internal::ActionEditor
*/

#include "actioneditor_p.h"
#include "actionrepository_p.h"
#include "iconloader_p.h"
#include "newactiondialog_p.h"
#include "qdesigner_menu_p.h"
#include "qdesigner_command_p.h"
#include "qdesigner_propertycommand_p.h"
#include "resourcemimedata_p.h"
#include "qdesigner_objectinspector_p.h"
#include "qdesigner_utils_p.h"

#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerPropertyEditorInterface>
#include <QtDesigner/QDesignerPropertySheetExtension>
#include <QtDesigner/QExtensionManager>
#include <QtDesigner/QDesignerMetaDataBaseInterface>
#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerIconCacheInterface>

#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QListWidget>
#include <QtGui/QToolBar>
#include <QtGui/QSplitter>
#include <QtGui/QAction>
#include <QtGui/QItemDelegate>
#include <QtGui/QPainter>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

#include <QtCore/QRegExp>

#include <qdebug.h>

Q_DECLARE_METATYPE(QAction*)
Q_DECLARE_METATYPE(QListWidgetItem*)

namespace qdesigner_internal {
//-------- ActionFilterWidget
class ActionFilterWidget: public QWidget
{
    Q_OBJECT
public:
    ActionFilterWidget(ActionEditor *actionEditor, QToolBar *parent)
        : QWidget(parent),
          m_button(new QPushButton(this)),
          m_editor(new QLineEdit(this)),
          m_actionEditor(actionEditor)
    {
        QHBoxLayout *l = new QHBoxLayout(this);
        l->setMargin(0);
        l->setSpacing(0);

        l->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

        QLabel *label = new QLabel(tr("Filter: "), this);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        l->addWidget(label);

        l->addWidget(m_editor);

        connect(m_editor, SIGNAL(textChanged(QString)), actionEditor, SLOT(setFilter(QString)));

        m_button->setIcon(createIconSet(QLatin1String("resetproperty.png")));
        m_button->setIconSize(QSize(16, 16));
        m_button->setFlat(true);
        l->addWidget(m_button);
        connect(m_button, SIGNAL(clicked()), m_editor, SLOT(clear()));
        connect(m_editor, SIGNAL(textChanged(QString)), this, SLOT(checkButton(QString)));
    }

private slots:
    void checkButton(const QString &text)
    {
        m_button->setEnabled(!text.isEmpty());
    }

private:
    QPushButton *m_button;
    QLineEdit *m_editor;
    ActionEditor *m_actionEditor;
};

//--------  ActionGroupDelegate
class ActionGroupDelegate: public QItemDelegate
{
public:
    ActionGroupDelegate(QObject *parent)
        : QItemDelegate(parent) {}

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());

        QItemDelegate::paint(painter, option, index);
    }

    virtual void drawFocus(QPainter * /*painter*/, const QStyleOptionViewItem &/*option*/, const QRect &/*rect*/) const {}
};

//--------  ActionEditor
ActionEditor::ActionEditor(QDesignerFormEditorInterface *core, QWidget *parent, Qt::WindowFlags flags)
    : QDesignerActionEditorInterface(parent, flags),
      m_core(core)
{
    setWindowTitle(tr("Actions"));

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setMargin(0);
    l->setSpacing(0);

    QToolBar *toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(24, 24));
    toolbar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    l->addWidget(toolbar);

    m_actionNew = toolbar->addAction(tr("New..."));
    m_actionNew->setIcon(createIconSet(QLatin1String("filenew.png")));
    m_actionNew->setEnabled(false);
    connect(m_actionNew, SIGNAL(triggered()), this, SLOT(slotNewAction()));

    m_actionDelete = toolbar->addAction(tr("Delete"));
    m_actionDelete->setIcon(createIconSet(QLatin1String("editdelete.png")));
    m_actionDelete->setEnabled(false);

    m_filterWidget = new ActionFilterWidget(this, toolbar);
    m_filterWidget->setEnabled(false);
    toolbar->addWidget(m_filterWidget);

    connect(m_actionDelete, SIGNAL(triggered()), this, SLOT(slotDeleteAction()));

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    l->addWidget(splitter);

#if 0 // ### implement me
    m_actionGroups = new QListWidget(splitter);
    splitter->addWidget(m_actionGroups);
    m_actionGroups->setItemDelegate(new ActionGroupDelegate(m_actionGroups));
    m_actionGroups->setMovement(QListWidget::Static);
    m_actionGroups->setResizeMode(QListWidget::Fixed);
    m_actionGroups->setIconSize(QSize(48, 48));
    m_actionGroups->setFlow(QListWidget::TopToBottom);
    m_actionGroups->setViewMode(QListWidget::IconMode);
    m_actionGroups->setWrapping(false);
#endif

    m_actionRepository = new ActionRepository(splitter);
    connect(m_actionRepository, SIGNAL(resourceImageDropped(ResourceMimeData,QAction*)),
            this, SLOT(resourceImageDropped(ResourceMimeData,QAction*)));
    splitter->addWidget(m_actionRepository);

    connect(m_actionRepository, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(slotItemChanged(QListWidgetItem*)));
    // make it possible for vs integration to reimplement edit action dialog
    connect(m_actionRepository, SIGNAL(itemActivated(QListWidgetItem*)),
            this, SIGNAL(itemActivated(QListWidgetItem*)));
    connect(m_actionRepository, SIGNAL(contextMenuRequested(QContextMenuEvent*, QListWidgetItem*)),
            this, SIGNAL(contextMenuRequested(QContextMenuEvent*, QListWidgetItem*)));
    connect(this, SIGNAL(itemActivated(QListWidgetItem*)),
            this, SLOT(editAction(QListWidgetItem*)));
}

ActionEditor::~ActionEditor()
{
}

QAction *ActionEditor::actionNew() const
{
    return m_actionNew;
}

QAction *ActionEditor::actionDelete() const
{
    return m_actionDelete;
}

QDesignerFormWindowInterface *ActionEditor::formWindow() const
{
    return m_formWindow;
}

void ActionEditor::setFormWindow(QDesignerFormWindowInterface *formWindow)
{
    if (formWindow != 0 && formWindow->mainContainer() == 0)
        formWindow = 0;

    // we do NOT rely on this function to update the action editor
    if (m_formWindow == formWindow)
        return;

    if (m_formWindow != 0) {
        const QList<QAction*> actionList = qFindChildren<QAction*>(m_formWindow->mainContainer());
        foreach (QAction *action, actionList)
            disconnect(action, SIGNAL(changed()), this, SLOT(slotActionChanged()));
    }

    m_formWindow = formWindow;

    m_actionRepository->clear();

    if (!formWindow || !formWindow->mainContainer()) {
        m_actionNew->setEnabled(false);
        m_actionDelete->setEnabled(false);
        m_filterWidget->setEnabled(false);
        return;
    }

    m_actionNew->setEnabled(true);
    m_filterWidget->setEnabled(true);

    const QList<QAction*> actionList = qFindChildren<QAction*>(formWindow->mainContainer());
    foreach (QAction *action, actionList) {
        if (!core()->metaDataBase()->item(action)
            || action->isSeparator()
            // ### || action->menu()
            ) {
            continue;
        }

        createListWidgetItem(action);
        connect(action, SIGNAL(changed()), this, SLOT(slotActionChanged()));
    }

    setFilter(m_filter);
}

static QIcon fixActionIcon(QIcon icon)
{
    if (icon.isNull())
        return emptyIcon();
    return icon;
}

// Set up list widget item, icon, tooltip
static void setListWidgetItem(const QAction *action,  QListWidgetItem *item)
{
    item->setText(action->objectName());
    item->setIcon(fixActionIcon(action->icon()));

    QString tooltip = action->objectName();
    const QString text = action->text();
    if (!text.isEmpty()) {
        tooltip += QLatin1Char('\n');
        tooltip += text;
    }
    item->setToolTip(tooltip);
    item->setWhatsThis(tooltip);
}

QListWidgetItem *ActionEditor::createListWidgetItem(QAction *action)
{
    if (action->menu())
        return 0;

    QListWidgetItem *item = new QListWidgetItem(m_actionRepository);
    const QSize s = m_actionRepository->iconSize();
    item->setSizeHint(QSize(s.width() * 3, s.height() * 2));

    setListWidgetItem(action, item);

    QVariant itemData;
    qVariantSetValue(itemData, action);
    item->setData(ActionRepository::ActionRole, itemData);
    item->setFlags(item->flags() | Qt::ItemIsDropEnabled);
    return item;
}

void ActionEditor::slotItemChanged(QListWidgetItem *item)
{
    QDesignerFormWindowInterface *fw = formWindow();
    if (!fw)
        return;

    QAction *action = 0;
    if (item)
        action = itemToAction(item);

    m_actionDelete->setEnabled(action != 0);

    if (!action) {
        fw->clearSelection();
        return;
    }

    QDesignerObjectInspector *oi = qobject_cast<QDesignerObjectInspector *>(core()->objectInspector());

    if (action->associatedWidgets().empty()) {
        // Special case: action not in object tree. Unselect all and set in property editor
        fw->clearSelection(false);
        if (oi)
            oi->clearSelection();
        core()->propertyEditor()->setObject(action);
    } else {
        if (oi)
            oi->selectObject(action);
    }
}

QListWidgetItem *ActionEditor::actionToItem(QAction *action) const
{
    const int cnt = m_actionRepository->count();
    for (int i = 0; i < cnt; ++i) {
        QListWidgetItem *item = m_actionRepository->item(i);
        if (itemToAction(item) == action)
            return item;
    }
    return 0;
}

QAction *ActionEditor::itemToAction(QListWidgetItem *item) const
{
    return qvariant_cast<QAction*>(item->data(ActionRepository::ActionRole));
}

void ActionEditor::slotActionChanged()
{
    QAction *action = qobject_cast<QAction*>(sender());
    Q_ASSERT(action != 0);

    QListWidgetItem *item = actionToItem(action);
    if (item == 0) {
        if (action->menu() == 0) // action got its menu deleted, create item
            createListWidgetItem(action);
    } else if (action->menu() != 0) { // action got its menu created, remove item
        delete item;
    } else {
        // action text or icon changed, update item
        setListWidgetItem(action, item);
    }
}

QDesignerFormEditorInterface *ActionEditor::core() const
{
    return m_core;
}

QString ActionEditor::filter() const
{
    return m_filter;
}

void ActionEditor::setFilter(const QString &f)
{
    m_filter = f;
    m_actionRepository->filter(m_filter);
}

// Set changed state of icon property,  reset when icon is cleared
static void refreshIconPropertyChanged(const QAction *action, QDesignerPropertySheetExtension *sheet)
{
    sheet->setChanged(sheet->indexOf(QLatin1String("icon")), !action->icon().isNull());
}

void ActionEditor::manageAction(QAction *action)
{
    action->setParent(formWindow()->mainContainer());
    core()->metaDataBase()->add(action);

    if (action->isSeparator() || action->menu() != 0)
        return;

    QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(core()->extensionManager(), action);
    sheet->setChanged(sheet->indexOf(QLatin1String("objectName")), true);
    sheet->setChanged(sheet->indexOf(QLatin1String("text")), true);
    refreshIconPropertyChanged(action, sheet);

    QListWidgetItem *item = createListWidgetItem(action);
    m_actionRepository->setCurrentItem(item);

    connect(action, SIGNAL(changed()), this, SLOT(slotActionChanged()));
}

void ActionEditor::unmanageAction(QAction *action)
{
    core()->metaDataBase()->remove(action);
    action->setParent(0);

    disconnect(action, SIGNAL(changed()), this, SLOT(slotActionChanged()));

    QListWidgetItem *item = actionToItem(action);
    if (item == 0)
        return;

    delete item;
}

void ActionEditor::slotNewAction()
{
    NewActionDialog dlg(this);
    dlg.setWindowTitle(tr("New action"));

    if (dlg.exec() == QDialog::Accepted) {
        QAction *action = new QAction(formWindow());
        action->setObjectName(dlg.actionName());
        formWindow()->ensureUniqueObjectName(action);
        action->setText(dlg.actionText());
        action->setIcon(dlg.actionIcon());

        AddActionCommand *cmd = new AddActionCommand(formWindow());
        cmd->init(action);
        formWindow()->commandHistory()->push(cmd);
    }
}

static inline bool isSameIcon(const QIcon &i1, const QIcon &i2)
{
    return i1.serialNumber() == i2.serialNumber();
}

// return a FormWindow command to apply an icon
static QDesignerFormWindowCommand *setIconPropertyCommand(const QIcon &newIcon, QAction *action, QDesignerFormWindowInterface *fw)
{
    const QString iconProperty = QLatin1String("icon");
    if (newIcon.isNull()) {
        ResetPropertyCommand *cmd = new ResetPropertyCommand(fw);
        cmd->init(action, iconProperty);
        return cmd;
    }
    SetPropertyCommand *cmd = new SetPropertyCommand(fw);
    cmd->init(action, iconProperty, newIcon);
    return cmd;
}

static QDesignerFormWindowCommand *setTextPropertyCommand(const QString &propertyName, const QString &text, QAction *action, QDesignerFormWindowInterface *fw)
{
    if (text.isEmpty()) {
        ResetPropertyCommand *cmd = new ResetPropertyCommand(fw);
        cmd->init(action, propertyName);
        return cmd;
    }
    SetPropertyCommand *cmd = new SetPropertyCommand(fw);
    cmd->init(action, propertyName, text);
    return cmd;
}

void ActionEditor::editAction(QListWidgetItem *item)
{
    if (!item)
        return;

    QAction *action = itemToAction(item);
    if (action == 0)
        return;

    NewActionDialog dlg(this);
    dlg.setWindowTitle(tr("Edit action"));
    dlg.setActionData(action->text(), action->objectName(), action->icon());

    if (!dlg.exec())
        return;

    // figure out changes and whether to start a macro
    enum ChangedMask { NameChanged = 1, TextChanged = 2 , IconChanged = 4 };
    const QString newName = dlg.actionName();
    const QString newText = dlg.actionText();
    const QIcon newIcon = dlg.actionIcon();

    int changedMask = 0;

    if (newName != action->objectName())
        changedMask |= NameChanged;
    if (newText != action->text())
        changedMask |= TextChanged;
    if (!isSameIcon(newIcon, action->icon()))
        changedMask |= IconChanged;

    if (!changedMask)
        return;

    const bool severalChanges = (changedMask != NameChanged) && (changedMask != TextChanged) && (changedMask != IconChanged);
    if (severalChanges)
        formWindow()->beginCommand(QLatin1String("Edit action"));

    if (changedMask & NameChanged)
        formWindow()->commandHistory()->push(setTextPropertyCommand(QLatin1String("objectName"), newName, action, formWindow()));

    if (changedMask & TextChanged)
        formWindow()->commandHistory()->push(setTextPropertyCommand(QLatin1String("text"), newText, action, formWindow()));

    if (changedMask & IconChanged)
        formWindow()->commandHistory()->push(setIconPropertyCommand(newIcon, action, formWindow()));

    if (severalChanges)
        formWindow()->endCommand();

}

void ActionEditor::slotDeleteAction()
{
    QListWidgetItem *item = m_actionRepository->currentItem();
    if (item == 0)
        return;

    QAction *action = itemToAction(item);
    if (action == 0)
        return;

    RemoveActionCommand *cmd = new RemoveActionCommand(formWindow());
    cmd->init(action);
    formWindow()->commandHistory()->push(cmd);
}

void ActionEditor::slotNotImplemented()
{
    QMessageBox::information(this, tr("Designer"), tr("Feature not implemented!"));
}

QString ActionEditor::actionTextToName(const QString &text, const QString &prefix)
{
    QString name = text;
    if (name.isEmpty())
        return QString();

    name[0] = name.at(0).toUpper();
    name.prepend(prefix);
    const QString underscore = QString(QLatin1Char('_'));
    name.replace(QRegExp(QString(QLatin1String("[^a-zA-Z_0-9]"))), underscore);
    name.replace(QRegExp(QLatin1String("__*")), underscore);
    if (name.endsWith(underscore.at(0)))
        name.truncate(name.size() - 1);

    return name;
}

void  ActionEditor::resourceImageDropped(const ResourceMimeData &data, QAction *action)
{
    QDesignerFormWindowInterface *fw =  formWindow();
    if (!fw)
        return;

    const QIcon icon = resourceMimeDataToIcon(data, fw);

    if (icon.isNull() || isSameIcon(icon, action->icon()))
        return;

    fw->commandHistory()->push(setIconPropertyCommand(icon , action, fw));
}

void ActionEditor::mainContainerChanged()
{
    // Invalidate references to objects kept in model
    if (sender() == formWindow())
        setFormWindow(0);
}
} // namespace qdesigner_internal

#include "actioneditor.moc"
