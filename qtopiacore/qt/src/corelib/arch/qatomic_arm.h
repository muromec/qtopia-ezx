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

#ifndef ARM_QATOMIC_H
#define ARM_QATOMIC_H

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

extern Q_CORE_EXPORT char q_atomic_lock;

inline char q_atomic_swp(volatile char *ptr, char newval)
{
    register int ret;
    asm volatile("swpb %0,%2,[%3]"
                 : "=&r"(ret), "=m" (*ptr)
                 : "r"(newval), "r"(ptr)
                 : "cc", "memory");
    return ret;
}

inline int q_atomic_test_and_set_int(volatile int *ptr, int expected, int newval)
{
    int ret = 0;
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0);
    if (*ptr == expected) {
	*ptr = newval;
	ret = 1;
    }
    q_atomic_swp(&q_atomic_lock, 0);
    return ret;
}

inline int q_atomic_test_and_set_acquire_int(volatile int *ptr, int expected, int newval)
{
    return q_atomic_test_and_set_int(ptr, expected, newval);
}

inline int q_atomic_test_and_set_release_int(volatile int *ptr, int expected, int newval)
{
    return q_atomic_test_and_set_int(ptr, expected, newval);
}

inline int q_atomic_test_and_set_ptr(volatile void *ptr, void *expected, void *newval)
{
    int ret = 0;
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0) ;
    if (*reinterpret_cast<void * volatile *>(ptr) == expected) {
	*reinterpret_cast<void * volatile *>(ptr) = newval;
	ret = 1;
    }
    q_atomic_swp(&q_atomic_lock, 0);
    return ret;
}

inline int q_atomic_increment(volatile int *ptr)
{
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0) ;
    int originalValue = *ptr;
    *ptr = originalValue + 1;
    q_atomic_swp(&q_atomic_lock, 0);
    return originalValue != -1;
}

inline int q_atomic_decrement(volatile int *ptr)
{
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0) ;
    int originalValue = *ptr;
    *ptr = originalValue - 1;
    q_atomic_swp(&q_atomic_lock, 0);
    return originalValue != 1;
}

inline int q_atomic_set_int(volatile int *ptr, int newval)
{
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0) ;
    int originalValue = *ptr;
    *ptr = newval;
    q_atomic_swp(&q_atomic_lock, 0);
    return originalValue;
}

inline void *q_atomic_set_ptr(volatile void *ptr, void *newval)
{
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0) ;
    void *originalValue = *reinterpret_cast<void * volatile *>(ptr);
    *reinterpret_cast<void * volatile *>(ptr) = newval;
    q_atomic_swp(&q_atomic_lock, 0);
    return originalValue;
}

inline int q_atomic_fetch_and_add_int(volatile int *ptr, int value)
{
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0) ;
    int originalValue = *ptr;
    *ptr += value;
    q_atomic_swp(&q_atomic_lock, 0);
    return originalValue;
}

inline int q_atomic_fetch_and_add_acquire_int(volatile int *ptr, int value)
{
    return q_atomic_fetch_and_add_int(ptr, value);
}

inline int q_atomic_fetch_and_add_release_int(volatile int *ptr, int value)
{
    return q_atomic_fetch_and_add_int(ptr, value);
}

QT_END_HEADER

#endif // ARM_QATOMIC_H
