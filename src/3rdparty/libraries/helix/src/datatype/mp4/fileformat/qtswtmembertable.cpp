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
#define MAX_PER_TRACK_RULEBOOK_SIZE 240


/****************************************************************************
 *  Includes
 */
#include "qtswtmembertable.h"
#include "qttrack.h"

#include "safestring.h"


/****************************************************************************
 *  CQTSwitchTrackMemberTable
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CQTSwitchTrackMemberTable::CQTSwitchTrackMemberTable(UINT16 uNumTracks,
						     CQTTrack* pQTTracks[])
    : m_pTrackTable(NULL)
    , m_uNumTrackTableEntries(0)
    , m_ulTrackSelectionMask(0)
    , m_pASMRuleBook(NULL)
    , m_bInitialized(FALSE)
    , m_lRefCount(0)
{
    if (uNumTracks > 0)
    {
	m_pTrackTable = new CTrackEntry[uNumTracks];
	if (m_pTrackTable)
	{
	    UINT16 uIdx = 0;

	    m_uNumTrackTableEntries = uNumTracks;

	    do
	    {
		m_pTrackTable[uIdx].m_pQTTrack = pQTTracks[uIdx];
		m_pTrackTable[uIdx].m_pQTTrack->AddRef();

		uIdx++;
	    } while (uIdx < uNumTracks);
	}
    }
}

CQTSwitchTrackMemberTable::~CQTSwitchTrackMemberTable()
{
    HX_RELEASE(m_pASMRuleBook);
    HX_VECTOR_DELETE(m_pTrackTable);
}


/****************************************************************************
 *  Main Interface
 */
/****************************************************************************
 *  Init
 */
HX_RESULT CQTSwitchTrackMemberTable::Init(UINT32 ulTrackSelectionMask,
					  IHXCommonClassFactory* pClassFactory)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pTrackTable)
    {
	retVal = HXR_OK;
    }

    // Only one successful init allowed
    if (m_bInitialized)
    {
	return retVal;
    }

    // Obtain all information relevant for switch selection criteria
    if (SUCCEEDED(retVal))
    {
	m_ulTrackSelectionMask = ulTrackSelectionMask;

	UINT16 uIdx = 0;

	do
	{
	    if (m_ulTrackSelectionMask & QT_TSEL_BANDWIDTH)
	    {
		retVal = m_pTrackTable[uIdx].m_pQTTrack->ObtainTrackBandwidth(
		    m_pTrackTable[uIdx].m_ulBandwidth);
	    }

	    uIdx++;
	} while (SUCCEEDED(retVal) && (uIdx < m_uNumTrackTableEntries));
    }

    if (SUCCEEDED(retVal))
    {
	retVal = SortTrackTable();
    }

    if (SUCCEEDED(retVal))
    {
	retVal = EstablishASMRuleBook(pClassFactory);
    }

    m_bInitialized = SUCCEEDED(retVal);

    return retVal;
}

/****************************************************************************
 *  GetMemeberId
 */
UINT16 CQTSwitchTrackMemberTable::GetMemberId(CQTTrack* pQTTrack)
{
    UINT16 uIdx = 0;

    HX_ASSERT(m_uNumTrackTableEntries > 0);

    do
    {
	if (m_pTrackTable[uIdx].m_pQTTrack == pQTTrack)
	{
	    return uIdx;
	}

	uIdx++;
    } while (uIdx < m_uNumTrackTableEntries);

    return QTSWT_BAD_TRACK_ID;
}


/****************************************************************************
 *  Protected utility methods
 */
/****************************************************************************
 *  SortTrackTable
 */
HX_RESULT CQTSwitchTrackMemberTable::SortTrackTable(void)
{
    HX_RESULT retVal = HXR_OK;

    if (m_ulTrackSelectionMask & QT_TSEL_BANDWIDTH)
    {
	// Sort by bandwidth
	UINT16 uIdx1;
	UINT16 uIdx2;
	CTrackEntry tempEntry;

	for (uIdx1 = 0; uIdx1 < m_uNumTrackTableEntries; uIdx1++)
	{
	    for (uIdx2 = uIdx1 + 1; uIdx2 < m_uNumTrackTableEntries; uIdx2++)
	    {
		if (m_pTrackTable[uIdx1].m_ulBandwidth > 
		    m_pTrackTable[uIdx2].m_ulBandwidth)
		{
		    tempEntry = m_pTrackTable[uIdx1];
		    m_pTrackTable[uIdx1] = m_pTrackTable[uIdx2];
		    m_pTrackTable[uIdx2] = tempEntry;
		}
	    }
	}

	tempEntry.Detach();
    }

    return retVal;
}

/****************************************************************************
 *  EstablishASMRuleBook
 */
HX_RESULT CQTSwitchTrackMemberTable::EstablishASMRuleBook(IHXCommonClassFactory* pClassFactory)
{
    IHXBuffer* pASMRuleBook = NULL;
    HX_RESULT retVal = HXR_FAIL;

    HX_RELEASE(m_pASMRuleBook);

    if (m_ulTrackSelectionMask & QT_TSEL_BANDWIDTH)
    {
	UINT32 ulMaxBookSize = MAX_PER_TRACK_RULEBOOK_SIZE * m_uNumTrackTableEntries;
	char* pRuleBook = new char [ulMaxBookSize];
	char* pRuleBookWriter = pRuleBook;

	retVal = HXR_OUTOFMEMORY;

	if (pRuleBook)
	{
	    retVal = HXR_OK;
	}

	if (SUCCEEDED(retVal))
	{
	    if (m_uNumTrackTableEntries > 1)
	    {
		UINT16 uIdx = 0;
		UINT32 ulBandwidth;
		UINT32 ulBandwidthTo;

		do
		{
		    ulBandwidth = m_pTrackTable[uIdx].m_ulBandwidth;

		    if (uIdx == 0)
		    {
			// first entry
			ulBandwidthTo = m_pTrackTable[uIdx + 1].m_ulBandwidth;

			pRuleBookWriter += SafeSprintf(pRuleBookWriter, 
				    ulMaxBookSize - (pRuleBookWriter - pRuleBook), 
				    "#($Bandwidth < %d),Marker=%d,AverageBandwidth=%d,TimeStampDelivery=TRUE;"
				    "#($Bandwidth < %d),Marker=%d,AverageBandwidth=0,TimeStampDelivery=TRUE;",
				    ulBandwidthTo,
				    (QTASM_MARKER_ON_RULE == 0) ? 1 : 0,
				    ulBandwidth,
				    ulBandwidthTo,
				    (QTASM_MARKER_ON_RULE == 0) ? 0 : 1,
				    ulBandwidth);
		    }
		    else if ((uIdx + 1) < m_uNumTrackTableEntries)
		    {
			// mid entry (not first and not last)
			ulBandwidthTo = m_pTrackTable[uIdx + 1].m_ulBandwidth;

			pRuleBookWriter += SafeSprintf(pRuleBookWriter, 
				    ulMaxBookSize - (pRuleBookWriter - pRuleBook), 
				    "#($Bandwidth >= %d) && ($Bandwidth < %d),Marker=%d,AverageBandwidth=%d,TimeStampDelivery=TRUE;"
				    "#($Bandwidth >= %d) && ($Bandwidth < %d),Marker=%d,AverageBandwidth=0,TimeStampDelivery=TRUE;",
				    ulBandwidth,
				    ulBandwidthTo,
				    (QTASM_MARKER_ON_RULE == 0) ? 1 : 0,
				    ulBandwidth,
				    ulBandwidth,
				    ulBandwidthTo,
				    (QTASM_MARKER_ON_RULE == 0) ? 0 : 1,
				    ulBandwidth);
		    }
		    else
		    {
			// last entry
			pRuleBookWriter += SafeSprintf(pRuleBookWriter, 
				    ulMaxBookSize - (pRuleBookWriter - pRuleBook), 
				    "#($Bandwidth >= %d),Marker=%d,AverageBandwidth=%d,TimeStampDelivery=TRUE;"
				    "#($Bandwidth >= %d),Marker=%d,AverageBandwidth=0,TimeStampDelivery=TRUE;",
				    ulBandwidth,
				    (QTASM_MARKER_ON_RULE == 0) ? 1 : 0,
				    ulBandwidth,
				    ulBandwidth,
				    (QTASM_MARKER_ON_RULE == 0) ? 0 : 1,
				    ulBandwidth);
		    }

		    m_pTrackTable[uIdx].m_pQTTrack->SetBaseRuleNumber(
			MapMemberIdToBaseRule(uIdx));

		    uIdx++;
		} while (uIdx < m_uNumTrackTableEntries);
	    }
	    else
	    {
		pRuleBookWriter += SafeSprintf(pRuleBookWriter, 
			    ulMaxBookSize - (pRuleBookWriter - pRuleBook), 
			    "Marker=%d,AverageBandwidth=%d,TimeStampDelivery=TRUE;"
			    "Marker=%d,AverageBandwidth=0,TimeStampDelivery=TRUE;",
			    (QTASM_MARKER_ON_RULE == 0) ? 1 : 0,
			    m_pTrackTable[0].m_ulBandwidth,
			    (QTASM_MARKER_ON_RULE == 0) ? 0 : 1);
	    }
	}

	if (SUCCEEDED(retVal))
	{
	    retVal = pClassFactory->CreateInstance(CLSID_IHXBuffer,
						   (void**) &pASMRuleBook);
	}

	if (SUCCEEDED(retVal))
	{
	    UINT32 ulRuleBookSize = pRuleBookWriter - pRuleBook + 1;
	    retVal = pASMRuleBook->Set((UCHAR*) pRuleBook, ulRuleBookSize);
	}

	if (SUCCEEDED(retVal))
	{
	    m_pASMRuleBook = pASMRuleBook;
	    pASMRuleBook = NULL;
	}

	HX_RELEASE(pASMRuleBook);
	HX_VECTOR_DELETE(pRuleBook);
    }

    return retVal;
}


/****************************************************************************
 *  IUnknown methods
 */
/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//
STDMETHODIMP CQTSwitchTrackMemberTable::QueryInterface(REFIID riid, void** ppvObj)
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
STDMETHODIMP_(ULONG32) CQTSwitchTrackMemberTable::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//
STDMETHODIMP_(ULONG32) CQTSwitchTrackMemberTable::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}
