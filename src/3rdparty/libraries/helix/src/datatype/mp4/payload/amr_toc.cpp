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
#include "amr_toc.h"

const ULONG32 AMRTOCInfo::zm_ulDefaultSize = 8;

AMRTOCInfo::AMRTOCInfo() :
    m_pTypes(new UINT8[zm_ulDefaultSize]),
    m_pQualities(new UINT8[zm_ulDefaultSize]),
    m_ulEntryCount(0),
    m_ulMaxCount(zm_ulDefaultSize)
{}

AMRTOCInfo::~AMRTOCInfo()
{
    delete [] m_pTypes;
    m_pTypes = 0;

    delete [] m_pQualities;
    m_pQualities = 0;
}

void AMRTOCInfo::AddInfo(UINT8 type, UINT8 quality)
{
    if (m_ulEntryCount + 1 >= m_ulMaxCount)
    {
	ULONG32 ulNewSize = (m_ulMaxCount) ? 2 * m_ulMaxCount : 1;
	UINT8* pTmpTypes = new UINT8[ulNewSize];
	UINT8* pTmpQualities = new UINT8[ulNewSize];

	for (ULONG32 i = 0; i < m_ulMaxCount; i++)
	{
	    pTmpTypes[i] = m_pTypes[i];
	    pTmpQualities[i] = m_pQualities[i];
	}

	m_ulMaxCount = ulNewSize;

	delete [] m_pTypes;
	delete [] m_pQualities;

	m_pTypes = pTmpTypes;
	m_pQualities = pTmpQualities;
    }

    m_pTypes[m_ulEntryCount] = type;
    m_pQualities[m_ulEntryCount] = quality;

    m_ulEntryCount++;
}
