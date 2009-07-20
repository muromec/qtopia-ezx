/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dbcs.cpp,v 1.18 2008/03/25 19:46:19 praveenkumar Exp $
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

#include "hxtypes.h"
#include "hlxclib/string.h"

#if defined( _WIN32 ) || defined( _WINDOWS )
#include "hlxclib/windows.h"	// for AnsiNext(), AnsiPrev()
#include <ctype.h>	// for isleadbyte()
#endif

#ifdef _MACINTOSH
#ifndef _MAC_MACHO
#include <script.h>		// for CharByte()
#endif

// These aren't defined by the OS headers, so I define them here. 
#define  MBCS_LEADBYTE      -1
#define  MBCS_SINGLEBYTE    0
#define  MBCS_LASTBYTE      1
#define  MBCS_MIDDLEBYTE    2

// Functions used to simplify the logic of moving to either end of a character.
char* MoveToLeadByte(char *pChar);
char* MoveToLastByte(char *pChar);

#endif


#include "dbcs.h"
#include "hxassert.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

/* optimized implementations for Unix are in dbcs.h */
#if !defined(_UNIX) && !defined(_SYMBIAN)

HXBOOL HXIsDBCSEnabled()
{
#if (defined( _WIN32 ) || defined( _WINDOWS )) && !defined(WIN32_PLATFORM_PSPC)
    static int DBCSEnabled = -1;
    if(DBCSEnabled < 0)
        DBCSEnabled = GetSystemMetrics(SM_DBCSENABLED); 
#elif defined(_MACINTOSH)
    int DBCSEnabled = 1;
#else
    int DBCSEnabled = 0;
#endif
    return DBCSEnabled;
}

// returns pointer to the next character in multi-bytes CS string
char*
HXGetNextChar(const char* pChar)
{
	if(!pChar)
		return NULL;

	if(!HXIsDBCSEnabled())
	    return (char *)(pChar + 1);

	char * pNext = NULL;

#if (defined( _WIN32 ) || defined( _WINDOWS )) && !defined(WIN32_PLATFORM_PSPC)
	pNext = AnsiNext(pChar);
#elif defined (_MACINTOSH)

	pNext=(char*)pChar;
	
	// If this is a multi-byte character then return the last byte of the character.
	// If it is a singbyte character then do nothing.
	pNext=MoveToLastByte(pNext);
	
	// If this is a single byte character then move to the next byte.
	// If this is a multi-byte character then we know we are at the end of the character,
	// so moving + 1 would correct.
	pNext=pNext+1;
	
#else  //unix
	pNext = (char*)pChar + 1;
#endif 

	if(pNext == pChar)
	return pNext + 1;

	return pNext;
}

// returns pointer to the previous character in multi-bytes CS string
char*
HXGetPrevChar(const char* pStart, const char* pChar)
{
	if(!pChar)
		return NULL;

	if(!HXIsDBCSEnabled())
	    return (char *)(pChar - 1);

	char * pPrev = NULL;
#if (defined( _WIN32 ) || defined( _WINDOWS )) && !defined(WIN32_PLATFORM_PSPC)
	pPrev = AnsiPrev(pStart, pChar);
#elif defined(_MACINTOSH)
	// Since there is no AnsiPrev equivalent on the Macintosh. 
     
    pPrev=(char*)pChar;
    
    //Move to LeadByte of current character.
    // If single byte do nothing.
    pPrev=MoveToLeadByte(pPrev);
    
    //Move back one byte.
    pPrev=pPrev-1;
    
    //Move to leadbyte of this character.
    //If single byte do nothing. 
    pPrev=MoveToLeadByte(pPrev);
	
#else // unix
	pPrev = (char*)pChar - 1;
#endif

	if(pPrev == pChar)
	return pPrev - 1;

	return pPrev;
}

HXBOOL
HXIsEqual(const char* pChar, char CharToCompare)
{
	if(!pChar)
		return FALSE;

	if(!HXIsDBCSEnabled())
	    return (* pChar == CharToCompare);

	char * pNext = HXGetNextChar(pChar);
	if(!pNext || pNext - pChar == 1)
	{
		if(*pChar == CharToCompare)
			return TRUE;
	}

	return FALSE;
}

char*
HXFindChar(const char* pStart, char CharToFind)
{
	if(!HXIsDBCSEnabled())
	    return (char*)strchr(pStart, CharToFind);

	const char * pCurrent = pStart;
	
	while(pCurrent && *pCurrent)
	{	
		char * pNext;
		pNext = HXGetNextChar(pCurrent);
		if(!pNext || pNext - pCurrent == 1)
			if(*pCurrent == CharToFind)
			    return (char *)pCurrent;
		pCurrent = pNext;
	}

	return NULL;
}

const char*
HXFindCharN(const char* pStart, char CharToFind, size_t maxlen)
{
    size_t nByte;

    if(!HXIsDBCSEnabled())
    {
        for(nByte = 0; nByte < maxlen && pStart[nByte] != '\0'; nByte++)
        {
            if(pStart[nByte] == CharToFind)
            {
                return (&pStart[nByte]);
            }
        }
    }
    else
    {
        for(nByte = 0; nByte < maxlen && pStart[nByte] != '\0'; nByte++)
        {
            // If lead byte, skip the next one
            if(HXIsLeadByte(pStart[nByte]))
            {
                nByte++;
            }
            else if(pStart[nByte] == CharToFind)
            {
                return (&pStart[nByte]);
            }
        }
	}

	return NULL;
}

char*
HXReverseFindChar(const char* pStart, char CharToFind)
{
	if(!HXIsDBCSEnabled())
		return (char*)strrchr(pStart, CharToFind);
	
	const char * pCurrent = pStart;
    
	char * pLastFound = NULL;

	while(*pCurrent)
	{
		char * pNext;
		pNext = HXGetNextChar(pCurrent);
		if(!pNext || pNext - pCurrent == 1)
			if(*pCurrent == CharToFind)
				pLastFound = (char *)pCurrent;
		pCurrent = pNext;
	}

	return pLastFound;
}

char*
HXFindString(const char* pStart, const char * pString)
{
	if(!HXIsDBCSEnabled())
	    return (char*)strstr(pStart, pString);
	
	const char * pCurrent = pStart;
    int Length = strlen(pString);

	while(*pCurrent)
	{
		if(!memcmp(pCurrent, pString, Length))
		{
			const char * pTest = pCurrent;
			while(pTest < pCurrent + Length)
				pTest = HXGetNextChar(pTest);
			if(pTest == pCurrent + Length)
			    return (char *)pCurrent;
		}
		pCurrent = HXGetNextChar(pCurrent);
	}

	return NULL;
}

int
HXCompareStrings(const char* pString1, const char* pString2, size_t count)
{
	if(!pString1 && !pString2)
		return 0;

	if(!pString1)
		return -1;

	if(!pString2)
		return 1;

	if(!HXIsDBCSEnabled())
		return strncmp(pString1,pString2, count);

	const char * pEnd1 = pString1;
	const char * pEnd2 = pString2;

	while(*pEnd1 && (size_t)(pEnd1 - pString1) < count)
		pEnd1 = HXGetNextChar( pEnd1 );

	while(*pEnd2 && (size_t)(pEnd2 - pString2) < count)
		pEnd2 = HXGetNextChar( pEnd2 );

	if(pEnd1 - pString1 < pEnd2 - pString2)
		return -1;

	if(pEnd1 - pString1 > pEnd2 - pString2)
		return 1;

	return memcmp(pString1, pString2, pEnd1 - pString1);
}

HXBOOL
HXIsLeadByte(char Byte)
{

	if(!HXIsDBCSEnabled()) return(FALSE);

#if (defined( _WIN32 ) || defined( _WINDOWS )) && !defined(WIN32_PLATFORM_PSPC)
	return _ismbblead((unsigned int)Byte);	
#elif defined(_MACINTOSH) || defined(_MAC_UNIX)
#if defined(_CARBON) || defined(_MAC_UNIX)
	return (CharacterType((Ptr)&Byte,0, smRoman) == MBCS_LEADBYTE);
#else
	return (CharType((Ptr)&Byte,0) == MBCS_LEADBYTE);
#endif
#else
	return FALSE;
#endif
}


// We do not want a LeadByte to be hanging loose.  If the last character
// to copy is a LeadByte, replace it with a null character.
char* HXCopyStringN(char* pOutput, const char* pInput, size_t count)
{
	HX_ASSERT(count > 0);

	if(!HXIsDBCSEnabled() || count == 0 || strlen(pInput) <= count)
		return (char*)strncpy(pOutput, pInput, count); /* Flawfinder: ignore */

	const char * pEnd = pInput + count;
	const char * pPrev = HXGetPrevChar(pInput, pEnd);

	if (pEnd != HXGetNextChar(pPrev))
	{
		memset(pOutput, 0, count);
		pEnd = pPrev;
	}

	return (char*)strncpy(pOutput, pInput, pEnd - pInput); /* Flawfinder: ignore */
}


//
// Please return 0 on a non-dbcs machine.  on a dbcs machine, return the code page number active.
//

INT32
GetDBCSCodePage(void) 
{
#if defined( _WINDOWS ) && !defined( WIN32_PLATFORM_PSPC )
return _getmbcp(); 
#else 
	return 0; 
#endif
}

//////////////////////////////
// MACINTOSH ONLY FUNCTIONS
//

#ifdef _MACINTOSH

//* Move to the first byte of the current character.
//* Does nothing if the current character is a single byte.
char* MoveToLeadByte(char* pChar) {
	char *pPrev=pChar;
#if 0	/* CharByte can't be executed at interrupt time */
	if (CharByte((Ptr)pPrev,0) != MBCS_SINGLEBYTE) {
		while (CharByte((Ptr)pPrev,0) != MBCS_LEADBYTE) 
	       pPrev=pPrev-1;
		}//*if
#endif		
	return(pPrev);
}

//* Move to the last byte of the current character.
//* Okay this is strange logic, CharByte searches upto the offset that is specified.
//* It never looks back, so it must always be at the beginning of a character in order to 
//* ever return a correct value, with the exception of the MBCS_LEADBYTE which could be specified
//* as 1 byte.	
//* Does nothing if the current character is a single byte.
char* MoveToLastByte(char *pChar) {
	char *pNext=pChar;
#if 0	/* CharByte can't be executed at interrupt time */
	if (CharByte((Ptr)pNext,1) != MBCS_SINGLEBYTE) {
		if (CharByte((Ptr)pNext,1) == MBCS_LASTBYTE) 
	       pNext=pNext+1;
		}//*if
#endif		
	return(pNext);
}

#endif

#endif // !defined(_UNIX)


