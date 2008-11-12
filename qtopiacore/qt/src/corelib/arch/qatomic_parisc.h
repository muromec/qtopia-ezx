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

#ifndef PARISC_QATOMIC_H
#define PARISC_QATOMIC_H

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

extern "C" {
    Q_CORE_EXPORT void q_atomic_lock(int *lock);
    Q_CORE_EXPORT void q_atomic_unlock(int *lock);
    Q_CORE_EXPORT int q_atomic_test_and_set_ptr(volatile void *ptr, void *expected, void *newval);
    Q_CORE_EXPORT void *q_atomic_set_ptr(volatile void *ptr, void *newval);
}

#define Q_SPECIALIZED_QATOMIC

struct QBasicAtomic
{
    int lock[4];
    int atomic;

    inline void init(int x = 0)
    {
        lock[0] = lock[1] = lock[2] = lock[3] = -1; atomic = x;
    }

    inline bool ref()
    {
	q_atomic_lock(lock);
	bool ret = (++atomic != 0);
	q_atomic_unlock(lock);
	return ret;
    }

    inline bool deref()
    {
	q_atomic_lock(lock);
	bool ret = (--atomic != 0);
	q_atomic_unlock(lock);
	return ret;
    }

    inline bool operator==(int x) const
    { return atomic == x; }

    inline bool operator!=(int x) const
    { return atomic != x; }

    inline bool operator!() const
    { return atomic == 0; }

    inline QBasicAtomic &operator=(int x)
    {
        atomic = x;
        return *this;
    }

    inline operator int() const
    { return atomic; }

    inline bool testAndSet(int expected, int newval)
    {
	q_atomic_lock(lock);
	if (atomic == expected) {
            atomic = newval;
	    q_atomic_unlock(lock);
	    return true;
        }
	q_atomic_unlock(lock);
	return false;
    }

    inline bool testAndSetAcquire(int expected, int newval)
    {
        return testAndSet(expected, newval);
    }

    inline bool testAndSetRelease(int expected, int newval)
    {
        return testAndSet(expected, newval);
    }

    inline int exchange(int newval)
    {
	q_atomic_lock(lock);
	int oldval = atomic;
	atomic = newval;
	q_atomic_unlock(lock);
	return oldval;
    }

    inline int fetchAndAdd(int value)
    {
	q_atomic_lock(lock);
        int originalValue = atomic;
        atomic += value;
	q_atomic_unlock(lock);
	return originalValue;
    }

    inline int fetchAndAddAcquire(int value)
    {
        return fetchAndAdd(value);
    }

    inline int fetchAndAddRelease(int value)
    {
        return fetchAndAdd(value);
    }
};

template <typename T>
struct QBasicAtomicPointer
{
    int lock[4];
    volatile T *pointer;

    inline void init(T *t = 0)
    {
        lock[0] = lock[1] = lock[2] = lock[3] = -1; pointer = t;
    }

    inline bool operator==(T *x) const
    {
	return pointer == x;
    }

    inline bool operator!=(T *x) const
    {
	return pointer != x;
    }

    inline bool operator!() const
    { return operator==(0); }

    inline QBasicAtomicPointer<T> &operator=(T *t)
    {
        pointer = const_cast<T *>(t);
        return *this;
    }

    inline T *operator->()
    { return const_cast<T *>(pointer); }
    inline const T *operator->() const
    { return pointer; }

    inline operator T*() const
    { return const_cast<T *>(pointer); }

    inline bool testAndSet(T* expected, T *newval)
    {
	q_atomic_lock(lock);
	if (pointer == expected) {
	    pointer = newval;
	    q_atomic_unlock(lock);
	    return true;
	}
	q_atomic_unlock(lock);
	return false;
    }

    inline T *exchange(T *newval)
    {
	q_atomic_lock(lock);
	T *oldval = const_cast<T *>(pointer);
	pointer = newval;
	q_atomic_unlock(lock);
	return oldval;
    }
};

#define Q_ATOMIC_INIT(a) {{-1,-1,-1,-1},(a)}

QT_END_HEADER

#endif // PARISC_QATOMIC_H
