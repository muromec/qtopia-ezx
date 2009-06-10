/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: regkey.cpp,v 1.4 2009/05/30 19:09:56 atin Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#include <string.h>
#include "hxtypes.h"
#include "regkey.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif

ServRegKey::ServRegKey(const char* pszKey, char chDelim)
    : m_pszKey(0)
    , m_pSubStrs(0)
    , m_pCurrPtr(0)
    , m_nLevels(0)
    , m_nSize(0)
{
    if (!pszKey || !*pszKey)
    {
	return;
    }
    m_pCurrPtr = pszKey;

    // We have to go through this two step process because of a problem with
    // the 16 bit compiler.
    int nMaxLevels = 128; // 128 levels should b sufficient for reg prop names
    const char* pTmpPtrs2[128];
    const char** pTmpPtrs = pTmpPtrs2;
    pTmpPtrs[0] = pszKey;

    /*
     * loop to find out how many levels are there in this key string
     * pointers in the string to the various sub-strings are stored in
     * a temporary array, which will then be xferred to a dynamic array
     * stored along with the key. this will speed up the sub-string 
     * operations done later.
     */
    m_nLevels = 1;
    m_nSize = 1;
    while (*m_pCurrPtr)
    {
	if (*m_pCurrPtr == chDelim)
	{
	    if (m_pCurrPtr > pszKey)
	    {
		pTmpPtrs[m_nLevels] = m_pCurrPtr;
		m_nLevels++;
		if (m_nLevels >= nMaxLevels)
		{
		    return;
		}
	    }
	}
        m_nSize++;
        m_pCurrPtr++;
    }
    pTmpPtrs[m_nLevels] = m_pCurrPtr;

        m_pSubStrs = new char*[m_nLevels + 1];
        m_pszKey = new char[m_nSize];
    memcpy(m_pszKey, pszKey, m_nSize);
    m_pSubStrs[0] = m_pszKey;

    for (int i = 1; i < m_nLevels + 1; i++)
    {
	m_pSubStrs[i] = m_pSubStrs[0] + (pTmpPtrs[i] - pTmpPtrs[0]);
    }
    m_pCurrPtr = m_pszKey;
    m_nCurrLevel = 0;
    m_chDelim = chDelim;

    m_pszLastSubStr = m_pSubStrs[m_nLevels - 1];
    if (*m_pszLastSubStr == m_chDelim)
    {
        ++m_pszLastSubStr;
    }
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	get_sub_str
 *  Input Params:   	char* chDelim
 *  Return Value:   	int
 *  Description:
 *  	gets a sub-string of "m_pszKey" delimited by "chDelim" and returns
 *  the number of bytes in the sub-string.
 */
int
ServRegKey::get_sub_str(char* pszBuf, int nBufLen, char chDelim)
{
    if (m_pCurrPtr && m_nCurrLevel >= m_nLevels)
    {
	return 0;
    }

    int nSubStrLen = m_pSubStrs[m_nCurrLevel + 1] - m_pSubStrs[m_nCurrLevel];
    if (nSubStrLen >= nBufLen)
    {
	nSubStrLen = nBufLen;
    }

    // XXXAAK -- for now use strncpy
    strncpy(pszBuf, m_pSubStrs[m_nCurrLevel], nSubStrLen);
    *(pszBuf + nSubStrLen) = '\0';
    m_nCurrLevel++;
	   
    m_pCurrPtr = (m_pSubStrs[m_nCurrLevel]) ? (m_pSubStrs[m_nCurrLevel] + 1)
					    :  m_pSubStrs[m_nCurrLevel];
    return nSubStrLen;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServRegKey::append_sub_str
 *  Input Params:   	char* pszBuf, int nBufLen, char chDelim
 *  Return Value:   	int
 *  Description:
 *  	appends the next sub-string to the string in the buffer
 *  passed as a parameter. if the buffer is empty the sub-string
 *  is just copied into it, but if it is not-empty then it first
 *  appends a delimiter and then the sub-string.
 */
int
ServRegKey::append_sub_str(char* pszBuf, int nBufLen, char chDelim)
{
    // if we have reached the end of the key string
    if (m_nCurrLevel >= m_nLevels)
	return 0;

    // loop until u reach the end of the buf before we append to it
    int nSubStrLen = 0;
    if (*pszBuf)
    {
	while (nSubStrLen < nBufLen && *pszBuf)
        {
	    pszBuf++;
            nSubStrLen++;
        }
	// return if no more space in the buffer
	if (nSubStrLen >= nBufLen)
        {
	    return 0;
        }
	*pszBuf = '\0';
    }

    int nNumChars = m_pSubStrs[m_nCurrLevel + 1] - m_pSubStrs[m_nCurrLevel];
    /*
     * if the combined length exceeds the buffer len then reduce
     * the number of chars copied to fit the buffer
     */
    if ((nSubStrLen + nNumChars) >= nBufLen)
    {
	nNumChars = nBufLen - nSubStrLen;
    }
    nSubStrLen += nNumChars;

    // XXXAAK -- for now use strncpy
    strncpy(pszBuf, m_pSubStrs[m_nCurrLevel], nNumChars);
    *(pszBuf + nNumChars) = '\0';
    m_nCurrLevel++;
	   
    m_pCurrPtr = (m_pSubStrs[m_nCurrLevel]) ? m_pSubStrs[m_nCurrLevel] + 1
					    : m_pSubStrs[m_nCurrLevel];
    return nSubStrLen;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	ServRegKey::is_a_sub_str_of const
 *  Input Params:   	char* pszBuf
 *  Return Value:   	BOOL
 *  Description:
 *  	check if the string "str" is a sub-string of the key string
 *  if it is then return TRUE else return FALSE. one criterion for'
 *  this operation is that a legal sub-string is defined as one which
 *  ends with a matching delimiter of the m_pszKey or a '\0' char.
 *  for example,
 *      foo is a VALID sub-string of "foo.bar.shmoo"
 *      and so is "foo.bar".
 *      whereas "foo.b" is NOT a VALID sub-string of "foo.bar.shmoo"
 */
BOOL
ServRegKey::is_a_sub_str_of(char* pszBuf) const
{
    if (!pszBuf || !*pszBuf)
    {
	return FALSE;
    }

    char* szTmp = m_pszKey;
    while (*pszBuf != '\0')
    {
	if (*pszBuf != *szTmp)
        {
	    return FALSE;
        }
        pszBuf++;
        szTmp++;
    }

    if (*szTmp != m_chDelim && *szTmp != '\0')
    {
	return FALSE;
    }

    return TRUE;
}
