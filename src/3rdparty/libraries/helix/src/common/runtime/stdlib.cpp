/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: stdlib.cpp,v 1.12 2008/01/18 09:17:26 vkathuria Exp $
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
#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"
#include "hlxclib/windows.h"
#include "hxassert.h"

char * __helix_itoa(int val, char *str, int radix)
{
    char IsNegative = 0;
    int theNum = val;
    int StrIndex = 0;
    
    if (theNum < 0) {
	theNum = -theNum;
	IsNegative = 1;
    }
    
    do {
	int CurDigit = theNum % radix;
	if (CurDigit > 9)
	    str[StrIndex++] = (char)(CurDigit + 'A' - 10);
	else
	    str[StrIndex++] = (char)(CurDigit + '0');
	
	theNum /= radix;
    } while (theNum);
    
    if (IsNegative) {
	str[StrIndex++] = '-';
    }
    str[StrIndex++] = 0;
    
    // Now reverse the string.
    strrev(str);
    
    return str;
}

char * __helix_i64toa(INT64 val, char *str, int radix)
{
    char IsNegative = 0;
    INT64 theNum = val;
    int StrIndex = 0;

    if (theNum < (INT64)0) {
	theNum = -theNum;
	IsNegative = 1;
    }
	
    do {
	int CurDigit = INT64_TO_INT32(theNum % radix);
	if (CurDigit > 9)
	    str[StrIndex++] = (char)(CurDigit + 'A' - 10);
	else
	    str[StrIndex++] = (char)(CurDigit + '0');
		
	theNum /= radix;
    } while (theNum!=0);
		
    if (IsNegative) {
	str[StrIndex++] = '-';
    }
    str[StrIndex++] = 0;
	
    // Now reverse the string.
    strrev(str);
	
    return str;
}
 
/*
 * Note: This is a fast but very basic implementation of atoi64. If you need 
 * something more robust in the future, consider replacing this with the 
 * following FreeBSD function:
 *
 * /usr/src/lib/libc/stdlib/strtoq.c
 */
INT64 __helix_atoi64(char* str)
{
    char IsNegative = 0;
    INT64 lTotal = 0;
    
    if (str)
    {
	if (*str == '-')
	{
	    IsNegative = 1;
	    str++;
	}
	else if (*str == '+')
	{
	    str++;
	}
	
	for ( ; *str; str++)
	{
	    if (*str < '0' || *str > '9')
	    {
		break;
	    }
	    
	    lTotal *= 10;
	    lTotal += (*str - '0');
	}
    }
    
    if (IsNegative)
    {
	lTotal = -lTotal;
    }

    return lTotal;
}

#if defined(WIN32_PLATFORM_PSPC)
int __helix_remove(const char* pPath)
{
    int ret = -1;

    if (GetFileAttributes(OS_STRING(pPath)) & FILE_ATTRIBUTE_DIRECTORY)
    {
	if (RemoveDirectory(OS_STRING(pPath)))
	    ret = 0;
    }
    else if (DeleteFile(OS_STRING(pPath)))
	ret = 0;

    return ret;
}
#endif /* defined(WIN32_PLATFORM_PSPC) */

#if defined (_OPENWAVE)
int __helix_remove(const char* pPath)
{
    int ret = -1;

    if (kOpFsErrAny != OpFsRemove(pPath))
    {
        return ret = 0;
    }
    return ret;
}
int __helix_putenv(const char* pStr)
{
    int ret = -1;
    assert(!"__helix_putenv(): Not implemented on OPENWAVE yet\n");
    return ret;
}
#endif // defined(_OPENWAVE)

#if defined (_SYMBIAN)
int __helix_putenv(const char* pStr)
{
    int ret = -1;

    if (pStr)
    {
	char* pTmpBuf = new_string(pStr);
	char* pCur = pTmpBuf;
	const char* pKey = pCur;
	const char* pValue = 0;

	int done = 0;
	while(!done)
	{
	    // Look for the end of the key
	    for (;*pCur && (*pCur != '='); pCur++)
		;

	    
	    if (*pCur == '=')
	    {
		// Replace the '=' with a null terminator
		*pCur++ = '\0';

		// Store where the value starts
		pValue = pCur;

		// Look for the end of the value
		for (;*pCur && (*pCur != ';'); pCur++)
		    ;

		if (*pCur == ';')
		{
		    // Replace the ';' with a null terminator
		    *pCur++ = '\0';
		}
		else
		    done = 1;

		ret = setenv(pKey, pValue, 1);

		if (!done)
		    pKey = pCur;
	    }
	    else
		done = 1;
	}

	delete [] pTmpBuf;
    }

    return ret;
}
#endif /* defined (_SYMBIAN) */

// The Helix implementation will act
// like the environment variable doesn't
// exist
char* __helix_getenv(const char* pName)
{
    return 0;
}

#if defined(_BREW)

void __helix_srand(unsigned int seed)
{
    HX_ASSERT(0);
}

int __helix_rand()
{
    HX_ASSERT(0);	
    return 0;
}

#endif //_BREW
