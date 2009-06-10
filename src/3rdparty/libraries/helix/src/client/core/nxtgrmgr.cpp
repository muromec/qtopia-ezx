/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: nxtgrmgr.cpp,v 1.6 2007/07/06 21:58:11 jfinnecy Exp $
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
#include "hxgroup.h"
#include "hxcore.h"
#include "hxengin.h"
#include "hxslist.h"
#include "hxstring.h"
#include "hxplay.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "nxtgrmgr.h"

#include "srcinfo.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

NextGroupManager::NextGroupManager(HXPlayer* pPlayer)
    : m_pPlayer(NULL)
    , m_pInterruptState(NULL)
    , m_pSourceList(NULL)
    , m_LastError(HXR_OK)
    , m_uGroupNumber(0)
    , m_pGroup(NULL)
    , m_pErrorSource(NULL)
    , m_bStopPrefetch(FALSE)
{
    m_pPlayer	    = pPlayer;
    m_pSourceList   = new CHXSimpleList;
    m_UserString    = "";

    m_pPlayer->AddRef();
}

NextGroupManager::~NextGroupManager()
{
    Cleanup();

    HX_RELEASE(m_pPlayer);
    HX_RELEASE(m_pInterruptState);
    HX_DELETE(m_pSourceList);
    m_pSourceList = 0;
}

void
NextGroupManager::Init()
{
    m_pPlayer->QueryInterface(IID_IHXInterruptState, (void**) &m_pInterruptState);
}

HX_RESULT	
NextGroupManager::SetCurrentGroup(UINT16 uGroupNumber, IHXGroup* pGroup)
{
    Cleanup();

    m_uGroupNumber  = uGroupNumber;
    m_pGroup	    = pGroup;
    m_pGroup->AddRef();

    return HXR_OK;
}

HX_RESULT	
NextGroupManager::GetCurrentGroup(UINT16& uCurrentGroup, IHXGroup*& pGroup)
{
    if (m_pGroup)
    {
	uCurrentGroup   = m_uGroupNumber;
	pGroup		= m_pGroup;
	pGroup->AddRef();
	return HXR_OK;
    }

    return HXR_NO_DATA;
}

HX_RESULT	
NextGroupManager::AddSource(SourceInfo* pSourceInfo)
{
    HX_ASSERT(pSourceInfo);
    
    m_pSourceList->AddTail(pSourceInfo);

    return HXR_OK;
}

UINT16	
NextGroupManager::GetNumSources(void)
{
    return m_pSourceList->GetCount();
}

HX_RESULT	
NextGroupManager::GetSource(UINT16 uIndex, SourceInfo*& pSourceInfo)
{
    HX_ASSERT(uIndex < m_pSourceList->GetCount());

    LISTPOSITION lPos = m_pSourceList->FindIndex((int) uIndex);
    if (!lPos)
    {
	pSourceInfo = NULL;
	return HXR_INVALID_PARAMETER;
    }

    pSourceInfo = (SourceInfo*) m_pSourceList->GetAt(lPos);
    return HXR_OK;
}
    
HX_RESULT	
NextGroupManager::RemoveSource(SourceInfo* pSourceInfo)
{    
    HXBOOL    bFound = FALSE;
    UINT16  nIndex = 0;
    LISTPOSITION lPos = 0;

    CHXSimpleList::Iterator ndx = m_pSourceList->Begin();
    for (; ndx != m_pSourceList->End(); ++ndx, nIndex++)
    {
	SourceInfo* pSrcInfo = (SourceInfo*) (*ndx);
	if (pSrcInfo == pSourceInfo)
	{    	    
	    bFound = TRUE;
	    break;
	}
    }

    if (bFound)
    {
	lPos = m_pSourceList->FindIndex(nIndex);
	m_pSourceList->RemoveAt(lPos);
    }

    m_LastError = HXR_OK;

    return HXR_OK;
}


HX_RESULT	
NextGroupManager::RemoveSource(UINT16 uIndex, SourceInfo*& pSourceInfo)
{
    HX_ASSERT(uIndex < m_pSourceList->GetCount());

    LISTPOSITION lPos = m_pSourceList->FindIndex((int) uIndex);
    if (!lPos)
    {
	pSourceInfo = NULL;
	return HXR_INVALID_PARAMETER;
    }

    pSourceInfo = (SourceInfo*) m_pSourceList->RemoveAt(lPos);
    return HXR_OK;
}

void
NextGroupManager::RemoveAllSources(void)
{
    m_pSourceList->RemoveAll();
}
    
HX_RESULT	
NextGroupManager::ProcessIdle(void)
{
    UINT32 ulDuration = 0;
    UINT32 ulDelay = 0;

    if (m_bStopPrefetch)
    {
	return HXR_OK;
    }

    HX_RESULT theErr = HXR_OK;
    CHXSimpleList::Iterator ndx = m_pSourceList->Begin();
    for (; !m_LastError && !theErr && ndx != m_pSourceList->End(); ++ndx)
    {
	SourceInfo* pSourceInfo = (SourceInfo*)(*ndx);

	if (!pSourceInfo->m_pSource)
	{
	    continue;
	}

	if (pSourceInfo->m_bToBeResumed && 
	    pSourceInfo->m_pSource->IsInitialized() &&
	    !m_pInterruptState->AtInterruptTime())
	{
	    if (!pSourceInfo->m_bIsTrackDurationSet)
	    {
		if (pSourceInfo->m_pRendererAdviseSink)
		{
		    ulDuration = pSourceInfo->m_pSource->GetDuration();
		    ulDelay = pSourceInfo->m_pSource->GetDelay();

		    //Fixes PR 27831: make sure the total track dur is
		    // being set for all groups otherwise a duration of
		    // 0 results and SMIL's time-bounds checking for
		    // whether or not a hyperlink is active always
		    // results in false (for non-fill="freeze" sources).
		    // This line is copied from SourceInfo::SetupRenderer()
		    // inside its
		    // 	if (!m_bIsTrackDurationSet)
		    // block of code:
		    pSourceInfo->m_ulTrackDuration = pSourceInfo->m_pSource->GetDuration();

		    pSourceInfo->m_pRendererAdviseSink->
			TrackDurationSet(pSourceInfo->m_uGroupID,
				       pSourceInfo->m_uTrackID,
				       ulDuration,
				       ulDelay,
				       pSourceInfo->m_pSource->IsLive());
		    //Moved this inside the if() because that's what makes
		    // sense; if the above if did not get entered, then we
		    // didn't call TrackDurationSet() and thus we shouldn't
		    // set the following HXBOOL to TRUE outside the if:
		    pSourceInfo->m_bIsTrackDurationSet = TRUE;
		}

		theErr = pSourceInfo->SetupStreams();
	    }

	    if (pSourceInfo->m_pSource && 
		(pSourceInfo->m_pSource->CanBeResumed() ||
		(pSourceInfo->m_pSource->IsDelayed() && pSourceInfo->m_pSource->TryResume())))
	    {   
		pSourceInfo->m_bToBeResumed = FALSE;

		pSourceInfo->Register();
		m_pPlayer->RegisterSourcesDone();
		theErr = pSourceInfo->m_pSource->DoResume();
	    }
	}

	if (pSourceInfo->m_bToBeResumed || !pSourceInfo->m_pSource->IsPreBufferingDone()
	    || pSourceInfo->m_pSource->IsLive())
	{
	    theErr = pSourceInfo->m_pSource->ProcessIdle();
	}
	else
	{
	    pSourceInfo->m_pSource->DoPause();
	}
    }

    if (theErr && !m_LastError)
    {
	m_LastError = theErr;
    }

    return HXR_OK;
}

HXBOOL
NextGroupManager::ReportError(HXSource* pSource, HX_RESULT theErr, 
			      const char* pUserString)
{
    CHXSimpleList::Iterator ndx = m_pSourceList->Begin();
    for (; ndx != m_pSourceList->End(); ++ndx)
    {
	SourceInfo* pSourceInfo = (SourceInfo*) (*ndx);
	if (pSourceInfo->m_pSource == pSource)
	{	    	 
	    m_LastError	    = theErr;
	    m_UserString    = pUserString;
	    m_pErrorSource  = pSource;
	    return TRUE;	    
	}
    }

    return FALSE;
}

HX_RESULT
NextGroupManager::CanBeStarted(HXSource* pSource, SourceInfo* pThisSourceInfo)
{
    UINT32 ulDelay = pSource->GetDelay();

    if (ulDelay == 0 || !pThisSourceInfo)
    {
	return TRUE;
    }

    CHXSimpleList::Iterator ndxSources = m_pSourceList->Begin();
    /* Check if we are done. This may be TRUE for empty files */
    for (; ndxSources != m_pSourceList->End(); ++ndxSources)
    {
	SourceInfo* pSourceInfo = (SourceInfo*) (*ndxSources);
	if (!pSourceInfo->m_pSource || 
	    pSourceInfo->m_pSource->IsSourceDone() ||
	    !pSourceInfo->m_pSource->IsInitialized())
	{
	    continue;
	}

#ifdef SEQ_DEPENDENCY
	int iRetVal = 0;
	if (!pSourceInfo->m_pSource->IsLive() &&
	    IsDependent(pThisSourceInfo, pSourceInfo) && 
	    !pSourceInfo->m_pSource->IsSourceDone())
	{
	    return FALSE;
	}
#else
	if (!pSourceInfo->m_pSource->IsLive() &&
	     pSourceInfo->m_pSource->GetDuration() <= ulDelay &&
	    !pSourceInfo->m_pSource->IsSourceDone())
	{
	    return FALSE;
	}
#endif /*SEQ_DEPENDENCY*/
	
    }

    return TRUE;
}
    
HXBOOL
NextGroupManager::Lookup(HXSource* pSource, SourceInfo*& pSourceInfo)
{
    HXBOOL bResult = FALSE;

    pSourceInfo = NULL;

    CHXSimpleList::Iterator ndx = m_pSourceList->Begin();
    for (; ndx != m_pSourceList->End(); ++ndx)
    {
	SourceInfo* pSrcInfo = (SourceInfo*) (*ndx);
	if (pSrcInfo->m_pSource == pSource)
	{
	    pSourceInfo = pSrcInfo;
	    bResult = TRUE;
	    break;
	}
    }

    return bResult;
}

void
NextGroupManager::Cleanup(void)
{
    CHXSimpleList::Iterator ndx = m_pSourceList->Begin();
    for (; ndx != m_pSourceList->End(); ++ndx)
    {
	SourceInfo* pSourceInfo = (SourceInfo*) (*ndx);
	
	pSourceInfo->Stop();

	pSourceInfo->CloseRenderers();

	HX_RELEASE(pSourceInfo->m_pStatus);

	if (pSourceInfo->m_pSource)
	{
	    // cleanup (i.e. registry)
	    pSourceInfo->m_pSource->DoCleanup();
	    pSourceInfo->m_pSource->Release();
	    pSourceInfo->m_pSource = 0;
	}
	delete pSourceInfo;
    }

    m_pSourceList->RemoveAll();
    HX_RELEASE(m_pGroup);

    m_LastError	    = HXR_OK;
    m_UserString    = "";
    m_pErrorSource  = NULL;
    m_bStopPrefetch = FALSE;
}


void
NextGroupManager::SetLastError(HX_RESULT theErr, HXSource* pSource, char* pUserString)
{
    if (!m_LastError)
    {
	m_LastError	= theErr;
	m_pErrorSource	= pSource; 
	m_UserString	= pUserString;
    }
}

void
NextGroupManager::StopPreFetch()
{
    if (m_bStopPrefetch)
    {
	return;
    }

    m_bStopPrefetch = TRUE;

    CHXSimpleList::Iterator ndx = m_pSourceList->Begin();
    for (; ndx != m_pSourceList->End(); ++ndx)
    {
	SourceInfo* pSourceInfo = (SourceInfo*)(*ndx);

	if (!pSourceInfo->m_pSource)
	{
	    continue;
	}

	if (!pSourceInfo->m_bToBeResumed)
	{
	    pSourceInfo->m_bToBeResumed = TRUE;
	    pSourceInfo->m_pSource->DoPause();
	    pSourceInfo->UnRegister();
	}
    }
}

void
NextGroupManager::ContinuePreFetch()
{
    m_bStopPrefetch = FALSE;
}

HX_RESULT	
NextGroupManager::AddRepeatTrack(UINT16 uTrackIndex, IHXValues* pTrack)
{
    SourceInfo*	pSourceInfo = NULL;

    if (HXR_OK == GetSourceInfo(uTrackIndex, pSourceInfo) && pSourceInfo)
    {
	return pSourceInfo->AppendRepeatRequest(uTrackIndex, pTrack);
    }
    else
    {
	HX_ASSERT(FALSE);
	return HXR_UNEXPECTED;
    }
}

HX_RESULT
NextGroupManager::GetSourceInfo(UINT16 uTrackIndex, SourceInfo*& pSourceInfo)
{
    HX_RESULT	hr = HXR_OK;
    SourceInfo*	pTempSourceInfo = NULL;
    CHXSimpleList::Iterator ndxSource;

    pSourceInfo = NULL;

    if (m_pSourceList)
    {
	// find the sourceinfo
	ndxSource = m_pSourceList->Begin();
	for (; ndxSource != m_pSourceList->End(); ++ndxSource)
	{	
	    pTempSourceInfo = (SourceInfo*)(*ndxSource);
    	    if (pTempSourceInfo->m_uTrackID == uTrackIndex)
	    {
		pSourceInfo = pTempSourceInfo;
		break;
	    }
	}
    }

    return hr;
}
