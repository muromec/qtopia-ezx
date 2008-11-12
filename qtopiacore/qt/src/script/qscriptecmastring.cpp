/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtScript module of the Qt Toolkit.
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

#include "qscriptecmastring_p.h"

#ifndef QT_NO_SCRIPT

#include "qscriptengine_p.h"
#include "qscriptvalueimpl_p.h"
#include "qscriptcontext_p.h"
#include "qscriptmember_p.h"
#include "qscriptobject_p.h"

#include <QtCore/QStringList>
#include <QtCore/QtDebug>
#include <QtCore/qnumeric.h>

#include <limits.h>

namespace QScript { namespace Ecma {

String::StringClassData::StringClassData(QScriptClassInfo *classInfo):
    m_classInfo(classInfo)
{
}

String::StringClassData::~StringClassData()
{
}

bool String::StringClassData::resolve(const QScriptValueImpl &object,
                                      QScriptNameIdImpl *nameId,
                                      QScript::Member *member,
                                      QScriptValueImpl *base)
{
    if (object.classInfo() != classInfo())
        return false;

    QScriptEnginePrivate *eng = QScriptEnginePrivate::get(object.engine());

    if (nameId == eng->idTable()->id_length) {
        member->native(nameId, /*id=*/ 0,
                       QScriptValue::Undeletable
                       | QScriptValue::ReadOnly);
        *base = object;
        return true;
    }

    bool ok = false;
    int index = nameId->s.toInt(&ok);

    if (ok)
        member->native(nameId, index, QScriptValue::Undeletable);

    return ok;
}

bool String::StringClassData::get(const QScriptValueImpl &object,
                                  const QScript::Member &member,
                                  QScriptValueImpl *result)
{
    Q_ASSERT(member.isValid());

    if (object.classInfo() != classInfo())
        return false;

    QScriptEnginePrivate *eng = QScriptEnginePrivate::get(object.engine());
    if (! member.isNativeProperty())
        return false;

    QScriptNameIdImpl *ref = object.internalValue().stringValue();
    int len = ref->s.length();

    if (member.nameId() == eng->idTable()->id_length)
        eng->newNumber(result, len);

    else if (member.id() >= 0 && member.id() < len)
        eng->newString(result, ref->s.at(member.id()));

    else
        eng->newUndefined(result);

    return true;
}

int String::StringClassData::extraMemberCount(const QScriptValueImpl &object)
{
    if (object.classInfo() != classInfo())
        return 0;

    QScriptNameIdImpl *ref = object.internalValue().stringValue();
    return ref->s.length();
}

bool String::StringClassData::extraMember(const QScriptValueImpl &object,
                                          int index, Member *member)
{
    if (object.classInfo() != classInfo())
        return false;

    member->native(/*nameId=*/ 0, index, QScriptValue::Undeletable);
    return true;
}

String::String(QScriptEnginePrivate *eng):
    Core(eng)
{
    m_classInfo = eng->registerClass(QLatin1String("String"));
    QExplicitlySharedDataPointer<QScriptClassData> data(new StringClassData(m_classInfo));
    m_classInfo->setData(data);

    publicPrototype.invalidate();
    newString(&publicPrototype, QString());

    eng->newConstructor(&ctor, this, publicPrototype);

    const QScriptValue::PropertyFlags flags = QScriptValue::SkipInEnumeration;
    publicPrototype.setProperty(QLatin1String("toString"),
                                eng->createFunction(method_toString, 0, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("valueOf"),
                                eng->createFunction(method_valueOf, 0, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("charAt"),
                                eng->createFunction(method_charAt, 1, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("charCodeAt"),
                                eng->createFunction(method_charCodeAt, 1, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("concat"),
                                eng->createFunction(method_concat, 0, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("indexOf"),
                                eng->createFunction(method_indexOf, 1, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("lastIndexOf"),
                                eng->createFunction(method_lastIndexOf, 1, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("localeCompare"),
                                eng->createFunction(method_localeCompare, 1, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("match"),
                                eng->createFunction(method_match, 1, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("replace"),
                                eng->createFunction(method_replace, 2, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("search"),
                                eng->createFunction(method_search, 1, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("slice"),
                                eng->createFunction(method_slice, 0, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("split"),
                                eng->createFunction(method_split, 2, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("substring"),
                                eng->createFunction(method_substring, 2, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("toLowerCase"),
                                eng->createFunction(method_toLowerCase, 0, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("toLocaleLowerCase"),
                                eng->createFunction(method_toLocaleLowerCase, 0, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("toUpperCase"),
                                eng->createFunction(method_toUpperCase, 0, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("toLocaleUpperCase"),
                                eng->createFunction(method_toLocaleUpperCase, 0, m_classInfo), flags);

    ctor.setProperty(QLatin1String("fromCharCode"),
                     eng->createFunction(method_fromCharCode, 1, m_classInfo), flags);
}

String::~String()
{
}

void String::execute(QScriptContextPrivate *context)
{
    QString value;

    if (context->argumentCount() > 0)
        value = context->argument(0).toString();

    QScriptValueImpl str(engine(), value);
    if (!context->isCalledAsConstructor()) {
        context->setReturnValue(str);
    } else {
        QScriptValueImpl &obj = context->m_thisObject;
        obj.setClassInfo(classInfo());
        obj.setInternalValue(str);
        obj.setPrototype(publicPrototype);
        context->setReturnValue(obj);
    }
}

void String::newString(QScriptValueImpl *result, const QString &value)
{
    engine()->newObject(result, publicPrototype, classInfo());
    result->setInternalValue(QScriptValueImpl(engine(), value));
}

QScriptValueImpl String::method_toString(QScriptContextPrivate *context, QScriptEnginePrivate *, QScriptClassInfo *classInfo)
{
    QScriptValueImpl self = context->thisObject();
    if (self.classInfo() != classInfo)
        return context->throwError(QScriptContext::TypeError, QLatin1String("String.prototype.toString"));

    return (self.internalValue());
}

QScriptValueImpl String::method_valueOf(QScriptContextPrivate *context, QScriptEnginePrivate *, QScriptClassInfo *classInfo)
{
    QScriptValueImpl self = context->thisObject();
    if (self.classInfo() != classInfo)
        return context->throwError(QScriptContext::TypeError, QLatin1String("String.prototype.valueOf"));

    return (self.internalValue());
}

QScriptValueImpl String::method_charAt(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QString str = context->thisObject().toString();

    int pos = 0;
    if (context->argumentCount() > 0)
        pos = int (context->argument(0).toInteger());

    QString result;
    if (pos >= 0 && pos < str.length())
        result += str.at(pos);

    return (QScriptValueImpl(eng, result));
}

QScriptValueImpl String::method_charCodeAt(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QString str = context->thisObject().toString();

    int pos = 0;
    if (context->argumentCount() > 0)
        pos = int (context->argument(0).toInteger());

    qsreal result = qSNaN();

    if (pos >= 0 && pos < str.length())
        result = str.at(pos).unicode();

    return (QScriptValueImpl(eng, result));
}

QScriptValueImpl String::method_concat(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QString value = context->thisObject().toString();

    for (int i = 0; i < context->argumentCount(); ++i)
        value += context->argument(i).toString();

    return (QScriptValueImpl(eng, value));
}

QScriptValueImpl String::method_indexOf(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QString value = context->thisObject().toString();

    QString searchString;
    if (context->argumentCount() > 0)
        searchString = context->argument(0).toString();

    int pos = 0;
    if (context->argumentCount() > 1)
        pos = int (context->argument(1).toInteger());

    int index = -1;
    if (! value.isEmpty())
        index = value.indexOf(searchString, qMax(pos, 0));

    return (QScriptValueImpl(eng, index));
}

QScriptValueImpl String::method_lastIndexOf(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QString value = context->thisObject().toString();

    QString searchString;
    if (context->argumentCount() > 0)
        searchString = context->argument(0).toString();

    qsreal position = context->argument(1).toNumber();
    if (qIsNaN(position))
        position = +qInf();
    else
        position = QScriptEnginePrivate::toInteger(position);

    int pos = QScriptEnginePrivate::toInt32(qMin(qMax(position, 0.0), qsreal(value.length())));
    if (!searchString.isEmpty() && pos == value.length())
        --pos;
    int index = value.lastIndexOf(searchString, pos);
    return (QScriptValueImpl(eng, index));
}

QScriptValueImpl String::method_localeCompare(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QString value = context->thisObject().toString();
    QString that = context->argument(0).toString();
    return QScriptValueImpl(eng, QString::localeAwareCompare(value, that));
}

QScriptValueImpl String::method_match(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QScriptValueImpl pattern = context->argument(0);

    if (! eng->regexpConstructor->get(pattern))
        eng->regexpConstructor->newRegExp(&pattern, context->argument(0).toString(), QString());

    QScriptValueImpl rx_exec = pattern.property(QLatin1String("exec"), QScriptValue::ResolvePrototype);
    if (! (rx_exec.isValid() && rx_exec.isFunction()))
        return context->throwError(QScriptContext::TypeError,
                                   QLatin1String("String.prototype.match"));

    QScriptValueImplList args;
    args << context->thisObject();

    QScriptValueImpl global = pattern.property(QLatin1String("global"));
    if (! (global.isValid() && global.toBoolean()))
        return (rx_exec.call(pattern, args));

    QScript::Array result;

    QScriptNameIdImpl *lastIndexId = eng->nameId(QLatin1String("lastIndex"));
    QScriptNameIdImpl *zeroId = eng->nameId(QLatin1String("0"));

    pattern.setProperty(lastIndexId, QScriptValueImpl(eng, 0));
    int n = 0;
    while (true) {
        qsreal lastIndex = pattern.property(lastIndexId).toNumber();
        QScriptValueImpl r = rx_exec.call(pattern, args);
        if (r.isNull())
            break;
        qsreal newLastIndex = pattern.property(lastIndexId).toNumber();
        if (newLastIndex == lastIndex)
            pattern.setProperty(lastIndexId, QScriptValueImpl(eng, lastIndex + 1));
        result.assign(n++, r.property(zeroId));
    }

    return (eng->newArray(result));
}

QScriptValueImpl String::method_replace(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QString input = context->thisObject().toString();
    QScriptValueImpl searchValue = context->argument(0);
    QScriptValueImpl replaceValue = context->argument(1);

    QString output;
    if (searchValue.classInfo() == eng->regexpConstructor->classInfo()) {
        // searchValue is a RegExp
        QScriptValueImpl rx_exec = searchValue.property(QLatin1String("exec"), QScriptValue::ResolvePrototype);
        if (!rx_exec.isFunction())
            return context->throwError(QScriptContext::TypeError,
                                       QLatin1String("String.prototype.replace"));
        QVector<QScriptValueImpl> occurrences;
        QScriptValueImpl global = searchValue.property(QLatin1String("global"));
        QScriptValueImplList args;
        args << QScriptValueImpl(eng, input);
        if (!global.toBoolean()) {
            QScriptValueImpl r = rx_exec.call(searchValue, args);
            if (!r.isNull())
                occurrences.append(r);
        } else {
            QScriptNameIdImpl *lastIndexId = eng->nameId(QLatin1String("lastIndex"));
            searchValue.setProperty(lastIndexId, QScriptValueImpl(eng, 0));
            while (true) {
                qsreal lastIndex = searchValue.property(lastIndexId).toNumber();
                QScriptValueImpl r = rx_exec.call(searchValue, args);
                if (r.isNull())
                    break;
                qsreal newLastIndex = searchValue.property(lastIndexId).toNumber();
                if (newLastIndex == lastIndex)
                    searchValue.setProperty(lastIndexId, QScriptValueImpl(eng, lastIndex + 1));
                occurrences.append(r);
            }
        }
        int pos = 0;
        if (replaceValue.isFunction()) {
            QScriptNameIdImpl *indexId = eng->nameId(QLatin1String("index"));
            QScriptNameIdImpl *lengthId = eng->nameId(QLatin1String("length"));
            for (int i = 0; i < occurrences.count(); ++i) {
                QScriptValueImpl needle = occurrences.at(i);
                int index = int (needle.property(indexId).toInteger());
                uint length = eng->toUint32(needle.property(lengthId).toNumber());
                output += input.mid(pos, index - pos);
                args.clear();
                args << QScriptValueImpl(eng, index);
                args << QScriptValueImpl(eng, input);
                for (uint j = 0; j < length; ++j)
                    args << needle.property(QScriptValueImpl(eng, j).toString());
                QScriptValueImpl ret = replaceValue.call(eng->nullValue(), args);
                output += ret.toString();
                pos = index + args[0].toString().length();
            }
        } else {
            // use string representation of replaceValue
            const QString replaceString = replaceValue.toString();
            const QLatin1Char dollar = QLatin1Char('$');
            QScriptNameIdImpl *indexId = eng->nameId(QLatin1String("index"));
            QScriptNameIdImpl *zeroId = eng->nameId(QLatin1String("0"));
            for (int i = 0; i < occurrences.count(); ++i) {
                QScriptValueImpl needle = occurrences.at(i);
                int index = int (needle.property(indexId).toInteger());
                output += input.mid(pos, index - pos);
                int j = 0;
                while (j < replaceString.length()) {
                    const QChar c = replaceString.at(j++);
                    if ((c == dollar) && (j < replaceString.length())) {
                        const QChar nc = replaceString.at(j);
                        if (nc == dollar) {
                            ++j;
                        } else if (nc == QLatin1Char('`')) {
                            ++j;
                            output += input.left(index);
                            continue;
                        } else if (nc == QLatin1Char('\'')) {
                            ++j;
                            output += input.mid(index + needle.property(zeroId).toString().length());
                            continue;
                        } else if (nc.isDigit()) {
                            ++j;
                            int cap = nc.toLatin1() - '0';
                            if ((j < replaceString.length()) && replaceString.at(j).isDigit()) {
                                cap = cap * 10;
                                cap = replaceString.at(j++).toLatin1() - '0';
                            }
                            output += needle.property(QScriptValueImpl(eng, cap).toString()).toString();
                            continue;
                        }
                    }
                    output += c;
                }
                pos = index + needle.property(zeroId).toString().length();
            }
            output += input.mid(pos);
        }
    } else {
        // use string representation of searchValue
        const QString searchString = searchValue.toString();
        int pos = 0;
        if (replaceValue.isFunction()) {
            QScriptValueImplList args;
            args << QScriptValueImpl(eng, searchString);
            args << eng->undefinedValue(); // dummy
            args << QScriptValueImpl(eng, input);
            int index = input.indexOf(searchString, pos);
            while (index != -1) {
                output += input.mid(pos, index - pos);
                args[1] = QScriptValueImpl(eng, index);
                QScriptValueImpl ret = replaceValue.call(eng->nullValue(), args);
                output += ret.toString();
                pos = index + searchString.length();
                index = input.indexOf(searchString, pos);
            }
            output += input.mid(pos);
        } else {
            // use string representation of replaceValue
            const QString replaceString = replaceValue.toString();
            const QLatin1Char dollar = QLatin1Char('$');
            int index = input.indexOf(searchString, pos);
            while (index != -1) {
                output += input.mid(pos, index - pos);
                int j = 0;
                while (j < replaceString.length()) {
                    const QChar c = replaceString.at(j++);
                    if ((c == dollar) && (j < replaceString.length())) {
                        const QChar nc = replaceString.at(j);
                        if (nc == dollar) {
                            ++j;
                        } else if (nc == QLatin1Char('`')) {
                            output += input.left(index);
                            ++j;
                            continue;
                        } else if (nc == QLatin1Char('\'')) {
                            output += input.mid(index + searchString.length());
                            ++j;
                            continue;
                        }
                    }
                    output += c;
                }
                pos = index + searchString.length();
                index = input.indexOf(searchString, pos);
            }
            output += input.mid(pos);
        }
    }
    return QScriptValueImpl(eng, output);
}

QScriptValueImpl String::method_search(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QScriptValueImpl pattern = context->argument(0);

    Ecma::RegExp::Instance *rx_data = 0;
    if (0 == (rx_data = eng->regexpConstructor->get(pattern))) {
        eng->regexpConstructor->newRegExp(&pattern, context->argument(0).toString(), QString());
        rx_data = eng->regexpConstructor->get(pattern);
    }

    QString value = context->thisObject().toString();
#ifndef QT_NO_REGEXP
    return (QScriptValueImpl(eng, value.indexOf(rx_data->value)));
#else
    return eng->nullValue();
#endif
}

QScriptValueImpl String::method_slice(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QString text = context->thisObject().toString();
    int length = text.length();

    int start = int (context->argument(0).toInteger());
    int end = context->argument(1).isUndefined()
              ? length : int (context->argument(1).toInteger());

    if (start < 0)
        start = qMax(length + start, 0);
    else
        start = qMin(start, length);

    if (end < 0)
        end = qMax(length + end, 0);
    else
        end = qMin(end, length);

    int count = qMax(0, end - start);
    return (QScriptValueImpl(eng, text.mid(start, count)));
}

QScriptValueImpl String::method_split(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QScriptValueImpl l = context->argument(1);
    quint32 lim = l.isUndefined() ? UINT_MAX : QScriptEnginePrivate::toUint32(l.toNumber());

    if (lim == 0)
        return eng->newArray();

    QString S = context->thisObject().toString();
    QScriptValueImpl separator = context->argument(0);

    QScript::Array A;
    // the argumentCount() check is for compatibility with spidermonkey;
    // it is not according to ECMA-262
    if (separator.isUndefined() && (context->argumentCount() == 0)) {
        A.assign(0, QScriptValueImpl(eng, S));
    } else {
        QStringList matches;
#ifndef QT_NO_REGEXP
        if (Ecma::RegExp::Instance *rx = eng->regexpConstructor->get(separator)) {
            matches = S.split(rx->value, rx->value.pattern().isEmpty()
                              ? QString::SkipEmptyParts : QString::KeepEmptyParts);
        } else
#endif // QT_NO_REGEXP
        {
            QString sep = separator.toString();
            matches = S.split(sep, sep.isEmpty()
                              ? QString::SkipEmptyParts : QString::KeepEmptyParts);
        }
        uint count = qMin(lim, uint(matches.count()));
        for (uint i = 0; i < count; ++i)
            A.assign(i, QScriptValueImpl(eng, matches.at(i)));
    }

    return eng->newArray(A);
}

QScriptValueImpl String::method_substring(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QString value = context->thisObject().toString();
    int length = value.length();

    qsreal start = 0;
    qsreal end = length;

    if (context->argumentCount() > 0)
        start = context->argument(0).toInteger();

    if (context->argumentCount() > 1)
        end = context->argument(1).toInteger();

    if (qIsNaN(start) || start < 0)
        start = 0;

    if (qIsNaN(end) || end < 0)
        end = 0;

    if (start > length)
        start = length;

    if (end > length)
        end = length;

    if (start > end) {
        qsreal was = start;
        start = end;
        end = was;
    }

    qint32 x = QScriptEnginePrivate::toInt32(start);
    qint32 y = QScriptEnginePrivate::toInt32(end - start);

    return (QScriptValueImpl(eng, value.mid(x, y)));
}

QScriptValueImpl String::method_toLowerCase(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QString value = context->thisObject().toString();
    return (QScriptValueImpl(eng, value.toLower()));
}

QScriptValueImpl String::method_toLocaleLowerCase(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *classInfo)
{
    return method_toLowerCase(context, eng, classInfo); // ### check me
}

QScriptValueImpl String::method_toUpperCase(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QString value = context->thisObject().toString();
    return (QScriptValueImpl(eng, value.toUpper()));
}

QScriptValueImpl String::method_toLocaleUpperCase(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *classInfo)
{
    return method_toUpperCase(context, eng, classInfo); // ### check me
}

QScriptValueImpl String::method_fromCharCode(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QString str;
    for (int i = 0; i < context->argumentCount(); ++i) {
        QChar c(context->argument(i).toUInt16());
        str += c;
    }
    return (QScriptValueImpl(eng, str));
}

} } // namespace QScript::Ecma

#endif // QT_NO_SCRIPT
