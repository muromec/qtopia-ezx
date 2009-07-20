/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: transport.cpp,v 1.5 2007/03/30 19:08:39 tknox Exp $
 * 
 * Portions Copyright (c) 1995-2007 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#include "hxtypes.h"
#include "hxassert.h"
#include "debug.h"
#include "hxcom.h"
#include "hxmarsh.h"
#include "hxstrutl.h"
#include "netbyte.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxcomm.h"
#include "hxmon.h"
#include "netbyte.h"
#include "hxstring.h"
#include "chxpckts.h"
#include "hxslist.h"
#include "hxmap.h"
#include "hxdeque.h"
#include "hxbitset.h"
#include "timebuff.h"
#include "timeval.h"
#include "tconverter.h"
#include "rtptypes.h"
#include "hxcorgui.h"

#include "ntptime.h"

#include "rtspif.h"
#include "transport.h"
#include "basepkt.h"
#include "hxtbuf.h"
#include "transbuf.h"
#include "hxtick.h"
#include "hxprefs.h"	// IHXPreferences
#include "hxmime.h"
#include "hxcore.h"

#include "servpckts.h"
#include "hxservinfo.h"

#include "hxheap.h"
#ifdef PAULM_IHXTCPSCAR
#include "objdbg.h"
#endif

#ifdef PAULM_TNGTCPTRANSTIMING
#include "classtimer.h"
ClassTimer g_TNGTCPTransTimer("TNGTCPTransport", 0, 3600);
#endif

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define STREAM_END_DELAY_BASE_TOLERANCE	3000

static const UINT32 TRANSPORT_BUF_GROWTH_RATE  = 1000;

TransportStreamHandler::TransportStreamHandler(IHXTransport* pOwner)
    : m_lRefCount(0)
    , m_pOwner(pOwner)
    , m_unMaxStreamGroup(0)
    , m_pStreamGroupToStreamNumMap(0)
{
    m_pStreamDataMap = new CHXMapLongToObj;
}

TransportStreamHandler::~TransportStreamHandler()
{
    CHXMapLongToObj::Iterator i;
    RTSPStreamData* pStreamData;

    for(i=m_pStreamDataMap->Begin();i!=m_pStreamDataMap->End();++i)
    {
	pStreamData = (RTSPStreamData*)(*i);
	delete pStreamData;
    }
    delete m_pStreamDataMap;

    HX_VECTOR_DELETE(m_pStreamGroupToStreamNumMap);
}


HX_RESULT
TransportStreamHandler::initStreamData(
    UINT16 streamNumber, UINT16 streamGroupNumber,
    HXBOOL needReliable, HXBOOL bIsSource, INT16 rtpPayloadType,
    HXBOOL bRecordFlag, UINT32 wrapSequenceNumber, UINT32 ulBufferDepth,
    HXBOOL bHasOutOfOrderTS, CHXTimestampConverter* pTSConverter,
    RTSPMediaType eMediaType)
{
    RTSPStreamData* pStreamData = 0;
    HX_RESULT hr = HXR_OK;

    hr = initStreamData( streamNumber, needReliable, bIsSource, rtpPayloadType,
			bRecordFlag, wrapSequenceNumber, ulBufferDepth, bHasOutOfOrderTS,
			pTSConverter, eMediaType);
    if (SUCCEEDED(hr))
    {
	if(m_pStreamDataMap->Lookup(streamNumber, (void*&)pStreamData))
	{
	    pStreamData->m_streamGroupNumber = streamGroupNumber;

	    //* m_pStreamGroupToStreamNumMap is currently recreated
	    //*  for every stream initialized. This should 
	    //*  eventually be replaced by adding mechanism for 
	    //*  passing StreamGroupCount to TransportStreamHandler
	    //*  constructor
	    //*
	    //* Recreate map only if new Max 
	    if (!m_pStreamGroupToStreamNumMap || (streamGroupNumber > m_unMaxStreamGroup))
	    { 
		UINT16* pNewMap = 0;
		UINT16  unOldMax = m_unMaxStreamGroup;

		//* reset MaxStreamGroup
		m_unMaxStreamGroup =  streamGroupNumber;
		pNewMap = new UINT16[m_unMaxStreamGroup + 1];
		
		if (m_pStreamGroupToStreamNumMap)
		{
		    memcpy(pNewMap, m_pStreamGroupToStreamNumMap, (unOldMax + 1) * sizeof(UINT16));
		    HX_VECTOR_DELETE(m_pStreamGroupToStreamNumMap);
		}
		
		m_pStreamGroupToStreamNumMap = pNewMap;
	    }

	    //* initialize map entry
	    m_pStreamGroupToStreamNumMap[streamGroupNumber] = streamNumber;
	}
	else
	{
	    hr = HXR_FAIL;
	}
    }

    return hr;
}

HX_RESULT
TransportStreamHandler::initStreamData(
    UINT16 streamNumber, HXBOOL needReliable, HXBOOL bIsSource, INT16 rtpPayloadType,
    HXBOOL bRecordFlag, UINT32 wrapSequenceNumber, UINT32 ulBufferDepth,
    HXBOOL bHasOutOfOrderTS, CHXTimestampConverter* pTSConverter,
    RTSPMediaType eMediaType)
{
    RTSPStreamData* pStreamData;

    if(!m_pStreamDataMap->Lookup(streamNumber, (void*&)pStreamData))
    {
	pStreamData = new RTSPStreamData(needReliable);
        if( !pStreamData )
        {
            return HXR_OUTOFMEMORY;
        }
	pStreamData->m_streamNumber = streamNumber;
        pStreamData->m_pTSConverter = pTSConverter;
	pStreamData->m_eMediaType = eMediaType;
	pStreamData->m_lastSeqNo = 0;
	(*m_pStreamDataMap)[streamNumber] = pStreamData;

	return HXR_OK;
    }

    return HXR_FAIL;
}

RTSPStreamData*
TransportStreamHandler::getStreamData(UINT16 streamNumber)
{
    RTSPStreamData* pStreamData = 0;

    if(!m_pStreamDataMap->Lookup(streamNumber, (void*&)pStreamData))
    {
	return 0;
    }

    return pStreamData;
}

RTSPStreamData*
TransportStreamHandler::getStreamDataForStreamGroup(UINT16 streamGroupNumber)
{
    HX_ASSERT(streamGroupNumber < m_unMaxStreamGroup);

    return getStreamData( m_pStreamGroupToStreamNumMap[streamGroupNumber]);
}

RTSPStreamData*
TransportStreamHandler::firstStreamData()
{
    streamIterator = m_pStreamDataMap->Begin();
    if(streamIterator == m_pStreamDataMap->End())
    {
	return 0;
    }
    return (RTSPStreamData*)(*streamIterator);
}

RTSPStreamData*
TransportStreamHandler::nextStreamData()
{
    ++streamIterator;
    if(streamIterator == m_pStreamDataMap->End())
    {
	return 0;
    }
    return (RTSPStreamData*)(*streamIterator);
}

HXBOOL
TransportStreamHandler::endStreamData()
{
    return (streamIterator == m_pStreamDataMap->End());
}

HX_RESULT
TransportStreamHandler::createResendBuffer(UINT16 streamNumber,
				      UINT32 wrapSequenceNumber)
{
    RTSPStreamData* pStreamData;

    if(!m_pStreamDataMap->Lookup(streamNumber, (void*&)pStreamData))
    {
	return HXR_FAILED;
    }

    RTSPResendBuffer* pResendBuffer;

    pResendBuffer = new RTSPResendBuffer(RESEND_BUF_DURATION,
                                         MAX_RESEND_BUF_DURATION,
                                         RESEND_BUF_GROWTH_RATE,
                                         wrapSequenceNumber);
    if( !pResendBuffer )
    {
        return HXR_OUTOFMEMORY;
    }

    pStreamData->m_pResendBuffer = pResendBuffer;
    return HXR_OK;
}

RTSPResendBuffer*
TransportStreamHandler::getResendBuffer(UINT16 streamNumber)
{
    RTSPStreamData* pStreamData = 0;

    if(!m_pStreamDataMap->Lookup(streamNumber, (void*&)pStreamData))
    {
	return 0;
    }
    return pStreamData->m_pResendBuffer;
}

/*
 * Transport methods
 */

Transport::Transport()
    : m_bHackedRecordFlag(FALSE)
    , m_pSessionStats(NULL)
    , m_pPlayerState(NULL)
    , m_pContext(NULL)
    , m_pCommonClassFactory(0)
    , m_pScheduler(0)
    , m_pResp(0)
    , m_pRegistry(0)
    , m_pInternalReset(0)
    , m_pSrcBufferStats(0)
    , m_pSourceLatencyStats(0)
    , m_pStreamHandler(0)
    , m_pPPS(NULL)
    , m_pBytesServed(NULL)
    , m_bIsSource(TRUE)
    , m_ulRegistryID(0)
    , m_ulPacketsSent(0)
    , m_lBytesSent(0)
    , m_ulPlayRangeFrom(RTSP_PLAY_RANGE_BLANK)
    , m_ulPlayRangeTo(RTSP_PLAY_RANGE_BLANK)
    , m_bIsInitialized(FALSE)
    , m_bIsUpdated(FALSE)
    , m_bPrefetch(FALSE)
    , m_bFastStart(FALSE)
    , m_bIsReceivedData(FALSE)
    , m_bSourceDone(FALSE)
    , m_wrapSequenceNumber(0)
    , m_bMulticast(FALSE)
    , m_pPacketFilter(NULL)
    , m_pClientPacketList(NULL)
    , m_pNoTransBufPktList(NULL)
    , m_bPlayRequestSent(FALSE)
    , m_ulTotalSuccessfulResends(0)
    , m_ulTotalFailedResends(0)
    , m_ulSendingTime(0)
    , m_drop_packets(FALSE)
    , m_packets_since_last_drop(0)
    , m_bSkipTimeAdjustment(FALSE)
    , m_nProbingPacketsRequested(0)
    , m_nProbingPacketsReceived(0)
    , m_pABDProbPktInfo(NULL)
#ifdef RDT_MESSAGE_DEBUG
    , m_bRDTMessageDebug(FALSE)
#endif 
    , m_bIsAggregate(FALSE)
    , m_ulRefCount(0UL)
{
    m_ulStartTime = HX_GET_TICKCOUNT();
}

Transport::~Transport()
{
    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pResp);
    HX_RELEASE(m_pRegistry);
    if (m_pStreamHandler)
    {
        m_pStreamHandler->Release();
        m_pStreamHandler = NULL;
    }
    HX_RELEASE(m_pInternalReset);
    HX_RELEASE(m_pSrcBufferStats);
    HX_RELEASE(m_pSourceLatencyStats);
    HX_RELEASE(m_pPlayerState);
    HX_RELEASE(m_pPacketFilter);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pSessionStats);

    destroyABDPktInfo();

    for (CHXSimpleList::Iterator i = m_StreamNumbers.Begin(); i != m_StreamNumbers.End(); ++i)
    {
        delete (*i);
    }
}

STDMETHODIMP
Transport::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXTransport))
    {
        AddRef();
        *ppvObj = static_cast<IHXTransport*>(this);
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerPacketSink))
    {
        AddRef();
        *ppvObj = static_cast<IHXServerPacketSink*>(this);
        return HXR_OK;
    }


    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
Transport::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
Transport::Release()
{
    if(InterlockedDecrement(&m_ulRefCount) > 0UL)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

void
Transport::addStreamInfo(RTSPStreamInfo* pStreamInfo, UINT32 ulBufferDepth)
{
    if(pStreamInfo)
    {
	if(!m_pStreamHandler)
	{
            // The TransportStreamHandler only needs an Transport* to create a Servertransport buffer,
            // which is a client side only usage. Das ist alles! So we can safely pass a NULL
            // pointer here.
	    m_pStreamHandler = new TransportStreamHandler(static_cast<Transport*>(0));
            if( !m_pStreamHandler )
            {
                return;
            }
	    m_pStreamHandler->AddRef();
	}

	CHXTimestampConverter*  pTSConverter = NULL;

	if (pStreamInfo->m_HXFactor && pStreamInfo->m_RTPFactor)
	{
	    pTSConverter = new CHXTimestampConverter(CHXTimestampConverter::FACTORS,
						     pStreamInfo->m_HXFactor,
						     pStreamInfo->m_RTPFactor);
            if( !pTSConverter )
            {
                m_pStreamHandler->Release();
                return;
            }
	}
	else if (pStreamInfo->m_sampleRate)
	{
	     
	    pTSConverter =  new CHXTimestampConverter(CHXTimestampConverter::SAMPLES,
						       pStreamInfo->m_sampleRate);
            if( !pTSConverter )
            {
                m_pStreamHandler->Release();
                return;
            }
	 }	

	 m_pStreamHandler->initStreamData(
	     pStreamInfo->m_streamNumber,
	     pStreamInfo->m_uStreamGroupNumber,
	     pStreamInfo->m_bNeedReliablePackets,
	     m_bIsSource,
	     pStreamInfo->m_rtpPayloadType,
	     m_bHackedRecordFlag,
	     m_wrapSequenceNumber,
	     ulBufferDepth,
	     pStreamInfo->m_bHasOutOfOrderTS,
	     pTSConverter,
	     pStreamInfo->m_eMediaType);

        m_bSkipTimeAdjustment = pStreamInfo->m_bRealMedia;

        // Get the stream data for this stream
	RTSPStreamData* pStreamData = NULL;
	pStreamData = m_pStreamHandler->getStreamData(pStreamInfo->m_streamNumber);
    }
}

void
Transport::setFirstTimeStamp(UINT16 uStreamNumber, UINT32 ulTS, 
                                 HXBOOL bIsRaw)
{
    RTSPStreamData* pStreamData = 
	m_pStreamHandler->getStreamData(uStreamNumber);

    if (pStreamData)
    {
	if (pStreamData->m_pTSConverter)
	{
	    pStreamData->m_pTSConverter->setHXAnchor(ulTS);
	}
	    	    
	HX_DELETE(pStreamData->m_pTSOrderHack);
    }
}

void 
Transport::setPlayRange(UINT32 ulFrom, UINT32 ulTo)
{
    // this is the Range values in PLAY request in RMA time (ms) called on PLAY 
    // request
    m_ulPlayRangeFrom = ulFrom; 
    m_ulPlayRangeTo = ulTo;
}

void
Transport::setSessionID(const char* pSessionID)
{
    m_sessionID = pSessionID;
}

UINT16
Transport::getSeqNum(UINT16 streamNumber)
{
    RTSPStreamData* pStreamData;

    pStreamData = m_pStreamHandler->getStreamData(streamNumber);

    if(pStreamData)
    {
	return pStreamData->m_seqNo;
    }
    else
    {
	return 0;	//XXXBAB - 0xffff?
    }
}

UINT16
Transport::getFirstSeqNum(UINT16 streamNumber)
{
    RTSPStreamData* pStreamData;

    pStreamData = m_pStreamHandler->getStreamData(streamNumber);

    if(pStreamData)
    {
	return pStreamData->m_firstSeqNo;
    }
    else
    {
	return 0;	//XXXBAB - 0xffff?
    }
}

UINT32
Transport::getTimestamp(UINT16 streamNumber)
{
    RTSPStreamData* pStreamData;

    pStreamData = m_pStreamHandler->getStreamData(streamNumber);

    if(pStreamData)
    {
	// XXXGo - this is RTP time w/ offset (NOT RMA time) if RTPUDPTransprot...
	return pStreamData->m_lastTimestamp;
    }
    else
    {
	return 0;
    }
}

HX_RESULT
Transport::getPacket(UINT16 uStreamNumber, IHXPacket*& pPacket)
{
    UINT32 uSeqNum;
    return getPacket(uStreamNumber, pPacket, uSeqNum);
}

HX_RESULT
Transport::getPacket(UINT16 uStreamNumber, IHXPacket*& pPacket,
                         REF(UINT32) uSeqNum)
{
    return HXR_NOTIMPL;
}

UINT32
Transport::getFirstTimestamp(UINT16 streamNumber)
{
    RTSPStreamData* pStreamData;

    pStreamData = m_pStreamHandler->getStreamData(streamNumber);

    if(pStreamData)
    {
	// XXXGo - this is RTP time w/ offset (NOT RMA time) if RTPUDPTransprot...
	return pStreamData->m_firstTimestamp;
    }
    else
    {
	return 0;
    }
}

HX_RESULT
Transport::startPackets(UINT16 uStreamNumber)
{
    return HXR_FAIL;
}

HX_RESULT
Transport::stopPackets(UINT16 uStreamNumber)
{
    return HXR_FAIL;
}

void Transport::GetContext(IUnknown*& pContext)   
{   
	pContext = m_pContext;   

	if (pContext)   
	{   
		pContext->AddRef();   
	}   
} 

HXBOOL 
Transport::isSparseStream(UINT16 uStreamNumber)
{
    HXBOOL		bResult = FALSE;
    const char*		pMimeType = NULL;
    IUnknown*		pUnknown = NULL;
    IHXStreamSource*	pStreamSource = NULL;    
    IHXStream*		pStream = NULL;

    if (m_pContext  &&
	HXR_OK == m_pContext->QueryInterface(IID_IHXStreamSource, (void**)&pStreamSource))
    {
	if (HXR_OK == pStreamSource->GetStream(uStreamNumber, pUnknown))
	{
	    if (HXR_OK == pUnknown->QueryInterface(IID_IHXStream, (void**)&pStream))
	    {
		pMimeType = pStream->GetStreamType();

		// special handling for sparsed streams for multicast
		if (pMimeType							&&
		    (strcasecmp(SYNCMM_MIME_TYPE, pMimeType) == 0		||
		     strcasecmp(REALEVENT_MIME_TYPE, pMimeType) == 0		||
		     strcasecmp("application/vnd.rn-realtext", pMimeType) == 0	||
		     strcasecmp("application/x-pn-realtext", pMimeType) == 0))
		{
		    bResult = TRUE;
		}
	    }
	    HX_RELEASE(pStream);
	}
	HX_RELEASE(pUnknown);
    }
    HX_RELEASE(pStreamSource);

    return bResult;
}

///brief Provide a default handler for classes that don't provide their own
STDMETHODIMP
Transport::PacketReady(ServerPacket* pPacket)
{
    UpdatePacketStats(pPacket);
    return sendPacket(static_cast<BasePacket*>(pPacket));
}

void
Transport::UpdatePacketStats(BasePacket* pPacket)
{
    HXAtomicAddUINT32( m_pPPS, pPacket->GetSize() );
    HXAtomicIncUINT32( m_pBytesServed );
}

HX_RESULT			
Transport::packetReady(HX_RESULT status, RTSPStreamData* pStreamData, IHXPacket* pPacket)
{
    return HXR_OK;
}

HX_RESULT
Transport::packetReady(HX_RESULT status, UINT16 uStreamNumber, IHXPacket* pPacket)
{
    return HXR_OK;
}

HX_RESULT
Transport::getStatus(UINT16&     /* uStatusCode */, 
                     IHXBuffer*& /* pStatusDesc */, 
                     UINT16&     /* ulPercentDone */)
{
    return HXR_NOTIMPL;
}

HXBOOL
Transport::IsSourceDone(void)
{
    return m_bSourceDone;
}

void      
Transport::CheckForSourceDone(UINT16 uStreamNumber)
{
    if (m_bSourceDone)
    {
        return;
    }

    HX_ASSERT(m_pStreamHandler);

    if (!m_pStreamHandler)
    {
        return;
    }

    RTSPStreamData* pStreamData = m_pStreamHandler->getStreamData(uStreamNumber);
    HX_ASSERT(pStreamData);

    if (pStreamData)
    {
        pStreamData->m_bReceivedAllPackets = TRUE;

        m_bSourceDone = TRUE;
        pStreamData = m_pStreamHandler->firstStreamData();      
        while(pStreamData)
        {
            if (!pStreamData->m_bReceivedAllPackets)
            {
                m_bSourceDone = FALSE;
                return;
            }
            pStreamData = m_pStreamHandler->nextStreamData();
        }
    }
}    

void      
Transport::HandleBufferError()
{
    if (m_pResp)
    {
	m_pResp->OnProtocolError(HXR_BUFFERING);
    }
}

RTSPResendBuffer*
Transport::getResendBuffer(UINT16 uStreamNumber)
{
    return m_pStreamHandler->getResendBuffer(uStreamNumber);
}

HX_RESULT
Transport::initializeStatistics(UINT32 ulRegistryID)
{
    m_bIsInitialized = TRUE;
    m_ulRegistryID = ulRegistryID;

    return HXR_OK;
}

HX_RESULT
Transport::SetStatistics(UINT16 uStreamNumber, STREAM_STATS* pStats)
{
    HX_RESULT	    rc = HXR_OK;
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    RTSPStreamData* pStreamData = NULL;

    if (!m_pStreamHandler)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    pStreamData = m_pStreamHandler->getStreamData(uStreamNumber);
    if (!pStreamData)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    pStreamData->m_pStreamStats = pStats;

cleanup:
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    return rc;
}

HX_RESULT 
Transport::updateStatistics(HXBOOL bUseRegistry)
{
    m_bIsUpdated = TRUE;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    if (!m_pStreamHandler)
    {
        HX_ASSERT(m_pStreamHandler);
        return HXR_FAIL;
    }

    RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

    if (!pStreamData)
    {
        return HXR_FAIL;
    }

    /*
     * XXXGH...streams are managed by the PPM and should not show up
     *         under the Servertransport
     */
    while (pStreamData)
    {
        UINT32 ulResendSuccess = 0;
        UINT32 ulResendFailure = 0;

        if (pStreamData->m_pResendBuffer)
        {
            pStreamData->m_pResendBuffer->UpdateStatistics(ulResendSuccess,
                    ulResendFailure);
        }

        m_ulTotalSuccessfulResends += ulResendSuccess;
        m_ulTotalFailedResends += ulResendFailure;

        pStreamData = m_pStreamHandler->nextStreamData();
    }

    m_ulSendingTime = CALCULATE_ELAPSED_TICKS(m_ulStartTime, HX_GET_TICKCOUNT()) / 1000;

    IHXBuffer* pName;

    if (bUseRegistry
            &&  m_pRegistry 
            &&  HXR_OK == m_pRegistry->GetPropName(m_ulRegistryID, pName))
    {
        char str[512]; /* Flawfinder: ignore */
        const char* name = (const char*)pName->GetBuffer();
        char pTemp[32]; /* Flawfinder: ignore */

        i64toa(m_lBytesSent, pTemp, 10);

        sprintf(str, "%-.400s.PacketsSent", name); /* Flawfinder: ignore */
        m_pRegistry->AddInt(str, m_ulPacketsSent);
        sprintf(str, "%-.400s.BytesSent", name); /* Flawfinder: ignore */
        m_pRegistry->AddInt(str, INT64_TO_INT32(m_lBytesSent));

        // Add the total bytes sent as a string because we don't want
        // to have to truncate the INT64 value that is storing it
        sprintf(str, "%-.400s.TotalBytesSent", name); /* Flawfinder: ignore */
        IHXBuffer* pBuffer = NULL;
        if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (UCHAR*)pTemp, 
                    strlen(pTemp) + 1, m_pContext))
        {
            m_pRegistry->AddStr(str, pBuffer);
            HX_RELEASE(pBuffer);
        }

        sprintf(str, "%-.400s.SendingTime", name); /* Flawfinder: ignore */
        m_pRegistry->AddInt(str, m_ulSendingTime); 
        sprintf(str, "%-.400s.ResendSuccess", name); /* Flawfinder: ignore */
        m_pRegistry->AddInt(str, m_ulTotalSuccessfulResends);
        sprintf(str, "%-.400s.ResendFailure", name); /* Flawfinder: ignore */
        m_pRegistry->AddInt(str, m_ulTotalFailedResends);
        pName->Release();
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    return HXR_OK;
}

HX_RESULT 
Transport::UpdateRegistry(UINT32 ulStreamNumber,
			      UINT32 ulRegistryID)
{
    m_ulRegistryID = ulRegistryID;

    return HXR_OK;
}

HX_RESULT
Transport::playReset()
{
    m_bSourceDone = FALSE;

    if(m_pStreamHandler)
    {
	RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

	while(pStreamData)
        {
            pStreamData->m_packetSent = FALSE;
	    pStreamData = m_pStreamHandler->nextStreamData();
	}
    }
    return HXR_OK;
}

HX_RESULT
Transport::setFirstSeqNum(UINT16 uStreamNumber, UINT16 uSeqNum)
{
    RTSPStreamData* pStreamData;

    pStreamData = m_pStreamHandler->getStreamData(uStreamNumber);

    if(pStreamData)
    {
        pStreamData->m_firstSeqNo = pStreamData->m_seqNo = uSeqNum;

        if(pStreamData->m_pResendBuffer)
        {
            pStreamData->m_pResendBuffer->SetFirstSequenceNumber(uSeqNum);
        }
    }

    return HXR_OK;
}

HX_RESULT
Transport::Init(IUnknown* pContext)
{
    HX_RESULT			hresult;
    
    if (!m_pContext)
    {
	m_pContext = pContext;
	m_pContext->AddRef();
    }

    hresult = pContext->QueryInterface(IID_IHXCommonClassFactory,
				       (void**)&m_pCommonClassFactory);
    if (HXR_OK != hresult)
    {
	return hresult;
    }

    IHXServerInfo* pServerInfo;
    hresult = pContext->QueryInterface(IID_IHXServerInfo, (void**)&pServerInfo);
    if (SUCCEEDED(hresult) && pServerInfo)
    {
        m_pPPS         = pServerInfo->GetServerGlobalPointer (SG_PPS);
        m_pBytesServed = pServerInfo->GetServerGlobalPointer (SG_BYTES_SERVED);
        HX_RELEASE( pServerInfo );
        if (!m_pBytesServed || !m_pPPS)
        {
            return HXR_FAIL;
        }
    }
    else
    {
        return hresult;
    }

    IHXThreadSafeScheduler* pIScheduler;
    if (SUCCEEDED(pContext->QueryInterface(IID_IHXThreadSafeScheduler,
                                       (void**)&pIScheduler)))
    {
        hresult = pIScheduler->QueryInterface(IID_IHXScheduler,
                                           (void**)&m_pScheduler);
        HX_RELEASE(pIScheduler);
    }
    else
    {
        hresult = pContext->QueryInterface(IID_IHXScheduler,
                                           (void**)&m_pScheduler);
    }
    if (HXR_OK != hresult)
    {
	return hresult;
    }

    pContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry);

    pContext->QueryInterface(IID_IHXInternalReset,
	                               (void**)&m_pInternalReset);

    pContext->QueryInterface(IID_IHXPlayerState,
	                               (void**)&m_pPlayerState);

    pContext->QueryInterface(IID_IHXSourceBufferingStats2,
			     (void**)&m_pSrcBufferStats);

    pContext->QueryInterface(IID_IHXSourceLatencyStats,
                             (void**)&m_pSourceLatencyStats);

#ifdef RDT_MESSAGE_DEBUG
    IHXPreferences* pPreferences = NULL;
    
    if (pContext &&
	(HXR_OK == pContext->QueryInterface(IID_IHXPreferences,
                                            (void**) &pPreferences)))
    {
	IHXBuffer* pBuffer = NULL;

        ReadPrefBOOL(pPreferences, "RDTMessageDebug", m_bRDTMessageDebug);
	if (m_bRDTMessageDebug)
	{
	    if (HXR_OK == pPreferences->ReadPref("RDTMessageDebugFile", pBuffer))
	    {
		if (pBuffer->GetSize() <= 0)
		{
		    // no file name, no log
		    m_bRDTMessageDebug = FALSE;
		}
		else
		{
		    m_RDTmessageDebugFileName = (const char*) pBuffer->GetBuffer();
		}			
	    }
            HX_RELEASE(pBuffer);
	}
    }

    HX_RELEASE(pPreferences);
#endif	// RDT_MESSAGE_DEBUG
    
    return HXR_OK;
}

HX_RESULT
Transport::SetResendBufferDepth(UINT32 uMilliseconds)
{
    RTSPStreamData* pStreamData = m_pStreamHandler->firstStreamData();

    ASSERT(pStreamData);

    while (pStreamData)
    {
        if (pStreamData->m_pResendBuffer)
        {
            pStreamData->m_pResendBuffer->SetBufferDepth(uMilliseconds);
        }

        pStreamData = m_pStreamHandler->nextStreamData();
    }

    return HXR_OK;
}

#ifdef RDT_MESSAGE_DEBUG
void Transport::RDTmessageFormatDebugFileOut(const char* fmt, ...)
{
    if(m_bRDTMessageDebug)
    {
        char buf[4096]; /* Flawfinder: ignore */
	va_list args;
	va_start(args, fmt);
	FILE* fp = fopen((const char*)m_RDTmessageDebugFileName, "a");
	if (fp)
	{
	    vsprintf(buf, fmt, args);
	    fprintf(fp, "%s\n", buf);
	    fclose(fp);
	}

	va_end(args);
    }
}
#endif	// RDT_MESSAGE_DEBUG

HX_RESULT 
Transport::SetProbingPacketsRequested(UINT8 nPacketsRequested)
{
    HX_RESULT res = HXR_FAILED;

    destroyABDPktInfo();

    m_pABDProbPktInfo = new ABD_PROBPKT_INFO*[nPacketsRequested];

    if (m_pABDProbPktInfo)
    {
        memset(m_pABDProbPktInfo, 0, 
               sizeof(ABD_PROBPKT_INFO*) * nPacketsRequested);

        m_nProbingPacketsRequested = nPacketsRequested;
        m_nProbingPacketsReceived = 0;

        res = HXR_OK;
    }
    else
    {
        res = HXR_OUTOFMEMORY;
    }

    return res;
}

void Transport::destroyABDPktInfo()
{
    if (m_pABDProbPktInfo)
    {
        for (int i = 0; i < m_nProbingPacketsRequested; i++)
        {
            HX_DELETE(m_pABDProbPktInfo[i]);
        }
        HX_VECTOR_DELETE(m_pABDProbPktInfo);
    }

    m_nProbingPacketsRequested = 0;
    m_nProbingPacketsReceived = 0;
}

HX_RESULT 
Transport::handleABDPktInfo(TransportMode mode, UINT32 uSeq, 
                                UINT32 uSendTime, UINT32 uRecvTime, 
                                UINT32 uPktSize)
{
    HX_RESULT res = HXR_UNEXPECTED;

    if (m_pABDProbPktInfo && 
        (m_nProbingPacketsReceived < m_nProbingPacketsRequested))
    {
        m_pABDProbPktInfo[m_nProbingPacketsReceived] = new ABD_PROBPKT_INFO;
        m_pABDProbPktInfo[m_nProbingPacketsReceived]->seq = (UINT8)uSeq;
        m_pABDProbPktInfo[m_nProbingPacketsReceived]->sendTime = uSendTime;
        m_pABDProbPktInfo[m_nProbingPacketsReceived]->recvTime = uRecvTime;
        m_pABDProbPktInfo[m_nProbingPacketsReceived]->dwSize = uPktSize;

        m_nProbingPacketsReceived++;

        // XXX HP still need to handle loss case
        if (m_nProbingPacketsReceived == m_nProbingPacketsRequested)
        {
            double dwResult = CalculateABD(1, mode, 
                                           &m_pABDProbPktInfo[0], 
                                           m_nProbingPacketsReceived);
            
            destroyABDPktInfo();

            IHXAutoBWDetectionAdviseSink* pSink = NULL;
            if (HXR_OK == m_pResp->QueryInterface(IID_IHXAutoBWDetectionAdviseSink, (void**)&pSink))
            {
                pSink->AutoBWDetectionDone(HXR_OK, (UINT32)dwResult);
            }
            else
            {
                // the owner has to support IHXAutoBWDetectionAdviseSink when
                // it initiates ABD
                HX_ASSERT(FALSE);
            }
            HX_RELEASE(pSink);
        }
    }

    return res;
}

HX_RESULT
Transport::pauseBuffers()
{
    return HXR_NOTIMPL;
}

HX_RESULT
Transport::resumeBuffers()
{
    return HXR_NOTIMPL;
}

// IHXTransport
STDMETHODIMP
Transport::InitializeStatistics (/*IN*/ UINT32 ulRegistryID)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
Transport::UpdateStatistics ()
{
    return HXR_NOTIMPL;
}

STDMETHODIMP_(void)
Transport::Done ()
{
    // Nothing, for now
}

STDMETHODIMP_(IHXBuffer*)
Transport::GetTransportHeader ()
{
    return static_cast<IHXBuffer*>(0);
}

STDMETHODIMP_(void)
Transport::AddStreamInfo (IHXValues* pStreamInfo, UINT32 ulBufferDepth)
{
    // Nothing, for now
}

STDMETHODIMP_(void)
Transport::SetSessionID (const char* pszSessionID)
{
    // Nothing, for now
}

STDMETHODIMP_(UINT32)
Transport::GetCapabilities ()
{
    return 0; // By default, capable of nothing
}

STDMETHODIMP
Transport::HandlePacket (IHXBuffer* pBuffer)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
Transport::SetFirstPlayTime (Timeval* pTv)
{
    return HXR_NOTIMPL;
}

static
HXBOOL
CompareUINT16Ptrs (void* lhs, void* rhs)
{
    return (*(reinterpret_cast<UINT16*>(lhs)) == *(reinterpret_cast<UINT16*>(rhs)));
}

HXBOOL
Transport::CarriesStreamNumber (UINT16 uStreamNumber)
{
    if (m_StreamNumbers.IsEmpty())
    {
        return FALSE;
    }

    return (m_StreamNumbers.ForEach(m_StreamNumbers.GetHeadPosition(),
                                    m_StreamNumbers.GetTailPosition(),
                                    &uStreamNumber,
                                    CompareUINT16Ptrs) != NULL);
}

void
Transport::AddStreamNumber (UINT16 uStreamNumber)
{
    UINT16* pStreamNumber = new UINT16;
    *pStreamNumber = uStreamNumber;
    m_StreamNumbers.AddTail(pStreamNumber);
}

