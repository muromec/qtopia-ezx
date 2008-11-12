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

#include "qdesigner_propertysheet_p.h"
#include "qdesigner_utils_p.h"
#include "layoutinfo_p.h"
#include "qlayout_widget_p.h"

#include <formbuilderextra_p.h>

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerWidgetDataBaseInterface>
#include <QtDesigner/QExtensionManager>

#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>

#include <QtGui/QLayout>
#include <QtGui/QDockWidget>
#include <QtGui/QDialog>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QStyle>
#include <QtGui/QApplication>
#include <QtGui/QToolBar>

static const QMetaObject *propertyIntroducedBy(const QMetaObject *meta, int index)
{
    if (index >= meta->propertyOffset())
        return meta;

    if (meta->superClass())
        return propertyIntroducedBy(meta->superClass(), index);

    return 0;
}

static QDesignerFormEditorInterface *formEditorForWidget(QWidget *w) {
    const QDesignerFormWindowInterface *formWindow =  QDesignerFormWindowInterface::findFormWindow(w);
    if (!formWindow)
        return 0;
    return formWindow->core();
}

static bool hasLayoutAttributes(QObject *object)
{
    if (!object->isWidgetType())
        return false;

    QWidget *w =  qobject_cast<QWidget *>(object);
    if (const QDesignerFormEditorInterface *core = formEditorForWidget(w)) {
        if (const QDesignerWidgetDataBaseInterface *db = core->widgetDataBase()) {
            if (db->isContainer(w))
                return true;
        }
    }
    return false;
}

// ---------- QDesignerPropertySheet::Info

QDesignerPropertySheet::Info::Info() :
    changed(0),
    visible(1),
    attribute(0),
    reset(1),
    defaultDynamic(0),
    propertyType(PropertyNone)
{
}

// ----------- QDesignerPropertySheet

QDesignerPropertySheet::ObjectType QDesignerPropertySheet::objectType(const QObject *o)
{
    if (qobject_cast<const QLayout *>(o))
        return ObjectLayout;

    if (!o->isWidgetType())
        return ObjectNone;

    if (qobject_cast<const QLayoutWidget *>(o))
        return ObjectLayoutWidget;

    if (qobject_cast<const QLabel*>(o))
        return ObjectLabel;

    if (o->inherits("Q3GroupBox"))
        return ObjectQ3GroupBox;

    return ObjectNone;
}

QDesignerPropertySheet::PropertyType QDesignerPropertySheet::propertyTypeFromName(const QString &name)
{
    if (name == QLatin1String("layoutLeftMargin"))
        return  PropertyLayoutLeftMargin;
    if (name == QLatin1String("layoutTopMargin"))
        return  PropertyLayoutTopMargin;
    if (name == QLatin1String("layoutRightMargin"))
        return  PropertyLayoutRightMargin;
    if (name == QLatin1String("layoutBottomMargin"))
        return  PropertyLayoutBottomMargin;
    if (name == QLatin1String("layoutSpacing"))
        return  PropertyLayoutSpacing;
    if (name == QLatin1String("layoutHorizontalSpacing"))
        return  PropertyLayoutHorizontalSpacing;
    if (name == QLatin1String("layoutVerticalSpacing"))
        return  PropertyLayoutVerticalSpacing;
    if (name == QLatin1String("buddy"))
        return  PropertyBuddy;
    if (name == QLatin1String("sizeConstraint"))
        return  PropertySizeConstraint;
    if (name == QLatin1String("geometry"))
        return PropertyGeometry;
    if (name == QLatin1String("checkable"))
        return PropertyCheckable;
    if (name.startsWith(QLatin1String("accessible")))
        return  PropertyAccessibility;
    return PropertyNone;
}

QDesignerPropertySheet::QDesignerPropertySheet(QObject *object, QObject *parent)
    : QObject(parent),
      m_meta(object->metaObject()),
      m_objectType(objectType(object)),
      m_canHaveLayoutAttributes( hasLayoutAttributes(object)),
      m_object(object),
      m_lastLayout(0),
      m_lastLayoutPropertySheet(0),
      m_LastLayoutByDesigner(false)
{
    const QMetaObject *baseMeta = m_meta;

    while (baseMeta && QString::fromUtf8(baseMeta->className()).startsWith(QLatin1String("QDesigner"))) {
        baseMeta = baseMeta->superClass();
    }
    Q_ASSERT(baseMeta != 0);

    for (int index=0; index<count(); ++index) {
        const QMetaProperty p = m_meta->property(index);
        const QString name = QString::fromUtf8(p.name());
        if (p.type() == QVariant::KeySequence) {
            createFakeProperty(name);
        } else {
            setVisible(index, false); // use the default for `real' properties
        }

        QString pgroup = QString::fromUtf8(baseMeta->className());

        if (const QMetaObject *pmeta = propertyIntroducedBy(baseMeta, index)) {
            pgroup = QString::fromUtf8(pmeta->className());
        }

        Info &info = ensureInfo(index);
        info.group = pgroup;
        info.propertyType = propertyTypeFromName(name);

        if (p.type() == QVariant::Cursor) {
            info.defaultValue = p.read(m_object);
        }
    }

    if (object->isWidgetType()) {
        createFakeProperty(QLatin1String("focusPolicy"));
        createFakeProperty(QLatin1String("cursor"));
        createFakeProperty(QLatin1String("toolTip"));
        createFakeProperty(QLatin1String("whatsThis"));
        createFakeProperty(QLatin1String("acceptDrops"));
        createFakeProperty(QLatin1String("dragEnabled"));
        createFakeProperty(QLatin1String("windowModality"));
        if (qobject_cast<const QToolBar *>(m_object)) // prevent toolbars from being dragged off
            createFakeProperty(QLatin1String("floatable"), QVariant(true));


        if (m_canHaveLayoutAttributes) {
            static const QString layoutGroup = QLatin1String("Layout");
            int pindex = count();
            createFakeProperty(QLatin1String("layoutLeftMargin"), 0);
            setAttribute(pindex, true);
            setPropertyGroup(pindex, layoutGroup);

            pindex = count();
            createFakeProperty(QLatin1String("layoutTopMargin"), 0);
            setAttribute(pindex, true);
            setPropertyGroup(pindex, layoutGroup);

            pindex = count();
            createFakeProperty(QLatin1String("layoutRightMargin"), 0);
            setAttribute(pindex, true);
            setPropertyGroup(pindex, layoutGroup);

            pindex = count();
            createFakeProperty(QLatin1String("layoutBottomMargin"), 0);
            setAttribute(pindex, true);
            setPropertyGroup(pindex, layoutGroup);

            pindex = count();
            createFakeProperty(QLatin1String("layoutSpacing"), 0);
            setAttribute(pindex, true);
            setPropertyGroup(pindex, layoutGroup);

            pindex = count();
            createFakeProperty(QLatin1String("layoutHorizontalSpacing"), 0);
            setAttribute(pindex, true);
            setPropertyGroup(pindex, layoutGroup);

            pindex = count();
            createFakeProperty(QLatin1String("layoutVerticalSpacing"), 0);
            setAttribute(pindex, true);
            setPropertyGroup(pindex, layoutGroup);
        }

        if (m_objectType == ObjectLabel)
            createFakeProperty(QLatin1String("buddy"), QVariant(QByteArray()));
    }

    if (qobject_cast<const QDialog*>(object)) {
        createFakeProperty(QLatin1String("modal"));
    }
    if (qobject_cast<const QDockWidget*>(object)) {
        createFakeProperty(QLatin1String("floating"));
    }

    typedef QList<QByteArray> ByteArrayList;
    const ByteArrayList names = object->dynamicPropertyNames();
    if (!names.empty()) {
        const ByteArrayList::const_iterator cend =  names.constEnd();
        for (ByteArrayList::const_iterator it = names.constBegin(); it != cend; ++it) {
            const char* cName = it->constData();
            const QString name = QString::fromLatin1(cName);
            const int idx = addDynamicProperty(name, object->property(cName));
            if (idx != -1)
                ensureInfo(idx).defaultDynamic = true;
        }
    }
}

QDesignerPropertySheet::~QDesignerPropertySheet()
{
}


QObject *QDesignerPropertySheet::object() const
{
    return m_object;
}

bool QDesignerPropertySheet::dynamicPropertiesAllowed() const
{
    return true;
}

bool QDesignerPropertySheet::canAddDynamicProperty(const QString &propName) const
{
    const int index = m_meta->indexOfProperty(propName.toUtf8());
    if (index != -1)
        return false; // property already exists and is not a dynamic one
    if (m_addIndex.contains(propName)) {
        const int idx = m_addIndex.value(propName);
        if (isVisible(idx))
            return false; // dynamic property already exists
        else
            return true;
    }
    if (propName.startsWith(QLatin1String("_q_")))
        return false;
    return true;
}

int QDesignerPropertySheet::addDynamicProperty(const QString &propName, const QVariant &value)
{
    if (!value.isValid())
        return -1; // property has invalid type
    if (!canAddDynamicProperty(propName))
        return -1;
    if (m_addIndex.contains(propName)) {
        const int idx = m_addIndex.value(propName);
        // have to be invisible, this was checked in canAddDynamicProperty() method
        setVisible(idx, true);
        m_addProperties.insert(idx, value);
        setChanged(idx, false);
        const int index = m_meta->indexOfProperty(propName.toUtf8());
        m_info[index].defaultValue = value;
        return idx;
    }

    const int index = count();
    m_addIndex.insert(propName, index);
    m_addProperties.insert(index, value);
    setVisible(index, true);
    setChanged(index, false);
    m_info[index].defaultValue = value;

    setPropertyGroup(index, tr("Dynamic Properties"));
    return index;
}

bool QDesignerPropertySheet::removeDynamicProperty(int index)
{
    if (!m_addIndex.contains(propertyName(index)))
        return false;

    setVisible(index, false);
    return true;
}

bool QDesignerPropertySheet::isDynamic(int index) const
{
    if (!m_addProperties.contains(index))
        return false;

    switch (propertyType(index)) {
    case PropertyBuddy:
        if (m_objectType == ObjectLabel)
            return false;
        break;
    case PropertyLayoutLeftMargin:
    case PropertyLayoutTopMargin:
    case PropertyLayoutRightMargin:
    case PropertyLayoutBottomMargin:
    case PropertyLayoutSpacing:
    case PropertyLayoutHorizontalSpacing:
    case PropertyLayoutVerticalSpacing:
        if (m_object->isWidgetType() && m_canHaveLayoutAttributes)
            return false;
    default:
        break;
    }
    return true;
}

bool QDesignerPropertySheet::isDynamicProperty(int index) const
{
    if (m_info.value(index).defaultDynamic)
        return false;

    return isDynamic(index);
}

void QDesignerPropertySheet::createFakeProperty(const QString &propertyName, const QVariant &value)
{
    // fake properties
    const int index = m_meta->indexOfProperty(propertyName.toUtf8());
    if (index != -1) {
        if (!m_meta->property(index).isDesignable())
            return;
        setVisible(index, false);
        const QVariant v = value.isValid() ? value : metaProperty(index);
        m_fakeProperties.insert(index, v);
    } else if (value.isValid()) { // additional properties
        const int index = count();
        m_addIndex.insert(propertyName, index);
        m_addProperties.insert(index, value);
        ensureInfo(index).propertyType = propertyTypeFromName(propertyName);
    }
}

bool QDesignerPropertySheet::isAdditionalProperty(int index) const
{
    return m_addProperties.contains(index);
}

bool QDesignerPropertySheet::isFakeProperty(int index) const
{
    // additional properties must be fake
    return (m_fakeProperties.contains(index) || isAdditionalProperty(index));
}

int QDesignerPropertySheet::count() const
{
    return m_meta->propertyCount() + m_addProperties.count();
}

int QDesignerPropertySheet::indexOf(const QString &name) const
{
    int index = m_meta->indexOfProperty(name.toUtf8());

    if (index == -1)
        index = m_addIndex.value(name, -1);

    return index;
}

QDesignerPropertySheet::PropertyType QDesignerPropertySheet::propertyType(int index) const
{
    const InfoHash::const_iterator it = m_info.constFind(index);
    if (it == m_info.constEnd())
        return PropertyNone;
    return it.value().propertyType;
}

QString QDesignerPropertySheet::propertyName(int index) const
{
    if (isAdditionalProperty(index))
        return m_addIndex.key(index);

    return QString::fromUtf8(m_meta->property(index).name());
}

QString QDesignerPropertySheet::propertyGroup(int index) const
{
    const QString g = m_info.value(index).group;

    if (!g.isEmpty())
        return g;

    if (propertyType(index) == PropertyAccessibility)
        return QString::fromUtf8("Accessibility");

    if (isAdditionalProperty(index))
        return QString::fromUtf8(m_meta->className());

    return g;
}

void QDesignerPropertySheet::setPropertyGroup(int index, const QString &group)
{
    ensureInfo(index).group = group;
}

QVariant QDesignerPropertySheet::property(int index) const
{
    if (isAdditionalProperty(index)) {
        if (isFakeLayoutProperty(index)) {
            QDesignerPropertySheetExtension *layoutPropertySheet;
            if (layout(&layoutPropertySheet) && layoutPropertySheet) {
                const QString newPropName = transformLayoutPropertyName(index);
                if (!newPropName.isEmpty())
                    return layoutPropertySheet->property(layoutPropertySheet->indexOf(newPropName));
            }
        }
        return m_addProperties.value(index);
    }

    if (isFakeProperty(index)) {
        return m_fakeProperties.value(index);
    }

    return metaProperty(index);
}

QVariant QDesignerPropertySheet::metaProperty(int index) const
{
    Q_ASSERT(!isFakeProperty(index));

    const QMetaProperty p = m_meta->property(index);
    QVariant v = p.read(m_object);

    static const QString doubleColon = QLatin1String("::");
    if (p.isFlagType()) {
        qdesigner_internal::FlagType e;
        e.value = v;
        const QMetaEnum me = p.enumerator();
        QString scope = QString::fromUtf8(me.scope());
        if (!scope.isEmpty())
            scope += doubleColon;
        for (int i=0; i<me.keyCount(); ++i) {
            const QString key = scope + QLatin1String(me.key(i));
            e.items.insert(key, me.keyToValue(key.toUtf8().constData()));
        }

        qVariantSetValue(v, e);
    } else if (p.isEnumType()) {
        qdesigner_internal::EnumType e;
        e.value = v;
        const QMetaEnum me = p.enumerator();
        QString scope = QString::fromUtf8(me.scope());
        if (!scope.isEmpty())
            scope += doubleColon;
        for (int i=0; i<me.keyCount(); ++i) {
            const QString key = scope + QLatin1String(me.key(i));
            e.items.insert(key, me.keyToValue(key.toUtf8().constData()));
            e.names.append(key);
        }

        qVariantSetValue(v, e);
    }

    return v;
}

QVariant QDesignerPropertySheet::resolvePropertyValue(const QVariant &value) const
{
    if (qVariantCanConvert<qdesigner_internal::FlagType>(value))
       return qvariant_cast<qdesigner_internal::FlagType>(value).value;

    if (qVariantCanConvert<qdesigner_internal::EnumType>(value))
       return qvariant_cast<qdesigner_internal::EnumType>(value).value;

    return value;
}

void QDesignerPropertySheet::setFakeProperty(int index, const QVariant &value)
{
    Q_ASSERT(isFakeProperty(index));

    QVariant &v = m_fakeProperties[index];

    if (qVariantCanConvert<qdesigner_internal::FlagType>(value) || qVariantCanConvert<qdesigner_internal::EnumType>(value)) {
        v = value;
    } else if (qVariantCanConvert<qdesigner_internal::FlagType>(v)) {
        qdesigner_internal::FlagType f = qvariant_cast<qdesigner_internal::FlagType>(v);
        f.value = value;
        qVariantSetValue(v, f);
        Q_ASSERT(f.value.type() == QVariant::Int);
    } else if (qVariantCanConvert<qdesigner_internal::EnumType>(v)) {
        qdesigner_internal::EnumType e = qvariant_cast<qdesigner_internal::EnumType>(v);
        e.value = value;
        qVariantSetValue(v, e);
        Q_ASSERT(e.value.type() == QVariant::Int);
    } else {
        v = value;
    }
}

// Buddy needs to be byte array, else uic won't work
static QVariant toByteArray(const QVariant &value) {
    if (value.type() == QVariant::ByteArray)
        return value;
    const QByteArray ba = value.toString().toUtf8();
    return QVariant(ba);
}

void QDesignerPropertySheet::setProperty(int index, const QVariant &value)
{
    if (isAdditionalProperty(index)) {
        if (m_objectType == ObjectLabel && propertyType(index) == PropertyBuddy) {
            QFormBuilderExtra::applyBuddy(value.toString(), QFormBuilderExtra::BuddyApplyVisibleOnly, qobject_cast<QLabel *>(m_object));
            m_addProperties[index] = toByteArray(value);
            return;
        }

        if (isFakeLayoutProperty(index)) {
            QDesignerPropertySheetExtension *layoutPropertySheet;
            if (layout(&layoutPropertySheet) && layoutPropertySheet) {
                const QString newPropName = transformLayoutPropertyName(index);
                if (!newPropName.isEmpty())
                    layoutPropertySheet->setProperty(layoutPropertySheet->indexOf(newPropName), value);
            }
        }

        if (isDynamicProperty(index)) {
            m_object->setProperty(propertyName(index).toUtf8(), value);
            if (m_object->isWidgetType()) {
                QWidget *w = qobject_cast<QWidget *>(m_object);
                w->setStyleSheet(w->styleSheet());
            }
        }
        m_addProperties[index] = value;
    } else if (isFakeProperty(index)) {
        setFakeProperty(index, value);
    } else {
        const QMetaProperty p = m_meta->property(index);
        p.write(m_object, resolvePropertyValue(value));
        if (qobject_cast<QGroupBox *>(m_object) && propertyType(index) == PropertyCheckable) {
            const int idx = indexOf(QLatin1String("focusPolicy"));
            if (!isChanged(idx)) {
                qdesigner_internal::EnumType e = qVariantValue<qdesigner_internal::EnumType>(property(idx));
                if (value.toBool()) {
                    const QMetaProperty p = m_meta->property(idx);
                    p.write(m_object, Qt::NoFocus);
                    e.value = Qt::StrongFocus;
                    QVariant v;
                    qVariantSetValue(v, e);
                    setFakeProperty(idx, v);
                } else {
                    e.value = Qt::NoFocus;
                    QVariant v;
                    qVariantSetValue(v, e);
                    setFakeProperty(idx, v);
                }
            }
        }
    }
}

bool QDesignerPropertySheet::hasReset(int index) const
{
    if (isAdditionalProperty(index))
        return m_info.value(index).reset;

    return true;
}

bool QDesignerPropertySheet::reset(int index)
{
    if (isDynamic(index)) {
        const QString propName = propertyName(index);
        const QVariant oldValue = m_addProperties.value(index);
        const QVariant newValue = m_info.value(index).defaultValue;
        if (oldValue == newValue)
            return true;
        m_object->setProperty(propName.toUtf8(), newValue);
        m_addProperties[index] = newValue;
        return true;
    } else if (!m_info.value(index).defaultValue.isNull()) {
        setProperty(index, m_info.value(index).defaultValue);
        return true;
    }
    if (isAdditionalProperty(index)) {
        const PropertyType pType = propertyType(index);

        if (m_objectType == ObjectLabel && pType == PropertyBuddy) {
            setProperty(index, QVariant(QByteArray()));
            return true;
        }

        if (isFakeLayoutProperty(index)) {
            int value = -1;
            switch (m_objectType) {
            case ObjectQ3GroupBox: {
                const QWidget *w = qobject_cast<const QWidget *>(m_object);
                switch (pType) {
                case PropertyLayoutLeftMargin:
                    value = w->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
                    break;
                case PropertyLayoutTopMargin:
                    value = w->style()->pixelMetric(QStyle::PM_LayoutTopMargin);
                    break;
                case PropertyLayoutRightMargin:
                    value = w->style()->pixelMetric(QStyle::PM_LayoutRightMargin);
                    break;
                case PropertyLayoutBottomMargin:
                    value = w->style()->pixelMetric(QStyle::PM_LayoutBottomMargin);
                    break;
                case PropertyLayoutSpacing:
                case PropertyLayoutHorizontalSpacing:
                case PropertyLayoutVerticalSpacing:
                    value = -1;
                    break;
                default:
                    break;
                }
            }
                break;
            case ObjectLayoutWidget:
                if (pType == PropertyLayoutLeftMargin ||
                        pType == PropertyLayoutTopMargin ||
                        pType == PropertyLayoutRightMargin ||
                        pType == PropertyLayoutBottomMargin)
                    value = 0;
                break;
            default:
                break;
            }
            setProperty(index, value);
            return true;
        }
        return false;
    } else if (isFakeProperty(index)) {
        const QMetaProperty p = m_meta->property(index);
        const bool result = p.reset(m_object);
        m_fakeProperties[index] = p.read(m_object);
        return result;
    } else if (propertyType(index) == PropertyGeometry && m_object->isWidgetType()) {
        if (QWidget *w = qobject_cast<QWidget*>(m_object)) {
            QWidget *widget = w;
            QDesignerFormWindowInterface *formWindow = QDesignerFormWindowInterface::findFormWindow(widget);
            if (qdesigner_internal::Utils::isCentralWidget(formWindow, widget) && formWindow->parentWidget())
                widget = formWindow->parentWidget();

            if (widget != w && widget->parentWidget()) {
                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
                widget->parentWidget()->adjustSize();
            }
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            widget->adjustSize();
            return true;
        }
    }

    // ### TODO: reset for fake properties.

    const QMetaProperty p = m_meta->property(index);
    return p.reset(m_object);
}

bool QDesignerPropertySheet::isChanged(int index) const
{
    if (isAdditionalProperty(index)) {
        if (isFakeLayoutProperty(index)) {
            QDesignerPropertySheetExtension *layoutPropertySheet;
            if (layout(&layoutPropertySheet) && layoutPropertySheet) {
                const QString newPropName = transformLayoutPropertyName(index);
                if (!newPropName.isEmpty())
                    return layoutPropertySheet->isChanged(layoutPropertySheet->indexOf(newPropName));
            }
        }
    }
    return m_info.value(index).changed;
}

void QDesignerPropertySheet::setChanged(int index, bool changed)
{
    if (isAdditionalProperty(index)) {
        if (isFakeLayoutProperty(index)) {
            QDesignerPropertySheetExtension *layoutPropertySheet;
            if (layout(&layoutPropertySheet) && layoutPropertySheet) {
                const QString newPropName = transformLayoutPropertyName(index);
                if (!newPropName.isEmpty())
                    layoutPropertySheet->setChanged(layoutPropertySheet->indexOf(newPropName), changed);
            }
        }
    }
    ensureInfo(index).changed = changed;
}

bool QDesignerPropertySheet::isFakeLayoutProperty(int index) const
{
    if (!isAdditionalProperty(index))
        return false;

    switch (propertyType(index)) {
    case PropertySizeConstraint:
        return true;
    case PropertyLayoutLeftMargin:
    case PropertyLayoutTopMargin:
    case PropertyLayoutRightMargin:
    case PropertyLayoutBottomMargin:
    case PropertyLayoutSpacing:
    case PropertyLayoutHorizontalSpacing:
    case PropertyLayoutVerticalSpacing:
        return m_canHaveLayoutAttributes;
    default:
        break;
    }
    return false;
}

bool QDesignerPropertySheet::isVisible(int index) const
{
    if (isAdditionalProperty(index)) {
        if (isFakeLayoutProperty(index) && m_object->isWidgetType()) {
            QLayout *currentLayout = layout();
            if (!currentLayout)
                return false;
            switch (propertyType(index)) {
            case  PropertyLayoutSpacing:
                if (qobject_cast<const QGridLayout *>(currentLayout))
                    return false;
                break;
            case PropertyLayoutHorizontalSpacing:
            case PropertyLayoutVerticalSpacing:
                if (!qobject_cast<const QGridLayout *>(currentLayout))
                    return false;
                break;
            default:
                break;
            }
            return true;
        }
        return m_info.value(index).visible;
    }

    if (isFakeProperty(index))
        return true;

    const QMetaProperty p = m_meta->property(index);
    return (p.isWritable() && p.isDesignable(m_object)) || m_info.value(index).visible;
}

void QDesignerPropertySheet::setVisible(int index, bool visible)
{
    ensureInfo(index).visible = visible;
}

bool QDesignerPropertySheet::isAttribute(int index) const
{
    if (isAdditionalProperty(index))
        return m_info.value(index).attribute;

    if (isFakeProperty(index))
        return false;

    return m_info.value(index).attribute;
}

void QDesignerPropertySheet::setAttribute(int index, bool attribute)
{
    ensureInfo(index).attribute = attribute;
}

QLayout* QDesignerPropertySheet::layout(QDesignerPropertySheetExtension **layoutPropertySheet) const
{
    // Return the layout and its property sheet
    // only if it is managed by designer and not one created on a custom widget.
    if (layoutPropertySheet)
        *layoutPropertySheet = 0;

    if (!m_object->isWidgetType() || !m_canHaveLayoutAttributes)
        return 0;

    QWidget *widget = qobject_cast<QWidget*>(m_object);
    QLayout *widgetLayout = qdesigner_internal::LayoutInfo::internalLayout(widget);
    if (!widgetLayout) {
        m_lastLayout = 0;
        m_lastLayoutPropertySheet = 0;
        return 0;
    }
    // Smart logic to avoid retrieving the meta DB from the widget every time.
    if (widgetLayout != m_lastLayout) {
        m_lastLayout = widgetLayout;
        m_LastLayoutByDesigner = false;
        m_lastLayoutPropertySheet = 0;
        // Is this a layout managed by designer or some layout on a custom widget?
        if (QDesignerFormEditorInterface *core = formEditorForWidget(widget)) {
            if (qdesigner_internal::LayoutInfo::managedLayout(core ,widgetLayout)) {
                m_LastLayoutByDesigner = true;
                m_lastLayoutPropertySheet = qt_extension<QDesignerPropertySheetExtension*>(core->extensionManager(), m_lastLayout);
            }
        }
    }
    if (!m_LastLayoutByDesigner)
        return 0;

    if (layoutPropertySheet)
        *layoutPropertySheet = m_lastLayoutPropertySheet;

    return  m_lastLayout;
}

QDesignerPropertySheet::Info &QDesignerPropertySheet::ensureInfo(int index)
{
    InfoHash::iterator it = m_info.find(index);
    if (it == m_info.end()) {
        it = m_info.insert(index, Info());
    }
    return it.value();
}

QString QDesignerPropertySheet::transformLayoutPropertyName(int index) const
{
    switch (propertyType(index)) {
    case PropertyLayoutLeftMargin:
        return QLatin1String("leftMargin");
    case PropertyLayoutTopMargin:
        return QLatin1String("topMargin");
    case PropertyLayoutRightMargin:
        return QLatin1String("rightMargin");
    case PropertyLayoutBottomMargin:
        return QLatin1String("bottomMargin");
    case PropertyLayoutSpacing:
        return QLatin1String("spacing");
    case PropertyLayoutHorizontalSpacing:
        return QLatin1String("horizontalSpacing");
    case PropertyLayoutVerticalSpacing:
        return QLatin1String("verticalSpacing");
    default:
        break;
    }
    return QString();
}

// ---------- QDesignerPropertySheetFactory
QDesignerPropertySheetFactory::QDesignerPropertySheetFactory(QExtensionManager *parent)
    : QExtensionFactory(parent)
{
}

QObject *QDesignerPropertySheetFactory::extension(QObject *object, const QString &iid) const
{
    if (!object)
        return 0;

    if (iid != Q_TYPEID(QDesignerPropertySheetExtension) && iid != Q_TYPEID(QDesignerDynamicPropertySheetExtension))
        return 0;

    if (!m_extensions.contains(object)) {
        if (QObject *ext = new QDesignerPropertySheet(object, const_cast<QDesignerPropertySheetFactory*>(this))) {
            connect(ext, SIGNAL(destroyed(QObject*)), this, SLOT(objectDestroyed(QObject*)));
            m_extensions.insert(object, ext);
        }
    }

    if (!m_extended.contains(object)) {
        connect(object, SIGNAL(destroyed(QObject*)), this, SLOT(objectDestroyed(QObject*)));
        m_extended.insert(object, true);
    }

    return m_extensions.value(object);
}

void QDesignerPropertySheetFactory::objectDestroyed(QObject *object)
{
    QMutableMapIterator<QObject*, QObject*> it(m_extensions);
    while (it.hasNext()) {
        it.next();

        QObject *o = it.key();
        if (o == object || object == it.value()) {
            it.remove();
        }
    }

    m_extended.remove(object);
}

