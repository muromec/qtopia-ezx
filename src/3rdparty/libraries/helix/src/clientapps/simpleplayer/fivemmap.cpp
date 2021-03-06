/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include <string.h>
#include "hxtypes.h"
#include "fivemmap.h"

void* FiveMinuteMap::GetFirstValue()
{
    m_nCursor = 0;

    if (m_nMapSize)
    {
       return m_pValueArray[m_nCursor];
    }
    else
    {
       return NULL;
    }
}

void* FiveMinuteMap::GetNextValue()
{
    m_nCursor++;

    if (m_nCursor < m_nMapSize)
    {
       return m_pValueArray[m_nCursor];
    }
    else
    {
       return NULL;
    }
}

HXBOOL FiveMinuteMap::Lookup(void* Key, void*& Value) const
{
    HXBOOL bFound = FALSE;
    int nIndex = 0;

    // If Key is alrady in the list, replace value
    for (; nIndex < m_nMapSize; nIndex++)
    {
	if (m_pKeyArray[nIndex] == Key)
	{
	    Value = m_pValueArray[nIndex];
	    bFound = TRUE;
	    goto exit;
	}
    }

exit:
    return bFound;    
}

void FiveMinuteMap::RemoveKey(void* Key)
{
    HXBOOL bFound = FALSE;
    int nIndex = 0;

    // If Key is alrady in the list, replace value
    for (; nIndex < m_nMapSize; nIndex++)
    {
	if (m_pKeyArray[nIndex] == Key)
	{
	    if (nIndex < (m_nMapSize-1))
	    {
		memmove(&(m_pKeyArray[nIndex]),&(m_pKeyArray[nIndex+1]),sizeof(void*)*(m_nMapSize-(nIndex+1)));
		memmove(&(m_pValueArray[nIndex]),&(m_pValueArray[nIndex+1]),sizeof(void*)*(m_nMapSize-(nIndex+1)));
	    }
	    m_nMapSize--;
	    goto exit;
	}
    }

exit:
    (NULL); // We're done!
}

void FiveMinuteMap::RemoveValue(void* Value)
{
    HXBOOL bFound = FALSE;
    int nIndex = 0;

    // If Value is alrady in the list, replace value
    for (; nIndex < m_nMapSize; nIndex++)
    {
	if (m_pValueArray[nIndex] == Value)
	{
	    if (nIndex < (m_nMapSize-1))
	    {
		memmove(&(m_pKeyArray[nIndex]),&(m_pKeyArray[nIndex+1]),sizeof(void*)*(m_nMapSize-(nIndex+1)));
		memmove(&(m_pValueArray[nIndex]),&(m_pValueArray[nIndex+1]),sizeof(void*)*(m_nMapSize-(nIndex+1)));
	    }
	    m_nMapSize--;
	    goto exit;
	}
    }

exit:
    (NULL); // We're done!
}


void FiveMinuteMap::SetAt(void* Key, void* Value)
{
    int nIndex = 0;

    // If Key is alrady in the list, replace value
    for (; nIndex < m_nMapSize; nIndex++)
    {
	if (m_pKeyArray[nIndex] == Key)
	{
	    m_pValueArray[nIndex] = Value;
	    goto exit;
	}
    }

    // If we have room, add it to the end!
    if (m_nAllocSize == m_nMapSize)
    {
	m_nAllocSize += AllocationSize;
	void** pNewKeys   = new void*[m_nAllocSize];
	void** pNewValues = new void*[m_nAllocSize];

	memcpy(pNewKeys,m_pKeyArray,sizeof(void*)*m_nMapSize); /* Flawfinder: ignore */
	memcpy(pNewValues,m_pValueArray,sizeof(void*)*m_nMapSize); /* Flawfinder: ignore */

	delete [] m_pKeyArray;
	delete [] m_pValueArray;

	m_pKeyArray = pNewKeys;
	m_pValueArray = pNewValues;
    }

    m_pKeyArray[m_nMapSize] = Key;
    m_pValueArray[m_nMapSize] = Value;
    m_nMapSize++;

exit:
    (NULL); // We're done!
}

