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

#include "qscriptextqobject_p.h"

#ifndef QT_NO_SCRIPT

#include "qscriptengine_p.h"
#include "qscriptvalueimpl_p.h"
#include "qscriptcontext_p.h"
#include "qscriptmember_p.h"
#include "qscriptobject_p.h"
#include "qscriptable.h"
#include "qscriptable_p.h"

#include <QtCore/QtDebug>
#include <QtCore/QMetaMethod>
#include <QtCore/QRegExp>
#include <QtCore/QVarLengthArray>
#include "qscriptextqobject_p.h"

// we use bits 15..12 of property flags
enum {
    PROPERTY_ID      = 0 << 12,
    DYNAPROPERTY_ID  = 1 << 12,
    METHOD_ID        = 2 << 12,
    CHILD_ID         = 3 << 12,
    ID_MASK          = 7 << 12,
    MAYBE_OVERLOADED = 8 << 12
};

static const bool GeneratePropertyFunctions = true;

QByteArray QScriptMetaType::name() const
{
    if (!m_name.isEmpty())
        return m_name;
    else if (m_kind == Variant)
        return "QVariant";
    return QByteArray();
}

namespace QScript {

class QtPropertyFunction: public QScriptFunction
{
public:
    QtPropertyFunction(const QMetaObject *meta, int index)
        : m_meta(meta), m_index(index)
        { }

    ~QtPropertyFunction() { }

    virtual void execute(QScriptContextPrivate *context);

    virtual Type type() const { return QScriptFunction::QtProperty; }

private:
    const QMetaObject *m_meta;
    int m_index;
};

class QObjectPrototype : public QObject
{
    Q_OBJECT
public:
    QObjectPrototype(QObject *parent = 0)
        : QObject(parent) { }
    ~QObjectPrototype() { }
};

static inline QByteArray methodName(const QMetaMethod &method)
{
    QByteArray signature = method.signature();
    return signature.left(signature.indexOf('('));
}

static inline QVariant variantFromValue(int targetType, const QScriptValueImpl &value)
{
    QVariant v(targetType, (void *)0);
    QScriptEngine *eng = value.engine();
    Q_ASSERT(eng);
    if (QScriptEnginePrivate::get(eng)->convert(value, targetType, v.data()))
        return v;
    if (uint(targetType) == QVariant::LastType)
        return value.toVariant();
    if (value.isVariant()) {
        v = value.toVariant();
        if (v.canConvert(QVariant::Type(targetType))) {
            v.convert(QVariant::Type(targetType));
            return v;
        }
        QByteArray typeName = v.typeName();
        if (typeName.endsWith('*')
            && (QMetaType::type(typeName.left(typeName.size()-1)) == targetType)) {
            return QVariant(targetType, *reinterpret_cast<void* *>(v.data()));
        }
    }

    return QVariant();
}

ExtQObject::Instance *ExtQObject::Instance::get(const QScriptValueImpl &object, QScriptClassInfo *klass)
{
    if (! klass || klass == object.classInfo())
        return static_cast<Instance*> (object.objectData().data());

    return 0;
}


void ExtQObject::Instance::execute(QScriptContextPrivate *context)
{
    if (!value) {
        context->throwError(QLatin1String("cannot call function of deleted QObject"));
        return;
    }

    const QMetaObject *meta = value->metaObject();
    // look for qscript_call()
    QByteArray qscript_call = QByteArray("qscript_call");
    int index;
    for (index = meta->methodCount() - 1; index >= 0; --index) {
        if (methodName(meta->method(index)) == qscript_call)
            break;
    }
    if (index < 0) {
        context->throwError(QScriptContext::TypeError,
                            QLatin1String("not a function"));
        return;
    }

    QtFunction fun(value, index, /*maybeOverloaded=*/true);
    fun.execute(context);
}

static inline QScriptable *scriptableFromQObject(QObject *qobj)
{
    void *ptr = qobj->qt_metacast("QScriptable");
    return reinterpret_cast<QScriptable*>(ptr);
}

static bool isObjectProperty(const QScriptValueImpl &object, const char *name)
{
    QScriptEnginePrivate *eng = QScriptEnginePrivate::get(object.engine());
    QScriptNameIdImpl *nameId = eng->nameId(QLatin1String(name));
    QScript::Member member;
    QScriptValueImpl base;
    return object.resolve(nameId, &member, &base, QScriptValue::ResolveLocal)
        && member.testFlags(QScript::Member::ObjectProperty);
}

static bool hasMethodAccess(const QMetaMethod &method)
{
    return (method.access() != QMetaMethod::Private);
}

class ExtQObjectData: public QScriptClassData
{
public:
    ExtQObjectData(QScriptEnginePrivate *, QScriptClassInfo *classInfo)
        : m_classInfo(classInfo)
    {
    }

    virtual bool resolve(const QScriptValueImpl &object, QScriptNameIdImpl *nameId,
                         QScript::Member *member, QScriptValueImpl *)
    {
        ExtQObject::Instance *inst = ExtQObject::Instance::get(object, m_classInfo);
        QObject *qobject = inst->value;
        if (! qobject) {
            // the object was deleted. We return true so we can
            // throw an error in get()/put()
            member->native(nameId, /*id=*/-1, /*flags=*/0);
            return true;
        }

        const QScriptEngine::QObjectWrapOptions &opt = inst->options;
        const QMetaObject *meta = qobject->metaObject();

        QScriptEnginePrivate *eng = QScriptEnginePrivate::get(object.engine());

#ifndef Q_SCRIPT_NO_QMETAOBJECT_CACHE
        QScriptMetaObject *metaCache = eng->cachedMetaObject(meta);
        if (metaCache->findMember(nameId, member)) {
            bool ignore = false;
            switch (member->flags() & ID_MASK) {
            case PROPERTY_ID:
                ignore = (opt & QScriptEngine::ExcludeSuperClassProperties)
                         && (member->id() < meta->propertyOffset());
                break;
            case METHOD_ID:
                ignore = (opt & QScriptEngine::ExcludeSuperClassMethods)
                         && (member->id() < meta->methodOffset());
                break;
            // we don't cache dynamic properties nor children,
            // so no need to handle DYNAPROPERTY_ID and CHILD_ID
            default:
                break;
            }
            if (!ignore)
                return true;
        }
#endif

        QString memberName = eng->toString(nameId);
        QByteArray name = memberName.toLatin1();

        int index = -1;

        if (name.contains('(')) {
            QByteArray normalized = QMetaObject::normalizedSignature(name);
            if (-1 != (index = meta->indexOfMethod(normalized))) {
                QMetaMethod method = meta->method(index);
                if (hasMethodAccess(method)) {
                    member->native(nameId, index,
                                   QScriptValue::QObjectMember
                                   | METHOD_ID);
#ifndef Q_SCRIPT_NO_QMETAOBJECT_CACHE
                    metaCache->registerMember(nameId, *member);
#endif
                    if (!(opt & QScriptEngine::ExcludeSuperClassMethods)
                        || (index >= meta->methodOffset())) {
                        return true;
                    }
                }
            }
        }

        index = meta->indexOfProperty(name);
        if (index != -1) {
            QMetaProperty prop = meta->property(index);
            if (prop.isScriptable()) {
                member->native(nameId, index,
                               QScriptValue::Undeletable
                               | (!prop.isWritable()
                                  ? QScriptValue::ReadOnly
                                  : QScriptValue::PropertyFlag(0))
                               | (GeneratePropertyFunctions
                                  ? (QScriptValue::PropertyGetter
                                     | QScriptValue::PropertySetter)
                                  : QScriptValue::PropertyFlag(0))
                               | QScriptValue::QObjectMember
                               | PROPERTY_ID);
#ifndef Q_SCRIPT_NO_QMETAOBJECT_CACHE
                metaCache->registerMember(nameId, *member);
#endif
                if (!(opt & QScriptEngine::ExcludeSuperClassProperties)
                    || (index >= meta->propertyOffset())) {
                    return true;
                }
            }
        }

        index = qobject->dynamicPropertyNames().indexOf(name);
        if (index != -1) {
            member->native(nameId, index,
                           QScriptValue::QObjectMember
                           | DYNAPROPERTY_ID);
            // not cached because it can be removed
            return true;
        }

        const int offset = (opt & QScriptEngine::ExcludeSuperClassMethods)
                           ? meta->methodOffset() : 0;
        for (index = meta->methodCount() - 1; index >= offset; --index) {
            QMetaMethod method = meta->method(index);
            if (hasMethodAccess(method)
                && (methodName(method) == name)) {
                member->native(nameId, index,
                               QScriptValue::QObjectMember
                               | METHOD_ID
                               | MAYBE_OVERLOADED);
#ifndef Q_SCRIPT_NO_QMETAOBJECT_CACHE
                metaCache->registerMember(nameId, *member);
#endif
                return true;
            }
        }

        if (!(opt & QScriptEngine::ExcludeChildObjects)) {
            QList<QObject*> children = qobject->children();
            for (index = 0; index < children.count(); ++index) {
                QObject *child = children.at(index);
                if (child->objectName() == memberName) {
                    member->native(nameId, index,
                                   QScriptValue::ReadOnly
                                   | QScriptValue::Undeletable
                                   | QScriptValue::SkipInEnumeration
                                   | CHILD_ID);
                    // not cached because it can be removed or change name
                    return true;
                }
            }
        }

        if (opt & QScriptEngine::AutoCreateDynamicProperties) {
            member->native(nameId, -1, DYNAPROPERTY_ID);
            return true;
        }

        return false;
     }

    virtual bool get(const QScriptValueImpl &obj, const QScript::Member &member, QScriptValueImpl *result)
    {
        if (! member.isNativeProperty())
            return false;

        QScriptEnginePrivate *eng = QScriptEnginePrivate::get(obj.engine());

        ExtQObject::Instance *inst = ExtQObject::Instance::get(obj, m_classInfo);
        QObject *qobject = inst->value;
        if (!qobject) {
            QScriptContextPrivate *ctx = QScriptContextPrivate::get(eng->currentContext());
            *result = ctx->throwError(
                QString::fromLatin1("cannot access member `%0' of deleted QObject")
                .arg(member.nameId()->s));
            return true;
        }

        switch (member.flags() & ID_MASK) {
        case PROPERTY_ID: {
            const QMetaObject *meta = qobject->metaObject();
            const int propertyIndex = member.id();
            QMetaProperty prop = meta->property(propertyIndex);
            Q_ASSERT(prop.isScriptable());
            if (GeneratePropertyFunctions) {
                QScriptValueImpl accessor;
#ifndef Q_SCRIPT_NO_QMETAOBJECT_CACHE
                QScriptMetaObject *metaCache = eng->cachedMetaObject(meta);
                accessor = metaCache->findPropertyAccessor(propertyIndex);
                if (!accessor.isValid()) {
#endif
                    accessor = eng->createFunction(new QtPropertyFunction(meta, propertyIndex));
#ifndef Q_SCRIPT_NO_QMETAOBJECT_CACHE
                    metaCache->registerPropertyAccessor(propertyIndex, accessor);
                }
#endif
                *result = accessor;
            } else {
                QVariant v = prop.read(qobject);
                *result = eng->valueFromVariant(v);
            }
        }   break;

        case DYNAPROPERTY_ID: {
            if (member.id() != -1) {
                QVariant v = qobject->property(member.nameId()->s.toLatin1());
                *result = eng->valueFromVariant(v);
            } else {
                *result = eng->undefinedValue();
            }
        }   break;

        case METHOD_ID: {
            QScript::Member m;
            bool maybeOverloaded = (member.flags() & MAYBE_OVERLOADED) != 0;
            *result = eng->createFunction(new QtFunction(qobject, member.id(),
                                                           maybeOverloaded));
            // make it persist (otherwise Function.prototype.disconnect() would fail)
            QScriptObject *instance = obj.objectValue();
            if (!instance->findMember(member.nameId(), &m)) {
                instance->createMember(member.nameId(), &m,
                                       QScriptValue::QObjectMember);
            }
            instance->put(m, *result);
        }   break;

        case CHILD_ID: {
            QObject *child = qobject->children().at(member.id());
            *result = eng->newQObject(child);
        }   break;

        } // switch

        return true;
    }

    virtual bool put(QScriptValueImpl *object, const QScript::Member &member, const QScriptValueImpl &value)
    {
        if (! member.isNativeProperty() || ! member.isWritable())
            return false;

        ExtQObject::Instance *inst = ExtQObject::Instance::get(*object, m_classInfo);
        QObject *qobject = inst->value;
        if (!qobject) {
            QScriptEnginePrivate *eng = QScriptEnginePrivate::get(object->engine());
            QScriptContextPrivate *ctx = QScriptContextPrivate::get(eng->currentContext());
            ctx->throwError(QString::fromLatin1("cannot access member `%0' of deleted QObject")
                            .arg(member.nameId()->s));
            return true;
        }

        switch (member.flags() & ID_MASK) {
        case CHILD_ID:
            return false;

        case METHOD_ID: {
            QScript::Member m;
            QScriptObject *instance = object->objectValue();
            if (!instance->findMember(member.nameId(), &m)) {
                instance->createMember(member.nameId(), &m,
                                       /*flags=*/0);
            }
            instance->put(m, value);
            return true;
        }

        case PROPERTY_ID:
            if (GeneratePropertyFunctions) {
                // we shouldn't get here, QScriptValueImpl::setProperty() messed up
                Q_ASSERT_X(0, "put", "Q_PROPERTY access cannot be overridden");
                return false;
            } else {
                const QMetaObject *meta = qobject->metaObject();
                QMetaProperty prop = meta->property(member.id());
                Q_ASSERT(prop.isScriptable());
                QVariant v = variantFromValue(prop.userType(), value);
                bool ok = prop.write(qobject, v);
                return ok;
            }

        case DYNAPROPERTY_ID: {
            QVariant v = value.toVariant();
            return ! qobject->setProperty(member.nameId()->s.toLatin1(), v);
        }

        } // switch
        return false;
    }

    virtual int extraMemberCount(const QScriptValueImpl &object)
    {
        ExtQObject::Instance *inst = ExtQObject::Instance::get(object, m_classInfo);
        QObject *qobject = inst->value;
        if (! qobject)
            return 0;
        const QScriptEngine::QObjectWrapOptions &opt = inst->options;
        const QMetaObject *meta = qobject->metaObject();
        int count = 0;
        // meta-object-defined properties
        int offset = (opt & QScriptEngine::ExcludeSuperClassProperties)
                     ? meta->propertyOffset() : 0;
        for (int i = offset; i < meta->propertyCount(); ++i) {
            QMetaProperty prop = meta->property(i);
            if (prop.isScriptable()
                && !isObjectProperty(object, prop.name())) {
                ++count;
            }
        }
        // dynamic properties
        QList<QByteArray> dpNames = qobject->dynamicPropertyNames();
        for (int i = 0; i < dpNames.count(); ++i) {
            if (!isObjectProperty(object, dpNames.at(i))) {
                ++count;
            }
        }
        // meta-object-defined methods
        offset = (opt & QScriptEngine::ExcludeSuperClassMethods)
                 ? meta->methodOffset() : 0;
        for (int i = offset; i < meta->methodCount(); ++i) {
            QMetaMethod method = meta->method(i);
            if (hasMethodAccess(method)
                && !isObjectProperty(object, method.signature())) {
                ++count;
            }
        }
        return count;
    }

    virtual bool extraMember(const QScriptValueImpl &object, int index, QScript::Member *member)
    {
        ExtQObject::Instance *inst = ExtQObject::Instance::get(object, m_classInfo);
        QObject *qobject = inst->value;
        const QScriptEngine::QObjectWrapOptions &opt = inst->options;
        const QMetaObject *meta = qobject->metaObject();
        QScriptEnginePrivate *eng = QScriptEnginePrivate::get(object.engine());
        int logicalIndex = 0;
        // meta-object-defined properties
        int physicalIndex = (opt & QScriptEngine::ExcludeSuperClassProperties)
                            ? meta->propertyOffset() : 0;
        for ( ; physicalIndex < meta->propertyCount(); ++physicalIndex) {
            QMetaProperty prop = meta->property(physicalIndex);
            if (prop.isScriptable()
                && !isObjectProperty(object, prop.name())) {
                if (logicalIndex == index)
                    break;
                ++logicalIndex;
            }
        }
        if (physicalIndex < meta->propertyCount()) {
            QMetaProperty prop = meta->property(physicalIndex);
            QScriptNameIdImpl *nameId = eng->nameId(QLatin1String(prop.name()));
            member->native(nameId, physicalIndex,
                           QScriptValue::Undeletable
                           | (!prop.isWritable()
                              ? QScriptValue::ReadOnly
                              : QScriptValue::PropertyFlag(0))
                           | QScriptValue::QObjectMember
                           | PROPERTY_ID);
            return true;
        }

        // dynamic properties
        physicalIndex = 0;
        QList<QByteArray> dpNames = qobject->dynamicPropertyNames();
        for ( ; physicalIndex < dpNames.count(); ++physicalIndex) {
            if (!isObjectProperty(object, dpNames.at(physicalIndex))) {
                if (logicalIndex == index)
                    break;
                ++logicalIndex;
            }
        }
        if (physicalIndex < dpNames.count()) {
            QByteArray name = dpNames.at(physicalIndex);
            QScriptNameIdImpl *nameId = eng->nameId(QLatin1String(name));
            member->native(nameId, physicalIndex,
                           QScriptValue::QObjectMember
                           | DYNAPROPERTY_ID);
            return true;
        }

        // meta-object-defined methods
        physicalIndex = (opt & QScriptEngine::ExcludeSuperClassMethods)
                        ? meta->methodOffset() : 0;
        for ( ; physicalIndex < meta->methodCount(); ++physicalIndex) {
            QMetaMethod method = meta->method(physicalIndex);
            if (hasMethodAccess(method)
                && !isObjectProperty(object, method.signature())) {
                if (logicalIndex == index)
                    break;
                ++logicalIndex;
            }
        }
        if (physicalIndex < meta->methodCount()) {
            QMetaMethod method = meta->method(physicalIndex);
            QScriptNameIdImpl *nameId = eng->nameId(QLatin1String(method.signature()));
            member->native(nameId, index,
                           QScriptValue::QObjectMember
                           | METHOD_ID);
            return true;
        }

        return false;
    }

    virtual bool removeMember(const QScriptValueImpl &object,
                              const QScript::Member &member)
    {
        QObject *qobject = object.toQObject();
        if (!qobject || !member.isNativeProperty() || !member.isDeletable())
            return false;

        if ((member.flags() & ID_MASK) == DYNAPROPERTY_ID) {
            qobject->setProperty(member.nameId()->s.toLatin1(), QVariant());
            return true;
        }

        return false;
    }

    virtual void mark(const QScriptValueImpl &object, int generation)
    {
        ExtQObject::Instance *inst = ExtQObject::Instance::get(object, m_classInfo);
        if (inst->isConnection) {
            ConnectionQObject *connection = static_cast<ConnectionQObject*>((QObject*)inst->value);
            Q_ASSERT(connection != 0);
            connection->mark(generation);
        }
    }

private:
    QScriptClassInfo *m_classInfo;
};

} // ::QScript



QScript::ExtQObject::ExtQObject(QScriptEnginePrivate *eng, QScriptClassInfo *classInfo):
    Ecma::Core(eng), m_classInfo(classInfo)
{
    newQObject(&publicPrototype, new QScript::QObjectPrototype(),
               QScriptEngine::AutoOwnership,
               QScriptEngine::ExcludeSuperClassMethods
               | QScriptEngine::ExcludeSuperClassProperties
               | QScriptEngine::ExcludeChildObjects);

    eng->newConstructor(&ctor, this, publicPrototype);
    const QScriptValue::PropertyFlags flags = QScriptValue::SkipInEnumeration;
    publicPrototype.setProperty(QLatin1String("toString"),
                                eng->createFunction(method_toString, 0, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("findChild"),
                                eng->createFunction(method_findChild, 1, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("findChildren"),
                                eng->createFunction(method_findChildren, 1, m_classInfo), flags);

    QExplicitlySharedDataPointer<QScriptClassData> data(new QScript::ExtQObjectData(eng, classInfo));
    m_classInfo->setData(data);
}

QScript::ExtQObject::~ExtQObject()
{
}

void QScript::ExtQObject::execute(QScriptContextPrivate *context)
{
    QScriptValueImpl tmp;
    newQObject(&tmp, 0);
    context->setReturnValue(tmp);
}

void QScript::ExtQObject::newQObject(QScriptValueImpl *result, QObject *value,
                                     QScriptEngine::ValueOwnership ownership,
                                     const QScriptEngine::QObjectWrapOptions &options,
                                     bool isConnection)
{
    Instance *instance = new Instance();
    instance->value = value;
    instance->isConnection = isConnection;
    instance->ownership = ownership;
    instance->options = options;

    engine()->newObject(result, publicPrototype, classInfo());
    result->setObjectData(QExplicitlySharedDataPointer<QScriptObjectData>(instance));
}

QScriptValueImpl QScript::ExtQObject::method_findChild(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *classInfo)
{
    if (Instance *instance = Instance::get(context->thisObject(), classInfo)) {
        QObject *obj = instance->value;
        QString name = context->argument(0).toString();
        return eng->newQObject(qFindChild<QObject*>(obj, name));
    }
    return eng->undefinedValue();
}

QScriptValueImpl QScript::ExtQObject::method_findChildren(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *classInfo)
{
    if (Instance *instance = Instance::get(context->thisObject(), classInfo)) {
        QObject *obj = instance->value;
        QList<QObject*> found;
        QScriptValueImpl arg = context->argument(0);
#ifndef QT_NO_REGEXP
        if (arg.isRegExp()) {
            QRegExp re = arg.toRegExp();
            found = qFindChildren<QObject*>(obj, re);
        } else
#endif
        {
            QString name = arg.toString();
            found = qFindChildren<QObject*>(obj, name);
        }
        QScriptValueImpl result = eng->newArray(found.size());
        for (int i = 0; i < found.size(); ++i) {
            QScriptValueImpl value = eng->newQObject(found.at(i));
            result.setProperty(i, value);
        }
        return result;
    }
    return eng->undefinedValue();
}

QScriptValueImpl QScript::ExtQObject::method_toString(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *classInfo)
{
    if (Instance *instance = Instance::get(context->thisObject(), classInfo)) {
        QObject *obj = instance->value;
        const QMetaObject *meta = obj ? obj->metaObject() : &QObject::staticMetaObject;
        QString name = obj ? obj->objectName() : QString::fromUtf8("unnamed");

        QString str = QString::fromUtf8("%0(name = \"%1\")")
                      .arg(QLatin1String(meta->className())).arg(name);
        return QScriptValueImpl(eng, str);
    }
    return eng->undefinedValue();
}

QScript::ConnectionQObject::ConnectionQObject(const QMetaMethod &method,
                                              const QScriptValueImpl &sender,
                                              const QScriptValueImpl &receiver,
                                              const QScriptValueImpl &slot)
    : m_method(method), m_sender(sender),
      m_receiver(receiver)
{
    m_slot = slot;

    QScriptEngine *eng = m_slot.engine();
    QScriptEnginePrivate *eng_p = QScriptEnginePrivate::get(eng);
    QScriptValueImpl me;
    eng_p->qobjectConstructor->newQObject(&me, this, QScriptEngine::QtOwnership,
                                          /*options=*/0, /*isConnection=*/true);
    QScriptValuePrivate::init(m_self, eng_p->registerValue(me));

    QObject *qobject = static_cast<QtFunction*>(sender.toFunction())->object();
    Q_ASSERT(qobject);
    connect(qobject, SIGNAL(destroyed()), this, SLOT(deleteLater()));
}

QScript::ConnectionQObject::~ConnectionQObject()
{
}

static const uint qt_meta_data_ConnectionQObject[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      19,   18,   18,   18, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ConnectionQObject[] = {
    "ConnectionQObject\0\0execute()\0"
};

const QMetaObject QScript::ConnectionQObject::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ConnectionQObject,
      qt_meta_data_ConnectionQObject, 0 }
};

const QMetaObject *QScript::ConnectionQObject::metaObject() const
{
    return &staticMetaObject;
}

void *QScript::ConnectionQObject::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ConnectionQObject))
        return static_cast<void*>(const_cast<ConnectionQObject*>(this));
    return QObject::qt_metacast(_clname);
}

int QScript::ConnectionQObject::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: execute(_a); break;
        }
        _id -= 1;
    }
    return _id;
}

void QScript::ConnectionQObject::execute(void **argv)
{
    Q_ASSERT(m_slot.isValid());

    QScriptEnginePrivate *eng = QScriptEnginePrivate::get(m_slot.engine());

    QScriptFunction *fun = eng->convertToNativeFunction(m_slot);
    Q_ASSERT(fun != 0);

    QList<QByteArray> parameterTypes = m_method.parameterTypes();
    int argc = parameterTypes.count();

    QScriptValueImpl activation;
    eng->newActivation(&activation);
    QScriptObject *activation_data = activation.objectValue();
    activation_data->m_scope = m_slot.scope();

    int formalCount = fun->formals.count();
    int mx = qMax(formalCount, argc);
    activation_data->m_members.resize(mx + 1);
    activation_data->m_objects.resize(mx + 1);
    for (int i = 0; i < mx; ++i) {
        QScriptNameIdImpl *nameId;
        if (i < formalCount)
            nameId = fun->formals.at(i);
        else
            nameId = 0;
        activation_data->m_members[i].object(nameId, i,
                                             QScriptValue::Undeletable
                                             | QScriptValue::SkipInEnumeration);
        if (i < argc) {
            int argType = QMetaType::type(parameterTypes.at(i));
            activation_data->m_objects[i] = eng->create(argType, argv[i + 1]);
        } else {
            activation_data->m_objects[i] = eng->undefinedValue();
        }
    }

    QScriptValueImpl senderObject;
    if (sender() == m_sender.toQObject())
        senderObject = m_sender;
    else
        senderObject = eng->newQObject(sender());
    activation_data->m_members[mx].object(eng->idTable()->id___qt_sender__, mx,
                                          QScriptValue::SkipInEnumeration);
    activation_data->m_objects[mx] = senderObject;

    QScriptValueImpl thisObject;
    if (m_receiver.isObject())
        thisObject = m_receiver;
    else
        thisObject = eng->globalObject();

    QScriptContext *context = eng->pushContext();
    QScriptContextPrivate *context_data = QScriptContextPrivate::get(context);
    context_data->m_activation = activation;
    context_data->m_callee = m_slot;
    context_data->m_thisObject = thisObject;
    context_data->argc = argc;
    context_data->args = const_cast<QScriptValueImpl*> (activation_data->m_objects.constData());

    fun->execute(context_data);

    if (context->state() == QScriptContext::ExceptionState) {
        qWarning() << "***" << context->returnValue().toString(); // ### fixme
    }

    eng->popContext();
}

void QScript::ConnectionQObject::mark(int generation)
{
    if (m_sender.isValid())
        m_sender.mark(generation);
    if (m_receiver.isValid())
        m_receiver.mark(generation);
    if (m_slot.isValid())
        m_slot.mark(generation);
}

bool QScript::ConnectionQObject::hasTarget(const QScriptValueImpl &receiver,
                                           const QScriptValueImpl &slot) const
{
    if (receiver.isObject() != m_receiver.isObject())
        return false;
    if ((receiver.isObject() && m_receiver.isObject())
        && (receiver.objectValue() != m_receiver.objectValue())) {
        return false;
    }
    return (slot.objectValue() == m_slot.objectValue());
}

void QScript::QtPropertyFunction::execute(QScriptContextPrivate *context)
{
    QScriptEngine *eng = context->engine();
    QScriptEnginePrivate *eng_p = QScriptEnginePrivate::get(eng);
    QScriptValueImpl result = eng_p->undefinedValue();

    QScriptValueImpl object = context->thisObject();
    QObject *qobject = object.toQObject();
    while ((!qobject || (qobject->metaObject() != m_meta))
           && object.prototype().isObject()) {
        object = object.prototype();
        qobject = object.toQObject();
    }
    Q_ASSERT(qobject);

    QMetaProperty prop = m_meta->property(m_index);
    Q_ASSERT(prop.isScriptable());

    if (context->argumentCount() == 0) {
        // get
        if (prop.isValid()) {
            QScriptable *scriptable = scriptableFromQObject(qobject);
            QScriptEngine *oldEngine = 0;
            if (scriptable) {
                oldEngine = QScriptablePrivate::get(scriptable)->engine;
                QScriptablePrivate::get(scriptable)->engine = eng;
            }

            QVariant v = prop.read(qobject);

            if (scriptable)
                QScriptablePrivate::get(scriptable)->engine = oldEngine;

            result = eng_p->valueFromVariant(v);
        }
    } else {
        // set
        QVariant v = variantFromValue(prop.userType(), context->argument(0));

        QScriptable *scriptable = scriptableFromQObject(qobject);
        QScriptEngine *oldEngine = 0;
        if (scriptable) {
            oldEngine = QScriptablePrivate::get(scriptable)->engine;
            QScriptablePrivate::get(scriptable)->engine = eng;
        }

        prop.write(qobject, v);

        if (scriptable)
            QScriptablePrivate::get(scriptable)->engine = oldEngine;

        result = context->argument(0);
    }
    context->m_result = result;
}

static int indexOfMetaEnum(const QMetaObject *meta, const QByteArray &str)
{
    QByteArray scope;
    QByteArray name;
    int scopeIdx = str.indexOf("::");
    if (scopeIdx != -1) {
        scope = str.left(scopeIdx);
        name = str.mid(scopeIdx + 2);
    } else {
        name = str;
    }
    for (int i = meta->enumeratorCount() - 1; i >= 0; --i) {
        QMetaEnum m = meta->enumerator(i);
        if ((m.name() == name)/* && (scope.isEmpty() || (m.scope() == scope))*/)
            return i;
    }
    return -1;
}

void QScript::QtFunction::execute(QScriptContextPrivate *context)
{
    if (!m_object) {
        context->throwError(QLatin1String("cannot call function of deleted QObject"));
        return;
    }

    QScriptEngine *eng = context->engine();
    QScriptEnginePrivate *eng_p = QScriptEnginePrivate::get(eng);

    QScriptValueImpl result = eng_p->undefinedValue();

    const QMetaObject *meta = m_object->metaObject();

    QObject *thisQObject = context->thisObject().toQObject();
    if (!thisQObject) // ### TypeError
        thisQObject = m_object;

    QByteArray funName;
    if (!meta->cast(thisQObject)) {
#if 0
        // ### find common superclass, see if initialIndex is
        //     in that class (or a superclass of that class),
        //     then it's still safe to execute it
        funName = methodName(meta->method(m_initialIndex));
        context->throwError(
            QString::fromUtf8("cannot execute %0: %1 does not inherit %2")
            .arg(QLatin1String(funName))
            .arg(QLatin1String(thisQObject->metaObject()->className()))
            .arg(QLatin1String(meta->className())));
        return;
#endif
        // invoking a function in the prototype
        thisQObject = m_object;
    }

#ifndef Q_SCRIPT_NO_QMETAOBJECT_CACHE
    QScriptMetaObject *metaCache = eng_p->cachedMetaObject(meta);
#endif

    QScriptMetaMethod chosenMethod;
    int chosenIndex = -1;
    QVarLengthArray<QVariant, 9> args;
    QVector<QScriptMetaArguments> candidates;
    for (int index = m_initialIndex; index >= 0; --index) {
#ifndef Q_SCRIPT_NO_QMETAOBJECT_CACHE
        QScriptMetaMethod mtd = metaCache->findMethod(index);
        if (!mtd.isValid())
#else
        QScriptMetaMethod mtd;
#endif
        {
            QMetaMethod method = meta->method(index);

            QVector<QScriptMetaType> types;
            // resolve return type
            QByteArray returnTypeName = method.typeName();
            int rtype = QMetaType::type(returnTypeName);
            if ((rtype == 0) && !returnTypeName.isEmpty()) {
                if (returnTypeName == "QVariant") {
                    types.append(QScriptMetaType::variant());
                } else if (returnTypeName.endsWith('*')) {
                    types.append(QScriptMetaType::metaType(QMetaType::VoidStar, returnTypeName));
                } else {
                    int enumIndex = indexOfMetaEnum(meta, returnTypeName);
                    if (enumIndex != -1)
                        types.append(QScriptMetaType::metaEnum(enumIndex, returnTypeName));
                    else
                        types.append(QScriptMetaType::unresolved(returnTypeName));
                }
            } else {
                types.append(QScriptMetaType::metaType(rtype, returnTypeName));
            }
            // resolve argument types
            QList<QByteArray> parameterTypeNames = method.parameterTypes();
            for (int i = 0; i < parameterTypeNames.count(); ++i) {
                QByteArray argTypeName = parameterTypeNames.at(i);
                int atype = QMetaType::type(argTypeName);
                if (atype == 0) {
                    if (argTypeName == "QVariant") {
                        types.append(QScriptMetaType::variant());
                    } else {
                        int enumIndex = indexOfMetaEnum(meta, argTypeName);
                        if (enumIndex != -1)
                            types.append(QScriptMetaType::metaEnum(enumIndex, argTypeName));
                        else
                            types.append(QScriptMetaType::unresolved(argTypeName));
                    }
                } else {
                    types.append(QScriptMetaType::metaType(atype, argTypeName));
                }
            }

            mtd = QScriptMetaMethod(methodName(method), types);

#ifndef Q_SCRIPT_NO_QMETAOBJECT_CACHE
            if (mtd.fullyResolved())
                metaCache->registerMethod(index, mtd);
#endif
        }

        if (index == m_initialIndex)
            funName = mtd.name();
        else if (mtd.name() != funName)
            continue;

        if (context->argumentCount() < mtd.argumentCount())
            continue;

        if (!mtd.fullyResolved()) {
            // remember it so we can give an error message later, if necessary
            candidates.append(QScriptMetaArguments(/*matchDistance=*/INT_MAX, index,
                                                   mtd, QVarLengthArray<QVariant, 9>()));
            continue;
        }

        if (args.count() != mtd.count())
            args.resize(mtd.count());

        QScriptMetaType retType = mtd.returnType();
        args[0] = QVariant(retType.typeId(), (void *)0); // the result

        // try to convert arguments
        bool converted = true;
        int matchDistance = 0;
        for (int i = 0; converted && i < mtd.argumentCount(); ++i) {
            QScriptValueImpl actual = context->argument(i);
            QScriptMetaType argType = mtd.argumentType(i);
            int tid = argType.typeId();
            QVariant v(tid, (void *)0);

            converted = eng_p->convert(actual, tid, v.data());

            if (!converted) {
                if (actual.isVariant()) {
                    QVariant &vv = actual.variantValue();
                    if (argType.isVariant()) {
                        v = vv;
                        converted = true;
                    } else if (vv.canConvert(QVariant::Type(tid))) {
                        v = vv;
                        converted = v.convert(QVariant::Type(tid));
                        if (converted)
                            matchDistance += 10;
                    } else {
                        QByteArray vvTypeName = vv.typeName();
                        if (vvTypeName.endsWith('*')
                            && (vvTypeName.left(vvTypeName.size()-1) == argType.name())) {
                            v = QVariant(tid, *reinterpret_cast<void* *>(vv.data()));
                            converted = true;
                            matchDistance += 10;
                        }
                    }
                } else if (actual.isNumber()) {
                    // see if it's an enum value
                    QMetaEnum m;
                    if (argType.isMetaEnum()) {
                        m = meta->enumerator(argType.enumeratorIndex());
                    } else {
                        int mi = indexOfMetaEnum(meta, argType.name());
                        if (mi != -1)
                            m = meta->enumerator(mi);
                    }
                    if (m.isValid()) {
                        int ival = actual.toInt32();
                        if (m.valueToKey(ival) != 0) {
                            qVariantSetValue(v, ival);
                            converted = true;
                            matchDistance += 10;
                        }
                    }
                }
            } else {
                // determine how well the conversion matched
                if (actual.isNumber()) {
                    switch (tid) {
                    case QMetaType::Double:
                        // perfect
                        break;
                    case QMetaType::Float:
                        matchDistance += 1;
                        break;
                    case QMetaType::LongLong:
                    case QMetaType::ULongLong:
                        matchDistance += 2;
                        break;
                    case QMetaType::Long:
                    case QMetaType::ULong:
                        matchDistance += 3;
                        break;
                    case QMetaType::Int:
                    case QMetaType::UInt:
                        matchDistance += 4;
                        break;
                    case QMetaType::Short:
                    case QMetaType::UShort:
                        matchDistance += 5;
                        break;
                    case QMetaType::Char:
                    case QMetaType::UChar:
                        matchDistance += 6;
                        break;
                    default:
                        matchDistance += 10;
                        break;
                    }
                } else if (actual.isString()) {
                    switch (tid) {
                    case QMetaType::QString:
                        // perfect
                        break;
                    default:
                        matchDistance += 10;
                        break;
                    }
                } else if (actual.isBoolean()) {
                    switch (tid) {
                    case QMetaType::Bool:
                        // perfect
                        break;
                    default:
                        matchDistance += 10;
                        break;
                    }
                } else if (actual.isDate()) {
                    switch (tid) {
                    case QMetaType::QDateTime:
                        // perfect
                        break;
                    case QMetaType::QDate:
                        matchDistance += 1;
                        break;
                    case QMetaType::QTime:
                        matchDistance += 2;
                        break;
                    default:
                        matchDistance += 10;
                        break;
                    }
                } else if (actual.isRegExp()) {
                    switch (tid) {
                    case QMetaType::QRegExp:
                        // perfect
                        break;
                    default:
                        matchDistance += 10;
                        break;
                    }
                } else if (actual.isVariant()) {
                    if (actual.variantValue().userType()
                        || argType.isVariant()) {
                        // perfect
                    } else {
                        matchDistance += 10;
                    }
                } else if (actual.isQObject()) {
                    switch (tid) {
                    case QMetaType::QObjectStar:
                    case QMetaType::QWidgetStar:
                        // perfect
                        break;
                    default:
                        matchDistance += 10;
                        break;
                    }
                } else {
                    matchDistance += 10;
                }
            }

            if (converted)
                args[i+1] = v;
        }

        if (converted) {
            if ((context->argumentCount() == mtd.argumentCount())
                && (matchDistance == 0)) {
                // perfect match, use this one
                chosenMethod = mtd;
                chosenIndex = index;
                break;
            } else {
                candidates.append(QScriptMetaArguments(matchDistance, index, mtd, args));
            }
        }

        if (!m_maybeOverloaded)
            break;
    }

    if ((chosenIndex == -1) && candidates.isEmpty()) {
        // conversion failed, or incorrect number of arguments
        result = context->throwError(
            QScriptContext::SyntaxError,
            QString::fromUtf8("incorrect number or type of arguments in call to %0::%1()")
            .arg(QLatin1String(meta->className()))
            .arg(QLatin1String(funName)));
    } else {
        if (chosenIndex == -1) {
            // pick the best match
            int dist = INT_MAX;
            args.clear();
            for (int i = 0; i < candidates.size(); ++i) {
                QScriptMetaArguments cdt = candidates.at(i);
                if (cdt.method.fullyResolved()
                    && ((cdt.args.count() > args.count())
                        || ((cdt.args.count() == args.count())
                            && (cdt.matchDistance < dist)))) {
                    dist = cdt.matchDistance;
                    chosenMethod = cdt.method;
                    chosenIndex = cdt.index;
                    args = cdt.args;
                }
            }
        }

        if (chosenIndex != -1) {
            // call it
            QVarLengthArray<void*, 9> array(args.count());
            void **params = array.data();
            for (int i = 0; i < args.count(); ++i) {
                const QVariant &v = args[i];
                switch (chosenMethod.type(i).kind()) {
                case QScriptMetaType::Variant:
                    params[i] = const_cast<QVariant*>(&v);
                    break;
                case QScriptMetaType::MetaType:
                case QScriptMetaType::MetaEnum:
                    params[i] = const_cast<void*>(v.constData());
                    break;
                default:
                    Q_ASSERT(0);
                }
            }

            QScriptable *scriptable = scriptableFromQObject(thisQObject);
            QScriptEngine *oldEngine = 0;
            if (scriptable) {
                oldEngine = QScriptablePrivate::get(scriptable)->engine;
                QScriptablePrivate::get(scriptable)->engine = eng;
            }

            thisQObject->qt_metacall(QMetaObject::InvokeMetaMethod, chosenIndex, params);

            if (scriptable)
                QScriptablePrivate::get(scriptable)->engine = oldEngine;

            if (context->state() == QScriptContext::ExceptionState) {
                result = context->returnValue(); // propagate
            } else {
                QScriptMetaType retType = chosenMethod.returnType();
                if (retType.typeId() != 0) {
                    result = eng_p->create(retType.typeId(), params[0]);
                    if (!result.isValid())
                        result = eng_p->newVariant(QVariant(retType.typeId(), params[0]));
                } else if (retType.isVariant()) {
                    result = eng_p->newVariant(*(QVariant *)params[0]);
                } else {
                    result = eng_p->undefinedValue();
                }
            }
        } else {
            // one or more types are unresolved
            QScriptMetaArguments argsInstance = candidates.first();
            int unresolvedIndex = argsInstance.method.firstUnresolvedIndex();
            Q_ASSERT(unresolvedIndex != -1);
            QScriptMetaType unresolvedType = argsInstance.method.type(unresolvedIndex);
            result = context->throwError(
                QScriptContext::TypeError,
                QString::fromUtf8("cannot call %0::%1(): unknown type `%2'")
                .arg(QLatin1String(meta->className()))
                .arg(QString::fromLatin1(funName))
                .arg(QLatin1String(unresolvedType.name())));
        }
    }

    context->m_result = result;
}

bool QScript::QtFunction::createConnection(const QScriptValueImpl &self,
                                           const QScriptValueImpl &receiver,
                                           const QScriptValueImpl &slot)
{
    Q_ASSERT(slot.isFunction());

    const QMetaObject *meta = m_object->metaObject();
    int index = m_initialIndex;
    QMetaMethod method = meta->method(index);
    if (maybeOverloaded() && (method.attributes() & QMetaMethod::Cloned)) {
        // find the most general method
        do {
            method = meta->method(--index);
        } while (method.attributes() & QMetaMethod::Cloned);
    }

    QObject *conn = new ConnectionQObject(method, self, receiver, slot);
    m_connections.append(conn);
    return QMetaObject::connect(m_object, index, conn, conn->metaObject()->methodOffset());
}

bool QScript::QtFunction::destroyConnection(const QScriptValueImpl &,
                                            const QScriptValueImpl &receiver,
                                            const QScriptValueImpl &slot)
{
    Q_ASSERT(slot.isFunction());
    // find the connection with the given receiver+slot
    QObject *conn = 0;
    for (int i = 0; i < m_connections.count(); ++i) {
        ConnectionQObject *candidate = static_cast<ConnectionQObject*>((QObject*)m_connections.at(i));
        if (candidate->hasTarget(receiver, slot)) {
            conn = candidate;
            m_connections.removeAt(i);
            break;
        }
    }
    if (! conn)
        return false;

    const QMetaObject *meta = m_object->metaObject();
    int index = m_initialIndex;
    QMetaMethod method = meta->method(index);
    if (maybeOverloaded() && (method.attributes() & QMetaMethod::Cloned)) {
        // find the most general method
        do {
            method = meta->method(--index);
        } while (method.attributes() & QMetaMethod::Cloned);
    }

    bool ok = QMetaObject::disconnect(m_object, index, conn, conn->metaObject()->methodOffset());
    delete conn;
    return ok;
}

QScript::QtFunction::~QtFunction()
{
    qDeleteAll(m_connections);
}

/////////////////////////////////////////////////////////

namespace QScript
{

ExtQMetaObject::Instance *ExtQMetaObject::Instance::get(const QScriptValueImpl &object,
                                                        QScriptClassInfo *klass)
{
    if (! klass || klass == object.classInfo())
        return static_cast<Instance*> (object.objectData().data());

    return 0;
}

void QScript::ExtQMetaObject::Instance::execute(QScriptContextPrivate *context)
{
    if (ctor.isFunction()) {
        QScriptValueImplList args;
        for (int i = 0; i < context->argumentCount(); ++i)
            args << context->argument(i);
        QScriptValueImpl result = ctor.call(context->thisObject(), args);
        context->m_thisObject = result;
        context->m_result = result;
    } else {
        context->m_result = context->throwError(
            QScriptContext::TypeError,
            QString::fromUtf8("no constructor for %0")
            .arg(QLatin1String(value->className())));
    }
}

struct StaticQtMetaObject : public QObject
{
    static const QMetaObject *get()
        { return &static_cast<StaticQtMetaObject*> (0)->staticQtMetaObject; }
};

class ExtQMetaObjectData: public QScriptClassData
{
public:
    ExtQMetaObjectData(QScriptEnginePrivate *, QScriptClassInfo *classInfo);

    virtual bool resolve(const QScriptValueImpl &object, QScriptNameIdImpl *nameId,
                         QScript::Member *member, QScriptValueImpl *base);
    virtual bool get(const QScriptValueImpl &obj, const QScript::Member &member,
                     QScriptValueImpl *result);
    virtual void mark(const QScriptValueImpl &object, int generation);

private:
    QScriptClassInfo *m_classInfo;
};

ExtQMetaObjectData::ExtQMetaObjectData(QScriptEnginePrivate *,
                                       QScriptClassInfo *classInfo)
    : m_classInfo(classInfo)
{
}

bool ExtQMetaObjectData::resolve(const QScriptValueImpl &object,
                                 QScriptNameIdImpl *nameId,
                                 QScript::Member *member,
                                 QScriptValueImpl *base)
{
    const QMetaObject *meta = object.toQMetaObject();
    if (!meta)
        return false;

    QScriptEngine *eng = object.engine();

    QByteArray name = QScriptEnginePrivate::get(eng)->toString(nameId).toLatin1();

    for (int i = 0; i < meta->enumeratorCount(); ++i) {
        QMetaEnum e = meta->enumerator(i);

        for (int j = 0; j < e.keyCount(); ++j) {
            const char *key = e.key(j);

            if (! qstrcmp (key, name.constData())) {
                member->native(nameId, e.value(j), QScriptValue::ReadOnly);
                *base = object;
                return true;
            }
        }
    }

    return false;
}

bool ExtQMetaObjectData::get(const QScriptValueImpl &obj,
                             const QScript::Member &member,
                             QScriptValueImpl *result)
{
    if (! member.isNativeProperty())
        return false;

    *result = QScriptValueImpl(QScriptEnginePrivate::get(obj.engine()), member.id());
    return true;
}

void ExtQMetaObjectData::mark(const QScriptValueImpl &object, int generation)
{
    ExtQMetaObject::Instance *inst = ExtQMetaObject::Instance::get(object, m_classInfo);
    if (inst->ctor.isObject() || inst->ctor.isString())
        inst->ctor.mark(generation);
}

} // namespace QScript

QScript::ExtQMetaObject::ExtQMetaObject(QScriptEnginePrivate *eng,
                                        QScriptClassInfo *classInfo):
    Ecma::Core(eng), m_classInfo(classInfo)
{
    publicPrototype.invalidate();
    newQMetaObject(&publicPrototype, QScript::StaticQtMetaObject::get());

    eng->newConstructor(&ctor, this, publicPrototype);
    const QScriptValue::PropertyFlags flags = QScriptValue::SkipInEnumeration;
    publicPrototype.setProperty(QLatin1String("className"),
                                eng->createFunction(method_className, 0, m_classInfo), flags);

    QExplicitlySharedDataPointer<QScriptClassData> data(new QScript::ExtQMetaObjectData(eng, classInfo));
    m_classInfo->setData(data);
}

QScript::ExtQMetaObject::~ExtQMetaObject()
{
}

void QScript::ExtQMetaObject::execute(QScriptContextPrivate *context)
{
    QScriptValueImpl tmp;
    newQMetaObject(&tmp, 0);
    context->setReturnValue(tmp);
}

void QScript::ExtQMetaObject::newQMetaObject(QScriptValueImpl *result, const QMetaObject *value,
                                             const QScriptValueImpl &ctor)
{
    Instance *instance = new Instance();
    instance->value = value;
    instance->ctor = ctor;

    engine()->newObject(result, publicPrototype, classInfo());
    result->setObjectData(QExplicitlySharedDataPointer<QScriptObjectData>(instance));
}

QScriptValueImpl QScript::ExtQMetaObject::method_className(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *classInfo)
{
    if (Instance *instance = Instance::get(context->thisObject(), classInfo)) {
        return QScriptValueImpl(eng, QString::fromLatin1(instance->value->className()));
    }
    return eng->undefinedValue();
}

#include "qscriptextqobject.moc"

#endif // QT_NO_SCRIPT
