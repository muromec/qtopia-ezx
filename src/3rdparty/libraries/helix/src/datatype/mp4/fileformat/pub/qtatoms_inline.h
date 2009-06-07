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

#ifndef QTATOMS_INLINE_H
#define QTATOMS_INLINE_H

/****************************************************************************
 *  stts Atom Class
 */
ULONG32 CQT_stts_Atom::Get_SampleDuration(ULONG32 ulEntryIdx)
{
    UINT8* pData = m_pData;
    HX_ASSERT(m_pData);

#if !defined(QTCONFIG_NO_PAGING)
    if (m_pMemPager)
    { 
	m_lastStatus = m_pMemPager->PageIn(
	    ((Data*) pData)->pTable[ulEntryIdx].pSampleDuration, 
	    sizeof(((Data*) pData)->pTable[ulEntryIdx].pSampleDuration),
	    pData);
    }
#endif /* #if !defined(QTCONFIG_NO_PAGING) */
    
    if (m_lastStatus == HXR_OK)
    {
	return GetUL32(((Data*) pData)->pTable[ulEntryIdx].pSampleDuration);
    }
    
    return 0;
}
 

/****************************************************************************
 *  stss Atom Class
 */
ULONG32 CQT_stss_Atom::Get_NumEntries(void)
{
    UINT8* pData;
    
    if (m_ulNumEntries != 0)
    {
	return m_ulNumEntries;
    }
    
    pData = m_pData;
    HX_ASSERT(m_pData);
    
#if !defined(QTCONFIG_NO_PAGING)
    if (m_pMemPager)
    { 
	m_lastStatus = m_pMemPager->PageIn(
	    ((Data*) pData)->pNumEntries, 
	    sizeof(((Data*) pData)->pNumEntries),
	    pData);
    }
#endif /* #if !defined(QTCONFIG_NO_PAGING) */
    
    if (m_lastStatus == HXR_OK)
    {
	m_ulNumEntries = GetUL32(((Data*) pData)->pNumEntries);
	return m_ulNumEntries;
    }
    
    return 0;
}

ULONG32 CQT_stss_Atom::Get_SampleNum(ULONG32 ulEntryIdx)
{
    UINT8* pData = m_pData;
    HX_ASSERT(m_pData);
    
#if !defined(QTCONFIG_NO_PAGING)
    if (m_pMemPager)
    { 
	m_lastStatus = m_pMemPager->PageIn(
	    ((Data*) pData)->pTable[ulEntryIdx].pSampleNum, 
	    sizeof(((Data*) pData)->pTable[ulEntryIdx].pSampleNum),
	    pData);
    }
#endif /* #if !defined(QTCONFIG_NO_PAGING) */
    
    if (m_lastStatus == HXR_OK)
    {
	return GetUL32(((Data*) pData)->pTable[ulEntryIdx].pSampleNum);
    }
    
    return 0;
}


/****************************************************************************
 *  stsc Atom Class
 */
ULONG32 CQT_stsc_Atom::Get_FirstChunk(ULONG32 ulEntryIdx)
{
    UINT8* pData = m_pData;
    HX_ASSERT(m_pData);
    
#if !defined(QTCONFIG_NO_PAGING)
    if (m_pMemPager)
    { 
	m_lastStatus = m_pMemPager->PageIn(
	    ((Data*) pData)->pTable[ulEntryIdx].pFirstChunk, 
	    sizeof(((Data*) pData)->pTable[ulEntryIdx].pFirstChunk),
	    pData);
    }
#endif /* #if !defined(QTCONFIG_NO_PAGING) */
    
    if (m_lastStatus == HXR_OK)
    {
	return GetUL32(((Data*) pData)->pTable[ulEntryIdx].pFirstChunk);
    }
    
    return 0;
}

ULONG32 CQT_stsc_Atom::Get_SamplesPerChunk(ULONG32 ulEntryIdx)
{
    UINT8* pData = m_pData;
    HX_ASSERT(m_pData);
    
#if !defined(QTCONFIG_NO_PAGING)
    if (m_pMemPager)
    { 
	m_lastStatus = m_pMemPager->PageIn(
	    ((Data*) pData)->pTable[ulEntryIdx].pSamplesPerChunk, 
	    sizeof(((Data*) pData)->pTable[ulEntryIdx].pSamplesPerChunk),
	    pData);
    }
#endif /* #if !defined(QTCONFIG_NO_PAGING) */
    
    if (m_lastStatus == HXR_OK)
    {
	return GetUL32(((Data*) pData)->pTable[ulEntryIdx].pSamplesPerChunk);
    }
    
    return 0;
}

ULONG32 CQT_stsc_Atom::Get_SampleDescID(ULONG32 ulEntryIdx)
{
    UINT8* pData = m_pData;
    HX_ASSERT(m_pData);
    
#if !defined(QTCONFIG_NO_PAGING)
    if (m_pMemPager)
    { 
	m_lastStatus = m_pMemPager->PageIn(
	    ((Data*) pData)->pTable[ulEntryIdx].pSampleDescID, 
	    sizeof(((Data*) pData)->pTable[ulEntryIdx].pSampleDescID),
	    pData);
    }
#endif /* #if !defined(QTCONFIG_NO_PAGING) */
    
    if (m_lastStatus == HXR_OK)
    {
	return GetUL32(((Data*) pData)->pTable[ulEntryIdx].pSampleDescID);
    }
    
    return 0;
}


/****************************************************************************
 *  stsz Atom Class
 */
ULONG32 CQT_stsz_Atom::Get_SampleSize(void)
{
    UINT8* pData;
    
    if (m_ulSampleSize != QT_BAD_SAMPLE_SIZE)
    {
	return m_ulSampleSize;
    }
    
    pData = m_pData;
    HX_ASSERT(m_pData);
    
#if !defined(QTCONFIG_NO_PAGING)
    if (m_pMemPager)
    { 
	m_lastStatus = m_pMemPager->PageIn(
	    ((Data*) pData)->pSampleSize, 
	    sizeof(((Data*) pData)->pSampleSize),
	    pData);
    }
#endif /* #if !defined(QTCONFIG_NO_PAGING) */
    
    if (m_lastStatus == HXR_OK)
    {
	m_ulSampleSize = GetUL32(((Data*) pData)->pSampleSize);
	return m_ulSampleSize;
    }
    
    return 0;
}

ULONG32 CQT_stsz_Atom::Get_SampleSize(ULONG32 ulFieldIdx)
{
    UINT8* pData = m_pData;
    HX_ASSERT(m_pData);
    
    if (m_uFieldSizeBits == 32)
    {
	// Non-compact sample size storage
#if !defined(QTCONFIG_NO_PAGING)
	if (m_pMemPager)
	{ 
	    m_lastStatus = m_pMemPager->PageIn(
		((Data*) pData)->pTable[ulFieldIdx].pSampleSize, 
		sizeof(((Data*) pData)->pTable[ulFieldIdx].pSampleSize),
		pData);
	}
#endif /* #if !defined(QTCONFIG_NO_PAGING) */
    
	if (m_lastStatus == HXR_OK)
	{
	    return GetUL32(((Data*) pData)->pTable[ulFieldIdx].pSampleSize);
	}
    }
    else
    {
	// We have compact sample size storage, thus this must be an 
	// instance of stz2 object.
	// Note we do this RTTI syle instead of polymorphism (virtual func)
	// for performance reasons (we want to inline).
	return ((CQT_stz2_Atom*) this)->GetCompactSampleSize(ulFieldIdx);
    }
    
    return 0;
}

#endif	// QTATOMS_INLINE_H

