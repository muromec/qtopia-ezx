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
#define MAX_FLOAT_STRING_LENGTH	    35

/****************************************************************************
 *  Includes
 */
#include <stdio.h>
#include "rendstats.h"
#include "audstats.h"
#include "hxstrutl.h"


/****************************************************************************
 *  CAudioStatistics
 */	
/****************************************************************************
 *  Costructor/Destructor
 */
CAudioStatistics::CAudioStatistics(IUnknown* pContext)
    : m_pRegistry(NULL)
    , m_pDisplay(NULL)
{
    pContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry);

    m_pDisplay = new CRendererStatisticsDisplay(pContext, 
						(UINT32) AS_NUM_ENTRIES);

    HX_ASSERT(m_pDisplay);

    PrimeEntries();
}


CAudioStatistics::~CAudioStatistics()
{
    HX_DELETE(m_pDisplay);
    HX_RELEASE(m_pRegistry);
}


/****************************************************************************
 *  PrimeEntries
 */
HX_RESULT CAudioStatistics::PrimeEntries(void)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pDisplay)
    {
	m_pDisplay->PrimeEntry((UINT32) AS_REND_NAME, "Name", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) AS_CODEC_NAME, "CodecName", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) AS_CODEC_4CC, "CodecFourCC", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) AS_CODEC_VERSION, "CodecVersion", REG_TYPE_NUMBER);
	m_pDisplay->PrimeEntry((UINT32) AS_SURESTREAM, "SureStream", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) AS_CODECS, "CodecsSuite", REG_TYPE_STRING);
	m_pDisplay->PrimeEntry((UINT32) AS_CHANNELS, "Channels", REG_TYPE_NUMBER);
	m_pDisplay->PrimeEntry((UINT32) AS_SAMPLING_RATE, "SamplesPerSec", REG_TYPE_NUMBER);
	m_pDisplay->PrimeEntry((UINT32) AS_SAMPLE_SIZE, "BitsPerSample", REG_TYPE_NUMBER);
	m_pDisplay->PrimeEntry((UINT32) AS_SURROUND, "Surround", REG_TYPE_STRING);

	retVal = HXR_OK;
    }

    return retVal;
}

/****************************************************************************
 *  UpdateStatistics
 */
HX_RESULT CAudioStatistics::DisplayStats(UINT32 ulRegistryID)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pDisplay && (ulRegistryID != 0))
    {
	retVal = HXR_OK;
    }

    // Refresh Display
    if (SUCCEEDED(retVal))
    {
	m_pDisplay->RefreshEntries(ulRegistryID);
    }

    return HXR_OK;
}

