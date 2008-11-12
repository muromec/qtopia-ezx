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

#include "qpropertyeditor_items_p.h"
#include "flagbox_p.h"
#include "stringlisteditorbutton.h"
#include "defs.h"
#include "qlonglongvalidator.h"
#include "qtcolorbutton.h"

#include <qdesigner_utils_p.h>
#include <textpropertyeditor_p.h>

#include <QtGui/QLineEdit>
#include <QtGui/QListView>
#include <QtGui/QComboBox>
#include <QtGui/QSpinBox>
#include <QtGui/QValidator>
#include <QtGui/QFontDatabase>
#include <QtGui/QPainter>
#include <QtGui/QDateTimeEdit>
#include <QtGui/QApplication>
#include <QtGui/QBitmap>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QHBoxLayout>
#include <QtCore/QUrl>
#include <QContextMenuEvent>
#include <private/qfont_p.h>

#include <QtCore/qdebug.h>
#include <limits.h>
#include <math.h>

using namespace qdesigner_internal;

Q_GLOBAL_STATIC(QFontDatabase, fontDatabase)

static QString matchStringInKeys(const QString &str, const QMap<QString, QVariant> &items) {
    for (QMap<QString, QVariant>::const_iterator it = items.begin(); it != items.end(); ++it) {
        if (it.key().contains(str))
            return it.key();
    }
    return str;
}


void IProperty::setDirty(bool b)
{
    if (isFake()) {
        IProperty *p = parent();
        while (p != 0 && p->isFake())
            p = p->parent();
        if (p != 0)
            p->setDirty(true);
    } else {
        m_dirty = b;
    }
}

void IProperty::setChanged(bool b)
{
    if (isFake()) {
        IProperty *p = parent();
        while (p != 0 && p->isFake())
            p = p->parent();
        if (p != 0)
            p->setChanged(true);
    } else {
        m_changed = b;
    }
    setDirty(true);
}

// -------------------------------------------------------------------------

QWidget *AbstractPropertyGroup::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    Q_UNUSED(target);
    Q_UNUSED(receiver);

    QLabel *label = new QLabel(parent);
    label->setIndent(2); // ### hardcode it should have the same value of textMargin in QItemDelegate
    label->setBackgroundRole(QPalette::Base);
    return label;
}

void AbstractPropertyGroup::updateEditorContents(QWidget *editor)
{
    QLabel *label = qobject_cast<QLabel*>(editor);
    if (label == 0)
        return;
    label->setText(toString());
}

QString AbstractPropertyGroup::toString() const
{
    QString text = QString(QLatin1Char('['));
    for (int i=0; i<propertyCount(); ++i) {
        if (i)
            text += QLatin1String(", ");
        text += propertyAt(i)->toString();
    }
    text += QLatin1Char(']');
    return text;
}

// -------------------------------------------------------------------------
BoolProperty::BoolProperty(bool value, const QString &name)
    : AbstractProperty<bool>(value, name)
{
}

void BoolProperty::setValue(const QVariant &value)
{
    m_value = value.toBool();
}

QString BoolProperty::toString() const
{
    return m_value ? QLatin1String("true") : QLatin1String("false");
}

QWidget *BoolProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QComboBox *combo = new QComboBox(parent);
    combo->view()->setTextElideMode(Qt::ElideLeft);
    combo->setFrame(0);
    combo->addItems(QStringList() << QString::fromUtf8("false") << QString::fromUtf8("true"));
    QObject::connect(combo, SIGNAL(activated(int)), target, receiver);

    return combo;
}

void BoolProperty::updateEditorContents(QWidget *editor)
{
    if (QComboBox *combo = qobject_cast<QComboBox*>(editor)) {
        combo->setCurrentIndex(m_value ? 1 : 0);
    }
}

void BoolProperty::updateValue(QWidget *editor)
{
    if (const QComboBox *combo = qobject_cast<const QComboBox*>(editor)) {
        const bool newValue = combo->currentIndex() ? true : false;

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }
    }
}

// -------------------------------------------------------------------------
PointProperty::PointProperty(const QPoint &value, const QString &name)
    : AbstractPropertyGroup(name)
{
    IProperty *px = new IntProperty(value.x(), QString(QLatin1Char('x')));
    px->setFake(true);
    px->setParent(this);

    IProperty *py = new IntProperty(value.y(), QString(QLatin1Char('y')));
    py->setFake(true);
    py->setParent(this);

    m_properties << px << py;
}

QVariant PointProperty::value() const
{
    return QPoint(propertyAt(0)->value().toInt(),
                  propertyAt(1)->value().toInt());
}

void PointProperty::setValue(const QVariant &value)
{
    const QPoint pt = value.toPoint();
    propertyAt(0)->setValue(pt.x());
    propertyAt(1)->setValue(pt.y());
}

// -------------------------------------------------------------------------
PointFProperty::PointFProperty(const QPointF &value, const QString &name)
    : AbstractPropertyGroup(name)
{
    DoubleProperty *px = new DoubleProperty(value.x(), QString(QLatin1Char('x')));
    px->setFake(true);
    px->setParent(this);

    DoubleProperty *py = new DoubleProperty(value.y(), QString(QLatin1Char('y')));
    py->setFake(true);
    py->setParent(this);

    m_properties << px << py;
}

QVariant PointFProperty::value() const
{
    return QPointF(propertyAt(0)->value().toDouble(),
                  propertyAt(1)->value().toDouble());
}

void PointFProperty::setValue(const QVariant &value)
{
    const QPointF pt = value.toPointF();
    propertyAt(0)->setValue(pt.x());
    propertyAt(1)->setValue(pt.y());
}

// -------------------------------------------------------------------------
PropertyCollection::PropertyCollection(const QString &name)
    : m_name(name)
{
}

PropertyCollection::~PropertyCollection()
{
    qDeleteAll(m_properties);
}

void PropertyCollection::addProperty(IProperty *property)
{
    property->setParent(this);
    m_properties.append(property);
}

void PropertyCollection::removeProperty(IProperty *property)
{
    Q_UNUSED(property);
}

int PropertyCollection::indexOf(IProperty *property) const
{
    return m_properties.indexOf(property);
}

int PropertyCollection::propertyCount() const
{
    return m_properties.size();
}

IProperty *PropertyCollection::propertyAt(int index) const
{
    return m_properties.at(index);
}

QString PropertyCollection::propertyName() const
{
    return m_name;
}

QVariant PropertyCollection::value() const
{
    return QVariant();
}

void PropertyCollection::setValue(const QVariant &value)
{
    Q_UNUSED(value);
}

QString PropertyCollection::toString() const
{
    return QString();
}

bool PropertyCollection::hasEditor() const
{
    return false;
}

QWidget *PropertyCollection::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    Q_UNUSED(parent);
    Q_UNUSED(target);
    Q_UNUSED(receiver);
    return 0;
}

bool PropertyCollection::hasExternalEditor() const
{
    return false;
}

QWidget *PropertyCollection::createExternalEditor(QWidget *parent)
{
    Q_UNUSED(parent);
    return 0;
}

// -------------------------------------------------------------------------

StringProperty::StringProperty(const QString &value, const QString &name,
                               TextPropertyValidationMode validationMode,
                               bool hasComment, const QString &comment)
    : AbstractPropertyGroup(name),
      m_validationMode(validationMode),
      m_value(value)
{
    if (hasComment) {
        StringProperty *pcomment = new StringProperty(comment, QLatin1String("comment"));
        pcomment->setParent(this);
        m_properties << pcomment;
    }
}


QVariant StringProperty::value() const
{
    return m_value;
}

void StringProperty::setValue(const QVariant &value)
{
    m_value = value.toString();
}

QString StringProperty::toString() const
{
    return TextPropertyEditor::stringToEditorString(m_value, m_validationMode);
}

bool StringProperty::hasEditor() const
{
    return true;
}

QWidget *StringProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    TextPropertyEditor* textEditor = new TextPropertyEditor(TextPropertyEditor::EmbeddingTreeView, m_validationMode, parent);

    QObject::connect(textEditor, SIGNAL(textChanged(QString)), target, receiver);
    return textEditor;
}

void StringProperty::updateEditorContents(QWidget *editor)
{
    if (TextPropertyEditor *textEditor = qobject_cast<TextPropertyEditor*>(editor)) {
        if (textEditor->text() != m_value)
            textEditor->setText(m_value);
    }
}

void StringProperty::updateValue(QWidget *editor)
{
    if (const TextPropertyEditor *textEditor = qobject_cast<const TextPropertyEditor*>(editor)) {
        const QString newValue = textEditor->text();

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }

    }
}


// -------------------------------------------------------------------------
ListProperty::ListProperty(const QStringList &items, int value, const QString &name)
    : AbstractProperty<int>(value, name), m_items(items)
{
}

QStringList ListProperty::items() const
{
    return m_items;
}

void ListProperty::setValue(const QVariant &value)
{
    m_value = value.toInt();
}

QString ListProperty::toString() const
{
    if (m_items.isEmpty())
        return QString();
    else if (m_value < 0 || m_value >= m_items.count())
        return m_items.first();

    return m_items.at(m_value);
}

QWidget *ListProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QComboBox *combo = new QComboBox(parent);
    combo->view()->setTextElideMode(Qt::ElideLeft);
    combo->setFrame(0);

    combo->addItems(items());
    QObject::connect(combo, SIGNAL(activated(int)), target, receiver);
    return combo;
}

void ListProperty::updateEditorContents(QWidget *editor)
{
    if (QComboBox *combo = qobject_cast<QComboBox*>(editor)) {
        combo->setCurrentIndex(m_value);
    }
}

void ListProperty::updateValue(QWidget *editor)
{
    if (QComboBox *combo = qobject_cast<QComboBox*>(editor)) {
        const int newValue = combo->currentIndex();

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }
    }
}

// -------------------------------------------------------------------------
SizeProperty::SizeProperty(const QSize &value, const QString &name)
    : AbstractPropertyGroup(name)
{
    IntProperty *pw = new IntProperty(value.width(), QLatin1String("width"));
    pw->setFake(true);
    pw->setParent(this);
    pw->setRange(0, INT_MAX);

    IntProperty *ph = new IntProperty(value.height(), QLatin1String("height"));
    ph->setFake(true);
    ph->setParent(this);
    ph->setRange(0, INT_MAX);

    if (name == QLatin1String("maximumSize")) {
        pw->setRange(0, 0xFFFFFF);
        ph->setRange(0, 0xFFFFFF);
    }
    if (name == QLatin1String("minimumSize")) {
        pw->setRange(0, 0xFFF);
        ph->setRange(0, 0xFFF);
    }

    m_properties << pw << ph;
}

QVariant SizeProperty::value() const
{
    return QSize(propertyAt(0)->value().toInt(),
                 propertyAt(1)->value().toInt());
}

void SizeProperty::setValue(const QVariant &value)
{
    const QSize pt = value.toSize();
    propertyAt(0)->setValue(pt.width());
    propertyAt(1)->setValue(pt.height());
}

// -------------------------------------------------------------------------
SizeFProperty::SizeFProperty(const QSizeF &value, const QString &name)
    : AbstractPropertyGroup(name)
{
    SpinBoxDoubleProperty *pw = new SpinBoxDoubleProperty(value.width(), QLatin1String("width"));
    pw->setFake(true);
    pw->setParent(this);
    pw->setRange(0.0, HUGE_VAL);

    SpinBoxDoubleProperty *ph = new SpinBoxDoubleProperty(value.height(), QLatin1String("height"));
    ph->setFake(true);
    ph->setParent(this);
    ph->setRange(0.0, HUGE_VAL);

    m_properties << pw << ph;
}

QVariant SizeFProperty::value() const
{
    return QSizeF(propertyAt(0)->value().toDouble(),
                 propertyAt(1)->value().toDouble());
}

void SizeFProperty::setValue(const QVariant &value)
{
    const QSizeF pt = value.toSizeF();
    propertyAt(0)->setValue(pt.width());
    propertyAt(1)->setValue(pt.height());
}

// -------------------------------------------------------------------------
// QIntPropertySpinBox also emits editingFinished when the spinbox is used
class QIntPropertySpinBox: public QSpinBox
{
public:
    QIntPropertySpinBox(QWidget *parent = 0)
        : QSpinBox(parent) { }

    void stepBy(int steps)
    {
        QSpinBox::stepBy(steps);
        emit editingFinished();
    }
};

IntProperty::IntProperty(int value, const QString &name)
    : AbstractProperty<int>(value, name), m_low(INT_MIN), m_hi(INT_MAX)
{
}

void IntProperty::setRange(int low, int hi)
{
    m_low = low;
    m_hi = hi;
}

QString IntProperty::specialValue() const
{
    return m_specialValue;
}

void IntProperty::setSpecialValue(const QString &specialValue)
{
    m_specialValue = specialValue;
}

void IntProperty::setValue(const QVariant &value)
{
    m_value = value.toInt();
}

QString IntProperty::toString() const
{
    return QString::number(m_value);
}

QWidget *IntProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QSpinBox *spinBox = new QIntPropertySpinBox(parent);
    spinBox->setFrame(0);
    spinBox->setSpecialValueText(m_specialValue);
    spinBox->setRange(m_low, m_hi);
    spinBox->setValue(m_value);
    spinBox->selectAll();

    QObject::connect(spinBox, SIGNAL(editingFinished()), target, receiver);

    return spinBox;
}

void IntProperty::updateEditorContents(QWidget *editor)
{
    if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(editor)) {
        spinBox->setValue(m_value);
    }
}

void IntProperty::updateValue(QWidget *editor)
{
    if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(editor)) {
        const int newValue = spinBox->value();

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }
    }
}

// -------------------------------------------------------------------------
RectProperty::RectProperty(const QRect &value, const QString &name)
    : AbstractPropertyGroup(name)
{
    IntProperty *px = new IntProperty(value.x(), QString(QLatin1Char('x')));
    px->setFake(true);
    px->setParent(this);

    IntProperty *py = new IntProperty(value.y(), QString(QLatin1Char('y')));
    py->setFake(true);
    py->setParent(this);

    IntProperty *pw = new IntProperty(value.width(), QLatin1String("width"));
    pw->setFake(true);
    pw->setParent(this);
    pw->setRange(0, INT_MAX);

    IntProperty *ph = new IntProperty(value.height(), QLatin1String("height"));
    ph->setFake(true);
    ph->setParent(this);
    ph->setRange(0, INT_MAX);

    if (name == QLatin1String("geometry")) {
        pw->setRange(0, 0xFFF);
        ph->setRange(0, 0xFFF);
    }

    m_properties << px << py << pw << ph;
}

QVariant RectProperty::value() const
{
    return QRect(propertyAt(0)->value().toInt(),
                 propertyAt(1)->value().toInt(),
                 propertyAt(2)->value().toInt(),
                 propertyAt(3)->value().toInt());
}

void RectProperty::setValue(const QVariant &value)
{
    QRect pt = value.toRect();
    propertyAt(0)->setValue(pt.x());
    propertyAt(1)->setValue(pt.y());
    propertyAt(2)->setValue(pt.width());
    propertyAt(3)->setValue(pt.height());
}

// -------------------------------------------------------------------------
RectFProperty::RectFProperty(const QRectF &value, const QString &name)
    : AbstractPropertyGroup(name)
{
    DoubleProperty *px = new DoubleProperty(value.x(), QString(QLatin1Char('x')));
    px->setFake(true);
    px->setParent(this);

    DoubleProperty *py = new DoubleProperty(value.y(), QString(QLatin1Char('y')));
    py->setFake(true);
    py->setParent(this);

    SpinBoxDoubleProperty *pw = new SpinBoxDoubleProperty(value.width(), QLatin1String("width"));
    pw->setFake(true);
    pw->setParent(this);
    pw->setRange(0.0, HUGE_VAL);

    SpinBoxDoubleProperty *ph = new SpinBoxDoubleProperty(value.height(), QLatin1String("height"));
    ph->setFake(true);
    ph->setParent(this);
    ph->setRange(0.0, HUGE_VAL);

    m_properties << px << py << pw << ph;
}

QVariant RectFProperty::value() const
{
    return QRectF(propertyAt(0)->value().toDouble(),
                 propertyAt(1)->value().toDouble(),
                 propertyAt(2)->value().toDouble(),
                 propertyAt(3)->value().toDouble());
}

void RectFProperty::setValue(const QVariant &value)
{
    QRectF pt = value.toRectF();
    propertyAt(0)->setValue(pt.x());
    propertyAt(1)->setValue(pt.y());
    propertyAt(2)->setValue(pt.width());
    propertyAt(3)->setValue(pt.height());
}

// -------------------------------------------------------------------------
ColorProperty::ColorProperty(const QColor &value, const QString &name)
    : AbstractPropertyGroup(name)
{
    IntProperty *r = new IntProperty(value.red(), QLatin1String("red"));
    r->setFake(true);
    r->setRange(0, 255);
    r->setParent(this);

    IntProperty *g = new IntProperty(value.green(), QLatin1String("green"));
    g->setFake(true);
    g->setRange(0, 255);
    g->setParent(this);

    IntProperty *b = new IntProperty(value.blue(), QLatin1String("blue"));
    b->setFake(true);
    b->setRange(0, 255);
    b->setParent(this);

    m_properties << r << g << b;
}

QVariant ColorProperty::value() const
{
    return qVariantFromValue(QColor(propertyAt(0)->value().toInt(),
                  propertyAt(1)->value().toInt(),
                  propertyAt(2)->value().toInt()));
}

void ColorProperty::setValue(const QVariant &value)
{
    QColor c = qvariant_cast<QColor>(value);
    propertyAt(0)->setValue(c.red());
    propertyAt(1)->setValue(c.green());
    propertyAt(2)->setValue(c.blue());
}

QVariant ColorProperty::decoration() const
{
    QPixmap pix(16, 16);
    pix.fill(qvariant_cast<QColor>(value()));
    return qVariantFromValue(pix);
}

QWidget *ColorProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QtColorButton *button = new QtColorButton(parent);
    QObject::connect(button, SIGNAL(colorChanged(const QColor &)), target, receiver);
    return button;
}

void ColorProperty::updateEditorContents(QWidget *editor)
{
    QtColorButton *button = qobject_cast<QtColorButton *>(editor);
    if (!button)
        return;
    button->setColor(qvariant_cast<QColor>(value()));
}

void ColorProperty::updateValue(QWidget *editor)
{
    QtColorButton *button = qobject_cast<QtColorButton *>(editor);
    if (!button)
        return;
    const QVariant color = qVariantFromValue(button->color());
    if (color != value()) {
        setValue(color);
        setChanged(true);
    }
}

// -------------------------------------------------------------------------
FontProperty::FontProperty(const QFont &value, const QString &name, QWidget *selectedWidget)
    : AbstractPropertyGroup(name)
{
    m_selectedWidget = selectedWidget;

    QStringList fonts = fontDatabase()->families();
    int index = fonts.indexOf(value.family());
    if (index == -1)
        index = 0;

    IProperty *i = 0;
    i = new ListProperty(fonts, index, QLatin1String("Family"));
    i->setFake(true);
    i->setHasReset(true);
    i->setParent(this);
    m_properties << i;

    int pointSize = value.pointSize();
    if (pointSize < 1) {
        // try to convert from pixel size and resolved font
        // see also code in FontProperty::setValue
        pointSize = QFontInfo(value).pointSize();
    }
    IntProperty *ii = new IntProperty(pointSize, QLatin1String("Point Size"));
    ii->setFake(true);
    ii->setHasReset(true);
    ii->setRange(1, INT_MAX); // ### check
    ii->setParent(this);
    m_properties << ii;

    i = new BoolProperty(value.bold(), QLatin1String("Bold"));
    i->setFake(true);
    i->setHasReset(true);
    i->setParent(this);
    m_properties << i;

    i = new BoolProperty(value.italic(), QLatin1String("Italic"));
    i->setFake(true);
    i->setHasReset(true);
    i->setParent(this);
    m_properties << i;

    i = new BoolProperty(value.underline(), QLatin1String("Underline"));
    i->setFake(true);
    i->setHasReset(true);
    i->setParent(this);
    m_properties << i;

    i = new BoolProperty(value.strikeOut(), QLatin1String("Strikeout"));
    i->setFake(true);
    i->setHasReset(true);
    i->setParent(this);
    m_properties << i;

    i = new BoolProperty(value.kerning(), QLatin1String("Kerning"));
    i->setFake(true);
    i->setHasReset(true);
    i->setParent(this);
    m_properties << i;

    QStringList keys;
    keys << QString::fromUtf8("QFont::PreferDefault");
    keys << QString::fromUtf8("QFont::NoAntialias");
    keys << QString::fromUtf8("QFont::PreferAntialias");
    QMap<QString, QVariant> map;
    map[QString::fromUtf8("QFont::PreferDefault")] = QVariant(QFont::PreferDefault);
    map[QString::fromUtf8("QFont::NoAntialias")] = QVariant(QFont::NoAntialias);
    map[QString::fromUtf8("QFont::PreferAntialias")] = QVariant(QFont::PreferAntialias);

    MapProperty *pa = new MapProperty(map, value.styleStrategy(), QLatin1String("Antialiasing"), keys);
    pa->setFake(true);
    pa->setHasReset(true);
    pa->setParent(this);
    m_properties << pa;

    m_font = value;
}

QVariant FontProperty::value() const
{
    return m_font;
}

void FontProperty::setValue(const QVariant &value)
{
    m_font = qvariant_cast<QFont>(value);
    QFont parentFont = QFont();
    if (m_selectedWidget) {
        if (m_selectedWidget->isWindow())
            parentFont = QApplication::font(m_selectedWidget);
        else {
            if (m_selectedWidget->parentWidget())
                parentFont = m_selectedWidget->parentWidget()->font();
        }
    }

    const uint mask = m_font.resolve();
    m_font = m_font.resolve(parentFont);
    m_font.resolve(mask);

    m_changed = mask != 0;

    const int family = fontDatabase()->families().indexOf(m_font.family());
    int pointSize = m_font.pointSize();

    if (pointSize < 1) {
        // try to convert from pixel size and resolved font
        // see also code in FontProperty constructor
        pointSize = QFontInfo(m_font).pointSize();
    }

    propertyAt(0)->setValue(family);
    propertyAt(1)->setValue(pointSize);
    propertyAt(2)->setValue(m_font.bold());
    propertyAt(3)->setValue(m_font.italic());
    propertyAt(4)->setValue(m_font.underline());
    propertyAt(5)->setValue(m_font.strikeOut());
    propertyAt(6)->setValue(m_font.kerning());
    propertyAt(7)->setValue(m_font.styleStrategy());
}

QVariant FontProperty::decoration() const
{
    QPixmap pix(16, 16);
    pix.fill(Qt::white);
    QPainter p(&pix);
    QFont fnt = qvariant_cast<QFont>(value());
    fnt.setPointSize(10); // ### always 10pt!!
    p.drawRect(0, 0, 16, 16);
    p.setFont(fnt);
    p.drawText(0, 16 - 2, QLatin1String("Aa")); // ### 2px for the border!!
    return qVariantFromValue(pix);
}

QString FontProperty::toString() const
{
    const QString family = propertyAt(0)->toString();
    const QString pointSize = propertyAt(1)->value().toString();
    QString rc(QLatin1String("  "));  // ### temp hack
    rc += QLatin1Char('[');
    rc += family;
    rc += QLatin1String(", ");
    rc += pointSize;
    rc += QLatin1Char(']');
    return rc;
}

// -------------------------------------------------------------------------
MapProperty::MapProperty(const QMap<QString, QVariant> &items,
                         const QVariant &value,
                         const QString &name,
                         const QStringList &okeys)
    : AbstractProperty<QVariant>(value, name),
      m_items(items),
      m_keys(items.keys()),
      comboKeys(okeys.isEmpty() ? m_keys : okeys)
{
}

QStringList MapProperty::keys() const
{
    return m_keys;
}

QMap<QString, QVariant> MapProperty::items() const
{
    return m_items;
}

QVariant MapProperty::value() const
{
    return m_value;
}

void MapProperty::setValue(const QVariant &value)
{
   if (qVariantCanConvert<EnumType>(value)) {
        const EnumType e = qvariant_cast<EnumType>(value);
        m_value = e.value;
    } else if (qVariantCanConvert<FlagType>(value)) {
        const FlagType e = qvariant_cast<FlagType>(value);
        m_value = e.value;
    } else {
        m_value = value;
    }
}

QString MapProperty::toString() const
{
    return m_items.key(m_value);
}

int MapProperty::indexOf(const QVariant &value) const
{
    const QString key = m_items.key(value);
    return comboKeys.indexOf(key);
}

QWidget *MapProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QComboBox *combo = new QComboBox(parent);
    combo->view()->setTextElideMode(Qt::ElideLeft);
    combo->setFrame(0);

    combo->addItems(comboKeys);
    QObject::connect(combo, SIGNAL(activated(int)), target, receiver);

    return combo;
}

void MapProperty::updateEditorContents(QWidget *editor)
{
    if (QComboBox *combo = qobject_cast<QComboBox*>(editor)) {
        combo->setCurrentIndex(indexOf(m_value));
    }
}

void MapProperty::updateValue(QWidget *editor)
{
    if (QComboBox *combo = qobject_cast<QComboBox*>(editor)) {
        const QString key = combo->currentText();
        const QVariant newValue = m_items.value(key);

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }
    }
}

// -------------------------------------------------------------------------
FlagsProperty::FlagsProperty(const QMap<QString, QVariant> &items, unsigned int value,
                             const QString &name)
    : MapProperty(items, QVariant(value), name)
{
}

QWidget *FlagsProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QList<FlagBoxModelItem> l;
    QMapIterator<QString, QVariant> it(items());
    const unsigned int v = m_value.toUInt();
    int initialIndex = -1;
    int i = 0;
    while (it.hasNext()) {
        it.next();
        unsigned int value = it.value().toUInt();
        bool checked = (value == 0) ? (v == 0) : ((value & v) == value);
        l.append(FlagBoxModelItem(it.key(), value, checked));
        if ((value & v) == value) {
            if (initialIndex == -1)
                initialIndex = i;
            else if (FlagBoxModel::bitcount(value) > FlagBoxModel::bitcount(l.at(initialIndex).value()))
                initialIndex = i;
        }
        ++i;
    }

    FlagBox *editor = new FlagBox(parent);
    editor->setItems(l);
    editor->setCurrentIndex(initialIndex);
    QObject::connect(editor, SIGNAL(activated(int)), target, receiver);
    return editor;
}

void FlagsProperty::updateEditorContents(QWidget *editor)
{
    FlagBox *box = qobject_cast<FlagBox*>(editor);
    if (box == 0)
        return;

    box->view()->reset();
}

void FlagsProperty::updateValue(QWidget *editor)
{
    FlagBox *box = qobject_cast<FlagBox*>(editor);
    if ((box == 0) || (box->currentIndex() < 0))
        return;

    unsigned int newValue = 0;

    FlagBoxModelItem &thisItem = box->item(box->currentIndex());
    if (thisItem.value() == 0) {
        // Uncheck all items except 0-mask
        for (int i=0; i<box->count(); ++i)
            box->item(i).setChecked(i == box->currentIndex());
    } else {
        // Compute new value, without including (additional) supermasks
        if (thisItem.isChecked())
            newValue = thisItem.value();
        for (int i=0; i<box->count(); ++i) {
            FlagBoxModelItem &item = box->item(i);
            if (item.isChecked() && (FlagBoxModel::bitcount(item.value()) == 1))
                newValue |= item.value();
        }
        if (newValue == 0) {
            // Uncheck all items except 0-mask
            for (int i=0; i<box->count(); ++i) {
                FlagBoxModelItem &item = box->item(i);
                item.setChecked(item.value() == 0);
            }
        } else if (newValue == m_value) {
            if (!thisItem.isChecked() && (FlagBoxModel::bitcount(thisItem.value()) > 1)) {
                // We unchecked something, but the original value still holds
                thisItem.setChecked(true);
            }
        } else {
            // Make sure 0-mask is not selected
            for (int i=0; i<box->count(); ++i) {
                FlagBoxModelItem &item = box->item(i);
                if (item.value() == 0)
                        item.setChecked(false);
            }
            // Check/uncheck proper masks
            if (thisItem.isChecked()) {
                // Make sure submasks and supermasks are selected
                for (int i=0; i<box->count(); ++i) {
                    FlagBoxModelItem &item = box->item(i);
                    if ((item.value() != 0) && ((item.value() & newValue) == item.value()) && !item.isChecked())
                        item.setChecked(true);
                }
            } else {
                // Make sure supermasks are not selected if they're no longer valid
                for (int i=0; i<box->count(); ++i) {
                    FlagBoxModelItem &item = box->item(i);
                    if (item.isChecked() && ((item.value() == thisItem.value()) || ((item.value() & newValue) != item.value())))
                        item.setChecked(false);
                }
            }
        }
    }

    if (newValue != m_value) {
        m_value = newValue;
        setChanged(true);
    }
}

// -------------------------------------------------------------------------
SizePolicyProperty::SizePolicyProperty(const QSizePolicy &value, const QString &name)
    : AbstractPropertyGroup(name)
{
    QStringList lst;
    lst << QString::fromUtf8("Fixed")
        << QString::fromUtf8("Minimum")
        << QString::fromUtf8("Maximum")
        << QString::fromUtf8("Preferred")
        << QString::fromUtf8("MinimumExpanding")
        << QString::fromUtf8("Expanding")
        << QString::fromUtf8("Ignored");

    IProperty *i = 0;
    i = new ListProperty(lst, size_type_to_int(value.horizontalPolicy()), QLatin1String("hSizeType"));
    i->setFake(true);
    i->setParent(this);
    m_properties << i;

    i = new ListProperty(lst, size_type_to_int(value.verticalPolicy()), QLatin1String("vSizeType"));
    i->setFake(true);
    i->setParent(this);
    m_properties << i;

    i = new IntProperty(value.horizontalStretch(), QLatin1String("horizontalStretch"));
    i->setFake(true);
    i->setParent(this);
    m_properties << i;

    i = new IntProperty(value.verticalStretch(), QLatin1String("verticalStretch"));
    i->setFake(true);
    i->setParent(this);
    m_properties << i;
}

QVariant SizePolicyProperty::value() const
{
    QSizePolicy sizePolicy;
    sizePolicy.setHorizontalPolicy(int_to_size_type(propertyAt(0)->value().toInt()));
    sizePolicy.setVerticalPolicy(int_to_size_type(propertyAt(1)->value().toInt()));
    sizePolicy.setHorizontalStretch(propertyAt(2)->value().toInt());
    sizePolicy.setVerticalStretch(propertyAt(3)->value().toInt());
    return qVariantFromValue(sizePolicy);
}

void SizePolicyProperty::setValue(const QVariant &value)
{
    QSizePolicy sizePolicy = qvariant_cast<QSizePolicy>(value);

    propertyAt(0)->setValue(size_type_to_int(sizePolicy.horizontalPolicy()));
    propertyAt(1)->setValue(size_type_to_int(sizePolicy.verticalPolicy()));
    propertyAt(2)->setValue(sizePolicy.horizontalStretch());
    propertyAt(3)->setValue(sizePolicy.verticalStretch());
}

QVariant SizePolicyProperty::decoration() const
{
    return QVariant();
}

QString SizePolicyProperty::toString() const
{
    return AbstractPropertyGroup::toString();
}

// -------------------------------------------------------------------------
DateTimeProperty::DateTimeProperty(const QDateTime &value, const QString &name)
    : AbstractProperty<QDateTime>(value, name)
{
}

void DateTimeProperty::setValue(const QVariant &value)
{
    m_value = value.toDateTime();
}

QString DateTimeProperty::toString() const
{
    return m_value.toString();
}

QWidget *DateTimeProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QDateTimeEdit *lineEdit = new QDateTimeEdit(parent);
    QObject::connect(lineEdit, SIGNAL(dateTimeChanged(QDateTime)), target, receiver);
    return lineEdit;
}

void DateTimeProperty::updateEditorContents(QWidget *editor)
{
    if (QDateTimeEdit *lineEdit = qobject_cast<QDateTimeEdit*>(editor)) {
        lineEdit->setDateTime(m_value);
    }
}

void DateTimeProperty::updateValue(QWidget *editor)
{
    if (QDateTimeEdit *lineEdit = qobject_cast<QDateTimeEdit*>(editor)) {
        const QDateTime newValue = lineEdit->dateTime();

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }

    }
}

// -------------------------------------------------------------------------
DateProperty::DateProperty(const QDate &value, const QString &name)
    : AbstractProperty<QDate>(value, name)
{
}

void DateProperty::setValue(const QVariant &value)
{
    m_value = value.toDate();
}

QString DateProperty::toString() const
{
    return m_value.toString();
}

QWidget *DateProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QDateEdit *lineEdit = new QDateEdit(parent);
    QObject::connect(lineEdit, SIGNAL(dateChanged(QDate)), target, receiver);
    return lineEdit;
}

void DateProperty::updateEditorContents(QWidget *editor)
{
    if (QDateEdit *lineEdit = qobject_cast<QDateEdit*>(editor)) {
        lineEdit->setDate(m_value);
    }
}

void DateProperty::updateValue(QWidget *editor)
{
    if (QDateEdit *lineEdit = qobject_cast<QDateEdit*>(editor)) {
        const QDate newValue = lineEdit->date();

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }

    }
}

// -------------------------------------------------------------------------
TimeProperty::TimeProperty(const QTime &value, const QString &name)
    : AbstractProperty<QTime>(value, name)
{
}

void TimeProperty::setValue(const QVariant &value)
{
    m_value = value.toTime();
}

QString TimeProperty::toString() const
{
    return m_value.toString();
}

QWidget *TimeProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QTimeEdit *lineEdit = new QTimeEdit(parent);
    QObject::connect(lineEdit, SIGNAL(timeChanged(QTime)), target, receiver);
    return lineEdit;
}

void TimeProperty::updateEditorContents(QWidget *editor)
{
    if (QTimeEdit *lineEdit = qobject_cast<QTimeEdit*>(editor)) {
        lineEdit->setTime(m_value);
    }
}

void TimeProperty::updateValue(QWidget *editor)
{
    if (QTimeEdit *lineEdit = qobject_cast<QTimeEdit*>(editor)) {
        const QTime newValue = lineEdit->time();

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }

    }
}

// QtKeqSequenceEdit

class QtKeySequenceEdit : public QWidget
{
    Q_OBJECT
public:
    QtKeySequenceEdit(QWidget *parent = 0);

    QKeySequence keySequence() const;
    bool eventFilter(QObject *o, QEvent *e);
public Q_SLOTS:
    void setKeySequence(const QKeySequence &sequence);
Q_SIGNALS:
    void keySequenceChanged(const QKeySequence &sequence);
protected:
    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
    bool event(QEvent *e);
private slots:
    void slotClearShortcut();
private:
    void handleKeyEvent(QKeyEvent *e);
    int translateModifiers(Qt::KeyboardModifiers state) const;

    int m_num;
    QKeySequence m_keySequence;
    QLineEdit *m_lineEdit;
};

bool QtKeySequenceEdit::event(QEvent *e)
{
    if (e->type() == QEvent::Shortcut ||
            e->type() == QEvent::ShortcutOverride  ||
            e->type() == QEvent::KeyRelease) {
        e->accept();
        return true;
    }
    return QWidget::event(e);
}

QtKeySequenceEdit::QtKeySequenceEdit(QWidget *parent)
    : QWidget(parent), m_num(0)
{
    m_lineEdit = new QLineEdit(this);
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_lineEdit);
    layout->setMargin(0);
    m_lineEdit->installEventFilter(this);
    m_lineEdit->setReadOnly(true);
    m_lineEdit->setFocusProxy(this);
    setFocusPolicy(m_lineEdit->focusPolicy());
    setAttribute(Qt::WA_InputMethodEnabled);
}

bool QtKeySequenceEdit::eventFilter(QObject *o, QEvent *e)
{
    if (o == m_lineEdit && e->type() == QEvent::ContextMenu) {
        QContextMenuEvent *c = static_cast<QContextMenuEvent *>(e);
        QMenu *menu = m_lineEdit->createStandardContextMenu();
        QList<QAction *> actions = menu->actions();
        QListIterator<QAction *> itAction(actions);
        while (itAction.hasNext()) {
            QAction *action = itAction.next();
            action->setShortcut(QKeySequence());
            QString actionString = action->text();
            int pos = actionString.lastIndexOf("\t");
            if (pos > 0) {
                actionString = actionString.mid(0, pos);
            }
            action->setText(actionString);
        }
        QAction *actionBefore = 0;
        if (actions.count() > 0)
            actionBefore = actions[0];
        QAction *clearAction = new QAction(tr("Clear Shortcut"), menu);
        menu->insertAction(actionBefore, clearAction);
        menu->insertSeparator(actionBefore);
        clearAction->setEnabled(!m_keySequence.isEmpty());
        connect(clearAction, SIGNAL(triggered()), this, SLOT(slotClearShortcut()));
        menu->exec(c->globalPos());
        delete menu;
        e->accept();
        return true;
    }

    return QWidget::eventFilter(o, e);
}

void QtKeySequenceEdit::slotClearShortcut()
{
    setKeySequence(QKeySequence());
}

void QtKeySequenceEdit::handleKeyEvent(QKeyEvent *e)
{
    int nextKey = e->key();
    if (nextKey == Qt::Key_Control || nextKey == Qt::Key_Shift ||
            nextKey == Qt::Key_Meta || nextKey == Qt::Key_Alt || nextKey == Qt::Key_Super_L)
        return;

    nextKey |= translateModifiers(e->modifiers());
    int k0 = m_keySequence[0];
    int k1 = m_keySequence[1];
    int k2 = m_keySequence[2];
    int k3 = m_keySequence[3];
    switch (m_num) {
        case 0: k0 = nextKey; k1 = 0; k2 = 0; k3 = 0; break;
        case 1: k1 = nextKey; k2 = 0; k3 = 0; break;
        case 2: k2 = nextKey; k3 = 0; break;
        case 3: k3 = nextKey; break;
        default: break;
    }
    ++m_num;
    if (m_num > 3)
        m_num = 0;
    m_keySequence = QKeySequence(k0, k1, k2, k3);
    m_lineEdit->setText(m_keySequence.toString(QKeySequence::NativeText));
    e->accept();
    emit keySequenceChanged(m_keySequence);
}

void QtKeySequenceEdit::setKeySequence(const QKeySequence &sequence)
{
    if (sequence == m_keySequence)
        return;
    m_num = 0;
    m_keySequence = sequence;
    m_lineEdit->setText(m_keySequence.toString(QKeySequence::NativeText));
}

QKeySequence QtKeySequenceEdit::keySequence() const
{
    return m_keySequence;
}

int QtKeySequenceEdit::translateModifiers(Qt::KeyboardModifiers state) const
{
    int result = 0;
    if (state & Qt::ShiftModifier)
        result |= Qt::SHIFT;
    if (state & Qt::ControlModifier)
        result |= Qt::CTRL;
    if (state & Qt::MetaModifier)
        result |= Qt::META;
    if (state & Qt::AltModifier)
        result |= Qt::ALT;
    return result;
}

void QtKeySequenceEdit::focusInEvent(QFocusEvent *e)
{
    m_lineEdit->event(e);
    m_lineEdit->selectAll();
    QWidget::focusInEvent(e);
}

void QtKeySequenceEdit::focusOutEvent(QFocusEvent *e)
{
    m_num = 0;
    m_lineEdit->event(e);
    QWidget::focusOutEvent(e);
}

void QtKeySequenceEdit::keyPressEvent(QKeyEvent *e)
{
    handleKeyEvent(e);
    e->accept();
}

void QtKeySequenceEdit::keyReleaseEvent(QKeyEvent *e)
{
    m_lineEdit->event(e);
}



// -------------------------------------------------------------------------
KeySequenceProperty::KeySequenceProperty(const QKeySequence &value, const QString &name)
    : AbstractProperty<QKeySequence>(value, name)
{
}

void KeySequenceProperty::setValue(const QVariant &value)
{
    m_value = qVariantValue<QKeySequence>(value);
}

QString KeySequenceProperty::toString() const
{
    return m_value.toString(QKeySequence::NativeText);
}

QWidget *KeySequenceProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QtKeySequenceEdit *keyEdit = new QtKeySequenceEdit(parent);
    QObject::connect(keyEdit, SIGNAL(keySequenceChanged(QKeySequence)), target, receiver);
    return keyEdit;
}

void KeySequenceProperty::updateEditorContents(QWidget *editor)
{
    if (QtKeySequenceEdit *keyEdit = qobject_cast<QtKeySequenceEdit*>(editor)) {
        keyEdit->setKeySequence(m_value);
    }
}

void KeySequenceProperty::updateValue(QWidget *editor)
{
    if (QtKeySequenceEdit *keyEdit = qobject_cast<QtKeySequenceEdit*>(editor)) {
        const QKeySequence newValue = keyEdit->keySequence();

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }

    }
}

// -------------------------------------------------------------------------
CursorProperty::CursorProperty(const QCursor &value, const QString &name)
    : AbstractProperty<QCursor>(value, name)
{
}

void CursorProperty::setValue(const QVariant &value)
{
    m_value = qvariant_cast<QCursor>(value);
}

QString CursorProperty::toString() const
{
    return cursorName(m_value.shape());
}

QVariant CursorProperty::decoration() const
{
    return qVariantFromValue(cursorPixmap(m_value.shape()));
}

QWidget *CursorProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QComboBox *combo = new QComboBox(parent);
    combo->view()->setTextElideMode(Qt::ElideLeft);
    combo->setFrame(0);

    addCursor(combo, Qt::ArrowCursor);
    addCursor(combo, Qt::UpArrowCursor);
    addCursor(combo, Qt::CrossCursor);
    addCursor(combo, Qt::WaitCursor);
    addCursor(combo, Qt::IBeamCursor);
    addCursor(combo, Qt::SizeVerCursor);
    addCursor(combo, Qt::SizeHorCursor);
    addCursor(combo, Qt::SizeBDiagCursor);
    addCursor(combo, Qt::SizeFDiagCursor);
    addCursor(combo, Qt::SizeAllCursor);
    addCursor(combo, Qt::BlankCursor);
    addCursor(combo, Qt::SplitVCursor);
    addCursor(combo, Qt::SplitHCursor);
    addCursor(combo, Qt::PointingHandCursor);
    addCursor(combo, Qt::ForbiddenCursor);
    addCursor(combo, Qt::WhatsThisCursor);
    addCursor(combo, Qt::BusyCursor);
    addCursor(combo, Qt::OpenHandCursor);
    addCursor(combo, Qt::ClosedHandCursor);

    QObject::connect(combo, SIGNAL(activated(int)), target, receiver);

    return combo;
}

void CursorProperty::updateEditorContents(QWidget *editor)
{
    if (QComboBox *combo = qobject_cast<QComboBox*>(editor)) {
        combo->setCurrentIndex(m_value.shape());
    }
}

void CursorProperty::updateValue(QWidget *editor)
{
    if (const QComboBox *combo = qobject_cast<const QComboBox*>(editor)) {
        const QCursor newValue(static_cast<Qt::CursorShape>(combo->currentIndex()));

        if (newValue.shape() != m_value.shape()) {
            m_value = newValue;
            setChanged(true);
        }
    }
}

QString CursorProperty::cursorName(int shape)
{
    switch (shape) {
    case Qt::ArrowCursor: return QString::fromUtf8("Arrow");
    case Qt::UpArrowCursor: return QString::fromUtf8("Up-Arrow");
    case Qt::CrossCursor: return QString::fromUtf8("Cross");
    case Qt::WaitCursor: return QString::fromUtf8("Waiting");
    case Qt::IBeamCursor: return QString::fromUtf8("IBeam");
    case Qt::SizeVerCursor: return QString::fromUtf8("Size Vertical");
    case Qt::SizeHorCursor: return QString::fromUtf8("Size Horizontal");
    case Qt::SizeBDiagCursor: return QString::fromUtf8("Size Slash");
    case Qt::SizeFDiagCursor: return QString::fromUtf8("Size Backslash");
    case Qt::SizeAllCursor: return QString::fromUtf8("Size All");
    case Qt::BlankCursor: return QString::fromUtf8("Blank");
    case Qt::SplitVCursor: return QString::fromUtf8("Split Vertical");
    case Qt::SplitHCursor: return QString::fromUtf8("Split Horizontal");
    case Qt::PointingHandCursor: return QString::fromUtf8("Pointing Hand");
    case Qt::ForbiddenCursor: return QString::fromUtf8("Forbidden");
    case Qt::OpenHandCursor: return QString::fromUtf8("Open Hand");
    case Qt::ClosedHandCursor: return QString::fromUtf8("Closed Hand");
    case Qt::WhatsThisCursor: return QString::fromUtf8("Whats This");
    case Qt::BusyCursor: return QString::fromUtf8("Busy");
    default: return QString();
    }
}

QPixmap CursorProperty::cursorPixmap(int shape)
{
    switch (shape) {
    case Qt::ArrowCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/arrow.png"));
    case Qt::UpArrowCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/uparrow.png"));
    case Qt::CrossCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/cross.png"));
    case Qt::WaitCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/wait.png"));
    case Qt::IBeamCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/ibeam.png"));
    case Qt::SizeVerCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/sizev.png"));
    case Qt::SizeHorCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/sizeh.png"));
    case Qt::SizeBDiagCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/sizef.png"));
    case Qt::SizeFDiagCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/sizeb.png"));
    case Qt::SizeAllCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/sizeall.png"));
    case Qt::BlankCursor:
    {
        QBitmap cur = QBitmap(25, 25);
        cur.clear();
        return cur;
    }
    case Qt::SplitVCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/vsplit.png"));
    case Qt::SplitHCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/hsplit.png"));
    case Qt::PointingHandCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/hand.png"));
    case Qt::ForbiddenCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/no.png"));
    case Qt::OpenHandCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/openhand.png"));
    case Qt::ClosedHandCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/closedhand.png"));
    case Qt::WhatsThisCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/whatsthis.png"));
    case Qt::BusyCursor: return QPixmap(QString::fromUtf8(":/trolltech/formeditor/images/cursors/busy.png"));
    default: return QPixmap();
    }
}

void CursorProperty::addCursor(QComboBox *combo, int shape) const
{
    combo->addItem(cursorPixmap(shape), cursorName(shape), shape);
}

// -------------------------------------------------------------------------
AlignmentProperty::AlignmentProperty(const QMap<QString, QVariant> &items, Qt::Alignment value, const QString &name)
    : AbstractPropertyGroup(name)
{
    QStringList horz_keys = QStringList()
                            << matchStringInKeys(QLatin1String("AlignLeft"), items)
                            << matchStringInKeys(QLatin1String("AlignRight"), items)
                            << matchStringInKeys(QLatin1String("AlignHCenter"), items)
                            << matchStringInKeys(QLatin1String("AlignJustify"), items);
                                                 // << "Qt::AlignAbsolute"

    QMap<QString, QVariant> horz_map;
    foreach (QString h, horz_keys) {
        horz_map.insert(h, items.value(h));
    }

    MapProperty *ph = new MapProperty(horz_map, uint(value & Qt::AlignHorizontal_Mask), QLatin1String("horizontal"));
    ph->setFake(true);
    ph->setParent(this);
    m_properties << ph;


    QStringList vert_keys = QStringList()
                            << matchStringInKeys(QLatin1String("AlignTop"), items)
                            << matchStringInKeys(QLatin1String("AlignBottom"), items)
                            << matchStringInKeys(QLatin1String("AlignVCenter"), items);

    QMap<QString, QVariant> vert_map;
    foreach (QString h, vert_keys) {
        vert_map.insert(h, items.value(h));
    }

    MapProperty *pv = new MapProperty(vert_map, int(value & Qt::AlignVertical_Mask), QLatin1String("vertical"));
    pv->setFake(true);
    pv->setParent(this);
    m_properties << pv;
}

QVariant AlignmentProperty::value() const
{
    return uint(propertyAt(0)->value().toUInt() | propertyAt(1)->value().toUInt());
}

void AlignmentProperty::setValue(const QVariant &value)
{
    QVariant v = value;
    if (qVariantCanConvert<FlagType>(value))
        v = qvariant_cast<FlagType>(value).value;
    else if (qVariantCanConvert<EnumType>(value))
        v = qvariant_cast<EnumType>(value).value;
    propertyAt(0)->setValue(v.toUInt() & Qt::AlignHorizontal_Mask);
    propertyAt(1)->setValue(v.toUInt() & Qt::AlignVertical_Mask);
}

// -------------------------------------------------------------------------
DoubleProperty::DoubleProperty(double value, const QString &name)
    : AbstractProperty<double>(value, name)
{
}

void DoubleProperty::setValue(const QVariant &value)
{
    m_value = value.toDouble();
}

QString DoubleProperty::toString() const
{
    return QString::number(m_value);
}

QWidget *DoubleProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QLineEdit *lineEdit = new QLineEdit(parent);
    lineEdit->setFrame(0);
    lineEdit->setValidator(new QDoubleValidator(lineEdit));

    QObject::connect(lineEdit, SIGNAL(textChanged(QString)), target, receiver);
    return lineEdit;
}

void DoubleProperty::updateEditorContents(QWidget *editor)
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor)) {
        double v = lineEdit->text().toDouble();
        if (v != m_value)
            lineEdit->setText(QString::number(m_value));
    }
}

void DoubleProperty::updateValue(QWidget *editor)
{
    if (const QLineEdit *lineEdit = qobject_cast<const QLineEdit*>(editor)) {
        const double newValue = lineEdit->text().toDouble();

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }

    }
}

// -------------------------------------------------------------------------
// QDoublePropertySpinBox also emits editingFinished when the spinbox is used
class QDoublePropertySpinBox: public QDoubleSpinBox
{
public:
    QDoublePropertySpinBox(QWidget *parent = 0)
        : QDoubleSpinBox(parent) { }

    void stepBy(int steps)
    {
        QDoubleSpinBox::stepBy(steps);
        emit editingFinished();
    }
};

SpinBoxDoubleProperty::SpinBoxDoubleProperty(double value, const QString &name)
    : AbstractProperty<double>(value, name), m_low(-HUGE_VAL), m_hi(HUGE_VAL)
{
}

void SpinBoxDoubleProperty::setRange(double low, double hi)
{
    m_low = low;
    m_hi = hi;
}

QString SpinBoxDoubleProperty::specialValue() const
{
    return m_specialValue;
}

void SpinBoxDoubleProperty::setSpecialValue(const QString &specialValue)
{
    m_specialValue = specialValue;
}

void SpinBoxDoubleProperty::setValue(const QVariant &value)
{
    m_value = value.toDouble();
}

QString SpinBoxDoubleProperty::toString() const
{
    return QString::number(m_value);
}

QWidget *SpinBoxDoubleProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QDoubleSpinBox *spinBox = new QDoublePropertySpinBox(parent);
    spinBox->setFrame(0);
    spinBox->setSpecialValueText(m_specialValue);
    spinBox->setDecimals(6);
    spinBox->setRange(m_low, m_hi);
    spinBox->setValue(m_value);
    spinBox->selectAll();

    QObject::connect(spinBox, SIGNAL(editingFinished()), target, receiver);

    return spinBox;
}

void SpinBoxDoubleProperty::updateEditorContents(QWidget *editor)
{
    if (QDoubleSpinBox *spinBox = qobject_cast<QDoubleSpinBox*>(editor)) {
        spinBox->setValue(m_value);
    }
}

void SpinBoxDoubleProperty::updateValue(QWidget *editor)
{
    if (const QDoubleSpinBox *spinBox = qobject_cast<const QDoubleSpinBox*>(editor)) {
        const double newValue = spinBox->value();

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }
    }
}

// -------------------------------------------------------------------------
CharProperty::CharProperty(QChar value, const QString &name)
    : AbstractProperty<QChar>(value, name)
{
}

void CharProperty::setValue(const QVariant &value)
{
    m_value = value.toChar();
}

QString CharProperty::toString() const
{
    return QString(m_value);
}

QWidget *CharProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QLineEdit *lineEdit = new QLineEdit(parent);
    lineEdit->setFrame(0);
    lineEdit->setInputMask(QLatin1String("X; "));
    QObject::connect(lineEdit, SIGNAL(textChanged(QString)), target, receiver);

    return lineEdit;
}

void CharProperty::updateEditorContents(QWidget *editor)
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor)) {
        if (lineEdit->text() != QString(m_value)) {
            lineEdit->setText(QString(m_value));
            lineEdit->setCursorPosition(0);
        }
    }
}

void CharProperty::updateValue(QWidget *editor)
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor)) {
        lineEdit->setCursorPosition(0);
        QChar newValue = QLatin1Char(' ');
        if (lineEdit->text().size() > 0)
            newValue = lineEdit->text().at(0);

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }
    }
}

// -------------------------------------------------------------------------

LongLongProperty::LongLongProperty(qlonglong value, const QString &name)
    : AbstractProperty<qlonglong>(value, name)
{
}

void LongLongProperty::setValue(const QVariant &value)
{
    m_value = value.toLongLong();
}

QString LongLongProperty::toString() const
{
    return QString::number(m_value);
}

QWidget *LongLongProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QLineEdit *lineEdit = new QLineEdit(parent);
    lineEdit->setFrame(0);
    lineEdit->setValidator(new QLongLongValidator(lineEdit));

    QObject::connect(lineEdit, SIGNAL(textChanged(QString)), target, receiver);
    return lineEdit;
}

void LongLongProperty::updateEditorContents(QWidget *editor)
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor)) {
        const qlonglong v = lineEdit->text().toLongLong();
        if (v != m_value)
            lineEdit->setText(QString::number(m_value));
    }
}

void LongLongProperty::updateValue(QWidget *editor)
{
    if (const QLineEdit *lineEdit = qobject_cast<const QLineEdit*>(editor)) {
        const qlonglong newValue = lineEdit->text().toLongLong();

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }

    }
}

// -------------------------------------------------------------------------
SeparatorProperty::SeparatorProperty(const QString &value, const QString &name)
    : StringProperty(value, name)
{
}

QWidget *SeparatorProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    Q_UNUSED(parent);
    Q_UNUSED(target);
    Q_UNUSED(receiver);
    return 0;
}

bool SeparatorProperty::hasEditor() const
{ return false; }

void SeparatorProperty::updateEditorContents(QWidget *editor)
{ Q_UNUSED(editor); }

void SeparatorProperty::updateValue(QWidget *editor)
{ Q_UNUSED(editor); }

// -------------------------------------------------------------------------
UrlProperty::UrlProperty(const QUrl &value, const QString &name)
    : AbstractPropertyGroup(name),
      m_value(value)
{
}

QVariant UrlProperty::value() const
{
    return m_value;
}

void UrlProperty::setValue(const QVariant &value)
{
    m_value = value.toUrl();
}

QString UrlProperty::toString() const
{
    return m_value.toString();
}

QWidget *UrlProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QLineEdit *lineEdit = new QLineEdit(parent);
    lineEdit->setFrame(0);

    QObject::connect(lineEdit, SIGNAL(textChanged(QString)), target, receiver);
    return lineEdit;
}

void UrlProperty::updateEditorContents(QWidget *editor)
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor)) {
        if (QUrl(lineEdit->text()) != m_value)
            lineEdit->setText(m_value.toString());
    }
}

void UrlProperty::updateValue(QWidget *editor)
{
    if (const QLineEdit *lineEdit = qobject_cast<const QLineEdit*>(editor)) {
        const QUrl newValue = QUrl(lineEdit->text());

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }

    }
}

// -------------------------------------------------------------------------
StringListProperty::StringListProperty(const QStringList &value, const QString &name)
    : AbstractProperty<QStringList>(value, name)
{
}

void StringListProperty::setValue(const QVariant &value)
{
    m_value = qvariant_cast<QStringList>(value);
}

QString StringListProperty::toString() const
{
    return m_value.join(QLatin1String(", "));
}

QWidget *StringListProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    StringListEditorButton *btn = new StringListEditorButton(m_value, parent);
    QObject::connect(btn, SIGNAL(changed()), target, receiver);
    return btn;
}

void StringListProperty::updateEditorContents(QWidget *editor)
{
    if (StringListEditorButton *btn = qobject_cast<StringListEditorButton*>(editor)) {
        btn->setStringList(m_value);
    }
}

void StringListProperty::updateValue(QWidget *editor)
{
    if (const StringListEditorButton *btn = qobject_cast<const StringListEditorButton*>(editor)) {
        const QStringList newValue = btn->stringList();
        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }
    }
}

// -------------------------------------------------------------------------
UIntProperty::UIntProperty(uint value, const QString &name)
    : AbstractProperty<uint>(value, name)
{
}

void UIntProperty::setValue(const QVariant &value)
{
    m_value = value.toUInt();
}

QString UIntProperty::toString() const
{
    return QString::number(m_value);
}

QWidget *UIntProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QLineEdit *lineEdit = new QLineEdit(parent);
    lineEdit->setFrame(0);
    lineEdit->setValidator(new QULongLongValidator(0, UINT_MAX, lineEdit));
    QObject::connect(lineEdit, SIGNAL(textChanged(QString)), target, receiver);

    return lineEdit;
}

void UIntProperty::updateEditorContents(QWidget *editor)
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor)) {
        const uint v = lineEdit->text().toUInt();
        if (v != m_value)
            lineEdit->setText(QString::number(m_value));
    }
}

void UIntProperty::updateValue(QWidget *editor)
{
    if (const QLineEdit *lineEdit = qobject_cast<const QLineEdit*>(editor)) {
        const uint newValue = lineEdit->text().toUInt();

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }

    }
}

// -------------------------------------------------------------------------
ULongLongProperty::ULongLongProperty(qulonglong value, const QString &name)
    : AbstractProperty<qulonglong>(value, name)
{
}

void ULongLongProperty::setValue(const QVariant &value)
{
    m_value = value.toULongLong();
}

QString ULongLongProperty::toString() const
{
    return QString::number(m_value);
}

QWidget *ULongLongProperty::createEditor(QWidget *parent, const QObject *target, const char *receiver) const
{
    QLineEdit *lineEdit = new QLineEdit(parent);
    lineEdit->setFrame(0);
    lineEdit->setValidator(new QULongLongValidator(lineEdit));
    QObject::connect(lineEdit, SIGNAL(textChanged(QString)), target, receiver);

    return lineEdit;
}

void ULongLongProperty::updateEditorContents(QWidget *editor)
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor)) {
        const qulonglong v = lineEdit->text().toULongLong();
        if (v != m_value)
            lineEdit->setText(QString::number(m_value));
    }
}

void ULongLongProperty::updateValue(QWidget *editor)
{
    if (const QLineEdit *lineEdit = qobject_cast<const QLineEdit*>(editor)) {
        const qulonglong newValue = lineEdit->text().toULongLong();

        if (newValue != m_value) {
            m_value = newValue;
            setChanged(true);
        }

    }
}

#include "qpropertyeditor_items.moc"
