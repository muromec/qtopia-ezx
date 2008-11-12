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

#ifndef QPROPERTYEDITOR_ITEMS_P_H
#define QPROPERTYEDITOR_ITEMS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "propertyeditor_global.h"
#include <shared_enums_p.h>

#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QMap>
#include <QtCore/QDateTime>
#include <QtCore/QUrl>

#include <QtGui/QCursor>
#include <QtGui/QPalette>
#include <QtGui/QFont>
#include <QtGui/QKeySequence>

class QWidget;
class QComboBox;

namespace qdesigner_internal {

class QT_PROPERTYEDITOR_EXPORT IProperty
{
    IProperty(const IProperty&);
    IProperty& operator=(const IProperty&);
public:
    enum Kind
    {
        Property_Normal,
        Property_Group

    // ### more
    };

    inline IProperty()
        : m_parent(0),
          m_changed(0),
          m_dirty(0),
          m_fake(0),
          m_reset(0) {}

    virtual ~IProperty() {}

    // ### pure
    bool changed() const { return m_changed; }
    void setChanged(bool b);

    bool dirty() const { return m_dirty; }
    void setDirty(bool b);

    bool hasReset() const { return m_reset; }
    void setHasReset(bool b) { m_reset = b; }

    bool isFake() const { return m_fake; }
    void setFake(bool b) { m_fake = b; }

    virtual IProperty::Kind kind() const = 0;

    virtual bool isSeparator() const { return false; }
    virtual IProperty *parent() const { return m_parent; }
    virtual void setParent(IProperty *parent) { m_parent = parent; }

    virtual QString propertyName() const = 0;

    virtual QVariant value() const = 0;
    virtual void setValue(const QVariant &value) = 0;

    virtual QString toString() const = 0;

    virtual QVariant decoration() const = 0;

    virtual bool hasEditor() const = 0;
    virtual QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const = 0;

    // ### pure
    virtual void updateEditorContents(QWidget *editor) { Q_UNUSED(editor); }
    virtual void updateValue(QWidget *editor) { Q_UNUSED(editor); }

    virtual bool hasExternalEditor() const = 0;
    virtual QWidget *createExternalEditor(QWidget *parent) = 0;

protected:
    IProperty *m_parent;
    uint m_changed : 1;
    uint m_dirty : 1;
    uint m_fake : 1;
    uint m_reset : 1;
};

class QT_PROPERTYEDITOR_EXPORT IPropertyGroup: public IProperty
{
public:
    virtual int indexOf(IProperty *property) const = 0;
    virtual int propertyCount() const = 0;
    virtual IProperty *propertyAt(int index) const = 0;
};

template <typename T>
class QT_PROPERTYEDITOR_EXPORT AbstractProperty: public IProperty
{
public:
    AbstractProperty(const T &value, const QString &name)
        : m_value(value), m_name(name) {}

    IProperty::Kind kind() const { return IProperty::Property_Normal; }

//
// IProperty Interface
//
    QVariant decoration() const { return QVariant(); }
    QString propertyName() const { return m_name; }
    QVariant value() const { return qVariantFromValue(m_value); }

    bool hasEditor() const { return true; }
    bool hasExternalEditor() const { return false; }
    QWidget *createExternalEditor(QWidget *parent) { Q_UNUSED(parent); return 0; }

protected:
    T m_value;
    const QString m_name;
};

class QT_PROPERTYEDITOR_EXPORT AbstractPropertyGroup: public IPropertyGroup
{
public:
    AbstractPropertyGroup(const QString &name)
        : m_name(name) {}

    ~AbstractPropertyGroup()
    { qDeleteAll(m_properties); }

    IProperty::Kind kind() const { return Property_Group; }

//
// IPropertyGroup Interface
//
    int indexOf(IProperty *property) const { return m_properties.indexOf(property); }
    int propertyCount() const { return m_properties.size(); }
    IProperty *propertyAt(int index) const { return m_properties.at(index); }

//
// IProperty Interface
//

    inline QString propertyName() const
    { return m_name; }

    inline QVariant decoration() const
    { return QVariant(); }

    QString toString() const;
    
    inline bool hasEditor() const
    { return true; }

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    virtual void updateEditorContents(QWidget *editor);

    inline bool hasExternalEditor() const
    { return false; }

    QWidget *createExternalEditor(QWidget *parent)
    { Q_UNUSED(parent); return 0; }

protected:
    const QString m_name;
    QList<IProperty*> m_properties;
};

class QT_PROPERTYEDITOR_EXPORT PropertyCollection: public IPropertyGroup
{
public:
    PropertyCollection(const QString &name);
    ~PropertyCollection();

    inline IProperty::Kind kind() const
    { return Property_Group; }

    void addProperty(IProperty *property);
    void removeProperty(IProperty *property);

//
// IPropertyGroup Interface
//
    int indexOf(IProperty *property) const;
    int propertyCount() const;
    IProperty *propertyAt(int index) const;

//
// IProperty Interface
//
    QString propertyName() const;

    QVariant value() const;
    void setValue(const QVariant &value);

    QVariant decoration() const { return QVariant(); }
    QString toString() const;

    bool hasEditor() const;
    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;

    bool hasExternalEditor() const;
    QWidget *createExternalEditor(QWidget *parent);

private:
    const QString m_name;
    QList<IProperty*> m_properties;
};

class QT_PROPERTYEDITOR_EXPORT IntProperty: public AbstractProperty<int>
{
public:
    IntProperty(int value, const QString &name);

    QString specialValue() const;
    void setSpecialValue(const QString &specialValue);

    void setRange(int low, int hi);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);

private:
    QString m_specialValue;
    int m_low;
    int m_hi;
};

class QT_PROPERTYEDITOR_EXPORT BoolProperty: public AbstractProperty<bool>
{
public:
    BoolProperty(bool value, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
};

class QT_PROPERTYEDITOR_EXPORT LongLongProperty: public AbstractProperty<qlonglong>
{
public:
    LongLongProperty(qlonglong value, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
};

class QT_PROPERTYEDITOR_EXPORT DoubleProperty: public AbstractProperty<double>
{
public:
    DoubleProperty(double value, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
};

class QT_PROPERTYEDITOR_EXPORT SpinBoxDoubleProperty: public AbstractProperty<double>
{
public:
    SpinBoxDoubleProperty(double value, const QString &name);

    QString specialValue() const;
    void setSpecialValue(const QString &specialValue);

    void setRange(double low, double hi);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);

private:
    QString m_specialValue;
    double m_low;
    double m_hi;
};

class QT_PROPERTYEDITOR_EXPORT CharProperty: public AbstractProperty<QChar>
{
public:
    CharProperty(QChar value, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
};

class QT_PROPERTYEDITOR_EXPORT StringProperty: public AbstractPropertyGroup
{
public:
    StringProperty(const QString &value, const QString &name,
                   TextPropertyValidationMode validationMode = ValidationMultiLine,
                   bool hasComment = false, const QString &comment = QString());


    QVariant value() const;
    void setValue(const QVariant &value);
    QString toString() const;

    bool hasEditor() const;
    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);

private:
    const TextPropertyValidationMode m_validationMode;
    QString m_value;
};

class QT_PROPERTYEDITOR_EXPORT SeparatorProperty: public StringProperty
{
public:
    SeparatorProperty(const QString &value, const QString &name);

    bool isSeparator() const { return true; }
    bool hasEditor() const;
    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
};

class QT_PROPERTYEDITOR_EXPORT ListProperty: public AbstractProperty<int>
{
public:
    ListProperty(const QStringList &items, int value,
                 const QString &name);

    QStringList items() const;

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);

private:
    QStringList m_items;
};

class QT_PROPERTYEDITOR_EXPORT MapProperty: public AbstractProperty<QVariant>
{
public:
    MapProperty(const QMap<QString, QVariant> &items, const QVariant &value,
                const QString &name, const QStringList &overrideKeys = QStringList());

    QStringList keys() const;
    QMap<QString, QVariant> items() const;
    int indexOf(const QVariant &value) const;

    QVariant value() const;
    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);

private:
    QMap<QString, QVariant> m_items;
    QStringList m_keys, comboKeys;
};

class QT_PROPERTYEDITOR_EXPORT FlagsProperty: public MapProperty
{
public:
    FlagsProperty(const QMap<QString, QVariant> &items, unsigned int m_value,
                  const QString &name);

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
};

class QT_PROPERTYEDITOR_EXPORT PointProperty: public AbstractPropertyGroup
{
public:
    PointProperty(const QPoint &value, const QString &name);

    QVariant value() const;
    void setValue(const QVariant &value);
};

class QT_PROPERTYEDITOR_EXPORT PointFProperty: public AbstractPropertyGroup
{
public:
    PointFProperty(const QPointF &value, const QString &name);

    QVariant value() const;
    void setValue(const QVariant &value);
};

class QT_PROPERTYEDITOR_EXPORT SizeProperty: public AbstractPropertyGroup
{
public:
    SizeProperty(const QSize &value, const QString &name);

    QVariant value() const;
    void setValue(const QVariant &value);
};

class QT_PROPERTYEDITOR_EXPORT SizeFProperty: public AbstractPropertyGroup
{
public:
    SizeFProperty(const QSizeF &value, const QString &name);

    QVariant value() const;
    void setValue(const QVariant &value);
};

class QT_PROPERTYEDITOR_EXPORT AlignmentProperty: public AbstractPropertyGroup
{
public:
    AlignmentProperty(const QMap<QString, QVariant> &items, Qt::Alignment value, const QString &name);

    QVariant value() const;
    void setValue(const QVariant &value);
};

class QT_PROPERTYEDITOR_EXPORT RectProperty: public AbstractPropertyGroup
{
public:
    RectProperty(const QRect &value, const QString &name);

    QVariant value() const;
    void setValue(const QVariant &value);
};

class QT_PROPERTYEDITOR_EXPORT RectFProperty: public AbstractPropertyGroup
{
public:
    RectFProperty(const QRectF &value, const QString &name);

    QVariant value() const;
    void setValue(const QVariant &value);
};

class QT_PROPERTYEDITOR_EXPORT ColorProperty: public AbstractPropertyGroup
{
public:
    ColorProperty(const QColor &value, const QString &name);

    QVariant value() const;
    void setValue(const QVariant &value);
    QVariant decoration() const;

    QString toString() const { return QLatin1String("  ") + AbstractPropertyGroup::toString(); } // ### temp hack remove me!!
    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);

};

class QT_PROPERTYEDITOR_EXPORT FontProperty: public AbstractPropertyGroup
{
public:
    FontProperty(const QFont &value, const QString &name, QWidget *selectedWidget);

    QString toString() const;
    QVariant value() const;
    void setValue(const QVariant &value);
    QVariant decoration() const;
private:
    QFont m_font;
    QWidget *m_selectedWidget;
};

class QT_PROPERTYEDITOR_EXPORT SizePolicyProperty: public AbstractPropertyGroup
{
public:
    SizePolicyProperty(const QSizePolicy &value, const QString &name);

    QString toString() const;
    QVariant value() const;
    void setValue(const QVariant &value);
    QVariant decoration() const;
};

class QT_PROPERTYEDITOR_EXPORT DateTimeProperty: public AbstractProperty<QDateTime>
{
public:
    DateTimeProperty(const QDateTime &value, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
};

class QT_PROPERTYEDITOR_EXPORT DateProperty: public AbstractProperty<QDate>
{
public:
    DateProperty(const QDate &value, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
};

class QT_PROPERTYEDITOR_EXPORT TimeProperty: public AbstractProperty<QTime>
{
public:
    TimeProperty(const QTime &value, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
};

class QT_PROPERTYEDITOR_EXPORT KeySequenceProperty: public AbstractProperty<QKeySequence>
{
public:
    KeySequenceProperty(const QKeySequence &value, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
};

class QT_PROPERTYEDITOR_EXPORT CursorProperty: public AbstractProperty<QCursor>
{
public:
    CursorProperty(const QCursor &value, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;
    QVariant decoration() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);

private:
    static QString cursorName(int shape);
    static QPixmap cursorPixmap(int shape);
    void addCursor(QComboBox *combo, int shape) const;
};

class QT_PROPERTYEDITOR_EXPORT UrlProperty: public AbstractPropertyGroup
{
public:
    UrlProperty(const QUrl &value, const QString &name);

    QVariant value() const;
    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);

private:
    QUrl m_value;
};

class QT_PROPERTYEDITOR_EXPORT StringListProperty: public AbstractProperty<QStringList>
{
public:
    StringListProperty(const QStringList &value, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
};

class QT_PROPERTYEDITOR_EXPORT UIntProperty: public AbstractProperty<uint>
{
public:
    UIntProperty(uint value, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
};

class QT_PROPERTYEDITOR_EXPORT ULongLongProperty: public AbstractProperty<qulonglong>
{
public:
    ULongLongProperty(qulonglong value, const QString &name);

    void setValue(const QVariant &value);
    QString toString() const;

    QWidget *createEditor(QWidget *parent, const QObject *target, const char *receiver) const;
    void updateEditorContents(QWidget *editor);
    void updateValue(QWidget *editor);
};

}  // namespace qdesigner_internal

#endif // QPROPERTYEDITOR_ITEMS_P_H
