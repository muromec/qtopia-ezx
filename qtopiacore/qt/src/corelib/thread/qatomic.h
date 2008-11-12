/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QATOMIC_H
#define QATOMIC_H

#if defined(QT_MOC) || defined(QT_BUILD_QMAKE) || defined(QT_RCC) || defined(QT_UIC)
#  include <QtCore/qatomic_generic.h>
#else
#  include <QtCore/qatomic_arch.h>
#endif
#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

QT_MODULE(Core)

#ifndef Q_SPECIALIZED_QATOMIC

/*
    We assume that the following 11 functions have been declared by the
    platform specific qatomic.h:

    int q_atomic_test_and_set_int(volatile int *ptr, int expected, int newval);
    int q_atomic_test_and_set_acquire_int(volatile int *ptr, int expected, int newval);
    int q_atomic_test_and_set_release_int(volatile int *ptr, int expected, int newval);

    int q_atomic_test_and_set_ptr(volatile void *ptr, void *expected, void *newval);
    int q_atomic_increment(volatile int *ptr);
    int q_atomic_decrement(volatile int *ptr);
    int q_atomic_set_int(volatile int *ptr, int newval);
    void *q_atomic_set_ptr(volatile void *ptr, void *newval);

    int q_atomic_fetch_and_add_int(volatile int *ptr, value);
    int q_atomic_fetch_and_add_acquire_int(volatile int *ptr, value);
    int q_atomic_fetch_and_add_release_int(volatile int *ptr, value);

    If you cannot implement these functions efficiently on your
    platform without great difficulty, consider defining
    Q_SPECIALIZED_QATOMIC.  By doing this, you need to implement:

    struct QBasicAtomic;
    template <typename T> struct QBasicAtomicPointer<T>;
    int q_atomic_test_and_set_ptr(volatile void *ptr, void *expected, void *newval);
    void *q_atomic_set_ptr(volatile void *ptr, void *newval);
*/

struct QBasicAtomic {
    volatile int value;

    void init(int x = 0)
    { value = x; }

    inline bool ref()
    { return q_atomic_increment(&value) != 0; }

    inline bool deref()
    { return q_atomic_decrement(&value) != 0; }

    inline bool operator==(int x) const
    { return value == x; }

    inline bool operator!=(int x) const
    { return value != x; }

    inline bool operator!() const
    { return value == 0; }

    inline operator int() const
    { return value; }

    inline QBasicAtomic &operator=(int x)
    {
        value = x;
        return *this;
    }

    inline bool testAndSet(int expected, int newval)
    { return q_atomic_test_and_set_int(&value, expected, newval) != 0; }

    inline bool testAndSetAcquire(int expected, int newval)
    { return q_atomic_test_and_set_acquire_int(&value, expected, newval) != 0; }

    inline bool testAndSetRelease(int expected, int newval)
    { return q_atomic_test_and_set_release_int(&value, expected, newval) != 0; }

    inline int exchange(int newval)
    { return q_atomic_set_int(&value, newval); }

    inline int fetchAndAdd(int aValue)
    { return q_atomic_fetch_and_add_int(&value, aValue); }

    inline int fetchAndAddAcquire(int aValue)
    { return q_atomic_fetch_and_add_acquire_int(&value, aValue); }

    inline int fetchAndAddRelease(int aValue)
    { return q_atomic_fetch_and_add_release_int(&value, aValue); }
};

template <typename T>
struct QBasicAtomicPointer
{
    volatile T *value;

    void init(T *t = 0)
    { value = t; }

    inline bool operator==(T *t) const
    { return value == t; }

    inline bool operator!=(T *t) const
    { return !operator==(t); }

    inline bool operator!() const
    { return operator==(0); }

    inline operator T *() const
    { return const_cast<T *>(value); }

    inline T *operator->() const
    { return const_cast<T *>(value); }

    inline QBasicAtomicPointer<T> &operator=(T *t)
    {
        value = t;
        return *this;
    }

    inline bool testAndSet(T *expected, T *newval)
    { return q_atomic_test_and_set_ptr(&value, expected, newval); }

    inline T *exchange(T * newval)
    { return static_cast<T *>(q_atomic_set_ptr(&value, newval)); }
};

#define Q_ATOMIC_INIT(a) { (a) }

#endif // Q_SPECIALIZED_QATOMIC

#if defined(QT_MOC) || defined(QT_BUILD_QMAKE) || defined(QT_RCC) || defined(QT_UIC)
// avoid aliasing problems when using the 'generic' arch
template <typename T>
inline T qAtomicSetPtr(T *ptr, T newval)
{ T ret = *ptr; *ptr = newval; return ret; }
#else
template <typename T>
inline T qAtomicSetPtr(volatile T *ptr, T newval)
{ return static_cast<T>(q_atomic_set_ptr(ptr, newval)); }
#endif

// High-level atomic integer operations
class QAtomic : public QBasicAtomic
{
public:
    inline QAtomic(int x = 0)
    { init(x); }
    inline QAtomic(const QAtomic &copy)
    { init(copy); }

    inline QAtomic &operator=(int x)
    {
        (void) QBasicAtomic::operator=(x);
        return *this;
    }

    inline QAtomic &operator=(const QAtomic &copy)
    {
        (void) QBasicAtomic::operator=(copy);
        return *this;
    }
};

// High-level atomic pointer operations
template <typename T>
class QAtomicPointer : public QBasicAtomicPointer<T>
{
public:
    inline QAtomicPointer(T *t = 0)
    { init(t); }
    inline QAtomicPointer(const QAtomicPointer<T> &copy)
    { init(copy); }

    inline QAtomicPointer<T> &operator=(T *t)
    {
        (void) QBasicAtomicPointer<T>::operator=(t);
        return *this;
    }

    inline QAtomicPointer<T> &operator=(const QAtomicPointer<T> &copy)
    {
        (void) QBasicAtomicPointer<T>::operator=(copy);
        return *this;
    }
};

/*!
    This is a helper for the assignment operators of implicitly
    shared classes. Your assignment operator should look like this:

    \code
        MyClass &MyClass:operator=(const MyClass &other)
        { qAtomicAssign(d, other.d); return *this; }
    \endcode
*/
template <typename T>
inline void qAtomicAssign(T *&d, T *x)
{
    if (d == x)
        return;
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
}

/*!
    This is a helper for the detach method of implicitly shared
    classes. Your private class needs a copy constructor which copies
    the members and sets the refcount to 1. After that, your detach
    function should look like this:

    \code
        void MyClass::detach()
        { qAtomicDetach(d); }
    \endcode
*/
template <typename T>
inline void qAtomicDetach(T *&d)
{
    if (d->ref == 1)
        return;
    T *x = d;
    d = new T(*d);
    if (!x->ref.deref())
        delete x;
}

QT_END_HEADER

#endif // QATOMIC_H
