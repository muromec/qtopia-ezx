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

#include "qscriptengine_p.h"

#ifndef QT_NO_SCRIPT

#include "qscriptvalueimpl_p.h"
#include "qscriptcontext_p.h"
#include "qscriptmember_p.h"
#include "qscriptobject_p.h"
#include "qscriptlexer_p.h"
#include "qscriptnodepool_p.h"
#include "qscriptparser_p.h"
#include "qscriptcompiler_p.h"
#include "qscriptvalueiterator.h"
#include "qscriptecmaglobal_p.h"
#include "qscriptecmamath_p.h"
#include "qscriptecmaarray_p.h"
#include "qscriptextenumeration_p.h"

#include <QtCore/QDate>
#include <QtCore/QDateTime>
#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

#ifndef QT_NO_QOBJECT
#include "qscriptextensioninterface.h"
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>
#include <QtCore/QPluginLoader>
#endif

Q_DECLARE_METATYPE(QScriptValue)
Q_DECLARE_METATYPE(QVariant)
#ifndef QT_NO_QOBJECT
Q_DECLARE_METATYPE(QObjectList)
#endif
Q_DECLARE_METATYPE(QList<int>)

namespace QScript {

NodePool::~NodePool()
{
    qDeleteAll(m_codeCache);
    m_codeCache.clear();
}

Code *NodePool::createCompiledCode(AST::Node *node, CompilationUnit &compilation)
{
    QHash<AST::Node*, Code*>::const_iterator it = m_codeCache.constFind(node);
    if (it != m_codeCache.constEnd())
        return it.value();

    Code *code = new Code();
    code->init(compilation, this);

    m_codeCache.insert(node, code);
    return code;
}

class EvalFunction : public QScriptFunction
{
public:
    EvalFunction(QScriptEnginePrivate *)
    { length = 1; }

    virtual ~EvalFunction() {}

    void evaluate(QScriptContextPrivate *context, const QString &contents,
                  int lineNo, const QString &fileName, bool calledFromScript)
    {
        QScriptEngine *engine = context->engine();
        QScriptEnginePrivate *eng_p = QScriptEnginePrivate::get(engine);

        QExplicitlySharedDataPointer<NodePool> pool;
        pool = new NodePool(fileName);
        eng_p->setNodePool(pool);

        AST::Node *program = eng_p->createAbstractSyntaxTree(contents, lineNo);

        eng_p->setNodePool(0);

        if (! program) {
            context->errorLineNumber = lineNo;
            context->currentLine = lineNo;
            context->throwError(QScriptContext::SyntaxError, eng_p->errorMessage());
            return;
        }

        Compiler compiler(engine);
        compiler.setTopLevelCompiler(true);
        CompilationUnit compilation = compiler.compile(program);
        if (! compilation.isValid()) {
            context->throwError(compilation.errorMessage());
            return;
        }

        Code *code = pool->createCompiledCode(program, compilation);

        if (calledFromScript) {
            if (QScriptContext *parentContext = context->parentContext()) {
                QScriptContextPrivate *pc_p = QScriptContextPrivate::get(parentContext);
                context->setActivationObject(pc_p->activationObject());
                context->setThisObject(pc_p->thisObject());
            }
        }

        const QScriptInstruction *iPtr = context->instructionPointer();
        context->execute(code);
        context->setInstructionPointer(iPtr);
    }

    virtual void execute(QScriptContextPrivate *context)
    {
        QScriptEnginePrivate *eng = QScriptEnginePrivate::get(context->engine());
        int lineNo = context->currentLine;
        QString fileName; // don't set this for now, we don't want to change the official eval() for now.

        if (context->argumentCount() == 0) {
            context->setReturnValue(eng->undefinedValue());
        } else {
            QScriptValueImpl arg = context->argument(0);
            if (arg.isString()) {
                QString contents = arg.toString();
                evaluate(context, contents, lineNo, fileName, /*calledFromScript=*/true);
            } else {
                context->setReturnValue(arg);
            }
        }
    }
};

class WithClassData: public QScriptClassData
{

public:
    virtual bool resolve(const QScriptValueImpl &object, QScriptNameIdImpl *nameId,
                         QScript::Member *member, QScriptValueImpl *base);
};

bool WithClassData::resolve(const QScriptValueImpl &object, QScriptNameIdImpl *nameId,
                            QScript::Member *member, QScriptValueImpl *base)
{
    QScriptValueImpl proto = object.prototype();
    Q_ASSERT(proto.isValid());
    Q_ASSERT(proto.isObject());
    return proto.resolve(nameId, member, base, QScriptValue::ResolveScope);
}


class ArgumentsClassData: public QScriptClassData
{

public:

    static inline QScript::ArgumentsObjectData *get(const QScriptValueImpl &object)
        { return static_cast<QScript::ArgumentsObjectData*>(object.objectData().data()); }

    virtual bool resolve(const QScriptValueImpl &object, QScriptNameIdImpl *nameId,
                         QScript::Member *member, QScriptValueImpl *base);
    virtual bool get(const QScriptValueImpl &object, const QScript::Member &member,
                     QScriptValueImpl *out_value);
    virtual bool put(QScriptValueImpl *object, const QScript::Member &member,
                     const QScriptValueImpl &value);
    virtual void mark(const QScriptValueImpl &object, int generation);
};

bool ArgumentsClassData::resolve(const QScriptValueImpl &object, QScriptNameIdImpl *nameId,
                                 QScript::Member *member, QScriptValueImpl *base)
{
    QScriptEnginePrivate *eng_p = QScriptEnginePrivate::get(object.engine());

    if (nameId == eng_p->idTable()->id_length) {
        member->native(nameId, /*id=*/ 0,
                       QScriptValue::Undeletable
                       | QScriptValue::ReadOnly);
        *base = object;
        return true;
    } else if (nameId == eng_p->idTable()->id_callee) {
        member->native(nameId, /*id=*/ 0,
                       QScriptValue::Undeletable
                       | QScriptValue::ReadOnly);
        *base = object;
        return true;
    }

    QString propertyName = eng_p->toString(nameId);
    bool isNumber;
    quint32 index = propertyName.toUInt(&isNumber);
    if (isNumber) {
        QScript::ArgumentsObjectData *data = ArgumentsClassData::get(object);
        if (index < data->length) {
            member->native(/*nameId=*/0, index, 0);
            *base = object;
            return true;
        }
    }

    return false;
}

bool ArgumentsClassData::get(const QScriptValueImpl &object, const QScript::Member &member,
                             QScriptValueImpl *out_value)
{
    QScriptEnginePrivate *eng_p = QScriptEnginePrivate::get(object.engine());
    QScript::ArgumentsObjectData *data = ArgumentsClassData::get(object);
    if (member.nameId() == 0) {
        QScriptObject *activation_data = data->activation.objectValue();
        *out_value = activation_data->m_objects[member.id()];
        return true;
    } else if (member.nameId() == eng_p->idTable()->id_length) {
        eng_p->newNumber(out_value, data->length);
        return true;
    } else if (member.nameId() == eng_p->idTable()->id_callee) {
        *out_value = data->callee;
        return true;
    }
    return false;
}

bool ArgumentsClassData::put(QScriptValueImpl *object, const QScript::Member &member,
                             const QScriptValueImpl &value)
{
    Q_ASSERT(member.nameId() == 0);
    QScript::ArgumentsObjectData *data = ArgumentsClassData::get(*object);
    QScriptObject *activation_data = data->activation.objectValue();
    activation_data->m_objects[member.id()] = value;
    return true;
}

void ArgumentsClassData::mark(const QScriptValueImpl &object, int generation)
{
    QScript::ArgumentsObjectData *data = ArgumentsClassData::get(object);
    data->activation.mark(generation);
    data->callee.mark(generation);
}

} // namespace QScript

const qsreal QScriptEnginePrivate::D16 = 65536.0;
const qsreal QScriptEnginePrivate::D32 = 4294967296.0;

QScriptEnginePrivate::~QScriptEnginePrivate()
{
    // invalidate values that we have references to
    {
        QHash<QScriptObject*, QScriptValuePrivate*>::const_iterator it;
        for (it = m_objectHandles.constBegin(); it != m_objectHandles.constEnd(); ++it)
            (*it)->value.invalidate();
    }
    {
        QHash<QScriptNameIdImpl*, QScriptValuePrivate*>::const_iterator it;
        for (it = m_stringHandles.constBegin(); it != m_stringHandles.constEnd(); ++it)
            (*it)->value.invalidate();
    }
    {
        QVector<QScriptValuePrivate*>::const_iterator it;
        for (it = m_otherHandles.constBegin(); it != m_otherHandles.constEnd(); ++it)
            (*it)->value.invalidate();
    }

    delete[] m_string_hash_base;
    qDeleteAll(m_stringRepository);
    qDeleteAll(m_tempStringRepository);

    delete[] tempStackBegin;

#ifndef QT_NO_QOBJECT
# ifndef Q_SCRIPT_NO_QMETAOBJECT_CACHE
    qDeleteAll(m_cachedMetaObjects);
# endif
#endif

    qDeleteAll(m_allocated_classes);
}

QScript::AST::Node *QScriptEnginePrivate::changeAbstractSyntaxTree(QScript::AST::Node *prg)
{
    QScript::AST::Node *was = m_abstractSyntaxTree;
    m_abstractSyntaxTree = prg;
    return was;
}

QScript::AST::Node *QScriptEnginePrivate::createAbstractSyntaxTree(const QString &source, int &lineNumber)
{
    m_errorMessage.clear();

    QScript::Lexer lex(q_func());
    setLexer(&lex);
    lex.setCode(source, lineNumber);

    QScriptParser parser;

    if (! parser.parse(this)) {
        m_errorMessage = parser.errorMessage();
        lineNumber = parser.errorLineNumber();
        return 0;
    }

    return abstractSyntaxTree();
}

void QScriptEnginePrivate::markObject(const QScriptValueImpl &object, int generation)
{
    QScriptObject *instance = object.objectValue();
    QScript::GCBlock *block = QScript::GCBlock::get(instance);

    enum { MAX_GC_DEPTH = 32 };

    if (block->generation + 1 != generation)
        return;

    if (m_gc_depth >= MAX_GC_DEPTH) {
        // do the marking later
        m_markStack.append(object);
        return;
    }

    ++block->generation;
    ++m_gc_depth;

    if (QScriptClassData *data = object.m_class->data())
        data->mark(object, generation);

    if (instance->m_prototype.isObject())
        markObject(instance->m_prototype, generation);

    if (instance->m_scope.isObject())
        markObject(instance->m_scope, generation);

    const QScriptValueImpl &internalValue = instance->m_internalValue;

    if (internalValue.isValid()) {
        if (internalValue.isObject())
            markObject(internalValue, generation);

        else if (internalValue.isString())
            markString(internalValue.m_string_value, generation);
    }

    int garbage = 0;

    for (int i = 0; i < instance->memberCount(); ++i) {
        QScript::Member m;
        instance->member(i, &m);

        if (! m.isValid()) {
            ++garbage;
            continue;
        }

        Q_ASSERT(m.isObjectProperty());

        QScriptValueImpl child;
        instance->get(m, &child);

        if (m.nameId())
            markString(m.nameId(), generation);

        if (! child.isValid())
            continue;

        else if (child.isObject())
            markObject(child, generation);

        else if (child.isString())
            markString(child.m_string_value, generation);
    }

    --m_gc_depth;

    if (garbage < 128) // ###
        return;

    int j = 0;
    for (int i = 0; i < instance->memberCount(); ++i) {
        QScript::Member m;
        instance->member(i, &m);

        if (! m.isValid())
            continue;

        if (i != j) {
            instance->m_members[j].object(m.nameId(), j, m.flags());
            instance->m_objects[j] = instance->m_objects[i];
        }
        ++j;
    }
    //qDebug() << "==> old:" << instance->m_members.size() << "new:" << j;
    instance->m_members.resize(j);
    instance->m_objects.resize(j);
}

void QScriptEnginePrivate::markFrame(QScriptContextPrivate *context, int generation)
{
    QScriptValueImpl activation = context->activationObject();
    QScriptValueImpl thisObject = context->thisObject();
    QScriptValueImpl scopeChain = context->m_scopeChain;
    QScriptValueImpl callee = context->m_callee;
    QScriptValueImpl arguments = context->m_arguments;

    if (activation.isObject())
        markObject(activation, generation);

    if (scopeChain.isObject())
        markObject(scopeChain, generation);

    if (thisObject.isObject())
        markObject(thisObject, generation);

    if (callee.isObject())
        markObject(callee, generation);

    if (arguments.isObject())
        markObject(arguments, generation);

    if (context->returnValue().isValid()) {
        if (context->returnValue().isObject())
            markObject(context->returnValue(), generation);

        else if (context->returnValue().isString())
            markString(context->returnValue().m_string_value, generation);
    }

    if (context->baseStackPointer() != context->currentStackPointer()) {
        // mark the temp stack

        for (const QScriptValueImpl *it = context->baseStackPointer(); it != (context->currentStackPointer() + 1); ++it) {
            if (! it) {
                qWarning() << "no temp stack!!!";
                break;
            }

            else if (! it->isValid()) // ### assert?
                continue;

            else if (it->isObject())
                markObject(*it, generation);

            else if (it->isString())
                markString(it->m_string_value, generation);
        }
    }
}

void QScriptEnginePrivate::maybeGC_helper(bool do_string_gc)
{
    // qDebug() << "==>" << objectAllocator.newAllocatedBlocks() << "free:" << objectAllocator.freeBlocks();

    Q_ASSERT(objectAllocator.head() == QScript::GCBlock::get(m_globalObject.objectValue()));

    m_gc_depth = 0;

    int generation = objectAllocator.generation(m_globalObject.objectValue()) + 1;

    markObject(m_globalObject, generation);

    {
        QHash<QScriptObject*, QScriptValuePrivate*>::const_iterator it;
        for (it = m_objectHandles.constBegin(); it != m_objectHandles.constEnd(); ++it)
            markObject((*it)->value, generation);
    }

    {
        QHash<QScriptNameIdImpl*, QScriptValuePrivate*>::const_iterator it;
        for (it = m_stringHandles.constBegin(); it != m_stringHandles.constEnd(); ++it)
            markString((*it)->value.stringValue(), generation);
    }

#ifndef QT_NO_QOBJECT
# ifndef Q_SCRIPT_NO_QMETAOBJECT_CACHE
    {
        QHash<const QMetaObject*, QScriptMetaObject*>::const_iterator it;
        for (it = m_cachedMetaObjects.constBegin(); it != m_cachedMetaObjects.constEnd(); ++it) {
            {
                QList<QScriptNameIdImpl*> memberNames = (*it)->registeredMemberNames();
                QList<QScriptNameIdImpl*>::const_iterator it2;
                for (it2 = memberNames.constBegin(); it2 != memberNames.constEnd(); ++it2)
                    markString(*it2, generation);
            }
            {
                QList<QScriptValueImpl> propertyAccessors = (*it)->registeredPropertyAccessors();
                QList<QScriptValueImpl>::const_iterator it2;
                for (it2 = propertyAccessors.constBegin(); it2 != propertyAccessors.constEnd(); ++it2)
                    markObject(*it2, generation);
            }
        }
    }
# endif
#endif

    {
        QScriptContext *current = currentContext();
        while (current != 0) {
            markFrame (QScriptContextPrivate::get(current), generation);
            current = current->parentContext();
        }
    }

    {
        QHash<int, QScriptCustomTypeInfo>::const_iterator it;
        for (it = m_customTypes.constBegin(); it != m_customTypes.constEnd(); ++it)
            (*it).prototype.mark(generation);
    }

    // mark the objects we couldn't process due to recursion depth
    while (!m_markStack.isEmpty())
        markObject(m_markStack.takeLast(), generation);

    objectAllocator.sweep(generation);

    //qDebug() << "free blocks:" << objectAllocator.freeBlocks();

    Q_ASSERT(objectAllocator.head() == QScript::GCBlock::get(m_globalObject.objectValue()));

    if (! do_string_gc)
        return;

#if 0
    qDebug() << "do_string_gc:" << do_string_gc
        << ((m_stringRepository.size() - m_oldStringRepositorySize) > 256)
        << ((m_tempStringRepository.size() - m_oldTempStringRepositorySize) > 2048);
#endif

    QVector<QScriptNameIdImpl*> compressed;
    compressed.reserve(m_stringRepository.size());

    for (int i = 0; i < m_stringRepository.size(); ++i) {
        QScriptNameIdImpl *entry = m_stringRepository.at(i);

        if (entry->used || entry->persistent) {
            compressed.append(entry);
            entry->used = false;
        }

        else {
            //qDebug() << "deleted unique:" << entry->s;
            delete entry;
      }
    }

    // qDebug() << "before:" << m_stringRepository.size() << "after:" << compressed.size() << globalObject.objectValue()->m_members.size();
    m_stringRepository = compressed;
    rehashStringRepository(/*resize=*/ false);
    m_oldStringRepositorySize = m_stringRepository.size();


    compressed.clear();
    for (int i = 0; i < m_tempStringRepository.size(); ++i) {
        QScriptNameIdImpl *entry = m_tempStringRepository.at(i);

        if (entry->used || entry->persistent) {
            compressed.append(entry);
            entry->used = false;
        }

        else {
          //qDebug() << "deleted:" << entry->s;
            delete entry;
      }
    }

    //qDebug() << "before:" << m_tempStringRepository.size() << "after:" << compressed.size();

    m_tempStringRepository = compressed;
    m_oldTempStringRepositorySize = m_tempStringRepository.size();
}

void QScriptEnginePrivate::evaluate(QScriptContextPrivate *context, const QString &contents, int lineNumber, const QString &fileName)
{
    // ### try to remove cast
    QScript::EvalFunction *evalFunction = static_cast<QScript::EvalFunction*>(m_evalFunction);
    evalFunction->evaluate(context, contents, lineNumber, fileName, /*calledFromScript=*/ false);
}

qsreal QScriptEnginePrivate::convertToNativeDouble_helper(const QScriptValueImpl &object)
{
    QScriptClassInfo *klass = object.m_class;
    Q_ASSERT(klass != 0);

    switch (klass->type()) {

    case QScript::UndefinedType:
    case QScript::PointerType:
    case QScript::FunctionType:
        break;

    case QScript::NullType:
        return 0;

    case QScript::BooleanType:
        return object.m_bool_value;

    case QScript::IntegerType:
        return object.m_int_value;

    case QScript::NumberType:
        return object.m_number_value;

    case QScript::StringType:
        return toNumber(toString(object.m_string_value));

    default: {
        QScriptValueImpl p = toPrimitive(object, QScriptValueImpl::NumberTypeHint);
        if (! p.isValid() || p.isObject())
            break;

        return convertToNativeDouble(p);
    } // default


    } // switch

    return SNaN();
}

bool QScriptEnginePrivate::convertToNativeBoolean_helper(const QScriptValueImpl &object)
{
    Q_ASSERT (object.isValid());

    QScriptClassInfo *klass = object.m_class;
    Q_ASSERT(klass != 0);

    switch (klass->type()) {

    case QScript::UndefinedType:
    case QScript::PointerType:
    case QScript::NullType:
        return false;

    case QScript::FunctionType:
        return true;

    case QScript::BooleanType:
        return object.m_bool_value;

    case QScript::IntegerType:
        return object.m_int_value != 0;

    case QScript::NumberType:
        return object.m_number_value != 0 && !isNaN(object.m_number_value);

    case QScript::StringType:
        return toString(object.m_string_value).length() != 0;

    case QScript::VariantType: {
        QScriptValueImpl p = toPrimitive(object, QScriptValueImpl::NumberTypeHint);
        if (! p.isValid() || p.isObject())
            break;

        return convertToNativeBoolean(p);
    }

    default:
        if (object.isObject())
            return true;

        break;
    } // switch

    return false;
}

QString QScriptEnginePrivate::convertToNativeString_helper(const QScriptValueImpl &object)
{
    QScriptClassInfo *klass = object.m_class;
    Q_ASSERT(klass != 0);

    const QScript::IdTable *ids = idTable();

    switch (klass->type()) {

    case QScript::UndefinedType:
        return toString(ids->id_undefined);

    case QScript::NullType:
        return toString(ids->id_null);

    case QScript::BooleanType:
        return toString(object.m_bool_value ? ids->id_true : ids->id_false);

    case QScript::IntegerType:
        return QString::number(object.m_int_value);

    case QScript::NumberType:
        return toString(object.m_number_value);

    case QScript::PointerType:
        return toString(ids->id_pointer);

    case QScript::StringType:
        return toString(object.m_string_value);

    default: {
        QScriptValueImpl p = toPrimitive(object, QScriptValueImpl::StringTypeHint);

        if (! p.isValid() || p.isObject())
            return klass->name();

        return convertToNativeString(p);
    } // default

    } // switch

    return toString(ids->id_undefined);
}

QScriptValueImpl QScriptEnginePrivate::toObject_helper(const QScriptValueImpl &value)
{
    QScriptValueImpl result;
    switch (value.type()) {
    case QScript::UndefinedType:
    case QScript::NullType:
        break;

    case QScript::BooleanType:
        booleanConstructor->newBoolean(&result, value.m_bool_value);
        break;

    case QScript::NumberType:
        numberConstructor->newNumber(&result, value.m_number_value);
        break;

    case QScript::StringType:
        stringConstructor->newString(&result, value.m_string_value->s);
        break;

    default:
        break;
    } // switch

    return result;
}

// [[defaultValue]]
QScriptValueImpl QScriptEnginePrivate::toPrimitive_helper(const QScriptValueImpl &object,
                                                          QScriptValueImpl::TypeHint hint)
{
    QScriptNameIdImpl *functionIds[2];

    if ((hint == QScriptValueImpl::NumberTypeHint)
        || (hint == QScriptValueImpl::NoTypeHint
            && object.m_class != dateConstructor->classInfo())) { // ### date?
        functionIds[0] = idTable()->id_valueOf;
        functionIds[1] = idTable()->id_toString;
    } else {
        functionIds[0] = idTable()->id_toString;
        functionIds[1] = idTable()->id_valueOf;
    }

    for (int i = 0; i < 2; ++i) {
        QScriptValueImpl base;
        QScript::Member member;

        if (! object.resolve(functionIds[i], &member, &base, QScriptValue::ResolvePrototype))
            return object;

        QScriptValueImpl f_valueOf;
        base.get(member, &f_valueOf);

        if (QScriptFunction *foo = convertToNativeFunction(f_valueOf)) {
            QScriptContextPrivate *me = QScriptContextPrivate::get(pushContext());
            QScriptValueImpl activation;
            newActivation(&activation);
            activation.setScope(m_globalObject);
            me->setActivationObject(activation);
            me->setThisObject(object);
            foo->execute(me);
            QScriptValueImpl result = me->returnValue();
            bool exception = (me->state() == QScriptContext::ExceptionState);
            popContext();
            if (exception || (result.isValid() && !result.isObject()))
                return result;
        }
    }

    return object;
}

void QScriptEnginePrivate::rehashStringRepository(bool resize)
{
    if (resize) {
        delete[] m_string_hash_base;
        m_string_hash_size <<= 1; // ### use primes

        m_string_hash_base = new QScriptNameIdImpl* [m_string_hash_size];
    }

    memset(m_string_hash_base, 0, sizeof(QScriptNameIdImpl*) * m_string_hash_size);

    for (int index = 0; index < m_stringRepository.size(); ++index) {
        QScriptNameIdImpl *entry = m_stringRepository.at(index);
        uint h = _q_scriptHash(entry->s) % m_string_hash_size;
        entry->h = h;
        entry->next = m_string_hash_base[h];
        m_string_hash_base[h] = entry;
    }
}

QScriptNameIdImpl *QScriptEnginePrivate::insertStringEntry(const QString &s)
{
    QScriptNameIdImpl *entry = new QScriptNameIdImpl(s);
    entry->unique = true;
    m_stringRepository.append(entry);

    uint h = _q_scriptHash(s) % m_string_hash_size;
    entry->h = h;
    entry->next = m_string_hash_base[h];
    m_string_hash_base[h] = entry;

    if (m_stringRepository.count() == m_string_hash_size)
        rehashStringRepository();

    return entry;
}

extern double qstrtod(const char *s00, const char **se, bool *ok);
extern char *qdtoa(double d, int mode, int ndigits, int *decpt, int *sign, char **rve, char **digits_str);

QString QScriptEnginePrivate::toString_helper(qsreal d)
{
    QByteArray buf;
    buf.reserve(80);

    int decpt;
    int sign;
    char *result = 0;
    (void) qdtoa(d, 0, 0, &decpt, &sign, 0, &result);

    if (! result)
        return QString();

    else if (decpt <= 0 && decpt > -6) {

        buf.fill('0', -decpt + 2 + sign);

        if (sign) // fix the sign.
            buf[0] = '-';

        buf[sign + 1] = '.';
        buf += result;
    }

    else {
        if (sign)
            buf += '-';

        buf += result;
        int length = buf.length() - sign;

        if (decpt <= 21 && decpt > 0) {
            if (length <= decpt)
                buf += QByteArray().fill('0', decpt - length);
            else
                buf.insert(decpt + sign, '.');
        }

        else if (result[0] >= '0' && result[0] <= '9') {
            if (length > 1)
                buf.insert(1, '.');

            buf += 'e';
            buf += (decpt >= 0) ? '+' : '-';

            int e = decpt - 1;

            if (e < 0)
                e = -e;

            if (e >= 100)
                buf += '0' + e / 100;

            if (e >= 10)
                buf += '0' + (e % 100) / 10;

            buf += '0' + e % 10;
        }
    }

    free(result);

    return QString::fromLatin1(buf);
}

QScriptValueImpl QScriptEnginePrivate::call(const QScriptValueImpl &callee,
                                        const QScriptValueImpl &thisObject,
                                        const QScriptValueImplList &args,
                                        bool asConstructor)
{
    Q_Q(QScriptEngine);
    QScriptFunction *function = callee.toFunction();
    Q_ASSERT(function);

    if (++m_callDepth == m_maxCallDepth) {
        QScriptContextPrivate *ctx_p = QScriptContextPrivate::get(currentContext());
        return ctx_p->throwError(QLatin1String("call stack overflow"));
    }

    QScriptContext *nested_frame = pushContext();
    QScriptContextPrivate *nested = QScriptContextPrivate::get(nested_frame);
    // set up the temp stack
    if (! nested->tempStack)
        nested->stackPtr = nested->tempStack = tempStackBegin;

    newActivation(&nested->m_activation);
    if (callee.m_object_value->m_scope.isValid())
        nested->m_activation.m_object_value->m_scope = callee.m_object_value->m_scope;
    else
        nested->m_activation.m_object_value->m_scope = m_globalObject;

    QScriptObject *activation_data = nested->m_activation.m_object_value;

    QScriptValueImpl undefined;
    newUndefined(&undefined);

    int formalCount = function->formals.count();
    int argc = args.count();
    int mx = qMax(formalCount, argc);
    activation_data->m_members.resize(mx);
    activation_data->m_objects.resize(mx);
    for (int i = 0; i < mx; ++i) {
        QScriptNameIdImpl *nameId = 0;
        if (i < formalCount)
            nameId = function->formals.at(i);

        activation_data->m_members[i].object(nameId, i, QScriptValue::SkipInEnumeration);
        QScriptValueImpl arg = (i < argc) ? args.at(i) : undefined;
        if (arg.isValid() && (arg.engine() != q)) {
            qWarning("QScriptValue::call() failed: "
                     "cannot call function with argument created in "
                     "a different engine");
            popContext();
            return QScriptValueImpl();
        }
        activation_data->m_objects[i] = arg.isValid() ? arg : undefined;
    }

    nested->argc = argc;
    QVector<QScriptValueImpl> argsv = args.toVector();
    nested->args = const_cast<QScriptValueImpl*> (argsv.constData());

    if (thisObject.isObject())
        nested->m_thisObject = thisObject;
    else
        nested->m_thisObject = m_globalObject;
    nested->m_callee = callee;
    nested->m_calledAsConstructor = asConstructor;

    newUndefined(&nested->m_result);
    function->execute(nested);
    --m_callDepth;
    QScriptValueImpl result = nested->m_result;
    nested->args = 0;
    popContext();

    return result;
}

QScriptValueImpl QScriptEnginePrivate::call(const QScriptValueImpl &callee,
                                        const QScriptValueImpl &thisObject,
                                        const QScriptValueImpl &args,
                                        bool asConstructor)
{
    QScriptValueImplList argsList;
    if (QScript::Ecma::Array::Instance *arr = arrayConstructor->get(args)) {
        QScript::Array actuals = arr->value;
        for (quint32 i = 0; i < actuals.count(); ++i) {
            QScriptValueImpl a = actuals.at(i);
            if (! a.isValid())
                argsList << undefinedValue();
            else
                argsList << a;
        }
    } else if (args.classInfo() == m_class_arguments) {
        QScript::ArgumentsObjectData *arguments;
        arguments = static_cast<QScript::ArgumentsObjectData*> (args.objectData().data());
        QScriptObject *activation = arguments->activation.objectValue();
        for (uint i = 0; i < arguments->length; ++i)
            argsList << activation->m_objects[i];
    } else if (!(args.isUndefined() || args.isNull())) {
        return QScriptContextPrivate::get(currentContext())->throwError(
            QScriptContext::TypeError,
            QLatin1String("QScriptValue::call(): arguments must be an array"));
    }
    return call(callee, thisObject, argsList, asConstructor);
}

QScriptValueImpl QScriptEnginePrivate::arrayFromStringList(const QStringList &lst)
{
    QScriptValueImpl arr = newArray(lst.size());
    for (int i = 0; i < lst.size(); ++i)
        arr.setProperty(i, QScriptValueImpl(this, lst.at(i)));
    return arr;
}

QStringList QScriptEnginePrivate::stringListFromArray(const QScriptValueImpl &arr)
{
    QStringList lst;
    uint len = arr.property(QLatin1String("length")).toUInt32();
    for (uint i = 0; i < len; ++i)
        lst.append(arr.property(i).toString());
    return lst;
}

QScriptValueImpl QScriptEnginePrivate::arrayFromVariantList(const QVariantList &lst)
{
    QScriptValueImpl arr = newArray(lst.size());
    for (int i = 0; i < lst.size(); ++i)
        arr.setProperty(i, valueFromVariant(lst.at(i)));
    return arr;
}

QVariantList QScriptEnginePrivate::variantListFromArray(const QScriptValueImpl &arr)
{
    QVariantList lst;
    uint len = arr.property(QLatin1String("length")).toUInt32();
    for (uint i = 0; i < len; ++i)
        lst.append(arr.property(i).toVariant());
    return lst;
}

QScriptValueImpl QScriptEnginePrivate::objectFromVariantMap(const QVariantMap &vmap)
{
    QScriptValueImpl obj = newObject();
    QVariantMap::const_iterator it;
    for (it = vmap.constBegin(); it != vmap.constEnd(); ++it)
        obj.setProperty(it.key(), valueFromVariant(it.value()));
    return obj;
}

QVariantMap QScriptEnginePrivate::variantMapFromObject(const QScriptValueImpl &obj)
{
    QVariantMap vmap;
    QScriptValueIterator it(obj);
    while (it.hasNext()) {
        it.next();
        vmap.insert(it.name(), it.value().toVariant());
    }
    return vmap;
}

QScriptValueImpl QScriptEnginePrivate::create(int type, const void *ptr)
{
    Q_Q(QScriptEngine);
    Q_ASSERT(ptr);
    QScriptValueImpl result;
    QScriptCustomTypeInfo info = m_customTypes.value(type);
    if (info.marshal) {
        result = QScriptValuePrivate::valueOf(info.marshal(q, ptr));
    } else {
        // check if it's one of the types we know
        switch (QMetaType::Type(type)) {
        case QMetaType::Bool:
            result = QScriptValueImpl(this, *reinterpret_cast<const bool*>(ptr));
            break;
        case QMetaType::Int:
            result = QScriptValueImpl(this, *reinterpret_cast<const int*>(ptr));
            break;
        case QMetaType::UInt:
            result = QScriptValueImpl(this, *reinterpret_cast<const uint*>(ptr));
            break;
        case QMetaType::LongLong:
            result = QScriptValueImpl(this, qsreal(*reinterpret_cast<const qlonglong*>(ptr)));
            break;
        case QMetaType::ULongLong:
#if defined(Q_OS_WIN) && defined(_MSC_FULL_VER) && _MSC_FULL_VER <= 12008804
#pragma message("** NOTE: You need the Visual Studio Processor Pack to compile support for 64bit unsigned integers.")
            result = QScriptValueImpl(this, qsreal((qlonglong)*reinterpret_cast<const qulonglong*>(ptr)));
#elif defined(Q_CC_MSVC) && !defined(Q_CC_MSVC_NET)
            result = QScriptValueImpl(this, qsreal((qlonglong)*reinterpret_cast<const qulonglong*>(ptr)));
#else
            result = QScriptValueImpl(this, qsreal(*reinterpret_cast<const qulonglong*>(ptr)));
#endif
            break;
        case QMetaType::Double:
            result = QScriptValueImpl(this, *reinterpret_cast<const double*>(ptr));
            break;
        case QMetaType::QString:
            result = QScriptValueImpl(this, *reinterpret_cast<const QString*>(ptr));
            break;
        case QMetaType::Float:
            result = QScriptValueImpl(this, *reinterpret_cast<const float*>(ptr));
            break;
        case QMetaType::Short:
            result = QScriptValueImpl(this, *reinterpret_cast<const short*>(ptr));
            break;
        case QMetaType::UShort:
            result = QScriptValueImpl(this, *reinterpret_cast<const unsigned short*>(ptr));
            break;
        case QMetaType::Char:
            result = QScriptValueImpl(this, *reinterpret_cast<const char*>(ptr));
            break;
        case QMetaType::UChar:
            result = QScriptValueImpl(this, *reinterpret_cast<const unsigned char*>(ptr));
            break;
        case QMetaType::QChar:
            result = QScriptValueImpl(this, (*reinterpret_cast<const QChar*>(ptr)).unicode());
            break;
        case QMetaType::QStringList:
            result = arrayFromStringList(*reinterpret_cast<const QStringList *>(ptr));
            break;
        case QMetaType::QVariantList:
            result = arrayFromVariantList(*reinterpret_cast<const QVariantList *>(ptr));
            break;
        case QMetaType::QVariantMap:
            result = objectFromVariantMap(*reinterpret_cast<const QVariantMap *>(ptr));
            break;
        case QMetaType::QDateTime: {
            QDateTime dateTime = *reinterpret_cast<const QDateTime *>(ptr);
            dateConstructor->newDate(&result, dateTime);
        } break;
        case QMetaType::QDate: {
            QDate date = *reinterpret_cast<const QDate *>(ptr);
            dateConstructor->newDate(&result, date);
        } break;
#ifndef QT_NO_REGEXP
        case QMetaType::QRegExp: {
            QRegExp rx = *reinterpret_cast<const QRegExp *>(ptr);
            regexpConstructor->newRegExp(&result, rx);
        } break;
#endif
#ifndef QT_NO_QOBJECT
        case QMetaType::QObjectStar:
        case QMetaType::QWidgetStar:
            result = newQObject(*reinterpret_cast<QObject* const *>(ptr));
            break;
#endif
        default:
            if (type == qMetaTypeId<QScriptValue>())
                result = QScriptValuePrivate::valueOf(*reinterpret_cast<const QScriptValue*>(ptr));
            else if (type == qMetaTypeId<QVariant>())
                result = newVariant(*reinterpret_cast<const QVariant*>(ptr));

#ifndef QT_NO_QOBJECT
            // lazy registration of some common list types
            else if (type == qMetaTypeId<QObjectList>()) {
                qScriptRegisterSequenceMetaType<QObjectList>(q);
                return create(type, ptr);
            }
#endif
            else if (type == qMetaTypeId<QList<int> >()) {
                qScriptRegisterSequenceMetaType<QList<int> >(q);
                return create(type, ptr);
            }

            else {
                QByteArray typeName = QMetaType::typeName(type);
                if (typeName.endsWith('*') && !*reinterpret_cast<void* const *>(ptr))
                    result = nullValue();
                else
                    result = newVariant(QVariant(type, ptr));
            }
        }
    }
    if (result.isObject() && info.prototype.isValid())
        result.setPrototype(info.prototype);
    return result;
}

bool QScriptEnginePrivate::convert(const QScriptValueImpl &value,
                                   int type, void *ptr)
{
    Q_Q(QScriptEngine);
    QScriptCustomTypeInfo info = m_customTypes.value(type);
    if (info.demarshal) {
        info.demarshal(value, ptr);
        return true;
    }

    // check if it's one of the types we know
    switch (QMetaType::Type(type)) {
    case QMetaType::Bool:
        *reinterpret_cast<bool*>(ptr) = value.toBoolean();
        return true;
    case QMetaType::Int:
        *reinterpret_cast<int*>(ptr) = value.toInt32();
        return true;
    case QMetaType::UInt:
        *reinterpret_cast<uint*>(ptr) = value.toUInt32();
        return true;
    case QMetaType::LongLong:
        *reinterpret_cast<qlonglong*>(ptr) = qlonglong(value.toInteger());
        return true;
    case QMetaType::ULongLong:
        *reinterpret_cast<qulonglong*>(ptr) = qulonglong(value.toInteger());
        return true;
    case QMetaType::Double:
        *reinterpret_cast<double*>(ptr) = value.toNumber();
        return true;
    case QMetaType::QString:
        *reinterpret_cast<QString*>(ptr) = value.toString();
        return true;
    case QMetaType::Float:
        *reinterpret_cast<float*>(ptr) = value.toNumber();
        return true;
    case QMetaType::Short:
        *reinterpret_cast<short*>(ptr) = short(value.toInt32());
        return true;
    case QMetaType::UShort:
        *reinterpret_cast<unsigned short*>(ptr) = value.toUInt16();
        return true;
    case QMetaType::Char:
        *reinterpret_cast<char*>(ptr) = char(value.toInt32());
        return true;
    case QMetaType::UChar:
        *reinterpret_cast<unsigned char*>(ptr) = (unsigned char)(value.toInt32());
        return true;
    case QMetaType::QChar:
        if (value.isString()) {
            QString str = value.toString();
            *reinterpret_cast<QChar*>(ptr) = str.isEmpty() ? QChar() : str.at(0);
        } else {
            *reinterpret_cast<QChar*>(ptr) = QChar(value.toUInt16());
        }
        return true;
    case QMetaType::QDateTime:
        if (value.isDate()) {
            *reinterpret_cast<QDateTime *>(ptr) = value.toDateTime();
            return true;
        } break;
    case QMetaType::QDate:
        if (value.isDate()) {
            *reinterpret_cast<QDate *>(ptr) = value.toDateTime().date();
            return true;
        } break;
#ifndef QT_NO_REGEXP
    case QMetaType::QRegExp:
        if (value.isRegExp()) {
            *reinterpret_cast<QRegExp *>(ptr) = value.toRegExp();
            return true;
        } break;
#endif
#ifndef QT_NO_QOBJECT
    case QMetaType::QObjectStar:
    case QMetaType::QWidgetStar:
        if (value.isQObject() || value.isNull()) {
            *reinterpret_cast<QObject* *>(ptr) = value.toQObject();
            return true;
        } break;
#endif
    case QMetaType::QStringList:
        if (value.isArray()) {
            *reinterpret_cast<QStringList *>(ptr) = stringListFromArray(value);
            return true;
        } break;
    case QMetaType::QVariantList:
        if (value.isArray()) {
            *reinterpret_cast<QVariantList *>(ptr) = variantListFromArray(value);
            return true;
        } break;
    case QMetaType::QVariantMap:
        if (value.isObject()) {
            *reinterpret_cast<QVariantMap *>(ptr) = variantMapFromObject(value);
            return true;
        } break;
    default:
    ;
    }

    QByteArray name = QMetaType::typeName(type);
#ifndef QT_NO_QOBJECT
    if (value.isQObject() && name.endsWith('*')) {
        QByteArray className = name.left(name.size()-1);
        QObject *qobject = value.toQObject();
        if (void *instance = qobject->qt_metacast(className)) {
            *reinterpret_cast<void* *>(ptr) = instance;
            return true;
        }
    }
#endif
    if (value.isVariant() && name.endsWith('*')) {
        int valueType = QMetaType::type(name.left(name.size()-1));
        QVariant &var = value.variantValue();
        if (valueType == var.userType()) {
            *reinterpret_cast<void* *>(ptr) = var.data();
            return true;
        } else {
            // look in the prototype chain
            QScriptValueImpl proto = value.prototype();
            while (proto.isObject() && proto.isVariant()) {
                if ((type == proto.variantValue().userType())
                    || (valueType && (valueType == proto.variantValue().userType()))) {
                    QByteArray varTypeName = QMetaType::typeName(var.userType());
                    if (varTypeName.endsWith('*'))
                        *reinterpret_cast<void* *>(ptr) = *reinterpret_cast<void* *>(var.data());
                    else
                        *reinterpret_cast<void* *>(ptr) = var.data();
                    return true;
                }
                proto = proto.prototype();
            }
        }
    } else if (type == qMetaTypeId<QScriptValue>()) {
        *reinterpret_cast<QScriptValue*>(ptr) = value;
        return true;
    } else if (type == qMetaTypeId<QVariant>()) {
        *reinterpret_cast<QVariant*>(ptr) = value.toVariant();
        return true;
    }

    // lazy registration of some common list types
#ifndef QT_NO_QOBJECT
    else if (type == qMetaTypeId<QObjectList>()) {
        qScriptRegisterSequenceMetaType<QObjectList>(q);
        return convert(value, type, ptr);
    }
#endif
    else if (type == qMetaTypeId<QList<int> >()) {
        qScriptRegisterSequenceMetaType<QList<int> >(q);
        return convert(value, type, ptr);
    }

#if 0
    if (!name.isEmpty()) {
        qWarning("QScriptEngine::convert: unable to convert value to type `%s'",
                 name.constData());
    }
#endif
    return false;
}

QScriptValuePrivate *QScriptEnginePrivate::registerValue(const QScriptValueImpl &value)
{
    if (value.isString()) {
        QScriptNameIdImpl *id = value.stringValue();
        QScriptValuePrivate *p = m_stringHandles.value(id);
        if (p)
            return p;
        p = m_handleRepository.get();
        p->value = value;
        m_stringHandles.insert(id, p);
        return p;
    } else if (value.isObject()) {
        QScriptObject *instance = value.objectValue();
        QScriptValuePrivate *p = m_objectHandles.value(instance);
        if (p)
            return p;
        p = m_handleRepository.get();
        p->value = value;
        m_objectHandles.insert(instance, p);
        return p;
    }

    QVector<QScriptValuePrivate*>::const_iterator it;
    for (it = m_otherHandles.constBegin(); it != m_otherHandles.constEnd(); ++it) {
        if ((*it)->value.strictlyEquals(value))
            return *it;
    }

    QScriptValuePrivate *p = m_handleRepository.get();
    p->value = value;
    m_otherHandles.append(p);
    return p;
}

void QScriptEnginePrivate::init()
{
    qMetaTypeId<QScriptValue>();

    m_callDepth = 0;
#if defined(Q_OS_WIN)
    m_maxCallDepth = 88;
#elif defined(Q_OS_MAC)
    m_maxCallDepth = 640;
#elif defined(QT_ARCH_ARM)
    m_maxCallDepth = 360;
#else
    m_maxCallDepth = 768;
#endif
    m_oldStringRepositorySize = 0;
    m_oldTempStringRepositorySize = 0;
    m_context = 0;
    m_abstractSyntaxTree = 0;
    m_lexer = 0;

    objectConstructor = 0;
    numberConstructor = 0;
    booleanConstructor = 0;
    stringConstructor = 0;
    dateConstructor = 0;
    functionConstructor = 0;
    arrayConstructor = 0;
    regexpConstructor = 0;
    errorConstructor = 0;
    enumerationConstructor = 0;
    variantConstructor = 0;
    qobjectConstructor = 0;
    qmetaObjectConstructor = 0;

    m_processEventsInterval = -1;
    m_nextProcessEvents = 0;
    m_processEventIncr = 0;

    m_stringRepository.reserve(DefaultHashSize);
    m_string_hash_size = DefaultHashSize;
    m_string_hash_base = new QScriptNameIdImpl* [m_string_hash_size];
    memset(m_string_hash_base, 0, sizeof(QScriptNameIdImpl*) * m_string_hash_size);

    m_class_prev_id = QScript::CustomType;
    m_class_activation = registerClass(QLatin1String("activation"), QScript::ActivationType);
    m_class_boolean = registerClass(QLatin1String("boolean"), QScript::BooleanType);
    m_class_double = registerClass(QLatin1String("qsreal"), QScript::NumberType);
    m_class_function = registerClass(QLatin1String("Function"), QScript::FunctionType);
    m_class_int = registerClass(QLatin1String("integer"), QScript::IntegerType);
    m_class_null = registerClass(QLatin1String("null"), QScript::NullType);
    m_class_object = registerClass(QLatin1String("Object"), QScript::ObjectType);
    m_class_pointer = registerClass(QLatin1String("pointer"), QScript::PointerType);
    m_class_reference = registerClass(QLatin1String("reference"), QScript::ReferenceType);
    m_class_string = registerClass(QLatin1String("string"), QScript::StringType);
    m_class_undefined = registerClass(QLatin1String("undefined"), QScript::UndefinedType);
    m_class_variant = registerClass(QLatin1String("variant"), QScript::VariantType);
    m_class_qobject = registerClass(QLatin1String("qobject"), QScript::QObjectType);
    m_class_qmetaobject = registerClass(QLatin1String("qmetaobject"), QScript::QMetaObjectType);

    m_class_arguments = registerClass(QLatin1String("arguments"), QScript::ObjectType);
    QExplicitlySharedDataPointer<QScriptClassData> data2(new QScript::ArgumentsClassData());
    m_class_arguments->setData(data2);

    m_class_with = registerClass(QLatin1String("__qscript_internal_with"), QScript::ObjectType);
    QExplicitlySharedDataPointer<QScriptClassData> data3(new QScript::WithClassData());
    m_class_with->setData(data3);

    // public name ids
    m_id_table.id_constructor = nameId(QLatin1String("constructor"), true);
    m_id_table.id_false       = nameId(QLatin1String("false"), true);
    m_id_table.id_null        = nameId(QLatin1String("null"), true);
    m_id_table.id_object      = nameId(QLatin1String("object"), true);
    m_id_table.id_pointer     = nameId(QLatin1String("pointer"), true);
    m_id_table.id_prototype   = nameId(QLatin1String("prototype"), true);
    m_id_table.id_arguments   = nameId(QLatin1String("arguments"), true);
    m_id_table.id_this        = nameId(QLatin1String("this"), true);
    m_id_table.id_toString    = nameId(QLatin1String("toString"), true);
    m_id_table.id_true        = nameId(QLatin1String("true"), true);
    m_id_table.id_undefined   = nameId(QLatin1String("undefined"), true);
    m_id_table.id_valueOf     = nameId(QLatin1String("valueOf"), true);
    m_id_table.id_length      = nameId(QLatin1String("length"), true);
    m_id_table.id_callee      = nameId(QLatin1String("callee"), true);
    m_id_table.id___proto__   = nameId(QLatin1String("__proto__"), true);
    m_id_table.id___fileName__ = nameId(QLatin1String("__fileName__"), true);
    m_id_table.id___qt_sender__  = nameId(QLatin1String("__qt_sender__"), true);

    const int TEMP_STACK_SIZE = 10 * 1024;
    tempStackBegin = new QScriptValueImpl[TEMP_STACK_SIZE];
    tempStackEnd = tempStackBegin + TEMP_STACK_SIZE;
    newUndefined(&tempStackBegin[0]);

    objectAllocator.blockGC(true);

    // GC requires that GlobalObject is the first object created
    QScript::Ecma::Global::construct(&m_globalObject, this);

    // create the prototypes first...
    objectConstructor = new QScript::Ecma::Object(this, m_class_object);
    functionConstructor = new QScript::Ecma::Function(this, m_class_function);
    // ... then we can initialize
    functionConstructor->initialize();
    objectConstructor->initialize();

    numberConstructor = new QScript::Ecma::Number(this);
    booleanConstructor = new QScript::Ecma::Boolean(this);
    stringConstructor = new QScript::Ecma::String(this);
    dateConstructor = new QScript::Ecma::Date(this);
    arrayConstructor = new QScript::Ecma::Array(this);
    regexpConstructor = new QScript::Ecma::RegExp(this);
    errorConstructor = new QScript::Ecma::Error(this);

    QScript::Ecma::Global::initialize(&m_globalObject, this);

    const QScriptValue::PropertyFlags flags = QScriptValue::SkipInEnumeration;

    m_globalObject.setProperty(QLatin1String("Object"),
                             objectConstructor->ctor, flags);
    m_globalObject.setProperty(QLatin1String("Function"),
                             functionConstructor->ctor, flags);
    m_globalObject.setProperty(QLatin1String("Number"),
                             numberConstructor->ctor, flags);
    m_globalObject.setProperty(QLatin1String("Boolean"),
                             booleanConstructor->ctor, flags);
    m_globalObject.setProperty(QLatin1String("String"),
                             stringConstructor->ctor, flags);
    m_globalObject.setProperty(QLatin1String("Date"),
                             dateConstructor->ctor, flags);
    m_globalObject.setProperty(QLatin1String("Array"),
                             arrayConstructor->ctor, flags);
    m_globalObject.setProperty(QLatin1String("RegExp"),
                             regexpConstructor->ctor, flags);
    m_globalObject.setProperty(QLatin1String("Error"),
                             errorConstructor->ctor, flags);

    m_globalObject.setProperty(QLatin1String("EvalError"),
                             errorConstructor->evalErrorCtor, flags);
    m_globalObject.setProperty(QLatin1String("RangeError"),
                             errorConstructor->rangeErrorCtor, flags);
    m_globalObject.setProperty(QLatin1String("ReferenceError"),
                             errorConstructor->referenceErrorCtor, flags);
    m_globalObject.setProperty(QLatin1String("SyntaxError"),
                             errorConstructor->syntaxErrorCtor, flags);
    m_globalObject.setProperty(QLatin1String("TypeError"),
                             errorConstructor->typeErrorCtor, flags);
    m_globalObject.setProperty(QLatin1String("URIError"),
                             errorConstructor->uriErrorCtor, flags);

    QScriptValueImpl tmp; // ### fixme
    m_evalFunction = new QScript::EvalFunction(this);
    functionConstructor->newFunction(&tmp, m_evalFunction);
    m_globalObject.setProperty(QLatin1String("eval"), tmp, flags);

    QScriptValueImpl mathObject;
    QScript::Ecma::Math::construct(&mathObject, this);
    m_globalObject.setProperty(QLatin1String("Math"), mathObject, flags);

    enumerationConstructor = new QScript::Ext::Enumeration(this);
    m_globalObject.setProperty(QLatin1String("Enumeration"),
                             enumerationConstructor->ctor, flags);

    variantConstructor = new QScript::Ext::Variant(this, m_class_variant);
    m_globalObject.setProperty(QLatin1String("Variant"),
                             variantConstructor->ctor, flags);

#ifndef QT_NO_QOBJECT
    qobjectConstructor = new QScript::ExtQObject(this, m_class_qobject);
    m_globalObject.setProperty(QLatin1String("QObject"),
                             qobjectConstructor->ctor, flags);
    qmetaObjectConstructor = new QScript::ExtQMetaObject(this, m_class_qmetaobject);
    m_globalObject.setProperty(QLatin1String("QMetaObject"),
                             qmetaObjectConstructor->ctor, flags);
#endif

    objectAllocator.blockGC(false);

    QScriptContext *context = pushContext();
    QScriptContextPrivate *context_p = QScriptContextPrivate::get(context);
    context_p->setActivationObject(m_globalObject);
    context_p->setThisObject(m_globalObject);
}

#if !defined(QT_NO_QOBJECT) && !defined(QT_NO_LIBRARY)
static QScriptValueImpl __setupPackage__(QScriptContextPrivate *ctx,
                                         QScriptEnginePrivate *eng,
                                         QScriptClassInfo *)
{
    QString path = ctx->argument(0).toString();
    QStringList components = path.split(QLatin1Char('.'));
    QScriptValueImpl o = eng->globalObject();
    for (int i = 0; i < components.count(); ++i) {
        QString name = components.at(i);
        QScriptValueImpl oo = o.property(name);
        if (!oo.isValid()) {
            oo = eng->newObject();
            o.setProperty(name, oo);
        }
        o = oo;
    }
    return o;
}
#endif

QScriptValueImpl QScriptEnginePrivate::importExtension(const QString &extension)
{
#if defined(QT_NO_QOBJECT) || defined(QT_NO_LIBRARY)
    Q_UNUSED(extension);
#else
    Q_Q(QScriptEngine);
    if (m_importedExtensions.contains(extension))
        return undefinedValue(); // already imported

    QScriptContextPrivate *context = QScriptContextPrivate::get(currentContext());
    QCoreApplication *app = QCoreApplication::instance();
    if (!app)
        return context->throwError(QLatin1String("No application object"));

    QStringList libraryPaths = app->libraryPaths();
    QString dot = QLatin1String(".");
    QStringList pathComponents = extension.split(dot);
    QString dotJs = QLatin1String(".js");
    QString initDotJs = QLatin1String("__init__.js");

    QString ext;
    for (int i = 0; i < pathComponents.count(); ++i) {
        if (!ext.isEmpty())
            ext.append(dot);
        ext.append(pathComponents.at(i));
        if (m_importedExtensions.contains(ext))
            continue; // already imported

        if (m_extensionsBeingImported.contains(ext)) {
            return context->throwError(QString::fromLatin1("recursive import of %0")
                                       .arg(extension));
        }
        m_extensionsBeingImported.insert(ext);

        // look for the extension in library paths
        bool loaded = false;
        for (int j = 0; j < libraryPaths.count(); ++j) {
            QString libPath = libraryPaths.at(j) + QDir::separator() + QLatin1String("script");
            QDir dir(libPath);
            if (!dir.exists(dot))
                continue;

            // look for C++ plugin
            QScriptExtensionInterface *iface = 0;
            QFileInfoList files = dir.entryInfoList(QDir::Files);
            for (int k = 0; k < files.count(); ++k) {
                QFileInfo entry = files.at(k);
                QString filePath = entry.canonicalFilePath();
                QPluginLoader loader(filePath);
                iface = qobject_cast<QScriptExtensionInterface*>(loader.instance());
                if (iface) {
                    if (iface->keys().contains(ext))
                        break; // use this one
                    else
                        iface = 0; // keep looking
                }
            }

            // look for __init__.js in the corresponding dir
            QDir dirdir(libPath);
            bool dirExists = dirdir.exists();
            for (int k = 0; dirExists && (k <= i); ++k)
                dirExists = dirdir.cd(pathComponents.at(k));
            QString initjsContents;
            QString initjsFileName;
            if (dirExists && dirdir.exists(initDotJs)) {
                QFile file(dirdir.canonicalPath()
                           + QDir::separator() + initDotJs);
                if (file.open(QIODevice::ReadOnly)) {
                    QTextStream ts(&file);
                    initjsContents = ts.readAll();
                    initjsFileName = file.fileName();
                    file.close();
                }
            }

            if (!iface && initjsContents.isEmpty())
                continue;

            // initialize the extension in a new context
            QScriptContext *ctx = pushContext();
            QScriptContextPrivate *ctx_p = QScriptContextPrivate::get(ctx);
            ctx_p->setThisObject(globalObject());
            newActivation(&ctx_p->m_activation);
            QScriptObject *activation_data = ctx_p->m_activation.m_object_value;
            activation_data->m_scope = globalObject();

            activation_data->m_members.resize(4);
            activation_data->m_objects.resize(4);
            activation_data->m_members[0].object(
                nameId(QLatin1String("__extension__")), 0,
                QScriptValue::ReadOnly | QScriptValue::Undeletable);
            activation_data->m_objects[0] = QScriptValueImpl(this, ext);
            activation_data->m_members[1].object(
                nameId(QLatin1String("__setupPackage__")), 1, 0);
            activation_data->m_objects[1] = createFunction(__setupPackage__, 0, 0);
            activation_data->m_members[2].object(
                nameId(QLatin1String("__all__")), 2, 0);
            activation_data->m_objects[2] = undefinedValue();
            activation_data->m_members[3].object(
                nameId(QLatin1String("__postInit__")), 3, 0);
            activation_data->m_objects[3] = undefinedValue();

            // the script is evaluated first
            if (!initjsContents.isEmpty()) {
                evaluate(ctx_p, initjsContents, 0, initjsFileName);
                if (hasUncaughtException()) {
                    QScriptValueImpl r = ctx_p->returnValue();
                    popContext();
                    return r;
                }
            }

            // next, the C++ plugin is called
            if (iface) {
                iface->initialize(ext, q);
                if (hasUncaughtException()) {
                    QScriptValueImpl r = ctx_p->returnValue();
                    popContext();
                    return r;
                }
            }

            // if the __postInit__ function has been set, we call it
            QScriptValueImpl postInit = ctx_p->m_activation.property(QLatin1String("__postInit__"));
            if (postInit.isFunction()) {
                postInit.call(globalObject());
                if (hasUncaughtException()) {
                    QScriptValueImpl r = ctx_p->returnValue();
                    popContext();
                    return r;
                }
            }

            popContext();

            m_importedExtensions.insert(ext);
            m_extensionsBeingImported.remove(ext);
            loaded = true;
            break;
        } // for (j)

        if (!loaded) {
            return context->throwError(
                QString::fromLatin1("Unable to import %0: no such extension")
                .arg(extension));
        }

    } // for (i)
#endif // QT_NO_QOBJECT
    return undefinedValue();
}

void QScriptEnginePrivate::gc()
{
    if (!objectAllocator.blocked()) {
        // do the GC now
        maybeGC_helper(/*do_string_gc=*/true);
    } else {
        // GC will be performed the next time maybeGC()
        // is called and the allocator is not blocked
        objectAllocator.requestGC();
    }
}

QStringList QScriptEnginePrivate::uncaughtExceptionBacktrace() const
{
    QScriptValueImpl value = uncaughtException();
    if (!value.isError())
        return QStringList();
    return QScript::Ecma::Error::backtrace(value);
}

void QScriptEnginePrivate::processEvents()
{
#ifndef QT_NO_QOBJECT
    if (m_nextProcessEvents < m_processEventTracker.elapsed()) {
        QCoreApplication::processEvents();
        m_nextProcessEvents = m_nextProcessEvents + m_processEventsInterval;
    }
#endif
}

void QScriptEnginePrivate::setupProcessEvents()
{
    if (m_processEventsInterval > 0) {
        m_nextProcessEvents = m_processEventsInterval;
        m_processEventTracker.restart();
    }
}

#endif // QT_NO_SCRIPT
