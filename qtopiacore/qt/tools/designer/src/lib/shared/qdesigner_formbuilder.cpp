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

#include "qdesigner_formbuilder_p.h"
#include "dynamicpropertysheet.h"
#include "qsimpleresource_p.h"

#include <ui4_p.h>
#include <formbuilderextra_p.h>
// sdk
#include <QtDesigner/container.h>
#include <QtDesigner/customwidget.h>
#include <QtDesigner/propertysheet.h>
#include <QtDesigner/QExtensionManager>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerWidgetFactoryInterface>
#include <QtDesigner/QDesignerCustomWidgetInterface>
#include <QtDesigner/abstracticoncache.h>

// shared
#include <resourcefile_p.h>
#include <scripterrordialog_p.h>

#include <QtGui/QWidget>
#include <QtGui/QMenu>
#include <QtGui/QToolBar>
#include <QtGui/QMenuBar>
#include <QtGui/QMainWindow>
#include <QtGui/QStyleFactory>
#include <QtGui/QStyle>
#include <QtGui/QApplication>
#include <QtGui/QAbstractScrollArea>
#include <QtGui/QMessageBox>

#include <QtCore/QBuffer>
#include <QtCore/qdebug.h>

#include <qdesigner_utils_p.h>

static QString summarizeScriptErrors(const QFormScriptRunner::Errors &errors) 
{
    QString rc =  QObject::tr("Script errors occurred:");
    foreach (QFormScriptRunner::Error error, errors) {
        rc += QLatin1Char('\n');
        rc += error.errorMessage;
    }
    return rc;
}
 
namespace qdesigner_internal {

QDesignerFormBuilder::QDesignerFormBuilder(QDesignerFormEditorInterface *core, Mode mode) : 
    m_core(core),
    m_mode(mode)
{
    Q_ASSERT(m_core);
    // Disable scripting in the editors.
    QFormScriptRunner::Options options = formScriptRunner()->options();
    switch (m_mode) {
    case DisableScripts:
        options |= QFormScriptRunner::DisableScripts;
        break;
    case UseScriptAndContainerExtension:
    case UseScriptForContainerExtension:
        options |= QFormScriptRunner::DisableWarnings;
        options &= ~QFormScriptRunner::DisableScripts;
        break;
    }
    formScriptRunner()-> setOptions(options);
}

QWidget *QDesignerFormBuilder::createWidgetFromContents(const QString &contents, QWidget *parentWidget)
{
    QByteArray data = contents.toUtf8();
    QBuffer buffer(&data);
    return load(&buffer, parentWidget);
}

QWidget *QDesignerFormBuilder::create(DomUI *ui, QWidget *parentWidget)
{
    m_customWidgetsWithScript.clear();
    return QFormBuilder::create(ui, parentWidget);
}

QWidget *QDesignerFormBuilder::createWidget(const QString &widgetName, QWidget *parentWidget, const QString &name)
{
    QWidget *widget = 0;

    if (widgetName == QLatin1String("QToolBar")) {
        widget = new QToolBar(parentWidget);
    } else if (widgetName == QLatin1String("QMenu")) {
        widget = new QMenu(parentWidget);
    } else if (widgetName == QLatin1String("QMenuBar")) {
        widget = new QMenuBar(parentWidget);
    } else {
        widget = core()->widgetFactory()->createWidget(widgetName, parentWidget);
    }

    if (widget) {
        widget->setObjectName(name);
        if (QSimpleResource::hasCustomWidgetScript(m_core, widget))
            m_customWidgetsWithScript.insert(widget);
    }

    return widget;
}

bool QDesignerFormBuilder::addItemContainerExtension(QWidget *widget, QWidget *parentWidget)
{
    QDesignerContainerExtension *container = qt_extension<QDesignerContainerExtension*>(m_core->extensionManager(), parentWidget);
    if (!container)
        return false;

    container->addWidget(widget);
    return true;
}

bool QDesignerFormBuilder::addItem(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget)
{
    // Use container extension or rely on scripts unless main window.
    if (QFormBuilder::addItem(ui_widget, widget, parentWidget))
        return true;

    // Use for mainwindow at any event
    if (qobject_cast<const QMainWindow*>(parentWidget))
        return addItemContainerExtension(widget, parentWidget);

    // Assume the script populates the container
    if (m_mode == UseScriptForContainerExtension && m_customWidgetsWithScript.contains(parentWidget))
         return true;

    return addItemContainerExtension(widget, parentWidget);;
}

bool QDesignerFormBuilder::addItem(DomLayoutItem *ui_item, QLayoutItem *item, QLayout *layout)
{
    return QFormBuilder::addItem(ui_item, item, layout);
}

QIcon QDesignerFormBuilder::nameToIcon(const QString &filePath, const QString &qrcPath)
{
    return QIcon(core()->iconCache()->resolveQrcPath(filePath, qrcPath, workingDirectory().absolutePath()));
}

QPixmap QDesignerFormBuilder::nameToPixmap(const QString &filePath, const QString &qrcPath)
{
    return QPixmap(core()->iconCache()->resolveQrcPath(filePath, qrcPath, workingDirectory().absolutePath()));
}

void QDesignerFormBuilder::applyProperties(QObject *o, const QList<DomProperty*> &properties)
{
    typedef QList<DomProperty*> DomPropertyList;

    if (properties.empty())
        return;

    QFormBuilderExtra *formBuilderExtra = QFormBuilderExtra::instance(this);
    const QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(core()->extensionManager(), o);
    const QDesignerDynamicPropertySheetExtension *dynamicSheet = qt_extension<QDesignerDynamicPropertySheetExtension*>(core()->extensionManager(), o);
    const QMetaObject *meta = o->metaObject();
    const bool dynamicPropertiesAllowed = dynamicSheet && dynamicSheet->dynamicPropertiesAllowed() && strcmp(meta->className(), "QAxWidget") != 0;

    const DomPropertyList::const_iterator cend = properties.constEnd();
    for (DomPropertyList::const_iterator it = properties.constBegin(); it != cend; ++it) {
        DomProperty *p = *it;
        const QString attributeName = p->attributeName();
        QVariant v;
        if (sheet && p->kind() == DomProperty::Enum && qVariantCanConvert<EnumType>(sheet->property(sheet->indexOf(attributeName)))) {
            const EnumType e = qvariant_cast<EnumType>(sheet->property(sheet->indexOf(attributeName)));
            if (e.items.contains(p->elementEnum()))
                v = e.items[p->elementEnum()];
        } else if (sheet && p->kind() == DomProperty::Set && qVariantCanConvert<FlagType>(sheet->property(sheet->indexOf(attributeName)))) {
            const FlagType e = qvariant_cast<FlagType>(sheet->property(sheet->indexOf(attributeName)));
            uint flags = 0;
            QStringList items = p->elementSet().split(QLatin1String("|"));
            foreach (QString item, items) {
                if (e.items.contains(item))
                    flags |= e.items[item].toUInt();
            }
            v = flags;
        } else {
            v = toVariant(meta, p);
        }

        if (v.isNull())
            continue;

        if (formBuilderExtra->applyPropertyInternally(o, attributeName, v))
            continue;

        const QByteArray pname = attributeName.toUtf8();
        const int index = meta->indexOfProperty(pname);

        QObject *obj = o;
        QAbstractScrollArea *scroll = qobject_cast<QAbstractScrollArea *>(o);
        if (scroll && QLatin1String(pname) == QLatin1String("cursor") && scroll->viewport())
            obj = scroll->viewport();

        if (index != -1 || dynamicPropertiesAllowed) {
            // a real property
            obj->setProperty(pname, v);
        }
    }
}

DomWidget *QDesignerFormBuilder::createDom(QWidget *widget, DomWidget *ui_parentWidget, bool recursive)
{
    DomWidget *ui_widget = QFormBuilder::createDom(widget, ui_parentWidget, recursive);
    QSimpleResource::addExtensionDataToDOM(this, m_core, ui_widget, widget);
    return ui_widget;
}

QWidget *QDesignerFormBuilder::create(DomWidget *ui_widget, QWidget *parentWidget)
{
    QWidget *widget = QFormBuilder::create(ui_widget, parentWidget);
    // Do not apply state if scripts are to be run in preview mode
    QSimpleResource::applyExtensionDataFromDOM(this, m_core, ui_widget, widget, m_mode == DisableScripts);
    return widget;
}

QLayout *QDesignerFormBuilder::create(DomLayout *ui_layout, QLayout *layout, QWidget *parentWidget)
{
    return QFormBuilder::create(ui_layout, layout, parentWidget);
}

void QDesignerFormBuilder::loadExtraInfo(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget)
{
    QFormBuilder::loadExtraInfo(ui_widget, widget, parentWidget);
}

QWidget *QDesignerFormBuilder::createPreview(const QDesignerFormWindowInterface *fw,
                                             const QString &styleName,
                                             ScriptErrors *scriptErrors, 
                                             QString *errorMessage)
{
    scriptErrors->clear();
    // style
    QStyle *style = 0;
    if (!styleName.isEmpty()) {
        style = QStyleFactory::create(styleName);
        if (!style) {
            *errorMessage = QObject::tr("The style %1 could not be loaded.").arg(styleName);
            return 0;
        }
    }

    // load
    QDesignerFormBuilder builder(fw->core(), UseScriptAndContainerExtension);
    builder.setWorkingDirectory(fw->absoluteDir());

    const bool warningsEnabled = QSimpleResource::setWarningsEnabled(false);
    QByteArray bytes = fw->contents().toUtf8();
    QSimpleResource::setWarningsEnabled(warningsEnabled);

    QBuffer buffer(&bytes);

    QWidget *widget = builder.load(&buffer, 0);
    if (!widget) { // Shouldn't happen
        *errorMessage = QObject::tr("The preview failed to build.");
        return  0;
    }
    // Check for script errors
    *scriptErrors = builder.formScriptRunner()->errors();
    if (!scriptErrors->empty()) {
        *errorMessage = summarizeScriptErrors(*scriptErrors);
        delete widget;
        return  0;        
    }

    // Apply style stored in action if any
    if (style) {
        style->setParent(widget);
        widget->setStyle(style);
        if (style->metaObject()->className() != QApplication::style()->metaObject()->className())
            widget->setPalette(style->standardPalette());
        
        const QList<QWidget*> lst = qFindChildren<QWidget*>(widget);
        foreach (QWidget *w, lst)
            w->setStyle(style);
    }
    widget->setWindowTitle(QObject::tr("%1 - [Preview]").arg(widget->windowTitle()));
    return widget;
}
    
QWidget *QDesignerFormBuilder::createPreview(const QDesignerFormWindowInterface *fw, const QString &styleName) 
{
    ScriptErrors scriptErrors;
    QString errorMessage;
    QWidget *widget = createPreview(fw, styleName, &scriptErrors, &errorMessage);
    if (!widget) {
        // Display Script errors or message box
        QWidget *dialogParent = fw->core()->topLevel();
        if (scriptErrors.empty()) {
            QMessageBox::critical(dialogParent, QObject::tr("Designer"), errorMessage);
        } else {
            ScriptErrorDialog scriptErrorDialog(scriptErrors, dialogParent);
            scriptErrorDialog.exec();
        }
        return 0;
    }    
    return widget;
}

} // namespace qdesigner_internal
