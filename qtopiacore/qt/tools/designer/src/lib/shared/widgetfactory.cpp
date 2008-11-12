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
TRANSLATOR qdesigner_internal::WidgetFactory
*/

#include "widgetfactory_p.h"
#include "widgetdatabase_p.h"
#include "metadatabase_p.h"
#include "qlayout_widget_p.h"
#include "qdesigner_widget_p.h"
#include "qdesigner_tabwidget_p.h"
#include "qdesigner_toolbox_p.h"
#include "qdesigner_stackedbox_p.h"
#include "qdesigner_toolbar_p.h"
#include "qdesigner_menubar_p.h"
#include "qdesigner_menu_p.h"
#include "qdesigner_dockwidget_p.h"
#include "qdesigner_utils_p.h"
#include "abstractformwindow.h"

// shared
#include "layoutinfo_p.h"
#include "spacer_widget_p.h"
#include "layout_p.h"

// sdk
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerContainerExtension>
#include <QtDesigner/QDesignerCustomWidgetInterface>
#include <QtDesigner/QExtensionManager>
#include <QtDesigner/QDesignerPropertySheetExtension>
#include <QtDesigner/QDesignerLanguageExtension>

#include <QtGui/QtGui>
#include <QtCore/qdebug.h>

namespace qdesigner_internal {

QPointer<QWidget> *WidgetFactory::m_lastPassiveInteractor = new QPointer<QWidget>();
bool WidgetFactory::m_lastWasAPassiveInteractor = false;


WidgetFactory::WidgetFactory(QDesignerFormEditorInterface *core, QObject *parent)
    : QDesignerWidgetFactoryInterface(parent),
      m_core(core),
      m_formWindow(0)
{
}

WidgetFactory::~WidgetFactory()
{
}

QDesignerFormWindowInterface *WidgetFactory::currentFormWindow(QDesignerFormWindowInterface *fw)
{
    QDesignerFormWindowInterface *was = m_formWindow;
    m_formWindow = fw;
    return was;
}

void WidgetFactory::loadPlugins()
{
    m_customFactory.clear();

    QDesignerPluginManager *pluginManager = m_core->pluginManager();

    QList<QDesignerCustomWidgetInterface*> lst = pluginManager->registeredCustomWidgets();
    foreach (QDesignerCustomWidgetInterface *c, lst) {
        m_customFactory.insert(c->name(), c);
    }
}

// Convencience to create non-widget objects. Returns 0 if unknown
QObject* WidgetFactory::createObject(const QString &className, QObject* parent) const
{
    if (className == QLatin1String("QAction"))
        return new QAction(parent);
    return 0;
}

QWidget*  WidgetFactory::createCustomWidget(const QString &className, QWidget *parentWidget) const
{
    CustomWidgetFactoryMap::const_iterator it = m_customFactory.constFind(className);
    if (it == m_customFactory.constEnd())
        return 0;

    QDesignerCustomWidgetInterface *factory = it.value();
    QWidget *rc = factory->createWidget(parentWidget);
    // shouldn't happen
    if (!rc) {
        designerWarning(QObject::tr("The custom widget factory registered for widgets of class %1 returned 0.").arg(className));
        return 0;
    }

    // Since a language plugin may lie about its names, like Qt Jambi
    // does, return immediatly here...
    QDesignerLanguageExtension *lang =
        qt_extension<QDesignerLanguageExtension *>(m_core->extensionManager(), m_core);
    if (lang)
        return rc;

    // Check for mismatched class names which is hard to track.
    // Perform literal comparison first for QAxWidget, for which a meta object hack is in effect.
    const char *createdClassNameC = rc->metaObject()->className();
    const QByteArray classNameB = className.toUtf8();
    const char *classNameC = classNameB.constData();

    if (qstrcmp(createdClassNameC, classNameC) && !rc->inherits(classNameC))
        designerWarning(QObject::tr("A class name mismatch occurred when creating a widget using the custom widget factory registered for widgets of class %1."
                                  " It returned a widget of class %2.").arg(className).arg(QString::fromUtf8(createdClassNameC)));
    return rc;
}

QWidget *WidgetFactory::createWidget(const QString &widgetName, QWidget *parentWidget) const
{
    QDesignerFormWindowInterface *fw = m_formWindow;
    if (! fw)
        fw = QDesignerFormWindowInterface::findFormWindow(parentWidget);

    QWidget *w = createCustomWidget(widgetName, parentWidget);

    if (w) {
    } else if (widgetName == QLatin1String("Line")) {
        w = new Line(parentWidget);
    } else if (widgetName == QLatin1String("QDockWidget")) {
        w = new QDesignerDockWidget(parentWidget);
    } else if (widgetName == QLatin1String("QTabWidget")) {
        w = new QDesignerTabWidget(parentWidget);
    } else if (widgetName == QLatin1String("QStackedWidget")) {
        w = new QDesignerStackedWidget(parentWidget);
    } else if (widgetName == QLatin1String("QToolBox")) {
        w = new QDesignerToolBox(parentWidget);
    } else if (widgetName == QLatin1String("QToolBar")) {
        QToolBar *tb = new QToolBar(parentWidget);
        w = tb;
        ToolBarEventFilter::install(tb);
    } else if (widgetName == QLatin1String("QMenuBar")) {
        w = new QDesignerMenuBar(parentWidget);
    } else if (widgetName == QLatin1String("QMenu")) {
        w = new QDesignerMenu(parentWidget);
    } else if (widgetName == QLatin1String("Spacer")) {
        w = new Spacer(parentWidget);
    } else if (widgetName == QLatin1String("QDockWidget")) {
        w = new QDesignerDockWidget(parentWidget);
    } else if (widgetName == QLatin1String("QLayoutWidget")) {
        w = fw ? new QLayoutWidget(fw, parentWidget) : new QWidget(parentWidget);
    } else if (widgetName == QLatin1String("QDialog")) {
        if (fw) {
            w = new QDesignerDialog(fw, parentWidget);
        } else {
            w = new QDialog(parentWidget);
        }
    } else if (widgetName == QLatin1String("QWidget")) {
        if (fw && parentWidget &&
             (qobject_cast<QDesignerFormWindowInterface*>(parentWidget) || qt_extension<QDesignerContainerExtension*>(m_core->extensionManager(), parentWidget))) {
             w = new QDesignerWidget(fw, parentWidget);
        } else {
            w = new QWidget(parentWidget);
        }
    }

#define DECLARE_LAYOUT(L, C)
#define DECLARE_COMPAT_WIDGET(W, C) /*DECLARE_WIDGET(W, C)*/
#define DECLARE_WIDGET(W, C) else if (widgetName == QLatin1String(#W)) { Q_ASSERT(w == 0); w = new W(parentWidget); }
#define DECLARE_WIDGET_1(W, C) else if (widgetName == QLatin1String(#W)) { Q_ASSERT(w == 0); w = new W(0, parentWidget); }

#include "widgets.table"

#undef DECLARE_COMPAT_WIDGET
#undef DECLARE_LAYOUT
#undef DECLARE_WIDGET
#undef DECLARE_WIDGET_1

    if (w == 0) {
        const QLatin1String fallBackBaseClass("QWidget");
        QDesignerWidgetDataBaseInterface *db = core()->widgetDataBase();
        QDesignerWidgetDataBaseItemInterface *item = db->item(db->indexOfClassName(widgetName));
        if (item == 0) {
            // Emergency: Create, derived from QWidget
            QString includeFile = widgetName.toLower();
            includeFile +=  QLatin1String(".h");
            item = appendDerived(db,widgetName,tr("%1 Widget").arg(widgetName),fallBackBaseClass,
                                 includeFile, true, true);
            Q_ASSERT(item);
        }
        QString baseClass = item->extends();
        if (baseClass.isEmpty()) {
            // Currently happens in the case of Q3-Support widgets
            baseClass =fallBackBaseClass;
        }
        w = createWidget(baseClass, parentWidget);
        promoteWidget(core(),w,widgetName);
    }

    Q_ASSERT(w != 0);

    if (fw != 0)
        initialize(w);

    return w;
}

QString WidgetFactory::classNameOf(QDesignerFormEditorInterface *c, QObject* o)
{
    if (o == 0)
        return QString();

    // check promoted before designer special
    if (o->isWidgetType()) {
        const QString customClassName = promotedCustomClassName(c,qobject_cast<QWidget*>(o));
        if (!customClassName.isEmpty())
            return customClassName;
    }

    if (qobject_cast<QDesignerTabWidget*>(o))
        return QLatin1String("QTabWidget");
    else if (qobject_cast<QDesignerStackedWidget*>(o))
        return QLatin1String("QStackedWidget");
    else if (qobject_cast<QDesignerMenuBar*>(o))
        return QLatin1String("QMenuBar");
    else if (qobject_cast<QDesignerDockWidget*>(o))
        return QLatin1String("QDockWidget");
    else if (qobject_cast<QDesignerToolBox*>(o))
        return QLatin1String("QToolBox");
    else if (qobject_cast<QDesignerDialog*>(o))
        return QLatin1String("QDialog");
    else if (qobject_cast<QDesignerWidget*>(o))
        return QLatin1String("QWidget");
    else if (qstrcmp(o->metaObject()->className(), "QAxBase") == 0)
        return QLatin1String("QAxWidget");
    else if (qstrcmp(o->metaObject()->className(), "QDesignerQ3WidgetStack") == 0)
        return QLatin1String("Q3WidgetStack");

    return QLatin1String(o->metaObject()->className());
}

QLayout *WidgetFactory::createUnmanagedLayout(QWidget *parentWidget, int type)
{
    switch (type) {
    case LayoutInfo::HBox:
        return new QHBoxLayout(parentWidget);
    case LayoutInfo::VBox:
        return new QVBoxLayout(parentWidget);
    case LayoutInfo::Grid:
        return new QGridLayout(parentWidget);
    default:
        Q_ASSERT(0);
        break;
    }
    return 0;
}


/*!  Creates a layout on the widget \a widget of the type \a type
  which can be \c HBox, \c VBox or \c Grid.
*/

QLayout *WidgetFactory::createLayout(QWidget *widget, QLayout *parentLayout, int type) const // ### (sizepolicy)
{
    QDesignerMetaDataBaseInterface *metaDataBase = core()->metaDataBase();

    if (parentLayout == 0) {
        QWidget *page = containerOfWidget(widget);
        if (page) {
            widget = page;
        } else {
            const QString msg = QObject::tr("The current page of the container '%1' (%2) could not be determined while creating a layout."
"This indicates an inconsistency in the ui-file, probably a layout being constructed on a container widget.").arg(widget->objectName()).arg(classNameOf(core(), widget));
            designerWarning(msg);
        }
    }

    Q_ASSERT(metaDataBase->item(widget) != 0); // ensure the widget is managed

    if (parentLayout == 0 && metaDataBase->item(widget->layout()) == 0) {
        parentLayout = widget->layout();
    }

    QWidget *parentWidget = parentLayout != 0 ? 0 : widget;

    QLayout *layout = createUnmanagedLayout(parentWidget, type);
    metaDataBase->add(layout); // add the layout in the MetaDataBase

    QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(core()->extensionManager(), layout);

    if (widget->inherits("Q3GroupBox")) {
        layout->setContentsMargins(widget->style()->pixelMetric(QStyle::PM_LayoutLeftMargin),
                                    widget->style()->pixelMetric(QStyle::PM_LayoutTopMargin),
                                    widget->style()->pixelMetric(QStyle::PM_LayoutRightMargin),
                                    widget->style()->pixelMetric(QStyle::PM_LayoutBottomMargin));
        QGridLayout *grid = qobject_cast<QGridLayout *>(layout);
        if (grid) {
            grid->setHorizontalSpacing(-1);
            grid->setVerticalSpacing(-1);
        } else {
            layout->setSpacing(-1);
        }
        layout->setAlignment(Qt::AlignTop);
        // Just to ensure; before 4.3 orientation property was always set (now only for QSplitter class).
        // Calling Q3GroupBox::setOrientation() invoked in turn setSpacing(0). Below fixes that
        widget->layout()->setSpacing(-1);
    } else if (widget->inherits("QLayoutWidget")) {
        sheet->setProperty(sheet->indexOf(QLatin1String("leftMargin")), 0);
        sheet->setProperty(sheet->indexOf(QLatin1String("topMargin")), 0);
        sheet->setProperty(sheet->indexOf(QLatin1String("rightMargin")), 0);
        sheet->setProperty(sheet->indexOf(QLatin1String("bottomMargin")), 0);
    }

    if (sheet)
        sheet->setChanged(sheet->indexOf(QLatin1String("alignment")), true);

    if (metaDataBase->item(widget->layout()) == 0) {
        Q_ASSERT(layout->parent() == 0);
        QBoxLayout *box = qobject_cast<QBoxLayout*>(widget->layout());
        Q_ASSERT(box != 0); // we support only unmanaged box layouts
        box->addLayout(layout);
    }

    return layout;
}

/*!  Returns the widget into which children should be inserted when \a
  w is a container known to designer.

  Usually, it is \a w itself, but there are exceptions (for example, a
  tabwidget is known to designer as a container, but the child
  widgets should be inserted into the current page of the
  tabwidget. In this case, the current page of
  the tabwidget  would be returned.)
 */
QWidget* WidgetFactory::containerOfWidget(QWidget *w) const
{
    if (QDesignerContainerExtension *container = qt_extension<QDesignerContainerExtension*>(core()->extensionManager(), w))
        return container->widget(container->currentIndex());

    return w;
}

/*!  Returns the actual designer widget of the container \a w. This is
  normally \a w itself, but it might be a parent or grand parent of \a w
  (for example, when working with a tabwidget and \a w is the container which
  contains and layouts children, but the actual widget known to
  designer is the tabwidget which is the parent of \a w. In this case,
  the tabwidget would be returned.)
*/

QWidget* WidgetFactory::widgetOfContainer(QWidget *w) const
{
    // ### cleanup
    if (!w)
        return 0;
    if (w->parentWidget() && w->parentWidget()->parentWidget() &&
         w->parentWidget()->parentWidget()->parentWidget() &&
         qobject_cast<QToolBox*>(w->parentWidget()->parentWidget()->parentWidget()))
        return w->parentWidget()->parentWidget()->parentWidget();

    while (w != 0) {
        if (core()->widgetDataBase()->isContainer(w) ||
             w && qobject_cast<QDesignerFormWindowInterface*>(w->parentWidget()))
            return w;

        w = w->parentWidget();
    }

    return w;
}

QDesignerFormEditorInterface *WidgetFactory::core() const
{
    return m_core;
}

void WidgetFactory::initialize(QObject *object) const
{
    QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(m_core->extensionManager(), object);

    if (object->metaObject()->indexOfProperty("focusPolicy") != -1)
        object->setProperty("focusPolicy", Qt::NoFocus);

    if (!sheet)
        return;

    sheet->setChanged(sheet->indexOf(QLatin1String("objectName")), true);
    sheet->setChanged(sheet->indexOf(QLatin1String("geometry")), true);

    if (qobject_cast<Spacer*>(object))
        sheet->setChanged(sheet->indexOf(QLatin1String("sizeHint")), true);

    int o = sheet->indexOf(QLatin1String("orientation"));
    if (o != -1 && object->inherits("QSplitter"))
        sheet->setChanged(o, true);

    if (QWidget *widget = qobject_cast<QWidget*>(object)) {
        QSize sz = widget->sizeHint();

        if (qobject_cast<QFrame*>(object) && sz.width() <= 0 && sz.height() <= 0)
            widget->setMinimumSize(QSize(16, 16));

        widget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    }

    if (QToolBar *toolBar = qobject_cast<QToolBar*>(object)) {
        sheet->setVisible(sheet->indexOf(QLatin1String("windowTitle")), true);
        toolBar->setFloatable(false);  // prevent toolbars from being dragged off
    }

    if (qobject_cast<QDockWidget*>(object)) {
        sheet->setVisible(sheet->indexOf(QLatin1String("windowTitle")), true);
        sheet->setVisible(sheet->indexOf(QLatin1String("windowIcon")), true);
    }

    if (qobject_cast<QAction*>(object)) {
        sheet->setChanged(sheet->indexOf(QLatin1String("text")), true);
    }

    if (qobject_cast<QMenu*>(object)) {
        sheet->setChanged(sheet->indexOf(QLatin1String("geometry")), false);
        sheet->setChanged(sheet->indexOf(QLatin1String("title")), true);
    }

    if (qobject_cast<QMenu*>(object) || qobject_cast<QMenuBar*>(object)) {
        qobject_cast<QWidget*>(object)->setFocusPolicy(Qt::StrongFocus);
    }
}

bool WidgetFactory::isPassiveInteractor(QWidget *widget)
{
    if (m_lastPassiveInteractor != 0 && (QWidget*)(*m_lastPassiveInteractor) == widget)
        return m_lastWasAPassiveInteractor;

    m_lastWasAPassiveInteractor = false;
    (*m_lastPassiveInteractor) = widget;

    if (QApplication::activePopupWidget()) // if a popup is open, we have to make sure that this one is closed, else X might do funny things
        return (m_lastWasAPassiveInteractor = true);
    else if (widget == 0)
        return m_lastWasAPassiveInteractor;

    if (qobject_cast<QTabBar*>(widget))
        return (m_lastWasAPassiveInteractor = true);
    else if (qobject_cast<QSizeGrip*>(widget))
        return (m_lastWasAPassiveInteractor = true);
    else if (qobject_cast<QAbstractButton*>(widget) && (qobject_cast<QTabBar*>(widget->parent()) || qobject_cast<QToolBox*>(widget->parent())))
        return (m_lastWasAPassiveInteractor = true);
    else if (qobject_cast<QMenuBar*>(widget))
        return (m_lastWasAPassiveInteractor = true);
    else if (qobject_cast<QToolBar*>(widget))
        return (m_lastWasAPassiveInteractor = true);
    else if (qstrcmp(widget->metaObject()->className(), "QDockWidgetTitle") == 0)
        return (m_lastWasAPassiveInteractor = true);
    else if (widget->objectName().startsWith(QLatin1String("__qt__passive_")))
        return (m_lastWasAPassiveInteractor = true);

    return m_lastWasAPassiveInteractor;
}

} // namespace qdesigner_internal
