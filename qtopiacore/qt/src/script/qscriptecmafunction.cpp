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

#include "qscriptecmafunction_p.h"

#ifndef QT_NO_SCRIPT

#include "qscriptengine_p.h"
#include "qscriptvalueimpl_p.h"
#include "qscriptcontext_p.h"
#include "qscriptmember_p.h"
#include "qscriptobject_p.h"

#include <QtCore/QtDebug>

#ifndef QT_NO_QOBJECT
#include "qscriptextqobject_p.h"
#include <QtCore/QMetaMethod>
#endif

namespace QScript { namespace Ecma {

Function::Function(QScriptEnginePrivate *eng, QScriptClassInfo *classInfo):
    Core(eng), m_classInfo(classInfo)
{
    publicPrototype = eng->createFunction(method_void, 0, m_classInfo); // public prototype
}

Function::~Function()
{
}

void Function::initialize()
{
    QScriptEnginePrivate *eng = engine();
    eng->newConstructor(&ctor, this, publicPrototype);

    const QScriptValue::PropertyFlags flags = QScriptValue::SkipInEnumeration;
    publicPrototype.setProperty(QLatin1String("toString"),
                                eng->createFunction(method_toString, 1, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("apply"),
                                eng->createFunction(method_apply, 1, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("call"),
                                eng->createFunction(method_call, 1, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("connect"),
                                eng->createFunction(method_connect, 1, m_classInfo), flags);
    publicPrototype.setProperty(QLatin1String("disconnect"),
                                eng->createFunction(method_disconnect, 1, m_classInfo), flags);
}

void Function::execute(QScriptContextPrivate *context)
{
    int lineNumber = context->currentLine;
    QString contents = buildFunction(context);
    engine()->evaluate(context, contents, lineNumber);
}

QString Function::buildFunction(QScriptContextPrivate *context)
{
    int argc = context->argumentCount();

    QString code;
    code += QLatin1String("return function(");

    // the formals
    for (int i = 0; i < argc - 1; ++i) {
        if (i != 0)
            code += QLatin1String(",");

        code += context->argument(i).toString();
    }

    code += QLatin1String("){");

    // the function body
    if (argc != 0)
        code += context->argument(argc - 1).toString();

    code += QLatin1String("\n}");

    return code;
}

void Function::newFunction(QScriptValueImpl *result, QScriptFunction *foo)
{
    engine()->newFunction(result, foo);
}

QScriptValueImpl Function::method_toString(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    QScriptValueImpl self = context->thisObject();
    if (QScriptFunction *foo = self.toFunction()) {
        QString code = foo->toString(context);
        return QScriptValueImpl(eng, code);
    }

    return context->throwError(QScriptContext::TypeError,
                               QLatin1String("Function.prototype.toString"));
}

QScriptValueImpl Function::method_call(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    if (! context->thisObject().isFunction()) {
        return context->throwError(QScriptContext::TypeError,
                                   QLatin1String("Function.prototype.call"));
    }

    QScriptValueImpl thisObject = context->argument(0).toObject();
    if (! (thisObject.isValid () && thisObject.isObject()))
        thisObject = eng->globalObject();

    QScriptValueImplList args;
    for (int i = 1; i < context->argumentCount(); ++i)
        args << context->argument(i);

    return context->thisObject().call(thisObject, args);
}

QScriptValueImpl Function::method_apply(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    if (! context->thisObject().isFunction()) {
        return context->throwError(QScriptContext::TypeError,
                                   QLatin1String("Function.prototype.apply"));
    }

    QScriptValueImpl thisObject = context->argument(0).toObject();
    if (! (thisObject.isValid () && thisObject.isObject()))
        thisObject = eng->globalObject();

    QScriptValueImplList args;
    QScriptValueImpl undefined = eng->undefinedValue();

    QScriptValueImpl arg = context->argument(1);

    if (Ecma::Array::Instance *arr = eng->arrayConstructor->get(arg)) {
        QScript::Array actuals = arr->value;

        for (quint32 i = 0; i < actuals.count(); ++i) {
            QScriptValueImpl a = actuals.at(i);
            if (! a.isValid())
                args << undefined;
            else
                args << a;
        }
    } else if (arg.classInfo() == eng->m_class_arguments) {
        QScript::ArgumentsObjectData *arguments;
        arguments = static_cast<QScript::ArgumentsObjectData*> (arg.objectData().data());
        QScriptObject *activation = arguments->activation.objectValue();
        for (uint i = 0; i < arguments->length; ++i)
            args << activation->m_objects[i];
    } else if (!(arg.isUndefined() || arg.isNull())) {
        return context->throwError(QScriptContext::TypeError,
                                   QLatin1String("Function.prototype.apply: second argument is not an array"));
    }

    return context->thisObject().call(thisObject, args);
}

QScriptValueImpl Function::method_void(QScriptContextPrivate *, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
    return eng->undefinedValue();
}

QScriptValueImpl Function::method_disconnect(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *)
{
#ifndef QT_NO_QOBJECT
    if (context->argumentCount() == 0) {
        return context->throwError(
            QLatin1String("Function.prototype.disconnect: no arguments given"));
    }

    QScriptValueImpl self = context->thisObject();
    QScriptFunction *fun = self.toFunction();
    if ((fun == 0) || (fun->type() != QScriptFunction::Qt)) {
        return context->throwError(
            QScriptContext::TypeError,
            QLatin1String("Function.prototype.disconnect: this object is not a signal"));
    }

    QtFunction *qtSignal = static_cast<QtFunction*>(fun);

    QMetaMethod sig = qtSignal->metaObject()->method(qtSignal->initialIndex());
    if (sig.methodType() != QMetaMethod::Signal) {
        return context->throwError(QScriptContext::TypeError,
            QString::fromLatin1("Function.prototype.disconnect: %0::%1 is not a signal")
            .arg(QLatin1String(qtSignal->metaObject()->className()))
            .arg(QLatin1String(sig.signature())));
    }

    QScriptValueImpl receiver;
    QScriptValueImpl slot;
    QScriptValueImpl arg0 = context->argument(0);
    if (context->argumentCount() < 2) {
        receiver = QScriptValueImpl();
        slot = arg0;
    } else {
        receiver = arg0;
        QScriptValueImpl arg1 = context->argument(1);
        if (arg1.isFunction())
            slot = arg1;
        else
            slot = receiver.property(arg1.toString(), QScriptValue::ResolvePrototype);
    }

    QScriptFunction *otherFun = slot.toFunction();
    if (otherFun == 0) {
        return context->throwError(
            QScriptContext::TypeError,
            QLatin1String("Function.prototype.disconnect: target is not a function"));
    }

    bool ok = qtSignal->destroyConnection(self, receiver, slot);
    if (!ok) {
        return context->throwError(
            QString::fromLatin1("Function.prototype.disconnect: failed to disconnect from %0::%1")
            .arg(QLatin1String(qtSignal->metaObject()->className()))
            .arg(QLatin1String(sig.signature())));
    }
    return eng->undefinedValue();
#else
    return context->throwError(QScriptContext::TypeError,
                               QLatin1String("Function.prototype.disconnect"));
#endif // QT_NO_QOBJECT
}

QScriptValueImpl Function::method_connect(QScriptContextPrivate *context, QScriptEnginePrivate *eng, QScriptClassInfo *classInfo)
{
    Q_UNUSED(classInfo);

#ifndef QT_NO_QOBJECT
    if (context->argumentCount() == 0) {
        return context->throwError(
            QLatin1String("Function.prototype.connect: no arguments given"));
    }

    QScriptValueImpl self = context->thisObject();
    QScriptFunction *fun = self.toFunction();
    if ((fun == 0) || (fun->type() != QScriptFunction::Qt)) {
        return context->throwError(
            QScriptContext::TypeError,
            QLatin1String("Function.prototype.connect: this object is not a signal"));
    }

    QtFunction *qtSignal = static_cast<QtFunction*>(fun);

    QMetaMethod sig = qtSignal->metaObject()->method(qtSignal->initialIndex());
    if (sig.methodType() != QMetaMethod::Signal) {
        return context->throwError(QScriptContext::TypeError,
            QString::fromLatin1("Function.prototype.connect: %0::%1 is not a signal")
            .arg(QLatin1String(qtSignal->metaObject()->className()))
            .arg(QLatin1String(sig.signature())));
    }

    QScriptValueImpl receiver;
    QScriptValueImpl slot;
    QScriptValueImpl arg0 = context->argument(0);
    if (context->argumentCount() < 2) {
        receiver = QScriptValueImpl();
        slot = arg0;
    } else {
        receiver = arg0;
        QScriptValueImpl arg1 = context->argument(1);
        if (arg1.isFunction())
            slot = arg1;
        else
            slot = receiver.property(arg1.toString(), QScriptValue::ResolvePrototype);
    }

    QScriptFunction *otherFun = slot.toFunction();
    if (otherFun == 0) {
        return context->throwError(
            QScriptContext::TypeError,
            QLatin1String("Function.prototype.connect: target is not a function"));
    }

    bool ok = qtSignal->createConnection(self, receiver, slot);
    if (!ok) {
        return context->throwError(
            QString::fromLatin1("Function.prototype.connect: failed to connect to %0::%1")
            .arg(QLatin1String(qtSignal->metaObject()->className()))
            .arg(QLatin1String(sig.signature())));
    }
    return eng->undefinedValue();
#else
    Q_UNUSED(eng);
    Q_UNUSED(classInfo);
    return context->throwError(QScriptContext::TypeError,
                               QLatin1String("Function.prototype.connect"));
#endif // QT_NO_QOBJECT
}

} } // namespace QScript::Ecma

#endif // QT_NO_SCRIPT
