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

#ifndef BOUNDSCHECKER_QATOMIC_H
#define BOUNDSCHECKER_QATOMIC_H

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

__forceinline int q_atomic_xchg(int newval)
{
    static int lockvar = 0;
    int *pointer = &lockvar;

    __asm {
        mov EDX,pointer
        mov ECX,newval
        xchg dword ptr[EDX],ECX
        mov newval,ECX
    }
    return newval;
}

inline void q_atomic_lock()
{
    while (q_atomic_xchg(~0) != 0)
        ;
}

inline void q_atomic_unlock()
{
    q_atomic_xchg(0);
}

inline int q_atomic_test_and_set_int(volatile int *pointer, int expected, int newval)
{
    q_atomic_lock();
    if (*pointer == expected) {
        *pointer = newval;
        q_atomic_unlock();
        return 1;
    }
    q_atomic_unlock();
    return 0;
}

inline int q_atomic_test_and_set_acquire_int(volatile int *ptr, int expected, int newval)
{
    return q_atomic_test_and_set_int(ptr, expected, newval);
}

inline int q_atomic_test_and_set_release_int(volatile int *ptr, int expected, int newval)
{
    return q_atomic_test_and_set_int(ptr, expected, newval);
}

inline int q_atomic_test_and_set_ptr(volatile void *pointer, void *expected, void *newval)
{
    q_atomic_lock();
    if (*reinterpret_cast<void * volatile *>(pointer) == expected) {
        *reinterpret_cast<void * volatile *>(pointer) = newval;
        q_atomic_unlock();
        return 1;
    }
    q_atomic_unlock();
    return 0;
}

inline int q_atomic_increment(volatile int *pointer)
{
    q_atomic_lock();
    int ret = ++(*pointer);
    q_atomic_unlock();
    return ret;
}

inline int q_atomic_decrement(volatile int *pointer)
{
    q_atomic_lock();
    int ret = --(*pointer);
    q_atomic_unlock();
    return ret;
}

inline int q_atomic_set_int(volatile int *pointer, int newval)
{
    q_atomic_lock();
    int ret = *pointer;
    *pointer = newval;
    q_atomic_unlock();
    return ret;
}

inline void *q_atomic_set_ptr(volatile void *pointer, void *newval)
{
    q_atomic_lock();
    void *ret = *reinterpret_cast<void * volatile *>(pointer);
    *reinterpret_cast<void * volatile *>(pointer) = newval;
    q_atomic_unlock();
    return ret;
}

#error "fetch-and-add not implemented"
// int q_atomic_fetch_and_add_int(volatile int *ptr, int value);
// int q_atomic_fetch_and_add_acquire_int(volatile int *ptr, int value);
// int q_atomic_fetch_and_add_release_int(volatile int *ptr, int value);

QT_END_HEADER

#endif // BOUNDSCHECKER_QATOMIC_H
