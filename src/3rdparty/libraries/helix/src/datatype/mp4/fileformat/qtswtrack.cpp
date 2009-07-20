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
 *  Includes
 */
#include "qtffplin.h"
#include "qtswtrack.h"
#include "qtpktasm.h"
#include "qtffrefcounter.h"

/****************************************************************************
 *  Defines
 */
#define MAX_RECURSIVE_GET_PACKETS   16
#define AFTER_SWITCH_WARMUP_DELAY   5	// ms

/****************************************************************************
 *  CQTSwitchTrack
 */
/****************************************************************************
 *  Constructors/Destructor
 */
CQTSwitchTrack::CQTSwitchTrack(UINT16 uNumSiwtchableTracks,
			       CQTTrack* pSwitchableTracks[],
			       CQTTrack* pPrimaryTrack,
			       UINT32 ulTrackSelectionMask,
			       CQTSwitchTrackMemberTable* &pSwitchTrackMemberTable)
{
    pSwitchTrackMemberTable = new CQTSwitchTrackMemberTable(uNumSiwtchableTracks,
							    pSwitchableTracks);

    if (pSwitchTrackMemberTable)
    {	
	pSwitchTrackMemberTable->AddRef();
    }

    _InitOnConstruct(pPrimaryTrack, ulTrackSelectionMask, pSwitchTrackMemberTable);
}

CQTSwitchTrack::CQTSwitchTrack(CQTSwitchTrackMemberTable* pSwitchTrackMemberTable,
			       CQTTrack* pPrimaryTrack,
			       UINT32 ulTrackSelectionMask)
{
    _InitOnConstruct(pPrimaryTrack, ulTrackSelectionMask, pSwitchTrackMemberTable);    
}

void CQTSwitchTrack::_InitOnConstruct(CQTTrack* pPrimaryTrack,
				      UINT32 ulTrackSelectionMask,
				      CQTSwitchTrackMemberTable* pSwitchTrackMemberTable)			      
{
    m_pClassFactory = NULL;
    m_pScheduler = NULL;
    m_pMemberTable = NULL;
    m_pPrimaryTrack = NULL;
    m_pFileFormat = NULL;
    m_pResponse = NULL;
    m_pSwitchBoard = NULL;
    m_pRunBoard = NULL;
    m_pRunBoardEntryPending = NULL;
    m_uPrimaryMemberId = QTSWT_BAD_TRACK_ID;
    m_bSwitchBoardDirty = FALSE;
    m_ulLastReturnedPacketTime = 0;
    m_bFirstPacket = TRUE;
    m_bFirstPacketAfterSwitch = FALSE;
    m_State = QTSWT_Offline;
    m_uStreamNum = 0;
    m_ulRecursionCount = 0;
    m_lRefCount = 0;

    m_ulTrackSelectionMask = ulTrackSelectionMask;

    HX_ASSERT(pPrimaryTrack);
    m_pPrimaryTrack = pPrimaryTrack;
    if (m_pPrimaryTrack)
    {
	m_pPrimaryTrack->AddRef();
    }

    HX_ASSERT(pSwitchTrackMemberTable);
    m_pMemberTable = pSwitchTrackMemberTable;
    if (m_pMemberTable)
    {
	m_pMemberTable->AddRef();

	m_uNumTracks = m_pMemberTable->GetNumTracks();
	m_uPrimaryMemberId = m_pMemberTable->GetMemberId(m_pPrimaryTrack);

	HX_ASSERT(m_uPrimaryMemberId != QTSWT_BAD_TRACK_ID);

	// We cannot accept a primary track that is not a member
	if (m_uPrimaryMemberId == QTSWT_BAD_TRACK_ID)
	{
	    HX_RELEASE(m_pPrimaryTrack);
	}
    }

    g_nRefCount_qtff++;
}

CQTSwitchTrack::~CQTSwitchTrack()
{
    Close();

    g_nRefCount_qtff--;
}


/****************************************************************************
 *  Main (IQTTrack) Interface
 */
/****************************************************************************
 *  Init
 */
HX_RESULT CQTSwitchTrack::Init(IQTTrackResponse* pResponse,
			       CQTFileFormat* pFileFormat,
			       CQTPacketAssembler* pPacketAssembler,
			       CQTPacketizerFactory* pPacketizerFactory,
			       const char* pProtocol)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pMemberTable && m_pPrimaryTrack)
    {
	retVal = HXR_OK;
    }
    
    if (SUCCEEDED(retVal))
    {
	HX_ASSERT(pResponse);
	HX_ASSERT(pFileFormat);
    
	m_pResponse = pResponse;
	pResponse->AddRef();
    
	m_pFileFormat = pFileFormat;
	pFileFormat->AddRef();
    
	retVal = pFileFormat->QueryInterface(IID_IHXCommonClassFactory,
					     (void**) &m_pClassFactory);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = m_pFileFormat->QueryInterface(IID_IHXScheduler, 
					       (void**) &m_pScheduler);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = m_pMemberTable->Init(m_ulTrackSelectionMask, 
				      m_pClassFactory);
    }

    if (SUCCEEDED(retVal))
    {
	m_pSwitchBoard = new CSwitchBoardEntry [m_uNumTracks];
	m_pRunBoard = new CRunBoardEntry [m_uNumTracks + 1];

	retVal = HXR_OUTOFMEMORY;
	if (m_pSwitchBoard && m_pRunBoard)
	{
	    retVal = HXR_OK;
	    m_State = QTSWT_Ready;
	}
    }

    if (SUCCEEDED(retVal) && pPacketAssembler)
    {
	// We need packet time stamps to hold decode time stamp rather
	// than transmission time (decode time ajusted by smoothing offset)
	// to be able to judge packet selection during switch better.
	pPacketAssembler->DisablePacketTimeOffset();
    }

    return retVal;
}

/****************************************************************************
 *  SetResponse
 */
 HX_RESULT CQTSwitchTrack::SetResponse(IQTTrackResponse* pResponse)
 {
     HX_RESULT retVal = HXR_OK;

     HX_ASSERT(pResponse);
     
     HX_RELEASE(m_pResponse);
     m_pResponse = pResponse;
     m_pResponse->AddRef();

     return retVal;
 }

/****************************************************************************
 *  Close
 */
void CQTSwitchTrack::Close(void)
{
    HX_VECTOR_DELETE(m_pSwitchBoard);
    HX_VECTOR_DELETE(m_pRunBoard);

    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pMemberTable);
    HX_RELEASE(m_pPrimaryTrack);
    HX_RELEASE(m_pFileFormat);
    HX_RELEASE(m_pResponse);
}

/****************************************************************************
 *  BuildStreamHeader
 */
HX_RESULT CQTSwitchTrack::BuildStreamHeader(IHXValues* &pHeader,
					    CQT_MovieInfo_Manager* pMovieInfo,
					    CQTTrackManager* pTrackManager)
{
    HX_RESULT retVal = HXR_OK;

    retVal = m_pPrimaryTrack->BuildStreamHeader(pHeader,
						pMovieInfo,
						pTrackManager);

    if (SUCCEEDED(retVal))
    {
	IHXBuffer* pASMRuleBook = m_pMemberTable->GetASMRuleBook();

	if (pASMRuleBook)
	{
	    retVal = pHeader->SetPropertyCString("ASMRuleBook", pASMRuleBook);
	}
    }

    return retVal;
}

/****************************************************************************
 *  GetPayloadIdentity
 */
HX_RESULT CQTSwitchTrack::GetPayloadIdentity(IHXValues* pHeader)
{
    return m_pPrimaryTrack->GetPayloadIdentity(pHeader);
}

/****************************************************************************
 *  Seek
 */
HX_RESULT CQTSwitchTrack::Seek(ULONG32 ulTime, HXBOOL bUseSyncPoints)
{
    HX_RESULT retVal = HXR_OK;

    m_bFirstPacket = TRUE;		    // Starting new packet sequence
    m_ulLastReturnedPacketTime = ulTime;

    retVal = Switch(TRUE, bUseSyncPoints);  // Switch to new time-line

    return retVal;
}

/****************************************************************************
 *  GetPacket
 */
HX_RESULT CQTSwitchTrack::GetPacket(UINT16 uStreamNum)
{
    HX_RESULT retVal = HXR_STREAM_DONE;

    if (m_ulRecursionCount <= MAX_RECURSIVE_GET_PACKETS)
    {
	m_ulRecursionCount++;

	// If Run board doesn't have tracks to process, this stream is done
	if (m_pRunBoard->m_pQTTrack)
	{
	    // If only one entry in the Run-board (common case), optimize
	    // by avoiding any search logic
	    if (!(m_pRunBoard[1].m_pQTTrack))
	    {
		retVal = m_pRunBoard->m_pQTTrack->GetPacket(uStreamNum);
	    }
	    else
	    {
		UINT32 ulMinTime = 0xFFFFFFFF;
		IQTTrack* pSelectedTrack = NULL;
		CRunBoardEntry* pSelectedEntry = NULL;
		CRunBoardEntry* pRunBoardEntry = m_pRunBoard;
		ULONG32 ulTestTime;
		
		do
		{
		    if (pRunBoardEntry->m_pNextPacket)
		    {
			ulTestTime = pRunBoardEntry->m_pNextPacket->GetTime();
			if ((!pSelectedEntry) ||
			    (((LONG32) (ulMinTime - ulTestTime)) > 0))
			{
			    pSelectedEntry = pRunBoardEntry;
			    ulMinTime = ulTestTime;
			}
		    }
		    else if (!pRunBoardEntry->m_bTrackDone)
		    {
			pSelectedEntry = pRunBoardEntry;
			break;
		    }
		    
		    pRunBoardEntry++;
		} while (pRunBoardEntry->m_pQTTrack);
		
		if (pSelectedEntry)
		{
		    m_pRunBoardEntryPending = pSelectedEntry;
		    
		    retVal = pSelectedEntry->m_pQTTrack->GetPacket(uStreamNum);
		}
		else
		{
		    retVal = m_pResponse->PacketReady(uStreamNum,
			HXR_STREAM_DONE,
			NULL);
		}
	    }
	}
	else
	{
	    // Run board is empty
	    // If we have not produced any packets, see if we can perform
	    // a switch to populate the run board (initial switch).
	    retVal = Switch();
	    
	    if (SUCCEEDED(retVal))
	    {
		if (m_pRunBoard->m_pQTTrack)
		{
		    // We have something to run on - retry GetPacket
		    retVal = GetPacket(uStreamNum);
		}
		else
		{
		    // Nothing to run with - end this stream
		    retVal = m_pResponse->PacketReady(uStreamNum,
			HXR_STREAM_DONE,
			NULL);
		}
	    }
	}
    }
    else
    {
	m_uStreamNum = uStreamNum;
	retVal = ((m_pScheduler->RelativeEnter(this, 0) == NULL) ? 
		  HXR_FAIL : 
		  HXR_OK);
    }

    m_ulRecursionCount = 0;

    return retVal;
}

/****************************************************************************
 *  PacketReady
 */
HX_RESULT CQTSwitchTrack::PacketReady(UINT16 uStreamNum, 
				      HX_RESULT status, 
				      IHXPacket* pPacket)
{
    IHXPacket* pPacketToSend;
    HX_RESULT retVal = HXR_OK;

    if (!(m_pRunBoard[1].m_pQTTrack))
    {
	// If only one entry on the run-board (common case), 
	// optimize by forwarding to ProcessPacket immediately.
	return ProcessPacket(uStreamNum, 
			     status, 
			     pPacket,
			     m_pRunBoard->m_uMemberId);
    }
    else
    {
	// Run-board contains more than one entry in this case.
	// Queue the newly obtained packet and send-off the one the
	// new packet replaces.
	HX_ASSERT(m_pRunBoardEntryPending);

	pPacketToSend = m_pRunBoardEntryPending->m_pNextPacket;
	m_pRunBoardEntryPending->m_pNextPacket = NULL;

	if (status == HXR_OK)
	{
	    m_pRunBoardEntryPending->m_pNextPacket = pPacket;
	    pPacket->AddRef();
	}
	else
	{
	    m_pRunBoardEntryPending->m_bTrackDone = TRUE;
	}
 
	// If we have packet to send, process it now.
	// If not, request more packets.
	if (pPacketToSend)
	{
	    retVal = ProcessPacket(uStreamNum, 
				   status, 
				   pPacketToSend,
				   m_pRunBoardEntryPending->m_uMemberId);
	    pPacketToSend->Release();
	}
	else
	{
	    retVal = GetPacket(uStreamNum);
	}
    }

    return retVal;
}

/****************************************************************************
 *  ProcessPacket
 */
HX_RESULT CQTSwitchTrack::ProcessPacket(UINT16 uStreamNum, 
					HX_RESULT status, 
					IHXPacket* pPacket,
					UINT16 uMemberId)
{
    HX_RESULT retVal = HXR_FAIL;

    // Only currently packets of currently subscribed tracks arrive here
    if (status == HXR_OK)
    {
	UINT32 ulPacketTime = pPacket->GetTime();
	HXBOOL bSwitchAllowed = 
	    (((LONG32) (ulPacketTime - m_ulLastReturnedPacketTime)) > 0);
	
	// Strip any packets with dispatch time prior to last returned
	// packet dispatch time
	if ((!m_bFirstPacketAfterSwitch) ||
	    bSwitchAllowed ||
	    m_bFirstPacket)
	{
	    // Perform switching only on timestamp changes
	    if (m_bSwitchBoardDirty &&
		(bSwitchAllowed || 
		 m_bFirstPacket ||
		 m_bFirstPacketAfterSwitch))
	    {
		Switch();  // This may cause change in subscription
	    }
	    
	    if (m_pSwitchBoard[uMemberId].m_bIsTrackSubscribed)
	    {
		m_ulLastReturnedPacketTime = ulPacketTime;
		m_bFirstPacket = FALSE;
		m_bFirstPacketAfterSwitch = FALSE;

		return m_pResponse->PacketReady(uStreamNum, 
						status, 
						pPacket);
	    }
	}

	// If we are here, the passed in packet is being stripped
	// since it is either stale or because recent subscription made it
	// member of unsubscribed track.
	// Issue the request for more packets.
	retVal = GetPacket(uStreamNum);
    }
    else
    {
	retVal = m_pResponse->PacketReady(uStreamNum, 
					  status, 
					  pPacket);
    }

    return retVal;
}

/****************************************************************************
 *  Switch
 */
HX_RESULT CQTSwitchTrack::Switch(HXBOOL bOnSeek, HXBOOL bUseSyncPoints)
{
    UINT16 uMemberId;
    CRunBoardEntry* pRunBoardEntry = m_pRunBoard;
    HX_RESULT retVal = HXR_OK;

    if ((!bOnSeek) && (!m_bSwitchBoardDirty))
    {
	return retVal;
    }
    
    // Refresh Switch-board by transfering information from Run-board for switch
    // board entires that are going to be subscribed by this switch..
    // Clear the Run-board in the process.
    while (pRunBoardEntry->m_pQTTrack)
    {
	HX_ASSERT(m_pSwitchBoard[pRunBoardEntry->m_uMemberId].m_pNextPacket == NULL);

	if (pRunBoardEntry->m_pNextPacket)
	{
	    if (m_pSwitchBoard[pRunBoardEntry->m_uMemberId].m_bIsSubscriptionPending)
	    {
		m_pSwitchBoard[pRunBoardEntry->m_uMemberId].m_pNextPacket = 
		    pRunBoardEntry->m_pNextPacket;
		pRunBoardEntry->m_pNextPacket = NULL;
	    }
	}

	pRunBoardEntry->Clear();

	pRunBoardEntry++;
    }

    // Initialize any subscribed tracks and install them into the run-board
    pRunBoardEntry = m_pRunBoard;

    for (uMemberId = 0; 
	 SUCCEEDED(retVal) && (uMemberId < m_uNumTracks); 
	 uMemberId++)
    {
	// Install into the run-board if subscribed
	if (m_pSwitchBoard[uMemberId].m_bIsSubscriptionPending)
	{
	    pRunBoardEntry->m_uMemberId = uMemberId;
	    pRunBoardEntry->m_pQTTrack = m_pMemberTable->GetTrack(uMemberId);
	    HX_ASSERT(pRunBoardEntry->m_pQTTrack);
	    retVal = HXR_FAIL;
	    if (pRunBoardEntry->m_pQTTrack)
	    {
		pRunBoardEntry->m_pQTTrack->AddRef();
		pRunBoardEntry->m_pQTTrack->SetResponse(this);

		// Seek the newly subscribd tracks or if this switch is due to
		// externally initiated seek action
		if (bOnSeek ||
		    (!m_pSwitchBoard[uMemberId].m_bIsTrackSubscribed))
		{
		    UINT32 ulSeekTime = m_ulLastReturnedPacketTime;

		    if (!bOnSeek)
		    {
			if (ulSeekTime > AFTER_SWITCH_WARMUP_DELAY)
			{
			    ulSeekTime -= AFTER_SWITCH_WARMUP_DELAY;
			}
			else
			{
			    ulSeekTime = 0;
			}
		    }

		    // Seek the track to last returned packet time
		    pRunBoardEntry->m_pQTTrack->Seek(ulSeekTime, 
						     bUseSyncPoints);
		}

		retVal = HXR_OK;
	    }
	    pRunBoardEntry->m_bTrackDone = FALSE;
	    pRunBoardEntry->m_pNextPacket = m_pSwitchBoard[uMemberId].m_pNextPacket;
	    m_pSwitchBoard[uMemberId].m_pNextPacket = NULL;

	    pRunBoardEntry++;
	}

	HX_ASSERT(m_pSwitchBoard[uMemberId].m_pNextPacket == NULL);
	HX_RELEASE(m_pSwitchBoard[uMemberId].m_pNextPacket);  // just in case

	// Execute pending subscriptions/unsubscriptions
	m_pSwitchBoard[uMemberId].m_bIsTrackSubscribed = 
	    m_pSwitchBoard[uMemberId].m_bIsSubscriptionPending;
    }

    m_bSwitchBoardDirty = FALSE;
    m_bFirstPacketAfterSwitch = TRUE;
    
    // Make certain the run-board is NULL terminated
    pRunBoardEntry->Clear();

    return retVal;
}

/****************************************************************************
 *  Subscribe
 */
HX_RESULT CQTSwitchTrack::Subscribe(UINT16 uRuleNumber)
{
    return _Subscribe(uRuleNumber, TRUE);
}

/****************************************************************************
 *  Unsubscribe
 */
HX_RESULT CQTSwitchTrack::Unsubscribe(UINT16 uRuleNumber)
{
    return _Subscribe(uRuleNumber, FALSE);
}

HX_RESULT CQTSwitchTrack::_Subscribe(UINT16 uRuleNumber, HXBOOL bDoSubscribe)
{
    HX_RESULT retVal = HXR_FAIL;

    UINT16 uMemberId = m_pMemberTable->MapRuleToMemberId(uRuleNumber);
    if (uMemberId < m_uNumTracks)
    {
	if (m_pSwitchBoard[uMemberId].m_bIsSubscriptionPending != bDoSubscribe)
	{
	    m_pSwitchBoard[uMemberId].m_bIsSubscriptionPending = bDoSubscribe;
	    m_bSwitchBoardDirty = TRUE;
	}

	retVal = HXR_OK;
    }

    return retVal;
}

/****************************************************************************
 *  SubscribeDefault
 */
HX_RESULT CQTSwitchTrack::SubscribeDefault(void)
{
    UINT16 uMemberId;
    HXBOOL bDoSubscribe;
    HX_RESULT retVal = HXR_OK;

    for (uMemberId = 0; uMemberId < m_uNumTracks; uMemberId++)
    {
	bDoSubscribe = (uMemberId == m_uPrimaryMemberId);

	if (m_pSwitchBoard[uMemberId].m_bIsSubscriptionPending != bDoSubscribe)
	{
	    m_pSwitchBoard[uMemberId].m_bIsSubscriptionPending = bDoSubscribe;
	    m_bSwitchBoardDirty = TRUE;
	}
    }

    return retVal;
}

/****************************************************************************
 *  SubscribeDefault
 */
HXBOOL CQTSwitchTrack::IsSubscribed(void)
{
    UINT16 uMemberId;
    HXBOOL bIsSubscribed = FALSE;

    for (uMemberId = 0; uMemberId < m_uNumTracks; uMemberId++)
    {
	bIsSubscribed = (bIsSubscribed ||
			 m_pSwitchBoard[uMemberId].m_bIsTrackSubscribed ||
			 m_pSwitchBoard[uMemberId].m_bIsSubscriptionPending);
    }

    return bIsSubscribed;
}

/****************************************************************************
 *  ComputeTrackSize
 */
HX_RESULT CQTSwitchTrack::ComputeTrackSize(ULONG32& ulTrackSizeOut)
{
    return m_pPrimaryTrack->ComputeTrackSize(ulTrackSizeOut);
}

/****************************************************************************
 *  GetID
 */
ULONG32 CQTSwitchTrack::GetID(void)
{
    return m_pPrimaryTrack->GetID();
}

/****************************************************************************
 *  GetTrackWidth
 */
ULONG32 CQTSwitchTrack::GetTrackWidth(void)
{
    return m_pPrimaryTrack->GetTrackWidth();
}

/****************************************************************************
 *  GetTrackHeight
 */
ULONG32 CQTSwitchTrack::GetTrackHeight(void)
{
    return m_pPrimaryTrack->GetTrackHeight();
}

/****************************************************************************
 *  GetSDPLength
 */
ULONG32 CQTSwitchTrack::GetSDPLength(void)
{
    return m_pPrimaryTrack->GetSDPLength();
}

/****************************************************************************
 *  GetSDP
 */
const UINT8* CQTSwitchTrack::GetSDP(void)
{
    return m_pPrimaryTrack->GetSDP();
}

/****************************************************************************
 *  ObtainTrackBitrate
 */
HX_RESULT CQTSwitchTrack::ObtainTrackBitrate(ULONG32& ulAvgBitrateOut)
{
    return m_pPrimaryTrack->ObtainTrackBitrate(ulAvgBitrateOut);
}

/****************************************************************************
 *  GetChunkToOffsetMgr
 */
CQT_ChunkToOffset_Manager* CQTSwitchTrack::GetChunkToOffsetMgr(void)
{
    return m_pPrimaryTrack->GetChunkToOffsetMgr();
}


/****************************************************************************
 *  IHXCallback methods
 */
/****************************************************************************
 *  Func
 */
STDMETHODIMP CQTSwitchTrack::Func(void)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pRunBoard)
    {
	retVal = GetPacket(m_uStreamNum);

	if (FAILED(retVal))
	{
	    retVal = m_pResponse->PacketReady(m_uStreamNum,
					      HXR_STREAM_DONE,
					      NULL);
	}
    }

    return retVal;
}

/****************************************************************************
 *  IUnknown methods
 */
STDMETHODIMP CQTSwitchTrack::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//
STDMETHODIMP_(ULONG32) CQTSwitchTrack::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//
STDMETHODIMP_(ULONG32) CQTSwitchTrack::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


