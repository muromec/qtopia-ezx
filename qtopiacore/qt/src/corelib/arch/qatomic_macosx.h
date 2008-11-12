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

#ifndef MACOSX_QATOMIC_H
#define MACOSX_QATOMIC_H

#if defined(__x86_64__)
#  include <QtCore/qatomic_x86_64.h>
#elif defined(__i386__)
#  include <QtCore/qatomic_i386.h>
#else // !__x86_64 && !__i386__

#include <QtCore/qglobal.h>

// Use the functions in OSAtomic.h if we are in 64-bit mode. This header is
// unfortunately not available on 10.3, so we can't use it in 32-bit
// mode. (64-bit is not supported on 10.3.)
#if defined (__LP64__)
#include <libkern/OSAtomic.h>
#endif

QT_BEGIN_HEADER

#if defined (__LP64__)

inline int q_atomic_test_and_set_int(volatile int *ptr, int expected, int newval)
{
     return OSAtomicCompareAndSwap32(expected, newval, const_cast<int *>(ptr));
}

inline int q_atomic_test_and_set_acquire_int(volatile int *ptr, int expected, int newval)
{
    return OSAtomicCompareAndSwap32Barrier(expected, newval, const_cast<int *>(ptr));
}

inline int q_atomic_test_and_set_release_int(volatile int *ptr, int expected, int newval)
{
    return OSAtomicCompareAndSwap32Barrier(expected, newval, const_cast<int *>(ptr));
}

inline int q_atomic_test_and_set_ptr(volatile void *ptr, void *expected, void *newval)
{
    return OSAtomicCompareAndSwap64(reinterpret_cast<int64_t>(expected), reinterpret_cast<int64_t>(newval), reinterpret_cast<int64_t *>(const_cast<void *>(ptr)));
}

inline int q_atomic_increment(volatile int *ptr)
{ return OSAtomicIncrement32(const_cast<int *>(ptr)); }

inline int q_atomic_decrement(volatile int *ptr)
{ return OSAtomicDecrement32(const_cast<int *>(ptr)); }


inline int q_atomic_set_int(volatile int *ptr, int newval)
{
    register int ret;
    do {
        ret = *ptr;
    } while (OSAtomicCompareAndSwap32(ret, newval, const_cast<int *>(ptr)) == false);
    return ret;
}

inline void *q_atomic_set_ptr(volatile void *ptr, void *newval)
{
    register int64_t ret;
    do {
        ret = *reinterpret_cast<int64_t *>(const_cast<void *>(ptr));
    } while (OSAtomicCompareAndSwap64(ret, reinterpret_cast<int64_t>(newval), reinterpret_cast<int64_t *>(const_cast<void *>(ptr))) == false);
    return reinterpret_cast<void *>(ret);
}

inline int q_atomic_fetch_and_add_int(volatile int *ptr, int value)
{
    register int ret;
    do {
        ret = *ptr;
    } while (OSAtomicCompareAndSwap32(ret, ret + value, const_cast<int *>(ptr)) == false);
    return ret;
}

inline int q_atomic_fetch_and_add_acquire_int(volatile int *ptr, int value)
{
    register int ret;
    do {
        ret = *ptr;
    } while (OSAtomicCompareAndSwap32Barrier(ret, ret + value, const_cast<int *>(ptr)) == false);
    return ret;
}

inline int q_atomic_fetch_and_add_release_int(volatile int *ptr, int value)
{
    register int ret;
    do {
        ret = *ptr;
    } while (OSAtomicCompareAndSwap32Barrier(ret, ret + value, const_cast<int *>(ptr)) == false);
    return ret;
}

#elif defined(_ARCH_PPC) || defined(Q_CC_XLC)

#if defined(Q_CC_GNU)
#ifdef __64BIT__
#  define LPARX "ldarx"
#  define CMPP  "cmpd"
#  define STPCX "stdcx."
#else
#  define LPARX "lwarx"
#  define CMPP  "cmpw"
#  define STPCX "stwcx."
#endif

inline int q_atomic_test_and_set_int(volatile int *ptr, int expected, int newval)
{
    register int tmp;
    register int ret;
    asm volatile("lwarx  %0,0,%2\n"
                 "cmpw   %0,%3\n"
                 "bne-   $+20\n"
                 "stwcx. %4,0,%2\n"
                 "bne-   $-16\n"
                 "li     %1,1\n"
                 "b      $+8\n"
                 "li     %1,0\n"
                 : "=&r" (tmp), "=&r" (ret)
                 : "r" (ptr), "r" (expected), "r" (newval)
                 : "cc", "memory");
    return ret;
}

inline int q_atomic_test_and_set_acquire_int(volatile int *ptr, int expected, int newval)
{
    register int tmp;
    register int ret;
    asm volatile("lwarx  %0,0,%2\n"
                 "cmpw   %0,%3\n"
                 "bne-   $+20\n"
                 "stwcx. %4,0,%2\n"
                 "bne-   $-16\n"
                 "li     %1,1\n"
                 "b      $+8\n"
                 "li     %1,0\n"
                 "eieio\n"
                 : "=&r" (tmp), "=&r" (ret)
                 : "r" (ptr), "r" (expected), "r" (newval)
                 : "cc", "memory");
    return ret;
}

inline int q_atomic_test_and_set_release_int(volatile int *ptr, int expected, int newval)
{
    register int tmp;
    register int ret;
    asm volatile("eieio\n"
                 "lwarx  %0,0,%2\n"
                 "cmpw   %0,%3\n"
                 "bne-   $+20\n"
                 "stwcx. %4,0,%2\n"
                 "bne-   $-16\n"
                 "li     %1,1\n"
                 "b      $+8\n"
                 "li     %1,0\n"
                 : "=&r" (tmp), "=&r" (ret)
                 : "r" (ptr), "r" (expected), "r" (newval)
                 : "cc", "memory");
    return ret;
}


inline int q_atomic_test_and_set_ptr(volatile void *ptr, void *expected, void *newval)
{
    register void *tmp;
    register int ret;
    asm volatile(LPARX"  %0,0,%2\n"
                 CMPP"   %0,%3\n"
                 "bne-   $+20\n"
                 STPCX"  %4,0,%2\n"
                 "bne-   $-16\n"
                 "li     %1,1\n"
                 "b      $+8\n"
                 "li     %1,0\n"
                 : "=&r" (tmp), "=&r" (ret)
                 : "r" (ptr), "r" (expected), "r" (newval)
                 : "cc", "memory");
    return ret;
}

inline int q_atomic_increment(volatile int *ptr)
{
    register int ret;
    register int one = 1;
    asm volatile("lwarx  %0, 0, %1\n"
                 "add    %0, %2, %0\n"
                 "stwcx. %0, 0, %1\n"
                 "bne-   $-12\n"
                 : "=&r" (ret)
                 : "r" (ptr), "r" (one)
                 : "cc", "memory");
    return ret;
}

inline int q_atomic_decrement(volatile int *ptr)
{
    register int ret;
    register int one = -1;
    asm volatile("lwarx  %0, 0, %1\n"
                 "add    %0, %2, %0\n"
                 "stwcx. %0, 0, %1\n"
                 "bne-   $-12\n"
                 : "=&r" (ret)
                 : "r" (ptr), "r" (one)
                 : "cc", "memory");
    return ret;
}

inline int q_atomic_set_int(volatile int *ptr, int newval)
{
    register int ret;
    asm volatile("lwarx  %0, 0, %1\n"
                 "stwcx. %2, 0, %1\n"
                 "bne-   $-8\n"
                 : "=&r" (ret)
                 : "r" (ptr), "r" (newval)
                 : "cc", "memory");
    return ret;
}

inline void *q_atomic_set_ptr(volatile void *ptr, void *newval)
{
    register void *ret;
    asm volatile(LPARX"  %0, 0, %1\n"
                 STPCX"  %2, 0, %1\n"
                 "bne-   $-8\n"
                 : "=&r" (ret)
                 : "r" (ptr), "r" (newval)
                 : "cc", "memory");
    return ret;
}


#undef LPARX
#undef CMPP
#undef STPCX

inline int q_atomic_fetch_and_add_int(volatile int *ptr, int value)
{
    register int tmp;
    register int ret;
    asm volatile("lwarx  %0, 0, %3\n"
                 "add    %1, %4, %0\n"
                 "stwcx. %1, 0, %3\n"
                 "bne-   $-12\n"
                 : "=&r" (ret), "=&r" (tmp), "=m" (*ptr)
                 : "r" (ptr), "r" (value)
                 : "cc", "memory");
    return ret;
}

inline int q_atomic_fetch_and_add_acquire_int(volatile int *ptr, int value)
{
    register int tmp;
    register int ret;
    asm volatile("lwarx  %0, 0, %3\n"
                 "add    %1, %4, %0\n"
                 "stwcx. %1, 0, %3\n"
                 "bne-   $-12\n"
                 "eieio\n"
                 : "=&r" (ret), "=&r" (tmp), "=m" (*ptr)
                 : "r" (ptr), "r" (value)
                 : "cc", "memory");
    return ret;
}

inline int q_atomic_fetch_and_add_release_int(volatile int *ptr, int value)
{
    register int tmp;
    register int ret;
    asm volatile("eieio\n"
                 "lwarx  %0, 0, %3\n"
                 "add    %1, %4, %0\n"
                 "stwcx. %1, 0, %3\n"
                 "bne-   $-12\n"
                 : "=&r" (ret), "=&r" (tmp), "=m" (*ptr)
                 : "r" (ptr), "r" (value)
                 : "cc", "memory");
    return ret;
}

#else // !Q_CC_GNU

// TODO: Implement TAS with acquire/release semantics.
extern "C" {
    int q_atomic_test_and_set_int(volatile int *ptr, int expected, int newval);
    int q_atomic_test_and_set_ptr(volatile void *ptr, void *expected, void *newval);
    int q_atomic_increment(volatile int *);
    int q_atomic_decrement(volatile int *);
    int q_atomic_set_int(volatile int *, int);
    void *q_atomic_set_ptr(volatile void *, void *);
    int q_atomic_test_and_set_acquire_int(volatile int *ptr, int expected, int newval);
    int q_atomic_test_and_set_release_int(volatile int *ptr, int expected, int newval);

#error "fetch-and-add not implemented"
    // int q_atomic_fetch_and_add_int(volatile int *ptr, int value);
    // int q_atomic_fetch_and_add_acquire_int(volatile int *ptr, int value);
    // int q_atomic_fetch_and_add_release_int(volatile int *ptr, int value);

} // extern "C"

#endif // Q_CC_GNU

#endif // _ARCH_PPC || Q_CC_XLC

QT_END_HEADER

#endif // !__x86_64__ && !__i386__

#endif // MACOSX_QATOMIC_H
