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
//#define _IGNORE_HINTS

#define INV_HINT_TRACK_IDX  0xFFFFFFFF
#define INV_STREAM_NUM	    0xFFFF

#define VISITED_ALT_GROUP	0x01
#define VISITED_SWITCH_GROUP	0x02
#define VISITED_ALL_GROUPS	(VISITED_ALT_GROUP | VISITED_SWITCH_GROUP)


/****************************************************************************
 *  Includes
 */
#include "qttrkmgr.h"
#include "qttrack.h"
#include "qtswtrack.h"
#include "qtffplin.h"


/****************************************************************************
 *  Class CTrackManager
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CQTTrackManager::CQTTrackManager(void)
    : m_uNumTracks(0)
    , m_uNumStreams(0)
    , m_uNumStreamGroups(0)
    , m_bHinted(FALSE)
    , m_bHintTracksActive(FALSE)
    , m_FType(QT_FTYPE_UNKNOWN)
    , m_EType(QT_ETYPE_UNKNOWN)
    , m_bInitialSubscriptionWindowClosed(FALSE)
    , m_pTrackTable(NULL)
    , m_pStreamToTrackMap(NULL)
    , m_pTrackAtomList(new CHXSimpleList)
    , m_pHintTrackIdxList(new CHXSimpleList)
    , m_pIodsAtom(NULL)
    , m_pFtypAtom(NULL)
{
    HX_ASSERT(m_pTrackAtomList);
    HX_ASSERT(m_pHintTrackIdxList);
}


CQTTrackManager::~CQTTrackManager()
{
    Clear();
}


/****************************************************************************
 *  Main Interface
 */
/****************************************************************************
 *  ManageTracks
 */
HX_RESULT CQTTrackManager::ManageTracks(CQTAtom *pRootAtom)
{
    if ((m_pTrackAtomList == NULL) ||
	(m_pHintTrackIdxList == NULL))
    {
	HX_ASSERT(!"shouldn't happen");
	return HXR_UNEXPECTED;
    }

    AddTracks(pRootAtom);

    if (m_FType == QT_FTYPE_UNKNOWN)
    {
	m_FType = QT_FTYPE_QT;
    }

    return HXR_OK;
}

/****************************************************************************
 *  ReadyTracks
 */
HX_RESULT CQTTrackManager::ReadyTracks(HXBOOL bIgnoreHintTracks,
				       HXBOOL bFallbackToTracks)
{
    HX_RESULT retVal = HXR_OK;

#ifdef _IGNORE_HINTS
    bIgnoreHintTracks = TRUE;
#endif	// _IGNORE_HINTS

    if ((m_pTrackAtomList == NULL) ||
	(m_pHintTrackIdxList == NULL) ||
	(m_pTrackAtomList->GetCount() == 0))
    {
	HX_ASSERT(!"shouldn't happen");
	retVal = HXR_CORRUPT_FILE;
    }

    if (SUCCEEDED(retVal))
    {
	m_uNumStreams = 0;
        m_uNumStreamGroups = 0;
	m_uNumTracks = m_pTrackAtomList->GetCount();

	if (!bIgnoreHintTracks)
	{
	    m_uNumStreams = m_pHintTrackIdxList->GetCount();
	}

	m_bHinted = (m_uNumStreams != 0);
	m_bHintTracksActive = m_bHinted;

	DecideHintTrackActivation(bFallbackToTracks);

        m_uNumStreamGroups = m_uNumStreams;

	if (m_uNumStreams <= 0)
	{
	    retVal = HXR_FAIL;	// No Streamable Tracks
	}
    }

    // Allocate Tables
    if (SUCCEEDED(retVal))
    {
	HX_ASSERT(!m_pTrackTable);
	HX_ASSERT(!m_pStreamToTrackMap);

	m_pStreamToTrackMap = new CQTStream [m_uNumStreams];
	m_pTrackTable = new CQTTrackTable [m_uNumTracks];

	if ((m_pStreamToTrackMap == NULL) ||
	    (m_pTrackTable == NULL))
	{
	    retVal = HXR_OUTOFMEMORY;
	}
    }

    // Fill Tables with info.
    if (SUCCEEDED(retVal))
    {
	LISTPOSITION HintTrackIdxListPosition;
	LISTPOSITION TrackAtomListPosition;
	ULONG32 ulHintTrackIdx = INV_HINT_TRACK_IDX;
	UINT16 uStreamTrackCount = 0;
	UINT16 uHintTrackCount = 0;
	CQTAtom* pTrackAtom;
	UINT16 uTrackIdx = 0;

	HintTrackIdxListPosition = m_pHintTrackIdxList->GetHeadPosition();
	TrackAtomListPosition = m_pTrackAtomList->GetHeadPosition();

	if (HintTrackIdxListPosition)
	{
	    ulHintTrackIdx = (UINT32) m_pHintTrackIdxList->GetNext(
						HintTrackIdxListPosition);
	}

	do
	{
	    pTrackAtom = (CQTAtom*) m_pTrackAtomList->GetNext(
					TrackAtomListPosition);

	    HX_ASSERT(pTrackAtom);

	    if (uTrackIdx == ulHintTrackIdx)
	    {
		// This is a hint track
		if (m_bHintTracksActive)
		{
		    retVal = CreateHintTrack(pTrackAtom,
					     uTrackIdx,
					     uStreamTrackCount);
		    if (retVal != HXR_OK)
		    {
			break;
		    }
		}

		uHintTrackCount++;

		if (uHintTrackCount < m_pHintTrackIdxList->GetCount())
		{
		    ulHintTrackIdx = (UINT32) m_pHintTrackIdxList->GetNext(
						    HintTrackIdxListPosition);
		}
	    }
	    else
	    {
		// This a standard track
		m_pTrackTable[uTrackIdx].m_pTrack = new CQTTrack(pTrackAtom);
		if (m_pTrackTable[uTrackIdx].m_pTrack == NULL)
		{
		    retVal = HXR_OUTOFMEMORY;
		    break;
		}

		m_pTrackTable[uTrackIdx].m_pTrack->AddRef();

    		m_pTrackTable[uTrackIdx].m_pTrackAtom = pTrackAtom;
		pTrackAtom->AddRef();

		if (!m_bHintTracksActive)
		{
		    HX_ASSERT(uStreamTrackCount < m_uNumStreams);

		    if (uStreamTrackCount < m_uNumStreams)
		    {
			m_pStreamToTrackMap[uStreamTrackCount].m_pIQTTrack =
			    m_pTrackTable[uTrackIdx].m_pTrack;
			m_pStreamToTrackMap[uStreamTrackCount].m_pIQTTrack->AddRef();
			m_pStreamToTrackMap[uStreamTrackCount].m_uStreamGroupNumber =
			    uStreamTrackCount;

			uStreamTrackCount++;
		    }
		}

		// For now keep references to all "standard" tracks.
		// To do : reference only "standard" tracks referenced
		//         by hint tracks - when hint tracks are active
		m_pTrackTable[uTrackIdx].m_uRefCount++;
	    }

	    uTrackIdx++;
	} while (uTrackIdx < m_uNumTracks);
    }

    // Free Unreferenced Standard Tracks
    if (SUCCEEDED(retVal))
    {
	UINT16 uTrackIdx;

	for (uTrackIdx = 0; uTrackIdx < m_uNumTracks; uTrackIdx++)
	{
	    if (m_pTrackTable[uTrackIdx].m_uRefCount == 0)
	    {
		m_pTrackTable[uTrackIdx].Clear();
	    }
	}
    }

    if (FAILED(retVal))
    {
	Clear();
    }

    return retVal;
}

/****************************************************************************
 *  CloseTracks
 */
void CQTTrackManager::CloseTracks(void)
{
    Clear();
}

/****************************************************************************
 *  ResetTracks - reset what's done in ReadyTracks()
 */
void CQTTrackManager::ResetTracks(void)
{
    HX_VECTOR_DELETE(m_pTrackTable);
    HX_VECTOR_DELETE(m_pStreamToTrackMap);

    m_uNumTracks = 0;
    m_uNumStreams = 0;
    m_uNumStreamGroups = 0;
}

/****************************************************************************
 *  InitTracks
 */
HX_RESULT CQTTrackManager::InitTracks(CQTFileFormat *pFileFormat,
				      CQTPacketAssembler *pPacketAssembler,
				      CQTPacketizerFactory* pPacketizerFactory,
				      const char* pProtocol)
{
    UINT16 uTrackIdx;
    HX_RESULT retVal = HXR_OK;

    if (m_pTrackTable)
    {
	UINT16 uStreamNum;
	HXBOOL bSomeStreamsDeactivated = FALSE;

	for (uTrackIdx = 0;
	     SUCCEEDED(retVal) && (uTrackIdx < m_uNumTracks);
	     uTrackIdx++)
	{
	    if (m_pTrackTable[uTrackIdx].m_pTrack)
	    {
		retVal = m_pTrackTable[uTrackIdx].m_pTrack->Init(pFileFormat,
								 pFileFormat,
								 pPacketAssembler,
								 pPacketizerFactory,
								 pProtocol);

		uStreamNum = GetTrackStreamNum(m_pTrackTable[uTrackIdx].m_pTrack);

		if (uStreamNum < m_uNumStreams)
		{
		    m_pStreamToTrackMap[uStreamNum].m_bActive = (retVal == HXR_OK);
		    bSomeStreamsDeactivated = bSomeStreamsDeactivated ||
					      (!m_pStreamToTrackMap[uStreamNum].m_bActive);

		    if ((retVal == HXR_IGNORE) ||
			(retVal == HXR_NO_DATA))
		    {
			retVal = HXR_OK;
		    }
		}
	    }
	}

	// Some streams could have been
        // disabled during initialization - remove the inactive ones
	if (SUCCEEDED(retVal) && bSomeStreamsDeactivated)
	{
	    retVal = RemoveInactiveStreams();
	}

#ifdef QTCONFIG_ALTERNATE_STREAMS
	// Return remaining streams to inactive (unsubscribed) state
	if (SUCCEEDED(retVal))
	{	    
	    for (uTrackIdx = 0; uTrackIdx < m_uNumStreams; uTrackIdx++)
	    {
		m_pStreamToTrackMap[uTrackIdx].m_bActive = FALSE;
	    }
	    
	    m_bInitialSubscriptionWindowClosed = FALSE;
	}
#endif	// QTCONFIG_ALTERNATE_STREAMS

	// After track initialization, release all Track Atoms from
	// the track manager
	for (uTrackIdx = 0;
	     uTrackIdx < m_uNumTracks;
	     uTrackIdx++)
	{
	    HX_RELEASE(m_pTrackTable[uTrackIdx].m_pTrackAtom);
	}
    }
    else
    {
	retVal = HXR_UNEXPECTED;
    }

    if (SUCCEEDED(retVal))
    {
        // Release Lists
	DeleteTrackAtomList();
	HX_DELETE(m_pHintTrackIdxList);
    }

    return retVal;
}

#ifdef QTCONFIG_ALTERNATE_STREAMS
/****************************************************************************
 *  InitStreamGroups()
 */
HX_RESULT CQTTrackManager::InitStreamGroups(CQTFileFormat *pFileFormat,
					    CQTPacketAssembler* pPacketAssembler)
{
    UINT16 uStrmIdx;
    HX_RESULT retVal = HXR_OUTOFMEMORY;
    UINT8* pVisitedArray = new UINT8 [m_uNumStreams];
    CQTTrack** ppQTTrackArray = new CQTTrack* [m_uNumStreams];
    UINT16* pSwitchIdxArray = new UINT16 [m_uNumStreams];
    CQTTrack** ppSwitchTrackArray = new CQTTrack* [m_uNumStreams];

    HX_ASSERT(pVisitedArray);

    if (pVisitedArray && ppQTTrackArray && pSwitchIdxArray && ppSwitchTrackArray)
    {
	memset(pVisitedArray, 0, sizeof(UINT8) * m_uNumStreams);
	memset(ppQTTrackArray, 0, sizeof(CQTTrack*) * m_uNumStreams);
	memset(pSwitchIdxArray, 0, sizeof(UINT16) * m_uNumStreams);
	memset(ppSwitchTrackArray, 0, sizeof(CQTTrack*) * m_uNumStreams);
	retVal = HXR_OK;
    }

    // Obtain all stream information available from initialized tracks
    if (SUCCEEDED(retVal))
    {
	CQTStream* pQTStream;
	CQTTrack* pQTTrack;

	for (uStrmIdx = 0; uStrmIdx < m_uNumStreams; uStrmIdx++)
	{
	    pQTStream = &m_pStreamToTrackMap[uStrmIdx];

	    // Look for group affiliation for yet unaffiliated streams
	    if (pQTStream->m_eGroupType == QTGROUP_NONE)
	    {
		pQTTrack = GetTrackById(pQTStream->m_pIQTTrack->GetID());

		HX_ASSERT(pQTTrack);

		ppQTTrackArray[uStrmIdx] = pQTTrack;

		pQTStream->m_uAltGroupId = pQTTrack->m_TrackInfo.GetAltGroupId();
		if (pQTStream->m_uAltGroupId != 0)
		{
		    if (GetFType() == QT_FTYPE_QT)
		    {
			// For QuickTime native files, assume legacy loose stream coupling
			// and bandwidth as selection criteria.
			pQTStream->m_eGroupType = QTGROUP_LOOSE;
			pQTStream->m_ulStreamSelectionMask = QT_TSEL_BANDWIDTH;
		    }
		    else
		    {
			// For non-QuickTime files, assume new tight stream coupling,
			// possibility of switch group use and explicit specification
			// of selection cfriteria.
			pQTStream->m_eGroupType = QTGROUP_TIGHT;
			pQTStream->m_ulSwitchGroupId = pQTTrack->m_TrackInfo.GetSwitchGroupId();
			pQTStream->m_ulStreamSelectionMask = pQTTrack->m_TrackInfo.GetTrackSelectionMask();
		    }
		}

		pQTStream->m_ulBandwidth = 0;
		if (pQTStream->m_ulStreamSelectionMask & QT_TSEL_BANDWIDTH)
		{
		    pQTTrack->ObtainTrackBandwidth(pQTStream->m_ulBandwidth);
		}
	    }
	}
    }

    // Assign stream group numbers, idetify default tracks amongst alternates and
    // create switch tracks if appropriate
    if (SUCCEEDED(retVal))
    {
	UINT16 uStreamGroupNumber = 0;

	for (uStrmIdx = 0; uStrmIdx < m_uNumStreams; uStrmIdx++)
	{
	    if (pVisitedArray[uStrmIdx] != VISITED_ALL_GROUPS)
	    {
		if (m_pStreamToTrackMap[uStrmIdx].m_eGroupType != QTGROUP_NONE)
		{
		    UINT16 uSubIdx;
		    UINT16 uAltGroupId = m_pStreamToTrackMap[uStrmIdx].m_uAltGroupId;
		    QTTrackGroupType eGroupType = m_pStreamToTrackMap[uStrmIdx].m_eGroupType;
		    UINT16 uDefaultStrmIdx = INV_STREAM_NUM;
		    UINT32 ulMinTrackID = 0xFFFFFFFF;

#ifdef QTCONFIG_SWITCHABLE_STREAMS
                    UINT16 uNumStreamsInGroup = 0;
                    UINT32 ulSwitchGroupId = 
                        m_pStreamToTrackMap[uStrmIdx].m_ulSwitchGroupId;
                    UINT32 ulStreamSelectionMask = 0;
#endif /* QTCONFIG_SWITCHABLE_STREAMS */
		    
		    for (uSubIdx = uStrmIdx; uSubIdx < m_uNumStreams; uSubIdx++)
		    {
			// Identify default track (one with smallest track id) in group
			// of alternates and assign stream group number to each alternate
			// group of the same type.
			if ((!(pVisitedArray[uSubIdx] & VISITED_ALT_GROUP)) &&
			    (m_pStreamToTrackMap[uSubIdx].m_uAltGroupId == uAltGroupId) &&
			    (m_pStreamToTrackMap[uSubIdx].m_eGroupType == eGroupType))
			{
			    // Identify smallest track id as basis for default stream search
			    if (ulMinTrackID >= m_pStreamToTrackMap[uSubIdx].m_pIQTTrack->GetID())
			    {
				ulMinTrackID = m_pStreamToTrackMap[uSubIdx].m_pIQTTrack->GetID();
				uDefaultStrmIdx = uSubIdx;
			    }

			    // Assign same Stream Group Number to all members 
			    // of alt-group of the same type.
			    m_pStreamToTrackMap[uSubIdx].m_uStreamGroupNumber = uStreamGroupNumber;

			    // Make note we processed this stream for items regarding
			    // alt-group consideration.
			    pVisitedArray[uSubIdx] |= VISITED_ALT_GROUP;

			    // If vistied track has no switch group affiliation, make note
			    // that it need not be revistied for purposes of switch group
			    // consideration (optimization).
			    if (m_pStreamToTrackMap[uSubIdx].m_ulSwitchGroupId == 0)
			    {
				pVisitedArray[uSubIdx] |= VISITED_SWITCH_GROUP;
			    }
			}
	
#ifdef QTCONFIG_SWITCHABLE_STREAMS
			// Identify indexes into streams belonging to the same switch group
			if ((!(pVisitedArray[uSubIdx] & VISITED_SWITCH_GROUP)) &&
			    (ulSwitchGroupId != 0) &&
			    (m_pStreamToTrackMap[uSubIdx].m_uAltGroupId == uAltGroupId) &&
			    (m_pStreamToTrackMap[uSubIdx].m_eGroupType == eGroupType) &&
			    (m_pStreamToTrackMap[uSubIdx].m_ulSwitchGroupId == ulSwitchGroupId))
			{
			    pSwitchIdxArray[uNumStreamsInGroup] = uSubIdx;
			    ppSwitchTrackArray[uNumStreamsInGroup] = ppQTTrackArray[uSubIdx];
			    ulStreamSelectionMask |= m_pStreamToTrackMap[uSubIdx].m_ulStreamSelectionMask;
			    uNumStreamsInGroup++;

			    // Make note we processed this stream for items regarding
			    // switch-group consideration.
			    pVisitedArray[uSubIdx] |= VISITED_SWITCH_GROUP;
			}
#endif	// QTCONFIG_SWITCHABLE_STREAMS
		    }
		    
#ifdef QTCONFIG_SWITCHABLE_STREAMS
		    // If there are more than 1 streams in the switch group,
		    // we need to form a switch track for each stream.
		    if (uNumStreamsInGroup > 1)
		    {
			UINT16 uGroupIdx;
			CQTSwitchTrackMemberTable* pSwitchTrackMemberTable = NULL;
			CQTSwitchTrack* pSwitchTrack = NULL;
			
			for (uGroupIdx = 0; 
			     SUCCEEDED(retVal) && (uGroupIdx < uNumStreamsInGroup); 
			     uGroupIdx++)
			{
			    retVal = HXR_OUTOFMEMORY;
			    
			    if (uGroupIdx == 0)
			    {
				pSwitchTrack = new CQTSwitchTrack(
				    uNumStreamsInGroup,
				    ppSwitchTrackArray,
				    ppSwitchTrackArray[uGroupIdx],  // primary track
				    ulStreamSelectionMask,
				    pSwitchTrackMemberTable);
			    }
			    else
			    {
				HX_ASSERT(pSwitchTrackMemberTable);
				
				pSwitchTrack = new CQTSwitchTrack(
				    pSwitchTrackMemberTable,
				    ppSwitchTrackArray[uGroupIdx],  // primary track
				    ulStreamSelectionMask);
			    }
			    
			    if (pSwitchTrack)
			    {
				pSwitchTrack->AddRef();
				
				if (pSwitchTrackMemberTable)
				{
				    // Switch Tracks have no need for packet assembler or
				    // protocol since all the packetization occurs in
				    // member tracks they own.
				    retVal = pSwitchTrack->Init(pFileFormat,
								pFileFormat,
								pPacketAssembler,
								NULL);	// Packetizer Factory
				}
			    }
			    
			    // Replace standard tracks with switch tracks
			    if (SUCCEEDED(retVal))
			    {
				CQTStream* pQTStream = 
				    &m_pStreamToTrackMap[pSwitchIdxArray[uGroupIdx]];
				
				HX_RELEASE(pQTStream->m_pIQTTrack);
				pQTStream->m_pIQTTrack = pSwitchTrack;
				pSwitchTrack->AddRef();
				HX_ASSERT(pQTStream->m_pQTSwitchTrack == NULL);
				pQTStream->m_pQTSwitchTrack = pSwitchTrack;
				pSwitchTrack = NULL;
				
				// Replace the stream selection mask with the union of the
				// stream selection masks of all tracks in the switch group
				pQTStream->m_ulStreamSelectionMask = ulStreamSelectionMask;
			    }

			    HX_RELEASE(pSwitchTrack);
			}
			
			HX_RELEASE(pSwitchTrackMemberTable);
		    }
#endif	// QTCONFIG_SWITCHABLE_STREAMS

		    if (uDefaultStrmIdx != INV_STREAM_NUM)
		    {
			m_pStreamToTrackMap[uDefaultStrmIdx].m_bDefaultAlternate = TRUE;

			// If we identified a default alternate stream in an 
			// alternate group,this means that we just processed
			// a new alternate group.  Update the stream group
			// number to a new, sequential, unique value for 
			// subsequent processing.
			uStreamGroupNumber++;
		    }
		}
		else
		{
		    // Each Non-grouped stram gets a stream group number of its own
		    HX_ASSERT(!pVisitedArray[uStrmIdx]);

		    m_pStreamToTrackMap[uStrmIdx].m_uStreamGroupNumber = uStreamGroupNumber;
		    uStreamGroupNumber++;
		}

		pVisitedArray[uStrmIdx] = VISITED_ALL_GROUPS;
	    }
	}
        m_uNumStreamGroups = uStreamGroupNumber;
    }

    HX_VECTOR_DELETE(pVisitedArray);
    HX_VECTOR_DELETE(ppQTTrackArray);
    HX_VECTOR_DELETE(pSwitchIdxArray);
    HX_VECTOR_DELETE(ppSwitchTrackArray);

    return retVal;
}

/****************************************************************************
 *  AddStreamToGroup
 */
HX_RESULT CQTTrackManager::AddStreamToGroup(UINT16 uStreamNumber,
					    UINT16 uAltGroupId,
					    ULONG32 ulBandwidth,
					    QTTrackGroupType eGroupType)
{
    HX_RESULT retVal = HXR_FAIL;

    HX_ASSERT(m_pStreamToTrackMap);
    HX_ASSERT(uStreamNumber < m_uNumStreams);

    if (uStreamNumber < m_uNumStreams)
    {
	m_pStreamToTrackMap[uStreamNumber].m_uAltGroupId = uAltGroupId;
	m_pStreamToTrackMap[uStreamNumber].m_ulBandwidth = ulBandwidth;
	m_pStreamToTrackMap[uStreamNumber].m_eGroupType = eGroupType;
	m_pStreamToTrackMap[uStreamNumber].m_ulStreamSelectionMask |= QT_TSEL_BANDWIDTH;

	retVal = HXR_OK;
    }

    return retVal;
}

/****************************************************************************
 *  SubscribeDefault
 */
HX_RESULT CQTTrackManager::SubscribeDefault(void)
{
    UINT16 uStrmIdx;
    HX_RESULT retVal = HXR_OUTOFMEMORY;
    HXBOOL* pVisitedArray = new HXBOOL [m_uNumStreams];

    HX_ASSERT(m_pStreamToTrackMap);
    HX_ASSERT(pVisitedArray);

    if (pVisitedArray)
    {
	for (uStrmIdx = 0; uStrmIdx < m_uNumStreams; uStrmIdx++)
	{
	    pVisitedArray[uStrmIdx] = FALSE;
	}

	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	for (uStrmIdx = 0; uStrmIdx < m_uNumStreams; uStrmIdx++)
	{
	    if (!pVisitedArray[uStrmIdx])
	    {
		if (m_pStreamToTrackMap[uStrmIdx].m_eGroupType != QTGROUP_NONE)
		{
		    UINT16 uSubIdx;
		    UINT16 uAltGroupId = m_pStreamToTrackMap[uStrmIdx].m_uAltGroupId;
		    UINT16 uSelectIdx = uStrmIdx;

		    // Activate the highest bitrate stream in the group matching all other
		    // criteria.
		    for (uSubIdx = uStrmIdx ; uSubIdx < m_uNumStreams; uSubIdx++)
		    {
			if (m_pStreamToTrackMap[uSubIdx].m_uAltGroupId == uAltGroupId)
			{
			    HXBOOL bSelect = TRUE;

			    m_pStreamToTrackMap[uSubIdx].m_bActive = FALSE;

			    // Check if stream can be selected based on bandwidth criteria
			    if (m_pStreamToTrackMap[uSubIdx].m_ulStreamSelectionMask & QT_TSEL_BANDWIDTH)
			    {
				if (m_pStreamToTrackMap[uSubIdx].m_ulBandwidth > 
				    m_pStreamToTrackMap[uSelectIdx].m_ulBandwidth)
				{
				    uSelectIdx = uSubIdx;
				}
				else
				{
				    bSelect = FALSE;
				}
			    }

			    // Check if stream can be selected based on language criteria
			    /* TODO:
				if (bSelect &&
				    (m_pStreamToTrackMap[uSubIdx].m_ulStreamSelectionMask & QT_TSEL_LANGUAGE))
				{
				    // Check if language meets criteria
				}
			     */

			    pVisitedArray[uSubIdx] = TRUE;
			}
		    }

		    m_pStreamToTrackMap[uSelectIdx].m_bActive = TRUE;
		    m_pStreamToTrackMap[uSelectIdx].m_pIQTTrack->SubscribeDefault();
		}
		else
		{
		    m_pStreamToTrackMap[uStrmIdx].m_bActive = TRUE;
		    m_pStreamToTrackMap[uStrmIdx].m_pIQTTrack->SubscribeDefault();
		}

		pVisitedArray[uStrmIdx] = TRUE;
	    }
	}
    }

    HX_VECTOR_DELETE(pVisitedArray);

    return retVal;
}

/****************************************************************************
 *  Subscribe
 */
HX_RESULT CQTTrackManager::Subscribe(UINT16 uStreamNum, UINT16 uRuleNumber)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    HX_ASSERT(uStreamNum < m_uNumStreams);

    if (uStreamNum < m_uNumStreams)
    {
	HX_ASSERT(m_pStreamToTrackMap);

	if (m_bInitialSubscriptionWindowClosed)
	{
	    // After initial subscription window closure, 
	    // subscriptions only on active stream are allowed.
	    retVal = HXR_FAIL;

	    if (m_pStreamToTrackMap[uStreamNum].m_bActive)
	    {
		HX_ASSERT(m_pStreamToTrackMap[uStreamNum].m_pIQTTrack);

		retVal = m_pStreamToTrackMap[uStreamNum].m_pIQTTrack->Subscribe(uRuleNumber);
	    }
	}
	else
	{
	    // While initial subscription windows is open, subscribed inactive
	    // streams will be made active unless this violates mutual exclusivness
	    // with another already active stream.
	    // Streams belonging to the same switch group are mutually exclusive and
	    // thus cannot be subscribed (active) at the same time.
	    // The reason for this mutual esclusivness is architectural - they share
	    // the track objects and thus using them simultanously cannot function
	    // correctly.
	    // The reason for not enforcing mutual exclusion between alternate streams
	    // (member of same alternate group) that are not members of same switch
	    // group is to allow full simultaneous access to all content-different 
	    // streams and thus allow file transformation or re-writing.
	    retVal = HXR_OK;

	    if (m_pStreamToTrackMap[uStreamNum].m_eGroupType != QTGROUP_NONE)
	    {
		UINT32 ulSwitchGroupId = m_pStreamToTrackMap[uStreamNum].m_ulSwitchGroupId;

		// If subscribed stream is member of a switch group, make sure its
		// subscription does not conflict with already subscribed stream within
		// the same switch group.
		if (ulSwitchGroupId != 0)
		{
		    UINT16 uStrmIdx;
		    UINT16 uAltGroupId = m_pStreamToTrackMap[uStreamNum].m_uAltGroupId;
	
		    // There can be only one stream subscribed in a switch group
		    for (uStrmIdx = 0; 
			 SUCCEEDED(retVal) && (uStrmIdx < m_uNumStreams); 
			 uStrmIdx++)
		    {
			if ((m_pStreamToTrackMap[uStrmIdx].m_uAltGroupId == uAltGroupId) &&
			    (m_pStreamToTrackMap[uStrmIdx].m_ulSwitchGroupId == ulSwitchGroupId) &&
			    (uStrmIdx != uStreamNum) &&
			    m_pStreamToTrackMap[uStrmIdx].m_bActive)
			{
			    retVal = HXR_FAIL;
			}
		    }
		}
	    }

	    if (SUCCEEDED(retVal))
	    {
		retVal = m_pStreamToTrackMap[uStreamNum].m_pIQTTrack->Subscribe(uRuleNumber);
	    }

	    if (SUCCEEDED(retVal))
	    {
		m_pStreamToTrackMap[uStreamNum].m_bActive = TRUE;
	    }
	}
    }

    return retVal;
}

/****************************************************************************
 *  Unsubscribe
 */
HX_RESULT CQTTrackManager::Unsubscribe(UINT16 uStreamNum, UINT16 uRuleNumber)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    HX_ASSERT(uStreamNum < m_uNumStreams);

    if (uStreamNum < m_uNumStreams)
    {
	if (m_bInitialSubscriptionWindowClosed)
	{
	    retVal = HXR_OK;

	    if (m_pStreamToTrackMap[uStreamNum].m_pIQTTrack)
	    {
		retVal = m_pStreamToTrackMap[uStreamNum].m_pIQTTrack->Unsubscribe(uRuleNumber);
	    }
	}
	else
	{
	    HX_ASSERT(m_pStreamToTrackMap[uStreamNum].m_pIQTTrack);

	    if (m_pStreamToTrackMap[uStreamNum].m_pIQTTrack)
	    {
		retVal = m_pStreamToTrackMap[uStreamNum].m_pIQTTrack->Unsubscribe(uRuleNumber);

		m_pStreamToTrackMap[uStreamNum].m_bActive = 
		    m_pStreamToTrackMap[uStreamNum].m_pIQTTrack->IsSubscribed();
	    }
	}
    }

    return retVal;
}

/****************************************************************************
 *  SequentializeStreamGroupNumbers
 */
HX_RESULT CQTTrackManager::SequentializeStreamGroupNumbers(void)
{
    UINT16 uStrmIdx;
    UINT16 uSubIdx;
    UINT16 uStreamGroupNumber = 0;
    HXBOOL* pVisitedArray = new HXBOOL [m_uNumStreams];
    HX_RESULT retVal = HXR_OUTOFMEMORY;

    HX_ASSERT(pVisitedArray);

    if (pVisitedArray)
    {
	memset(pVisitedArray, 0, sizeof(HXBOOL) * m_uNumStreams);
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	for (uStrmIdx = 0; uStrmIdx < m_uNumStreams; uStrmIdx++)
	{
	    if (!pVisitedArray[uStrmIdx])
	    {
		UINT16 uOldStreamGroupNumber = 
		    m_pStreamToTrackMap[uStrmIdx].m_uStreamGroupNumber;

		for (uSubIdx = uStrmIdx; uSubIdx < m_uNumStreams; uSubIdx++)
		{
		    if (uOldStreamGroupNumber == 
			m_pStreamToTrackMap[uSubIdx].m_uStreamGroupNumber)
		    {
			m_pStreamToTrackMap[uSubIdx].m_uStreamGroupNumber = uStreamGroupNumber;
			pVisitedArray[uStrmIdx] = TRUE;
		    }
		}

		uStreamGroupNumber++;
	    }
	}
    }

    HX_VECTOR_DELETE(pVisitedArray);

    return retVal;
}

/****************************************************************************
 *  AreStreamGroupsPresent
 */
HXBOOL CQTTrackManager::AreStreamGroupsPresent(void)
{
    return m_uNumStreamGroups < m_uNumStreams ? TRUE : FALSE;
}

#else	// QTCONFIG_ALTERNATE_STREAMS

HX_RESULT CQTTrackManager::InitStreamGroups(CQTFileFormat *pFileFormat,
					    CQTPacketAssembler* pPacketAssembler)
{
    return HXR_NOTIMPL;
}

HX_RESULT CQTTrackManager::AddStreamToGroup(UINT16 uStreamNumber,
					    UINT16 uAltGroupId,
					    ULONG32 ulBandwidth,
					    QTTrackGroupType eGroupType)
{
    return HXR_NOTIMPL;
}

HX_RESULT CQTTrackManager::SubscribeDefault(void)
{
    return HXR_NOTIMPL;
}

HX_RESULT CQTTrackManager::Subscribe(UINT16 uStreamNum, UINT16 uRuleNumber)
{
    return HXR_NOTIMPL;
}

HX_RESULT CQTTrackManager::Unsubscribe(UINT16 uStreamNum, UINT16 uRuleNumber)
{
    return HXR_NOTIMPL;
}

HX_RESULT CQTTrackManager::SequentializeStreamGroupNumbers(void)
{
    return HXR_NOTIMPL;
}

HXBOOL CQTTrackManager::AreStreamGroupsPresent(void)
{
    return FALSE;
}

#endif	// QTCONFIG_ALTERNATE_STREAMS

/****************************************************************************
 *  RemoveInactiveStreams
 */
HX_RESULT CQTTrackManager::RemoveInactiveStreams(HXBOOL bLeaveReferences)
{
    HX_RESULT retVal;

    HX_ASSERT(m_pStreamToTrackMap);

    UINT16 uActiveStreamCount = GetNumActiveStreams();

    retVal = (uActiveStreamCount == 0) ? HXR_FAIL : HXR_OK;

    if (SUCCEEDED(retVal) &&
	(uActiveStreamCount != m_uNumStreams))
    {
	UINT16 uStrmIdx;
	UINT16 uNewStrmIdx = 0;
	
	for (uStrmIdx = 0; uStrmIdx < m_uNumStreams; uStrmIdx++)
	{	    
	    if (m_pStreamToTrackMap[uStrmIdx].m_bActive)
	    {
		if (!bLeaveReferences)
		{
		    HX_ASSERT(uNewStrmIdx < uActiveStreamCount);
		    
		    if (uNewStrmIdx != uStrmIdx)
		    {
			HX_ASSERT(uNewStrmIdx < uStrmIdx);
			
			m_pStreamToTrackMap[uNewStrmIdx] =
			    m_pStreamToTrackMap[uStrmIdx];
			m_pStreamToTrackMap[uStrmIdx].Detach();
		    }
		    uNewStrmIdx++;
		}
	    }
	    else
	    {
		ReleaseStreamTrack(m_pStreamToTrackMap[uStrmIdx].m_pIQTTrack);
	    }
	}
	
	if (!bLeaveReferences)
	{
	    m_uNumStreams = uActiveStreamCount;
	    retVal = SequentializeStreamGroupNumbers();
	}
    }

    m_bInitialSubscriptionWindowClosed = 
	(m_bInitialSubscriptionWindowClosed || bLeaveReferences);

    return retVal;
}

/****************************************************************************
 *  IsStreamTrack
 */
HXBOOL CQTTrackManager::IsStreamTrack(IQTTrack* pTrack)
{
    UINT16 uStreamCount = 0;
    CQTStream* pStreamTracer = m_pStreamToTrackMap;

    HX_ASSERT(pTrack);

    while (uStreamCount < m_uNumStreams)
    {
	if (pTrack == pStreamTracer->m_pIQTTrack)
	{
	    return TRUE;
	}

	pStreamTracer++;
	uStreamCount++;
    }

    return FALSE;
}

/****************************************************************************
 *  GetTrackStreamNum
 */
UINT16 CQTTrackManager::GetTrackStreamNum(IQTTrack* pIQTTrack)
{
    UINT16 uStreamCount = 0;
    CQTStream* pStreamTracer = m_pStreamToTrackMap;

    HX_ASSERT(pIQTTrack);

    while (uStreamCount < m_uNumStreams)
    {
	if (pIQTTrack == pStreamTracer->m_pIQTTrack)
	{
	    return uStreamCount;
	}

	pStreamTracer++;
	uStreamCount++;
    }

    return INV_STREAM_NUM;
}

/****************************************************************************
 *  GetTrackById
 */
CQTTrack* CQTTrackManager::GetTrackById(ULONG32 ulTrackID)
{
    UINT16 uTrackCount = 0;
    CQTTrackTable* pTableTracer = m_pTrackTable;

    while (uTrackCount < m_uNumTracks)
    {
	if (pTableTracer->m_pTrack && (ulTrackID == pTableTracer->m_pTrack->GetID()))
	{
	    return pTableTracer->m_pTrack;
	}

	uTrackCount++;
	pTableTracer++;
    }

    return NULL;
}

/****************************************************************************
 *  GetAtomById
 */
CQTAtom* CQTTrackManager::GetTrackAtomById(ULONG32 ulTrackID)
{
    UINT16 uTrackCount = 0;
    CQTTrackTable* pTableTracer = m_pTrackTable;

    while (uTrackCount < m_uNumTracks)
    {
	if (ulTrackID == pTableTracer->m_pTrack->GetID())
	{
	    return pTableTracer->m_pTrackAtom;
	}

	uTrackCount++;
	pTableTracer++;
    }

    return NULL;
}

/****************************************************************************
 *  GetStreamNumberByTrackId
 */
UINT16 CQTTrackManager::GetStreamNumberByTrackId(ULONG32 ulTrackId)
{
    UINT16 uStreamIdx;

    for (uStreamIdx = 0; uStreamIdx < m_uNumStreams; uStreamIdx++)
    {
	if (GetStreamTrack(uStreamIdx) &&
	    (GetStreamTrack(uStreamIdx)->GetID() == ulTrackId))
	{
	    return uStreamIdx;
	}
    }

    return INV_STREAM_NUM;
}

/****************************************************************************
 *  GetMajorBrand
 */
HX_RESULT CQTTrackManager::GetMajorBrand(UINT32* pulMajorBrand)
{
    HX_RESULT res = HXR_FAIL;

    if (!pulMajorBrand)
    {
	HX_ASSERT(FALSE);
	return HXR_POINTER;
    }

    if (m_pFtypAtom)
    {
	*pulMajorBrand = m_pFtypAtom->Get_MajorBrand();
	res = HXR_OK;
    }

    return res;
}

/****************************************************************************
 *  GetTrackAtomHdlr
 */
CQT_hdlr_Atom* CQTTrackManager::GetTrackAtomHdlr(CQT_trak_Atom* pTrakAtom)
{
    CQT_mdia_Atom* pMdiaAtom;
    CQT_hdlr_Atom* pHdlrAtom = NULL;

    pMdiaAtom = (CQT_mdia_Atom*) pTrakAtom->FindPresentChild(QT_mdia);
    if (pMdiaAtom)
    {
	pHdlrAtom = (CQT_hdlr_Atom*) pMdiaAtom->FindPresentChild(QT_hdlr);
    }

    return pHdlrAtom;
}

/****************************************************************************
 *  GetTrackAtomStbl
 */
CQT_stbl_Atom* CQTTrackManager::GetTrackAtomStbl(CQT_trak_Atom* pTrakAtom)
{
    CQT_mdia_Atom* pMdiaAtom;
    CQT_hdlr_Atom* pMinfAtom;
    CQT_stbl_Atom* pStblAtom = NULL;

    pMdiaAtom = (CQT_mdia_Atom*) pTrakAtom->FindPresentChild(QT_mdia);
    if (pMdiaAtom)
    {
	pMinfAtom = (CQT_hdlr_Atom*) pMdiaAtom->FindPresentChild(QT_minf);
	if (pMinfAtom)
	{
	    pStblAtom = (CQT_stbl_Atom*) pMinfAtom->FindPresentChild(QT_stbl);
	}
    }

    return pStblAtom;
}

/****************************************************************************
 *  IsHintTrackAtom
 */
HXBOOL CQTTrackManager::IsHintTrackAtom(CQT_trak_Atom* pTrakAtom)
{
    CQT_hdlr_Atom* pHdlrAtom = GetTrackAtomHdlr(pTrakAtom);

    if (pHdlrAtom &&
	/*** MBO: To strict for some files
	(pHdlrAtom->Get_CompType() == QT_mhlr) &&
	***/
	(pHdlrAtom->Get_CompSubtype() == QT_hint))
    {
	return TRUE;
    }

    return FALSE;
}

/****************************************************************************
 *  IsNonEmptyTrackAtom
 */
HXBOOL CQTTrackManager::IsNonEmptyTrackAtom(CQT_trak_Atom* pTrakAtom)
{
    CQT_SampleSize_Manager sampleSize;
    CQT_stbl_Atom* pStblAtom = GetTrackAtomStbl(pTrakAtom);

    if (pStblAtom)
    {
	if (sampleSize.Init(pStblAtom) == HXR_OK)
	{
	    return TRUE;
	}
    }

    return FALSE;
}


/****************************************************************************
 *  Private Methods
 */
/****************************************************************************
 *  AddTracks
 */
void CQTTrackManager::AddTracks(CQTAtom *pRootAtom)
{
    UINT16 uChildCount;

    if (pRootAtom->GetType() == QT_trak)
    {
	HXBOOL bAdd = TRUE;

	if (IsHintTrackAtom((CQT_trak_Atom *) pRootAtom))
	{
	    bAdd = IsNonEmptyTrackAtom((CQT_trak_Atom*) pRootAtom);

	    if (bAdd)
	    {
		m_pHintTrackIdxList->AddTail(
		    (void *) m_pTrackAtomList->GetCount());
	    }
	}

	if (bAdd)
	{
	    m_pTrackAtomList->AddTail(pRootAtom);
	    pRootAtom->AddRef();
	}
    }
    else if (((pRootAtom->GetType() == QT_HXROOT) ||
	      (pRootAtom->GetType() == QT_moov)) &&
	     (uChildCount = pRootAtom->GetPresentChildCount()))
    {
	CQTAtom::ChildIterator i = pRootAtom->BeginChildren();

	do
	{
	    AddTracks(*i);
	    if ((--uChildCount) == 0)
	    {
		break;
	    }

	    ++i;
	} while (TRUE);
    }
    else if (pRootAtom->GetType() == QT_iods)
    {
	HX_ASSERT(m_pIodsAtom == NULL);
	if (m_pIodsAtom == NULL)
	{
	    m_pIodsAtom = (CQT_iods_Atom*) pRootAtom;
	    m_pIodsAtom->AddRef();
	    m_FType = QT_FTYPE_MP4;
	}
    }
    else if (pRootAtom->GetType() == QT_ftyp)
    {
	HX_ASSERT(m_pFtypAtom == NULL);
	if (m_pFtypAtom == NULL)
	{
	    UINT32 ulBrand;
	    HXBOOL bKeepLooking;
	    UINT32 ulCompBrandIdx = 0;
	    UINT32 ulNumCompBrands = 0;
	    m_pFtypAtom = (CQT_ftyp_Atom*) pRootAtom;
	    m_pFtypAtom->AddRef();

	    ulBrand = m_pFtypAtom->Get_MajorBrand();
	    ulNumCompBrands = m_pFtypAtom->Get_NumCompatibleBrands();

	    do
	    {
		bKeepLooking = FALSE;

		switch (ulBrand)
		{
		case QT_isom:
		case QT_3gp4:
		case QT_3gp5:
		case QT_mp41:
		case QT_mp42:
		case QT_mmp4:
		case QT_m4a:
		case QT_qcelp:
		case QT_3gg6:
		case QT_3gp6:
		case QT_3gr6:
		case QT_3gs6:
		    m_FType = QT_FTYPE_MP4;
		    break;

		default:
		    if (ulCompBrandIdx < ulNumCompBrands)
		    {
			ulBrand = m_pFtypAtom->Get_CompatibleBrand(ulCompBrandIdx);
		    	ulCompBrandIdx++;
			bKeepLooking = TRUE;
		    }
		    break;
		}
	    } while (bKeepLooking);
	}
    }
}

/****************************************************************************
 *  DeleteTrackAtomList
 */
void CQTTrackManager::DeleteTrackAtomList(void)
{
    if (m_pTrackAtomList)
    {
	CQTAtom *pAtom;

	while (!m_pTrackAtomList->IsEmpty())
	{
	    pAtom = (CQTAtom*) m_pTrackAtomList->RemoveHead();
	    HX_ASSERT(pAtom);
	    pAtom->Release();
	}

	delete m_pTrackAtomList;
	m_pTrackAtomList = NULL;
    }
}

/****************************************************************************
 *  Clear
 */
void CQTTrackManager::Clear(void)
{
    HX_VECTOR_DELETE(m_pTrackTable);
    HX_VECTOR_DELETE(m_pStreamToTrackMap);

    DeleteTrackAtomList();
    HX_DELETE(m_pHintTrackIdxList);

    HX_RELEASE(m_pIodsAtom);
    HX_RELEASE(m_pFtypAtom);

    m_uNumTracks = 0;
    m_uNumStreams = 0;
    m_uNumStreamGroups = 0;
}


/****************************************************************************
 *  GetNumActiveStreams
 */
UINT16 CQTTrackManager::GetNumActiveStreams(void)
{
    UINT16 uStrmIdx;
    UINT16 uActiveCount = 0;

    HX_ASSERT(m_pStreamToTrackMap);

    for (uStrmIdx = 0; uStrmIdx < m_uNumStreams; uStrmIdx++)
    {
	if (m_pStreamToTrackMap[uStrmIdx].m_bActive)
	{
	    uActiveCount++;
	}
    }

    return uActiveCount;
}


/****************************************************************************
 *  ReleaseStreamTrack
 */
void CQTTrackManager::ReleaseStreamTrack(IQTTrack* pIQTTrack)
{
    // Remove Track from the Track Table
    if (m_pTrackTable)
    {
	UINT16 uTrackCount = 0;
	CQTTrackTable* pTableTracer = m_pTrackTable;
	IQTTrack* pIQTTableTrack;

	while (uTrackCount < m_uNumTracks)
	{
	    pIQTTableTrack = pTableTracer->m_pTrack;
	    if (pIQTTableTrack == pIQTTrack)
	    {
		pTableTracer->Clear();
		break;
	    }

	    uTrackCount++;
	    pTableTracer++;
	}
    }

    // Remove The track from the Stream Map (if there)
    if (m_pStreamToTrackMap)
    {
	UINT16 uStreamCount = 0;
	CQTStream* pStreamTracer = m_pStreamToTrackMap;

	while (uStreamCount < m_uNumStreams)
	{
	    if (pStreamTracer->m_pIQTTrack == pIQTTrack)
	    {
		pStreamTracer->Clear();
		break;
	    }

	    uStreamCount++;
	    pStreamTracer++;
	}
    }
}



/****************************************************************************
 *  CQTTrackManager::CQTTrackTable
 */
/****************************************************************************
 *  Clear
 */
void CQTTrackManager::CQTTrackTable::Clear(void)
{
    if (m_pTrack)
    {
	m_pTrack->Close();
	m_pTrack->Release();
	m_pTrack = NULL;
    }
    HX_RELEASE(m_pTrackAtom);
    m_uRefCount = 0;
}

/****************************************************************************
 *  Destructor
 */
CQTTrackManager::CQTTrackTable::~CQTTrackTable()
{
    Clear();
}


/****************************************************************************
 *  CQTTrackManager::CQTStream
 */
/****************************************************************************
 *  Clear
 */
void CQTTrackManager::CQTStream::Clear(void)
{
    HX_RELEASE(m_pIQTTrack);
    if (m_pQTSwitchTrack)
    {
	m_pQTSwitchTrack->Close();
	m_pQTSwitchTrack->Release();
	m_pQTSwitchTrack = NULL;
    }
    m_bActive = FALSE;
    m_eGroupType = QTGROUP_NONE;
    m_uAltGroupId = 0;
    m_uStreamGroupNumber = 0;
    m_ulSwitchGroupId = 0;
    m_ulStreamSelectionMask = 0;
    m_ulBandwidth = 0;
    HX_RELEASE(m_pLanguage);
    m_bDefaultAlternate = FALSE;
}

void CQTTrackManager::CQTStream::Detach(void)
{
    m_pIQTTrack = NULL;
    m_pQTSwitchTrack = NULL;
    m_pLanguage = NULL;
}

/****************************************************************************
 *  Destructor
 */
CQTTrackManager::CQTStream::~CQTStream()
{
    Clear();
}

