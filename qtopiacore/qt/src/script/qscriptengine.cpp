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

#include "qscriptengine.h"

#ifndef QT_NO_SCRIPT

#include "qscriptengine_p.h"
#include "qscriptvalueimpl_p.h"
#include "qscriptcontext_p.h"
#include "qscriptmember_p.h"
#include "qscriptobject_p.h"
#include "qscriptsyntaxchecker_p.h"

/*!
  \since 4.3
  \class QScriptEngine

  \brief The QScriptEngine class provides an environment for evaluating Qt Script code.

  \ingroup script
  \mainclass

  See the \l{QtScript} documentation for information about the Qt Script language,
  and how to get started with scripting your C++ application.

  Use evaluate() to evaluate script code.
  \code
    QScriptEngine myEngine;
    QScriptValue three = myEngine.evaluate("1 + 2");
  \endcode

  evaluate() can throw a script exception (e.g. due to a syntax
  error); in that case, the return value is the value that was thrown
  (typically an \c{Error} object). You can check whether the
  evaluation caused an exception by calling hasUncaughtException(). In
  that case, you can call toString() on the error object to obtain an
  error message. The current uncaught exception is also available
  through uncaughtException(). You can obtain a human-readable
  backtrace of the exception with uncaughtExceptionBacktrace().

  \code
    QScriptValue result = myEngine.evaluate(...);
    if (myEngine.hasUncaughtException()) {
        int line = myEngine.uncaughtExceptionLineNumber();
        qDebug() << "uncaught exception at line" << line << ":" << result.toString();
    }
  \endcode

  When handling possibly incomplete input, the canEvaluate() function
  can be used to determine whether code can usefully be passed to
  evaluate(). This can be useful when implementing tools that allow
  code to be written incrementally, such as command line interpreters.

  Use newObject() to create a standard Qt Script object. You can use
  the object-specific functionality in QScriptValue to manipulate the
  script object (e.g. QScriptValue::setProperty()). Use newArray() to
  create a Qt script array object. Use newDate() to create a \c{Date}
  object, and newRegExp() to create a \c{RegExp} object. Use
  newVariant() to wrap a QVariant.

  Use newQObject() to wrap a QObject (or subclass) pointer, and
  newQMetaObject() to wrap a QMetaObject. When wrapping a QObject
  pointer with newQObject(), properties, children and signals and
  slots of the QObject will then become available to script code as
  properties of the created Qt Script object.  No binding code is
  needed because it is done dynamically using the Qt meta object
  system. See the \l{QtScript} documentation for more information.

  Use newFunction() to wrap native (C++) functions, including
  constructors for your own custom types.

  Use importExtension() to import plugin-based extensions into the
  engine.

  Use globalObject() to access the unique \bold {Global Object}
  associated with the script engine. Properties of the Global Object
  are accessible from any script code. Typically, you set properties
  in the engine's Global Object to make your own extensions available
  to scripts. Here is an example of how to expose a number value
  through the Global Object:

  \code
    QScriptValue myNumber = QScriptValue(&myEngine, 123);
    myEngine.globalObject().setProperty("myNumber", myNumber);
    ...
    QScriptValue myNumberPlusOne = myEngine.evaluate("myNumber + 1");
  \endcode

  In addition to exposing plain data, you can also write C++ functions
  that can be invoked from script code. Such functions must have the
  signature QScriptEngine::FunctionSignature. You may then pass the function
  as argument to newFunction(). Here is an example of a function that
  returns the sum of its first two arguments:

  \code
    QScriptValue myAdd(QScriptContext *context, QScriptEngine *engine)
    {
       QScriptValue a = context->argument(0);
       QScriptValue b = context->argument(1);
       return QScriptValue(engine, a.toNumber() + b.toNumber());
    }
  \endcode

  To expose this function to script code, you can set it as a property
  of the Global Object:

  \code
    QScriptValue fun = myEngine.newFunction(myAdd);
    myEngine.globalObject().setProperty("myAdd", fun);
  \endcode

  Once this is done, script code can call your function in the exact
  same manner as a "normal" script function:

  \code
    QScriptValue result = myEngine.evaluate("myAdd(myNumber, 1)");
  \endcode

  You can define shared script functionality for a custom C++ type
  by creating your own default prototype object and setting it with
  setDefaultPrototype(); see also QScriptable.

  Use fromScriptValue() to cast from a QScriptValue to another type,
  and toScriptValue() to create a QScriptValue from another value.
  You can specify how the conversion of C++ types is to be performed
  with qScriptRegisterMetaType() and qScriptRegisterSequenceMetaType().

  \sa QScriptValue, QScriptContext

*/

/*!
    \enum QScriptEngine::ValueOwnership

    This enum specifies the ownership when wrapping a C++ value, e.g. by using newQObject().

    \value QtOwnership The standard Qt ownership rules apply, i.e. the associated object will never be explicitly deleted by the script engine. This is the default. (QObject ownership is explained in \l{Object Trees and Object Ownership}.)
    \value ScriptOwnership The value is owned by the script environment. The associated data will be deleted when appropriate (i.e. after the garbage collector has discovered that there are no more live references to the value).
    \value AutoOwnership If the associated object has a parent, the Qt ownership rules apply (QtOwnership); otherwise, the object is owned by the script environment (ScriptOwnership).
*/

/*!
    \enum  QScriptEngine::QObjectWrapOption

    These flags specify options when wrapping a QObject pointer with newQObject().

    \value ExcludeChildObjects The script object will not expose child objects as properties.
    \value ExcludeSuperClassMethods The script object will not expose signals and slots inherited from the superclass.
    \value ExcludeSuperClassProperties The script object will not expose properties inherited from the superclass.
    \value AutoCreateDynamicProperties Properties that don't already exist in the QObject will be created as dynamic properties of that object, rather than as properties of the script object.
*/

#ifdef QT_NO_QOBJECT
QScriptEngine::QScriptEngine()
    : d_ptr(new QScriptEnginePrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->init();
}

/*! \internal
*/
QScriptEngine::QScriptEngine(QScriptEnginePrivate &dd)
    : d_ptr(&dd)
{
    d_ptr->q_ptr = this;
    d_ptr->init();
}
#else

/*!
    Constructs a QScriptEngine object.

    The globalObject() is initialized to have properties as described in
    \l{ECMA-262}, Section 15.1.
*/
QScriptEngine::QScriptEngine()
    : QObject(*new QScriptEnginePrivate, 0)
{
    Q_D(QScriptEngine);
    d->init();
}

/*!
    Constructs a QScriptEngine object with the given \a parent.

    The globalObject() is initialized to have properties as described in
    \l{ECMA-262}, Section 15.1.
*/

QScriptEngine::QScriptEngine(QObject *parent)
    : QObject(*new QScriptEnginePrivate, parent)
{
    Q_D(QScriptEngine);
    d->init();
}

/*! \internal
*/
QScriptEngine::QScriptEngine(QScriptEnginePrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QScriptEngine);
    d->init();
}
#endif

/*!
  Destroys this QScriptEngine.
*/
QScriptEngine::~QScriptEngine()
{
    Q_D(QScriptEngine);
    d->popContext();
    d->objectAllocator.destruct();
#ifdef QT_NO_QOBJECT
    delete d_ptr;
    d_ptr = 0;
#endif
}

/*!
  Returns this engine's Global Object.

  The Global Object contains the built-in objects that are part of
  \l{ECMA-262}, such as Math, Date and String. Additionally, you can
  set properties of the Global Object to make your own extensions
  available to all script code. Non-local variables in script code
  will be created as properties of the Global Object, as well as local
  variables in global code.
*/
QScriptValue QScriptEngine::globalObject() const
{
    Q_D(const QScriptEngine);
    return d->m_globalObject;
}

/*!
  Returns a QScriptValue of the primitive type Null.

  \sa undefinedValue()
*/
QScriptValue QScriptEngine::nullValue()
{
    Q_D(QScriptEngine);
    return d->nullValue();
}

/*!
  Returns a QScriptValue of the primitive type Undefined.

  \sa nullValue()
*/
QScriptValue QScriptEngine::undefinedValue()
{
    Q_D(QScriptEngine);
    return d->undefinedValue();
}

/*!
  Creates a constructor function from \a fun, with the given \a length.
  The \c{prototype} property of the resulting function is set to be the
  given \a prototype. The \c{constructor} property of \a prototype is
  set to be the resulting function.

  When a function is called as a constructor (e.g. \c{new Foo()}), the
  `this' object associated with the function call is the new object
  that the function is expected to initialize; the prototype of this
  default constructed object will be the function's public
  \c{prototype} property. If you always want the function to behave as
  a constructor (e.g. \c{Foo()} should also create a new object), or
  if you need to create your own object rather than using the default
  `this' object, you should make sure that the prototype of your
  object is set correctly; either by setting it manually, or, when
  wrapping a custom type, by having registered the defaultPrototype()
  of that type. Example:

  \code
  QScriptValue Foo(QScriptContext *context, QScriptEngine *engine)
  {
      if (context->calledAsConstructor()) {
          // initialize the new object
          context->thisObject().setProperty("bar", ...);
          // ...
          // return a non-object value to indicate that the
          // thisObject() should be the result of the "new Foo()" expression
          return engine->undefinedValue();
      } else {
          // not called as "new Foo()", just "Foo()"
          // create our own object and return that one
          QScriptValue object = engine->newObject();
          object.setPrototype(context->callee().property("prototype"));
          object.setProperty("baz", ...);
          return object;
      }
  }

  ...

  QScriptValue fooProto = engine->newObject();
  fooProto.setProperty("whatever", ...);
  engine->globalObject().setProperty("Foo", engine->newFunction(Foo, fooProto));
  \endcode

  To wrap a custom type and provide a constructor for it, you'd typically
  do something like this:

  \code
  class Bar { ... };

  Q_DECLARE_METATYPE(Bar)

  QScriptValue constructBar(QScriptContext *context, QScriptEngine *engine)
  {
      Bar bar;
      // initialize from arguments in context, if desired
      ...
      return engine->toScriptValue(bar);
  }

  class BarPrototype : public QObject, public QScriptable
  {
  // provide the scriptable interface of this type using slots and properties
  ...
  };

  ...

  // create and register the Bar prototype and constructor in the engine
  BarPrototype *barPrototypeObject = new BarPrototype(...);
  QScriptValue barProto = engine->newQObject(barPrototypeObject);
  engine->setDefaultPrototype(qMetaTypeId<Bar>, barProto);
  QScriptValue barCtor = engine->newFunction(constructBar, barProto);
  engine->globalObject().setProperty("Bar", barCtor);
  \endcode
*/
QScriptValue QScriptEngine::newFunction(QScriptEngine::FunctionSignature fun,
                                        const QScriptValue &prototype,
                                        int length)
{
    Q_D(QScriptEngine);
    QScriptValueImpl v = d->createFunction(new QScript::CFunction(fun, length));
    QScriptValueImpl proto = QScriptValuePrivate::valueOf(prototype);
    v.setProperty(d->idTable()->id_prototype, proto,
                  QScriptValue::Undeletable);
    proto.setProperty(d->idTable()->id_constructor, v,
                      QScriptValue::Undeletable
                      | QScriptValue::SkipInEnumeration);
    return v;
}

#ifndef QT_NO_REGEXP
/*!
  Creates a QtScript object of class RegExp with the given
  \a regexp.

  \sa QScriptValue::toRegExp()
*/
QScriptValue QScriptEngine::newRegExp(const QRegExp &regexp)
{
    Q_D(QScriptEngine);
    QScriptValueImpl v;
    d->regexpConstructor->newRegExp(&v, regexp);
    return v;
}

#endif // QT_NO_REGEXP

/*!
  Creates a QtScript object holding the given variant \a value.

  If a default prototype has been registered with the meta type id of
  \a value, then the prototype of the created object will be that
  prototype; otherwise, the prototype will be the Object prototype
  object.

  \sa setDefaultPrototype(), QScriptValue::toVariant()
*/
QScriptValue QScriptEngine::newVariant(const QVariant &value)
{
    Q_D(QScriptEngine);
    return d->newVariant(value);
}

#ifndef QT_NO_QOBJECT
/*!
  Creates a QtScript object that wraps the given QObject \a
  object, using the given \a ownership. The given \a options control
  various aspects of the interaction with the resulting script object.

  Signals and slots, properties and children of \a object are
  available as properties of the created QScriptValue. For more
  information, see the \l{QtScript} documentation.

  If \a object is a null pointer, this function returns nullValue().

  If the given \a object is deleted outside of QtScript's control, any
  attempt to access the deleted QObject's members through the QtScript
  wrapper object (either by script code or C++) will result in a
  script exception.

  \sa QScriptValue::toQObject()
*/
QScriptValue QScriptEngine::newQObject(QObject *object, ValueOwnership ownership,
                                       const QObjectWrapOptions &options)
{
    Q_D(QScriptEngine);
    return d->newQObject(object, ownership, options);
}

#endif // QT_NO_QOBJECT

/*!
  Creates a QtScript object of class Object.

  The prototype of the created object will be the Object
  prototype object.

  \sa newArray(), QScriptValue::setProperty()
*/
QScriptValue QScriptEngine::newObject()
{
    Q_D(QScriptEngine);
    QScriptValueImpl v;
    d->newObject(&v, d->objectConstructor->publicPrototype);
    return v;
}

/*!
  \internal
*/
QScriptValue QScriptEngine::newActivationObject()
{
    Q_D(QScriptEngine);
    QScriptValueImpl v;
    d->newActivation(&v);
    return v;
}

/*!
  Creates a QScriptValue that wraps a native (C++) function. \a fun
  must be a C++ function with signature QScriptEngine::FunctionSignature.  \a
  length is the number of arguments that \a fun expects; this becomes
  the \c{length} property of the created QScriptValue.

  Note that \a length only gives an indication of the number of
  arguments that the function expects; an actual invocation of a
  function can include any number of arguments. You can check the
  \l{QScriptContext::argumentCount()}{argumentCount()} of the
  QScriptContext associated with the invocation to determine the
  actual number of arguments passed.

  By combining newFunction() and the property flags
  QScriptValue::PropertyGetter and QScriptValue::PropertySetter, you
  can create script object properties that behave like normal
  properties in script code, but are in fact accessed through
  functions (analogous to how properties work in \l{Qt's Property
  System}). Example:

  \code
    static QScriptValue getSetFoo(QScriptContext *context, QScriptEngine *engine)
    {
        QScriptValue callee = context->callee();
        if (context->argumentCount() == 1) // writing?
            callee.setProperty("value", context->argument(0));
        return callee.property("value");
    }

    ....

    QScriptValue object = engine.newObject();
    object.setProperty("foo", engine.newFunction(getSetFoo),
        QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
  \endcode

  When the property \c{foo} of the script object is subsequently
  accessed in script code, \c{getSetFoo()} will be invoked to handle
  the access.  In this particular case, we chose to store the "real"
  value of \c{foo} as a property of the accessor function itself; you
  are of course free to do whatever you like in this function.

  In the above example, a single native function was used to handle
  both reads and writes to the property; the argument count is used to
  determine if we are handling a read or write. You can also use two
  separate functions; just specify the relevant flag
  (QScriptValue::PropertyGetter or QScriptValue::PropertySetter) when
  setting the property, e.g.:

  \code
    QScriptValue object = engine.newObject();
    object.setProperty("foo", engine.newFunction(getFoo), QScriptValue::PropertyGetter);
    object.setProperty("foo", engine.newFunction(setFoo), QScriptValue::PropertySetter);
  \endcode

  \sa QScriptValue::call()
*/
QScriptValue QScriptEngine::newFunction(QScriptEngine::FunctionSignature fun, int length)
{
    Q_D(QScriptEngine);
    QScriptValueImpl v = d->createFunction(new QScript::CFunction(fun, length));
    QScriptValueImpl prototype = d->newObject();
    v.setProperty(d->idTable()->id_prototype, prototype);
    prototype.setProperty(d->idTable()->id_constructor, v);
    return v;
}


/*!
  Creates a QtScript object of class Array with the given \a length.

  \sa newObject()
*/
QScriptValue QScriptEngine::newArray(uint length)
{
    Q_D(QScriptEngine);
    QScriptValueImpl v;
    QScript::Array a;
    a.resize(length);
    d->newArray(&v, a);
    return v;
}

/*!
  Creates a QtScript object of class RegExp with the given
  \a pattern and \a flags.
*/
QScriptValue QScriptEngine::newRegExp(const QString &pattern, const QString &flags)
{
    Q_D(QScriptEngine);
    QScriptValueImpl v;
    d->regexpConstructor->newRegExp(&v, pattern, flags);
    return v;
}

/*!
  Creates a QtScript object of class Date with the given
  \a value (the number of milliseconds since 01 January 1970,
  UTC).
*/
QScriptValue QScriptEngine::newDate(qsreal value)
{
    Q_D(QScriptEngine);
    QScriptValueImpl v;
    d->dateConstructor->newDate(&v, value);
    return v;
}

/*!
  Creates a QtScript object of class Date from the given \a value.

  \sa QScriptValue::toDateTime()
*/
QScriptValue QScriptEngine::newDate(const QDateTime &value)
{
    Q_D(QScriptEngine);
    QScriptValueImpl v;
    d->dateConstructor->newDate(&v, value);
    return v;
}

#ifndef QT_NO_QOBJECT
/*!
  Creates a QtScript object that represents a QObject class, using the
  the given \a metaObject and constructor \a ctor.

  Enums of \a metaObject are available as properties of the created
  QScriptValue. When the class is called as a function, \a ctor will
  be called to create a new instance of the class.

  \sa newQObject()
*/
QScriptValue QScriptEngine::newQMetaObject(
    const QMetaObject *metaObject, const QScriptValue &ctor)
{
    Q_D(QScriptEngine);
    QScriptValueImpl v;
    d->qmetaObjectConstructor->newQMetaObject(&v, metaObject, QScriptValuePrivate::valueOf(ctor));
    return v;
}

/*!
  \fn QScriptValue QScriptEngine::scriptValueFromQMetaObject()

  Creates a QScriptValue that represents the Qt class \c{T}.

  This function is used in combination with one of the
  Q_SCRIPT_DECLARE_QMETAOBJECT() macro. Example:

  \code
  Q_SCRIPT_DECLARE_QMETAOBJECT(QLineEdit, QWidget*)

  ...

  QScriptValue lineEditClass = engine.scriptValueFromQMetaObject<QLineEdit>();
  engine.globalObject().setProperty("QLineEdit", lineEditClass);
  \endcode

  \warning This function is not available with MSVC 6. Use
  qScriptValueFromQMetaObject() instead if you need to support that version
  of the compiler.

*/

/*!
  \fn QScriptValue qScriptValueFromQMetaObject(QScriptEngine *engine)
  \since 4.3
  \relates QScriptEngine

  Uses \a engine to create a QScriptValue that represents the Qt class
  \c{T}.

  This function is equivalent to
  QScriptEngine::scriptValueFromQMetaObject(). It is provided as a
  work-around for MSVC 6, which doesn't support member template
  functions.
*/
#endif // QT_NO_QOBJECT

/*!
  Returns true if \a program can be evaluated; i.e. the code is
  sufficient to determine whether it appears to be a syntactically
  correct program, or contains a syntax error.

  This function returns false if \a program is incomplete; i.e. the
  input is syntactically correct up to the point where the input is
  terminated.

  Note that this function only does a static check of \a program;
  e.g. it does not check whether references to variables are
  valid, and so on.

  A typical usage of canEvaluate() is to implement an interactive
  interpreter for QtScript. The user is repeatedly queried for
  individual lines of code; the lines are concatened internally, and
  only when canEvaluate() returns true for the resulting program is it
  passed to evaluate().

  The following are some examples to illustrate the behavior of
  canEvaluate(). (Note that all example inputs are assumed to have an
  explicit newline as their last character, since otherwise the
  QtScript parser would automatically insert a semi-colon character at
  the end of the input, and this could cause canEvaluate() to produce
  different results.)

  Given the input
  \code
  if (hello && world)
    print("hello world");
  \endcode
  canEvaluate() will return true, since the program appears to be complete.

  Given the input
  \code
  if (hello &&
  \endcode
  canEvaluate() will return false, since the if-statement is not complete,
  but is syntactically correct so far.

  Given the input
  \code
  0 = 0
  \endcode
  canEvaluate() will return true, but evaluate() will throw a
  SyntaxError given the same input.

  Given the input
  \code
  ./test.js
  \endcode
  canEvaluate() will return true, even though the code is clearly not
  syntactically valid QtScript code. evaluate() will throw a
  SyntaxError when this code is evaluated.

  Given the input
  \code
  foo["bar"]
  \endcode
  canEvaluate() will return true, but evaluate() will throw a
  ReferenceError if \c{foo} is not defined in the script
  environment.

  \sa evaluate()
*/
bool QScriptEngine::canEvaluate(const QString &program) const
{
    QScript::SyntaxChecker checker;
    return checker.parse(program);
}

/*!
  Evaluates \a program, using \a lineNumber as the base line number,
  and returns the result of the evaluation.

  The script code will be evaluated in the current context.

  The evaluation of \a program can cause an exception in the
  engine; in this case the return value will be the exception
  that was thrown (typically an \c{Error} object). You can call
  hasUncaughtException() to determine if an exception occurred in
  the last call to evaluate().

  \a lineNumber is used to specify a starting line number for \a
  program; line number information reported by the engine that pertain
  to this evaluation (e.g. uncaughtExceptionLineNumber()) will be
  based on this argument. For example, if \a program consists of two
  lines of code, and the statement on the second line causes a script
  exception, uncaughtExceptionLineNumber() would return the given \a
  lineNumber plus one. When no starting line number is specified, line
  numbers will be 1-based.

  \a fileName is used for error reporting. For example in error objects
  the file name is accessible through the "fileName" property if it's
  provided with this function.

  \sa canEvaluate(), hasUncaughtException()
*/
QScriptValue QScriptEngine::evaluate(const QString &program, const QString &fileName, int lineNumber)
{
    Q_D(QScriptEngine);
    QScriptContextPrivate *ctx_p = QScriptContextPrivate::get(d->currentContext());
    d->evaluate(ctx_p, program, lineNumber, fileName);
    return ctx_p->m_result;
}

/*!
  Returns the current context.

  The current context is typically accessed to retrieve the arguments
  and `this' object in native functions; for convenience, it is
  available as the first argument in QScriptEngine::FunctionSignature.
*/
QScriptContext *QScriptEngine::currentContext() const
{
    Q_D(const QScriptEngine);
    return d->currentContext();
}

/*!
  Enters a new execution context and returns the associated
  QScriptContext object.

  Once you are done with the context, you should call popContext() to
  restore the old context.

  By default, the `this' object of the new context is the Global Object.
  The context's \l{QScriptContext::callee()}{callee}() will be invalid.

  This function is useful when you want to evaluate script code
  as if it were the body of a function. You can use the context's
  \l{QScriptContext::activationObject()}{activationObject}() to initialize
  local variables that will be available to scripts. Example:

  \code
  QScriptEngine engine;
  QScriptContext *context = engine.pushContext();
  context->activationObject().setProperty("myArg", QScriptValue(&engine, 123));
  engine.evaluate("var tmp = myArg + 42");
  ...
  engine.popContext();
  \endcode

  In the above example, the new variable "tmp" defined in the script
  will be local to the context; in other words, the script doesn't
  have any effect on the global environment.

  \sa popContext()
*/
QScriptContext *QScriptEngine::pushContext()
{
    Q_D(QScriptEngine);
    QScriptContext *context = d->pushContext();
    context->setThisObject(globalObject());
    QScriptValue activation = newActivationObject();
    activation.setScope(globalObject());
    context->setActivationObject(activation);
    return context;
}

/*!
  Pops the current execution context and restores the previous one.
  This function must be used in conjunction with pushContext().

  \sa pushContext()
*/
void QScriptEngine::popContext()
{
    Q_D(QScriptEngine);
    if (d->currentContext() && d->currentContext()->parentContext())
        d->popContext();
}

/*!
  Returns true if the last script evaluation (whether direct or
  indirect) resulted in an uncaught exception; otherwise returns
  false.

  The exception state is cleared every time a script function call is
  done in the engine, or when evaluate() is called.

  \sa uncaughtException(), uncaughtExceptionLineNumber(),
      uncaughtExceptionBacktrace()
*/
bool QScriptEngine::hasUncaughtException() const
{
    Q_D(const QScriptEngine);
    return d->hasUncaughtException();
}

/*!
  Returns the current uncaught exception, or an invalid QScriptValue
  if there is no uncaught exception.

  The exception value is typically an \c{Error} object; in that case,
  you can call toString() on the return value to obtain an error
  message.

  \sa hasUncaughtException(), uncaughtExceptionLineNumber(),
      uncaughtExceptionBacktrace()
*/
QScriptValue QScriptEngine::uncaughtException() const
{
    Q_D(const QScriptEngine);
    return d->uncaughtException();
}

/*!
  Returns the line number where the last uncaught exception occurred.

  Line numbers are 1-based, unless a different base was specified as
  the second argument to evaluate().

  \sa hasUncaughtException(), uncaughtExceptionBacktrace()
*/
int QScriptEngine::uncaughtExceptionLineNumber() const
{
    return QScriptContextPrivate::get(currentContext())->errorLineNumber;
}

/*!
  Returns a human-readable backtrace of the last uncaught exception.

  Each line is of the form \c{<function-name>(<arguments>)@<file-name>:<line-number>}.

  \sa uncaughtException()
*/
QStringList QScriptEngine::uncaughtExceptionBacktrace() const
{
    Q_D(const QScriptEngine);
    return d->uncaughtExceptionBacktrace();
}

/*!
  Returns the default prototype associated with the given \a metaTypeId,
  or an invalid QScriptValue if no default prototype has been set.

  \sa setDefaultPrototype()
*/
QScriptValue QScriptEngine::defaultPrototype(int metaTypeId) const
{
    Q_D(const QScriptEngine);
    return d->defaultPrototype(metaTypeId);
}

/*!
  Sets the default prototype of the given \a metaTypeId to \a prototype.

  The default prototype provides a script interface for values of
  type \a metaTypeId when a value of that type is accessed from script
  code.  Whenever the script engine (implicitly or explicitly) creates
  a QScriptValue from a value of type \a metaTypeId, the default
  prototype will be set as the QScriptValue's prototype.

  \sa defaultPrototype(), qScriptRegisterMetaType(), QScriptable, {Default Prototypes Example}
*/
void QScriptEngine::setDefaultPrototype(int metaTypeId, const QScriptValue &prototype)
{
    Q_D(QScriptEngine);
    d->setDefaultPrototype(metaTypeId, QScriptValuePrivate::valueOf(prototype));
}

/*!
    \typedef QScriptEngine::FunctionSignature
    \relates QScriptEngine

    The function signature \c{QScriptValue f(QScriptContext *, QScriptEngine *)}.

    A function with such a signature can be passed to QScriptEngine::newFunction()
    to wrap the function.
*/

/*!
    \typedef QScriptEngine::MarshalFunction
    \internal
*/

/*!
    \typedef QScriptEngine::DemarshalFunction
    \internal
*/

/*!
    \internal
*/
QScriptValue QScriptEngine::create(int type, const void *ptr)
{
    Q_D(QScriptEngine);
    return d->create(type, ptr);
}

/*!
    \internal
*/
bool QScriptEngine::convert(const QScriptValue &value, int type, void *ptr)
{
    Q_D(QScriptEngine);
    return d->convert(QScriptValuePrivate::valueOf(value), type, ptr);
}

/*!
    \internal
*/
void QScriptEngine::registerCustomType(int type, MarshalFunction mf,
                                       DemarshalFunction df,
                                       const QScriptValue &prototype)
{
    Q_D(QScriptEngine);
    QScriptCustomTypeInfo info = d->m_customTypes.value(type);
    info.marshal = mf;
    info.demarshal = df;
    info.prototype = QScriptValuePrivate::valueOf(prototype);
    d->m_customTypes.insert(type, info);
}

/*!
    Imports the given \a extension into this QScriptEngine.  Returns
    undefinedValue() if the extension was successfully imported. You
    can call hasUncaughtException() to check if an error occurred; in
    that case, the return value is the value that was thrown by the
    exception (usually an \c{Error} object).

    QScriptEngine ensures that a particular extension is only imported
    once; subsequent calls to importExtension() with the same extension
    name will do nothing and return undefinedValue().

    \sa QScriptExtensionPlugin, {Creating QtScript Extensions}
*/
QScriptValue QScriptEngine::importExtension(const QString &extension)
{
    Q_D(QScriptEngine);
    return d->importExtension(extension);
}

/*! \fn QScriptValue QScriptEngine::toScriptValue(const T &value)

    Creates a QScriptValue with the given \a value.

    Note that the template type \c{T} must be known to QMetaType.

    See \l{Conversion Between QtScript and C++ Types} for a
    description of the built-in type conversion provided by
    QtScript. By default, the types that are not specially handled by
    QtScript are represented as QVariants (e.g. the \a value is passed
    to newVariant()); you can change this behavior by installing your
    own type conversion functions with qScriptRegisterMetaType().

    \warning This function is not available with MSVC 6. Use
    qScriptValueFromValue() instead if you need to support that
    version of the compiler.

    \sa fromScriptValue(), qScriptRegisterMetaType()
*/

/*! \fn T QScriptEngine::fromScriptValue(const QScriptValue &value)

    Returns the given \a value converted to the template type \c{T}.

    Note that \c{T} must be known to QMetaType.

    See \l{Conversion Between QtScript and C++ Types} for a
    description of the built-in type conversion provided by
    QtScript.

    \warning This function is not available with MSVC 6. Use
    qScriptValueToValue() or qscriptvalue_cast() instead if you need
    to support that version of the compiler.

    \sa toScriptValue(), qScriptRegisterMetaType()
*/

/*!
    \fn QScriptValue qScriptValueFromValue(QScriptEngine *engine, const T &value)
    \since 4.3
    \relates QScriptEngine

    Creates a QScriptValue using the given \a engine with the given \a
    value of template type \c{T}.

    This function is equivalent to QScriptEngine::toScriptValue().
    It is provided as a work-around for MSVC 6, which doesn't support
    member template functions.

    \sa qScriptValueToValue()
*/

/*!
    \fn T qScriptValueToValue(const QScriptValue &value)
    \since 4.3
    \relates QScriptEngine

    Returns the given \a value converted to the template type \c{T}.

    This function is equivalent to QScriptEngine::fromScriptValue().
    It is provided as a work-around for MSVC 6, which doesn't
    support member template functions.

    \sa qScriptValueFromValue()
*/

/*!
    \fn QScriptValue qScriptValueFromSequence(QScriptEngine *engine, const Container &container)
    \since 4.3
    \relates QScriptEngine

    Creates an array in the form of a QScriptValue using the given \a engine
    with the given \a container of template type \c{Container}.

    The \c Container type must provide a \c const_iterator class to enable the
    contents of the container to be copied into the array.

    Additionally, the type of each element in the sequence should be suitable
    for conversion to a QScriptValue.
    See \l{QtScript Module#Conversion Between QtScript and C++ Types}
    {Conversion Between QtScript and C++ Types} for more information about the
    restrictions on types that can be used with QScriptValue.

    \sa qScriptValueFromValue()
*/

/*!
    \fn void qScriptValueToSequence(const QScriptValue &value, Container &container)
    \since 4.3
    \relates QScriptEngine

    Copies the elements in the sequence specified by \a value to the given
    \a container of template type \c{Container}.

    The \a value used is typically an array, but any container can be copied
    as long as it provides a \c length property describing how many elements
    it contains.

    Additionally, the type of each element in the sequence must be suitable
    for conversion to a C++ type from a QScriptValue.
    See \l{QtScript Module#Conversion Between QtScript and C++ Types}
    {Conversion Between QtScript and C++ Types} for more information about the
    restrictions on types that can be used with QScriptValue.

    \sa qscriptvalue_cast()
*/

/*!
    \fn T qscriptvalue_cast(const QScriptValue &value)
    \since 4.3
    \relates QScriptValue

    Returns the given \a value converted to the template type \c{T}.

    \sa qScriptRegisterMetaType(), QScriptEngine::toScriptValue()
*/

/*! \fn int qScriptRegisterMetaType(
            QScriptEngine *engine,
            QScriptValue (*toScriptValue)(QScriptEngine *, const T &t),
            void (*fromScriptValue)(const QScriptValue &, T &t),
            const QScriptValue &prototype = QScriptValue())
    \relates QScriptEngine

    Registers the type \c{T} in the given \a engine. \a toScriptValue must
    be a function that will convert from a value of type \c{T} to a
    QScriptValue, and \a fromScriptValue a function that does the
    opposite. \a prototype, if valid, is the prototype that's set on
    QScriptValues returned by \a toScriptValue.

    Returns the internal ID used by QMetaType.

    You only need to call this function if you want to provide custom
    conversion of values of type \c{T}, i.e. if the default
    QVariant-based representation and conversion is not
    appropriate. If you only want to define a common script interface
    for values of type \c{T}, and don't care how those values are
    represented, use
    \l{QScriptEngine::setDefaultPrototype()}{setDefaultPrototype}()
    instead.

    You need to declare the custom type first with
    Q_DECLARE_METATYPE().

    After a type has been registered, you can convert from a
    QScriptValue to that type using
    \l{QScriptEngine::fromScriptValue()}{fromScriptValue}(), and
    create a QScriptValue from a value of that type using
    \l{QScriptEngine::toScriptValue()}{toScriptValue}(). The engine
    will take care of calling the proper conversion function when
    calling C++ slots, and when getting or setting a C++ property;
    i.e. the custom type may be used seamlessly on both the C++ side
    and the script side.

    The following is an example of how to use this function. We will
    specify custom conversion of our type \c{MyStruct}. Here's the C++
    type:

    \code
      struct MyStruct {
        int x;
        int y;
      };
    \endcode

    We must declare it so that the type will be known to QMetaType:

    \code
      Q_DECLARE_METATYPE(MyStruct)
    \endcode

    Next, the \c{MyStruct} conversion functions. We represent the
    \c{MyStruct} value as a script object and just copy the properties:

    \code
    QScriptValue toScriptValue(QScriptEngine *engine, const MyStruct &s)
    {
      QScriptValue obj = engine->newObject();
      obj.setProperty("x", QScriptValue(engine, s.x));
      obj.setProperty("y", QScriptValue(engine, s.y));
      return obj;
    }

    void fromScriptValue(const QScriptValue &obj, MyStruct &s)
    {
      s.x = obj.property("x").toInt32();
      s.y = obj.property("y").toInt32();
    }
    \endcode

    Now we can register \c{MyStruct} with the engine:
    \code
    qScriptRegisterMetaType(engine, toScriptValue, fromScriptValue);
    \endcode

    Working with \c{MyStruct} values is now easy:
    \code
    MyStruct s = qscriptvalue_cast<MyStruct>(context->argument(0));
    ...
    MyStruct s2;
    s2.x = s.x + 10;
    s2.y = s.y + 20;
    QScriptValue v = engine->toScriptValue(s2);
    \endcode

    If you want to be able to construct values of your custom type
    from script code, you have to register a constructor function for
    the type. For example:

    \code
    QScriptValue createMyStruct(QScriptContext *, QScriptEngine *engine)
    {
        MyStruct s;
        s.x = 123;
        s.y = 456;
        return engine->toScriptValue(s);
    }
    ...
    QScriptValue ctor = engine.newFunction(createMyStruct);
    engine.globalObject().setProperty("MyStruct", ctor);
    \endcode

    \sa qScriptRegisterSequenceMetaType(), qRegisterMetaType()
*/

/*!
    \macro Q_SCRIPT_DECLARE_QMETAOBJECT(QMetaObject, ArgType)
    \since 4.3
    \relates QScriptEngine

    Declares the given \a QMetaObject. Used in combination with
    QScriptEngine::scriptValueFromQMetaObject() to make enums and
    instantiation of \a QMetaObject available to script code. The
    constructor generated by this macro takes a single argument of
    type \a ArgType; typically the argument is the parent type of the
    new instance, in which case \a ArgType is \c{QWidget*} or
    \c{QObject*}. Objects created by the constructor will have
    QScriptEngine::AutoOwnership ownership.
*/

/*! \fn int qScriptRegisterSequenceMetaType(
            QScriptEngine *engine,
            const QScriptValue &prototype = QScriptValue())
    \relates QScriptEngine

    Registers the sequence type \c{T} in the given \a engine. This
    function provides conversion functions that convert between \c{T}
    and Qt Script \c{Array} objects. \c{T} must provide a
    const_iterator class and begin(), end() and push_back()
    functions. If \a prototype is valid, it will be set as the
    prototype of \c{Array} objects due to conversion from \c{T};
    otherwise, the standard \c{Array} prototype will be used.

    Returns the internal ID used by QMetaType.

    You need to declare the container type first with
    Q_DECLARE_METATYPE(). Example:

    \code
    Q_DECLARE_METATYPE(QVector<int>)

    ...

    qScriptRegisterSequenceMetaType<QVector<int> >(engine);
    ...
    QVector<int> v = qscriptvalue_cast<QVector<int> >(engine->evaluate("[5, 1, 3, 2]"));
    qSort(v.begin(), v.end());
    QScriptValue a = engine->toScriptValue(v);
    qDebug() << a.toString(); // outputs "[1, 2, 3, 5]"
    \endcode

    \sa qScriptRegisterMetaType()
*/

/*!
  Runs the garbage collector.

  The garbage collector will attempt to reclaim memory by locating and
  disposing of objects that are no longer reachable in the script
  environment.

  Normally you don't need to call this function; the garbage collector
  will automatically be invoked when the QScriptEngine decides that
  it's wise to do so (i.e. when a certain number of new objects have
  been created). However, you can call this function to explicitly
  request that garbage collection should be performed as soon as
  possible.
*/
void QScriptEngine::collectGarbage()
{
    Q_D(QScriptEngine);
    d->gc();
}

/*!

  Sets the interval between calls to QCoreApplication::processEvents
  to \a interval milliseconds.

  While the interpreter is running, all event processing is by default
  blocked. This means for instance that the gui will not be updated
  and timers will not be fired. To allow event processing during
  interpreter execution one can specify the processing interval to be
  a positive value, indicating the number of milliseconds between each
  time QCoreApplication::processEvents() is called.

  The default value is -1, which disables event processing during
  interpreter execution.

  \sa processEventsInterval()
*/
void QScriptEngine::setProcessEventsInterval(int interval)
{
    Q_D(QScriptEngine);
    d->m_processEventsInterval = interval;
}

/*!

  Returns the interval in milliseconds between calls to
  QCoreApplication::processEvents() while the interpreter is running.

  \sa setProcessEventsInterval()
*/
int QScriptEngine::processEventsInterval() const
{
    Q_D(const QScriptEngine);
    return d->m_processEventsInterval;
}

#endif // QT_NO_SCRIPT
