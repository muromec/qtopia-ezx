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

#ifndef ALPHA_QATOMIC_H
#define ALPHA_QATOMIC_H

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

#if defined(Q_CC_GNU)

inline int q_atomic_test_and_set_int(volatile int *ptr, int expected, int newval)
{
    register int ret;
    asm volatile("1:\n"
                 "ldl_l %0,%1\n"   /* ret=*ptr;                               */
                 "cmpeq %0,%2,%0\n"/* if (ret==expected) ret=0; else ret=1;   */
                 "beq   %0,3f\n"   /* if (ret==0) goto 3;                     */
                 "mov   %3,%0\n"   /* ret=newval;                             */
                 "stl_c %0,%1\n"   /* if ((*ptr=ret)!=ret) ret=0; else ret=1; */
                 "beq   %0,2f\n"   /* if (ret==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (ret), "+m" (*ptr)
                 : "r" (expected), "r" (newval)
                 : "memory");
    return ret;
}

inline int q_atomic_test_and_set_acquire_int(volatile int *ptr, int expected, int newval)
{
    register int ret;
    asm volatile("1:\n"
                 "ldl_l %0,%1\n"   /* ret=*ptr;                               */
                 "cmpeq %0,%2,%0\n"/* if (ret==expected) ret=0; else ret=1;   */
                 "beq   %0,3f\n"   /* if (ret==0) goto 3;                     */
                 "mov   %3,%0\n"   /* ret=newval;                             */
                 "stl_c %0,%1\n"   /* if ((*ptr=ret)!=ret) ret=0; else ret=1; */
                 "beq   %0,2f\n"   /* if (ret==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 "mb\n"
                 : "=&r" (ret), "+m" (*ptr)
                 : "r" (expected), "r" (newval)
                 : "memory");
    return ret;
}

inline int q_atomic_test_and_set_release_int(volatile int *ptr, int expected, int newval)
{
    register int ret;
    asm volatile("mb\n"
                 "1:\n"
                 "ldl_l %0,%1\n"   /* ret=*ptr;                               */
                 "cmpeq %0,%2,%0\n"/* if (ret==expected) ret=0; else ret=1;   */
                 "beq   %0,3f\n"   /* if (ret==0) goto 3;                     */
                 "mov   %3,%0\n"   /* ret=newval;                             */
                 "stl_c %0,%1\n"   /* if ((*ptr=ret)!=ret) ret=0; else ret=1; */
                 "beq   %0,2f\n"   /* if (ret==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (ret), "+m" (*ptr)
                 : "r" (expected), "r" (newval)
                 : "memory");
    return ret;
}

inline int q_atomic_test_and_set_ptr(volatile void *ptr, void *expected, void *newval)
{
    register void *ret;
    asm volatile("1:\n"
                 "ldq_l %0,%1\n"   /* ret=*ptr;                               */
                 "cmpeq %0,%2,%0\n"/* if (ret==expected) tmp=0; else tmp=1;   */
                 "beq   %0,3f\n"   /* if (tmp==0) goto 3;                     */
                 "mov   %3,%0\n"   /* tmp=newval;                             */
                 "stq_c %0,%1\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %0,2f\n"   /* if (ret==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (ret), "+m" (*reinterpret_cast<volatile long *>(ptr))
                 : "r" (expected), "r" (newval)
                 : "memory");
    return static_cast<int>(reinterpret_cast<long>(ret));
}

inline int q_atomic_increment(volatile int *ptr)
{
    register int old, tmp;
    asm volatile("1:\n"
                 "ldl_l %0,%2\n"   /* old=*ptr;                               */
                 "addl  %0,1,%1\n" /* tmp=old+1;                              */
                 "stl_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp == 0) goto 2;                   */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m"(*ptr)
                 :
                 : "memory");
    return old != -1;
}

inline int q_atomic_decrement(volatile int *ptr)
{
    register int old, tmp;
    asm volatile("1:\n"
                 "ldl_l %0,%2\n"   /* old=*ptr;                               */
                 "subl  %0,1,%1\n" /* tmp=old-1;                              */
                 "stl_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m"(*ptr)
                 :
                 : "memory");
    return old != 1;
}

inline int q_atomic_set_int(volatile int *ptr, int newval)
{
    register int old, tmp;
    asm volatile("1:\n"
                 "ldl_l %0,%2\n"   /* old=*ptr;                               */
                 "mov   %3,%1\n"   /* tmp=newval;                             */
                 "stl_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m" (*ptr)
                 : "r" (newval)
                 : "memory");
    return old;
}

inline void *q_atomic_set_ptr(volatile void *ptr, void *newval)
{
    register void *old, *tmp;
    asm volatile("1:\n"
                 "ldq_l %0,%2\n"   /* old=*ptr;                               */
                 "mov   %3,%1\n"   /* tmp=newval;                             */
                 "stq_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m" (*reinterpret_cast<volatile long *>(ptr))
                 : "r" (newval)
                 : "memory");
    return old;
}

inline int q_atomic_fetch_and_add_int(volatile int *ptr, int value)
{
    register int old, tmp;
    asm volatile("1:\n"
                 "ldl_l %0,%2\n"   /* old=*ptr;                               */
                 "addl  %0,%3,%1\n"/* tmp=old+value;                          */
                 "stl_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp == 0) goto 2;                   */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m"(*ptr)
                 : "r" (value)
                 : "memory");
    return old;
}

inline int q_atomic_fetch_and_add_acquire_int(volatile int *ptr, int value)
{
    register int old, tmp;
    asm volatile("1:\n"
                 "ldl_l %0,%2\n"   /* old=*ptr;                               */
                 "addl  %0,%3,%1\n"/* tmp=old+value;                          */
                 "stl_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp == 0) goto 2;                   */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 "mb\n"
                 : "=&r" (old), "=&r" (tmp), "+m"(*ptr)
                 : "r" (value)
                 : "memory");
    return old;
}

inline int q_atomic_fetch_and_add_release_int(volatile int *ptr, int value)
{
    register int old, tmp;
    asm volatile("mb\n"
                 "1:\n"
                 "ldl_l %0,%2\n"   /* old=*ptr;                               */
                 "addl  %0,%3,%1\n"/* tmp=old+value;                          */
                 "stl_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp == 0) goto 2;                   */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m"(*ptr)
                 : "r" (value)
                 : "memory");
    return old;
}

#else // !Q_CC_GNU

extern "C" {
    Q_CORE_EXPORT int q_atomic_test_and_set_int(volatile int *ptr, int expected, int newval);
    Q_CORE_EXPORT int q_atomic_test_and_set_acquire_int(volatile int *ptr, int expected, int newval);
    Q_CORE_EXPORT int q_atomic_test_and_set_release_int(volatile int *ptr, int expected, int newval);
    Q_CORE_EXPORT int q_atomic_test_and_set_ptr(volatile void *ptr, void *expected, void *newval);
    Q_CORE_EXPORT int q_atomic_increment(volatile int *ptr);
    Q_CORE_EXPORT int q_atomic_decrement(volatile int *ptr);
    Q_CORE_EXPORT int q_atomic_set_int(volatile int *ptr, int newval);
    Q_CORE_EXPORT void *q_atomic_set_ptr(volatile void *ptr, void *newval);
    Q_CORE_EXPORT int q_atomic_fetch_and_add_int(volatile int *ptr, int value);
    Q_CORE_EXPORT int q_atomic_fetch_and_add_acquire_int(volatile int *ptr, int value);
    Q_CORE_EXPORT int q_atomic_fetch_and_add_release_int(volatile int *ptr, int value);
} // extern "C"

#endif // Q_CC_GNU

QT_END_HEADER

#endif // ALPHA_QATOMIC_H
