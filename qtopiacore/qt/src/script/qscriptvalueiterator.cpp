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

#include "qscriptvalueiterator.h"

#ifndef QT_NO_SCRIPT

#include "qscriptvalueiterator_p.h"
#include "qscriptengine_p.h"
#include "qscriptvalueimpl_p.h"
#include "qscriptcontext_p.h"
#include "qscriptmember_p.h"
#include "qscriptobject_p.h"

/*!
  \since 4.3
  \class QScriptValueIterator

  \brief The QScriptValueIterator class provides a Java-style iterator for QScriptValue.

  \ingroup script
  \mainclass

  The QScriptValueIterator constructor takes a QScriptValue as
  argument.  After construction, the iterator is located at the very
  beginning of the sequence of properties. Here's how to iterate over
  all the properties of a QScriptValue:

  \code
  QScriptValue object;
  ...
  QScriptValueIterator it(object);
  while (it.hasNext()) {
    it.next();
    qDebug() << it.name() << ": " << it.value().toString();
  }
  \endcode

  The next() advances the iterator. The name(), value() and flags()
  functions return the name, value and flags of the last item that was
  jumped over.

  If you want to remove properties as you iterate over the
  QScriptValue, use remove(). If you want to modify the value of a
  property, use setValue().

  Note that QScriptValueIterator only iterates over the QScriptValue's
  own properties; i.e. it does not follow the prototype chain.

  Note that QScriptValueIterator will not automatically skip over
  properties that have the QScriptValue::SkipInEnumeration flag set;
  that flag only affects iteration in script code.  If you want, you
  can skip over such properties with code like the following:

  \code
  while (it.hasNext()) {
    it.next();
    if (it.flags() & QScriptValue::SkipInEnumeration)
        continue;
    qDebug() << "found enumerated property:" << it.name();
  }
  \endcode

  \sa QScriptValue::property()
*/

/*!
  Constructs an iterator for traversing \a object. The iterator is
  set to be at the front of the sequence of properties (before the
  first property).
*/
QScriptValueIterator::QScriptValueIterator(const QScriptValue &object)
    : d_ptr(new QScriptValueIteratorPrivate(this, object))
{
}

/*!
  Destroys the iterator.
*/
QScriptValueIterator::~QScriptValueIterator()
{
    delete d_ptr;
    d_ptr = 0;
}

/*!
  Returns true if there is at least one item ahead of the iterator
  (i.e. the iterator is \e not at the back of the property sequence);
  otherwise returns false.

  \sa next(), hasPrevious()
*/
bool QScriptValueIterator::hasNext() const
{
    Q_D(const QScriptValueIterator);
    if ((d->nextIndex != -1) && d->forward)
        return true;

    if (!d->object.isObject())
        return false;

    QScriptValueImpl v = QScriptValuePrivate::valueOf(d->object);
    int i = d->index;
    if ((i != -1) && !d->forward)
        --i;

    int count = v.memberCount();
    bool found = false;
    while (! found && ++i < count) {
        QScript::Member member;
        v.member(i, &member);
        found = member.isValid();
        if (found && (member.isObjectProperty() || v.isArray())) {
            QScriptValueImpl vv;
            v.get(member, &vv);
            found = vv.isValid();
        }
    }

    QScriptValueIteratorPrivate *that;
    that = const_cast<QScriptValueIteratorPrivate*>(d);
    if (found) {
        that->forward = true;
        that->nextIndex = i;
        return true;
    } else {
        that->nextIndex = -1;
        return false;
    }
}

/*!
  Advances the iterator by one position.

  \sa hasNext(), previous(), name()
*/
void QScriptValueIterator::next()
{
    Q_D(QScriptValueIterator);
    (void)hasNext();

    d->index = d->nextIndex;
    d->nextIndex = -1;
}

/*!
  Returns true if there is at least one item behind the iterator
  (i.e. the iterator is \e not at the front of the property sequence);
  otherwise returns false.

  \sa previous(), hasNext()
*/
bool QScriptValueIterator::hasPrevious() const
{
    Q_D(const QScriptValueIterator);
    if ((d->nextIndex != -1) && !d->forward)
        return true;

    if (!d->object.isObject())
        return false;

    QScriptValueImpl v = QScriptValuePrivate::valueOf(d->object);
    int i = d->index;
    if ((i != -1) && d->forward)
        ++i;

    bool found = false;
    while (! found && --i >= 0) {
        QScript::Member member;
        v.member(i, &member);
        found = member.isValid();
        if (found) {
            QScriptValueImpl vv;
            v.get(member, &vv);
            found = vv.isValid();
        }
    }

    QScriptValueIteratorPrivate *that;
    that = const_cast<QScriptValueIteratorPrivate*>(d);
    if (found) {
        that->forward = false;
        that->nextIndex = i;
        return true;
    } else {
        that->nextIndex = -1;
        return false;
    }

}

/*!
  Moves the iterator back by one position.

  \sa hasPrevious(), next(), name()
*/
void QScriptValueIterator::previous()
{
    Q_D(QScriptValueIterator);
    (void)hasPrevious();

    d->index = d->nextIndex;
    d->nextIndex = -1;
}

/*!
  Moves the iterator to the front of the QScriptValue (before the
  first property).

  \sa toBack(), next()
*/
void QScriptValueIterator::toFront()
{
    Q_D(QScriptValueIterator);
    d->index = -1;
    d->nextIndex = -1;
}

/*!
  Moves the iterator to the back of the QScriptValue (after the
  last property).

  \sa toFront(), previous()
*/
void QScriptValueIterator::toBack()
{
    Q_D(QScriptValueIterator);
    d->index = QScriptValuePrivate::valueOf(d->object).memberCount();
    d->nextIndex = -1;
}

/*!
  Returns the name of the last property that was jumped over using
  next() or previous().

  \sa value(), flags()
*/
QString QScriptValueIterator::name() const
{
    Q_D(const QScriptValueIterator);
    if (d->index == -1)
        return QString();

    QScript::Member member;
    QScriptValuePrivate::valueOf(d->object).member(d->index, &member);

    if (member.isObjectProperty() || member.nameId())
        return member.nameId()->s;

    else if (member.isNativeProperty())
        return QScriptEnginePrivate::toString(member.id());

    return QString();
}

/*!
  Returns the value of the last property that was jumped over using
  next() or previous().

  \sa setValue(), name()
*/
QScriptValue QScriptValueIterator::value() const
{
    Q_D(const QScriptValueIterator);
    if (d->index == -1)
        return QScriptValue();

    QScript::Member member;
    QScriptValuePrivate::valueOf(d->object).member(d->index, &member);
    if (!member.isValid())
        return QScriptValue();

    QScriptValueImpl result;
    QScriptValuePrivate::valueOf(d->object).get(member, &result);
    return result;
}

/*!
  Sets the \a value of the last property that was jumped over using
  next() or previous().

  \sa value(), name()
*/
void QScriptValueIterator::setValue(const QScriptValue &value)
{
    Q_D(const QScriptValueIterator);
    if (d->index == -1)
        return;

    QScript::Member member;
    QScriptValuePrivate::valueOf(d->object).member(d->index, &member);
    if (!member.isValid())
        return;

    QScriptValuePrivate::valueOf(d->object).put(member, QScriptValuePrivate::valueOf(value));
}

/*!
  Returns the flags of the last property that was jumped over using
  next() or previous().

  \sa value()
*/
QScriptValue::PropertyFlags QScriptValueIterator::flags() const
{
    Q_D(const QScriptValueIterator);
    if (d->index == -1)
        return 0;

    QScript::Member member;
    QScriptValuePrivate::valueOf(d->object).member(d->index, &member);
    if (!member.isValid())
        return 0;

    return QScriptValue::PropertyFlags(member.flags());
}

/*!
  Removes the last property that was jumped over using next()
  or previous().

  \sa setValue()
*/
void QScriptValueIterator::remove()
{
    Q_D(const QScriptValueIterator);
    if (d->index == -1)
        return;

    QScript::Member member;
    QScriptValuePrivate::valueOf(d->object).member(d->index, &member);
    if (!member.isValid())
        return;

    QScriptValuePrivate::valueOf(d->object).removeMember(member);
}

/*!
  Makes the iterator operate on \a object. The iterator is set to be
  at the front of the sequence of properties (before the first
  property).
*/
QScriptValueIterator& QScriptValueIterator::operator=(QScriptValue &object)
{
    Q_D(QScriptValueIterator);
    d->object = object;
    d->index = -1;
    d->nextIndex = -1;
    return *this;
}

#endif // QT_NO_SCRIPT
