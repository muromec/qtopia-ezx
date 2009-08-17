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

#ifndef QTATMMGS_INLINE_H
#define QTATMMGS_INLINE_H

/****************************************************************************
 *  Time To Sample Manager
 */
/****************************************************************************
 *  AdvanceSyncSampleNumber
 */
HXBOOL CQT_TimeToSample_Manager::AdvanceSyncSampleNumber(void)
    {
	if (m_ulNumSyncEntries == 0)
	{
	    m_ulSyncSampleNumber = m_ulSampleNumber;
	    return TRUE;
	}
	
	if (m_ulSyncSampleNumber >= m_ulSampleNumber)
	{
	    return TRUE;
	}

	m_ulCurrentSyncEntryIdx++;
	if (m_ulCurrentSyncEntryIdx < m_ulNumSyncEntries)
	{
	    m_ulSyncSampleNumber = m_pSyncSampleAtom->
		Get_SampleNum(m_ulCurrentSyncEntryIdx) + m_ulKeyFrameNumOffset;
	    return TRUE;
	}

	return TRUE;
    }

/****************************************************************************
 *  AdvanceCompBySample
 */
HXBOOL CQT_TimeToSample_Manager::AdvanceCompBySample(void)
{
    if (m_ulNumCompEntries == 0)
    {
	return TRUE;
    }
    
    do
    {
	if (m_ulSamplesLeftInCompEntry > 0)
	{
	    m_ulCompSampleNumber++;
	    m_ulSamplesLeftInCompEntry--;
	    return TRUE;	// Success
	}
	
	m_ulCurrentCompEntryIdx++;
	
	if (m_ulCurrentCompEntryIdx < m_ulNumCompEntries)
	{
	    m_ulSamplesLeftInCompEntry = m_pCompOffsetAtom->
		Get_SampleCount(m_ulCurrentCompEntryIdx);
	    m_ulCompOffset = m_pCompOffsetAtom->
		Get_SampleOffset(m_ulCurrentCompEntryIdx);
	}
	else
	{
	    m_ulCurrentCompEntryIdx--;
	    break;
	}
    } while(1);
    
    return FALSE;   // Failure
}

/****************************************************************************
 *  AdvanceCompBySample
 */
HXBOOL CQT_TimeToSample_Manager::AdvanceBySample(void)
{
    AdvanceCompBySample();
    
    do
    {
	if (m_ulSamplesLeftInEntry > 0)
	{
	    m_ulSampleNumber++;
	    m_ulSamplesLeftInEntry--;
	    
	    return AdvanceSyncSampleNumber();
	}
	
	m_ulCurrentEntryIdx++;
	
	if (m_ulCurrentEntryIdx < m_ulNumEntries)
	{
	    m_ulSamplesLeftInEntry = m_pTimeToSampleAtom->
		Get_SampleCount(m_ulCurrentEntryIdx);
	    m_ulSampleDuration = m_pTimeToSampleAtom->
		Get_SampleDuration(m_ulCurrentEntryIdx);
		/***
		if (m_ulSampleDuration == 0)
		{
		// Skip over the zero length samples
		m_ulSampleNumber += m_ulSamplesLeftInEntry;
		m_ulSamplesLeftInEntry = 0;
		}
	    ***/
	}
	else
	{
	    m_ulCurrentEntryIdx--;
	    break;
	}
    } while(1);
    
    return FALSE;   // Failure
}

#endif	// QTATMMGS_INLINE_H

