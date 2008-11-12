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

#include "properties_p.h"
#include "ui4_p.h"
#include "abstractformbuilder.h"
#include "formbuilderextra_p.h"

#include <QtCore/QDateTime>
#include <QtCore/QUrl>
#include <QtCore/qdebug.h>

#include <QtGui/QIcon>
#include <QtGui/QPixmap>
#include <QtGui/QFont>
#include <QtGui/QFrame>
#include <QtGui/QAbstractScrollArea>

#include <private/qfont_p.h>

static bool toBool(const QString &str)
{
    return str.toLower() == QLatin1String("true");
}

#ifdef QFORMINTERNAL_NAMESPACE
namespace QFormInternal
{
#endif

// Convert complex DOM types with the help of  QAbstractFormBuilder
QVariant domPropertyToVariant(QAbstractFormBuilder *afb,const QMetaObject *meta,const  DomProperty *p)
{
    // Complex types that need functions from QAbstractFormBuilder
    switch(p->kind()) {
    case DomProperty::String: {
        const int index = meta->indexOfProperty(p->attributeName().toUtf8());
        if (index != -1 && meta->property(index).type() == QVariant::KeySequence)
            return qVariantFromValue(QKeySequence(p->elementString()->text()));
    }
        break;

    case DomProperty::Pixmap: {
        const DomResourcePixmap * dpx = afb->domPixmap(p);
        return qVariantFromValue(dpx ? afb->domPropertyToPixmap(p) : QPixmap());
    }

    case DomProperty::IconSet: {
        const DomResourcePixmap * dpx = afb->domPixmap(p);
        return qVariantFromValue(dpx ? afb->domPropertyToIcon(p) : QIcon());
    }

    case DomProperty::Palette: {
        const DomPalette *dom = p->elementPalette();
        QPalette palette;

        if (dom->elementActive())
            afb->setupColorGroup(palette, QPalette::Active, dom->elementActive());

        if (dom->elementInactive())
            afb->setupColorGroup(palette, QPalette::Inactive, dom->elementInactive());

        if (dom->elementDisabled())
            afb->setupColorGroup(palette, QPalette::Disabled, dom->elementDisabled());

        palette.setCurrentColorGroup(QPalette::Active);
        return qVariantFromValue(palette);
    }

    case DomProperty::Set: {
        const QByteArray pname = p->attributeName().toUtf8();
        const int index = meta->indexOfProperty(pname);
        if (index == -1) {
            uiLibWarning(QObject::tr("The set-type property %1 could not be read.").arg(p->attributeName()));
            return QVariant();
        }

        const QMetaEnum e = meta->property(index).enumerator();
        Q_ASSERT(e.isFlag() == true);
        return QVariant(e.keysToValue(p->elementSet().toUtf8()));
    }

    case DomProperty::Enum: {
        const QByteArray pname = p->attributeName().toUtf8();
        const int index = meta->indexOfProperty(pname);
        if (index == -1) {
            // ### special-casing for Line (QFrame) -- fix for 4.2
            if (!qstrcmp(meta->className(), "QFrame")
                && (pname == QByteArray("orientation"))) {
                return QVariant((p->elementEnum() == QLatin1String("Qt::Horizontal")) ? QFrame::HLine : QFrame::VLine);
            } else {
                uiLibWarning(QObject::tr("The enumeration-type property %1 could not be read.").arg(p->attributeName()));
                return QVariant();
            }
        }

        const QMetaEnum e = meta->property(index).enumerator();
        return QVariant(e.keyToValue(p->elementEnum().toUtf8()));
    }
    default:
        break;
    }

    // simple type
    return domPropertyToVariant(p);
}

// Convert simple DOM types
QVariant domPropertyToVariant(const DomProperty *p)
{
    // requires non-const virtual nameToIcon, etc.
    switch(p->kind()) {
    case DomProperty::Bool:
        return QVariant(toBool(p->elementBool()));

    case DomProperty::Cstring:
        return QVariant(p->elementCstring());

    case DomProperty::Point: {
        const DomPoint *point = p->elementPoint();
        return QVariant(QPoint(point->elementX(), point->elementY()));
    }

    case DomProperty::PointF: {
        const DomPointF *pointf = p->elementPointF();
        return QVariant(QPointF(pointf->elementX(), pointf->elementY()));
    }

    case DomProperty::Size: {
        const DomSize *size = p->elementSize();
        return QVariant(QSize(size->elementWidth(), size->elementHeight()));
    }

    case DomProperty::SizeF: {
        const DomSizeF *sizef = p->elementSizeF();
        return QVariant(QSizeF(sizef->elementWidth(), sizef->elementHeight()));
    }

    case DomProperty::Rect: {
        const DomRect *rc = p->elementRect();
        const QRect g(rc->elementX(), rc->elementY(), rc->elementWidth(), rc->elementHeight());
        return QVariant(g);
    }

    case DomProperty::RectF: {
        const DomRectF *rcf = p->elementRectF();
        const QRectF g(rcf->elementX(), rcf->elementY(), rcf->elementWidth(), rcf->elementHeight());
        return QVariant(g);
    }

    case DomProperty::String:
        return QVariant(p->elementString()->text());

    case DomProperty::Number:
        return QVariant(p->elementNumber());

    case DomProperty::UInt:
        return QVariant(p->elementUInt());

    case DomProperty::LongLong:
        return QVariant(p->elementLongLong());

    case DomProperty::ULongLong:
        return QVariant(p->elementULongLong());

    case DomProperty::Double:
        return QVariant(p->elementDouble());

    case DomProperty::Char: {
        const DomChar *character = p->elementChar();
        const QChar c(character->elementUnicode());
        return qVariantFromValue(c);
    }

    case DomProperty::Color: {
        const DomColor *color = p->elementColor();
        const QColor c(color->elementRed(), color->elementGreen(), color->elementBlue());
        return qVariantFromValue(c);
    }

    case DomProperty::Font: {
        const DomFont *font = p->elementFont();

        QFont f;
        if (font->hasElementFamily() && !font->elementFamily().isEmpty())
            f.setFamily(font->elementFamily());
        if (font->hasElementPointSize() && font->elementPointSize() > 0)
            f.setPointSize(font->elementPointSize());
        if (font->hasElementWeight() && font->elementWeight() > 0)
            f.setWeight(font->elementWeight());
        if (font->hasElementItalic())
            f.setItalic(font->elementItalic());
        if (font->hasElementBold())
            f.setBold(font->elementBold());
        if (font->hasElementUnderline())
            f.setUnderline(font->elementUnderline());
        if (font->hasElementStrikeOut())
            f.setStrikeOut(font->elementStrikeOut());
        if (font->hasElementKerning())
            f.setKerning(font->elementKerning());
        if (font->hasElementAntialiasing())
            f.setStyleStrategy(font->elementAntialiasing() ? QFont::PreferDefault : QFont::NoAntialias);
        if (font->hasElementStyleStrategy()) {
            f.setStyleStrategy(enumKeyOfObjectToValue<QAbstractFormBuilderGadget, QFont::StyleStrategy>("styleStrategy", font->elementStyleStrategy().toLatin1()));
        }
        return qVariantFromValue(f);
    }

    case DomProperty::Date: {
        const DomDate *date = p->elementDate();
        return QVariant(QDate(date->elementYear(), date->elementMonth(), date->elementDay()));
    }

    case DomProperty::Time: {
        const DomTime *t = p->elementTime();
        return QVariant(QTime(t->elementHour(), t->elementMinute(), t->elementSecond()));
    }

    case DomProperty::DateTime: {
        const DomDateTime *dateTime = p->elementDateTime();
        const QDate d(dateTime->elementYear(), dateTime->elementMonth(), dateTime->elementDay());
        const QTime tm(dateTime->elementHour(), dateTime->elementMinute(), dateTime->elementSecond());
        return QVariant(QDateTime(d, tm));
    }

    case DomProperty::Url: {
        const DomUrl *url = p->elementUrl();
        return QVariant(QUrl(url->elementString()->text()));
    }

    case DomProperty::Cursor:
        return qVariantFromValue(QCursor(static_cast<Qt::CursorShape>(p->elementCursor())));

    case DomProperty::CursorShape:
        return qVariantFromValue(QCursor(enumKeyOfObjectToValue<QAbstractFormBuilderGadget, Qt::CursorShape>("cursorShape", p->elementCursorShape().toLatin1())));

    case DomProperty::Locale: {
        const DomLocale *locale = p->elementLocale();
        return qVariantFromValue(QLocale(enumKeyOfObjectToValue<QAbstractFormBuilderGadget, QLocale::Language>("language", locale->attributeLanguage().toLatin1()),
                    enumKeyOfObjectToValue<QAbstractFormBuilderGadget, QLocale::Country>("country", locale->attributeCountry().toLatin1())));
    }
    case DomProperty::SizePolicy: {
        const DomSizePolicy *sizep = p->elementSizePolicy();

        QSizePolicy sizePolicy;
        sizePolicy.setHorizontalStretch(sizep->elementHorStretch());
        sizePolicy.setVerticalStretch(sizep->elementVerStretch());

        const QMetaEnum sizeType_enum = metaEnum<QAbstractFormBuilderGadget>("sizeType");

        if (sizep->hasElementHSizeType()) {
            sizePolicy.setHorizontalPolicy((QSizePolicy::Policy) sizep->elementHSizeType());
        } else if (sizep->hasAttributeHSizeType()) {
            const QSizePolicy::Policy sp = enumKeyToValue<QSizePolicy::Policy>(sizeType_enum, sizep->attributeHSizeType().toLatin1());
            sizePolicy.setHorizontalPolicy(sp);
        }

        if (sizep->hasElementVSizeType()) {
            sizePolicy.setVerticalPolicy((QSizePolicy::Policy) sizep->elementVSizeType());
        } else if (sizep->hasAttributeVSizeType()) {
            const  QSizePolicy::Policy sp = enumKeyToValue<QSizePolicy::Policy>(sizeType_enum, sizep->attributeVSizeType().toLatin1());
            sizePolicy.setVerticalPolicy(sp);
        }

        return qVariantFromValue(sizePolicy);
    }

    case DomProperty::StringList:
        return QVariant(p->elementStringList()->elementString());

    default:
        uiLibWarning(QObject::tr("Reading properties of the type %1 is not supported yet.").arg(p->kind()));
        break;
    }

    return QVariant();
}

// Apply a simple variant type to a DOM property
static bool applySimpleProperty(const QVariant &v, bool translateString, DomProperty *dom_prop)
{
    switch (v.type()) {
    case QVariant::String: {
        DomString *str = new DomString();
        str->setText(v.toString());
        if (!translateString)
            str->setAttributeNotr(QLatin1String("true"));
        dom_prop->setElementString(str);
    }
        return true;

    case QVariant::ByteArray:
        dom_prop->setElementCstring(QString::fromUtf8(v.toByteArray()));
        return true;

    case QVariant::Int:
        dom_prop->setElementNumber(v.toInt());
        return true;

    case QVariant::UInt:
        dom_prop->setElementUInt(v.toUInt());
        return true;

    case QVariant::LongLong:
        dom_prop->setElementLongLong(v.toLongLong());
        return true;

    case QVariant::ULongLong:
        dom_prop->setElementULongLong(v.toULongLong());
        return true;

    case QVariant::Double:
        dom_prop->setElementDouble(v.toDouble());
        return true;

    case QVariant::Bool:
        dom_prop->setElementBool(v.toBool() ? QLatin1String("true") : QLatin1String("false"));
        return true;

    case QVariant::Char: {
        DomChar *ch = new DomChar();
        const QChar character = v.toChar();
        ch->setElementUnicode(character.unicode());
        dom_prop->setElementChar(ch);
    }
        return true;

    case QVariant::Point: {
        DomPoint *pt = new DomPoint();
        const QPoint point = v.toPoint();
        pt->setElementX(point.x());
        pt->setElementY(point.y());
        dom_prop->setElementPoint(pt);
    }
        return true;

    case QVariant::PointF: {
        DomPointF *ptf = new DomPointF();
        const QPointF pointf = v.toPointF();
        ptf->setElementX(pointf.x());
        ptf->setElementY(pointf.y());
        dom_prop->setElementPointF(ptf);
    }
        return true;

    case QVariant::Color: {
        DomColor *clr = new DomColor();
        const QColor color = qvariant_cast<QColor>(v);
        clr->setElementRed(color.red());
        clr->setElementGreen(color.green());
        clr->setElementBlue(color.blue());
        dom_prop->setElementColor(clr);
    }
        return true;

    case QVariant::Size: {
        DomSize *sz = new DomSize();
        const QSize size = v.toSize();
        sz->setElementWidth(size.width());
        sz->setElementHeight(size.height());
        dom_prop->setElementSize(sz);
    }
        return true;

    case QVariant::SizeF: {
        DomSizeF *szf = new DomSizeF();
        const QSizeF sizef = v.toSizeF();
        szf->setElementWidth(sizef.width());
        szf->setElementHeight(sizef.height());
        dom_prop->setElementSizeF(szf);
    }
        return true;

    case QVariant::Rect: {
        DomRect *rc = new DomRect();
        const QRect rect = v.toRect();
        rc->setElementX(rect.x());
        rc->setElementY(rect.y());
        rc->setElementWidth(rect.width());
        rc->setElementHeight(rect.height());
        dom_prop->setElementRect(rc);
    }
        return true;

    case QVariant::RectF: {
        DomRectF *rcf = new DomRectF();
        const QRectF rectf = v.toRectF();
        rcf->setElementX(rectf.x());
        rcf->setElementY(rectf.y());
        rcf->setElementWidth(rectf.width());
        rcf->setElementHeight(rectf.height());
        dom_prop->setElementRectF(rcf);
    }
        return true;

    case QVariant::Font: {
        DomFont *fnt = new DomFont();
        const QFont font = qvariant_cast<QFont>(v);
        const uint mask = font.resolve();
        if (mask & QFontPrivate::Weight) {
            fnt->setElementBold(font.bold());
            fnt->setElementWeight(font.weight());
        }
        if (mask & QFontPrivate::Family)
            fnt->setElementFamily(font.family());
        if (mask & QFontPrivate::Style)
            fnt->setElementItalic(font.italic());
        if (mask & QFontPrivate::Size)
            fnt->setElementPointSize(font.pointSize());
        if (mask & QFontPrivate::StrikeOut)
            fnt->setElementStrikeOut(font.strikeOut());
        if (mask & QFontPrivate::Underline)
            fnt->setElementUnderline(font.underline());
        if (mask & QFontPrivate::Kerning)
            fnt->setElementKerning(font.kerning());
        if (mask & QFontPrivate::StyleStrategy) {
            const QMetaEnum styleStrategy_enum = metaEnum<QAbstractFormBuilderGadget>("styleStrategy");
            fnt->setElementStyleStrategy(QLatin1String(styleStrategy_enum.valueToKey(font.styleStrategy())));
        }
        dom_prop->setElementFont(fnt);
    }
        return true;

    case QVariant::Cursor: {
        const QMetaEnum cursorShape_enum = metaEnum<QAbstractFormBuilderGadget>("cursorShape");
        dom_prop->setElementCursorShape(QLatin1String(cursorShape_enum.valueToKey(qvariant_cast<QCursor>(v).shape())));
        }
        return true;

    case QVariant::KeySequence: {
        DomString *s = new DomString();
        s->setText(qvariant_cast<QKeySequence>(v).toString(QKeySequence::PortableText));
        dom_prop->setElementString(s);
        }
        return true;

    case QVariant::Locale: {
        DomLocale *dom = new DomLocale();
        const QLocale locale = qvariant_cast<QLocale>(v);

        const QMetaEnum language_enum = metaEnum<QAbstractFormBuilderGadget>("language");
        const QMetaEnum country_enum = metaEnum<QAbstractFormBuilderGadget>("country");

        dom->setAttributeLanguage(QLatin1String(language_enum.valueToKey(locale.language())));
        dom->setAttributeCountry(QLatin1String(country_enum.valueToKey(locale.country())));

        dom_prop->setElementLocale(dom);
        }
        return true;

    case QVariant::SizePolicy: {
        DomSizePolicy *dom = new DomSizePolicy();
        const QSizePolicy sizePolicy = qvariant_cast<QSizePolicy>(v);

        dom->setElementHorStretch(sizePolicy.horizontalStretch());
        dom->setElementVerStretch(sizePolicy.verticalStretch());

        const QMetaEnum sizeType_enum = metaEnum<QAbstractFormBuilderGadget>("sizeType");

        dom->setAttributeHSizeType(QLatin1String(sizeType_enum.valueToKey(sizePolicy.horizontalPolicy())));
        dom->setAttributeVSizeType(QLatin1String(sizeType_enum.valueToKey(sizePolicy.verticalPolicy())));

        dom_prop->setElementSizePolicy(dom);
    }
        return true;

    case QVariant::Date: {
        DomDate *dom = new DomDate();
        const QDate date = qvariant_cast<QDate>(v);

        dom->setElementYear(date.year());
        dom->setElementMonth(date.month());
        dom->setElementDay(date.day());

        dom_prop->setElementDate(dom);
        }
        return true;

    case QVariant::Time: {
        DomTime *dom = new DomTime();
        const QTime time = qvariant_cast<QTime>(v);

        dom->setElementHour(time.hour());
        dom->setElementMinute(time.minute());
        dom->setElementSecond(time.second());

        dom_prop->setElementTime(dom);
        }
        return true;

    case QVariant::DateTime: {
        DomDateTime *dom = new DomDateTime();
        const QDateTime dateTime = qvariant_cast<QDateTime>(v);

        dom->setElementHour(dateTime.time().hour());
        dom->setElementMinute(dateTime.time().minute());
        dom->setElementSecond(dateTime.time().second());
        dom->setElementYear(dateTime.date().year());
        dom->setElementMonth(dateTime.date().month());
        dom->setElementDay(dateTime.date().day());

        dom_prop->setElementDateTime(dom);
    }
        return true;

    case QVariant::Url: {
        DomUrl *dom = new DomUrl();
        const QUrl url = v.toUrl();

        DomString *str = new DomString();
        str->setText(url.toString());
        dom->setElementString(str);

        dom_prop->setElementUrl(dom);
    }
        return true;

    case QVariant::StringList: {
        DomStringList *sl = new DomStringList;
        sl->setElementString(qvariant_cast<QStringList>(v));
        dom_prop->setElementStringList(sl);
    }
        return true;

    default:
        break;
    }

    return false;
}
static QString msgCannotWriteProperty(const QString &pname, const QVariant &v)
{
    return QObject::tr("The property %1 could not be written. The type %2 is not supported yet.").
                       arg(pname).arg(QVariant::typeToName (v.type()));

}
// Convert simple variant types to DOM properties
DomProperty *variantToDomProperty(const QString &pname, const QVariant &v, bool translateString)
{
    DomProperty *dom_prop = new DomProperty();
    dom_prop->setAttributeName(pname);
    if (applySimpleProperty(v, translateString, dom_prop))
        return dom_prop;

    delete dom_prop;
    uiLibWarning(msgCannotWriteProperty(pname, v));
    return 0;
}

// Convert complex variant types to DOM properties with the help of  QAbstractFormBuilder
// Does not perform a check using  QAbstractFormBuilder::checkProperty().
DomProperty *variantToDomProperty(QAbstractFormBuilder *afb, QObject *obj,
                                  const QString &pname, const QVariant &v)
{
    DomProperty *dom_prop = new DomProperty();
    dom_prop->setAttributeName(pname);

    const QMetaObject *meta = obj->metaObject();
    const int pindex = meta->indexOfProperty(pname.toLatin1());
    if (pindex != -1) {
        QMetaProperty meta_property = meta->property(pindex);
        if (!meta_property.hasStdCppSet() || (qobject_cast<QAbstractScrollArea *>(obj) && pname == QLatin1String("cursor")))
            dom_prop->setAttributeStdset(0);
    }

    // Try simple properties
    const bool translateString = pname != QLatin1String("objectName");
    if (applySimpleProperty(v, translateString, dom_prop))
        return dom_prop;

    // Complex properties
    switch (v.type()) {
    case QVariant::Palette: {
        DomPalette *dom = new DomPalette();
        QPalette palette = qvariant_cast<QPalette>(v);

        palette.setCurrentColorGroup(QPalette::Active);
        dom->setElementActive(afb->saveColorGroup(palette));

        palette.setCurrentColorGroup(QPalette::Inactive);
        dom->setElementInactive(afb->saveColorGroup(palette));

        palette.setCurrentColorGroup(QPalette::Disabled);
        dom->setElementDisabled(afb->saveColorGroup(palette));

        dom_prop->setElementPalette(dom);
    } break;

    case QVariant::Pixmap:
        afb->setPixmapProperty(*dom_prop, afb->pixmapPaths(qvariant_cast<QPixmap>(v)));
        dom_prop->setAttributeName(pname);
        break;

    case QVariant::Icon:
        afb->setIconProperty(*dom_prop, afb->iconPaths(qvariant_cast<QIcon>(v)));
        dom_prop->setAttributeName(pname);
        break;

    default:
        delete dom_prop;
        uiLibWarning(msgCannotWriteProperty(pname, v));
        return 0;
    }
    return dom_prop;
}

#ifdef QFORMINTERNAL_NAMESPACE
}
#endif
