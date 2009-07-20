/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: basgroup.cpp,v 1.11 2007/07/06 21:58:11 jfinnecy Exp $
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

#include "hxresult.h"
#include "hxausvc.h"
#include "ihxpckts.h"
#include "hxmap.h"
#include "smiltype.h"
#include "hxgroup.h"
#include "basgroup.h"
#include "advgroup.h"
#include "hxwintyp.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxplay.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "srcinfo.h"
#include "hxslist.h"
#include "hxtac.h"
#include "hxstrutl.h"
#include "hxtlogutil.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

HXBasicTrack::HXBasicTrack(HXBasicGroup* pHXGroup)
            : m_lRefCount(0)
            , m_pHXGroup(NULL)
            , m_pValues(NULL)
            , m_bActive(TRUE)
            , m_uTrackIndex(0)
{
    m_pHXGroup = pHXGroup;
}

HXBasicTrack::~HXBasicTrack(void)
{
    Close();
}

/*
 *  IUnknown methods
 */
STDMETHODIMP HXBasicTrack::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXTrack))
    {
        AddRef();
        *ppvObj = (IHXGroup*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) HXBasicTrack::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXBasicTrack::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *  IHXTrack methods
 */
/************************************************************************
*  Method:
*	    IHXBasicTrack::Begin()
*  Purpose:
*	    start the track
*/
STDMETHODIMP
HXBasicTrack::Begin(void)
{
    return HXR_NOTIMPL;
}

/************************************************************************
*  Method:
*	    IHXBasicTrack::Pause()
*  Purpose:
*	    pause the track
*/
STDMETHODIMP
HXBasicTrack::Pause(void)
{
    return HXR_NOTIMPL;
}

/************************************************************************
*  Method:
*	    IHXBasicTrack::Seek()
*  Purpose:
*	    seek the track
*/
STDMETHODIMP
HXBasicTrack::Seek(UINT32 ulSeekTime)
{
    return HXR_NOTIMPL;
}

/************************************************************************
*  Method:
*	    IHXBasicTrack::Stop()
*  Purpose:
*	    stop the track
*/
STDMETHODIMP
HXBasicTrack::Stop(void)
{
    return HXR_NOTIMPL;
}

/************************************************************************
*  Method:
*	    IHXBasicTrack::AddRepeat()
*  Purpose:
*	    add repeat tracks
*/
STDMETHODIMP
HXBasicTrack::AddRepeat(IHXValues* pTrack)
{
    return HXR_NOTIMPL;
}

/************************************************************************
*  Method:
*	    IHXBasicTrack::GetTrackProperties()
*  Purpose:
*	    get track properties
*/
STDMETHODIMP
HXBasicTrack::GetTrackProperties(REF(IHXValues*) pValues,
			     REF(IHXValues*) pValuesInRequest)
{
    pValues = m_pValues;
    if (pValues)
    {
	pValues->AddRef();
    }

    pValuesInRequest = m_pValuesInRequest;
    if (pValuesInRequest)
    {
	pValuesInRequest->AddRef();
    }

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXBasicTrack::GetSource
 *	Purpose:
 *	    Returns the Nth source instance supported by this player.
 */
STDMETHODIMP
HXBasicTrack::GetSource(REF(IHXStreamSource*)	pStreamSource)
{
    return HXR_NOTIMPL;
}

/************************************************************************
*  Method:
*	    IHXBasicTrack::SetSoundLevel()
*  Purpose:
*	    Set Audio Level
*/
STDMETHODIMP
HXBasicTrack::SetSoundLevel(UINT16 uSoundLevel)
{
    return HXR_NOTIMPL;
}

/************************************************************************
*  Method:
*	    IHXBasicTrack::GetSoundLevel()
*  Purpose:
*	    Get Audio Level
*/
STDMETHODIMP_(UINT16)
HXBasicTrack::GetSoundLevel()
{
    return 0;
}

/************************************************************************
*  Method:
*	    IHXBasicTrack::BeginSoundLevelAnimation()
*  Purpose:
*	    notify the start of soundlevel animation
*/
STDMETHODIMP
HXBasicTrack::BeginSoundLevelAnimation(UINT16 uSoundLevelBeginWith)
{
    return HXR_NOTIMPL;
}

/************************************************************************
*  Method:
*	    IHXBasicTrack::EndSoundLevelAnimation()
*  Purpose:
*	    notify the stop of soundlevel animation
*/
STDMETHODIMP
HXBasicTrack::EndSoundLevelAnimation(UINT16 uSoundLevelEndWith)
{
    return HXR_NOTIMPL;
}

HX_RESULT
HXBasicTrack::SetTrackProperties(IHXValues* pValues,
			     IHXValues* pValuesInRequest)
{    
    m_pValues = pValues;    
    if (m_pValues)
    {
	m_pValues->AddRef();
    }

    m_pValuesInRequest = pValuesInRequest;
    if (m_pValuesInRequest)
    {
	m_pValuesInRequest->AddRef();
    }

    return HXR_OK;
}

void
HXBasicTrack::Close(void)
{
    HX_RELEASE(m_pValues);
    HX_RELEASE(m_pValuesInRequest);
}

HXBasicGroup::HXBasicGroup(HXBasicGroupManager* pManager)
            : m_lRefCount(0)
            , m_pGroupManager(NULL)
            , m_pPlayer(NULL)
            , m_bToNotifyTrack(FALSE)
            , m_uTrackCount(0)
            , m_pTrackMap(NULL)
            , m_uGroupIndex(0)
{
    m_pTrackMap = new CHXMapLongToObj;

    m_pGroupManager = pManager;
    m_pGroupManager->AddRef();

    m_pPlayer = m_pGroupManager->m_pPlayer;
}

HXBasicGroup::~HXBasicGroup(void)
{
    Close();
    HX_RELEASE(m_pGroupManager);
}

/*
 *  IUnknown methods
 */
STDMETHODIMP HXBasicGroup::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXGroup))
    {
        AddRef();
        *ppvObj = (IHXGroup*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) HXBasicGroup::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXBasicGroup::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *  IHXGroup methods
 */
/************************************************************************
*  Method:
*      IHXBasicGroup::SetGroupProperties
*  Purpose:
*		Set any group specific information like Title Author 
*		Copyright etc. 
*/
STDMETHODIMP
HXBasicGroup::SetGroupProperties(IHXValues*  /*IN*/ pProperties)
{
    return HXR_NOTIMPL;
}

/************************************************************************
*  Method:
*      IHXBasicGroup::GetGroupProperties
*  Purpose:
*		Get any group specific information. May return NULL.
*/
STDMETHODIMP_(IHXValues*)
HXBasicGroup::GetGroupProperties(void)
{
    return NULL;
}

/************************************************************************
*  Method:
*      IHXBasicGroup::GetTrackCount
*  Purpose:
*		Get the number of tracks within this group.
*/
STDMETHODIMP_(UINT16)
HXBasicGroup::GetTrackCount(void)
{
    return m_uTrackCount;
}

/************************************************************************
*  Method:
*      IHXBasicGroup::GetTrack
*  Purpose:
*		Call this to hook audio data after all audio streams in this
*		have been mixed.
*/
STDMETHODIMP
HXBasicGroup::GetTrack(UINT16 	        /*IN*/  uTrackIndex,
		       REF(IHXValues*)  /*OUT*/ pTrack)
{
    HX_RESULT	rc = HXR_OK;
    IHXValues* pTrackPropInRequest = NULL;

    rc = DoGetTrack(uTrackIndex, pTrack, pTrackPropInRequest);

    HX_RELEASE(pTrackPropInRequest);

    return rc;
}

HX_RESULT
HXBasicGroup::DoGetTrack(UINT16 		/*IN*/	uTrackIndex,
		         REF(IHXValues*)	/*OUT*/ pTrack,
		         REF(IHXValues*)	/*OUT*/	pTrackPropInRequest)
{
    HX_RESULT	    rc = HXR_OK;
    HXBasicTrack*   pHXTrack = NULL;

    if (!m_pTrackMap->Lookup(uTrackIndex, (void*&)pHXTrack))
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    if (!pHXTrack->m_bActive)
    {
	rc = HXR_IGNORE;
	goto cleanup;
    }

    rc = pHXTrack->GetTrackProperties(pTrack, pTrackPropInRequest);

cleanup:
    
    return rc;
}

/************************************************************************
*  Method:
*      IHXBasicGroup::AddTrack
*  Purpose:
*		SMIL renderer (IHXGroupSupplier) may want to add tracks even 
*		after adding IHXGroup to the IHXGroupManager
*/
STDMETHODIMP
HXBasicGroup::AddTrack(IHXValues*	/*IN*/ pTrack)
{
    HX_RESULT   rc = HXR_OK;

    HXBasicTrack* pHXTrack = new HXBasicTrack(this);
    pHXTrack->AddRef();

    rc = DoAddTrack(pTrack, NULL, pHXTrack);
    if (HXR_OK != rc)
    {
        HX_RELEASE(pHXTrack);
    }

    return rc;
}

HX_RESULT
HXBasicGroup::DoAddTrack(IHXValues*     /*IN*/ pTrack,
		         IHXValues*     /*IN*/ pTrackPropInRequest,
                         HXBasicTrack*  /*IN*/ pHXTrack)
{
    HX_RESULT	    theErr = HXR_OK;

    if (!pTrack || !pHXTrack)
    {
	return HXR_UNEXPECTED;
    }

    HXLOGL3(HXLOG_CORP, "HXBasicGroup[%p]::DoAddTrack() Entering...");

    pHXTrack->SetTrackProperties(pTrack, pTrackPropInRequest);
    pHXTrack->m_uTrackIndex = m_uTrackCount;
    (*m_pTrackMap)[m_uTrackCount] = pHXTrack;
    m_uTrackCount++;

    if (m_bToNotifyTrack)
    {
	theErr = m_pGroupManager->TrackAdded(m_uGroupIndex, pHXTrack->m_uTrackIndex, pTrack);
	HX_ASSERT(theErr == HXR_OK);
    }

    HXLOGL3(HXLOG_CORP, "HXBasicGroup[%p]::DoAddTrack() Exiting...");

    return theErr;
}

/************************************************************************
*  Method:
*      IHXBasicGroup::RemoveTrack
*  Purpose:
*		Remove an already added track
*/
STDMETHODIMP
HXBasicGroup::RemoveTrack(UINT16	/*IN*/ uTrackIndex)
{
    return HXR_NOTIMPL;
}

HX_RESULT
HXBasicGroup::CurrentGroupSet(void)
{
    return HXR_OK;
}

void		    
HXBasicGroup::StartTrackNotification(void)
{
    m_bToNotifyTrack = TRUE;
}

void
HXBasicGroup::Close(void)
{
    CHXMapLongToObj::Iterator i;
 
    m_uTrackCount = 0;

    if (m_pTrackMap)
    {
	i = m_pTrackMap->Begin();
	for(; i != m_pTrackMap->End(); ++i)
	{
	    HXBasicTrack* pHXTrack = (HXBasicTrack*)(*i);
	    pHXTrack->Close();
	    HX_RELEASE(pHXTrack);
	}
	HX_DELETE(m_pTrackMap);
    }
}

/*HXGroupManager*/

HXBasicGroupManager::HXBasicGroupManager(HXPlayer* pPlayer)
                    : m_lRefCount(0)
                    , m_pGroupMap(NULL)
                    , m_pSinkList(NULL)
                    , m_pPresentationProperties(NULL)
                    , m_uGroupCount(0)
                    , m_uCurrentGroup(0)
                    , m_uNextGroup(0)
                    , m_bDefaultNextGroup(TRUE)
                    , m_bCurrentGroupInitialized(FALSE)
                    , m_pPlayer(pPlayer)
{
    m_pGroupMap	    = new CHXMapLongToObj;
    m_pSinkList	    = new CHXSimpleList;
	
    if (m_pPlayer)
	m_pPlayer->AddRef();
}

HXBasicGroupManager::~HXBasicGroupManager(void)
{
    Close();
}

/*
 *  IUnknown methods
 */
STDMETHODIMP HXBasicGroupManager::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXGroupManager))
    {
        AddRef();
        *ppvObj = (IHXGroupManager*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPreCacheGroupMgr))
    {
        AddRef();
        *ppvObj = (IHXPreCacheGroupMgr*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) HXBasicGroupManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXBasicGroupManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/*
 *  IHXGroupManager methods
 */

/************************************************************************
*  Method:
*      IHXBasicGroupManager::CreateGroup
*  Purpose:
*		Create a group
*/
STDMETHODIMP
HXBasicGroupManager::CreateGroup(REF(IHXGroup*) pGroup)
{
    pGroup = new HXBasicGroup(this);
    if (!pGroup)
    {
        return HXR_OUTOFMEMORY;
    }
    
    pGroup->AddRef();

    return HXR_OK;
}
/************************************************************************
*  Method:
*      IHXBasicGroupManager::GetGroupCount
*  Purpose:
*		Get the number of groups within the presentation.
*/
STDMETHODIMP_(UINT16)
HXBasicGroupManager::GetGroupCount(void)
{
    return m_uGroupCount;
}

/************************************************************************
*  Method:
*      IHXBasicGroupManager::GetGroup
*  Purpose:
*		Get ith group in the presentation
*/
STDMETHODIMP
HXBasicGroupManager::GetGroup(UINT16 	    /*IN*/ uGroupIndex,
			  REF(IHXGroup*)  /*OUT*/ pGroup)
{
    HX_RESULT	rc = HXR_OK;

    pGroup = NULL;

    if (!m_pGroupMap->Lookup(uGroupIndex, (void*&)pGroup))
    {
        rc = HXR_UNEXPECTED;
        goto cleanup;
    }

    pGroup->AddRef();

cleanup:
    
    return rc;
}

/************************************************************************
*  Method:
*      IHXBasicGroupManager::SetCurrentGroup
*  Purpose:
*		Play this group in the presentation.
*/
STDMETHODIMP
HXBasicGroupManager::SetCurrentGroup(UINT16 	    /*IN*/ uGroupIndex)
{
    HX_RESULT	rc = HXR_OK;
    IHXGroup*	pHXGroup = NULL;
    CHXSimpleList::Iterator ndx;

    if (m_bCurrentGroupInitialized)
    {
        // XXX HP
        // we should not receive multiple SetCurrentGroup() calls
        // on the same group, it would screw-up the layout stuff!!
        HX_ASSERT(m_uCurrentGroup != uGroupIndex);
    }

    if (HXR_OK != GetGroup(uGroupIndex, pHXGroup))
    {
        rc = HXR_UNEXPECTED;
        goto cleanup;
    }

    ndx = m_pSinkList->Begin();
    for (; ndx != m_pSinkList->End(); ++ndx)
    {
        IHXGroupSink* pGroupSink = (IHXGroupSink*) (*ndx);
        pGroupSink->CurrentGroupSet(uGroupIndex, pHXGroup);
    }

    // add repeated sources
#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    ((HXAdvancedGroup*)pHXGroup)->CurrentGroupSet();
#else
    ((HXBasicGroup*)pHXGroup)->CurrentGroupSet();
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */

    m_uCurrentGroup = uGroupIndex;
    m_bCurrentGroupInitialized = TRUE;

cleanup:

    HX_RELEASE(pHXGroup);

    return rc;
}

/************************************************************************
*  Method:
*      IHXBasicGroupManager::GetCurrentGroup
*  Purpose:
*		Get the current presentation group index
*/
STDMETHODIMP
HXBasicGroupManager::GetCurrentGroup(REF(UINT16)	/*OUT*/ uGroupIndex)
{
    uGroupIndex = m_uCurrentGroup;
    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXBasicGroupManager::AddGroup
*  Purpose:
*		Add a group to the presentation.
*/
STDMETHODIMP
HXBasicGroupManager::AddGroup(IHXGroup*	/*IN*/ pGroup)
{
    HX_RESULT	    theErr = HXR_OK;

    if (!pGroup)
    {
        return HXR_UNEXPECTED;
    }
    
    theErr = InsertGroupAt(m_uGroupCount, pGroup);

    return theErr;
}

/************************************************************************
*  Method:
*      IHXBasicGroupManager::RemoveGroup
*  Purpose:
*		Remove an already added group
*/
STDMETHODIMP
HXBasicGroupManager::RemoveGroup(UINT16 	/*IN*/ uGroupIndex)
{
    return HXR_NOTIMPL;
}    

/************************************************************************
*  Method:
*      IHXBasicGroupManager::AddSink
*  Purpose:
*		Add a sink to get notifications about any tracks or groups
*		being added to the presentation.
*/
STDMETHODIMP
HXBasicGroupManager::AddSink(IHXGroupSink* /*IN*/ pGroupSink)
{
    if (!pGroupSink)
    {
        return HXR_UNEXPECTED;
    }

    pGroupSink->AddRef();
    m_pSinkList->AddTail(pGroupSink);

    return HXR_OK;
}


/************************************************************************
*  Method:
*      IHXBasicGroupManager::RemoveSink
*  Purpose:
*		Remove Sink
*/
STDMETHODIMP
HXBasicGroupManager::RemoveSink(IHXGroupSink* /*IN*/ pGroupSink)
{

    LISTPOSITION lPosition = m_pSinkList->Find(pGroupSink);

    if (!lPosition)
    {
        return HXR_UNEXPECTED;
    }

    m_pSinkList->RemoveAt(lPosition);
    pGroupSink->Release();
    
    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXGroupManager::SetNextGroup
*  Purpose:
*		Set the next group to play
*/
STDMETHODIMP
HXBasicGroupManager::SetNextGroup(UINT16 /*IN*/ uGroupIndex)
{
    // setting group index to total number of groups is valid; it means we are done
    // record the next group index
    m_uNextGroup = uGroupIndex;
    m_bDefaultNextGroup = FALSE;

    // tell the player about the next group
    if (m_pPlayer)
    {
        m_pPlayer->NextGroupSet(uGroupIndex);
    }
	
    return HXR_OK;
}	

/************************************************************************
*  Method:
*      IHXGroupManager::GetNextGroup
*  Purpose:
*		Get the next group to play
*/
STDMETHODIMP
HXBasicGroupManager::GetNextGroup(REF(UINT16) uGroupIndex)
{
    IHXGroup*	pNextGroup = NULL;

    if (m_bDefaultNextGroup)
    {
        uGroupIndex = m_uCurrentGroup + 1;
    }
    else
    {
        uGroupIndex = m_uNextGroup;
    }
    // make sure the next group we returned has >= 1 track
    while (m_pGroupMap->Lookup(uGroupIndex, (void*&)pNextGroup))
    {
        if (pNextGroup->GetTrackCount() > 0)
        {
            break;
        }
        uGroupIndex++;
    }

    return HXR_OK;
}	

/************************************************************************
*  Method:
*      IHXGroupManager::DefaultNextGroup
*  Purpose:
*		reset to default the next group to play
*/
STDMETHODIMP
HXBasicGroupManager::DefaultNextGroup(void)
{
    m_bDefaultNextGroup = TRUE;
    return HXR_OK;
}	

/************************************************************************
*  Method:
*      IHXBasicGroupManager::SetPresentationProperties
*  Purpose:
*		Set any presentation information like Title Author 
*		Copyright etc. 
*/
STDMETHODIMP
HXBasicGroupManager::SetPresentationProperties(IHXValues*  /*IN*/ pProperties)
{
    if(!pProperties)
    {
        return HXR_UNEXPECTED;
    }

    HX_RELEASE(m_pPresentationProperties);

    m_pPresentationProperties = pProperties;
    m_pPresentationProperties->AddRef();
    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXBasicGroupManager::GetPresentationProperties
*  Purpose:
*		Get any presentation information. May return NULL.
*/
STDMETHODIMP_(IHXValues*)
HXBasicGroupManager::GetPresentationProperties(void)
{
    if (m_pPresentationProperties)
    {
        m_pPresentationProperties->AddRef();
    }

    return m_pPresentationProperties;
}

HX_RESULT	    
HXBasicGroupManager::TrackAdded(UINT16 uGroupIndex, UINT16 uTrackIndex, 
			    IHXValues* pTrack)
{
    HX_RESULT	rc = HXR_OK;

    CHXSimpleList::Iterator ndx = m_pSinkList->Begin();
    for (; ndx != m_pSinkList->End(); ++ndx)
    {
        IHXGroupSink* pGroupSink = (IHXGroupSink*) (*ndx);
        pGroupSink->TrackAdded(uGroupIndex, uTrackIndex, pTrack);
    }

    return rc;
}

HX_RESULT
HXBasicGroupManager::TrackRemoved(UINT16 uGroupIndex, UINT16 uTrackIndex, 
			      IHXValues* pTrack)
{
    HX_RESULT	rc = HXR_OK;

    CHXSimpleList::Iterator ndx = m_pSinkList->Begin();
    for (; ndx != m_pSinkList->End(); ++ndx)
    {
        IHXGroupSink* pGroupSink = (IHXGroupSink*) (*ndx);
        pGroupSink->TrackRemoved(uGroupIndex, uTrackIndex, pTrack);
    }

    return rc;
}

void 
HXBasicGroupManager::SetMasterTAC(HXMasterTAC* pMasterTAC)
{
    return;
}

HX_RESULT	    
HXBasicGroupManager::TrackStarted(UINT16 uGroupIndex, UINT16 uTrackIndex, HXBOOL bIsRepeating /*=FALSE*/)
{
    HX_RESULT	rc = HXR_OK;
    IHXGroup*	pHXGroup = NULL;
    IHXValues* pValues = NULL;
    CHXSimpleList::Iterator ndx;

    if (!m_pGroupMap->Lookup(uGroupIndex, (void*&)pHXGroup))
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    if (HXR_OK != pHXGroup->GetTrack(uTrackIndex, pValues))
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }
    
#if defined(HELIX_FEATURE_SMIL_REPEAT)
    if (pValues)
    {
        pValues->SetPropertyULONG32("isRepeating", (ULONG32)bIsRepeating);
    }
#endif // / HELIX_FEATURE_SMIL_REPEAT.

    ndx = m_pSinkList->Begin();
    for (; ndx != m_pSinkList->End(); ++ndx)
    {
	IHXGroupSink* pGroupSink = (IHXGroupSink*) (*ndx);
	pGroupSink->TrackStarted(uGroupIndex, uTrackIndex, pValues);
    }

cleanup:

    HX_RELEASE(pValues);    
    return rc;
}

HX_RESULT	    
HXBasicGroupManager::TrackStopped(UINT16 uGroupIndex, UINT16 uTrackIndex)
{
    HX_RESULT	rc = HXR_OK;
    IHXGroup*	pHXGroup = NULL;
    IHXValues* pValues = NULL;
    CHXSimpleList::Iterator ndx;

    if (!m_pGroupMap->Lookup(uGroupIndex, (void*&)pHXGroup))
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    if (HXR_OK != pHXGroup->GetTrack(uTrackIndex, pValues))
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }
    
    ndx = m_pSinkList->Begin();
    for (; ndx != m_pSinkList->End(); ++ndx)
    {
	IHXGroupSink* pGroupSink = (IHXGroupSink*) (*ndx);
	pGroupSink->TrackStopped(uGroupIndex, uTrackIndex, pValues);
    }

cleanup:

    HX_RELEASE(pValues);
    return rc;
}

void
HXBasicGroupManager::PersistentComponentAdded(UINT16 uGroupIndex, UINT16 uTrackIndex)
{
    return;
}

void 
HXBasicGroupManager::PersistentComponentRemoved(UINT16 uGroupIndex, UINT16 uTrackIndex)
{    
    return;
}

HX_RESULT
HXBasicGroupManager::InsertGroupAt(UINT16 uGroupIndex, IHXGroup* pGroup)
{
    HX_RESULT		rc = HXR_OK;
    int			i = 0;
    CHXMapLongToObj*	pNewGroupMap = NULL;
    IHXGroup*		pHXGroup = NULL;

    HX_ASSERT(uGroupIndex <= m_uGroupCount);

    // make the spot for insertion
    if (uGroupIndex < m_uGroupCount)
    {
	// adjust group map index
	pNewGroupMap = new CHXMapLongToObj;
	for (i = 0; i < uGroupIndex; i++)
	{
	    m_pGroupMap->Lookup(i, (void*&)pHXGroup);
	    HX_ASSERT(pHXGroup);

	    (*pNewGroupMap)[i] = pHXGroup;
	}

	for (i = uGroupIndex; i < m_uGroupCount; i++)
	{
	    m_pGroupMap->Lookup(i, (void*&)pHXGroup);
	    HX_ASSERT(pHXGroup);

	    ((HXBasicGroup*)pHXGroup)->m_uGroupIndex = i + 1;
	    (*pNewGroupMap)[i + 1] = pHXGroup;
	}
	HX_DELETE(m_pGroupMap);

	m_pGroupMap = pNewGroupMap;
    }

    pGroup->AddRef();
    ((HXBasicGroup*)pGroup)->m_uGroupIndex = uGroupIndex;
    (*m_pGroupMap)[uGroupIndex] = pGroup;

    m_uGroupCount++;

    ((HXBasicGroup*)pGroup)->StartTrackNotification();

    CHXSimpleList::Iterator ndx = m_pSinkList->Begin();
    for (; ndx != m_pSinkList->End(); ++ndx)
    {
	IHXGroupSink* pGroupSink = (IHXGroupSink*) (*ndx);
	// XXX HP replace this with the commented out line
	// after the TLC supports new IHXGroupSink2
	pGroupSink->GroupAdded(m_uGroupCount-1, pGroup);
	//pGroupSink->GroupInsertedAfter(uGroupIndex, pGroup);
    }

    return rc;
}

void 
HXBasicGroupManager::RemoveAllGroup(void)
{
    HX_RELEASE(m_pPresentationProperties);
    
    m_uGroupCount = 0;
    m_uCurrentGroup = 0;
    m_uNextGroup = 0;

    m_bCurrentGroupInitialized = FALSE;
    m_bDefaultNextGroup = TRUE;

    if (m_pGroupMap)
    {
        CHXMapLongToObj::Iterator i = m_pGroupMap->Begin();    
        for (; i != m_pGroupMap->End(); ++i)
        {
            IHXGroup* pGroup = (IHXGroup*) (*i);
            HX_RELEASE(pGroup);
        }
        m_pGroupMap->RemoveAll();
    }

    if (m_pSinkList)
    {
        CHXSimpleList::Iterator ndx = m_pSinkList->Begin();
        for (; ndx != m_pSinkList->End(); ++ndx)
        {
            IHXGroupSink* pGroupSink = (IHXGroupSink*) (*ndx);
            pGroupSink->AllGroupsRemoved();
        }
    }
}

void
HXBasicGroupManager::Close(void)
{
    RemoveAllGroup();

    HX_DELETE(m_pGroupMap);
    
    if (m_pSinkList)
    {
        CHXSimpleList::Iterator ndx = m_pSinkList->Begin();
        for (; ndx != m_pSinkList->End(); ++ndx)
        {
            IHXGroupSink* pGroupSink = (IHXGroupSink*) (*ndx);
            HX_RELEASE(pGroupSink);
        }
        HX_DELETE(m_pSinkList);
    }
    HX_RELEASE(m_pPlayer);
    HX_RELEASE(m_pPresentationProperties);
}
