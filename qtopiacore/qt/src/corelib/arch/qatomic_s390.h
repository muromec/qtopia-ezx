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

#ifndef S390_QATOMIC_H
#define S390_QATOMIC_H

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

#define __CS_LOOP(ptr, op_val, op_string) ({				\
	volatile int old_val, new_val;					\
        __asm__ __volatile__("   l     %0,0(%3)\n"			\
                             "0: lr    %1,%0\n"				\
                             op_string "  %1,%4\n"			\
                             "   cs    %0,%1,0(%3)\n"			\
                             "   jl    0b"				\
                             : "=&d" (old_val), "=&d" (new_val),	\
			       "=m" (*ptr)	\
			     : "a" (ptr), "d" (op_val),			\
			       "m" (*ptr)	\
			     : "cc", "memory" );			\
	new_val;							\
})

#define __CS_OLD_LOOP(ptr, op_val, op_string) ({			\
	volatile int old_val, new_val;					\
        __asm__ __volatile__("   l     %0,0(%3)\n"			\
                             "0: lr    %1,%0\n"				\
                             op_string "  %1,%4\n"			\
                             "   cs    %0,%1,0(%3)\n"			\
                             "   jl    0b"				\
                             : "=&d" (old_val), "=&d" (new_val),	\
			       "=m" (*ptr)	\
			     : "a" (ptr), "d" (op_val),			\
			       "m" (*ptr)	\
			     : "cc", "memory" );			\
	old_val;							\
})

#ifdef __s390x__
#define __CSG_OLD_LOOP(ptr, op_val, op_string) ({				\
	long old_val, new_val;						\
        __asm__ __volatile__("   lg    %0,0(%3)\n"			\
                             "0: lgr   %1,%0\n"				\
                             op_string "  %1,%4\n"			\
                             "   csg   %0,%1,0(%3)\n"			\
                             "   jl    0b"				\
                             : "=&d" (old_val), "=&d" (new_val),	\
			       "=m" (*ptr)	\
			     : "a" (ptr), "d" (op_val),			\
			       "m" (*ptr)	\
			     : "cc", "memory" );			\
	old_val;							\
})
#endif

inline int q_atomic_test_and_set_int(volatile int *ptr, int expected, int newval)
{
        int retval;

        __asm__ __volatile__(
                "  lr   %0,%3\n"
                "  cs   %0,%4,0(%2)\n"
                "  ipm  %0\n"
                "  srl  %0,28\n"
                "0:"
                : "=&d" (retval), "=m" (*ptr)
                : "a" (ptr), "d" (expected) , "d" (newval),
		  "m" (*ptr) : "cc", "memory" );

        if(retval) return 0;
        else return 1;
}

inline int q_atomic_test_and_set_acquire_int(volatile int *ptr, int expected, int newval)
{
        int retval;

        __asm__ __volatile__(
                "  lr   %0,%3\n"
                "  cs   %0,%4,0(%2)\n"
                "  ipm  %0\n"
                "  srl  %0,28\n"
                "0:\n"
                "  bcr 15,0\n"
                : "=&d" (retval), "=m" (*ptr)
                : "a" (ptr), "d" (expected) , "d" (newval),
		  "m" (*ptr) : "cc", "memory" );

        if(retval) return 0;
        else return 1;
}

inline int q_atomic_test_and_set_release_int(volatile int *ptr, int expected, int newval)
{
        int retval;

        __asm__ __volatile__(
                "  bcr 15,0\n"
                "  lr   %0,%3\n"
                "  cs   %0,%4,0(%2)\n"
                "  ipm  %0\n"
                "  srl  %0,28\n"
                "0:"
                : "=&d" (retval), "=m" (*ptr)
                : "a" (ptr), "d" (expected) , "d" (newval),
		  "m" (*ptr) : "cc", "memory" );

        if(retval) return 0;
        else return 1;
}


inline int q_atomic_test_and_set_ptr(volatile void *ptr, void *expected, void* newval)
{
        int retval;

#ifndef __s390x__
        __asm__ __volatile__(
                "  lr   %0,%3\n"
                "  cs   %0,%4,0(%2)\n"
                "  ipm  %0\n"
                "  srl  %0,28\n"
                "0:"
                : "=&d" (retval), "=m" (*reinterpret_cast<void * volatile *>(ptr))
                : "a" (ptr), "d" (expected) , "d" (newval),
		  "m" (*reinterpret_cast<void* volatile *>(ptr)) : "cc", "memory" );
#else
        __asm__ __volatile__(
                "  lgr   %0,%3\n"
                "  csg   %0,%4,0(%2)\n"
                "  ipm  %0\n"
                "  srl  %0,28\n"
                "0:"
                : "=&d" (retval), "=m" (*reinterpret_cast<void * volatile *>(ptr))
                : "a" (ptr), "d" (expected) , "d" (newval),
		  "m" (*reinterpret_cast<void* volatile *>(ptr)) : "cc", "memory" );
#endif

        if(retval) return 0;
        else return 1;
}

inline int q_atomic_increment(volatile int *ptr)
{ return __CS_LOOP(ptr, 1, "ar"); }

inline int q_atomic_decrement(volatile int *ptr)
{ return __CS_LOOP(ptr, 1, "sr"); }

inline int q_atomic_set_int(volatile int *ptr, int newval)
{
    return __CS_OLD_LOOP(ptr, newval, "lr");
}

inline void *q_atomic_set_ptr(volatile void *ptr, void *newval)
{
#ifndef __s390x__
    return (void*)__CS_OLD_LOOP(reinterpret_cast<volatile long*>(ptr), (int)newval, "lr");
#else
    return (void*)__CSG_OLD_LOOP(reinterpret_cast<volatile long*>(ptr), (long)newval, "lgr");
#endif
}

#error "fetch-and-add not implemented"
// int q_atomic_fetch_and_add_int(volatile int *ptr, int value);
// int q_atomic_fetch_and_add_acquire_int(volatile int *ptr, int value);
// int q_atomic_fetch_and_add_release_int(volatile int *ptr, int value);

QT_END_HEADER

#endif // S390_QATOMIC_H
