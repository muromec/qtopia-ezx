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
#define _OVERALLOC_CODEC_DATA	3

#define FLUSH_ALL_PACKETS   0xFFFFFFFF

#define CHAR_LF	0x0a
#define CHAR_CR	0x0d

#define FORMAT_PARAMETERS_PREFIX	"a=fmtp:0 "
#define FORMAT_PARAMETERS_PREFIX_SIZE	(sizeof(FORMAT_PARAMETERS_PREFIX) - 1)

#define MAX_AUPACKETS_IN_RTPPACKET	50

#define MAX_PACKET_FRAGMENTS	256
#define MAX_AU_FRAGMENTS	256
#define MAX_AU_PACKETS		256

#define MAX_INT_TEXT_LENGTH	10


/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxcom.h"
#include "hxcomm.h"

#include "hxassert.h"
#include "hxslist.h"
#include "hxstrutl.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxformt.h"
#include "hxrendr.h"
#include "hxformt.h"
#include "hxengin.h"

#include "bitstuff.h"

#include "sdpchunk.h"
#include "sdptools.h"
#include "mp4gpyld.h"


MP4GPayloadFormat::MP4GPayloadFormat()
    : m_lRefCount	    (0)
    , m_pClassFactory	    (NULL)
    , m_pStreamHeader	    (NULL)
    , m_bFlushed	    (FALSE)
    , m_bUsesRTPPackets	    (FALSE)
    , m_bRTPPacketTested    (FALSE)
    , m_bPacketize	    (FALSE)
    , m_bPriorLoss	    (FALSE)
    , m_bStartPacket	    (TRUE)
    , m_ulFrameCount	    (0)
    , m_pPacketFragments    (NULL)
    , m_pAUFragments	    (NULL)
    , m_pAUPackets	    (NULL)
    , m_ulNumPacketFragments(0)
    , m_ulNumAUFragments    (0)
    , m_ulNumAUPackets	    (0)
    , m_ulMaxPacketFragments(MAX_PACKET_FRAGMENTS)
    , m_ulMaxAUFragments    (MAX_AU_FRAGMENTS)
    , m_ulMaxAUPackets	    (MAX_AU_PACKETS)
    , m_bEarliestDeintTimeKnown(FALSE)
    , m_bLastReapedSet	    (FALSE)
    , m_ulSamplesPerSecond  (1000)
    , m_ulRTPSamplesPerSecond(0)
    , m_ulAUDuration	    (0)
{
    ;
}

MP4GPayloadFormat::~MP4GPayloadFormat()
{
    Reset();

    HX_VECTOR_DELETE(m_pPacketFragments);
    HX_VECTOR_DELETE(m_pAUFragments);
    HX_VECTOR_DELETE(m_pAUPackets);

    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pStreamHeader);
}

HX_RESULT MP4GPayloadFormat::Build(REF(IMP4APayloadFormat*) pFmt)
{
    pFmt = new MP4GPayloadFormat();

    HX_RESULT res = HXR_OUTOFMEMORY;
    if (pFmt)
    {
	pFmt->AddRef();
	res = HXR_OK;
    }

    return res;
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your
//	object.
//
STDMETHODIMP
MP4GPayloadFormat::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), this },
	{ GET_IIDHANDLE(IID_IHXPayloadFormatObject), (IHXPayloadFormatObject*) this },
    };
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32)
MP4GPayloadFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32)
MP4GPayloadFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
MP4GPayloadFormat::Init(IUnknown* pContext,
			HXBOOL bPacketize)
{
    HX_RESULT retVal = HXR_OK;

    HX_RELEASE(m_pClassFactory);

    m_bPacketize = bPacketize;

    if (SUCCEEDED(retVal))
    {
	retVal = pContext->QueryInterface(IID_IHXCommonClassFactory,
					  (void**) &m_pClassFactory);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = HXR_OUTOFMEMORY;

	m_pPacketFragments = new IHXPacket* [m_ulMaxPacketFragments];
	m_pAUFragments = new CAUPacket* [m_ulMaxAUFragments];
	m_pAUPackets = new CAUPacket* [m_ulMaxAUPackets];

	if (m_pPacketFragments &&
	    m_pAUFragments &&
	    m_pAUPackets)
	{
	    memset(m_pPacketFragments, 0, sizeof(IHXPacket*) * m_ulMaxPacketFragments);
	    memset(m_pAUFragments, 0, sizeof(CAUPacket*) * m_ulMaxAUFragments);
	    memset(m_pAUPackets, 0, sizeof(CAUPacket*) * m_ulMaxAUPackets);
	    retVal = HXR_OK;
	}
    }

    return retVal;
}

STDMETHODIMP
MP4GPayloadFormat::Reset()
{
    // Release all input packets we have stored
    FlushQueues();
    FlushArrays();

    m_bFlushed = FALSE;
    m_bPriorLoss = FALSE;
    m_bStartPacket = TRUE;
    m_ulFrameCount = 0;

    m_TSConverter.Reset();

    return HXR_OK;
}

STDMETHODIMP
MP4GPayloadFormat::SetStreamHeader(IHXValues* pHeader)
{
    HX_RESULT retVal;

    if (pHeader)
    {
	m_pStreamHeader = pHeader;
	m_pStreamHeader->AddRef();
    }

    if (m_bPacketize)
    {
	retVal = SetPacketizerHeader(pHeader);
    }
    else
    {
	retVal = SetAssemblerHeader(pHeader);
    }

    return retVal;
}

HX_RESULT MP4GPayloadFormat::SetPacketizerHeader(IHXValues* pHeader)
{
    return HXR_OK;
}

HX_RESULT MP4GPayloadFormat::SetAssemblerHeader(IHXValues* pHeader)
{
    IHXBuffer* pMimeType = NULL;
    const char* pMimeTypeData = NULL;
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    if (pHeader)
    {
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	retVal = pHeader->GetPropertyCString("MimeType", pMimeType);
    }

    if (SUCCEEDED(retVal))
    {
	pMimeTypeData = (char*) pMimeType->GetBuffer();

	retVal = HXR_FAIL;
	if (pMimeTypeData)
	{
	    retVal = HXR_OK;
	}
    }

    // Determine payload type here based on mime type
    if (SUCCEEDED(retVal))
    {
	retVal = HXR_FAIL;
	if ((strcasecmp(pMimeTypeData, "audio/mpeg4-simple-A2") == 0) ||
	    (strcasecmp(pMimeTypeData, "audio/x-ralf-mpeg4-generic") == 0) ||
	    (strcasecmp(pMimeTypeData, "audio/mpeg4-generic") == 0) ||
	    (strcasecmp(pMimeTypeData, "video/mpeg4-generic") == 0))
	{
	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	char *pData = NULL;
	char* pAllocData = NULL;
	IHXBuffer* pSDPData = NULL;
	IHXValues* pValues = NULL;
	ULONG32 ulTryIdx = 0;

	do
	{
	    if (ulTryIdx == 0)
	    {
		retVal = pHeader->GetPropertyCString("SDPData", pSDPData);

		if (SUCCEEDED(retVal))
		{
		    pData = (char*) pSDPData->GetBuffer();

		    retVal = HXR_FAIL;
		    if (pData)
		    {
			retVal = HXR_OK;
		    }
		}
	    }
	    else
	    {
		retVal = pHeader->GetPropertyCString("PayloadParameters", pSDPData);

		if (SUCCEEDED(retVal))
		{
		    ULONG32 ulDataLen;

		    pData = (char*) pSDPData->GetBuffer();
		    ulDataLen = strlen(pData);

		    pAllocData = new char [ulDataLen +
			FORMAT_PARAMETERS_PREFIX_SIZE +
			2];

		    retVal = HXR_OUTOFMEMORY;
		    if (pAllocData)
		    {
			strcpy(pAllocData, FORMAT_PARAMETERS_PREFIX); /* Flawfinder: ignore */
			strcpy(pAllocData + FORMAT_PARAMETERS_PREFIX_SIZE, pData); /* Flawfinder: ignore */
			pAllocData[FORMAT_PARAMETERS_PREFIX_SIZE + ulDataLen] =
			    CHAR_LF;
			pAllocData[FORMAT_PARAMETERS_PREFIX_SIZE + 1 + ulDataLen] =
			    '\0';

			pData = pAllocData;
			retVal = HXR_OK;
		    }
		}
	    }

	    if (SUCCEEDED(retVal))
	    {
		retVal = SDPParseChunk(pData,
		    strlen(pData),
		    pValues,
		    m_pClassFactory,
		    SDPCTX_Renderer);
	    }

	    HX_VECTOR_DELETE(pAllocData);

	    if (SUCCEEDED(retVal))
	    {
		retVal = SetAssemblerConfig(pValues);
	    }

	    if (SUCCEEDED(retVal))
	    {
		m_ulAUDuration = m_Config.m_ulConstantDuration;
	    }

	    HX_RELEASE(pSDPData);
	    HX_RELEASE(pValues);

	    ulTryIdx++;
	} while (FAILED(retVal) && (ulTryIdx < 2));
    }

    if (SUCCEEDED(retVal))
    {
	m_pStreamHeader->GetPropertyULONG32("SamplesPerSecond",
					    m_ulRTPSamplesPerSecond);
    }

    HX_RELEASE(pMimeType);

    return retVal;
}

HX_RESULT MP4GPayloadFormat::SetAssemblerConfig(IHXValues* pFMTParams)
{
    return m_Config.Init(pFMTParams, m_pClassFactory, m_pStreamHeader);
}

HX_RESULT MP4GPayloadFormat::CFormatConfig::Init(IHXValues* pFMTParams,
						 IHXCommonClassFactory* pClassFactory,
                         IHXValues* pStreamHeader)
{
    IHXBuffer* pConfigString = NULL;
    IHXBuffer* pModeString = NULL;
    IHXBuffer* pMimeType = NULL;
    const char* pMimeTypeData = NULL;
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(pFMTParams);

    // Obtain Required parameters
    retVal = pFMTParams->GetPropertyULONG32("FMTPstreamtype",
					    m_ulStreamType);
    if(FAILED(retVal))
    {
        // SDP didn't specify streamType, but there's only two choices
        // deduce it from the mime type
        if(pStreamHeader)
        {
	        retVal = pStreamHeader->GetPropertyCString("MimeType", pMimeType);
        }
        

        if (SUCCEEDED(retVal))
        {
	        pMimeTypeData = (char*) pMimeType->GetBuffer();

	        retVal = HXR_FAIL;
	        if (pMimeTypeData)
	        {
	            retVal = HXR_OK;
	        }
        }

        if(SUCCEEDED(retVal))
        {
	    const char* pszAudioPrefix = "audio/";
	    const char* pszVideoPrefix = "video/";
            if(!strncmp(pMimeTypeData, pszAudioPrefix, strlen(pszAudioPrefix)))
            {
                // Table 9 (streamType Values) of ISO/IEC 14496-1 specifies 5 for audio streams
                m_ulStreamType = 5;
                retVal = HXR_OK;
            }
	    else if(!strncmp(pMimeTypeData, pszVideoPrefix, strlen(pszVideoPrefix)))
            {
                // Table 9 (streamType Values) of ISO/IEC 14496-1 specifies 4 for visual streams
                m_ulStreamType = 4;
                retVal = HXR_OK;
            }
            else
            {
                // it didn't specify and we can't deduce, so fail
                retVal = HXR_FAIL;
            }
        }
    }

    if (SUCCEEDED(retVal))
    {
	retVal = pFMTParams->GetPropertyULONG32("FMTPprofile-level-id",
						m_ulProfileLevelID);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = pFMTParams->GetPropertyCString("FMTPconfig",
						pConfigString);

	if (FAILED(retVal))
	{
	    ULONG32 ulConfigVal = 0;

	    retVal = pFMTParams->GetPropertyULONG32("FMTPconfig",
						    ulConfigVal);

	    if (SUCCEEDED(retVal))
	    {
		retVal = pClassFactory->CreateInstance(IID_IHXBuffer,
						       (void**) &pConfigString);
	    }

	    if (SUCCEEDED(retVal))
	    {
		retVal = pConfigString->SetSize(MAX_INT_TEXT_LENGTH + 1);
	    }

	    if (SUCCEEDED(retVal))
	    {
		SafeSprintf((char*) pConfigString->GetBuffer(), MAX_INT_TEXT_LENGTH + 1, "%ld",
			ulConfigVal);
	    }
	}
    }

    // Obtain optional parameters
    if (SUCCEEDED(retVal))
    {
	pFMTParams->GetPropertyCString("FMTPmode", pModeString);
	pFMTParams->GetPropertyULONG32("FMTPConstantSize", m_ulConstantSize);
	pFMTParams->GetPropertyULONG32("FMTPConstantDuration", m_ulConstantDuration);
	pFMTParams->GetPropertyULONG32("FMTPSizeLength", m_ulSizeLength);
	pFMTParams->GetPropertyULONG32("FMTPIndexLength", m_ulIndexLength);
	pFMTParams->GetPropertyULONG32("FMTPIndexDeltaLength", m_ulIndexDeltaLength);
	pFMTParams->GetPropertyULONG32("FMTPCTSDeltaLength", m_ulCTSDeltaLength);
	pFMTParams->GetPropertyULONG32("FMTPDTSDeltaLength", m_ulDTSDeltaLength);
	pFMTParams->GetPropertyULONG32("FMTPAuxDataSizeLength", m_ulAuxDataSizeLength);
	pFMTParams->GetPropertyULONG32("FMTPProfile", m_ulProfile);
    }

    // Perform any needed translations
    if (SUCCEEDED(retVal))
    {
	// Determine Mode from Mode String
	if (pModeString)
	{
	    const char* pMode = (char*) pModeString->GetBuffer();

	    if (pMode)
	    {
		if (strcasecmp(pMode, "A0") == 0)
		{
		    m_Mode = MODE_A0;
		}
		else if (strcasecmp(pMode, "A1") == 0)
		{
		    m_Mode = MODE_A1;
		}
		else if (strcasecmp(pMode, "A2") == 0)
		{
		    m_Mode = MODE_A2;
		}
	    }
	}

	// Convert decoder configuration from ascii hex to binary
	if (pConfigString)
	{
	    ULONG32 ulConfigSize;
	    const char* pConfig = (char*) pConfigString->GetBuffer();
	    IHXBuffer* pConfigBuffer = NULL;

	    if (pConfig)
	    {
		ulConfigSize = strlen(pConfig) / 2;

		if (ulConfigSize > 0)
		{
		    retVal = pClassFactory->CreateInstance(IID_IHXBuffer,
							   (void**) &pConfigBuffer);

		    if (SUCCEEDED(retVal))
		    {
			retVal = pConfigBuffer->SetSize(ulConfigSize);
		    }

		    if (SUCCEEDED(retVal))
		    {
			retVal = HexStringToBinary(pConfigBuffer->GetBuffer(),
						   pConfig);
		    }
		}
	    }

	    if (SUCCEEDED(retVal))
	    {
		m_pConfig = pConfigBuffer;

		if (m_pConfig)
		{
		    m_pConfig->AddRef();
		}
	    }

	    HX_RELEASE(pConfigBuffer);
	}
    }

    // Compute dervied parameter
    if (SUCCEEDED(retVal))
    {
	m_bHasAUHeaders = ((m_ulSizeLength != 0) ||
			   (m_ulIndexLength != 0) ||
			   (m_ulIndexDeltaLength != 0) ||
			   (m_ulCTSDeltaLength != 0) ||
			   (m_ulDTSDeltaLength != 0));
    }

    HX_RELEASE(pModeString);
    HX_RELEASE(pConfigString);

    return retVal;
}

STDMETHODIMP
MP4GPayloadFormat::GetStreamHeader(REF(IHXValues*) pHeader)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pStreamHeader)
    {
	retVal = HXR_OK;
	pHeader = m_pStreamHeader;
	pHeader->AddRef();
    }

    return retVal;
}

STDMETHODIMP
MP4GPayloadFormat::SetPacket(IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(pPacket);

    if (!m_bRTPPacketTested)
    {
	IHXRTPPacket* pRTPPacket = NULL;

	m_bUsesRTPPackets = (pPacket->QueryInterface(
				IID_IHXRTPPacket,
				(void**) &pRTPPacket)
			    == HXR_OK);

	m_bRTPPacketTested = TRUE;

	HX_RELEASE(pRTPPacket);

	if (m_bUsesRTPPackets)
	{
	    if (m_ulRTPSamplesPerSecond == 0)
	    {
		m_ulRTPSamplesPerSecond = m_ulSamplesPerSecond;
	    }

	    HX_ASSERT(m_ulSamplesPerSecond != 0);
	}
	else
	{
	    m_ulRTPSamplesPerSecond = 1000; // RDT time stamp
	}

	m_TSConverter.SetBase(m_ulRTPSamplesPerSecond,
			      m_ulSamplesPerSecond);
    }

    if (m_bPacketize)
    {
	retVal = SetPacketizerPacket(pPacket);
    }
    else
    {
	retVal = SetAssemblerPacket(pPacket);
    }

    if (retVal == HXR_OK)
    {
	// Add this packet to our list of input packets
	pPacket->AddRef();
	m_InputQueue.AddTail(pPacket);
    }
    else if (retVal == HXR_NO_DATA)
    {
	retVal = HXR_OK;
    }

    return retVal;
}

HX_RESULT MP4GPayloadFormat::SetPacketizerPacket(IHXPacket* pPacket)
{
    return HXR_NOTIMPL;
}

HX_RESULT MP4GPayloadFormat::SetAssemblerPacket(IHXPacket* pPacket)
{
    ULONG32 ulPacketTime;
    HX_RESULT retVal = HXR_NO_DATA;

    if (!pPacket->IsLost())
    {
	ulPacketTime = GetPacketTime(pPacket);

	if (m_bStartPacket)
	{
	    m_bStartPacket = FALSE;
	    m_ulLastPacketTime = ulPacketTime;
	}
	else if (ulPacketTime != m_ulLastPacketTime)
	{
	    m_ulFrameCount++;
	    m_ulLastPacketTime = ulPacketTime;
	}

	if (pPacket->GetASMRuleNumber() == 1)
	{
	    m_ulFrameCount++;
	    m_bStartPacket = TRUE;
	}

	retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP
MP4GPayloadFormat::GetPacket(REF(IHXPacket*) pOutPacket)
{
    HX_RESULT retVal = HXR_OK;

    if (m_bPacketize)
    {
	retVal = GetPacketizerPacket(pOutPacket);
    }
    else
    {
	retVal = GetAssemblerPacket(pOutPacket);
    }

    return retVal;
}

HX_RESULT MP4GPayloadFormat::GetPacketizerPacket(IHXPacket* &pOutPacket)
{
    HX_RESULT retVal = HXR_NOTIMPL;

    return retVal;
}

HX_RESULT MP4GPayloadFormat::GetAssemblerPacket(IHXPacket* &pOutPacket)
{
    HX_RESULT retVal = HXR_NOTIMPL;

    return retVal;
}


HX_RESULT MP4GPayloadFormat::CreateMediaPacket(CMediaPacket* &pOutMediaPacket)
{
    HX_RESULT retVal = HXR_OK;
    CMediaPacket* pMediaPacket;

    pMediaPacket = StripPacket();

    if (!pMediaPacket)
    {
	retVal = ProducePackets();

	if (retVal == HXR_OK)
	{
	    pMediaPacket = StripPacket();
	}
	else if ((retVal == HXR_NO_DATA) &&
	         m_bFlushed)
	{
	    retVal = HXR_STREAM_DONE;
	}
    }

    if (retVal == HXR_OK)
    {
	pOutMediaPacket = pMediaPacket;
    }

    return retVal;
}

HX_RESULT MP4GPayloadFormat::SetSamplingRate(ULONG32 ulSamplesPerSecond)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (!m_bRTPPacketTested)
    {
	m_ulSamplesPerSecond = ulSamplesPerSecond;
	retVal = HXR_OK;
    }
    
    return retVal;
}

ULONG32 MP4GPayloadFormat::GetTimeBase(void)
{
    return m_ulSamplesPerSecond;
}

HX_RESULT MP4GPayloadFormat::SetAUDuration(ULONG32 ulAUDuration)
{
    HX_RESULT retVal = HXR_OK;

    if (m_Config.m_ulConstantDuration == 0)
    {
	CTSConverter tempConverter;

	tempConverter.SetBase(m_ulSamplesPerSecond, m_ulRTPSamplesPerSecond);
	m_ulAUDuration = tempConverter.ConvertVector(ulAUDuration);
    }

    return retVal;
}

HX_RESULT MP4GPayloadFormat::SetTimeAnchor(ULONG32 ulTimeMs)
{
    CTSConverter tempConverter;
    ULONG32 ulSamplesAnchor;
    ULONG32 ulRTPSamplesAnchor;

    tempConverter.SetBase(1000, m_ulSamplesPerSecond);
    ulSamplesAnchor = tempConverter.ConvertVector(ulTimeMs);

    tempConverter.SetBase(1000, m_ulRTPSamplesPerSecond);
    ulRTPSamplesAnchor = tempConverter.ConvertVector(ulTimeMs);

    m_TSConverter.SetAnchor(ulRTPSamplesAnchor, ulSamplesAnchor);

    return HXR_OK;
}

CMediaPacket* MP4GPayloadFormat::StripPacket(void)
{
    CMediaPacket* pMediaPacket = NULL;

    if (!m_OutputQueue.IsEmpty())
    {
	pMediaPacket = (CMediaPacket*) m_OutputQueue.RemoveHead();
    }

    return pMediaPacket;
}


HX_RESULT MP4GPayloadFormat::ProducePackets(void)
{
    HXBOOL bTryAgain;
    HX_RESULT retVal = HXR_OK;

    do
    {
	bTryAgain = FALSE;

	retVal = CollectPacketFragments(m_pPacketFragments,	// out
					m_ulNumPacketFragments,	// out
					m_ulMaxPacketFragments);// out

	if (retVal == HXR_OK)
	{
	    retVal = ParsePacketFragments(m_pAUFragments,	    // out
					  m_ulNumAUFragments,	    // out
					  m_ulMaxAUFragments,	    // in/out
					  m_pPacketFragments,	    // in
					  m_ulNumPacketFragments);  // in

	    if (retVal == HXR_OK)
	    {
		retVal = AggregateAUFragments(m_pAUPackets,	    // out
					      m_ulNumAUPackets,	    // out
					      m_ulMaxAUPackets,	    // in/out
					      m_pAUFragments,	    // in
					      m_ulNumAUFragments);  // in
	    }

	    bTryAgain = (retVal != HXR_OK);

	    if (retVal == HXR_OK)
	    {
		retVal = DeinterleaveAUPackets(m_pAUPackets,
					       m_ulNumAUPackets);
	    }
	}
    } while (bTryAgain);

    if (SUCCEEDED(retVal))
    {
	retVal = ReapMediaPacket(m_ulLastPacketTime);
    }

    return retVal;
}


HX_RESULT MP4GPayloadFormat::CollectPacketFragments(IHXPacket** &pPacketFragments,
						    ULONG32 &ulNumPacketFragments,
						    ULONG32 &ulMaxPacketFragments)
{
    ULONG32 ulPacketTime;
    IHXPacket* pPacket;
    HX_RESULT retVal = HXR_NO_DATA;

    FlushPacketArray(pPacketFragments, ulNumPacketFragments);

    if (m_ulFrameCount != 0)
    {
	pPacket = (IHXPacket*) m_InputQueue.RemoveHead();
	pPacketFragments[ulNumPacketFragments++] = pPacket;

	if (pPacket->GetASMRuleNumber() == 0)
	{
	    ulPacketTime = GetPacketTime(pPacket);

	    pPacket = (IHXPacket*) m_InputQueue.GetHead();

	    while (GetPacketTime(pPacket) == ulPacketTime)
	    {
		pPacket = (IHXPacket*) m_InputQueue.RemoveHead();
		pPacketFragments[ulNumPacketFragments++] = pPacket;

		if (pPacket->GetASMRuleNumber() != 0)
		{
		    // the last fragment carries the M(arker) bit, which is
		    // signalled by a rule != 0
		    break;
		}

		pPacket = (IHXPacket*) m_InputQueue.GetHead();
	    }
	}

	HX_ASSERT(ulNumPacketFragments <= ulMaxPacketFragments);

	m_ulFrameCount--;
	retVal = HXR_OK;
    }

    return retVal;
}


HX_RESULT MP4GPayloadFormat::ParsePacketFragments(CAUPacket** &pAUFragments,
						  ULONG32 &ulNumAUFragments,
						  ULONG32 &ulMaxAUFragments,
						  IHXPacket** pPacketFragments,
						  ULONG32& ulNumPacketFragments)
{
    HX_RESULT retVal;
    ULONG32 ulIdx = 0;

    FlushAUArray(pAUFragments, ulNumAUFragments);

    do
    {
	retVal = ParsePacketFragment(pAUFragments,
				     ulNumAUFragments,
				     ulMaxAUFragments,
				     pPacketFragments[ulIdx]);

	pPacketFragments[ulIdx]->Release();
	pPacketFragments[ulIdx] = NULL;

	ulIdx++;
    } while ((retVal == HXR_OK) && (ulIdx < ulNumPacketFragments));

    if (ulIdx >= ulNumPacketFragments)
    {
	ulNumPacketFragments = 0;
    }

    return retVal;
}


HX_RESULT MP4GPayloadFormat::ParsePacketFragment(CAUPacket** &pAUFragments,
						 ULONG32 &ulNumAUFragments,
						 ULONG32 &ulMaxAUFragments,
						 IHXPacket* pPacketFragment)
{
    ULONG32 ulFirstAUFragment = ulNumAUFragments;
    ULONG32 ulStartCTS = GetPacketTime(pPacketFragment);
    ULONG32 ulCTS = ulStartCTS;
    ULONG32 ulDTS = ulCTS;
    ULONG32 ulIdx = 0;
    ULONG32 ulBaseIdx = 0;
    ULONG32 ulVal = 0;
    ULONG32 ulAUHdrStart = 0;
    ULONG32 ulAUHdrLength = 0;	// in bits
    ULONG32 ulAUHdrSize = 0;	// in bytes
    ULONG32 ulAuxStart = 0;
    ULONG32 ulAuxLength = 0;	// in bits
    ULONG32 ulAuxSize = 0;	// in bytes
    ULONG32 ulAUStart = 0;
    ULONG32 ulAUSizeSum = 0;
    ULONG32 ulAUSize = 0;
    ULONG32 ulFixedSizeAUCount = 0;
    IHXBuffer* pBuffer = NULL;
    UINT8* pData = NULL;
    LD bitInfo;
    CAUPacket* pAUPacket = 0;
    HXBOOL bFirstAU = TRUE;
    HX_RESULT retVal = HXR_FAIL;

    HX_ASSERT(pPacketFragment);

    pBuffer = pPacketFragment->GetBuffer();
    if (pBuffer)
    {
	pData = pBuffer->GetBuffer();
	if (pData)
	{
	     retVal = HXR_OK;
	}
	pBuffer->Release();
    }

    if (retVal == HXR_OK)
    {
	// AU-headers-length
	if (m_Config.m_bHasAUHeaders)
	{
	    ulAUHdrLength = (pData[0] << 8) + (pData[1]);
	    ulAUHdrSize = ulAUHdrLength / 8;
	    if ((ulAUHdrSize * 8) != ulAUHdrLength)
	    {
		ulAUHdrSize++;
	    }

	    ulAUHdrStart += 2;
	}

	if (ulAUHdrLength > 0)
	{
	    // AU headers are present
	    initbuffer(pData + ulAUHdrStart, &bitInfo);

	    // AU-headers
	    do
	    {
		pAUPacket = new CAUPacket;

		if (!pAUPacket)
		{
		    retVal = HXR_OUTOFMEMORY;
		    break;
		}

		// Buffer
		pAUPacket->m_pBuffer = pBuffer;
		pBuffer->AddRef();

		// Data
		pAUPacket->m_pData = pData + ulAUSizeSum;

		// AU - size
		if (m_Config.m_ulSizeLength > 0)
		{
		    pAUPacket->m_ulSize = getbits(m_Config.m_ulSizeLength, &bitInfo);
		}
		else
		{
		    pAUPacket->m_ulSize = m_Config.m_ulConstantSize;
		}

		ulAUSizeSum += pAUPacket->m_ulSize;

		// AU - index
		if (bFirstAU)
		{
		    bFirstAU = FALSE;
		    pAUPacket->m_bMinTime = FALSE;

		    if (m_Config.m_ulIndexLength > 0)
		    {
			ulIdx = getbits(m_Config.m_ulIndexLength,
					&bitInfo);
		    }
		    else
		    {
			ulIdx = 0;
		    }

		    ulBaseIdx = ulIdx;
		}
		else
		{
		    pAUPacket->m_bMinTime = TRUE;

		    if (m_Config.m_ulIndexDeltaLength > 0)
		    {
			ulIdx += getbits(m_Config.m_ulIndexDeltaLength,
					 &bitInfo);
		    }

		    ulIdx++;
		}

		pAUPacket->m_ulIdx = ulIdx;

		// CTS
		ulVal = 0;
		if ((m_Config.m_ulCTSDeltaLength > 0) &&
		    getbits1(&bitInfo))
		{
		    ulVal = getbits(m_Config.m_ulCTSDeltaLength, &bitInfo);

		    ulVal = SignBitField(ulVal, m_Config.m_ulCTSDeltaLength);

		    ulVal = m_TSConverter.ConvertVector(ulVal);

		    pAUPacket->m_bMinTime = FALSE;
		}
		else
		{
		    ulVal = (ulIdx - ulBaseIdx) * m_TSConverter.ConvertVector(m_ulAUDuration);
		    pAUPacket->m_bMinTime = (pAUPacket->m_bMinTime &&
					     (ulVal == 0));
		}

		ulCTS = ulStartCTS + ulVal;
		pAUPacket->m_ulCTS = ulCTS;

		// DTS
		ulVal = 0;
		if (m_Config.m_ulDTSDeltaLength > 0)
		{
		    if (getbits1(&bitInfo))
		    {
			ulVal = getbits(m_Config.m_ulDTSDeltaLength, &bitInfo);

			// can be negative value
			if (ulVal >> (m_Config.m_ulDTSDeltaLength - 1))
			{
			    ulVal |= (~((1 << m_Config.m_ulDTSDeltaLength) - 1));
			}

			ulVal = m_TSConverter.ConvertVector(ulVal);
		    }
		}

		ulDTS = ulCTS + ulVal;
		pAUPacket->m_ulDTS = ulDTS;

		pAUFragments[ulNumAUFragments++] = pAUPacket;
	    } while (ulAUHdrLength > ((ULONG32) bitInfo.bitcnt) && ulMaxAUFragments > ulNumAUFragments);
	}
    }

    if (retVal == HXR_OK)
    {
	// auxiliary data length
	ulAuxStart = ulAUHdrStart + ulAUHdrSize;

	ulAuxLength = m_Config.m_ulAuxDataSizeLength;
	if (ulAuxLength > 0)
	{
	    initbuffer(pData + ulAuxStart, &bitInfo);
	    ulAuxLength += getbits(ulAuxLength, &bitInfo);
	}
	ulAuxSize = ulAuxLength / 8;
	if ((ulAuxSize * 8) != ulAuxLength)
	{
	    ulAuxSize++;
	}

	ulAUStart = ulAuxStart + ulAuxSize;

	if (ulAUHdrSize > 0)
	{
	    // AU Headers were present and locations of AUs is
	    // known - just offset the AU location for inserted
	    // auxiliary data
	    for (ulIdx = ulFirstAUFragment; ulIdx < ulNumAUFragments; ulIdx++)
	    {
		pAUFragments[ulIdx]->m_pData += ulAUStart;
	    }
	}
	else
	{
	    // AU Headers are not present - rely on
	    // constant size configuration te determine the
	    // AU offsets
	    ulAUSizeSum = pBuffer->GetSize() - ulAUStart;
	    ulAUSize = m_Config.m_ulConstantSize;

	    if (ulAUSize == 0)
	    {
		ulAUSize = ulAUSizeSum;
	    }

	    if (ulAUSize > 0)
	    {
		if (ulAUSize <= ulAUSizeSum)
		{
		    ulFixedSizeAUCount = ulAUSizeSum / ulAUSize;
		}
		else
		{
		    // this can only happen if we have pre-configured AU sizes,
		    // no AU headers, and this AU has been fragmented.
		    if (ulAUSizeSum != 0)
		    {
			ulFixedSizeAUCount = 1;
		    }
		}

		if (ulFixedSizeAUCount > 0)
		{
		    ulIdx = 0;
		    retVal = HXR_OK;

		    do
		    {
			pAUPacket = new CAUPacket;

			if (!pAUPacket)
			{
			    retVal = HXR_OUTOFMEMORY;
			    break;
			}

			// Buffer
			pAUPacket->m_pBuffer = pBuffer;
			pBuffer->AddRef();

			// Data
			pAUPacket->m_pData = pData + ulAUStart + ulIdx * ulAUSize;

			// AU - size
			pAUPacket->m_ulSize = ulAUSize;

			// AU - index
			pAUPacket->m_ulIdx = ulIdx;

			// CTS
			ulVal = ulIdx * m_TSConverter.ConvertVector(m_ulAUDuration);
			pAUPacket->m_ulCTS = ulStartCTS + ulVal;
			if ((ulVal == 0) && (ulIdx != 0))
			{
			    pAUPacket->m_bMinTime = TRUE;
			}

			// DTS
			pAUPacket->m_ulDTS = pAUPacket->m_ulCTS;

			ulIdx++;
			pAUFragments[ulNumAUFragments++] = pAUPacket;
		    } while (ulIdx < ulFixedSizeAUCount && ulNumAUFragments < ulMaxAUFragments);
		}
	    }
	}
    }

    return retVal;
}


HX_RESULT MP4GPayloadFormat::AggregateAUFragments(CAUPacket** &pAUPackets,
						  ULONG32 &ulNumAUPackets,
						  ULONG32 &ulMaxAUPackets,
						  CAUPacket** pAUFragments,
						  ULONG32 &ulNumAUFragments)
{
    HX_RESULT retVal = HXR_OK;

    FlushAUArray(pAUPackets, ulNumAUPackets);

    if (IsFragmentCollection(pAUFragments, ulNumAUFragments))
    {
	// Compute actual fragment collection size
	ULONG32 ulFragmentyIdx;
	UINT32 ulHeaderSize;
	ULONG32 ulActualCollectionSize = 0;

	for (ulFragmentyIdx = 0;
	     ulFragmentyIdx < ulNumAUFragments;
	     ulFragmentyIdx++)
	{
	    ulHeaderSize = (UINT32) (pAUFragments[ulFragmentyIdx]->m_pData -
				     pAUFragments[ulFragmentyIdx]->m_pBuffer->GetBuffer());
	    ulActualCollectionSize += (pAUFragments[ulFragmentyIdx]->m_pBuffer->GetSize() -
				       ulHeaderSize);
	}

	if (ulActualCollectionSize == pAUFragments[0]->m_ulSize)
	{
	    // All fragments are accounted for - aggregate all
	    IHXBuffer* pBuffer = NULL;
	    CAUPacket* pAUPacket = new CAUPacket;

	    retVal = HXR_OUTOFMEMORY;

	    if (pAUPacket)
	    {
		retVal = HXR_OK;
	    }

	    // Create Buffer
	    if (SUCCEEDED(retVal))
	    {
		retVal = m_pClassFactory->CreateInstance(IID_IHXBuffer,
							 (void**) &pBuffer);
	    }

	    if (SUCCEEDED(retVal))
	    {
		retVal = pBuffer->SetSize(ulActualCollectionSize);
	    }

	    // Aggregate into the Buffer
	    if (SUCCEEDED(retVal))
	    {
		UINT8* pDataWriter = pBuffer->GetBuffer();
		UINT32 ulFragmentSize;

		for (ulFragmentyIdx = 0;
		     ulFragmentyIdx < ulNumAUFragments;
		     ulFragmentyIdx++)
		{
		    ulHeaderSize = (UINT32) (pAUFragments[ulFragmentyIdx]->m_pData -
					     pAUFragments[ulFragmentyIdx]->m_pBuffer->GetBuffer());
		    ulFragmentSize = pAUFragments[ulFragmentyIdx]->m_pBuffer->GetSize() -
				     ulHeaderSize;
		    memcpy(pDataWriter,	    /* Flawfinder: ignore */
			   pAUFragments[ulFragmentyIdx]->m_pData,
			   ulFragmentSize);

		    pDataWriter += ulFragmentSize;
		}
	    }

	    // Populate the aggregated AUPacket
	    if (SUCCEEDED(retVal))
	    {
		// Buffer
		pAUPacket->m_pBuffer = pBuffer;
		pBuffer = NULL;

		// Data
		pAUPacket->m_pData = pAUPacket->m_pBuffer->GetBuffer();

		// AU - size
		pAUPacket->m_ulSize = ulActualCollectionSize;

		// AU - index
		pAUPacket->m_ulIdx = pAUFragments[0]->m_ulIdx;

		// CTS
		pAUPacket->m_ulCTS = pAUFragments[0]->m_ulCTS;
		pAUPacket->m_bMinTime = pAUFragments[0]->m_bMinTime;

		// DTS
		pAUPacket->m_ulDTS = pAUFragments[0]->m_ulDTS;

		// Set to output
		pAUPackets[0] = pAUPacket;
		ulNumAUPackets = 1;
		pAUPacket = NULL;
	    }

	    HX_RELEASE(pBuffer);
	    HX_DELETE(pAUPacket);
	}
	else
	{
	    // Something is missing or is corrupted - discard data
	    retVal = HXR_FAIL;
	}

	// Flush the input fragmants
	FlushAUArray(pAUFragments, ulNumAUFragments);
    }
    else
    {
	// No aggregation necessary
	ulNumAUPackets = ulNumAUFragments;
	memcpy(pAUPackets, pAUFragments, sizeof(CAUPacket*) * ulNumAUFragments); /* Flawfinder: ignore */
	memset(pAUFragments, 0, sizeof(CAUPacket*) * ulNumAUFragments);
	ulNumAUFragments = 0;
    }
    
    return retVal;
}

HXBOOL MP4GPayloadFormat::IsFragmentCollection(CAUPacket** pAUFragments,
					     ULONG32 ulNumAUFragments)
{
    HXBOOL bRetVal = FALSE;

    if (ulNumAUFragments > 0)
    {
	// Even if we have only one fragment, this could still be a fragment
	// collection in which all but one fragment were lost
	UINT32 ulHeaderSize = (UINT32) (pAUFragments[0]->m_pData -
				        pAUFragments[0]->m_pBuffer->GetBuffer());

	bRetVal = ((ulHeaderSize + pAUFragments[0]->m_ulSize)
		   > pAUFragments[0]->m_pBuffer->GetSize());
    }

    return bRetVal;
}


HX_RESULT MP4GPayloadFormat::DeinterleaveAUPackets(CAUPacket** pAUPackets,
						   ULONG32 &ulNumAUPackets)
{
    ULONG32 ulIdx;
    HX_RESULT retVal = HXR_OK;

    for (ulIdx = 0; ulIdx < ulNumAUPackets; ulIdx++)
    {
	if (SUCCEEDED(DeinterleaveAUPacket(pAUPackets[ulIdx])))
	{
	    pAUPackets[ulIdx] = NULL;
	}
	else
	{
	    HX_DELETE(pAUPackets[ulIdx]);
	}
    }

    ulNumAUPackets = 0;

    return retVal;
}


HX_RESULT MP4GPayloadFormat::DeinterleaveAUPacket(CAUPacket* pAUPacket)
{
    ULONG32 ulCount = m_DeinterleaveQueue.GetCount();

    if (ulCount == 0)
    {
	m_DeinterleaveQueue.AddTail(pAUPacket);
    }
    else
    {
	CAUPacket* pListAUPacket;
	LISTPOSITION lPos = m_DeinterleaveQueue.GetTailPosition();
	LISTPOSITION lInsertPos = lPos;
	pListAUPacket = (CAUPacket*) m_DeinterleaveQueue.GetAt(lPos);

	do
	{
	    if (!MayAUPacketPrecede(pAUPacket, pListAUPacket))
	    {
		m_DeinterleaveQueue.InsertAfter(lInsertPos, pAUPacket);
		break;
	    }

	    ulCount--;
	    if (ulCount == 0)
	    {
		CAUPacket lastReapedAUPacket;

		lastReapedAUPacket.m_ulDTS = m_ulLastReapedDTS;
		lastReapedAUPacket.m_ulIdx = m_ulLastReapedIdx;
		lastReapedAUPacket.m_bMinTime = m_bLastReapedMinTime;

		if (m_bLastReapedSet &&
		    MayAUPacketPrecede(&lastReapedAUPacket, pAUPacket))
		{
		    m_DeinterleaveQueue.InsertBefore(lPos, pAUPacket);
		}
		else
		{
		    m_DeinterleaveQueue.InsertAfter(lInsertPos, pAUPacket);
		}
		break;
	    }

	    pListAUPacket = (CAUPacket*) m_DeinterleaveQueue.GetAtPrev(lPos);

	    if (MayAUPacketPrecede(pListAUPacket, pAUPacket))
	    {
		lInsertPos = lPos;
	    }
	} while (TRUE);
    }

    return HXR_OK;
}


HX_RESULT MP4GPayloadFormat::ReapMediaPacket(ULONG32 ulLastPacketTime)
{
    CAUPacket* pAUPacket;
    CMediaPacket* pMediaPacket;
    HX_RESULT retVal = HXR_NO_DATA;

    if (!m_DeinterleaveQueue.IsEmpty())
    {
	HXBOOL bTryAgain;
	HXBOOL bUnknownTime;

	if ((!m_bEarliestDeintTimeKnown) && (!m_bFlushed))
	{
	    m_bEarliestDeintTimeKnown = FindEarliestKnownDeintTime(m_ulEarliestDeintTimeKnown);
	}

	do
	{
	    bTryAgain = FALSE;

	    pAUPacket = (CAUPacket*) m_DeinterleaveQueue.GetHead();

	    if ((m_bEarliestDeintTimeKnown &&
		 (!TestTimeGreater(m_ulEarliestDeintTimeKnown, m_ulLastPacketTime)))
		||
		(m_bLastReapedSet &&
		 (IndexDiff(pAUPacket->m_ulIdx, m_ulLastReapedIdx) == 1))
		||
		m_bFlushed)
	    {
		ULONG32 ulFlags = MDPCKT_USES_IHXBUFFER_FLAG;

		pAUPacket = (CAUPacket*) m_DeinterleaveQueue.GetHead();

		bUnknownTime = pAUPacket->m_bMinTime;

		if (bUnknownTime)
		{
		    ulFlags |= MDPCKT_HAS_UKNOWN_TIME_FLAG;
		}
		else
		{
		    m_bEarliestDeintTimeKnown = FALSE;
		}

		if (m_bLastReapedSet)
		{
		    if (IndexDiff(pAUPacket->m_ulIdx, m_ulLastReapedIdx) != 1)
		    {
			ulFlags |= MDPCKT_FOLLOWS_LOSS_FLAG;
		    }
		}

		m_bLastReapedMinTime = pAUPacket->m_bMinTime;
		m_ulLastReapedDTS = pAUPacket->m_ulCTS;
		m_ulLastReapedIdx = pAUPacket->m_ulIdx;
		m_bLastReapedSet = TRUE;

		pMediaPacket = new CMediaPacket(pAUPacket->m_pBuffer,
						pAUPacket->m_pData,
						pAUPacket->m_ulSize,
						pAUPacket->m_ulSize,
						pAUPacket->m_ulCTS,
						ulFlags,
						NULL);

		retVal = HXR_OUTOFMEMORY;
		if (pMediaPacket)
		{
		    m_DeinterleaveQueue.RemoveHead();
		    delete pAUPacket;
		    m_OutputQueue.AddTail(pMediaPacket);
		    bTryAgain = TRUE;
		    retVal = HXR_OK;
		}
	    }
	    else if ((!m_bEarliestDeintTimeKnown) && (!m_bFlushed))
	    {
		m_bEarliestDeintTimeKnown = FindEarliestKnownDeintTime(m_ulEarliestDeintTimeKnown);
		bTryAgain = m_bEarliestDeintTimeKnown;
	    }
	} while (bTryAgain &&
		 (!m_DeinterleaveQueue.IsEmpty()));
    }

    return retVal;
}


STDMETHODIMP
MP4GPayloadFormat::Flush()
{
    m_bFlushed = TRUE;

    return HXR_OK;
}

void MP4GPayloadFormat::FlushQueues(void)
{
    FlushInputQueue(FLUSH_ALL_PACKETS);
    FlushDeinterleaveQueue(FLUSH_ALL_PACKETS);
    FlushOutputQueue(FLUSH_ALL_PACKETS);
}

void MP4GPayloadFormat::FlushInputQueue(ULONG32 ulCount)
{
    IHXPacket* pDeadPacket;

    while ((ulCount > 0) && (!m_InputQueue.IsEmpty()))
    {
	pDeadPacket = (IHXPacket*) m_InputQueue.RemoveHead();
	HX_RELEASE(pDeadPacket);
	if (ulCount != FLUSH_ALL_PACKETS)
	{
	    ulCount--;
	}
    }
}

void MP4GPayloadFormat::FlushDeinterleaveQueue(ULONG32 ulCount)
{
    CAUPacket* pDeadPacket;

    while ((ulCount > 0) && (!m_DeinterleaveQueue.IsEmpty()))
    {
	pDeadPacket = (CAUPacket*) m_DeinterleaveQueue.RemoveHead();
	HX_DELETE(pDeadPacket);
	if (ulCount != FLUSH_ALL_PACKETS)
	{
	    ulCount--;
	}
    }

    m_bEarliestDeintTimeKnown = FALSE;
    m_bLastReapedSet = FALSE;
}

void MP4GPayloadFormat::FlushOutputQueue(ULONG32 ulCount)
{
    CMediaPacket* pDeadPacket;

    while ((ulCount > 0) && (!m_OutputQueue.IsEmpty()))
    {
	pDeadPacket = (CMediaPacket*) m_OutputQueue.RemoveHead();
	HX_DELETE(pDeadPacket);
	if (ulCount != FLUSH_ALL_PACKETS)
	{
	    ulCount--;
	}
    }
}

HXBOOL MP4GPayloadFormat::FindEarliestKnownDeintTime(ULONG32 &ulEarliestKnownDeintTime)
{
    ULONG32 ulCount;
    LISTPOSITION lPos;
    CAUPacket* pPacket;

    ulCount = m_DeinterleaveQueue.GetCount();

    if (ulCount > 0)
    {
	lPos = m_DeinterleaveQueue.GetHeadPosition();
	pPacket = (CAUPacket*) m_DeinterleaveQueue.GetHead();

	do
	{
	    if (!pPacket->m_bMinTime)
	    {
		ulEarliestKnownDeintTime = pPacket->m_ulCTS;
		return TRUE;
	    }

	    ulCount--;

	    if (ulCount == 0)
	    {
		break;
	    }

	    pPacket = (CAUPacket*) m_DeinterleaveQueue.GetAtNext(lPos);
	} while (TRUE);
    }

    return FALSE;
}

void MP4GPayloadFormat::FlushArrays(void)
{
    FlushPacketArray(m_pPacketFragments, m_ulNumPacketFragments);
    FlushAUArray(m_pAUFragments, m_ulNumAUFragments);
    FlushAUArray(m_pAUPackets, m_ulNumAUPackets);
}


ULONG32 MP4GPayloadFormat::GetPacketTime(IHXPacket* pPacket)
{
    ULONG32 ulTime;

    HX_ASSERT(pPacket);

    if (m_bUsesRTPPackets)
    {
	ulTime = ((IHXRTPPacket*) pPacket)->GetRTPTime();
    }
    else
    {
	ulTime = pPacket->GetTime();
    }

    ulTime = m_TSConverter.Convert(ulTime);

    return ulTime;
}


ULONG32 MP4GPayloadFormat::SignBitField(ULONG32 ulVal, ULONG32 ulNumBits)
{
    if (ulNumBits > 0)
    {
	if (ulVal >> (ulNumBits - 1))
	{
	    ulVal = (0xFFFFFFFF & ulVal);
	}
    }
    else
    {
	ulVal = 0;
    }

    return ulVal;
}


HXBOOL MP4GPayloadFormat::TestIndexGreater(ULONG32 ulIdx1, ULONG32 ulIdx2)
{
    return (ulIdx1 > ulIdx2);
}


HXBOOL MP4GPayloadFormat::TestTimeGreater(ULONG32 ulTime1, ULONG32 ulTime2)
{
    return (((LONG32) (ulTime1 - ulTime2)) > 0);
}

LONG32 MP4GPayloadFormat::IndexDiff(ULONG32 ulIdx1, ULONG32 ulIdx2)
{
    return SignBitField(ulIdx1 - ulIdx2, m_Config.m_ulIndexLength);
}
