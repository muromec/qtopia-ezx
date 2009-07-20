/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: advgroup.cpp,v 1.11 2007/07/06 21:58:11 jfinnecy Exp $
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
#include "pckunpck.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

HXAdvancedTrack::HXAdvancedTrack(HXAdvancedGroup* pHXGroup)
                :HXBasicTrack(pHXGroup)
                ,m_pRepeatList(NULL)
                ,m_bInSoundLevelAnimation(FALSE)
                ,m_uSoundLevel(100)
                ,m_ulSoundLevelAnimationTime(0)
                ,m_ulGranularity(0)
{
}

HXAdvancedTrack::~HXAdvancedTrack(void)
{
    Close();
}

#if defined(HELIX_FEATURE_AUDIOHOOK)
/*
 *  IUnknown methods
 */
STDMETHODIMP HXAdvancedTrack::QueryInterface(REFIID riid, void** ppvObj)
{
    if (HXR_OK == HXBasicTrack::QueryInterface(riid, ppvObj))
    {
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAudioHook))
    {
        AddRef();
        *ppvObj = (IHXAudioHook*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) HXAdvancedTrack::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXAdvancedTrack::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *  IHXAudioHook methods
 */
/************************************************************************
 *  Method:
 *      IHXAudioHook::OnInit
 *  Purpose:
 *      Audio Services calls OnInit() with the audio data format of the
 *	    audio data that will be provided in the OnBuffer() method.
 */
STDMETHODIMP
HXAdvancedTrack::OnInit(HXAudioFormat* /*IN*/ pFormat)
{
    CHXAudioPlayer* pAudioPlayer = NULL;
    
    pAudioPlayer = m_pHXGroup->m_pPlayer->GetAudioPlayer();
    if (pAudioPlayer)
    {
	m_ulSoundLevelAnimationTime = pAudioPlayer->GetCurrentPlayBackTime();
	m_ulGranularity = pAudioPlayer->GetGranularity();
    }

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXAudioHook::OnBuffer
 *  Purpose:
 *      Audio Services calls OnBuffer() with audio data packets. The
 *	    renderer should not modify the data in the IHXBuffer part of
 *	    pAudioInData.  If the renderer wants to write a modified
 *	    version of the data back to Audio Services, then it should 
 *	    create its own IHXBuffer, modify the data and then associate 
 *	    this buffer with the pAudioOutData->pData member.
 */
STDMETHODIMP
HXAdvancedTrack::OnBuffer(HXAudioData*	/*IN*/   pAudioInData,
		   HXAudioData*	/*OUT*/  pAudioOutData)
{
    HX_RESULT	rc = HXR_OK;

    if (!m_pHXGroup)
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    m_ulSoundLevelAnimationTime += m_ulGranularity;
    rc = ((HXAdvancedGroup*)m_pHXGroup)->OnSoundLevelAnimation(m_uTrackIndex, m_uSoundLevel, m_ulSoundLevelAnimationTime);

cleanup:

    return rc;
}
#endif /* HELIX_FEATURE_AUDIOHOOK */

/*
 *  IHXTrack methods
 */
/************************************************************************
*  Method:
*	    IHXTrack::Begin()
*  Purpose:
*	    start the track
*/
STDMETHODIMP
HXAdvancedTrack::Begin(void)
{
    HX_RESULT	rc = HXR_OK;

    if (!m_pHXGroup)
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    rc = ((HXAdvancedGroup*)m_pHXGroup)->BeginTrack(m_uTrackIndex, m_pValues);

cleanup:

    return rc;
}

/************************************************************************
*  Method:
*	    IHXTrack::Pause()
*  Purpose:
*	    pause the track
*/
STDMETHODIMP
HXAdvancedTrack::Pause(void)
{
    HX_RESULT	rc = HXR_OK;

    if (!m_pHXGroup)
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    rc = ((HXAdvancedGroup*)m_pHXGroup)->PauseTrack(m_uTrackIndex, m_pValues);

cleanup:

    return rc;
}

/************************************************************************
*  Method:
*	    IHXTrack::Seek()
*  Purpose:
*	    seek the track
*/
STDMETHODIMP
HXAdvancedTrack::Seek(UINT32 ulSeekTime)
{
    HX_RESULT	rc = HXR_OK;

    if (!m_pHXGroup)
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    rc = ((HXAdvancedGroup*)m_pHXGroup)->SeekTrack(m_uTrackIndex, m_pValues, ulSeekTime);

cleanup:

    return rc;
}

/************************************************************************
*  Method:
*	    IHXTrack::Stop()
*  Purpose:
*	    stop the track
*/
STDMETHODIMP
HXAdvancedTrack::Stop(void)
{
    HX_RESULT	rc = HXR_OK;

    if (!m_pHXGroup)
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    rc = ((HXAdvancedGroup*)m_pHXGroup)->StopTrack(m_uTrackIndex, m_pValues);

cleanup:

    return rc;
}

/************************************************************************
*  Method:
*	    IHXTrack::AddRepeat()
*  Purpose:
*	    add repeat tracks
*/
STDMETHODIMP
HXAdvancedTrack::AddRepeat(IHXValues* pTrack)
{
    if (!m_pRepeatList)
    {
	m_pRepeatList = new CHXSimpleList();
    }

    m_pRepeatList->AddTail(pTrack);
    pTrack->AddRef();

    ((HXAdvancedGroup*)m_pHXGroup)->RepeatTrackAdded(m_uTrackIndex, pTrack);

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXTrack::GetSource
 *	Purpose:
 *	    Returns the Nth source instance supported by this player.
 */
STDMETHODIMP
HXAdvancedTrack::GetSource(REF(IHXStreamSource*) pStreamSource)
{
    HX_RESULT	rc = HXR_OK;

    if (!m_pHXGroup)
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    rc = ((HXAdvancedGroup*)m_pHXGroup)->GetSource(m_uTrackIndex, pStreamSource);

cleanup:

    return rc;
}

/************************************************************************
*  Method:
*	    IHXTrack::SetSoundLevel()
*  Purpose:
*	    Set Audio Level
*/
STDMETHODIMP
HXAdvancedTrack::SetSoundLevel(UINT16 uSoundLevel)
{
    HX_RESULT	rc = HXR_OK;

    if (!m_pHXGroup)
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    if (m_uSoundLevel != uSoundLevel)
    {
	m_uSoundLevel = uSoundLevel;
	rc = ((HXAdvancedGroup*)m_pHXGroup)->SetSoundLevel(m_uTrackIndex, m_uSoundLevel, !m_bInSoundLevelAnimation);
    }

cleanup:

    return rc;
}

/************************************************************************
*  Method:
*	    IHXTrack::GetSoundLevel()
*  Purpose:
*	    Get Audio Level
*/
STDMETHODIMP_(UINT16)
HXAdvancedTrack::GetSoundLevel()
{
    return m_uSoundLevel;
}

/************************************************************************
*  Method:
*	    IHXTrack::BeginSoundLevelAnimation()
*  Purpose:
*	    notify the start of soundlevel animation
*/
STDMETHODIMP
HXAdvancedTrack::BeginSoundLevelAnimation(UINT16 uSoundLevelBeginWith)
{
    HX_RESULT	rc = HXR_OK;

    if (!m_pHXGroup)
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    m_uSoundLevel = uSoundLevelBeginWith;
    m_bInSoundLevelAnimation = TRUE;

    rc = ((HXAdvancedGroup*)m_pHXGroup)->BeginSoundLevelAnimation(m_uTrackIndex, m_uSoundLevel);
    
cleanup:

    return rc;
}

/************************************************************************
*  Method:
*	    IHXTrack::EndSoundLevelAnimation()
*  Purpose:
*	    notify the stop of soundlevel animation
*/
STDMETHODIMP
HXAdvancedTrack::EndSoundLevelAnimation(UINT16 uSoundLevelEndWith)
{
    HX_RESULT	rc = HXR_OK;

    if (!m_pHXGroup)
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    m_uSoundLevel = uSoundLevelEndWith;

    rc = ((HXAdvancedGroup*)m_pHXGroup)->EndSoundLevelAnimation(m_uTrackIndex, m_uSoundLevel);
    m_bInSoundLevelAnimation = FALSE;

cleanup:

    return rc;
}

void
HXAdvancedTrack::Close(void)
{
    if (m_bInSoundLevelAnimation)
    {
	EndSoundLevelAnimation(0);
    }

    if (m_pRepeatList)
    {
	CHXSimpleList::Iterator i = m_pRepeatList->Begin();
	for (; i != m_pRepeatList->End(); ++i)
	{
	    IHXValues* pValues = (IHXValues*)*i;
	    HX_RELEASE(pValues);
	}
	HX_DELETE(m_pRepeatList);
    }

    HXBasicTrack::Close();
}

HXAdvancedGroup::HXAdvancedGroup(HXAdvancedGroupManager* pManager)
                :HXBasicGroup(pManager)
                ,m_pGroupProperties(NULL)
                ,m_bPrefetchSinkAdded(FALSE)
                ,m_uPrefetchTrackCount(0)
                ,m_pPrefetchTrackMap(NULL)
                ,m_pPersistentComponentPropertyMap(NULL)
                ,m_pTrackSinkList(NULL)
                ,m_pPrefetchSinkList(NULL)
{
}

HXAdvancedGroup::~HXAdvancedGroup(void)
{
    Close();
}

/*
 *  IUnknown methods
 */
STDMETHODIMP HXAdvancedGroup::QueryInterface(REFIID riid, void** ppvObj)
{
    if (HXR_OK == HXBasicGroup::QueryInterface(riid, ppvObj))
    {
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXGroup2))
    {
        AddRef();
        *ppvObj = (IHXGroup2*)this;
        return HXR_OK;
    }
#if defined(HELIX_FEATURE_PREFETCH)
    else if (IsEqualIID(riid, IID_IHXPrefetch))
    {
        AddRef();
        *ppvObj = (IHXPrefetch*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPrefetchSink))
    {
        AddRef();
        *ppvObj = (IHXPrefetchSink*)this;
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_PREFETCH */

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) HXAdvancedGroup::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXAdvancedGroup::Release()
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
*      IHXGroup::SetGroupProperties
*  Purpose:
*		Set any group specific information like Title Author 
*		Copyright etc. 
*/
STDMETHODIMP
HXAdvancedGroup::SetGroupProperties(IHXValues*  /*IN*/ pProperties)
{
    UINT32  ulDuration = 0;

    if (m_pGroupProperties || !pProperties)
    {
	return HXR_UNEXPECTED;
    }

    m_pGroupProperties = pProperties;
    m_pGroupProperties->AddRef();

    // support adjust current group's duration on the fly
    if (m_uGroupIndex == m_pGroupManager->m_uCurrentGroup)
    {	
	if (HXR_OK == m_pGroupProperties->GetPropertyULONG32("duration", ulDuration))
	{
	    m_pPlayer->SetPresentationTime(ulDuration);
	}
    }

    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXGroup::GetGroupProperties
*  Purpose:
*		Get any group specific information. May return NULL.
*/
STDMETHODIMP_(IHXValues*)
HXAdvancedGroup::GetGroupProperties(void)
{
    if (m_pGroupProperties)
    {
	m_pGroupProperties->AddRef();
    }

    return m_pGroupProperties;
}

/************************************************************************
*  Method:
*      IHXGroup2::SetPersistentComponentProperties
*  Purpose:
*		Set persistent component properties associated with this group
*		One group may contain multiple persistent components
*/
STDMETHODIMP
HXAdvancedGroup::SetPersistentComponentProperties(UINT32        /*IN*/ ulPersistentComponentID,
					          IHXValues*	/*IN*/ pProperties)
{
    HX_RESULT	rc = HXR_OK;
    IHXValues*	pValues = NULL;

    if (!pProperties)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    if (!m_pPersistentComponentPropertyMap)
    {
	m_pPersistentComponentPropertyMap = new CHXMapLongToObj;
    }

    if (!m_pPersistentComponentPropertyMap->Lookup(ulPersistentComponentID, 
						   (void*&)pValues))
    {
	(*m_pPersistentComponentPropertyMap)[ulPersistentComponentID] = pProperties;
	pProperties->AddRef();
    }

cleanup:

    return rc;
}

/************************************************************************
*  Method:
*      IHXGroup2::GetPersistentComponentProperties
*  Purpose:
*		Get any persistent component specific information associated with 
*		the group.
*		One group may contain multiple persistent components
*/
STDMETHODIMP
HXAdvancedGroup::GetPersistentComponentProperties(UINT32          /*IN*/  ulPersistentComponentID,
					          REF(IHXValues*) /*OUT*/ pProperties)
{	
    HX_RESULT	rc = HXR_OK;
    
    pProperties = NULL;

    if (m_pPersistentComponentPropertyMap &&
	m_pPersistentComponentPropertyMap->Lookup(ulPersistentComponentID, (void*&)pProperties))
    {
	pProperties->AddRef();
    }
    else
    {
	rc = HXR_FAILED;
    }

    return rc;
}

/************************************************************************
*  Method:
*      IHXGroup2::GetTrack2
*  Purpose:
*		Get ith track in this group
*/
STDMETHODIMP
HXAdvancedGroup::GetTrack2(UINT16		/*IN*/	uTrackIndex,
		           REF(IHXValues*)	/*OUT*/ pTrack,
		           REF(IHXValues*)	/*OUT*/	pTrackPropInRequest)
{
    return DoGetTrack(uTrackIndex, pTrack, pTrackPropInRequest);
}

STDMETHODIMP
HXAdvancedGroup::AddTrack(IHXValues*	/*IN*/ pTrack)
{
    HX_RESULT   rc = HXR_OK;

    HXAdvancedTrack* pHXTrack = new HXAdvancedTrack(this);
    pHXTrack->AddRef();

    rc = DoAddTrack(pTrack, NULL, pHXTrack);
    if (HXR_OK != rc)
    {
        HX_RELEASE(pHXTrack);
    }

    return rc;
}

/************************************************************************
*  Method:
*      IHXGroup2::AddTrack2
*  Purpose:
*		Add Tracks to the group, including the props set in RequestHeader
*/
STDMETHODIMP
HXAdvancedGroup::AddTrack2(IHXValues*   /*IN*/ pTrack,
		           IHXValues*   /*IN*/ pTrackPropInRequest)
{
    HX_RESULT   rc = HXR_OK;

    HXAdvancedTrack* pHXTrack = new HXAdvancedTrack(this);
    pHXTrack->AddRef();
   
    rc = DoAddTrack(pTrack, pTrackPropInRequest, pHXTrack);
    if (HXR_OK != rc)
    {
        HX_RELEASE(pHXTrack);
    }

    return rc;
}

/************************************************************************
*  Method:
*      IHXGroup::RemoveTrack
*  Purpose:
*		Remove an already added track
*/
STDMETHODIMP
HXAdvancedGroup::RemoveTrack(UINT16	/*IN*/ uTrackIndex)
{
    HX_RESULT		theErr = HXR_OK;
    int			i = 0;
    IHXValues*		pValues = NULL;
    IHXValues*		pValuesInRequest = NULL;
    IHXValues*		pUpdateValues = NULL;
    CHXMapLongToObj*	pNewTrackMap = NULL;
    HXAdvancedTrack*	pHXTrackRemoved = NULL;
    HXAdvancedTrack*	pHXTrack = NULL;

    if (!m_pTrackMap->Lookup(uTrackIndex, (void*&)pHXTrackRemoved))
    {
	theErr = HXR_UNEXPECTED;
	goto cleanup;
    }

    m_pTrackMap->RemoveKey(uTrackIndex);

    pHXTrackRemoved->GetTrackProperties(pValues, pValuesInRequest);
    HX_ASSERT(pValues);

    HX_ASSERT(m_pPlayer);
    if (HXR_OK != m_pPlayer->RemoveTrack(m_uGroupIndex, uTrackIndex, pValues))
    {
	theErr = HXR_UNEXPECTED;
	goto cleanup;
    }

    if (m_bToNotifyTrack)
    {
	theErr = m_pGroupManager->TrackRemoved(m_uGroupIndex, uTrackIndex, pValues);
    }

    // adjust track map index
    pNewTrackMap = new CHXMapLongToObj;
    for (i = 0; i < uTrackIndex; i++)
    {
	m_pTrackMap->Lookup(i, (void*&)pHXTrack);
	HX_ASSERT(pHXTrack);

	(*pNewTrackMap)[i] = pHXTrack;
    }

    for (i = uTrackIndex + 1; i < m_uTrackCount; i++)
    {
	m_pTrackMap->Lookup(i, (void*&)pHXTrack);
	HX_ASSERT(pHXTrack);

	pHXTrack->m_uTrackIndex = i - 1;
	(*pNewTrackMap)[i - 1] = pHXTrack;

	if (HXR_OK == CreateValuesCCF(pUpdateValues, (IUnknown*)(IHXPlayer*)m_pPlayer))
	{
	    pUpdateValues->SetPropertyULONG32("TrackIndex", i - 1);
	    m_pPlayer->UpdateTrack(m_uGroupIndex, i, pUpdateValues);
	    HX_RELEASE(pUpdateValues);
	}
    }
    HX_DELETE(m_pTrackMap);

    m_pTrackMap = pNewTrackMap;
    m_uTrackCount--;
    
cleanup:

    HX_RELEASE(pValues);
    HX_RELEASE(pValuesInRequest);
    HX_RELEASE(pHXTrackRemoved);

    return theErr;
}

HX_RESULT
HXAdvancedGroup::BeginTrack(UINT16      /*IN*/ uTrackIndex,
		            IHXValues*  /*IN*/ pTrack)
{
    HX_RESULT	theErr = HXR_OK;

    HX_ASSERT(m_pPlayer);
    theErr = m_pPlayer->BeginTrack(m_uGroupIndex, uTrackIndex, pTrack);

    if (m_pTrackSinkList)
    {
	CHXSimpleList::Iterator ndx = m_pTrackSinkList->Begin();
	for (; ndx != m_pTrackSinkList->End(); ++ndx)
	{
	    IHXTrackSink* pTrackSink = (IHXTrackSink*) (*ndx);
	    pTrackSink->BeginDone(m_uGroupIndex, uTrackIndex);
	}
    }

    return theErr;
}

HX_RESULT
HXAdvancedGroup::PauseTrack(UINT16      /*IN*/ uTrackIndex,
		            IHXValues*  /*IN*/ pTrack)
{
    HX_RESULT	theErr = HXR_OK;

    HX_ASSERT(m_pPlayer);
    theErr = m_pPlayer->PauseTrack(m_uGroupIndex, uTrackIndex, pTrack);

    if (m_pTrackSinkList)
    {
	CHXSimpleList::Iterator ndx = m_pTrackSinkList->Begin();
	for (; ndx != m_pTrackSinkList->End(); ++ndx)
	{
	    IHXTrackSink* pTrackSink = (IHXTrackSink*) (*ndx);
	    pTrackSink->PauseDone(m_uGroupIndex, uTrackIndex);
	}
    }

    return theErr;
}

HX_RESULT
HXAdvancedGroup::SeekTrack(UINT16	    /*IN*/ uTrackIndex,
		           IHXValues*	    /*IN*/ pTrack,
		           UINT32	    /*IN*/ ulSeekTime)
{
    HX_RESULT	theErr = HXR_OK;

    HX_ASSERT(m_pPlayer);
    theErr = m_pPlayer->SeekTrack(m_uGroupIndex, uTrackIndex, pTrack, ulSeekTime);

    if (m_pTrackSinkList)
    {
	CHXSimpleList::Iterator ndx = m_pTrackSinkList->Begin();
	for (; ndx != m_pTrackSinkList->End(); ++ndx)
	{
	    IHXTrackSink* pTrackSink = (IHXTrackSink*) (*ndx);
	    pTrackSink->SeekDone(m_uGroupIndex, uTrackIndex, ulSeekTime);
	}
    }

    return theErr;
}

HX_RESULT
HXAdvancedGroup::StopTrack(UINT16	/*IN*/ uTrackIndex,
		           IHXValues*	/*IN*/ pTrack)
{
    HX_RESULT	theErr = HXR_OK;

    HX_ASSERT(m_pPlayer);    
    theErr = m_pPlayer->StopTrack(m_uGroupIndex, uTrackIndex, pTrack);

    if (m_pTrackSinkList)
    {
	CHXSimpleList::Iterator ndx = m_pTrackSinkList->Begin();
	for (; ndx != m_pTrackSinkList->End(); ++ndx)
	{
	    IHXTrackSink* pTrackSink = (IHXTrackSink*) (*ndx);
	    pTrackSink->StopDone(m_uGroupIndex, uTrackIndex);
	}
    }

    return theErr;
}

HX_RESULT	
HXAdvancedGroup::GetSource(UINT16		uTrackIndex, 
		           IHXStreamSource*&	pStreamSource)
{
    SourceInfo*	pSourceInfo = NULL;
    pStreamSource = NULL;

    HX_ASSERT(m_pPlayer);
    if (HXR_OK == m_pPlayer->GetSourceInfo(m_uGroupIndex, uTrackIndex, pSourceInfo) && 
	pSourceInfo)
    {
	return pSourceInfo->m_pSource->QueryInterface(IID_IHXStreamSource, (void**)&pStreamSource);
    }

    return HXR_FAILED;
}

HX_RESULT
HXAdvancedGroup::SetSoundLevel(UINT16	/*IN*/ uTrackIndex,
			       UINT16	/*IN*/ uSoundLevel,
			       HXBOOL	/*IN*/ bReflushAudioDevice)
{
    HX_RESULT	theErr = HXR_OK;


    HX_ASSERT(m_pPlayer);
    theErr = m_pPlayer->SetSoundLevel(m_uGroupIndex, 
				      uTrackIndex, 
				      uSoundLevel,
				      bReflushAudioDevice);

    return theErr;
}

HX_RESULT
HXAdvancedGroup::BeginSoundLevelAnimation(UINT16 uTrackIndex, 
				          UINT16 uSoundLevelBeginWith)
{    
    HX_RESULT	        rc = HXR_OK;
    CHXAudioPlayer*     pAudioPlayer = NULL;
    HXAdvancedTrack*	pHXTrack = NULL;

    HX_ASSERT(m_pPlayer);
    if ((pAudioPlayer = m_pPlayer->GetAudioPlayer()) &&
	m_pTrackMap->Lookup(uTrackIndex, (void*&)pHXTrack))
    {	    
	pAudioPlayer->ActualAddPostMixHook((IHXAudioHook*)pHXTrack, 0, 1);

	m_pPlayer->SetSoundLevel(m_uGroupIndex, 
				 uTrackIndex, 
				 uSoundLevelBeginWith, 
				 TRUE);	
    }

    return rc;
}

HX_RESULT
HXAdvancedGroup::EndSoundLevelAnimation(UINT16 uTrackIndex, 
				        UINT16 uSoundLevelEndWith)
{
    HX_RESULT	        theErr = HXR_OK;
    CHXAudioPlayer*     pAudioPlayer = NULL;
    HXAdvancedTrack*	pHXTrack = NULL;

    HX_ASSERT(m_pPlayer);
    if ((pAudioPlayer = m_pPlayer->GetAudioPlayer()) &&
	m_pTrackMap->Lookup(uTrackIndex, (void*&)pHXTrack))
    {	
	pAudioPlayer->ActualRemovePostMixHook((IHXAudioHook*)pHXTrack);
    
	m_pPlayer->SetSoundLevel(m_uGroupIndex, 
				 uTrackIndex, 
				 uSoundLevelEndWith, 
				 FALSE);
    }

    return theErr;
}

HX_RESULT
HXAdvancedGroup::OnSoundLevelAnimation(UINT16 uTrackIndex, 
                                       UINT16 uSoundLevel, 
                                       UINT32 ulSoundLevelAnimationTime)
{
    HX_RESULT	theErr = HXR_OK;

    if (m_pTrackSinkList)
    {
	CHXSimpleList::Iterator ndx = m_pTrackSinkList->Begin();
	for (; ndx != m_pTrackSinkList->End(); ++ndx)
	{
	    IHXTrackSink* pTrackSink = (IHXTrackSink*) (*ndx);
	    pTrackSink->OnSoundLevelAnimation(m_uGroupIndex, uTrackIndex, ulSoundLevelAnimationTime);
	}
    }

    return theErr;
}

void
HXAdvancedGroup::PersistentComponentAdded(UINT16 uTrackIndex)
{
    HXAdvancedTrack*    pPersistentHXTrack = NULL;

    // XXX HP we don't actually remove the track in order
    // to preserve the group/track ID of subsequent sources
    // Need more work on RemoveTrack() so that all the group/
    // track ID get updated across the board(esp. in persistent
    // renderers)!!
    if (m_pTrackMap->Lookup(uTrackIndex, (void*&)pPersistentHXTrack))
    {
	pPersistentHXTrack->m_bActive = FALSE;
    }

    return;
}

void
HXAdvancedGroup::PersistentComponentRemoved(UINT16 uTrackIndex)
{
    return;
}

HX_RESULT
HXAdvancedGroup::CurrentGroupSet(void)
{
    HX_RESULT	rc = HXR_OK;

    CHXMapLongToObj::Iterator i = m_pTrackMap->Begin();
    for(; i != m_pTrackMap->End(); ++i)
    {
	HXAdvancedTrack*    pHXTrack = (HXAdvancedTrack*)(*i);
	CHXSimpleList*	    pRepeatList = pHXTrack->m_pRepeatList;
	
	if (pRepeatList)
	{
	    CHXSimpleList::Iterator ndx = pRepeatList->Begin();
	    for (; ndx != pRepeatList->End(); ++ndx)
	    {
		IHXValues* pRepeatTrack = (IHXValues*) (*ndx);
		((HXAdvancedGroupManager*)m_pGroupManager)->RepeatTrackAdded(m_uGroupIndex, 
                                                                             pHXTrack->m_uTrackIndex, 
                                                                             pRepeatTrack);
	    }
	}
    }

    return rc;
}

HX_RESULT	
HXAdvancedGroup::RepeatTrackAdded(UINT16 uTrackIndex, IHXValues* pValues)
{
    HX_RESULT rc = HXR_OK;

    if (m_bToNotifyTrack)
    {
	rc = ((HXAdvancedGroupManager*)m_pGroupManager)->RepeatTrackAdded(m_uGroupIndex, 
                                                                          uTrackIndex, 
                                                                          pValues);
	HX_ASSERT(rc == HXR_OK);
    }

    return rc;
}

/************************************************************************
*  Method:
*      IHXGroup2::GetIHXTrack
*  Purpose:
*		get the IHXTrack object
*/
STDMETHODIMP
HXAdvancedGroup::GetIHXTrack(UINT16         /*IN*/  uTrackIndex,
		             REF(IHXTrack*) /*OUT*/ pTrack)
{
    HX_RESULT rc = HXR_OK;

    pTrack = NULL;
    if (!m_pTrackMap->Lookup(uTrackIndex, (void*&)pTrack))
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    pTrack->AddRef();

cleanup:

    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXGroup2::AddTrackSink
*  Purpose:
*		add advise sink on track
*/
STDMETHODIMP
HXAdvancedGroup::AddTrackSink(IHXTrackSink*   /*IN*/  pSink)
{
    if (!pSink)
    {
	return HXR_UNEXPECTED;
    }

    if (!m_pTrackSinkList)
    {
	m_pTrackSinkList = new CHXSimpleList;
    }

    pSink->AddRef();
    m_pTrackSinkList->AddTail(pSink);

    return HXR_OK;    
}

/************************************************************************
*  Method:
*      IHXGroup2::RemoveTrackSink
*  Purpose:
*		remove advise sink on track
*/
STDMETHODIMP
HXAdvancedGroup::RemoveTrackSink(IHXTrackSink*   /*IN*/  pSink)
{
    if (!m_pTrackSinkList)
    {
	return HXR_UNEXPECTED;
    }

    LISTPOSITION lPosition = m_pTrackSinkList->Find(pSink);
    if (!lPosition)
    {
	return HXR_UNEXPECTED;
    }

    m_pTrackSinkList->RemoveAt(lPosition);
    HX_RELEASE(pSink);
    
    return HXR_OK;
}

#if defined(HELIX_FEATURE_PREFETCH)
/************************************************************************
*  Method:
*      IHXPrefetch::AddPrefetchTrack
*  Purpose:
*      adds prefetch track by specifying "PrefetchType" and "PrefetchValue"
*      in pTrack's IHXValues
*/
STDMETHODIMP
HXAdvancedGroup::AddPrefetchTrack(IHXValues* /*IN*/  pTrack)
{
    HX_RESULT	    rc = HXR_OK;
    UINT16	    uPrefetchIndex = 0;
    IHXPrefetch*    pPrefetch = NULL;

    if (!pTrack)
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    uPrefetchIndex = m_uPrefetchTrackCount;

    pTrack->SetPropertyULONG32("GroupIndex", m_uGroupIndex);
    pTrack->SetPropertyULONG32("TrackIndex", uPrefetchIndex);

    // give TLC a first bite on prefetch track
    if (HXR_OK == m_pPlayer->QueryInterface(IID_IHXPrefetch, (void**)&pPrefetch))
    {
	rc = pPrefetch->AddPrefetchTrack(pTrack);

	if (HXR_OK == rc && !m_bPrefetchSinkAdded)
	{
	    m_bPrefetchSinkAdded = TRUE;
	    pPrefetch->AddPrefetchSink(this);
	}
	else if (HXR_NOT_SUPPORTED == rc)
	{
	    rc = m_pPlayer->AddPrefetchTrack(m_uGroupIndex, uPrefetchIndex, pTrack);
	}
    }
    else
    {	
	rc = m_pPlayer->AddPrefetchTrack(m_uGroupIndex, uPrefetchIndex, pTrack);
    }

    if (HXR_OK != rc)
    {
	goto cleanup;
    }

    if (!m_pPrefetchTrackMap)
    {
	m_pPrefetchTrackMap = new CHXMapLongToObj;
    }

    (*m_pPrefetchTrackMap)[uPrefetchIndex] = pTrack;
    pTrack->AddRef();

    m_uPrefetchTrackCount++;

    if (m_pPrefetchSinkList)
    {
	CHXSimpleList::Iterator ndx = m_pPrefetchSinkList->Begin();
	for (; ndx != m_pPrefetchSinkList->End(); ++ndx)
	{
	    IHXPrefetchSink* pPrefetchSink = (IHXPrefetchSink*) (*ndx);
	    pPrefetchSink->PrefetchTrackAdded(m_uGroupIndex, uPrefetchIndex, pTrack);
	}
    }

cleanup:

    HX_RELEASE(pPrefetch);

    return rc;
}

/************************************************************************
*  Method:
*      IHXPrefetch::RemovePrefetchTrack
*  Purpose:
*      removes prefetched track
*/
STDMETHODIMP
HXAdvancedGroup::RemovePrefetchTrack(UINT16 /*IN*/ uTrackIndex)
{
    HX_RESULT		rc = HXR_OK;
    int			i = 0;
    IHXValues*		pTrack = NULL;
    IHXValues*		pUpdateValues = NULL;
    CHXMapLongToObj*	pNewPrefetchTrackMap = NULL;

    if (!m_pPrefetchTrackMap)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    if (!m_pPrefetchTrackMap->Lookup(uTrackIndex, (void*&)pTrack))
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    m_pPrefetchTrackMap->RemoveKey(uTrackIndex);

    if (HXR_OK != m_pPlayer->RemovePrefetchTrack(m_uGroupIndex, uTrackIndex, pTrack))
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    if (m_pPrefetchSinkList)
    {
	CHXSimpleList::Iterator ndx = m_pPrefetchSinkList->Begin();
	for (; ndx != m_pPrefetchSinkList->End(); ++ndx)
	{
	    IHXPrefetchSink* pPrefetchSink = (IHXPrefetchSink*) (*ndx);
	    pPrefetchSink->PrefetchTrackRemoved(m_uGroupIndex, uTrackIndex, pTrack);
	}
    }

    HX_RELEASE(pTrack);

    // adjust track map index
    pNewPrefetchTrackMap = new CHXMapLongToObj;
    for (i = 0; i < uTrackIndex; i++)
    {
	pTrack = (IHXValues*)(*m_pPrefetchTrackMap)[i];
	HX_ASSERT(pTrack);
	(*pNewPrefetchTrackMap)[i] = pTrack;
    }

    for (i = uTrackIndex + 1; i < m_uPrefetchTrackCount; i++)
    {
	pTrack = (IHXValues*)(*m_pPrefetchTrackMap)[i];	
	HX_ASSERT(pTrack);
	(*pNewPrefetchTrackMap)[i - 1] = pTrack;

	if (HXR_OK == CreateValuesCCF(pUpdateValues, (IUnknown*)(IHXPlayer*)m_pPlayer))
	{
	    pUpdateValues->SetPropertyULONG32("TrackIndex", i - 1);
	    m_pPlayer->UpdatePrefetchTrack(m_uGroupIndex, i, pUpdateValues);
	    HX_RELEASE(pUpdateValues);
	}
    }
    HX_DELETE(m_pPrefetchTrackMap);

    m_pPrefetchTrackMap = pNewPrefetchTrackMap;
    m_uPrefetchTrackCount--;

cleanup:

    return rc;
}

/************************************************************************
*  Method:
*      IHXPrefetch::GetPrefetchTrackCount
*  Purpose:
*      get number of prefetch tracks added
*/
STDMETHODIMP_(UINT16)
HXAdvancedGroup::GetPrefetchTrackCount()
{
    return m_uPrefetchTrackCount;
}

/************************************************************************
*  Method:
*      IHXPrefetch::GetPrefetchTrack
*  Purpose:
*      get prefetch track based on the index
*/
STDMETHODIMP
HXAdvancedGroup::GetPrefetchTrack(UINT16            /*IN*/  uTrackIndex,
			          REF(IHXValues*)   /*OUT*/ pTrack)
{
    HX_RESULT	rc = HXR_OK;

    pTrack = NULL;

    if (!m_pPrefetchTrackMap)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    if (!m_pPrefetchTrackMap->Lookup(uTrackIndex, (void*&)pTrack))
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

cleanup:

    return rc;
}

/************************************************************************
*  Method:
*      IHXPrefetch::AddPrefetchSink
*  Purpose:
*      add prefetch sink
*/
STDMETHODIMP
HXAdvancedGroup::AddPrefetchSink(IHXPrefetchSink* /*IN*/ pSink)
{
    HX_RESULT	rc = HXR_OK;

    if (!pSink)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    if (!m_pPrefetchSinkList)
    {
	m_pPrefetchSinkList = new CHXSimpleList;
    }

    m_pPrefetchSinkList->AddTail(pSink);
    pSink->AddRef();

cleanup:

    return rc;
}

/************************************************************************
*  Method:
*      IHXPrefetch::RemovePrefetchSink
*  Purpose:
*      remove prefetch sink
*/
STDMETHODIMP
HXAdvancedGroup::RemovePrefetchSink(IHXPrefetchSink* /*IN*/ pSink)
{
    HX_RESULT	    rc = HXR_OK;
    LISTPOSITION    lPosition = 0;

    if (!m_pPrefetchSinkList || !pSink)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    lPosition = m_pPrefetchSinkList->Find(pSink);
    if (!lPosition)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    m_pPrefetchSinkList->RemoveAt(lPosition);
    HX_RELEASE(pSink);
    
cleanup:

    return rc;
}

/************************************************************************
*  Method:
*	    IHXPrefetchSink::PrefetchTrackAdded()
*  Purpose:
*	    prefetch track is added
*/
STDMETHODIMP
HXAdvancedGroup::PrefetchTrackAdded(UINT16      uGroupIndex,
			            UINT16      uTrackIndex,
			            IHXValues*  pTrack)
{
    return HXR_OK;
}

/************************************************************************
*  Method:
*	    IHXPrefetchSink::PrefetchTrackRemoved()
*  Purpose:
*	    prefetch track is removed
*/
STDMETHODIMP
HXAdvancedGroup::PrefetchTrackRemoved(UINT16	    uGroupIndex,
			              UINT16        uTrackIndex,
			              IHXValues*    pTrack)
{
    return HXR_OK;
}

/************************************************************************
*  Method:
*	    IHXPrefetchSink::PrefetchTrackDone()
*  Purpose:
*	    prefetch track is done
*/
STDMETHODIMP
HXAdvancedGroup::PrefetchTrackDone(UINT16	uGroupIndex,
			           UINT16	uTrackIndex,			    
			           HX_RESULT	status)
{
    HX_RESULT	rc = HXR_OK;

    if (m_pPrefetchSinkList)
    {
	CHXSimpleList::Iterator ndx = m_pPrefetchSinkList->Begin();
	for (; ndx != m_pPrefetchSinkList->End(); ++ndx)
	{
	    IHXPrefetchSink* pPrefetchSink = (IHXPrefetchSink*) (*ndx);
	    pPrefetchSink->PrefetchTrackDone(uGroupIndex, uTrackIndex, status);
	}
    }

    return rc;
}
#endif /* HELIX_FEATURE_PREFETCH */

void
HXAdvancedGroup::Close(void)
{
    HXBasicGroup::Close();

    HX_RELEASE(m_pGroupProperties);

    CHXMapLongToObj::Iterator i;

    m_uPrefetchTrackCount = 0;

    if (m_pTrackSinkList)
    {
	CHXSimpleList::Iterator ndx = m_pTrackSinkList->Begin();
	for (; ndx != m_pTrackSinkList->End(); ++ndx)
	{
	    IHXTrackSink* pTrackSink = (IHXTrackSink*) (*ndx);
	    HX_RELEASE(pTrackSink);
	}
	HX_DELETE(m_pTrackSinkList);
    }

#if defined(HELIX_FEATURE_PREFETCH)
    IHXPrefetch*   pPrefetch = NULL;
	CHXSimpleList::Iterator j;
    if (m_pPrefetchTrackMap)
    {
	i = m_pPrefetchTrackMap->Begin();
	for(; i != m_pPrefetchTrackMap->End(); ++i)
	{
	    IHXValues* pTrack = (IHXValues*)(*i);
	    HX_RELEASE(pTrack);
	}
	HX_DELETE(m_pPrefetchTrackMap);
    }

    if (m_pPrefetchSinkList)
    {
	j = m_pPrefetchSinkList->Begin();
	for (; j != m_pPrefetchSinkList->End(); ++j)
	{
	    IHXPrefetchSink* pPrefetchSink = (IHXPrefetchSink*) (*j);
	    HX_RELEASE(pPrefetchSink);
	}
	HX_DELETE(m_pPrefetchSinkList);
    }

    if (m_bPrefetchSinkAdded)
    {
	if (HXR_OK == m_pPlayer->QueryInterface(IID_IHXPrefetch, (void**)&pPrefetch))
	{
	    pPrefetch->RemovePrefetchSink(this);
	    m_bPrefetchSinkAdded = FALSE;
	}
	HX_RELEASE(pPrefetch);
    }
#endif /* HELIX_FEATURE_PREFETCH */

    if (m_pPersistentComponentPropertyMap)
    {
	i = m_pPersistentComponentPropertyMap->Begin();
	for(; i != m_pPersistentComponentPropertyMap->End(); ++i)
	{
	    IHXValues* pValues = (IHXValues*)(*i);
	    HX_RELEASE(pValues);
	}
	HX_DELETE(m_pPersistentComponentPropertyMap);
    }
}

/*HXAdvancedGroupManager*/

HXAdvancedGroupManager::HXAdvancedGroupManager(HXPlayer* pPlayer)
                       :HXBasicGroupManager(pPlayer)
                       ,m_pMasterTAC(NULL)

{
}

HXAdvancedGroupManager::~HXAdvancedGroupManager(void)
{
    Close();
}

void HXAdvancedGroupManager::SetMasterTAC(HXMasterTAC* pMasterTAC)
{
    m_pMasterTAC = pMasterTAC;
    m_pMasterTAC->AddRef();
}

void
HXAdvancedGroupManager::PersistentComponentAdded(UINT16 uGroupIndex, UINT16 uTrackIndex)
{
    HXAdvancedGroup* pGroup = NULL;

    if (HXR_OK == GetGroup(uGroupIndex, (IHXGroup*&)pGroup))
    {
	pGroup->PersistentComponentAdded(uTrackIndex);
    }
    HX_RELEASE(pGroup);

    return;
}

void 
HXAdvancedGroupManager::PersistentComponentRemoved(UINT16 uGroupIndex, UINT16 uTrackIndex)
{    
    HXAdvancedGroup* pGroup = NULL;

    if (HXR_OK == GetGroup(uGroupIndex, (IHXGroup*&)pGroup))
    {
	pGroup->PersistentComponentRemoved(uTrackIndex);
    }

    return;
}

/*
 *  IHXGroupManager methods
 */

/************************************************************************
*  Method:
*      IHXGroupManager::CreateGroup
*  Purpose:
*		Create a group
*/
STDMETHODIMP
HXAdvancedGroupManager::CreateGroup(REF(IHXGroup*) pGroup)
{
    pGroup = new HXAdvancedGroup(this);
    if (!pGroup)
    {
	return HXR_OUTOFMEMORY;
    }
    
    pGroup->AddRef();

    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXGroupManager::AddGroup
*  Purpose:
*		Add a group to the presentation.
*/
STDMETHODIMP
HXAdvancedGroupManager::AddGroup(IHXGroup*	/*IN*/ pGroup)
{
    HX_RESULT	        theErr = HXR_OK;
    UINT16	        uInsertGroupAt = 0;
    UINT32	        ulLastGroupInRAM20 = 0;
    UINT32	        ulPersistentComponentID = 0;
    IHXValues*	        pGroupProperties = NULL;
    IHXPersistentComponent* pPersistentComponent = NULL;
    HXPersistentComponentManager* pPersistentComponentManager = NULL;

    if (!pGroup)
    {
	return HXR_UNEXPECTED;
    }
    
    pGroupProperties = ((HXAdvancedGroup*)pGroup)->m_pGroupProperties;

#if defined(HELIX_FEATURE_NESTEDMETA)
    if (pGroupProperties)
    {
	if (HXR_OK != pGroupProperties->GetPropertyULONG32("LastGroupInRAM20", ulLastGroupInRAM20) &&
	    HXR_OK == pGroupProperties->GetPropertyULONG32("PersistentComponentID", ulPersistentComponentID))
	{
	    pPersistentComponentManager = m_pPlayer->m_pPersistentComponentManager;	
	    if (HXR_OK == pPersistentComponentManager->GetPersistentComponent(ulPersistentComponentID,
									      pPersistentComponent))
	    {
		((HXPersistentComponent*)pPersistentComponent)->m_uGroups++;
		uInsertGroupAt = ((HXPersistentComponent*)pPersistentComponent)->m_pSourceInfo->m_uGroupID +
				 ((HXPersistentComponent*)pPersistentComponent)->m_uGroups;

		theErr = InsertGroupAt(uInsertGroupAt, pGroup);
	    }
	    else
	    {
		HX_ASSERT(FALSE);
	    }
	}
	else
	{
	    InsertGroupAt(m_uGroupCount, pGroup);
	}
    }
    else
#endif /* HELIX_FEATURE_NESTEDMETA */
    {
	InsertGroupAt(m_uGroupCount, pGroup);
    }
    
    HX_RELEASE(pPersistentComponent);

    return theErr;
}

/************************************************************************
*  Method:
*      IHXGroupManager::RemoveGroup
*  Purpose:
*		Remove an already added group
*/
STDMETHODIMP
HXAdvancedGroupManager::RemoveGroup(UINT16 	/*IN*/ uGroupIndex)
{
    HX_RESULT		theErr = HXR_OK;
    int			i = 0;
    CHXMapLongToObj*	pNewGroupMap = NULL;
    IHXGroup*		pHXGroupRemoved = NULL;
    IHXGroup*		pHXGroup = NULL;
    CHXSimpleList::Iterator ndx;

    if (!m_pGroupMap->Lookup(uGroupIndex, (void*&)pHXGroupRemoved))
    {
	theErr = HXR_UNEXPECTED;
	goto cleanup;
    }

    m_pGroupMap->RemoveKey(uGroupIndex);

    // adjust track map index
    pNewGroupMap = new CHXMapLongToObj;
    for (i = 0; i < uGroupIndex; i++)
    {
	m_pGroupMap->Lookup(i, (void*&)pHXGroup);
	HX_ASSERT(pHXGroup);

	(*pNewGroupMap)[i] = pHXGroup;
    }

    for (i = uGroupIndex + 1; i < m_uGroupCount; i++)
    {
	m_pGroupMap->Lookup(i, (void*&)pHXGroup);
	HX_ASSERT(pHXGroup);

	((HXAdvancedGroup*)pHXGroup)->m_uGroupIndex = i - 1;
	(*pNewGroupMap)[i - 1] = pHXGroup;
    }
    HX_DELETE(m_pGroupMap);

    m_pGroupMap = pNewGroupMap;
    m_uGroupCount--;
    
    ndx = m_pSinkList->Begin();
    for (; ndx != m_pSinkList->End(); ++ndx)
    {
	IHXGroupSink* pGroupSink = (IHXGroupSink*) (*ndx);
	pGroupSink->GroupRemoved(uGroupIndex, pHXGroupRemoved);
    }

    if (m_uCurrentGroup == uGroupIndex)
    {
	m_bCurrentGroupInitialized = FALSE;
    }

cleanup:

    HX_RELEASE(pHXGroupRemoved);

    return theErr;
}    

STDMETHODIMP
HXAdvancedGroupManager::SetCurrentGroup(UINT16 uGroupIndex)
{
    HX_RESULT res = HXBasicGroupManager::SetCurrentGroup(uGroupIndex);

#if defined(HELIX_FEATURE_MASTERTAC)
    if (SUCCEEDED(res) && m_pMasterTAC)
    {
       // check for TAC info from new group
       m_pMasterTAC->ResetTAC(TRUE, TRUE); // reset status & clear master props
       m_pMasterTAC->CheckGroupForTACInfo(uGroupIndex);
    }
#endif // defined(HELIX_FEATURE_MASTERTAC)

    return res;
}

HX_RESULT	    
HXAdvancedGroupManager::RepeatTrackAdded(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pTrack)
{
    return m_pPlayer->RepeatTrackAdded(uGroupIndex, uTrackIndex, pTrack);
}

void
HXAdvancedGroupManager::Close(void)
{
    HX_RELEASE(m_pMasterTAC);
    HXBasicGroupManager::Close();
}
