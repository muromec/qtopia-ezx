/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxtac.cpp,v 1.17 2007/07/06 21:58:11 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * terms of the GNU General Public License Version 2 (the
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxcomm.h"
#include "hxausvc.h"
#include "ihxpckts.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxmap.h"
#include "hxgroup.h"
#include "basgroup.h"
#include "advgroup.h"
#include "hxplay.h"
#include "hxstrutl.h"
#include "pckunpck.h"

#include "hxtac.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

// note: these constants much match the order of szTACNames
#define TitlePosition	   0
#define AuthorPosition	   1
#define CopyrightPosition  2
#define AbstractPosition   3
#define KeywordsPosition   4
#define DescriptionPosition 5

static const char* const szTACNames[] =
{
    "Title",
    "Author",
    "Copyright",
    "Abstract",
    "Keywords",
    "Description"
};


// init static data
UINT32 const TACData::NoFind = 9;

/////////////////////////////////////////////////////////////////////////
//      Method:
//              HXMasterTAC::HXMasterTAC
//      Purpose:
//              Constructor
//
HXMasterTAC::HXMasterTAC(IUnknown* pContext, HXBasicGroupManager* pGroupManager) :
      m_tacStatus(TAC_Pending)
    , m_pTACPropWatch(NULL)
    , m_pTACProps(NULL)
    , m_lRefCount(0)
#if defined(HELIX_FEATURE_REGISTRY)
    , m_pRegistry(NULL)
#endif /* HELIX_FEATURE_REGISTRY */
    , m_ptacPropIDs(NULL)
    , m_pContext(NULL)
{
    m_pGroupManager = pGroupManager;
    HX_ADDREF(m_pGroupManager);

    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    CreateValuesCCF(m_pTACProps, m_pContext);

    // init master tac prop ID array
    for (int n=0; n<NUMB_TAC_NAMES; n++)
    {
	m_masterTACPropIDs[n] = 0;
    }
}


/////////////////////////////////////////////////////////////////////////
//      Method:
//              HXMasterTAC::~HXMasterTAC
//      Purpose:
//              Destructor
//
HXMasterTAC::~HXMasterTAC()
{
    HX_RELEASE(m_pTACProps);
#if defined(HELIX_FEATURE_REGISTRY)
    if (m_pRegistry)
    {
	for (int n=0; n<NUMB_TAC_NAMES; n++)
	{
	    if (m_masterTACPropIDs[n])
	    {
		m_pRegistry->DeleteById(m_masterTACPropIDs[n]);
	    }
	}
    }
    HX_RELEASE(m_pRegistry);
#endif /* HELIX_FEATURE_REGISTRY */
    HX_DELETE(m_ptacPropIDs);
    HX_RELEASE(m_pContext);
}

void HXMasterTAC::Close()
{
    // clear TAC prop watches and release prop watch object
    if (m_pTACPropWatch)
    {
	ResetTAC();

	HX_RELEASE(m_pTACPropWatch);
    }
    HX_RELEASE(m_pGroupManager);
}

HXBOOL		
HXMasterTAC::IsTACComplete(IHXValues* pProps)
{
    HXBOOL	bResult = TRUE;
    UINT16	nIdx = 0;
    IHXBuffer* pValue = NULL;
    
    if (!pProps)
    {
	bResult = FALSE;
	goto cleanup;
    }

    for(nIdx = 0; nIdx < NUMB_TAC_NAMES - 1; nIdx++)
    {
	if (HXR_OK != pProps->GetPropertyCString(szTACNames[nIdx], pValue) || !pValue)
	{	
	    bResult = FALSE;
	    break;
	}
	HX_RELEASE(pValue);
    }

cleanup:

    HX_RELEASE(pValue);

    return bResult;
}

void HXMasterTAC::SetRegistry(HXClientRegistry* pRegistry, UINT32 playerID)
{
#if defined(HELIX_FEATURE_REGISTRY)
    // save the reg ptr and addref
    m_pRegistry = pRegistry;
    m_pRegistry->AddRef();

    // and create properties in player's space
    char szPropName[1024]; /* Flawfinder: ignore */
    IHXBuffer* pPlayerName = NULL;
    HX_ASSERT(playerID != 0);
    if (HXR_OK == m_pRegistry->GetPropName(playerID, pPlayerName))
    {
	for (int n=0; n<NUMB_TAC_NAMES; n++)
	{
	    // add each master TAC prop
	    SafeSprintf(szPropName, 1024, "%s.%s", (char*) pPlayerName->GetBuffer(), szTACNames[n]);
	    m_masterTACPropIDs[n] = m_pRegistry->AddStr(szPropName, NULL);
	}

	HX_RELEASE(pPlayerName);
    }

    // create prop watch object
    m_pRegistry->CreatePropWatch(m_pTACPropWatch);
    m_pTACPropWatch->Init((IHXPropWatchResponse*) this);
#endif /* HELIX_FEATURE_REGISTRY */
}

/*
 * IUnknown methods
 */

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your
//              object.
//
STDMETHODIMP HXMasterTAC::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPropWatchResponse), (IHXPropWatchResponse*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPropWatchResponse*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::AddRef
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) HXMasterTAC::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::Release
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) HXMasterTAC::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

#if defined(HELIX_FEATURE_REGISTRY)
// IHXPropWatchResponse methods

STDMETHODIMP
HXMasterTAC::AddedProp(const UINT32 id,
			    const HXPropType propType,
			    const UINT32 ulParentHash)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXMasterTAC::ModifiedProp(const UINT32 id,
			       const HXPropType propType,
			       const UINT32 ulParentHash)
{
    if (m_ptacPropIDs)
    {
	// first see which prop this is (0-3) and copy data 
	// to equivilent unfied Player prop
	UINT32 masterID = 0;
	INT32 sourceID;
	void *pTAC;
	TACData* pTACData = NULL;
	IHXBuffer*	pValue = NULL;
	POSITION pos = m_ptacPropIDs->GetStartPosition();
	while (pos)
	{
	    // get next tac data object
	    m_ptacPropIDs->GetNextAssoc(pos, sourceID, pTAC);
	    pTACData = (TACData *) pTAC;

	    // find the equivilent master prop ID
	    masterID = pTACData->FindMasterIndex(id);
	    if (masterID != TACData::NoFind)
	    {
		// get prop from ID 
		m_pRegistry->GetStrById(id, pValue);
		if (pValue)
		{
		    // set new master tac prop
		    m_pRegistry->SetStrById(m_masterTACPropIDs[masterID], pValue);
		    HX_RELEASE(pValue);
		}

		break;
	    }
	}
    }

    return HXR_OK;
}

STDMETHODIMP
HXMasterTAC::DeletedProp(const UINT32 id,
			      const UINT32 ulParentHash)
{
    HRESULT hRes = HXR_OK;

    // clear TAC prop watches and release prop watch object
    if (m_pTACPropWatch)
    {
	if (m_ptacPropIDs)
	{
	    INT32 sourceID;
	    void *pTAC;
	    TACData* pTACData = NULL;
	    POSITION pos = m_ptacPropIDs->GetStartPosition();
	    while (pos)
	    {
		// get next TAC data object 
		m_ptacPropIDs->GetNextAssoc(pos, sourceID, pTAC);
		pTACData = (TACData *) pTAC;

		// see if this ID is part of this TAC data object
		if (pTACData->IsIDPresent(id))
		{
		    pTACData->Clear(id);
		    break;
		}
	    }
	}

	// clear the watch now
	hRes = m_pTACPropWatch->ClearWatchById(id);
    }

    return hRes;
}
#endif /* HELIX_FEATURE_REGISTRY */

/************************************************************************
 *      Method:
 *          HXMasterTAC::ResetTAC
 *      Purpose:
 *          Reset all TAC related members in preparation for next group
 *
 */
void HXMasterTAC::ResetTAC(HXBOOL bResetStatus /*=TRUE*/, HXBOOL bClearMasterProps /*=FALSE*/)
{
    if (bResetStatus)
    {
	m_tacStatus = TAC_Pending;
    }

    INT32 sourceID;
    void *pTAC;
    TACData* pTACData = NULL;
    if (m_ptacPropIDs)
    {
	POSITION pos = m_ptacPropIDs->GetStartPosition();
	while (pos)
	{
	    // get next TAC data object 
	    m_ptacPropIDs->GetNextAssoc(pos, sourceID, pTAC);
	    pTACData = (TACData *) pTAC;

	    // clear all watches set
	    pTACData->ClearAll(m_pTACPropWatch);

	    // free tac data object
	    delete pTACData;
	    pTACData = NULL;
	}

	// remove all list entries 
	m_ptacPropIDs->RemoveAll();
	HX_DELETE(m_ptacPropIDs);
    }

    // clear the master tac props value if this flag is set 
    if (bClearMasterProps)
    {
	HX_RELEASE(m_pTACProps);
	CreateValuesCCF(m_pTACProps, m_pContext);

#if defined(HELIX_FEATURE_REGISTRY)
	// set values in registry
	IHXBuffer* pValue = NULL;
	for (int i=0; i<NUMB_TAC_NAMES; i++)
	{
	    UCHAR nullString[1];
	    *nullString = '\0';

	    // set to blank string
	    if (HXR_OK == CreateAndSetBufferCCF(pValue, nullString, 1, m_pContext))
	    {
		m_pRegistry->SetStrById(m_masterTACPropIDs[i], pValue);
		HX_RELEASE(pValue);
	    }
	}
#endif /* HELIX_FEATURE_REGISTRY */
    }
}

/************************************************************************
 *      Method:
 *          HXMasterTAC::CheckTrackAndSourceOnTrackStarted
 *      Purpose:
 *           Private function used to examine track props for TAC info
 *
 */
HXBOOL HXMasterTAC::CheckTrackAndSourceOnTrackStarted(INT32 nGroup, 
						  INT32 nTrack, 
						  UINT32 sourceID)
{
    HXBOOL res = TRUE;

    // if we have no tac yet, or, if we have a Source TAC set and 
    // in fact the track props have TAC info, try for Track props...
    if (m_tacStatus == TAC_Pending || m_tacStatus == TAC_Source)
    {
	// see if track props have TAC info
	if (!CheckTrackForTACInfo(nGroup, nTrack))
	{
	    // finally, if Track Props have no TAC, check the Source props! 
	    res = CheckSourceForTACInfo(nGroup, nTrack, sourceID);
	}
    }

    return res;
}

/************************************************************************
 *      Method:
 *          HXMasterTAC::CheckGroupForTACInfo
 *      Purpose:
 *          Private function used to examine group props for TAC info
 *
 */
HXBOOL HXMasterTAC::CheckGroupForTACInfo(INT32 nGroup)
{
    HXBOOL bFoundTAC = CheckPresentationForTACInfo();
    if (bFoundTAC)
    {
	return bFoundTAC;
    }
	
    // first get the group object's properties using the IHXGroup
    IHXGroup* pGroup = NULL;
    if (m_pGroupManager && 
	HXR_OK == m_pGroupManager->GetGroup((UINT16) nGroup, pGroup))
    {
	// now get the props
	IHXValues* pGroupProps = pGroup->GetGroupProperties();

	// now search the group props for TAC info
	RetrieveTACProperties(pGroupProps);
	if (m_pTACProps && IsTACComplete(m_pTACProps))
	{
	    SetTAC(m_pTACProps, TAC_Group);

	    bFoundTAC = TRUE;
	}
	else if (pGroupProps) // the group has no TAC props, so check the individidual tracks
	{
	    for (int n=0; !bFoundTAC && n<pGroup->GetTrackCount(); n++)
	    {
		// now get the next track
		IHXValues* pTrack = NULL;
		if (HXR_OK == pGroup->GetTrack((UINT16) n, pTrack))
		{	
		    // now search the props for TAC info
		    RetrieveTACProperties(pTrack);
		    if (m_pTACProps && IsTACComplete(m_pTACProps))
		    {
			// promote track props to group
			IHXBuffer* pValue = NULL;
			UINT16 nIdx;
			for(nIdx = 0; nIdx < NUMB_TAC_NAMES; nIdx++)
			{
			    m_pTACProps->GetPropertyCString(szTACNames[nIdx], pValue);
			    if (pValue)
			    {
	    			pGroupProps->SetPropertyCString(szTACNames[nIdx], pValue);
	    			HX_RELEASE(pValue);
			    }
			}

			// set new tac info 
			SetTAC(m_pTACProps, TAC_Group);

			bFoundTAC = TRUE;
		    }

		    HX_RELEASE(pTrack);
		}
	    }
	}
	
	HX_RELEASE(pGroupProps);
	HX_RELEASE(pGroup);
    }

    return bFoundTAC;
}

/************************************************************************
 *      Method:
 *          HXMasterTAC::CheckPresentationForTACInfo
 *      Purpose:
 *           Private function used to examine presentation props for TAC info
 *
 */
HXBOOL HXMasterTAC::CheckPresentationForTACInfo()
{
    HXBOOL bFoundTAC = FALSE;

    IHXValues* pPresProps = m_pGroupManager ? m_pGroupManager->GetPresentationProperties() : NULL;
    if (pPresProps)
    {
	// now search the group props for TAC info
	RetrieveTACProperties(pPresProps);
	if (m_pTACProps && IsTACComplete(m_pTACProps))
	{
	    SetTAC(m_pTACProps, TAC_Presentation);

	    bFoundTAC = TRUE;
	}
	HX_RELEASE(pPresProps);
    }

    return bFoundTAC;
}

/************************************************************************
 *      Method:
 *          HXMasterTAC::CheckTrackForTACInfo
 *      Purpose:
 *           Private function used to examine track props for TAC info
 *
 */
HXBOOL HXMasterTAC::CheckTrackForTACInfo(INT32 nGroup, INT32 nTrack)
{
    HXBOOL bFoundTAC = CheckPresentationForTACInfo();
    if (bFoundTAC)
    {
	return bFoundTAC;
    }
	
    // first get the group object's properties using the IHXGroup
    IHXGroup* pGroup = NULL;
    if (m_pGroupManager && 
	HXR_OK == m_pGroupManager->GetGroup((UINT16) nGroup, pGroup))
    {
	// but first check the group's props 
	IHXValues* pGroupProps = pGroup->GetGroupProperties();

	// now search the group props for TAC info
	RetrieveTACProperties(pGroupProps);
	if (m_pTACProps && IsTACComplete(m_pTACProps))
	{
	    SetTAC(m_pTACProps, TAC_Group);

	    bFoundTAC = TRUE;
	}
	else
	{
	    // now get the track using track ID
	    IHXValues* pTrack = NULL;
	    if (HXR_OK == pGroup->GetTrack((UINT16) nTrack, pTrack))
	    {
		// now search the props for TAC info
		RetrieveTACProperties(pTrack);
		if (m_pTACProps && IsTACComplete(m_pTACProps))
		{
		    SetTAC(m_pTACProps, TAC_Track);
    
		    bFoundTAC = TRUE;
		}

		HX_RELEASE(pTrack);
	    }
	}

	HX_RELEASE(pGroupProps);
    }
   
    HX_RELEASE(pGroup);

    return bFoundTAC;
}

/************************************************************************
 *      Method:
 *          HXMasterTAC::CheckSourceForTACInfo
 *      Purpose:
 *           Private function used to examine source props for TAC
 *	     info. If found, adds them to the Track props and sets
 *	     the player's TAC props.
 *
 *	      If Track props already had TAC props, this method would
 *	      not be called!
 *
 */
HXBOOL HXMasterTAC::CheckSourceForTACInfo(INT32 nGroup, INT32 nTrack, UINT32 sourceID)
{
    HXBOOL bFoundTAC = FALSE;

    // first get the group object's properties using the IHXGroup
    IHXGroup* pGroup = NULL;
    IHXValues* pTrack = NULL;
    if (m_pGroupManager && 
	HXR_OK == m_pGroupManager->GetGroup((UINT16) nGroup, pGroup))
    {
	pGroup->GetTrack((UINT16) nTrack, pTrack);
    }

#if defined(HELIX_FEATURE_REGISTRY)
    // get source prop name 
    IHXBuffer* pSourceName = NULL;
    if (HXR_OK == m_pRegistry->GetPropName(sourceID, pSourceName))
    {
	if (!m_ptacPropIDs)
	{
	    m_ptacPropIDs = new CHXMapLongToObj;
	    if (!m_ptacPropIDs)
	    {
		return FALSE;
	    }
	}

	// build TAC property strings & get props	
	IHXValues* pTACProps = NULL;
	CreateValuesCCF(pTACProps, m_pContext);

	IHXBuffer* pValue = NULL;
	POSITION pos = m_ptacPropIDs->Lookup (sourceID);

	if (pos == NULL)
	{
	    // create tac data object and add to collection
	    TACData* pTACData = new TACData();
	    m_ptacPropIDs->SetAt(sourceID, pTACData);

	    // get each prop from the registry and add them to the 
	    // pTACProps & pTrack collection also
	    char szPropName[1024]; /* Flawfinder: ignore */
	    for (int n=0; n<NUMB_TAC_NAMES; n++)
	    {
		SafeSprintf(szPropName, 1024, "%s.%s", (char*) pSourceName->GetBuffer(), szTACNames[n]);
		if (HXR_OK == m_pRegistry->GetStrByName(szPropName, pValue) ||
		    m_pRegistry->GetId(szPropName))
		{
		    if (pValue)
		    {
			// now add it to the pTACProps values
			pTACProps->SetPropertyCString(szTACNames[n], pValue);

			// if we have a track 
			if (pTrack)
			{
			    pTrack->SetPropertyCString(szTACNames[n], pValue);
			}

			HX_RELEASE(pValue);

			bFoundTAC = TRUE;
		    }

		    // set prop watch on this property (even if it has no TAC data so far - it may later!)
		    pTACData->SetPropAndWatch(n, m_pRegistry->GetId(szPropName), m_pTACPropWatch);
		}
	    }
	}

	// if we found some (we might not have) set them as the player TAC props
	if (bFoundTAC)
	{
	    RetrieveTACProperties(pTACProps);
	}

	SetTAC(m_pTACProps, TAC_Source);

	// cleanup
	HX_RELEASE(pTACProps);
	HX_RELEASE(pSourceName);
    }
#endif /* HELIX_FEATURE_REGISTRY */

    // clean up track & group - if present
    if (pTrack)
    {
	HX_RELEASE(pTrack);
    }

    if (pGroup)
    {
	HX_RELEASE(pGroup);
    }

    return bFoundTAC;
}

/************************************************************************
 *      Method:
 *          HXMasterTAC::RetrieveTACProperties
 *      Purpose:
 *          Get registry ID(hash_key) of the objects(player, source and stream)
 *
 */
void HXMasterTAC::RetrieveTACProperties(IHXValues* pFromProps)
{
    IHXBuffer*	pValue1 = NULL;
    IHXBuffer*	pValue2 = NULL;
    IHXValues* pResult = NULL;

    if (pFromProps)
    {
	UINT16 nIdx;
	for(nIdx = 0; nIdx < NUMB_TAC_NAMES; nIdx++)
	{
	    pFromProps->GetPropertyCString(szTACNames[nIdx], pValue1);
	    if (pValue1)
	    {
		m_pTACProps->GetPropertyCString(szTACNames[nIdx], pValue2);

		if (!pValue2)
		{
		    m_pTACProps->SetPropertyCString(szTACNames[nIdx], pValue1);
		}

	    	HX_RELEASE(pValue1);
		HX_RELEASE(pValue2);
	    }
	}
    }
}

/************************************************************************
 *      Method:
 *          HXMasterTAC::SetTAC
 *      Purpose:
 *          Set TAC properties in registry using values in tacProps 
 *
 */
void HXMasterTAC::SetTAC(IHXValues* tacProps, TACStatus status)
{
    // ignore new TAC settings unless this setting has a higher "status"
    if (status < m_tacStatus)
    {
	return;
    }

    // if we had Source based TAC before, and now we have a Track with
    // TAC props (ie. from the SMIL) we want that to be it for the group
    // so ignore any new mm sync events - clear all Source prop watches
    if (status == TAC_Track && m_tacStatus == TAC_Source)
    {
	// clear all TAC watches
	ResetTAC();
    }

#if defined(HELIX_FEATURE_REGISTRY)
    // set values in registry
    IHXBuffer*	pValue = NULL;
    for (int i=0; i<NUMB_TAC_NAMES; i++)
    {
	tacProps->GetPropertyCString(szTACNames[i],pValue);    
	if (pValue)
	{
	    m_pRegistry->SetStrById(m_masterTACPropIDs[i], pValue);
	    HX_RELEASE(pValue);
	}
	else
	{
	    UCHAR nullString[1];
	    *nullString = '\0';

	    // set to blank string
	    if (HXR_OK == CreateAndSetBufferCCF(pValue, nullString, 1, m_pContext))
	    {
		m_pRegistry->SetStrById(m_masterTACPropIDs[i], pValue);
		HX_RELEASE(pValue);
	    }
	}
    }
#endif /* HELIX_FEATURE_REGISTRY */

    // set new status
    m_tacStatus = status;
}

/************************************************************************
 *      Method:
 *          TACData::SetPropAndWatch
 *      Purpose:
 *          Set a prop watch on the propID passed and save the ID
 *	    The propIndex value denotes which of the 4 TACA items we are setting
 *
 */
void TACData::SetPropAndWatch(UINT32 propIndex, UINT32 propID, IHXPropWatch* pPropWatch)
{   
    // first save the propID in the appropriate member
    if (propIndex == TitlePosition)
    {
	m_titleID = propID;
    }
    else if (propIndex == AuthorPosition) 
    {
	m_authorID = propID;
    }
    else if (propIndex == CopyrightPosition)
    {
	m_copyrightID = propID;
    }
    else if (propIndex == AbstractPosition)
    {
	m_abstractID = propID;
    }
    else if (propIndex == KeywordsPosition)
    {
	m_keywordsID = propID;
    }
    else if (propIndex == DescriptionPosition )
    {
	m_descriptionID = propID;
    }

    // set the prop watch
    pPropWatch->SetWatchById(propID);
}


/************************************************************************
 *      Method:
 *          TACData::ClearAll
 *      Purpose:
 *          Clear all/any prop watches for this TAC objects prop IDs
 *
 */
void TACData::ClearAll(IHXPropWatch* pPropWatch)
{

    if (m_titleID > 0)
    {
	pPropWatch->ClearWatchById(m_titleID);
	m_titleID = 0;
    }

    if (m_authorID > 0)
    {
	pPropWatch->ClearWatchById(m_authorID);
	m_authorID = 0;
    }

    if (m_copyrightID > 0)
    {
	pPropWatch->ClearWatchById(m_copyrightID);
	m_copyrightID = 0;
    }

    if (m_abstractID > 0)
    {
	pPropWatch->ClearWatchById(m_abstractID);
	m_abstractID = 0;
    }

    if (m_keywordsID > 0)
    {
	pPropWatch->ClearWatchById(m_keywordsID);
	m_keywordsID = 0;
    }

    if (m_descriptionID > 0)
    {
	pPropWatch->ClearWatchById(m_descriptionID);
	m_descriptionID = 0;
    }

}


/************************************************************************
 *      Method:
 *          TACData::FindMasterID
 *      Purpose:
 *          Given a Source0 TAC prop id, find the correlating master ID
 *
 */
UINT32 TACData::FindMasterIndex(UINT32 sourcePropID)
{
    UINT32 res = NoFind;  

    if (m_titleID == sourcePropID)
    {
	res = TitlePosition;
    }

    if (m_authorID == sourcePropID)
    {
	res = AuthorPosition;
    }

    if (m_copyrightID == sourcePropID)
    {
	res = CopyrightPosition;
    }

    if (m_abstractID == sourcePropID)
    {
	res = AbstractPosition;
    }

    if (m_keywordsID == sourcePropID)
    {
	res = KeywordsPosition;
    }
    
    if (m_descriptionID == sourcePropID)
    {
	res = DescriptionPosition;
    }


    return res;
}

/************************************************************************
 *      Method:
 *          TACData::IsIDPresent
 *      Purpose:
 *          Given a Source0 TAC prop id, find the correlating master ID
 *
 */
HXBOOL TACData::IsIDPresent(UINT32 sourcePropID)
{
    HXBOOL res = FALSE;

    if (m_titleID == sourcePropID || 
	m_authorID == sourcePropID || 
	m_copyrightID == sourcePropID || 
	m_abstractID == sourcePropID ||
	m_keywordsID == sourcePropID ||
        m_descriptionID == sourcePropID ) 

    {
	res = TRUE;
    }

    return res;
}


/************************************************************************
 *      Method:
 *          TACData::IsIDPresent
 *      Purpose:
 *          Given a Source0 TAC prop id, find the correlating master ID
 *
 */
void TACData::Clear(UINT32 sourcePropID)
{
    if (m_titleID == sourcePropID)
    {
	m_titleID = 0;
    }

    if (m_authorID == sourcePropID)
    {
	m_authorID = 0;
    }

    if (m_copyrightID == sourcePropID)
    {
	m_copyrightID = 0;
    }

    if (m_abstractID == sourcePropID)
    {
	m_abstractID = 0;
    }

    if (m_keywordsID == sourcePropID)
    {
	m_keywordsID = 0;
    }
    
    if (m_descriptionID == sourcePropID)
    {
	m_descriptionID = 0;
    }

}
