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

#include "qscriptextenumeration_p.h"

#ifndef QT_NO_SCRIPT

#include "qscriptengine_p.h"
#include "qscriptvalueimpl_p.h"
#include "qscriptcontext_p.h"
#include "qscriptmember_p.h"
#include "qscriptobject_p.h"

#include <QtCore/QtDebug>

namespace QScript { namespace Ext {

EnumerationClassData::EnumerationClassData(QScriptClassInfo *classInfo):
    m_classInfo(classInfo)
{
}

EnumerationClassData::~EnumerationClassData()
{
}

void EnumerationClassData::mark(const QScriptValueImpl &object, int generation)
{
    Q_ASSERT(object.isValid());

    QScriptEnginePrivate *eng = QScriptEnginePrivate::get(object.engine());

    if (Enumeration::Instance *instance = Enumeration::Instance::get(object, classInfo())) {
        eng->markObject(instance->object, generation);
        eng->markObject(instance->value, generation);
    }
}


Enumeration::Enumeration(QScriptEnginePrivate *eng):
    Ecma::Core(eng)
{
    m_classInfo = eng->registerClass(QLatin1String("Enumeration"));
    QExplicitlySharedDataPointer<QScriptClassData> data(new EnumerationClassData(m_classInfo));
    m_classInfo->setData(data);

    publicPrototype.invalidate();
    newEnumeration(&publicPrototype, eng->newArray());

    eng->newConstructor(&ctor, this, publicPrototype);

    const QScriptValue::PropertyFlags flags = QScriptValue::SkipInEnumeration;
    publicPrototype.setProperty(QLatin1String("toFirst"),
                                eng->createFunction(method_toFirst, 0, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("hasNext"),
                                eng->createFunction(method_hasNext, 0, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("next"),
                                eng->createFunction(method_next, 0, m_classInfo), flags);
}

Enumeration::~Enumeration()
{
}

Enumeration::Instance *Enumeration::Instance::get(const QScriptValueImpl &object, QScriptClassInfo *klass)
{
    if (! klass || klass == object.classInfo())
        return static_cast<Instance*> (object.objectData().data());

    return 0;
}

void Enumeration::execute(QScriptContextPrivate *context)
{
    if (context->argumentCount() > 0) {
        newEnumeration(&context->m_result, context->argument(0));
    } else {
        context->throwError(QScriptContext::TypeError,
                            QLatin1String("Enumeration.execute"));
    }
}

void Enumeration::newEnumeration(QScriptValueImpl *result, const QScriptValueImpl &object)
{
    Instance *instance = new Instance();
    instance->object = object;
    instance->value = object;
    instance->index = -1;
    instance->toFirst();

    engine()->newObject(result, publicPrototype, classInfo());
    result->setObjectData(QExplicitlySharedDataPointer<QScriptObjectData>(instance));
}

QScriptValueImpl Enumeration::method_toFirst(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *classInfo)
{
    if (Instance *instance = Instance::get(context->thisObject(), classInfo)) {
        instance->toFirst();
        return eng->undefinedValue();
    } else {
        return context->throwError(QScriptContext::TypeError,
                                   QLatin1String("Enumeration.toFirst"));
    }
}

QScriptValueImpl Enumeration::method_hasNext(QScriptContextPrivate *context, QScriptEnginePrivate *, QScriptClassInfo *classInfo)
{
    Instance *instance = Instance::get(context->thisObject(), classInfo);
    if (! instance || ! instance->value.isObject())
        return context->throwError(QScriptContext::TypeError,
                                   QLatin1String("Enumeration.hasNext"));

    QScriptValueImpl v;
    instance->hasNext(context, &v);
    return v;
}

QScriptValueImpl Enumeration::method_next(QScriptContextPrivate *context, QScriptEnginePrivate *, QScriptClassInfo *classInfo)
{
    Instance *instance = Instance::get(context->thisObject(), classInfo);
    if (! instance || ! instance->value.isObject())
        return context->throwError(QScriptContext::TypeError,
                                   QLatin1String("Enumeration.next"));

    QScriptValueImpl v;
    instance->next(context, &v);
    return v;
}

void Enumeration::Instance::toFirst()
{
    value = object;
    index = -1;
}

void Enumeration::Instance::hasNext(QScriptContextPrivate *context, QScriptValueImpl *result)
{
    QScriptEnginePrivate *eng = QScriptEnginePrivate::get(context->engine());
Lagain:
    int count = value.memberCount();
    bool found = false;
    while (! found && ++index < count) {
        QScript::Member member;
        value.member(index, &member);
        found = member.isValid() && ! member.dontEnum();
        if (found) {
            if (member.isObjectProperty() || value.isArray()) {
                QScriptValueImpl current;
                value.get(member, &current);
                found = current.isValid();
            }
            if (found && member.nameId()) {
                // make sure that it's not a shadow
                Member m;
                QScriptValueImpl b;
                if (object.resolve(member.nameId(), &m, &b, QScriptValue::ResolvePrototype))
                    found = (b.objectValue() == value.objectValue());
            }
        }
    }

    if (! found && value.prototype().isObject()) {
        value = value.prototype();
        index = -1;
        goto Lagain;
    }

    *result = QScriptValueImpl(eng, found);
}

void Enumeration::Instance::next(QScriptContextPrivate *context, QScriptValueImpl *result)
{
    QScriptEnginePrivate *eng = QScriptEnginePrivate::get(context->engine());

    QScript::Member member;
    value.member(index, &member);

    if (member.isObjectProperty() || member.nameId())
        eng->newNameId(result, member.nameId());

    else if (member.isNativeProperty() && ! member.nameId())
        eng->newNumber(result, member.id());

    else
        eng->newUndefined(result);
}

} } // namespace QScript::Ext

#endif // QT_NO_SCRIPT
