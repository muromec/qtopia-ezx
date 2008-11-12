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

#include "qdesigner_taskmenu_p.h"
#include "qdesigner_command_p.h"
#include "richtexteditor_p.h"
#include "stylesheeteditor_p.h"
#include "qlayout_widget_p.h"
#include "layout_p.h"
#include "spacer_widget_p.h"
#include "textpropertyeditor_p.h"
#include "promotiontaskmenu_p.h"
#include "metadatabase_p.h"
#include "scriptdialog_p.h"
#include "scriptcommand_p.h"

#include <shared_enums_p.h>

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormWindowCursorInterface>
#include <QtDesigner/QDesignerPropertySheetExtension>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerLanguageExtension>
#include <QtDesigner/QExtensionManager>

#include <QtGui/QAction>
#include <QtGui/QWidget>
#include <QtGui/QMenuBar>
#include <QtGui/QMainWindow>
#include <QtGui/QStatusBar>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>

static QMenuBar *findMenuBar(const QWidget *widget)
{
    const QList<QObject*> children = widget->children();
    foreach (QObject *obj, widget->children()) {
        if (QMenuBar *mb = qobject_cast<QMenuBar*>(obj)) {
            return mb;
        }
    }

    return 0;
}

static QStatusBar *findStatusBar(const QWidget *widget)
{
    const QList<QObject*> children = widget->children();
    foreach (QObject *obj, widget->children()) {
        if (QStatusBar *sb = qobject_cast<QStatusBar*>(obj)) {
            return sb;
        }
    }

    return 0;
}

namespace  {
class ObjectNameDialog : public QDialog
{
     public:
         ObjectNameDialog(QWidget *parent, const QString &oldName);
         QString newObjectName() const;

     private:
         qdesigner_internal::TextPropertyEditor *m_editor;
};

ObjectNameDialog::ObjectNameDialog(QWidget *parent, const QString &oldName)
    : QDialog(parent),
      m_editor( new qdesigner_internal::TextPropertyEditor(qdesigner_internal::TextPropertyEditor::EmbeddingNone,
                                                           qdesigner_internal::ValidationObjectName, this))
{
    setWindowTitle(QObject::tr("Change Object Name"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *vboxLayout = new QVBoxLayout(this);
    vboxLayout->addWidget(new QLabel(QObject::tr("Object Name")));

    m_editor->setText(oldName);
    m_editor->selectAll();
    m_editor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    vboxLayout->addWidget(m_editor);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    vboxLayout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

QString ObjectNameDialog::newObjectName() const
{
    return m_editor->text();
}

}

namespace qdesigner_internal {

QDesignerTaskMenu::QDesignerTaskMenu(QWidget *widget, QObject *parent) :
    QObject(parent),
    m_widget(widget),
    m_separator(createSeparator()),
    m_separator2(createSeparator()),
    m_separator3(createSeparator()),
    m_separator4(createSeparator()),
    m_changeObjectNameAction(new QAction(tr("Change objectName..."), this)),
    m_changeToolTip(new QAction(tr("Change toolTip..."), this)),
    m_changeWhatsThis(new QAction(tr("Change whatsThis..."), this)),
    m_changeStyleSheet(new QAction(tr("Change styleSheet..."), this)),
    m_addMenuBar(new QAction(tr("Create Menu Bar"), this)),
    m_addToolBar(new QAction(tr("Add Tool Bar"), this)),
    m_addStatusBar(new QAction(tr("Create Status Bar"), this)),
    m_removeStatusBar(new QAction(tr("Remove Status Bar"), this)),
    m_changeScript(new QAction(tr("Change script..."), this)),
    m_promotionTaskMenu(new PromotionTaskMenu(widget, PromotionTaskMenu::ModeMultiSelection, this))
{
    Q_ASSERT(qobject_cast<QDesignerFormWindowInterface*>(widget) == 0);

    connect(m_changeObjectNameAction, SIGNAL(triggered()), this, SLOT(changeObjectName()));
    connect(m_changeToolTip, SIGNAL(triggered()), this, SLOT(changeToolTip()));
    connect(m_changeWhatsThis, SIGNAL(triggered()), this, SLOT(changeWhatsThis()));
    connect(m_changeStyleSheet, SIGNAL(triggered()), this,  SLOT(changeStyleSheet()));
    connect(m_addMenuBar, SIGNAL(triggered()), this, SLOT(createMenuBar()));
    connect(m_addToolBar, SIGNAL(triggered()), this, SLOT(addToolBar()));
    connect(m_addStatusBar, SIGNAL(triggered()), this, SLOT(createStatusBar()));
    connect(m_removeStatusBar, SIGNAL(triggered()), this, SLOT(removeStatusBar()));
    connect(m_changeScript, SIGNAL(triggered()), this, SLOT(changeScript()));
}

QDesignerTaskMenu::~QDesignerTaskMenu()
{
}

QAction *QDesignerTaskMenu::createSeparator() {
    QAction *rc = new QAction(this);
    rc->setSeparator(true);
    return rc;
}

QWidget *QDesignerTaskMenu::widget() const
{
    return m_widget;
}

QDesignerFormWindowInterface *QDesignerTaskMenu::formWindow() const
{
    QDesignerFormWindowInterface *result = QDesignerFormWindowInterface::findFormWindow(widget());
    Q_ASSERT(result != 0);
    return result;
}

void QDesignerTaskMenu::createMenuBar()
{
    if (QDesignerFormWindowInterface *fw = formWindow()) {
        QMainWindow *mw = qobject_cast<QMainWindow*>(fw->mainContainer());
        if (!mw) {
            // ### warning message
            return;
        }

        CreateMenuBarCommand *cmd = new CreateMenuBarCommand(fw);
        cmd->init(mw);
        fw->commandHistory()->push(cmd);
    }
}

void QDesignerTaskMenu::addToolBar()
{
    if (QDesignerFormWindowInterface *fw = formWindow()) {
        QMainWindow *mw = qobject_cast<QMainWindow*>(fw->mainContainer());
        if (!mw) {
            // ### warning message
            return;
        }

        AddToolBarCommand *cmd = new AddToolBarCommand(fw);
        cmd->init(mw);
        fw->commandHistory()->push(cmd);
    }
}

void QDesignerTaskMenu::createStatusBar()
{
    if (QDesignerFormWindowInterface *fw = formWindow()) {
        QMainWindow *mw = qobject_cast<QMainWindow*>(fw->mainContainer());
        if (!mw) {
            // ### warning message
            return;
        }

        CreateStatusBarCommand *cmd = new CreateStatusBarCommand(fw);
        cmd->init(mw);
        fw->commandHistory()->push(cmd);
    }
}

void QDesignerTaskMenu::removeStatusBar()
{
    if (QDesignerFormWindowInterface *fw = formWindow()) {
        QMainWindow *mw = qobject_cast<QMainWindow*>(fw->mainContainer());
        if (!mw) {
            // ### warning message
            return;
        }

        DeleteStatusBarCommand *cmd = new DeleteStatusBarCommand(fw);
        cmd->init(findStatusBar(mw));
        fw->commandHistory()->push(cmd);
    }
}

QList<QAction*> QDesignerTaskMenu::taskActions() const
{
    QDesignerFormWindowInterface *formWindow = QDesignerFormWindowInterface::findFormWindow(widget());
    Q_ASSERT(formWindow);

    const bool isMainContainer = formWindow->mainContainer() == widget();

    QList<QAction*> actions;

    if (const QMainWindow *mw = qobject_cast<const QMainWindow*>(formWindow->mainContainer()))  {
        if (isMainContainer || mw->centralWidget() == widget()) {
            if (!findMenuBar(mw)) {
                actions.append(m_addMenuBar);
            }

            actions.append(m_addToolBar);
            // ### create the status bar
            if (!findStatusBar(mw))
                actions.append(m_addStatusBar);
            else
                actions.append(m_removeStatusBar);
            actions.append(m_separator);
        }
    }
    actions.append(m_changeObjectNameAction);
    actions.append(m_separator2);
    actions.append(m_changeToolTip);
    actions.append(m_changeWhatsThis);
    actions.append(m_changeStyleSheet);

    m_promotionTaskMenu->addActions(formWindow, PromotionTaskMenu::LeadingSeparator, actions);

#ifdef WANT_SCRIPT_OPTION
    if (!isMainContainer) {
        actions.append(m_separator4);
        actions.append(m_changeScript);
    }
#endif
    return actions;
}

void QDesignerTaskMenu::changeObjectName()
{
    QDesignerFormWindowInterface *fw = formWindow();
    Q_ASSERT(fw != 0);

    QDesignerFormEditorInterface *core = fw->core();
    QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(core->extensionManager(), widget());
    Q_ASSERT(sheet != 0);

    ObjectNameDialog dialog(fw, sheet->property(sheet->indexOf(QLatin1String("objectName"))).toString());
    if (dialog.exec() == QDialog::Accepted) {
        const QString newObjectName = dialog.newObjectName();
        if (!newObjectName.isEmpty())
            fw->cursor()->setProperty(QLatin1String("objectName"), newObjectName);
    }
}


QDesignerTaskMenuFactory::QDesignerTaskMenuFactory(QExtensionManager *extensionManager)
    : QExtensionFactory(extensionManager)
{
}

QObject *QDesignerTaskMenuFactory::createExtension(QObject *object, const QString &iid, QObject *parent) const
{
    if (iid != Q_TYPEID(QDesignerTaskMenuExtension))
        return 0;

    QWidget *widget = qobject_cast<QWidget*>(object);
    if (widget == 0)
        return 0;

    // check if is an internal widget (### generalize)
    if (qobject_cast<const QLayoutWidget*>(widget) || qobject_cast<const Spacer*>(widget))
        return 0;

    return new QDesignerTaskMenu(widget, parent);
}

void QDesignerTaskMenu::changeRichTextProperty(const QString &propertyName)
{
    if (QDesignerFormWindowInterface *fw = formWindow()) {
        Q_ASSERT(m_widget->parentWidget() != 0);

        RichTextEditorDialog dlg(fw);
        RichTextEditor *editor = dlg.editor();

        QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(fw->core()->extensionManager(), m_widget);
        Q_ASSERT(sheet != 0);

        editor->setDefaultFont(m_widget->font());
        editor->setText(sheet->property(sheet->indexOf(propertyName)).toString());
        editor->selectAll();
        editor->setFocus();

        if (dlg.exec()) {
            const QString text = editor->text(Qt::RichText);
            fw->cursor()->setWidgetProperty(m_widget, propertyName, QVariant(text));
        }
    }
}

void QDesignerTaskMenu::changeToolTip()
{
    changeRichTextProperty(QLatin1String("toolTip"));
}

void QDesignerTaskMenu::changeWhatsThis()
{
    changeRichTextProperty(QLatin1String("whatsThis"));
}

void QDesignerTaskMenu::changeStyleSheet()
{
    if (QDesignerFormWindowInterface *fw = formWindow()) {
        StyleSheetEditorDialog dlg(fw, m_widget);
        dlg.exec();
    }
}

void QDesignerTaskMenu::changeScript()
{
    QDesignerFormWindowInterface *fw = formWindow();
    if (!fw)
        return;

    MetaDataBase *metaDataBase = qobject_cast<MetaDataBase*>(fw->core()->metaDataBase());
    if (!metaDataBase)
        return;

    const MetaDataBaseItem* item = metaDataBase->metaDataBaseItem(m_widget);
    if (!item)
        return;

    const QString oldScript = item->script();
    QString newScript = oldScript;
   
    ScriptDialog scriptDialog(fw);
    if (!scriptDialog.editScript(newScript))
        return;
    
    // compile list of selected objects
    ScriptCommand::ObjectList objects;
    objects += (QWidget *)m_widget;

    const QDesignerFormWindowCursorInterface *cursor = fw->cursor();
    const int selectionCount =  cursor->selectedWidgetCount();
    for (int i = 0; i < selectionCount; i++) {
        QWidget *w = cursor->selectedWidget(i);
        if (w != m_widget)
             objects += w;
    }
    ScriptCommand *scriptCommand = new ScriptCommand(fw);
    if (!scriptCommand->init(objects, newScript)) {
        delete scriptCommand;
        return;
    }
    
    fw->commandHistory()->push(scriptCommand); 
}
} // namespace qdesigner_internal
