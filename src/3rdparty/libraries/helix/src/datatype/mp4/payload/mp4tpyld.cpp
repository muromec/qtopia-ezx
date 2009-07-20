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
#define MP4T_RN_3GPP_TEXT_PAYLOAD_MIME_TYPE "video/X-RN-3GPP-TEXT"

#define FLUSH_ALL_PACKETS   0xFFFFFFFF

#define FORMAT_PARAMETERS_PREFIX	"a=fmtp:0 "
#define FORMAT_PARAMETERS_PREFIX_SIZE	(sizeof(FORMAT_PARAMETERS_PREFIX) - 1)

#define MAX_INT_TEXT_LENGTH	10

// /XXXEH- need to revisit this.  How (if at all) is a sparse data stream
// handled in RDT:
#define _3GPPT_DEFAULT_SAMPLES_PER_SEC 10

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

#include "rule2flg.h" // /For RuleToFlagMap (ASM-rule-to-flag map)

#include "qtatmmgs.h" // /For CQT_TrackInfo_Manager
#include "qttrack.h"  // /For CQTTrack

#include "mp4desc.h"
#include "mp4tpyld.h"


MP4TPayloadFormat::MP4TPayloadFormat(CQTTrack* pTrack,
	    CQT_TrackInfo_Manager* pTrackInfoMgr,
	    CHXBufferMemoryAllocator* pAllocator)
    : m_lRefCount	(0)
    , m_pClassFactory	(NULL)
    , m_pAllocator	(pAllocator)
    , m_pStreamHeader	(NULL)
    , m_pTextHeader	(NULL)
    , m_ulTextCfgHeaderSize (0)
    , m_ulSamplesPerSecond(1000)
    , m_bFlushed	(FALSE)
    , m_bFirstPacket	(TRUE)
    , m_bUsesRTPPackets (FALSE)
    , m_bRTPPacketTested(FALSE)
    , m_bPacketize	(FALSE)
    , m_PayloadID	(PYID_X_HX_3GPP_TEXT)
    , m_pTrack          (pTrack)
    , m_pTrackInfoMgr   (pTrackInfoMgr)
    , m_pRuleToFlagMap(NULL)
    , m_ulPriorPacketContentBeginTime(0)
{
    if (m_pAllocator)
    {
	m_pAllocator->AddRef();
    }
}

MP4TPayloadFormat::~MP4TPayloadFormat()
{
    FlushPackets(FLUSH_ALL_PACKETS);

    if (m_pRuleToFlagMap)
    {
        HX_VECTOR_DELETE(m_pRuleToFlagMap->rule_to_flag_map);
        HX_DELETE(m_pRuleToFlagMap);
    }

    HX_VECTOR_DELETE(m_pTextHeader);
    if (m_pAllocator)
    {
        m_pAllocator->Release();
        m_pAllocator = NULL;
    }
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pStreamHeader);
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
MP4TPayloadFormat::QueryInterface(REFIID riid, void** ppvObj)
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
MP4TPayloadFormat::AddRef()
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
MP4TPayloadFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
MP4TPayloadFormat::Init(IUnknown* pContext,
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

    return retVal;
}

STDMETHODIMP
MP4TPayloadFormat::Reset()
{
    // Release all input packets we have stored
    FlushPackets(FLUSH_ALL_PACKETS);

    m_bFlushed = FALSE;
    m_bFirstPacket = TRUE;
    m_bUsesRTPPackets = FALSE;
    m_bRTPPacketTested = FALSE;

    m_TSConverter.Reset();

    return HXR_OK;
}

STDMETHODIMP
MP4TPayloadFormat::SetStreamHeader(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(pHeader);

    HX_RELEASE(m_pStreamHeader);
    m_pStreamHeader = pHeader;
    if (m_pStreamHeader)
    {
	m_pStreamHeader->AddRef();
    }
    // /We need to add an ASM rulebook with n rules, one for each sample
    // description.  Note: in future, we may have multiple rules that map to
    // a particular sample description, so we use a RuleToFlagMap to
    // make sure the renderer knows which maps to which:
    IHXBuffer* pASMRuleBook = NULL;
    if (!m_pClassFactory  ||  !m_pTrackInfoMgr)
    {
	retVal = HXR_UNEXPECTED;
    }
    else
    {
	// /3GPP Timed Text renderer needs width, height, and matrix (for
	// origin offset),
	HX_ASSERT(m_pTrackInfoMgr->GetTrackWidth());
	if (m_pTrackInfoMgr->GetTrackWidth())
	{
	    m_pStreamHeader->SetPropertyULONG32("3GPPTextTrackWidth",
		    m_pTrackInfoMgr->GetTrackWidth());
	}

	HX_ASSERT(m_pTrackInfoMgr->GetTrackHeight());
	if (m_pTrackInfoMgr->GetTrackHeight())
	{
	    m_pStreamHeader->SetPropertyULONG32("3GPPTextTrackHeight",
		    m_pTrackInfoMgr->GetTrackHeight());
	}

	if (m_pTrackInfoMgr->GetTrackTransformX())
	{
	    m_pStreamHeader->SetPropertyULONG32("3GPPTextTrackTransformX",
		    m_pTrackInfoMgr->GetTrackTransformX());
	}

	if (m_pTrackInfoMgr->GetTrackTransformY())
	{
	    m_pStreamHeader->SetPropertyULONG32("3GPPTextTrackTransformY",
		    m_pTrackInfoMgr->GetTrackTransformY());
	}


	retVal = m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
					       (void**) &pASMRuleBook);

	// /XXXEH- should the rule(s) contain "priority=10" as well?:
	char* pszNewRuleStr = "TimestampDelivery=TRUE;";
	UINT32 ulNewRuleStrLen = (UINT32)strlen(pszNewRuleStr);
	UINT32 ulNumEntries = m_pTrackInfoMgr->GetNumSamplesInOpaqueData();
	char* pRuleBook = new char[(ulNumEntries * ulNewRuleStrLen) + 1];
	if (pRuleBook)
	{
	    char* pTmp = pRuleBook;
	    for (UINT32 ulIi = 0; ulIi<ulNumEntries;
		    ulIi++, pTmp += ulNewRuleStrLen)
	    {
		memcpy(pTmp, pszNewRuleStr, ulNewRuleStrLen); /* Flawfinder: ignore */
	    }
	    *pTmp = '\0';
	}
	else
	{
	    retVal = HXR_OUTOFMEMORY;
	}

	// /Next, build the RuleToFlagMap so the renderer can map each
	// text sample (one per packet) to its sample description (array of
	// which is in the stream header's "OpaqueData" property):
	IHXBuffer* p3GPPTTRuleToFlagMapBuffer = NULL;
	if (SUCCEEDED(retVal)  &&  HXR_OK ==
		m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
		(void**)&p3GPPTTRuleToFlagMapBuffer))
	{
	    m_pRuleToFlagMap = new RuleToFlagMap;
	    m_pRuleToFlagMap->num_rules = (UINT16)ulNumEntries;

	    m_pRuleToFlagMap->rule_to_flag_map =
		    new UINT16[m_pRuleToFlagMap->num_rules];

	    // /Note: if more rules get added, above, this will need redoing:
	    for (UINT16 uiI = 0; uiI<m_pRuleToFlagMap->num_rules; uiI++)
	    {
		m_pRuleToFlagMap->rule_to_flag_map[uiI] = uiI;
	    }

	    p3GPPTTRuleToFlagMapBuffer->SetSize(sizeof(UINT16) +
		    (sizeof(UINT16) * m_pRuleToFlagMap->num_rules));

	    UINT32 ulSize;
	    // /Put it into network-order (Big-endian):
	    m_pRuleToFlagMap->pack(
		    p3GPPTTRuleToFlagMapBuffer->GetBuffer(), ulSize);

	    HX_ASSERT(ulSize == p3GPPTTRuleToFlagMapBuffer->GetSize());

	    m_pStreamHeader->SetPropertyBuffer(HX_3GPPTT_RULE_TO_FLAG_MAP_PROPERTY,
		    p3GPPTTRuleToFlagMapBuffer);

	    HX_RELEASE(p3GPPTTRuleToFlagMapBuffer);
	}


	if (SUCCEEDED(retVal))
	{
	    retVal = pASMRuleBook->Set((UCHAR*) pszNewRuleStr,
		    strlen(pszNewRuleStr) + 1);
	}

	if (SUCCEEDED(retVal))
	{
	    m_pStreamHeader->SetPropertyCString("ASMRuleBook", pASMRuleBook);
	}
    }
    HX_RELEASE(pASMRuleBook);


    if (m_pStreamHeader)
    {
	m_pStreamHeader->AddRef();
    }

    return retVal;
}



STDMETHODIMP
MP4TPayloadFormat::GetStreamHeader(REF(IHXValues*) pHeader)
{
    HX_RESULT retVal = HXR_FAIL;

    HX_ASSERT(m_pStreamHeader);
    if (m_pStreamHeader)
    {
	retVal = HXR_OK;
	pHeader = m_pStreamHeader;
	pHeader->AddRef();
    }

    return retVal;
}


STDMETHODIMP
MP4TPayloadFormat::SetPacket(IHXPacket* pOrigPacket)
{
    HX_RESULT retVal;

    HX_ASSERT(pOrigPacket);

    IHXBuffer* pBuffer = pOrigPacket->GetBuffer();
    if (!pBuffer  ||  !m_pTrackInfoMgr  ||  !m_pTrack)
    {
	HX_ASSERT(pBuffer);
	HX_ASSERT(m_pTrackInfoMgr);
	HX_ASSERT(m_pTrack);
	retVal = HXR_UNEXPECTED;
    }
    else
    {
	if (!m_bRTPPacketTested)
	{
	    IHXRTPPacket* pRTPPacket = NULL;

	    m_bUsesRTPPackets = (pOrigPacket->QueryInterface(
				    IID_IHXRTPPacket,
				    (void**) &pRTPPacket)
				== HXR_OK);

	    m_bRTPPacketTested = TRUE;

	    HX_RELEASE(pRTPPacket);

	    if (!m_bUsesRTPPackets)
            {
		m_ulSamplesPerSecond = _3GPPT_DEFAULT_SAMPLES_PER_SEC; // RDT time stamp
            }

	    HX_ASSERT(m_ulSamplesPerSecond != 0);

	    m_TSConverter.SetBase(m_ulSamplesPerSecond,
				  m_ulSamplesPerSecond);
	}

	// /Now, set the ASM rule # to map to the sample desc index:

	// / For 3GPP Text tracks, we'll use the ASM rule number to
	// map to the sample description index for the renderer's use:
	UINT32 ulSamplDescIndx = m_pTrack->GetLastSampleDescIdx();
	// Search through m_pRuleToFlagMap to find one whose flag
	// matches this sampleDescr index:
	// /First, see if it's a 1-1 mapping to save time in most cases:
	UINT16 uiASMRuleNum = 0;
	if (m_pRuleToFlagMap->rule_to_flag_map[ulSamplDescIndx] ==
		ulSamplDescIndx)
	{
	    uiASMRuleNum = (UINT16)ulSamplDescIndx;
        }
	else
	{
	    for (UINT16 uii=0; uii < m_pRuleToFlagMap->num_rules; uii++)
            {
		if (m_pRuleToFlagMap->rule_to_flag_map[uii] == ulSamplDescIndx)
                {
		    uiASMRuleNum = uii;
		    break; // /Found it
                }
            }
        }

#if defined(HELIX_FEATURE_3GPPTT_STREAMING)
	UINT32 ulTimeStamp = m_ulPriorPacketContentBeginTime;
#else
        // /Just make every packet a time-0 packet.  This will have to be
        // revisited as soon as we handle post-Rel5 version of 3GPP-TT.
        // Rel5 specifies that all packets should be delivered up front:
        // (Helps fix Helix Issue #667: allows for seeking without
        // packet-offset problems, and Negates Helix issue #675: allows
        // for "streaming" without packet-offset problems) :
        UINT32 ulTimeStamp = 0; 
#endif // /XXXEH_TESTING_FIXING_SEEKING_AND_STREAMING
        UINT32 ulRTPTime = pOrigPacket->GetTime();
	UINT16 uiStream = pOrigPacket->GetStreamNumber();
	UINT16 uiASMFlags = pOrigPacket->GetASMRuleNumber();


        if (!m_bUsesRTPPackets)
        {
            retVal = pOrigPacket->Set(pBuffer, ulTimeStamp, uiStream,
                    uiASMFlags, uiASMRuleNum);
        }
        else
        {
            // /Set the time stamp to the prior packet's time so it gets
            // to the renderer in time to set the prior packet's end time,
            // then set the RTP time to the begin time of the contents:
            retVal = ((IHXRTPPacket*) pOrigPacket) ->SetRTP(
                    pBuffer,
                    ulTimeStamp,
                    ulRTPTime,
                    uiStream,
                    uiASMFlags,
                    uiASMRuleNum);
        }

        m_ulPriorPacketContentBeginTime = ulRTPTime; // /Save for next pkt

	// Add this packet to our list of input packets
	pOrigPacket->AddRef();
	m_InputPackets.AddTail(pOrigPacket);
    }

    return retVal;
}

STDMETHODIMP
MP4TPayloadFormat::GetPacket(REF(IHXPacket*) pOutPacket)
{
    HX_RESULT retVal = HXR_OK;

    retVal = GetPacketizerPacket(pOutPacket);

    return retVal;
}

HX_RESULT
MP4TPayloadFormat::GetPacketizerPacket(IHXPacket* &pOutPacket)
{
    HX_RESULT retVal = HXR_INCOMPLETE;

    if (!m_InputPackets.IsEmpty())
    {
	pOutPacket = (IHXPacket*) m_InputPackets.RemoveHead();
	retVal = HXR_OK;
    }
    else if (m_bFlushed)
    {
	retVal = HXR_STREAM_DONE;
    }

    return retVal;
}

void
MP4TPayloadFormat::FlushPackets(ULONG32 ulCount)
{
    IHXPacket* pDeadPacket;

    while ((ulCount > 0) && (!m_InputPackets.IsEmpty()))
    {
	pDeadPacket = (IHXPacket*) m_InputPackets.RemoveHead();
	HX_RELEASE(pDeadPacket);
	if (ulCount != FLUSH_ALL_PACKETS)
	{
	    ulCount--;
	}
    }
}


STDMETHODIMP
MP4TPayloadFormat::Flush()
{
    m_bFlushed = TRUE;

    return HXR_OK;
}


ULONG32
MP4TPayloadFormat::GetPacketTime(IHXPacket* pPacket)
{
    ULONG32 ulTime;

    HX_ASSERT(pPacket);

    if (m_bUsesRTPPackets && (m_ulSamplesPerSecond != 0))
    {
	ulTime = ((IHXRTPPacket*) pPacket)->GetRTPTime();

	ulTime = m_TSConverter.Convert(ulTime);
    }
    else
    {
	ulTime = pPacket->GetTime();
    }

    return ulTime;
}

