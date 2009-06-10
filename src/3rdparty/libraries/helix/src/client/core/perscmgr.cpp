/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: perscmgr.cpp,v 1.17 2007/07/06 21:58:11 jfinnecy Exp $
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
#include "hxstrutl.h"
#include "hxstring.h"
#include "hxmap.h"
#include "smiltype.h"
#include "hxausvc.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxgroup.h"
#include "basgroup.h"
#include "advgroup.h"
#include "hxplay.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "hxplugn.h"
#include "hxrendr.h"
#include "srcinfo.h"
#include "sitemgr.h"
#include "pckunpck.h"
#include "perscmgr.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

HXPersistentComponent::HXPersistentComponent(HXPersistentComponentManager* pManager)
    : m_lRefCount(0)
    , m_bInitialized(FALSE)
    , m_bToBeClosed(FALSE)
    , m_bCleanupLayoutCalled(FALSE)
    , m_uGroups(0)
    , m_uTracks(0)
    , m_ulComponentID(0)
    , m_ulPersistentType(0)
    , m_pSourceInfo(NULL)
    , m_pPersistentParent(NULL)
    , m_pPersistentChildList(NULL)
    , m_pProperties(NULL)
    , m_pPersistentRenderer(NULL)
    , m_pRendererAdviseSink(NULL)
    , m_pGroupSink(NULL)
    , m_pComponentManager(NULL)
{
    m_pComponentManager = pManager;    
    HX_ADDREF(m_pComponentManager);
}

HXPersistentComponent::~HXPersistentComponent(void)
{
    Remove();

    HX_DELETE(m_pPersistentChildList);
    HX_RELEASE(m_pPersistentParent);
    HX_RELEASE(m_pComponentManager);
}

/*
 *  IUnknown methods
 */
STDMETHODIMP HXPersistentComponent::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPersistentComponent), (IHXPersistentComponent*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPersistentComponent*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXPersistentComponent::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXPersistentComponent::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *	Method:
 *	    IHXPersistentComponent::Init
 *	Purpose:
 *	    initialize persistent component
 */
STDMETHODIMP
HXPersistentComponent::Init(IHXPersistentRenderer* pPersistentRenderer)
{
    m_bInitialized = TRUE;

    m_pPersistentRenderer = pPersistentRenderer;
    HX_ADDREF(m_pPersistentRenderer);

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXPersistentComponent::AddRendererAdviseSink
 *	Purpose:
 *	    add renderer advise sink
 */
STDMETHODIMP
HXPersistentComponent::AddRendererAdviseSink(IHXRendererAdviseSink* pSink)
{
    m_pRendererAdviseSink = pSink;
    HX_ADDREF(m_pRendererAdviseSink);

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXPersistentComponent::RemoveRendererAdviseSink
 *	Purpose:
 *	    remove renderer advise sink
 */
STDMETHODIMP
HXPersistentComponent::RemoveRendererAdviseSink(IHXRendererAdviseSink* pSink)
{    
    HX_RELEASE(m_pRendererAdviseSink);

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXPersistentComponent::AddGroupSink
 *	Purpose:
 *	    add renderer advise sink
 */
STDMETHODIMP
HXPersistentComponent::AddGroupSink(IHXGroupSink* pSink)
{
    m_pGroupSink = pSink;
    HX_ADDREF(m_pGroupSink);

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXPersistentComponent::RemoveGroupSink
 *	Purpose:
 *	    remove renderer advise sink
 */
STDMETHODIMP
HXPersistentComponent::RemoveGroupSink(IHXGroupSink* pSink)
{
    HX_RELEASE(m_pGroupSink);

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXPersistentComponent::GetPersistentRenderer
 *	Purpose:
 *	    get persistent renderer
 */
STDMETHODIMP
HXPersistentComponent::GetPersistentRenderer(REF(IHXPersistentRenderer*) pPersistentRenderer)
{
    HX_RESULT	rc = HXR_OK;

    pPersistentRenderer = m_pPersistentRenderer;
    HX_ADDREF(pPersistentRenderer);

    return rc;
}
    
/************************************************************************
 *	Method:
 *	    IHXPersistentComponent::GetPersistentProperties
 *	Purpose:
 *	    get persistent component properties
 */
STDMETHODIMP
HXPersistentComponent::GetPersistentProperties(REF(IHXValues*) pProperties)
{
    pProperties = m_pProperties;
    if (pProperties)
    {
	pProperties->AddRef();
	return HXR_OK;
    }

    return HXR_FAILED;
}

HX_RESULT	
HXPersistentComponent::GetPersistentComponent(UINT32				ulComponentID, 
					       REF(IHXPersistentComponent*)	pComponent)
{
    HX_RESULT		    rc = HXR_FAILED;
    HXPersistentComponent* pHXPersistentComponent = NULL;
    CHXSimpleList::Iterator i;

    pComponent = NULL;

    if (m_ulComponentID == ulComponentID)
    {
	QueryInterface(IID_IHXPersistentComponent, (void**)&pComponent);
	rc = HXR_OK;
    }
    else if (m_pPersistentChildList)
    {
	for(i = m_pPersistentChildList->Begin();i != m_pPersistentChildList->End();++i)
	{
	    pHXPersistentComponent = (HXPersistentComponent*)(*i);
	    rc = pHXPersistentComponent->GetPersistentComponent(ulComponentID, pComponent);

	    if (HXR_OK == rc && pComponent)
	    {
		break;
	    }
	}
    }

    return rc;
}

HX_RESULT
HXPersistentComponent::CurrentGroupSet(UINT16 uGroupIndex, IHXGroup* pGroup)
{
    HX_RESULT		    rc = HXR_OK;
    HXPersistentComponent* pHXPersistentComponent = NULL;
    CHXSimpleList::Iterator i;

    if (m_pPersistentChildList)
    {
	for(i = m_pPersistentChildList->Begin();i != m_pPersistentChildList->End();++i)
	{
	    pHXPersistentComponent = (HXPersistentComponent*)(*i);
	    rc = pHXPersistentComponent->CurrentGroupSet(uGroupIndex, pGroup);
	}
    }

    m_pSourceInfo->Reset();	

    if (m_pGroupSink)
    {
	rc = m_pGroupSink->CurrentGroupSet(uGroupIndex, pGroup);
    }

    return rc;
}

HX_RESULT	
HXPersistentComponent::OnTimeSync(UINT32 ulCurrentTime)
{
    HX_RESULT		    rc = HXR_OK;
    HXPersistentComponent* pHXPersistentComponent = NULL;
    CHXSimpleList::Iterator i;

    if (m_pPersistentChildList)
    {
	for(i = m_pPersistentChildList->Begin();i != m_pPersistentChildList->End();++i)
	{
	    pHXPersistentComponent = (HXPersistentComponent*)(*i);
	    rc = pHXPersistentComponent->OnTimeSync(ulCurrentTime);
	}
    }

    if (m_pSourceInfo)
    {
	m_pSourceInfo->OnTimeSync(ulCurrentTime);
    }

   return rc;
}

UINT32
HXPersistentComponent::GetPersistentComponentCount()
{
    UINT32 ulTotalCount = 0;
    HXPersistentComponent* pHXPersistentComponent = NULL;
    CHXSimpleList::Iterator i;

    if (m_pPersistentChildList)
    {
	for(i = m_pPersistentChildList->Begin();i != m_pPersistentChildList->End();++i)
	{
	    pHXPersistentComponent = (HXPersistentComponent*)(*i);
	    ulTotalCount += pHXPersistentComponent->GetPersistentComponentCount();
	}
    }

    return (ulTotalCount + 1);
}

void	
HXPersistentComponent::TrackUpdated(UINT16 uGroupIndex, 
				     UINT16 uTrackIndex, 
				     IHXValues* pValues)
{
    HXPersistentComponent* pHXPersistentComponent = NULL;
    CHXSimpleList::Iterator i;

    if (m_pPersistentChildList)
    {
	for(i = m_pPersistentChildList->Begin();i != m_pPersistentChildList->End();++i)
	{
	    pHXPersistentComponent = (HXPersistentComponent*)(*i);
	    pHXPersistentComponent->TrackUpdated(uGroupIndex, uTrackIndex, pValues);
	}
    }

    if (m_pRendererAdviseSink)
    {
	m_pRendererAdviseSink->TrackUpdated(uGroupIndex, uTrackIndex, pValues);
    }

    return;
}

void	
HXPersistentComponent::AllRenderersClosed(void)
{
    HXPersistentComponent* pHXPersistentComponent = NULL;
    CHXSimpleList::Iterator i;

    if (m_pPersistentChildList)
    {
	for(i = m_pPersistentChildList->Begin();i != m_pPersistentChildList->End();++i)
	{
	    pHXPersistentComponent = (HXPersistentComponent*)(*i);
	    pHXPersistentComponent->AllRenderersClosed();
	}
    }

    if (m_pSourceInfo)
    {
	m_pSourceInfo->Reset();
    }

    return;
}

void
HXPersistentComponent::Reset(void)
{
    HXPersistentComponent* pHXPersistentComponent = NULL;
    CHXSimpleList::Iterator i;

    if (m_pPersistentChildList)
    {
	for(i = m_pPersistentChildList->Begin();i != m_pPersistentChildList->End();++i)
	{
	    pHXPersistentComponent = (HXPersistentComponent*)(*i);
	    pHXPersistentComponent->Reset();
	}
    }

    m_pComponentManager->m_pPlayer->m_pSourceMap->RemoveKey(m_pSourceInfo->m_pSource);
    m_pComponentManager->m_pPlayer->m_bSourceMapUpdated = TRUE;

    m_pSourceInfo->m_bIsPersistentSource = FALSE;
    m_pSourceInfo->Stop();

    m_bToBeClosed = TRUE;

    return;
}

void
HXPersistentComponent::Remove()
{
    HXPersistentComponent* pHXPersistentComponent = NULL;
    CHXSimpleList::Iterator i;

    if (m_pPersistentChildList)
    {
	for(i = m_pPersistentChildList->Begin();i != m_pPersistentChildList->End();++i)
	{
	    pHXPersistentComponent = (HXPersistentComponent*)(*i);
	    pHXPersistentComponent->Remove();
	    HX_RELEASE(pHXPersistentComponent);
	}
	m_pPersistentChildList->RemoveAll();
    }
    
    // If this's m_pSourceInfo is not already set to be closed, then
    // we don't want to delete it.  This happens, e.g., if persistent
    // component is ended early on an event.  (Is part of the fix for
    // PR 123782.)
    if (m_pSourceInfo  &&  m_bToBeClosed)
    {
	HX_ASSERT(!m_pSourceInfo->m_bIsPersistentSource);
	m_pSourceInfo->CloseRenderers();
	HX_DELETE(m_pSourceInfo);
    }

    HX_RELEASE(m_pProperties);
    HX_RELEASE(m_pRendererAdviseSink);
    HX_RELEASE(m_pGroupSink);
    HX_RELEASE(m_pPersistentRenderer);

    return;
}

HXPersistentComponentManager::HXPersistentComponentManager(HXPlayer* pPlayer)
    : m_lRefCount(0)
    , m_ulComponentIndex(0)
    , m_nCurrentGroup(0)
    , m_pPlayer(pPlayer)
    , m_pRootPersistentComponent(NULL)
{
    HX_ADDREF(m_pPlayer);
}

HXPersistentComponentManager::~HXPersistentComponentManager(void)
{
    Close();

    HX_RELEASE(m_pPlayer);
}

/*
 *  IUnknown methods
 */
STDMETHODIMP HXPersistentComponentManager::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPersistentComponentManager), (IHXPersistentComponentManager*)this },
            { GET_IIDHANDLE(IID_IHXGroupSink), (IHXGroupSink*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPersistentComponentManager*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXPersistentComponentManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXPersistentComponentManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *	Method:
 *	    IHXPersistentComponentManager::CreatePersistentComponent
 *	Purpose:
 *	    create persistent component
 */
STDMETHODIMP
HXPersistentComponentManager::CreatePersistentComponent(REF(IHXPersistentComponent*)   pPersistentComponent)
{
    pPersistentComponent = new HXPersistentComponent(this);    
    if (!pPersistentComponent)
    {
	return HXR_OUTOFMEMORY;
    }
    
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXPersistentComponentManager::AddPersistentComponent
 *	Purpose:
 *	    add persistent component
 */
STDMETHODIMP
HXPersistentComponentManager::AddPersistentComponent(IHXPersistentComponent*	pPersistentComponent)
{
    HX_RESULT			rc = HXR_OK;
    HXBOOL			bFound = FALSE;
    HXSource*			pSource = NULL;
    SourceInfo*			pSourceInfo = NULL;
    RendererInfo*		pRendInfo = NULL;
    HXPersistentComponent*	pHXPersistentComponent = NULL;
    HXPersistentComponent*	pHXPersistentParentComponent = NULL;
    IHXValues*			pProperties = NULL;
    IHXGroup*			pGroup = NULL;
    IHXPersistentComponent*	pPersistentParentComponent = NULL;
    IHXPersistentRenderer*	pPersistentRenderer = NULL;
    IHXRenderer*		pRenderer = NULL;    
    CHXMapPtrToPtr::Iterator ndxSource;
    CHXMapLongToObj::Iterator ndxRend;
    
    pHXPersistentComponent = (HXPersistentComponent*)pPersistentComponent;
    if (!pHXPersistentComponent || !pHXPersistentComponent->m_bInitialized)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    pPersistentRenderer = pHXPersistentComponent->m_pPersistentRenderer;
    if (HXR_OK != pPersistentRenderer->QueryInterface(IID_IHXRenderer, (void**)&pRenderer))
    {
	rc = HXR_INVALID_PARAMETER;
	goto cleanup;
    }

    // find component's source info - better way?
    ndxSource = m_pPlayer->m_pSourceMap->Begin();
    for (; ndxSource != m_pPlayer->m_pSourceMap->End() && !bFound; ++ndxSource)
    {
	pSourceInfo = (SourceInfo*)(*ndxSource);
	pSource = pSourceInfo->m_pSource;

	ndxRend = pSourceInfo->m_pRendererMap->Begin();
	for (; ndxRend != pSourceInfo->m_pRendererMap->End(); ++ndxRend)
	{
	    pRendInfo = (RendererInfo*) (*ndxRend);
	    if (pRendInfo->m_pRenderer == pRenderer)
	    {
		bFound = TRUE;
		break;
	    }
	}
    }

    if (!bFound)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    if (!m_pRootPersistentComponent)
    {
	m_ulComponentIndex = 0;
	m_pRootPersistentComponent = pHXPersistentComponent;
	m_pRootPersistentComponent->AddRef();

	// XXX HP TBD - when we support timing attributes on a SMIL URL
	// for now, we resets all timing attributes on the root persistent
	// component since there could be a case when the same timing values
	// are specified in a CGI URL sent to the player
	if (pSourceInfo && pSourceInfo->m_pSource)
	{	    
	    IHXValues* pValues = NULL;

	    if (HXR_OK == CreateValuesCCF(pValues, (IUnknown*)(IHXPlayer*)m_pPlayer))
	    {
		pValues->SetPropertyULONG32("Start", 0);
		pValues->SetPropertyULONG32("End", 0);
		pValues->SetPropertyULONG32("Delay", 0);
		pValues->SetPropertyULONG32("Duration", 0);

		pSourceInfo->m_pSource->UpdatePlayTimes(pValues);
		HX_RELEASE(pValues);
	    }
	}	    
    }
    else if (HXR_OK == GetPersistentComponent(pSourceInfo->m_ulPersistentComponentID, pPersistentParentComponent))
    {
	pHXPersistentParentComponent = (HXPersistentComponent*)pPersistentParentComponent;

	// XXX HP 
	// to workaround the screwed-up layout handling within nested meta, we call
	// CleanupLayout() on the alternated persistent source if we haven't called 
	// CleanupLayout() yet. this is a no-worse-than-before temporary solution.
	// REMOVE THIS AFTER PROPERLY HANDLING LAYOUT WITHIN NESTED META
	if (pSourceInfo->m_bAltURL && !pHXPersistentParentComponent->m_bCleanupLayoutCalled)
	{
	    m_pPlayer->CleanupLayout();
	    pHXPersistentParentComponent->m_bCleanupLayoutCalled = TRUE;
	}

	if (!pHXPersistentParentComponent->m_pPersistentChildList)
	{
	    pHXPersistentParentComponent->m_pPersistentChildList = new CHXSimpleList();
	}
    
	pHXPersistentParentComponent->m_pPersistentChildList->AddTail(pHXPersistentComponent);
	pHXPersistentComponent->AddRef();

	pHXPersistentComponent->m_pPersistentParent = pHXPersistentParentComponent;
	pHXPersistentParentComponent->AddRef();
    }

    pSourceInfo->m_bIsPersistentSource = TRUE;

    if (HXR_OK == pHXPersistentComponent->m_pPersistentRenderer->GetPersistentProperties(pProperties))
    {
	pProperties->GetPropertyULONG32("PersistentType", pHXPersistentComponent->m_ulPersistentType);
    }
    HX_RELEASE(pProperties);

    pSourceInfo->m_ulPersistentComponentSelfID = m_ulComponentIndex;

    // new persistent info	
    pHXPersistentComponent->m_pSourceInfo = pSourceInfo;
    pHXPersistentComponent->m_ulComponentID = m_ulComponentIndex;

    // get the persistent properties(track properties)
    if (HXR_OK == m_pPlayer->m_pGroupManager->GetGroup(pSourceInfo->m_uGroupID, pGroup))
    {
	pGroup->GetTrack(pSourceInfo->m_uTrackID, pHXPersistentComponent->m_pProperties);
    }
    HX_RELEASE(pGroup);

    pPersistentRenderer->InitPersistent(m_ulComponentIndex, 
					pSourceInfo->m_uGroupID,
					pSourceInfo->m_uTrackID,
					pHXPersistentParentComponent?pHXPersistentParentComponent->m_pPersistentRenderer:((IHXPersistentRenderer*) NULL));
    m_ulComponentIndex++;

    m_pPlayer->m_pGroupManager->PersistentComponentAdded(pSourceInfo->m_uGroupID, pSourceInfo->m_uTrackID);	

cleanup:

    HX_RELEASE(pPersistentParentComponent);
    HX_RELEASE(pRenderer);

    return rc;
}

/************************************************************************
 *	Method:
 *	    IHXPersistentComponentManager::RemovePersistentComponent
 *	Purpose:
 *	    remove persistent component
 */
STDMETHODIMP
HXPersistentComponentManager::RemovePersistentComponent(UINT32 ulPersistentComponentID)
{
    HX_RESULT			rc = HXR_OK;
    LISTPOSITION		lPosition = NULL;
    HXPersistentComponent*	pHXPersistentParentComponent = NULL;
    HXPersistentComponent*	pHXPersistentComponent = NULL;
    IHXPersistentComponent*	pPersistentComponent = NULL;

    if (HXR_OK == GetPersistentComponent(ulPersistentComponentID, pPersistentComponent))
    {
	pHXPersistentComponent = (HXPersistentComponent*)pPersistentComponent;
	pHXPersistentParentComponent = pHXPersistentComponent->m_pPersistentParent;

	if (pHXPersistentParentComponent)
	{
	    pHXPersistentComponent->Remove();

	    HX_ASSERT(pHXPersistentParentComponent->m_pPersistentChildList &&
		      pHXPersistentParentComponent->m_pPersistentChildList->GetCount());

	    lPosition = pHXPersistentParentComponent->m_pPersistentChildList->Find(pHXPersistentComponent);
	    HX_ASSERT(lPosition);

	    pHXPersistentParentComponent->m_pPersistentChildList->RemoveAt(lPosition);
	    HX_RELEASE(pHXPersistentComponent);
	}
    }
    HX_RELEASE(pPersistentComponent);

    if (m_pRootPersistentComponent  &&
	m_pRootPersistentComponent->m_ulComponentID == ulPersistentComponentID)
    {
	HX_RELEASE(m_pRootPersistentComponent);
    }

    return rc;
}

/************************************************************************
 *	Method:
 *	    IHXPersistentComponentManager::GetPersistentComponent
 *	Purpose:
 *	    get persistent component information
 */
STDMETHODIMP
HXPersistentComponentManager::GetPersistentComponent(UINT32			    ulPersistentComponentID,
						      REF(IHXPersistentComponent*) pPersistentComponent)
{
    HX_RESULT	rc = HXR_FAILED;

    pPersistentComponent = NULL;

    if (m_pRootPersistentComponent)
    {
	rc = m_pRootPersistentComponent->GetPersistentComponent(ulPersistentComponentID, pPersistentComponent);
    }

    return rc;
}

/************************************************************************
 *	Method:
 *	    IHXPersistentComponentManager::AttachPersistentComponentLayout
 *	Purpose:
 *	    get persistent component information
 */
STDMETHODIMP
HXPersistentComponentManager::AttachPersistentComponentLayout(IUnknown*    pLSG,
							       IHXValues*  pProps)
{
    HX_RESULT	    rc = HXR_OK;
    IHXSiteUser*   pSiteUser = NULL;

    if (m_pPlayer)
    {
#if defined(HELIX_FEATURE_VIDEO)
        if (HXR_OK == pLSG->QueryInterface(IID_IHXSiteUser, (void**)&pSiteUser))
	{
	    rc = m_pPlayer->m_pSiteManager->HookupSingleSiteByPlayToFrom(pSiteUser, pProps, FALSE);
	}
	HX_RELEASE(pSiteUser);
#endif //HELIX_FEATURE_VIDEO
    }

    return rc;
}

/************************************************************************
*  Method:
*      IHXGroupSink::GroupAdded
*  Purpose:
*		Notification of a new group being added to the presentation.
*/
STDMETHODIMP
HXPersistentComponentManager::GroupAdded(UINT16	/*IN*/ uGroupIndex,
					  IHXGroup*	/*IN*/ pGroup)
{
    HX_RESULT			rc = HXR_OK;
    UINT32			ulPersistentComponentID = 0;
    IHXValues*			pProperties = NULL;
    IHXGroupSink*		pGroupSink = NULL;
    IHXPersistentComponent*	pPersistentComponent = NULL;

    pProperties = pGroup->GetGroupProperties();
    if (pProperties)
    {
	if (HXR_OK == pProperties->GetPropertyULONG32("PersistentComponentID", ulPersistentComponentID))
	{
	    if (HXR_OK == GetPersistentComponent(ulPersistentComponentID, pPersistentComponent))
	    {
		pGroupSink = ((HXPersistentComponent*)pPersistentComponent)->m_pGroupSink;
		if (pGroupSink)
		{
		    pGroupSink->GroupAdded(uGroupIndex, pGroup);
		}
	    }
	    HX_RELEASE(pPersistentComponent);
	}
    }
    HX_RELEASE(pProperties);

    return rc;
}

/************************************************************************
*  Method:
*      IHXGroupSink::GroupRemoved
*  Purpose:
*		Notification of a group being removed from the presentation.
*/
STDMETHODIMP
HXPersistentComponentManager::GroupRemoved(UINT16	/*IN*/ uGroupIndex,
					    IHXGroup*  /*IN*/ pGroup)
{
    HX_RESULT			rc = HXR_OK;
    UINT32			ulPersistentComponentID = 0;
    IHXValues*			pProperties = NULL;
    IHXGroupSink*		pGroupSink = NULL;
    IHXPersistentComponent*	pPersistentComponent = NULL;

    pProperties = pGroup->GetGroupProperties();
    if (pProperties)
    {
	if (HXR_OK == pProperties->GetPropertyULONG32("PersistentComponentID", ulPersistentComponentID))
	{
	    if (HXR_OK == GetPersistentComponent(ulPersistentComponentID, pPersistentComponent))
	    {
		pGroupSink = ((HXPersistentComponent*)pPersistentComponent)->m_pGroupSink;
		if (pGroupSink)
		{
		    pGroupSink->GroupRemoved(uGroupIndex, pGroup);
		}
	    }
	    HX_RELEASE(pPersistentComponent);
	}
    }
    HX_RELEASE(pProperties);

    return rc;
}

/************************************************************************
*  Method:
*      IHXGroupSink::AllGroupsRemoved
*  Purpose:
*		Notification that all groups have been removed from the 
*		current presentation.
*/
STDMETHODIMP
HXPersistentComponentManager::AllGroupsRemoved(void)
{
    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXGroupSink::TrackAdded
*  Purpose:
*		Notification of a new track being added to a group.
*/
STDMETHODIMP
HXPersistentComponentManager::TrackAdded(UINT16 	    /*IN*/ uGroupIndex,
					  UINT16 	    /*IN*/ uTrackIndex,
					  IHXValues*	    /*IN*/ pTrack)
{
    HX_RESULT			rc = HXR_OK;
    UINT32			ulPersistentComponentID = 0;
    IHXGroupSink*		pGroupSink = NULL;
    IHXPersistentComponent*	pPersistentComponent = NULL;
    UINT32                   bNotUsed = 0;

    if (HXR_OK == pTrack->GetPropertyULONG32("PersistentComponentID", ulPersistentComponentID))
    {
	if (HXR_OK == GetPersistentComponent(ulPersistentComponentID, pPersistentComponent))
	{
	    pGroupSink = ((HXPersistentComponent*)pPersistentComponent)->m_pGroupSink;
	    if (pGroupSink)
	    {
            //If the persistant renderer isn't using groups don't call track added.
            if (HXR_OK != pTrack->GetPropertyULONG32("NoGroupsPresent", bNotUsed))
            {
                pGroupSink->TrackAdded(uGroupIndex, uTrackIndex, pTrack);
            }
            
	    }
	}
	HX_RELEASE(pPersistentComponent);
    }

    return rc;
}

/************************************************************************
*  Method:
*      IHXGroupSink::TrackRemoved
*  Purpose:
*		Notification of a track being removed from a group.
*/
STDMETHODIMP
HXPersistentComponentManager::TrackRemoved(UINT16	    /*IN*/ uGroupIndex,
					    UINT16 	    /*IN*/ uTrackIndex,
					    IHXValues*	    /*IN*/ pTrack)
{
    HX_RESULT			rc = HXR_OK;
    UINT32			ulPersistentComponentID = 0;
    IHXGroupSink*		pGroupSink = NULL;
    IHXPersistentComponent*	pPersistentComponent = NULL;

    if (HXR_OK == pTrack->GetPropertyULONG32("PersistentComponentID", ulPersistentComponentID))
    {
	if (HXR_OK == GetPersistentComponent(ulPersistentComponentID, pPersistentComponent))
	{
	    pGroupSink = ((HXPersistentComponent*)pPersistentComponent)->m_pGroupSink;
	    if (pGroupSink)
	    {
		pGroupSink->TrackRemoved(uGroupIndex, uTrackIndex, pTrack);
	    }
	}
	HX_RELEASE(pPersistentComponent);
    }

    return rc;
}

/************************************************************************
*  Method:
*      IHXGroupSink::TrackStarted
*  Purpose:
*		Notification of a track being started (to get duration, for
*		instance...)
*/
STDMETHODIMP
HXPersistentComponentManager::TrackStarted(UINT16	    /*IN*/ uGroupIndex,
					    UINT16	    /*IN*/ uTrackIndex,
					    IHXValues*	    /*IN*/ pTrack)
{
    HX_RESULT			rc = HXR_OK;
    UINT32			ulPersistentComponentID = 0;
    IHXGroupSink*		pGroupSink = NULL;
    IHXPersistentComponent*	pPersistentComponent = NULL;

    if (HXR_OK == pTrack->GetPropertyULONG32("PersistentComponentID", ulPersistentComponentID))
    {
	if (HXR_OK == GetPersistentComponent(ulPersistentComponentID, pPersistentComponent))
	{
	    pGroupSink = ((HXPersistentComponent*)pPersistentComponent)->m_pGroupSink;
	    if (pGroupSink)
	    {
		pGroupSink->TrackStarted(uGroupIndex, uTrackIndex, pTrack);
	    }
	}
	HX_RELEASE(pPersistentComponent);
    }

    return rc;
}

/************************************************************************
*  Method:
*      IHXGroupSink::TrackStopped
*  Purpose:
*		Notification of a track being stopped
*
*/
STDMETHODIMP
HXPersistentComponentManager::TrackStopped(UINT16	    /*IN*/ uGroupIndex,
					    UINT16	    /*IN*/ uTrackIndex,
					    IHXValues*	    /*IN*/ pTrack)
{
    HX_RESULT			rc = HXR_OK;
    UINT32			ulPersistentComponentID = 0;
    IHXGroupSink*		pGroupSink = NULL;
    IHXPersistentComponent*	pPersistentComponent = NULL;

    if (HXR_OK == pTrack->GetPropertyULONG32("PersistentComponentID", ulPersistentComponentID))
    {
	if (HXR_OK == GetPersistentComponent(ulPersistentComponentID, pPersistentComponent))
	{
	    pGroupSink = ((HXPersistentComponent*)pPersistentComponent)->m_pGroupSink;
	    if (pGroupSink)
	    {
		pGroupSink->TrackStopped(uGroupIndex, uTrackIndex, pTrack);
	    }
	}
	HX_RELEASE(pPersistentComponent);
    }

    return rc;
}

/************************************************************************
*  Method:
*      IHXGroupSink::CurrentGroupSet
*  Purpose:
*		This group is being currently played in the presentation.
*/
STDMETHODIMP
HXPersistentComponentManager::CurrentGroupSet(UINT16	    /*IN*/ uGroupIndex,
					       IHXGroup*   /*IN*/ pGroup)
{
    HX_RESULT			rc = HXR_OK;

    m_nCurrentGroup = uGroupIndex;

    if (m_pRootPersistentComponent)
    {
	rc = m_pRootPersistentComponent->CurrentGroupSet(uGroupIndex, pGroup);
    }

    return rc;
}

HX_RESULT
HXPersistentComponentManager::OnTimeSync(ULONG32 ulCurrentTime)
{
    HX_RESULT	rc = HXR_OK;

    if (m_pRootPersistentComponent)
    {
	rc = m_pRootPersistentComponent->OnTimeSync(ulCurrentTime);
    }

    return rc;
}

HXBOOL
HXPersistentComponentManager::IsCleanupLayoutNeeded(INT32 nCurrentGroup, INT32 nGroupSwitchTo)
{
    HXBOOL			bResult = FALSE;
    UINT32			ulPersistentComponentIDSwitchTo = 0;
    IHXValues*			pGroupProperties = NULL;
    IHXValues*			pGroupSwitchToProperties = NULL;
    IHXGroup*			pGroup = NULL;
    IHXGroup*			pGroupSwitchTo = NULL;
    IHXGroup2*			pGroup2 = NULL;
    IHXGroupManager*		pGroupManager = NULL;
    IHXPersistentComponent*	pPersistentComponent = NULL;

    // we will cleanup the layout if the from_group and to_group:
    // * associated with different persistent component OR
    // * associated with the same RAM
    if (HXR_OK == m_pPlayer->QueryInterface(IID_IHXGroupManager, (void**)&pGroupManager))
    {
	if (HXR_OK == pGroupManager->GetGroup(nGroupSwitchTo, pGroupSwitchTo))
	{
	    pGroupSwitchToProperties = pGroupSwitchTo->GetGroupProperties();
	    if (pGroupSwitchToProperties)
	    {
		if (HXR_OK == pGroupSwitchToProperties->GetPropertyULONG32("PersistentComponentID", 
									   ulPersistentComponentIDSwitchTo))
		{
		    if (HXR_OK == pGroupManager->GetGroup(nCurrentGroup, pGroup) &&
			HXR_OK == pGroup->QueryInterface(IID_IHXGroup2, (void**)&pGroup2))
		    {	
			if (HXR_OK == pGroup2->GetPersistentComponentProperties(ulPersistentComponentIDSwitchTo,
										pGroupProperties))
			{
			    if (HXR_OK == GetPersistentComponent(ulPersistentComponentIDSwitchTo, pPersistentComponent))
			    {
				// switch group within RAM
				if (((HXPersistentComponent*)pPersistentComponent)->m_ulPersistentType == PersistentRAM)
				{
				    bResult = TRUE;
				}
				// switch group within SMIL without root layout
				// m_pPlayer->m_bAddLayoutSiteGroupCalled = TRUE when there is root layout
				// specified in SMIL
				// this needs to be revisited when fixing the nested meta layout
				else if (!m_pPlayer->m_bAddLayoutSiteGroupCalled)
				{
				    bResult = TRUE;
				}
			    }
			    else
			    {
				HX_ASSERT(FALSE);
			    }
			    HX_RELEASE(pPersistentComponent);
			}
			else
			{
			    bResult = TRUE;
			}
			HX_RELEASE(pGroupProperties);
		    }
		    HX_RELEASE(pGroup2);
		    HX_RELEASE(pGroup);
		}
	    }
	    HX_RELEASE(pGroupSwitchToProperties);
	}
	HX_RELEASE(pGroupSwitchTo);
    }
    HX_RELEASE(pGroupManager);

    return bResult;
}

UINT32
HXPersistentComponentManager::GetPersistentComponentCount(void)
{
    if (m_pRootPersistentComponent)
    {
	return m_pRootPersistentComponent->GetPersistentComponentCount();
    }

    return 0;
}

void		
HXPersistentComponentManager::TrackUpdated(UINT16 uGroupIndex, 
					    UINT16 uTrackIndex, 
					    IHXValues* pValues)
{
    if (m_pRootPersistentComponent)
    {
	m_pRootPersistentComponent->TrackUpdated(uGroupIndex, 
	   				         uTrackIndex,
						 pValues);
    }

    return;
}

void
HXPersistentComponentManager::CloseAllRenderers(INT32 nGroupSwitchTo)
{
    // nested meta support
    if (m_pRootPersistentComponent)
    {
	if (m_pRootPersistentComponent->m_bToBeClosed)
	{
            // /Call CleanupLayout before calling Remove fixes PR 116482:
	    m_pPlayer->CleanupLayout();
	    m_pRootPersistentComponent->Remove();
	    HX_RELEASE(m_pRootPersistentComponent);
        }
    	else
	{
	    m_pRootPersistentComponent->AllRenderersClosed();

	    if (IsCleanupLayoutNeeded(m_nCurrentGroup, nGroupSwitchTo))
	    {
		m_pPlayer->CleanupLayout();
	    }
	}
    }
    else
    {	
	m_pPlayer->CleanupLayout();
    }

    return;
}

void
HXPersistentComponentManager::Reset()
{
    if (m_pRootPersistentComponent)
    {
	m_pRootPersistentComponent->Reset();
    }
    m_ulComponentIndex = 0;

    return;
}

void 
HXPersistentComponentManager::Close()
{
    HX_ASSERT(!m_pRootPersistentComponent);

    return;
}
