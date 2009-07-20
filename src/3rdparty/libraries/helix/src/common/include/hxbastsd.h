/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxbastsd.h,v 1.6 2007/07/06 20:43:41 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

#if !(defined(_MSC_VER) && (_MSC_VER > 1100))
#error hxbastsd.h is only intended for use with Microsoft VC++ 6.0 or higher
#endif

#ifndef _HXBASETSD_H_
#define _HXBASETSD_H_

#ifndef _BASETSD_H_
#define _BASETSD_H_
#endif

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_MAC) && (defined(_M_MRX000) || defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_VER >= 1100) && !(defined(MIDL_PASS) || defined(RC_INVOKED))
#define POINTER_64 __ptr64
typedef unsigned __int64 POINTER_64_INT;
#if defined(_WIN64)
#define POINTER_32 __ptr32
#else
#define POINTER_32
#endif
#else
#if defined(_MAC) && defined(_MAC_INT_64)
#define POINTER_64 __ptr64
typedef unsigned __int64 POINTER_64_INT;
#else
#if (_MSC_VER >= 1300) && !(defined(MIDL_PASS) || defined(RC_INVOKED))
#define POINTER_64 __ptr64
#else
#define POINTER_64
#endif
typedef unsigned long POINTER_64_INT;
#endif
#define POINTER_32
#endif

typedef int *PLONG32;
typedef int *PINT32;

typedef unsigned int *PULONG32;
typedef unsigned int DWORD32, *PDWORD32;
typedef unsigned int *PUINT32;

typedef __int64	SHANDLE_PTR;

#if !defined(_W64)
#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300
#define _W64 __w64
#else
#define _W64
#endif
#endif

#if ( 501 < __midl )

    typedef [public] __int3264 INT_PTR, *PINT_PTR;
    typedef [public] unsigned __int3264 UINT_PTR, *PUINT_PTR;

    typedef [public] __int3264 LONG_PTR, *PLONG_PTR;
    typedef [public] unsigned __int3264 ULONG_PTR, *PULONG_PTR;

#else  // midl64
// old midl and C++ compiler

#if defined(_WIN64)
    typedef __int64 INT_PTR, *PINT_PTR;
    typedef unsigned __int64 UINT_PTR, *PUINT_PTR;

    typedef __int64 LONG_PTR, *PLONG_PTR;
    typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;

    #define __int3264   __int64

#else
    typedef _W64 int INT_PTR, *PINT_PTR;
    typedef _W64 unsigned int UINT_PTR, *PUINT_PTR;

    typedef _W64 long LONG_PTR, *PLONG_PTR;
    typedef _W64 unsigned long ULONG_PTR, *PULONG_PTR;

    #define __int3264   __int32

#endif
#endif // midl64

//
// HALF_PTR is half the size of a pointer it intended for use with
// within strcuture which contain a pointer and two small fields.
// UHALF_PTR is the unsigned variation.
//

#ifdef _WIN64

#define ADDRESS_TAG_BIT 0x40000000000UI64

typedef unsigned __int64 HANDLE_PTR;
typedef unsigned int UHALF_PTR, *PUHALF_PTR;
typedef int HALF_PTR, *PHALF_PTR;

#pragma warning(disable:4311)   // type cast truncation
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4305)   // type cast truncation

#if !defined(__midl)
__inline
unsigned long
HandleToUlong(
    const void *h
    )
{
    return((unsigned long) h );
}

__inline
long
HandleToLong(
    const void *h
    )
{
    return((long) h );
}

__inline
void *
LongToHandle(
    const long h
    )
{
    return((void *) (INT_PTR) h );
}


__inline
unsigned long
PtrToUlong(
    const void  *p
    )
{
    return((unsigned long) p );
}

__inline
unsigned int
PtrToUint(
    const void  *p
    )
{
    return((unsigned int) p );
}

__inline
unsigned short
PtrToUshort(
    const void  *p
    )
{
    return((unsigned short) p );
}

__inline
long
PtrToLong(
    const void  *p
    )
{
    return((long) p );
}

__inline
int
PtrToInt(
    const void  *p
    )
{
    return((int) p );
}

__inline
short
PtrToShort(
    const void  *p
    )
{
    return((short) p );
}

__inline
void *
IntToPtr(
    const int i
	)
// Caution: IntToPtr() sign-extends the int value.
{
    return( (void *)(INT_PTR)i );
}

__inline
void *
UIntToPtr(
    const unsigned int ui
    )
// Caution: UIntToPtr() zero-extends the unsigned int value.
{
    return( (void *)(UINT_PTR)ui );
}

__inline
void *
LongToPtr(
    const long l
    )
// Caution: LongToPtr() sign-extends the long value.
{
    return( (void *)(LONG_PTR)l );
}

__inline
void *
ULongToPtr(
    const unsigned long ul
    )
// Caution: ULongToPtr() zero-extends the unsigned long value.
{
    return( (void *)(ULONG_PTR)ul );
}

#endif // !_midl
#if _MSC_VER >= 1200
#pragma warning(pop)      // restore 4305
#endif
#pragma warning(3:4311)   // bump 4311 to a level 3

#else  // !_WIN64

#define ADDRESS_TAG_BIT 0x80000000UL

typedef unsigned short UHALF_PTR, *PUHALF_PTR;
typedef short HALF_PTR, *PHALF_PTR;
typedef _W64 unsigned long HANDLE_PTR;

#define HandleToUlong( h ) ((ULONG)(ULONG_PTR)(h) )
#define HandleToLong( h ) ((LONG)(LONG_PTR) (h) )
#define LongToHandle( h) ((HANDLE)(LONG_PTR) (h))
#define PtrToUlong( p ) ((ULONG)(ULONG_PTR) (p) )
#define PtrToLong( p ) ((LONG)(LONG_PTR) (p) )
#define PtrToUint( p ) ((UINT)(UINT_PTR) (p) )
#define PtrToInt( p ) ((INT)(INT_PTR) (p) )
#define PtrToUshort( p ) ((unsigned short)(ULONG_PTR)(p) )
#define PtrToShort( p ) ((short)(LONG_PTR)(p) )
#define IntToPtr( i )    ((VOID *)(INT_PTR)((int)i))
#define UIntToPtr( ui )  ((VOID *)(UINT_PTR)((unsigned int)ui))
#define LongToPtr( l )   ((VOID *)(LONG_PTR)((long)l))
#define ULongToPtr( ul )  ((VOID *)(ULONG_PTR)((unsigned long)ul))

#endif // !_WIN64

#define UlongToPtr(ul) ULongToPtr(ul)
#define UintToPtr(ui) UIntToPtr(ui)

#define MAXUINT_PTR  (~((UINT_PTR)0))
#define MAXINT_PTR   ((INT_PTR)(MAXUINT_PTR >> 1))
#define MININT_PTR   (~MAXINT_PTR)

#define MAXULONG_PTR (~((ULONG_PTR)0))
#define MAXLONG_PTR  ((LONG_PTR)(MAXULONG_PTR >> 1))
#define MINLONG_PTR  (~MAXLONG_PTR)

#define MAXUHALF_PTR ((UHALF_PTR)~0)
#define MAXHALF_PTR  ((HALF_PTR)(MAXUHALF_PTR >> 1))
#define MINHALF_PTR  (~MAXHALF_PTR)

//
// SIZE_T used for counts or ranges which need to span the range of
// of a pointer.  SSIZE_T is the signed variation.
//

typedef ULONG_PTR SIZE_T, *PSIZE_T;
typedef LONG_PTR SSIZE_T, *PSSIZE_T;

//
// Add Windows flavor DWORD_PTR types
//

typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

typedef __int64 LONG64, *PLONG64;
typedef __int64 INT64,  *PINT64;

typedef unsigned __int64 ULONG64, *PULONG64;
typedef unsigned __int64 DWORD64, *PDWORD64;
typedef unsigned __int64 UINT64,  *PUINT64;

#ifdef __cplusplus
}
#endif

#endif // _HXBASETSD_H_
