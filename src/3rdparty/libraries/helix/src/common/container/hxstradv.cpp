/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxstradv.cpp,v 1.17 2007/07/06 20:34:58 jfinnecy Exp $
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

#include "hxstring.h"

#include "hlxclib/string.h"
#include "hlxclib/ctype.h"
#include "hxassert.h"
#include "hxstrutl.h"

CHXString CHXString::SpanIncluding(const char* pCharSet) const
{
    if (m_pRep)
	return CHXString(m_pRep->GetBuffer(), 
			 strspn(m_pRep->GetBuffer(), pCharSet));
    return CHXString();
}

CHXString CHXString::SpanExcluding(const char* pCharSet) const
{
    if (m_pRep)
	return CHXString(m_pRep->GetBuffer(), 
			 strcspn(m_pRep->GetBuffer(), pCharSet));
    return CHXString();
}

void CHXString::MakeUpper()
{
    if (m_pRep)
    {
	EnsureUnique();
	
	char* pCur = m_pRep->GetBuffer();
	
	for(; *pCur; ++pCur)
	    *pCur = toupper(*pCur);
    }
}

void CHXString::MakeLower()
{
    if (m_pRep)
    {
	EnsureUnique();
	
	char* pCur = m_pRep->GetBuffer();
	
	for(; *pCur; ++pCur)
	    *pCur = tolower(*pCur);
    }
}

INT32 CHXString::Find(char ch) const
{
    INT32 ret = -1;

    if (m_pRep)
    {
	const char* pTmp = strchr(m_pRep->GetBuffer(), ch);
	
	if (pTmp)
	    ret = pTmp - m_pRep->GetBuffer();
    }

    return ret;
}

INT32 CHXString::ReverseFind(char ch) const
{
    INT32 ret = -1;

    if (m_pRep)
    {
	const char* pTmp = strrchr(m_pRep->GetBuffer(), ch);
	
	if (pTmp)
	    ret = pTmp - m_pRep->GetBuffer();
    }

    return ret;
}

INT32 CHXString::Find(const char* pStr) const
{
    INT32 ret = -1;
    
    if (m_pRep)
    {
	const char* pTmp = strstr(m_pRep->GetBuffer(), pStr);
	
	if (pTmp)
	    ret = pTmp - m_pRep->GetBuffer();
    }

    return ret;
}

void CHXString::AppendULONG(ULONG32 value)
{
    static const int MaxULongString = 12;

    char buf[MaxULongString]; /* Flawfinder: ignore */
    int tmp=0;
	tmp = SafeSprintf(buf, MaxULongString, "%lu", value);
    HX_ASSERT(tmp < MaxULongString);
    *this += buf;
}

void CHXString::AppendEndOfLine()
{
#if defined(_WINDOWS) || defined(_SYMBIAN)
    *this += "\r\n";
#elif defined(_MACINTOSH)
    *this += '\r';
#else
    *this += '\n';
#endif /* defined(_WINDOWS) */
}


void CHXString::Center(short length)
{
    if (m_pRep)
    {
	EnsureUnique();
	
	TrimLeft();
	TrimRight();
	
	int offset = 0;
	
	if (length > m_pRep->GetStringSize())
	    offset = length / 2 - m_pRep->GetStringSize() / 2;
	
	HX_ASSERT(offset >= 0);
	
	int newSize = offset + m_pRep->GetStringSize();
	
	if (m_pRep->GetBufferSize() < (newSize + 1))
	    m_pRep->ResizeAndCopy(newSize);
	
	char* pSrc = m_pRep->GetBuffer() + m_pRep->GetStringSize();
	char* pDest = m_pRep->GetBuffer() + newSize;
	
	// Copy the string so that it is located offset characters
	// from the start of the string
	while (pSrc >= m_pRep->GetBuffer())
	    *pDest-- = *pSrc--;
	
	// Put offset number of space characters at the start of the string
	while (pDest >= m_pRep->GetBuffer())
	    *pDest-- = ' ';

	m_pRep->SetStringSize(newSize);
    }
    else if (length > 0)
	m_pRep = new CHXStringRep(' ', length / 2);
}

CHXString CHXString::Mid(INT32 i, INT32 length) const
{
    HX_ASSERT(m_pRep && (i <= m_pRep->GetStringSize()));
    
    if (m_pRep)
    {
	if ((i + length) > m_pRep->GetStringSize())
	    length = m_pRep->GetStringSize() - i;
	
	return CHXString(m_pRep->GetBuffer() + i, length);
    }

    return CHXString();
}

CHXString CHXString::Mid(INT32 i) const
{
    HX_ASSERT(m_pRep &&(i <= m_pRep->GetStringSize()));

    if (m_pRep)
	return CHXString(m_pRep->GetBuffer() + i);

    return CHXString();
}

CHXString CHXString::Left(INT32 length) const
{
    HX_ASSERT(length >= 0);
    
    if (m_pRep)
    {
	if (length > m_pRep->GetStringSize())
	    length = m_pRep->GetStringSize();

	return CHXString(m_pRep->GetBuffer(), length);
    }

    return CHXString();
}

CHXString CHXString::Right(INT32 length) const
{
    HX_ASSERT(length >= 0);
    
    if (m_pRep)
    {
	if (length > m_pRep->GetStringSize())
	    length = m_pRep->GetStringSize();
	
	int offset = m_pRep->GetStringSize() - length;
	
	return CHXString(m_pRep->GetBuffer() + offset, length);
    }

    return CHXString();
}

ULONG32 CHXString::CountFields(char delim) const
{
    ULONG32 ret = 0;

    if (m_pRep && m_pRep->GetStringSize())
    {
	ret++; // If the string is not empty we have at least 1 field.
	
	for (const char* pCur = m_pRep->GetBuffer(); *pCur; pCur++)
	    if (*pCur == delim)
		ret++;
    }

    return ret;
}

static UINT64 PackState(ULONG32 index, ULONG32 offset)
{
    return (((UINT64)index) << 32) | offset;
}

static void UnpackState(UINT64 state, ULONG32& index, ULONG32& offset)
{
    index = INT64_TO_ULONG32(state >> 32);
    offset = INT64_TO_ULONG32((state & 0x0ffffffff));
}

CHXString CHXString::GetNthField(char delim, ULONG32 i, UINT64& state) const
{
    CHXString ret;

    if (m_pRep)
    {
	ULONG32 index = 0;
	ULONG32 offset = 0;
	
	// Get the state values
	UnpackState(state, index, offset);
	
	if (offset >= (ULONG32)(m_pRep->GetStringSize()))
	    offset = 0;
	
	// 'i' passed in as a 1 based, with the exception that 0 also means 1.
	// This converts the 1 based index into a 0 base index
	if (i > 0)
	    i = i - 1;
	
	if (i >= index)
	{
	    const char* pStart = m_pRep->GetBuffer();
	    
	    // Apply state information
	    pStart += offset;
	    
	    // Find the start of the field
	    for(;(*pStart) && (index < i); pStart++)
		if (*pStart == delim)
		    index++;
	    
	    const char* pEnd = pStart;
	    
	    // Find the end of the field
	    while ( (*pEnd) && (*pEnd != delim)) 
	    { 
	        pEnd++; 
	    }
	    
	    if (pStart != pEnd)
		ret = CHXString(pStart, pEnd - pStart);
	    
	    // Update the state
	    PackState(index, pEnd - m_pRep->GetBuffer());
	}
    }

    return ret;
}

CHXString CHXString::NthField(char delim, ULONG32 i) const
{
    UINT64 state = PackState(0,0);
    return GetNthField(delim, i, state);
}

void CHXString::TrimRight()
{
    if (m_pRep)
    {
	EnsureUnique();
	
	INT32 strSize = m_pRep->GetStringSize();
	
	if (strSize)
	{
	    char* pCur = m_pRep->GetBuffer() + strSize - 1;
	    
	    while((pCur >= m_pRep->GetBuffer()) && (IS_SPACE(*pCur)) )
	    {
	        pCur--; strSize--;
	    }
	    
	    // Null terminate the string
	    m_pRep->GetBuffer()[strSize] = '\0';
	    
	    m_pRep->SetStringSize(strSize);
	}
    }
}

void CHXString::TrimLeft()
{
    if (m_pRep)
    {
	EnsureUnique();
	
	char* pStart = m_pRep->GetBuffer();
	
	while( (*pStart) && (IS_SPACE(*pStart))) 
	    {
	        pStart++;
	    }
	
	INT32 newSize = m_pRep->GetStringSize() - (pStart - m_pRep->GetBuffer());
	memmove(m_pRep->GetBuffer(), pStart, newSize + 1);
	
	m_pRep->SetStringSize(newSize);
    }
}

HXBOOL CHXString::FindAndReplace(const char* pSearch , const char* pReplace,
				HXBOOL bReplaceAll)
{
    HXBOOL ret = FALSE;

    if (m_pRep)
    {
	const char* pStart = m_pRep->GetBuffer();
	const char* pMatch = strstr(pStart, pSearch);
    
	if (pMatch)
	{
	    INT32 searchLen = SafeStrlen(pSearch);
	    CHXString buf;

	    while(pMatch)
	    {
		// Append any data that was between pStart and pMatch
		buf.Append(pStart, pMatch - pStart);

		// Append the replacement string to our buffer
		buf += pReplace;

		// Update search start position
		pStart = pMatch + searchLen;

		// See if we need to do more replacements
		if (bReplaceAll)
		    pMatch = strstr(pStart, pSearch);
		else
		    pMatch = 0;
	    }

	    // Append trailing part of the string
	    buf += pStart;

	    // Update the this objects m_pRep with the one we 
	    // just constructed in buf
	    *this = buf;

	    ret = TRUE;
	}
    }
    return ret;
}
