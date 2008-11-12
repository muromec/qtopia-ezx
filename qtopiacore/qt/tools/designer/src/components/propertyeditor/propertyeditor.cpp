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

#include "propertyeditor.h"
#include "qpropertyeditor_model_p.h"
#include "qpropertyeditor_items_p.h"
#include "newdynamicpropertydialog.h"
#include "dynamicpropertysheet.h"
#include "paletteeditorbutton.h"
#include "graphicspropertyeditor.h"

// sdk
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowManagerInterface>
#include <QtDesigner/QExtensionManager>
#include <QtDesigner/QDesignerPropertySheetExtension>
// shared
#include <qdesigner_utils_p.h>
#include <qdesigner_propertycommand_p.h>
#include <metadatabase_p.h>

#include <QtGui/QAction>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>
#include <QtGui/QApplication>
#include <QtGui/QVBoxLayout>

// ---------------------------------------------------------------------------------

namespace qdesigner_internal {

IProperty *PropertyEditor::createSpecialProperty(const QVariant &value, const QString &name)
{
    Q_UNUSED(value);
    Q_UNUSED(name);

    return 0;
}

class PaletteProperty : public AbstractProperty<QPalette>
{
public:
    PaletteProperty(QDesignerFormEditorInterface *core, const QPalette &value,
                QWidget *selectedWidget, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);

private:
    QDesignerFormEditorInterface *m_core;
    QWidget *m_selectedWidget;
};

// -------------------------------------------------------------------------
PaletteProperty::PaletteProperty(QDesignerFormEditorInterface *core, const QPalette &value, QWidget *selectedWidget,
                const QString &name)
    : AbstractProperty<QPalette>(value, name),
      m_core(core),
      m_selectedWidget(selectedWidget)
{
}

void PaletteProperty::setValue(const QVariant &value)
{
    m_value = qvariant_cast<QPalette>(value);
    QPalette parentPalette = QPalette();
    if (m_selectedWidget) {
        if (m_selectedWidget->isWindow())
            parentPalette = QApplication::palette(m_selectedWidget);
        else {
            if (m_selectedWidget->parentWidget())
                parentPalette = m_selectedWidget->parentWidget()->palette();
        }
    }
    const uint mask = m_value.resolve();
    m_value = m_value.resolve(parentPalette);
    m_value.resolve(mask);
}

QString PaletteProperty::toString() const
{
    return QString(); // ### implement me
}

QWidget *PaletteProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    PaletteEditorButton *btn = new PaletteEditorButton(m_core, m_value, m_selectedWidget, parent);
    QObject::connect(btn, SIGNAL(changed()), target, receiver);
    return btn;
}

void PaletteProperty::updateEditorContents(QWidget *editor)
{
    if (PaletteEditorButton *btn = qobject_cast<PaletteEditorButton*>(editor)) {
        btn->setPalette(m_value);
    }
}

void PaletteProperty::updateValue(QWidget *editor)
{
    if (PaletteEditorButton *btn = qobject_cast<PaletteEditorButton*>(editor)) {
        const QPalette newValue = btn->palette();

        if (newValue.resolve() != m_value.resolve() || newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }
    }
}

// -------------------------------------------------------------------------------------

struct Group
{
    QString name;
    QList<IProperty*> properties;

    inline Group() {}
    inline Group(const QString &n): name(n) {}

    inline bool operator == (const Group &other) const
    { return name == other.name; }
};


// A pair <ValidationMode, bool hasComment>.
typedef QPair<TextPropertyValidationMode, bool> StringPropertyParameters;

// Return a pair of validation mode and flag indicating whether property has a comment
// for textual properties.

StringPropertyParameters textPropertyValidationMode(const QObject *object,const QString &pname,
                                                    QVariant::Type type, bool isMainContainer)   
{
    if (type == QVariant::ByteArray) {
        return StringPropertyParameters(ValidationMultiLine, false);
    }
    // object name - no comment
    if (pname == QLatin1String("objectName")) {
        const TextPropertyValidationMode vm =  isMainContainer ? ValidationObjectNameScope : ValidationObjectName;
        return StringPropertyParameters(vm, false);
    }

    // Accessibility. Both are texts the narrator reads
    if (pname == QLatin1String("accessibleDescription") || pname == QLatin1String("accessibleName"))
        return StringPropertyParameters(ValidationMultiLine, true);

    // Any names
    if (pname == QLatin1String("buddy") || pname.endsWith(QLatin1String("Name")))
        return StringPropertyParameters(ValidationObjectName, false);
        
    // Multi line?
    if (pname == QLatin1String("styleSheet")) 
        return StringPropertyParameters(ValidationStyleSheet, false);
    
    if (pname == QLatin1String("styleSheet")     || pname == QLatin1String("toolTip")   || 
        pname.endsWith(QLatin1String("ToolTip")) || pname == QLatin1String("whatsThis") ||
        pname == QLatin1String("iconText")       || pname == QLatin1String("windowIconText")  ||
        pname == QLatin1String("html"))
        return StringPropertyParameters(ValidationMultiLine, true);


    // text only if not Action, LineEdit
    if (pname == QLatin1String("text") && !(qobject_cast<const QAction *>(object) || qobject_cast<const QLineEdit *>(object)))
        return StringPropertyParameters(ValidationMultiLine, true);

    // default to single
    return StringPropertyParameters(ValidationSingleLine, true);    
}


// Create a string prop with proper validation mode
StringProperty* PropertyEditor::createStringProperty(QObject *object, const QString &pname, const QVariant &value, bool isMainContainer) const 
{
    const StringPropertyParameters params = textPropertyValidationMode(object, pname, value.type(), isMainContainer);
    // Does a meta DB entry exist - add comment
    const bool hasComment = params.second && metaDataBaseItem();
    const QString comment = hasComment ? propertyComment(m_core, object, pname) : QString();
    const QString stringValue = value.type() == QVariant::ByteArray ? QString::fromUtf8(value.toByteArray()) : value.toString();
    return new StringProperty(stringValue, pname, params.first, hasComment, comment );
}

QDesignerMetaDataBaseItemInterface* PropertyEditor::metaDataBaseItem() const 
{
    QObject *o = object();
    if (!o) 
        return 0;
    QDesignerMetaDataBaseInterface *db = core()->metaDataBase();
    if (!db) 
        return 0;
    return db->item(o);
}

void PropertyEditor::createPropertySheet(PropertyCollection *root, QObject *object)
{
    QList<Group> groups;

    QExtensionManager *m = m_core->extensionManager();

    bool isMainContainer = false;
    if (QWidget *widget = qobject_cast<QWidget*>(object)) {
        if (QDesignerFormWindowInterface *fw = QDesignerFormWindowInterface::findFormWindow(widget)) {
            isMainContainer = (fw->mainContainer() == widget);
        }
    }
    m_prop_sheet = qobject_cast<QDesignerPropertySheetExtension*>(m->extension(object, Q_TYPEID(QDesignerPropertySheetExtension)));
    const int count = m_prop_sheet->count();
    for (int i=0; i < count; ++i) {
        if (!m_prop_sheet->isVisible(i))
            continue;

        const QString pname = m_prop_sheet->propertyName(i);
        // Is this property redefined/hidden in a derived class?
        // Make it appear under that category only
        if (m_prop_sheet->indexOf(pname) != i)
            continue;
        const QVariant value = m_prop_sheet->property(i);

        IProperty *p = 0;
        if (qVariantCanConvert<FlagType>(value)) {
            FlagType f = qvariant_cast<FlagType>(value);
            if (pname == QLatin1String("alignment")) {
                p = new AlignmentProperty(f.items, Qt::Alignment(f.value.toInt()), pname);
            } else {
                p = new FlagsProperty(f.items, f.value.toInt(), pname);
            }
        } else if (qVariantCanConvert<EnumType>(value)) {
            EnumType e = qvariant_cast<EnumType>(value);
            p = new MapProperty(e.items, e.value, pname, e.names);
        }

        if (!p) {
            switch (value.type()) {
            case 0:
                p = createSpecialProperty(value, pname);
                break;
            case QVariant::Int:
                p = new IntProperty(value.toInt(), pname);
                break;
            case QVariant::UInt:
                p = new UIntProperty(value.toUInt(), pname);
                break;
            case QVariant::LongLong:
                p = new LongLongProperty(value.toLongLong(), pname);
                break;
            case QVariant::ULongLong:
                p = new ULongLongProperty(value.toULongLong(), pname);
                break;
            case QVariant::Double:
                p = new DoubleProperty(value.toDouble(), pname);
                break;
            case QVariant::Char:
                p = new CharProperty(value.toChar(), pname);
                break;
            case QVariant::Bool:
                p = new BoolProperty(value.toBool(), pname);
                break;
            case QVariant::ByteArray:
            case QVariant::String: 
                p = createStringProperty(object, pname, value, isMainContainer);
                break;
            case QVariant::Size:
                p = new SizeProperty(value.toSize(), pname);
                break;
            case QVariant::SizeF:
                p = new SizeFProperty(value.toSizeF(), pname);
                break;
            case QVariant::Point:
                p = new PointProperty(value.toPoint(), pname);
                break;
            case QVariant::PointF:
                p = new PointFProperty(value.toPointF(), pname);
                break;
            case QVariant::Rect:
                p = new RectProperty(value.toRect(), pname);
                break;
            case QVariant::RectF:
                p = new RectFProperty(value.toRectF(), pname);
                break;
            case QVariant::Icon:
                p = new IconProperty(m_core, qvariant_cast<QIcon>(value), pname);
                break;
            case QVariant::Pixmap:
                p = new PixmapProperty(m_core, qvariant_cast<QPixmap>(value), pname);
                break;
            case QVariant::Font:
                p = new FontProperty(qvariant_cast<QFont>(value), pname, qobject_cast<QWidget *>(object));
                break;
            case QVariant::Color:
                p = new ColorProperty(qvariant_cast<QColor>(value), pname);
                break;
            case QVariant::SizePolicy:
                p = new SizePolicyProperty(qvariant_cast<QSizePolicy>(value), pname);
                break;
            case QVariant::DateTime:
                p = new DateTimeProperty(value.toDateTime(), pname);
                break;
            case QVariant::Date:
                p = new DateProperty(value.toDate(), pname);
                break;
            case QVariant::Time:
                p = new TimeProperty(value.toTime(), pname);
                break;
            case QVariant::Cursor:
                p = new CursorProperty(qvariant_cast<QCursor>(value), pname);
                break;
            case QVariant::KeySequence:
                p = new KeySequenceProperty(qvariant_cast<QKeySequence>(value), pname);
                break;
            case QVariant::Palette:
                p = new PaletteProperty(m_core, qvariant_cast<QPalette>(value),
                                qobject_cast<QWidget *>(object), pname);
                break;
            case QVariant::Url:
                p = new UrlProperty(value.toUrl(), pname);
                break;
            case QVariant::StringList:
                p = new StringListProperty(qvariant_cast<QStringList>(value), pname);
                break;
            default:
                // ### qDebug() << "property" << pname << "with type" << value.type() << "not supported yet!";
                break;
            } // end switch
        }

        if (p != 0) {
            p->setHasReset(m_prop_sheet->hasReset(i));
            p->setChanged(m_prop_sheet->isChanged(i));
            p->setDirty(false);

            const QString pgroup = m_prop_sheet->propertyGroup(i);
            int groupIndex = groups.indexOf(pgroup);
            if (groupIndex == -1) {
                groupIndex = groups.count();
                groups.append(Group(pgroup));
            }

            QList<IProperty*> &groupProperties = groups[groupIndex].properties;
            groupProperties.append(p);
            m_indexToProperty[i] = p;
        }
    }

    foreach (Group g, groups) {
        root->addProperty(new SeparatorProperty(QString(), g.name));
        foreach (IProperty *p, g.properties) {
            root->addProperty(p);
        }
    }
}

void PropertyEditor::updatePropertySheet()
{
    if (!m_prop_sheet)
        return;

    const int count = m_prop_sheet->count();
    for (int i = 0; i < count; ++i) {
        IndexToPropertyMap::const_iterator it = m_indexToProperty.constFind(i);
        if (it !=  m_indexToProperty.constEnd()) {
            IProperty *p = it.value();
            p->setValue(m_prop_sheet->property(i));
            m_editor->editorModel()->refresh(p);
        }
    }
}

PropertyEditor::PropertyEditor(QDesignerFormEditorInterface *core,
            QWidget *parent, Qt::WindowFlags flags)
    : QDesignerPropertyEditor(parent, flags),
      m_core(core),
      m_editor(new QPropertyEditor(this)),
      m_properties(0),
      m_prop_sheet(0)
{
    connect(m_editor, SIGNAL(editorOpened()), this, SIGNAL(editorOpened()));
    connect(m_editor, SIGNAL(editorClosed()), this, SIGNAL(editorClosed()));
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setMargin(0);
    lay->addWidget(m_editor);

    connect(m_editor, SIGNAL(propertyChanged(IProperty*)),
        this, SLOT(slotFirePropertyChanged(IProperty*)));
    connect(m_editor->editorModel(), SIGNAL(resetProperty(QString)),
                this, SLOT(slotResetProperty(QString)));
    connect(m_editor, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(slotCustomContextMenuRequested(QPoint)));

}

PropertyEditor::~PropertyEditor()
{
     delete m_properties;
}

bool PropertyEditor::isReadOnly() const
{
    return m_editor->isReadOnly();
}

void PropertyEditor::setReadOnly(bool readOnly)
{
    m_editor->setReadOnly(readOnly);
}

QDesignerFormEditorInterface *PropertyEditor::core() const
{
    return m_core;
}

IProperty *PropertyEditor::propertyByName(IProperty *p, const QString &name)
{
    if (p->propertyName() == name)
        return p;

    if (p->kind() == IProperty::Property_Group) {
        IPropertyGroup *g = static_cast<IPropertyGroup*>(p);
        for (int i=0; i<g->propertyCount(); ++i)
            if (IProperty *c = propertyByName(g->propertyAt(i), name))
                return c;
    }

    return 0;
}

void PropertyEditor::setPropertyValue(const QString &name, const QVariant &value, bool changed)
{
    if (isReadOnly())
        return;
    
    IProperty *p = propertyByName(m_editor->initialInput(), name);
    if (!p)
        return;

    if (p->value() != value) 
        p->setValue(value);
    
    p->setChanged(changed);
    p->setDirty(false);
    
    m_editor->editorModel()->refresh(p);
}

void PropertyEditor::setPropertyComment(const QString &name, const QString &value)
{
    if (isReadOnly())
        return;

    IProperty *parent = propertyByName(m_editor->initialInput(), name);
    if (!parent || parent->kind() != IProperty::Property_Group)
        return;
    
    AbstractPropertyGroup *parentGroup = static_cast<AbstractPropertyGroup *>(parent);
    
    if (parentGroup->propertyCount() != 1)
        return;
    
    IProperty *commentProperty = parentGroup->propertyAt(0);
    if (commentProperty->value().toString() != value)
        commentProperty->setValue(value);
    
    commentProperty->setDirty(false);

    m_editor->editorModel()->refresh(commentProperty);    
}

void PropertyEditor::slotFirePropertyChanged(IProperty *p)
{
    if (isReadOnly() || !object())
        return;

    // Comment or property
    if (p->parent() && p->propertyName() == QLatin1String("comment")) {
        const QString parentProperty = p->parent()->propertyName();
        emit propertyCommentChanged(parentProperty, p->value().toString());
    } else {
        emit propertyChanged(p->propertyName(), p->value());
    }
}

void PropertyEditor::clearDirty(IProperty *p)
{
    p->setDirty(false);

    if (p->kind() == IProperty::Property_Normal)
        return;

    IPropertyGroup *g = static_cast<IPropertyGroup*>(p);
    for (int i=0; i<g->propertyCount(); ++i)
        clearDirty(g->propertyAt(i));
}

void PropertyEditor::setObject(QObject *object)
{
    if (m_editor->initialInput())
        clearDirty(m_editor->initialInput());

    m_object = object;
    IPropertyGroup *old_properties = m_properties;
    m_properties = 0;
    m_prop_sheet = 0;

    m_indexToProperty.clear();

    if (m_object) {
        PropertyCollection *collection = new PropertyCollection(QLatin1String("<root>"));
        createPropertySheet(collection, object);
        m_properties = collection;
    }

    m_editor->setInitialInput(m_properties);

    delete old_properties;
}

void PropertyEditor::slotResetProperty(const QString &prop_name)
{
    QDesignerFormWindowInterface *form = m_core->formWindowManager()->activeFormWindow();
    if (form == 0) {
        qDebug("PropertyEditor::resetProperty(): widget does not belong to any form");
        return;
    }
    emit resetProperty(prop_name);
}

QString PropertyEditor::currentPropertyName() const
{
    const QModelIndex index = m_editor->selectionModel()->currentIndex();
    if (index.isValid()) {
        IProperty *property = static_cast<IProperty*>(index.internalPointer());

        while (property && property->isFake())
            property = property->parent();

        if (property)
            return property->propertyName();
    }

    return QString();
}

void PropertyEditor::slotCustomContextMenuRequested(const QPoint &pos)
{    
    const QModelIndex idx = m_editor->indexAt(pos);
    if (!idx.isValid())
        return;
    
    QPropertyEditorModel *model = m_editor->editorModel();
    IProperty *nonfake = model->privateData(idx);
    while (nonfake != 0 && nonfake->isFake())
        nonfake = nonfake->parent();

    const QDesignerPropertySheetExtension *sheet = m_prop_sheet;
    const QDesignerDynamicPropertySheetExtension *dynamicSheet = qt_extension<QDesignerDynamicPropertySheetExtension*>(m_core->extensionManager(), m_object);;
    if (!sheet || !dynamicSheet)
        return;

    int index = -1;
    const bool addEnabled = dynamicSheet->dynamicPropertiesAllowed();
    bool insertRemoveEnabled = false;
    if (addEnabled) {
        if (nonfake) {
            const int idx = sheet->indexOf(nonfake->propertyName());
            if (dynamicSheet->isDynamicProperty(idx)) {
                insertRemoveEnabled = true;
                index = idx;
            }
        }
    }

    QMenu menu(this);
    QAction *addAction = menu.addAction(tr("Add Dynamic Property..."));
    addAction->setEnabled(addEnabled);
    QAction *removeAction = menu.addAction(tr("Remove Dynamic Property"));
    removeAction->setEnabled(insertRemoveEnabled);
    const QAction *result = menu.exec(mapToGlobal(pos));

    if (result == removeAction && nonfake) {
        emit removeDynamicProperty(nonfake->propertyName());
    } else if (result == addAction) {
        NewDynamicPropertyDialog dlg(this);
        QStringList reservedNames;
        for (int i = 0; i < sheet->count(); i++) {
            if (!dynamicSheet->isDynamicProperty(i) || sheet->isVisible(i))
                reservedNames.append(sheet->propertyName(i));
        }
        dlg.setReservedNames(reservedNames);
        if (dlg.exec() == QDialog::Accepted) {
            const QString newName = dlg.propertyName();
            const QVariant newValue = dlg.propertyValue();

            emit addDynamicProperty(newName, newValue);
        }
    }
}

}
