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

/****************************************************************************
 *  Defines
 */

// Certain clips may contain other boxes than avcC in the beginning of the AVC Config data
// This flag will enable parsing through these boxes until avcC is found, is available.
#define _SCAN_FOR_AVCC_START

/****************************************************************************
 *  Includes
 */
#include "hlxclib/memory.h"
#include "avcconfig.h"


/****************************************************************************
 *  Statics
 */
inline static ULONG32 GetUL32(UINT8 *pData)
{
    return (*pData << 24) |
	   (*(pData + 1) << 16) |
	   (*(pData + 2) << 8 ) |
	   *(pData + 3);
}

/****************************************************************************
 *  AVCDecoderConfigurationRecord
 */
AVCDecoderConfigurationRecord::~AVCDecoderConfigurationRecord(void)
{
    HX_DELETE(m_pData);
}
    
HX_RESULT AVCDecoderConfigurationRecord::Unpack(UINT8* &pData, 
						ULONG32 &ulSize, 
						ULONG32 ulFlags)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    if (ulSize >= 7)
    {
	m_ulLength = ulSize;
	ulSize = 0;
	retVal = HXR_OK;
    }
    
    if (retVal == HXR_OK)
    {
	retVal = HXR_OUTOFMEMORY;
	m_pData = new UINT8 [m_ulLength];
	if (m_pData)
	{
	    retVal = HXR_OK;
	}
    }

    if (retVal == HXR_OK)
    {
	memcpy(m_pData, pData, m_ulLength);
	pData += m_ulLength;
    }

    return retVal;
}


/****************************************************************************
 *  AVCConfigurationBox
 */
AVCConfigurationBox::~AVCConfigurationBox(void)
{
    HX_DELETE(m_pAVCDecoderConfigurationRecord);
}

HX_RESULT AVCConfigurationBox::Unpack(UINT8* &pData, 
				     ULONG32 &ulSize,
				     ULONG32 ulFlags)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

#ifdef _SCAN_FOR_AVCC_START
    while ( (ulSize >= 8) && (retVal != HXR_OK) )
    {
    	m_ulSize = GetUL32(pData);
    	if ((m_ulSize >= 8) &&
    	    (ulSize >= m_ulSize) &&
    	    (*(pData + 4) == 'a') &&
    	    (*(pData + 5) == 'v') &&
    	    (*(pData + 6) == 'c') &&
    	    (*(pData + 7) == 'C'))
    	{
    	    retVal = HXR_OK;
    	    pData += 8;
    	    ulSize -= 8;
    	} else {
            if (m_ulSize == 0)
            {
                break;
            }
    	    pData += m_ulSize;
    	    ulSize -= m_ulSize;
    	}
    }
#else
    if (ulSize >= 8)
    {
        m_ulSize = GetUL32(pData);
        if ((m_ulSize >= 8) &&
            (ulSize >= m_ulSize) &&
            (*(pData)+ 4) == 'a') &&
            (*(pData + 5) == 'v') &&
            (*(pData + 6) == 'c') &&
            (*(pData + 7) == 'C'))
        {
            retVal = HXR_OK;
        }
        pData += 8;
        ulSize -= 8;
    }
#endif
    
    if (retVal == HXR_OK)
    {
        UINT32 ulConfigSize = m_ulSize - 8;
        retVal = HXR_OUTOFMEMORY;
        m_pAVCDecoderConfigurationRecord = new AVCDecoderConfigurationRecord();
        if (m_pAVCDecoderConfigurationRecord)
        {
            ulSize -= ulConfigSize;
            retVal = m_pAVCDecoderConfigurationRecord->Unpack(pData, ulConfigSize);
            ulSize += ulConfigSize;
        }
    }

    return retVal;
}
	    
