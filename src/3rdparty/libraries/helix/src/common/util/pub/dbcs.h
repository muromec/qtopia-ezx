/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dbcs.h,v 1.12 2008/03/25 19:46:19 praveenkumar Exp $
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

#ifndef _DBCS_H_
#define _DBCS_H_

#include "hlxclib/string.h"

#if defined(_WIN32) && !defined(WIN32_PLATFORM_PSPC)
#include <mbctype.h>
#endif // _WIN32

#include "hxtypes.h"

// DBCS - relative functions


/************************************************************************
 * These can be optimized for Unix since HXIsDBCSEnabled
 * is always FALSE in this case.
 */

#if defined(_UNIX) || defined(_SYMBIAN)
inline HXBOOL
HXIsDBCSEnabled()
{
    return FALSE;
}

inline char*
HXGetNextChar(const char* pChar)
{
    return pChar ? (char*)(pChar + 1) : NULL;
}

inline char*
HXGetPrevChar(const char* pStart, const char* pChar)
{
    return pChar ? (char*)(pChar - 1) : NULL;
}

inline HXBOOL
HXIsEqual(const char* pChar, char CharToCompare)
{
    return pChar ? (*pChar == CharToCompare) : FALSE;
}

inline char*
HXFindChar(const char* pStart, char CharToFind)
{
    return (char*)strchr(pStart, CharToFind);
}

inline const char*
HXFindCharN(const char* pStart, char CharToFind, size_t maxlen)
{
    return (const char*)memchr(pStart, CharToFind, maxlen);
}

inline char*
HXReverseFindChar(const char* pStart, char CharToFind)
{
    return (char*)strrchr(pStart, CharToFind);
}

inline char*
HXFindString(const char* pStart, const char* pString)
{
    return (char*)strstr(pStart, pString);
}


inline int
HXCompareStrings(const char* pString1, const char* pString2, size_t count)
{
    if (!pString1 && !pString2)
        return 0;

    if (!pString1)
	return -1;

    if (!pString2)
	return 1;

    return strncmp(pString1,pString2, count);
}

inline HXBOOL
HXIsLeadByte(char Byte)
{
    return FALSE;
}

inline char*
HXCopyStringN(char* pOutput, const char* pInput, size_t count)
{
    return (char*)strncpy(pOutput, pInput, count); /* Flawfinder: ignore */
}

inline INT32
GetDBCSCodePage(void) 
{
    return 0; 
}

/************************************************************************
 * Windows and Mac
 */
#else

HXBOOL   HXIsDBCSEnabled();
char*  HXGetNextChar(const char* pChar);
char*  HXGetPrevChar(const char* pStart, const char* pChar);
HXBOOL   HXIsEqual(const char* pChar, char CharToCompare);
char*  HXFindChar(const char* pStart, char CharToFind);
const char* HXFindCharN(const char* pStart, char CharToFind, size_t maxlen);
char*  HXReverseFindChar(const char* pStart, char CharToFind);
char*  HXFindString(const char* pStart, const char* pString);
int    HXCompareStrings(const char* pString1, const char* pString2, size_t count);
HXBOOL   HXIsLeadByte(char Byte);
char*  HXCopyStringN(char* pOutput, const char* pInput, size_t count);

// it is necessary for Mac and Unix to implement GetDBCSCodePage for
// those platforms.  All that is important, really, is that zero is
// returned on a non-DBCS machine.  But the code page in use probably
// has a number, so return it.
INT32 GetDBCSCodePage(void);
#endif

#endif // _DBCS_H_
 
