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

#include "qdesigner_integration_p.h"
#include "qdesigner_propertycommand_p.h"
#include "qdesigner_propertycommentcommand_p.h"
#include "qdesigner_propertyeditor_p.h"
#include "qdesigner_objectinspector_p.h"
#include "widgetdatabase_p.h"
#include "pluginmanager_p.h"
#include "widgetfactory_p.h"
#include "qdesigner_widgetbox_p.h"

// sdk
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormWindowManagerInterface>
#include <QtDesigner/QDesignerFormWindowCursorInterface>
#include <QtDesigner/QDesignerActionEditorInterface>
#include <QtDesigner/QDesignerWidgetBoxInterface>
#include <QtDesigner/QExtensionManager>

#include <QtCore/QVariant>

#include <QtCore/qdebug.h>

namespace qdesigner_internal {

QDesignerIntegration::QDesignerIntegration(QDesignerFormEditorInterface *core, QObject *parent)
    : QDesignerIntegrationInterface(core, parent)
{
    initialize();
}

QDesignerIntegration::~QDesignerIntegration()
{
}

void QDesignerIntegration::initialize()
{
    //
    // integrate the `Form Editor component'
    //
    connect(core()->propertyEditor(), SIGNAL(propertyChanged(QString,QVariant)),
            this, SLOT(updateProperty(QString,QVariant)));


    // Extensions
    if (QDesignerPropertyEditor *designerPropertyEditor= qobject_cast<QDesignerPropertyEditor *>(core()->propertyEditor())) {
        connect(designerPropertyEditor, SIGNAL(propertyCommentChanged(QString, QString)), this, SLOT(updatePropertyComment(QString, QString)));
        connect(designerPropertyEditor, SIGNAL(resetProperty(QString)), this, SLOT(resetProperty(QString)));
        connect(designerPropertyEditor, SIGNAL(addDynamicProperty(QString,QVariant)),
                this, SLOT(addDynamicProperty(QString,QVariant)));
        connect(designerPropertyEditor, SIGNAL(removeDynamicProperty(QString)),
                this, SLOT(removeDynamicProperty(QString)));
    }

    connect(core()->formWindowManager(), SIGNAL(formWindowAdded(QDesignerFormWindowInterface*)),
            this, SLOT(setupFormWindow(QDesignerFormWindowInterface*)));

    connect(core()->formWindowManager(), SIGNAL(activeFormWindowChanged(QDesignerFormWindowInterface*)),
            this, SLOT(updateActiveFormWindow(QDesignerFormWindowInterface*)));
}

void QDesignerIntegration::updateProperty(const QString &name, const QVariant &value)
{
    QDesignerFormWindowInterface *formWindow = core()->formWindowManager()->activeFormWindow();
    if (!formWindow)
        return;

    Selection selection;
    getSelection(selection);
    if (selection.empty())
        return;
    // Legacy: set properties on widgets via cursor
    if (!selection.m_cursorSelection.empty() && selection.m_selectedObjects.empty()) {
        QDesignerFormWindowCursorInterface *cursor = formWindow->cursor();
        cursor->setProperty(name, value);
    } else {
        SetPropertyCommand *cmd = new SetPropertyCommand(formWindow);
        // find a reference object to compare to and to find the right group
        if (cmd->init(selection.selection(), name, value, propertyEditorObject())) {
            formWindow->commandHistory()->push(cmd);
        } else {
            delete cmd;
            qDebug() << "Unable to set  property " << name << '.';
        }
    }

    emit propertyChanged(formWindow, name, value);
}

void QDesignerIntegration::updatePropertyComment(const QString &name, const QString &value)
{
    QDesignerFormWindowInterface *formWindow = core()->formWindowManager()->activeFormWindow();
    if (!formWindow)
        return;

    Selection selection;
    getSelection(selection);
    if (selection.empty())
        return;

    SetPropertyCommentCommand *cmd = new SetPropertyCommentCommand(formWindow);
    if (cmd->init(selection.selection(), name, value)) {
        formWindow->commandHistory()->push(cmd);
    } else {
        delete cmd;
        qDebug() << "** WARNING Unable to update property comment of " << name << '.';
    }

}

void QDesignerIntegration::resetProperty(const QString &name)
{
    QDesignerFormWindowInterface *formWindow = core()->formWindowManager()->activeFormWindow();
    if (!formWindow)
        return;

    Selection selection;
    getSelection(selection);
    if (selection.empty())
        return;


    ResetPropertyCommand *cmd = new ResetPropertyCommand(formWindow);
    // find a reference object to find the right group
    if (cmd->init(selection.selection(), name, propertyEditorObject())) {
        formWindow->commandHistory()->push(cmd);
    } else {
        delete cmd;
        qDebug() << "** WARNING Unable to reset property " << name << '.';
    }
}

void QDesignerIntegration::addDynamicProperty(const QString &name, const QVariant &value)
{
    QDesignerFormWindowInterface *formWindow = core()->formWindowManager()->activeFormWindow();
    if (!formWindow)
        return;

    Selection selection;
    getSelection(selection);
    if (selection.empty())
        return;

    AddDynamicPropertyCommand *cmd = new AddDynamicPropertyCommand(formWindow);
    if (cmd->init(selection.selection(), propertyEditorObject(), name, value)) {
        formWindow->commandHistory()->push(cmd);
    } else {
        delete cmd;
        qDebug() <<  "** WARNING Unable to add dynamic property " << name << '.';
    }
}

void QDesignerIntegration::removeDynamicProperty(const QString &name)
{
    QDesignerFormWindowInterface *formWindow = core()->formWindowManager()->activeFormWindow();
    if (!formWindow)
        return;

    Selection selection;
    getSelection(selection);
    if (selection.empty())
        return;

    RemoveDynamicPropertyCommand *cmd = new RemoveDynamicPropertyCommand(formWindow);
    if (cmd->init(selection.selection(), propertyEditorObject(), name)) {
        formWindow->commandHistory()->push(cmd);
    } else {
        delete cmd;
        qDebug() << "** WARNING Unable to remove dynamic property " << name << '.';
    }

}


void QDesignerIntegration::updateActiveFormWindow(QDesignerFormWindowInterface *formWindow)
{
    Q_UNUSED(formWindow);
    updateSelection();
}

void QDesignerIntegration::setupFormWindow(QDesignerFormWindowInterface *formWindow)
{
    connect(formWindow, SIGNAL(selectionChanged()), this, SLOT(updateSelection()));
    connect(formWindow, SIGNAL(activated(QWidget*)), this, SLOT(activateWidget(QWidget*)));
}

void QDesignerIntegration::updateGeometry()
{
}

void QDesignerIntegration::updateSelection()
{
    QDesignerFormWindowInterface *formWindow = core()->formWindowManager()->activeFormWindow();
    QWidget *selection = 0;

    if (formWindow) {
        selection = formWindow->cursor()->current();
    }

    if (QDesignerActionEditorInterface *actionEditor = core()->actionEditor())
        actionEditor->setFormWindow(formWindow);

    if (QDesignerPropertyEditorInterface *propertyEditor = core()->propertyEditor()) {
        propertyEditor->setObject(selection);
        propertyEditor->setEnabled(formWindow && formWindow->cursor()->selectedWidgetCount());
    }
    if (QDesignerObjectInspectorInterface *objectInspector = core()->objectInspector())
        objectInspector->setFormWindow(formWindow);

}

void QDesignerIntegration::activateWidget(QWidget *widget)
{
    Q_UNUSED(widget);
}

QWidget *QDesignerIntegration::containerWindow(QWidget *widget) const
{
    // Find the parent window to apply a geometry to.
    while (widget) {
        if (widget->isWindow())
            break;
        if (!qstrcmp(widget->metaObject()->className(), "QMdiSubWindow"))
            break;

        widget = widget->parentWidget();
    }

    return widget;
}

void QDesignerIntegration::getSelection(Selection &s)
{
    // Get multiselection from object inspector
    if (QDesignerObjectInspector *designerObjectInspector = qobject_cast<QDesignerObjectInspector *>(core()->objectInspector())) {
        designerObjectInspector->getSelection(s);
        // Action editor puts actions that are not on the form yet
        // into the property editor only.
        if (s.empty())
            if (QObject *object = core()->propertyEditor()->object())
                s.m_selectedObjects.push_back(object);
            
    } else {
        s.clear();
        // get single selection

        QObject *object = core()->propertyEditor()->object();
        QWidget *widget = qobject_cast<QWidget*>(object);

        QDesignerFormWindowInterface *formWindow = core()->formWindowManager()->activeFormWindow();
        if (!formWindow)
            return;
        QDesignerFormWindowCursorInterface *cursor = formWindow->cursor();

        if (widget && cursor->isWidgetSelected(widget)) {
            s.m_cursorSelection.push_back(widget);
        } else {
            s.m_selectedObjects.push_back(object);
        }
    }
}

QObject *QDesignerIntegration::propertyEditorObject()
{
    QDesignerPropertyEditorInterface *propertyEditor = core()->propertyEditor();
    if (!propertyEditor)
        return 0;
    return propertyEditor->object();
}

// Load plugins into widget database and factory.
void QDesignerIntegration::initializePlugins(QDesignerFormEditorInterface *formEditor)
{
    // load the plugins
    if (WidgetDataBase *widgetDatabase = qobject_cast<WidgetDataBase*>(formEditor->widgetDataBase())) {
        widgetDatabase->loadPlugins();
        widgetDatabase->grabDefaultPropertyValues();
    }

    if (WidgetFactory *widgetFactory = qobject_cast<WidgetFactory*>(formEditor->widgetFactory())) {
        widgetFactory->loadPlugins();
    }
}

void QDesignerIntegration::updateCustomWidgetPlugins()
{
    QDesignerFormEditorInterface *formEditor = core();
    if (QDesignerPluginManager *pm = formEditor->pluginManager())
        pm->registerNewPlugins();

    initializePlugins(formEditor);

    // Do not just reload the last file as the WidgetBox merges the compiled-in resources
    // and $HOME/.designer/widgetbox.xml. This would also double the scratchpad.
    if (QDesignerWidgetBox *wb = qobject_cast<QDesignerWidgetBox*>(formEditor->widgetBox())) {
        const QDesignerWidgetBox::LoadMode oldLoadMode = wb->loadMode();
        wb->setLoadMode(QDesignerWidgetBox::LoadCustomWidgetsOnly);
        wb->load();
        wb->setLoadMode(oldLoadMode);
    }
}

void QDesignerIntegration::emitObjectNameChanged(QDesignerFormWindowInterface *formWindow, QObject *object, const QString &name)
{
    emit objectNameChanged(formWindow, object, name);
}

} // namespace qdesigner_internal
