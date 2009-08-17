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
#define _ATTEMPT_ALTERNATE_PARSING_MODES
#define _ATTEMPT_PREOCR_SYNTAX
#define _ALLOW_SHORT_SLCONFIG
#define _SHORTCUT_ON_DECCONFIG


/****************************************************************************
 *  Includes
 */
#include "mp4desc.h"
#include "netbyte.h"


/****************************************************************************
 *  Statics
 */
inline static int swapEndian(UCHAR* pInOut, UINT32 ulLen)
{
    UINT32 i = 0, j = 0;
    UCHAR c;

    for (j = ulLen-1; i < ulLen/2; ++i, --j)
    {
        c = pInOut[i];
        pInOut[i] = pInOut[j];
        pInOut[j] = c;
    }

    return 0;
}

inline static ULONG32 GetUL32(UINT8 *pData)
{
    return (*pData << 24) |
	   (*(pData + 1) << 16) |
	   (*(pData + 2) << 8 ) |
	   *(pData + 3);
}

inline static UINT16 GetUI16(UINT8 *pData)
{
    return (*pData << 8) |
	    *(pData + 1);
}

inline static ULONG32 GetUI24(UINT8 *pData)
{
    return  (*pData << 16) |
	    (*(pData + 1) << 8) |
	    *(pData + 2);
}

inline static ULONG32 RoundUp(double x)
{
    ULONG32 rx = (ULONG32) x;
    
    if (rx != x)
    {
	rx++;
    }
    
    return rx;
}


/****************************************************************************
 *  MP4BaseDescriptor
 */
/****************************************************************************
 *  Public functions
 */
MP4BaseDescriptor::DescType MP4BaseDescriptor::GetDescType(UINT8 uTag)
{
    DescType eDescType = DTYP_UNKNOWN;
    
    if (uTag == 0x03)
    {
	eDescType = DTYP_ESTREAM;
    }
    else if (uTag == 0x04)
    {
	eDescType = DTYP_DECODER_CONFIG;
    }
    else if (uTag == 0x05)
    {
	eDescType = DTYP_DECODER_INFO_SHORT;
    }
    else if (uTag == 0x06)
    {
	eDescType = DTYP_SL_CONFIG;
    }
    
    return eDescType;
}

/****************************************************************************
 *  Protected functions
 */
UINT8 MP4BaseDescriptor::GetTag(UINT8* &pData, 
				ULONG32 &ulDataSize,
				HXBOOL bAdvance)
{
    UINT8 uTag;
    ULONG32 ulTagSize = 0;
    
    uTag = pData[ulTagSize];
    ulTagSize++;
        
    if (bAdvance)
    {
	pData += ulTagSize;
	ulDataSize -= ulTagSize;
    }
    
    return uTag;
}


ULONG32 MP4BaseDescriptor::GetSize(UINT8* &pData, 
				   ULONG32 &ulDataSize,
				   HXBOOL bAdvance)
{
    ULONG32 ulSize = 0;
    ULONG32 ulFieldSize = 0;

    while (ulDataSize > ulFieldSize)
    {
	ulSize = (ulSize << 7);
	ulSize += (pData[ulFieldSize] & 0x7f);
	if ((pData[ulFieldSize] & 0x80) == 0)
	{
	    ulFieldSize++;
	    break;
	}
	ulFieldSize++;
    }
    
    if (bAdvance)
    {
	pData += ulFieldSize;
	ulDataSize -= ulFieldSize;
    }
    
    return ulSize;
}


/****************************************************************************
 *  SLConfigPredefined
 */
/****************************************************************************
 *  Constructor/Destructor
 */


/****************************************************************************
 *  Public functions
 */
HX_RESULT SLConfigPredefined::Unpack(UINT8* &pData, 
				     ULONG32 &ulSize,
				     ULONG32 ulFlags)
{
    LD bitInfo;
    HX_RESULT retVal = HXR_INVALID_PARAMETER;
    
    if (ulSize >= 14)
    {
	retVal = HXR_OK;

	initbuffer(pData, &bitInfo);

	m_bUseAccessUnitStartFlag = getbits1(&bitInfo);
	m_bUseAccessUnitEndFlag = getbits1(&bitInfo);
	m_bUseRandomAccessPointFlag = getbits1(&bitInfo);
	m_bUsePaddingFlag = getbits1(&bitInfo);
	m_bUseTimeStampsFlag = getbits1(&bitInfo);
	m_bUseWallClockTimeStampFlag = getbits1(&bitInfo);
	m_bUseIdleFlag = getbits1(&bitInfo);
	m_bDurationFlag = getbits1(&bitInfo);

	pData++;

	m_ulTimeStampResolution = GetUL32(pData);
	pData += 4;
	m_ulOCRResolution = GetUL32(pData);
	pData += 4;

	m_uTimeStampLength = *(pData++);
	m_uOCRLength = *(pData++);
	m_AU_Length = *(pData++);
	m_uInstantBitrateLength = *(pData++);

	initbuffer(pData, &bitInfo);
	m_uDegradationPriorityLength = getbits(4, &bitInfo);
	m_uSeqNumLength = getbits(4, &bitInfo);
	pData++;

	ulSize -= 14;
    }
    
    if ((retVal == HXR_OK) && 
	m_bDurationFlag)
    {
	retVal = HXR_INVALID_PARAMETER;

	if (ulSize >= 8)
	{
	    m_ulTimeScale = GetUL32(pData);
	    pData += 4;
	    m_uAccessUnitDuration = GetUI16(pData);
	    pData += 2;
	    m_uCompositionUnitDuration = GetUI16(pData);
	    pData += 2;

	    ulSize -= 8;
	}
    }

    if ((retVal == HXR_OK) &&
	(!m_bUseTimeStampsFlag))
    {
	retVal = HXR_INVALID_PARAMETER;
	ULONG32 ulSegmentSize = RoundUp(8.0 + m_uTimeStampLength / 4.0);

	m_ullStartDecodingTimeStamp = 0;
	m_ullStartCompositionTimeStamp = 0;

	if ((m_uTimeStampLength < 64) &&
	    (ulSize >= ulSegmentSize) &&
	    (sizeof(double) == 8))
	{	    
	    memcpy(&m_dWallClockTimeStamp, pData, 8); /* Flawfinder: ignore */
	    if (!TestBigEndian())
	    {
		swapEndian((UINT8*) &m_dWallClockTimeStamp, 8);
	    }

	    // initialization of m_ullStartDecodingTimeStamp
	    // and m_ullStartCompositionTimeStamp needed
	    HX_ASSERT(HXR_OK == HXR_NOTIMPL);

	    pData += ulSegmentSize;
	    ulSize -= ulSegmentSize;

	    retVal = HXR_NOTIMPL;
	}
    }

    return retVal;
}


/****************************************************************************
 *  SLConfigDescriptor
 */
/****************************************************************************
 *  Constructor/Destructor
 */
SLConfigDescriptor::SLConfigDescriptor(void)
    : m_uLength(0)
    , m_pPredefined(NULL)
    , m_bOCRstreamFlag(FALSE)
{
    ;
}

SLConfigDescriptor::~SLConfigDescriptor()
{
    HX_DELETE(m_pPredefined);
}

/****************************************************************************
 *  Public functions
 */
HX_RESULT SLConfigDescriptor::Unpack(UINT8* &pData, 
				     ULONG32 &ulSize,
				     ULONG32 ulFlags)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;
    
    if (ulSize >= 3)
    {
	m_uTag = GetTag(pData, ulSize, TRUE);
	retVal = HXR_OK;
    }
    
    if (retVal == HXR_OK)
    {
	retVal = HXR_INVALID_PARAMETER;

	if (ulSize >= 2)
	{
	    m_uLength = (UINT8) GetSize(pData, ulSize, TRUE);

	    if (ulSize >= 1)
	    {
		m_uPredefined = *(pData++);

		ulSize--;

		retVal = HXR_OK;
	    }
	}
    }

    if ((retVal == HXR_OK) &&
	(m_uPredefined == 0))
    {
	retVal = HXR_OUTOFMEMORY;
	m_pPredefined = new SLConfigPredefined;
	
	if (m_pPredefined)
	{
	    retVal = m_pPredefined->Unpack(pData, ulSize);
	}
    }

    if (retVal == HXR_OK)
    {
#ifndef _ALLOW_SHORT_SLCONFIG
	retVal = HXR_INVALID_PARAMETER;
#endif	// _ALLOW_SHORT_SLCONFIG

	if (ulSize >= 1)
	{
	    m_bOCRstreamFlag = ((((*pData) >> 7) & 0x01) == 1);
	    m_uReserved = ((*(pData++)) & 0x7F);
	    ulSize--;

	    retVal = HXR_OK;
	}
    }

    if ((retVal == HXR_OK) &&
	(m_bOCRstreamFlag))
    {
	retVal = HXR_INVALID_PARAMETER;

	if (ulSize >= 2)
	{
	    m_OCR_ES_Id = GetUI16(pData);
	    pData += 2;
	    ulSize -= 2;

	    retVal = HXR_OK;
	}
    }

    return retVal;
}
	    

/****************************************************************************
 *  DecoderSpecifcInfo
 */
/****************************************************************************
 *  Constructor/Destructor
 */
DecoderSpecifcInfo::DecoderSpecifcInfo(void)
    : m_ulLength(0)
    , m_pData(NULL)
{
    ;
}

DecoderSpecifcInfo::~DecoderSpecifcInfo()
{
    HX_DELETE(m_pData);
}

/****************************************************************************
 *  Public functions
 */
HX_RESULT DecoderSpecifcInfo::Unpack(UINT8* &pData, 
				     ULONG32 &ulSize,
				     ULONG32 ulFlags)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    m_ulFlags = ulFlags;
    
    if (ulSize >= 1)
    {
	m_uTag = GetTag(pData, ulSize, TRUE);
	retVal = HXR_OK;
    }
    
    if (retVal == HXR_OK)
    {
	switch (GetDescType(m_uTag))
	{
	case DTYP_DECODER_INFO_SHORT:
	    m_ulLength = GetSize(pData, ulSize, TRUE);
	    break;
	default:
	    retVal = HXR_INVALID_PARAMETER;
	    break;
	}
    }

    if ((ulFlags & SIZE_HEADER_INCLUSIVE) && (retVal == HXR_OK))
    {
	if (m_ulLength > 2)
	{
	    m_ulLength -= 2;
	}
    }

    if ((retVal == HXR_OK) &&
	(m_ulLength > 0))
    {
	retVal = HXR_INVALID_PARAMETER;

	if (ulSize >= m_ulLength)
	{
	    m_pData = new UINT8 [m_ulLength];
	
	    retVal = HXR_OUTOFMEMORY;
	    if (m_pData)
	    {
		memcpy(m_pData, pData, m_ulLength); /* Flawfinder: ignore */
		pData += m_ulLength;
		ulSize -= m_ulLength;
		retVal = HXR_OK;
	    }
	}
    }

    return retVal;
}


/****************************************************************************
 *  DecoderConfigDescriptor
 */
/****************************************************************************
 *  Constructor/Destructor
 */
DecoderConfigDescriptor::DecoderConfigDescriptor(void)
    : m_uLength(0)
    , m_pDecSpecificInfo(NULL)
{
    ;
}

DecoderConfigDescriptor::~DecoderConfigDescriptor()
{
    HX_DELETE(m_pDecSpecificInfo);
}

/****************************************************************************
 *  Public functions
 */
HX_RESULT DecoderConfigDescriptor::Unpack(UINT8* &pData, 
					  ULONG32 &ulSize,
					  ULONG32 ulFlags)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;
    
    m_ulFlags = ulFlags;

    if (ulSize >= 15)
    {
	m_uTag = GetTag(pData, ulSize, TRUE);
	retVal = HXR_OK;
    }
    
    if (retVal == HXR_OK)
    {
	retVal = HXR_INVALID_PARAMETER;

	if (ulSize >= 14)
	{
	    m_uLength = (UINT8) GetSize(pData, ulSize, TRUE);

	    if (ulSize >= 13)
	    {
		m_uObjectProfileIndication = *(pData++);
		m_uStreamType = ((*pData) >> 2);
		m_bUpStream = (((*pData) >> 1) & 0x01);
		m_bReservedBit = ((*pData) & 0x01);
		pData++;
		m_ulBufferSizeDB = GetUI24(pData);
		pData += 3;
		m_ulMaxBitrate = GetUL32(pData);
		pData += 4;
		m_ulAvgBitrate = GetUL32(pData);
		pData += 4;
		
		ulSize -= 13;
		
		retVal = HXR_OK;
	    }
	}
    }

    if (retVal == HXR_OK)
    {
	if (ulSize > 0)
	{
	    UINT8 uTag = GetTag(pData, ulSize);
	
	    if (GetDescType(uTag) == DTYP_DECODER_INFO_SHORT)
	    {
		m_pDecSpecificInfo = new DecoderSpecifcInfo;
	    
		retVal = HXR_OUTOFMEMORY;
		if (m_pDecSpecificInfo)
		{
		    retVal = m_pDecSpecificInfo->Unpack(pData, 
							ulSize, 
							ulFlags);
		}
	    }
	}
    }

    return retVal;
}
    

/****************************************************************************
 *  ES_Descriptor
 */
/****************************************************************************
 *  Constructor/Destructor
 */
ES_Descriptor::ES_Descriptor(void)
    : m_uLength(0)
    , m_pURLString(NULL)
    , m_pExtDescrArray(NULL)
    , m_pLangDescr(NULL)
    , m_pDecConfigDescr(NULL)
    , m_pSLConfigDescr(NULL)
    , m_pIPIPtr(NULL)
    , m_pIPIDS(NULL)
    , m_pQOSDescr(NULL)
{
    ;
}

ES_Descriptor::~ES_Descriptor(void)
{
    HX_VECTOR_DELETE(m_pURLString);
    HX_DELETE(m_pExtDescrArray);
    HX_DELETE(m_pLangDescr);
    HX_DELETE(m_pDecConfigDescr);
    HX_DELETE(m_pSLConfigDescr);
    HX_DELETE(m_pIPIPtr);
    HX_DELETE(m_pIPIDS);
    HX_DELETE(m_pQOSDescr);
}


/****************************************************************************
 *  Public functions
 */
HX_RESULT ES_Descriptor::Unpack(UINT8* &pData, 
				ULONG32 &ulSize,
				ULONG32 ulFlags)
{
    LD bitInfo;
    UINT8 uTag;
    UINT8* pPreOCRData = NULL;
    ULONG32 ulPreOCRSize = 0;
    UINT8* pLastData = NULL;
    ULONG32 ulLastSize = 0;
    ULONG32 ulStartSize = ulSize;
    HXBOOL bAlternateParseMode = FALSE;
    HXBOOL bAttemptPreOCRSyntax = FALSE;
    HX_RESULT retVal = HXR_INVALID_PARAMETER;
    
    m_ulFlags = ulFlags;

    if (ulSize >= 5)
    {
	m_uTag = GetTag(pData, ulSize, TRUE);
	retVal = HXR_OK;
    }
    
    if (retVal == HXR_OK)
    {
	retVal = HXR_INVALID_PARAMETER;
	if (ulSize >= 4)
	{   
	    m_uLength = (UINT16) GetSize(pData, ulSize, TRUE);
	    
	    if (ulSize >= 3)
	    {
		m_uESid = GetUI16(pData);
		pData += 2;
	    
		initbuffer(pData, &bitInfo);
		m_bStreamDependenceFlag = getbits1(&bitInfo);
		m_bURLflag = getbits1(&bitInfo);
		m_bOCRStreamFlag = (getbits1(&bitInfo) != 0);
		m_uStreamPriority = (UINT8) getbits(5, &bitInfo);
		pData++;
	    
		ulSize -= 3;

		retVal = HXR_OK;
	    }
	}
    }
    
    if (retVal == HXR_OK)
    {
	if (m_bStreamDependenceFlag)
	{
	    retVal = HXR_INVALID_PARAMETER;
	    if (ulSize >= 2)
	    {
		retVal = HXR_OK;
		
		m_uDependsOn_ES_ID = GetUI16(pData);
		pData += 2;
		
		ulSize -= 2;
	    }
	}
    }
    
    if (retVal == HXR_OK)
    {
	if (m_bURLflag)
	{
	    retVal = HXR_INVALID_PARAMETER;
	    if (ulSize >= 1)
	    {
		ULONG32 ulURLLength = pData[0];
		pData++;
		ulSize--;

		if (ulSize >= ulURLLength)
		{
		    m_pURLString = new UINT8 [ulURLLength + 1];
	    
		    retVal = HXR_OUTOFMEMORY;
		    if (m_pURLString)
		    {
			retVal = HXR_OK;
		    }
	    
		    if (retVal == HXR_OK)
		    {
			memcpy(m_pURLString, pData, ulURLLength); /* Flawfinder: ignore */
			m_pURLString[ulURLLength] = '\0';

			pData += ulURLLength;
			ulSize -= ulURLLength;
		    }
		}
	    }
	}
    }
    
    do
    {
	bAttemptPreOCRSyntax = FALSE;

	if (retVal == HXR_OK)
	{
	    if (m_bOCRStreamFlag)
	    {
		pPreOCRData = pData;
		ulPreOCRSize = ulSize;
		
		retVal = HXR_INVALID_PARAMETER;
		if (ulSize >= 2)
		{
		    retVal = HXR_OK;
		    
		    m_uOCR_ES_ID = GetUI16(pData);
		    pData += 2;
		    
		    ulSize -= 2;
		}
	    }
	}
	
	while ((retVal == HXR_OK) &&
	    (ulSize > 0))
	{
	    uTag = GetTag(pData, ulSize);
	    
	    switch (GetDescType(uTag))
	    {
	    case DTYP_DECODER_CONFIG:	    
		pLastData = pData;
		ulLastSize = ulSize;
		
		HX_DELETE(m_pDecConfigDescr);
		m_pDecConfigDescr = new DecoderConfigDescriptor;
		if (bAlternateParseMode)
		{
		    m_ulFlags |= SIZE_HEADER_INCLUSIVE;
		}
		retVal = m_pDecConfigDescr->Unpack(pData, 
		    ulSize, 
		    m_ulFlags);
#ifdef _SHORTCUT_ON_DECCONFIG
		if (SUCCEEDED(retVal))
		{
		    ulSize = 0;
		}
#endif	// _SHORTCUT_ON_DECCONFIG
		break;
		
	    case DTYP_SL_CONFIG:
		HX_DELETE(m_pSLConfigDescr);
		m_pSLConfigDescr = new SLConfigDescriptor;
		retVal = m_pSLConfigDescr->Unpack(pData, ulSize);
		break;
		
	    default:
		retVal = HXR_INVALID_PARAMETER;
		break;
	    }
	    
#ifdef _ATTEMPT_ALTERNATE_PARSING_MODES
	    if (FAILED(retVal) &&
		pLastData &&
		(!bAlternateParseMode))
	    {
		bAlternateParseMode = TRUE;
		pData = pLastData;
		ulSize = ulLastSize;
		pLastData = NULL;
		ulLastSize = 0;
		retVal = HXR_OK;
	    }
#endif	// _ATTEMPT_ALTERNATE_PARSING_MODES
	}
	
	if (FAILED(retVal) && m_bOCRStreamFlag && pPreOCRData)
	{
#ifdef _ATTEMPT_PREOCR_SYNTAX
	    // OCR was introduced later - after Phillips produced its files
	    // and thus some older files have OCR flag and no OCR_ES_ID
	    // since previously the OCR flag bit was considered reserved and
	    // had no meaning.
	    pData = pPreOCRData;
	    ulSize = ulPreOCRSize;
	    m_bOCRStreamFlag = FALSE;
	    bAttemptPreOCRSyntax = TRUE;
	    retVal = HXR_OK;
#endif	// _ATTEMPT_PREOCR_SYNTAX
	}
    } while (bAttemptPreOCRSyntax);
    
    if (SUCCEEDED(retVal))
    {
	if ((ulStartSize - ulSize) == m_uLength)
	{
	    m_ulFlags |= SIZE_HEADER_INCLUSIVE;
	}
    }

    return retVal;
}

