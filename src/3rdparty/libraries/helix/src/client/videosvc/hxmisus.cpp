/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmisus.cpp,v 1.7 2005/03/14 20:32:08 bobclark Exp $
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxslist.h"
#include "hxwin.h"
#include "hxengin.h"
#include "hxsite2.h"
#include "ihxpckts.h"
#include "hxvsurf.h"
#include "hxmisus.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

/************************************************************************
 *  Method:
 *    Constructor
 */
CHXMultiInstanceSiteUserSupplier::CHXMultiInstanceSiteUserSupplier()
    : m_lRefCount(0)
    , m_pSingleUser(NULL)
    , m_bIsAttached(FALSE)
    , m_pSurfaceBitmapInfo(NULL)
    , m_bSetSizeHasBeenCalled(FALSE)
    , m_bSetPositionHasBeenCalled(FALSE)
    , m_bSetZOrderHasBeenCalled(FALSE)
    , m_zorder(0)
    , m_bIsInterrupSafe(FALSE)
{
    m_size.cx       = 0;
    m_size.cy       = 0;
    m_position.x    = 0;
    m_position.y    = 0;
    m_pSurfaceBitmapInfo = 0;
}

/************************************************************************
 *  Method:
 *    Destructor
 */
CHXMultiInstanceSiteUserSupplier::~CHXMultiInstanceSiteUserSupplier()
{
    CHXSimpleList::Iterator ndx = m_PassiveSiteWatchers.Begin();
    for (; ndx != m_PassiveSiteWatchers.End(); ++ndx)
    {
	IHXPassiveSiteWatcher* pWatcher =
	    (IHXPassiveSiteWatcher*)(*ndx);
	delete pWatcher;
    }
    m_PassiveSiteWatchers.RemoveAll();
    HX_DELETE (m_pSurfaceBitmapInfo);
}

/************************************************************************
 *  Method:
 *    IUnknown::QueryInterface
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXVideoSurface2))
    {
        // All of our target site surfaces must support IHXVideoSurface2
        // for this QI to succeed; this is required both for the proxy logic
        // in this class as well as to signal to the renderer that it's using
        // and old or 3rd party site. 
        
        CHXSimpleList::Iterator i = m_SiteUsers.Begin();
        if (i != m_SiteUsers.End())
        {
            CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
            if (pSiteUser->m_pSite2)
            {
                IHXVideoSurface* pFirstSurface;
                if (HXR_OK == pSiteUser->m_pSite2->GetVideoSurface(pFirstSurface))
                {
                    IHXVideoSurface* pSecondSurface;
                    if (HXR_OK == pFirstSurface->QueryInterface(riid, (void**) &pSecondSurface))
                    {
                        HX_RELEASE(pFirstSurface);
                        HX_RELEASE(pSecondSurface);
                        AddRef();
                        *ppvObj = (IUnknown*)(IHXVideoSurface2*)this;
                        return HXR_OK;
                    }
                    
                    HX_RELEASE(pFirstSurface);
                }
            }
        }
    }
	
    QInterfaceList qiList[] =
    {
	    { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXSite*)this },
	    { GET_IIDHANDLE(IID_IHXVideoSurface), (IHXVideoSurface*) this },
	    { GET_IIDHANDLE(IID_IHXSite), (IHXSite*) this },
	    { GET_IIDHANDLE(IID_IHXSite2), (IHXSite2*) this },
	    { GET_IIDHANDLE(IID_IHXSiteEnumerator), (IHXSiteEnumerator*) this },
	    { GET_IIDHANDLE(IID_IHXSiteUserSupplier), (IHXSiteUserSupplier*) this },
	    { GET_IIDHANDLE(IID_IHXMultiInstanceSiteUserSupplier), (IHXMultiInstanceSiteUserSupplier*) this },
	    { GET_IIDHANDLE(IID_IHXInterruptSafe), (IHXInterruptSafe*) this },
    };	

    if (QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj) == HXR_OK)
    {
	    return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXValues)) //XXXCXZ
    {
	if (m_pSingleUser)
	{
	    return m_pSingleUser->QueryInterface(IID_IHXValues, ppvObj);
	}
    }
    else if (IsEqualIID(riid, IID_IHXSiteWindowless))
    {
	// check if the underlying implementation is the new one!
	CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	if(i != m_SiteUsers.End())
	{
	    CHXMultiInstanceSiteUser* pSiteUser = 
			(CHXMultiInstanceSiteUser*)(*i);
	    if (pSiteUser->m_pSite)
	    {
		return pSiteUser->m_pSite->QueryInterface(riid, ppvObj);
	    }
	}
    }
    else if (IsEqualIID(riid, IID_IHXSubRectSite))
    {
	//Check to see if this site support this new interface....
	CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	if(i != m_SiteUsers.End())
	{
	    CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
	    if(pSiteUser->m_pSite)
	    {
		return pSiteUser->m_pSite->QueryInterface(riid, ppvObj);
	    }
	}
    }
    else if (IsEqualIID(riid, IID_IHXKeyBoardFocus))
    {
	CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	if(i != m_SiteUsers.End())
	{
	    CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
	    if(pSiteUser->m_pSite)
	    {
                // Return fhe first we find:
		if (SUCCEEDED(pSiteUser->m_pSite->QueryInterface(riid, ppvObj)))
                {
                    return HXR_OK;
                }
	    }
	}
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/************************************************************************
 *  Method:
 *    IUnknown::AddRef
 */
STDMETHODIMP_(ULONG32) 
CHXMultiInstanceSiteUserSupplier::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/************************************************************************
 *  Method:
 *    IUnknown::Release
 */
STDMETHODIMP_(ULONG32) 
CHXMultiInstanceSiteUserSupplier::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *  Method:
 *    IHXSite::AttachUser
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::AttachUser(IHXSiteUser* /*IN*/ pUser)
{
    return HXR_NOTIMPL;
}

/************************************************************************
 *  Method:
 *    IHXSite::DetachUser
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::DetachUser()
{
    return HXR_NOTIMPL;
}

/************************************************************************
 *  Method:
 *    IHXSite::GetUser
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::GetUser(REF(IHXSiteUser*) /*OUT*/ pUser)
{
    return HXR_NOTIMPL;
}


/************************************************************************
 *  Method:
 *    IHXSite::CreateChild
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::CreateChild(REF(IHXSite*) /*OUT*/ pChildSite)
{
    return HXR_NOTIMPL;
}

/************************************************************************
 *  Method:
 *    IHXSite::DestroyChild
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::DestroyChild(IHXSite* /*IN*/ pChildSite)
{
    return HXR_NOTIMPL;
}

/************************************************************************
 *  Method:
 *    IHXSite::SetSize
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::SetSize(HXxSize size)
{
    m_bSetSizeHasBeenCalled = TRUE;
    m_size = size;
    CHXSimpleList::Iterator i = m_SiteUsers.Begin();
    for ( ; i != m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = 
		    (CHXMultiInstanceSiteUser*)(*i);
	pSiteUser->m_pSite->SetSize(size);
    }

    CHXSimpleList::Iterator ndx = m_PassiveSiteWatchers.Begin();
    for (; ndx != m_PassiveSiteWatchers.End(); ++ndx)
    {
	IHXPassiveSiteWatcher* pWatcher =
	    (IHXPassiveSiteWatcher*)(*ndx);
	pWatcher->SizeChanged(&size);
    }
    
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSite::SetPosition
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::SetPosition(HXxPoint position)
{
    m_bSetPositionHasBeenCalled = TRUE;
    m_position = position;
    CHXSimpleList::Iterator i = m_SiteUsers.Begin();
    for ( ; i != m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = 
		    (CHXMultiInstanceSiteUser*)(*i);
	pSiteUser->m_pSite->SetPosition(position);
    }
    
    CHXSimpleList::Iterator ndx = m_PassiveSiteWatchers.Begin();
    for (; ndx != m_PassiveSiteWatchers.End(); ++ndx)
    {
	IHXPassiveSiteWatcher* pWatcher =
	    (IHXPassiveSiteWatcher*)(*ndx);
	pWatcher->PositionChanged(&position);
    }
    
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSite::GetSize
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::GetSize(REF(HXxSize) size)
{
    // We assume the first instance has the correct size and position.
    CHXSimpleList::Iterator i = m_SiteUsers.Begin();
    if(i != m_SiteUsers.End())
    {
	CHXMultiInstanceSiteUser* pSiteUser = 
		    (CHXMultiInstanceSiteUser*)(*i);
	return pSiteUser->m_pSite->GetSize(size);
    }
    // If there is no actual instance, then use the cached size.
    size = m_size;
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSite::GetPosition
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::GetPosition(REF(HXxPoint) position)
{
    // We assume the first instance has the correct size and position.
    CHXSimpleList::Iterator i = m_SiteUsers.Begin();
    if(i != m_SiteUsers.End())
    {
	CHXMultiInstanceSiteUser* pSiteUser = 
		    (CHXMultiInstanceSiteUser*)(*i);
	return pSiteUser->m_pSite->GetPosition(position);
    }
    // If there is no actual instance, then use the cached position
    position = m_position;
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSite::DamageRect
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::DamageRect(HXxRect rect)
{
    /*
     * Coding this loop like this instead of using an iterator
     * makes things slightly more thread safe. This fixes the known
     * repro cases for bug 3787.
     */
    LISTPOSITION pos = m_SiteUsers.GetHeadPosition();
    while(pos)
    {
	CHXMultiInstanceSiteUser* pSiteUser = 
	    (CHXMultiInstanceSiteUser*)m_SiteUsers.GetNext(pos);

	if (pSiteUser)
	    pSiteUser->m_pSite->DamageRect(rect);
    }
   
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSite::DamageRegion
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::DamageRegion(HXxRegion region)
{
    /*
     * Coding this loop like this instead of using an iterator
     * makes things slightly more thread safe. This fixes the known
     * repro cases for bug 3787.
     */
    LISTPOSITION pos = m_SiteUsers.GetHeadPosition();
    while(pos)
    {
	CHXMultiInstanceSiteUser* pSiteUser = 
	    (CHXMultiInstanceSiteUser*)m_SiteUsers.GetNext(pos);

	if (pSiteUser)
	    pSiteUser->m_pSite->DamageRegion(region);
    }
   
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSite::ForceRedraw
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::ForceRedraw()
{
    /*
     * Coding this loop like this instead of using an iterator
     * makes things slightly more thread safe. This fixes the known
     * repro cases for bug 3787.
     */
    LISTPOSITION pos = m_SiteUsers.GetHeadPosition();
    while(pos)
    {
	CHXMultiInstanceSiteUser* pSiteUser = 
	    (CHXMultiInstanceSiteUser*)m_SiteUsers.GetNext(pos);

	if (pSiteUser)
	    pSiteUser->m_pSite->ForceRedraw();
    }
   
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXMultiInstanceSiteUserSupplier::SetSingleSiteUser
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::SetSingleSiteUser(IUnknown* pUnknown)
{
    if (HXR_OK != pUnknown->QueryInterface(IID_IHXSiteUser,(void**)&m_pSingleUser))
    {
	return HXR_INVALID_PARAMETER;
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXMultiInstanceSiteUserSupplier::ReleaseSingleSiteUser
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::ReleaseSingleSiteUser()
{
    HX_RELEASE(m_pSingleUser);
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSiteUserSupplier::CreateSiteUser
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::CreateSiteUser
(
    REF(IHXSiteUser*)/*OUT*/ pSiteUser
)
{
    CHXMultiInstanceSiteUser* pMISU = new CHXMultiInstanceSiteUser(this);
    if (!pMISU)
    {
	return HXR_OUTOFMEMORY;
    }

    // This is our caller's ref.
    pMISU->QueryInterface(IID_IHXSiteUser,(void**)&pSiteUser);

    m_SiteUsers.AddTail(pSiteUser);

    pSiteUser->AddRef(); // this is for the caller
    
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSiteUserSupplier::DestroySiteUser
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::DestroySiteUser
(
    IHXSiteUser* /*IN*/ pSiteUser
)
{
    LISTPOSITION pos = m_SiteUsers.Find(pSiteUser);
    if (!pos)
    {
        return HXR_INVALID_PARAMETER;
    }
    m_SiteUsers.RemoveAt(pos);
    pSiteUser->Release();

    // If this is the last of the multi-instance sites, then
    // tell the real site user to detach.
    if (m_SiteUsers.IsEmpty())
    {
	AddRef();
	IHXSiteUser* pSingleUser = m_pSingleUser;
	pSingleUser->AddRef();
        m_pSingleUser->DetachSite();
	pSingleUser->Release();
	m_bIsAttached = FALSE;
	Release();
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSiteUserSupplier::NeedsWindowedSites
 */
STDMETHODIMP_(HXBOOL)
CHXMultiInstanceSiteUserSupplier::NeedsWindowedSites()
{
    return FALSE;
}

/************************************************************************
 *  Method:
 *    IHXSite::AttachWatcher
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::AttachWatcher(IHXSiteWatcher* /*IN*/ pWatcher)
{
    return HXR_NOTIMPL;
}

/************************************************************************
 *  Method:
 *    IHXSite::DetachWatcher
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::DetachWatcher()
{
    return HXR_NOTIMPL;
}

/************************************************************************
 *  Method:
 *    IHXSite2::UpdateSiteWindow
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::UpdateSiteWindow(HXxWindow* /*IN*/ pWindow)
{
    CHXSimpleList::Iterator i = m_SiteUsers.Begin();
    for ( ; i != m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = 
		    (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
	    pSiteUser->m_pSite2->UpdateSiteWindow(pWindow);
	}
    }
    
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSite2::ShowSite
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::ShowSite(HXBOOL bShow)
{
    return HXR_NOTIMPL;
}

/************************************************************************
 *  Method:
 *    IHXSite2::IsSiteVisible
 */
STDMETHODIMP_(HXBOOL)
CHXMultiInstanceSiteUserSupplier::IsSiteVisible()
{
    // Not implemented
    HX_ASSERT(0);
    return FALSE;
}

/************************************************************************
 *  Method:
 *    IHXSite2::SetZOrder
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::SetZOrder(INT32 lZOrder)
{
    m_bSetZOrderHasBeenCalled = TRUE;
    m_zorder = lZOrder;
    
    CHXSimpleList::Iterator i = m_SiteUsers.Begin();
    for ( ; i != m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = 
		    (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
	    pSiteUser->m_pSite2->SetZOrder(lZOrder);
	}
    }
    
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSite2::GetZOrder
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::GetZOrder(REF(INT32) lZOrder)
{
    return HXR_NOTIMPL;
}

/************************************************************************
 *  Method:
 *    IHXSite2::MoveSiteToTop
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::MoveSiteToTop()
{
    CHXSimpleList::Iterator i = m_SiteUsers.Begin();
    for ( ; i != m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = 
		    (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
	    pSiteUser->m_pSite2->MoveSiteToTop();
	}
    }
    
    return HXR_OK;
}

#if 0 
/************************************************************************
 *  Method:
 *    IHXSite2::GetVideoSurface
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::GetVideoSurface(REF(IHXVideoSurface*) pSurface)
{
    CHXSimpleList::Iterator i = m_SiteUsers.Begin();
    CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
    if (pSiteUser->m_pSite2)
    {
	return pSiteUser->m_pSite2->GetVideoSurface(pSurface);
    }
    
    return HXR_FAIL;
}
#endif

/************************************************************************
 *  Method:
 *    IHXSite2::GetVideoSurface
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::GetVideoSurface(REF(IHXVideoSurface*) pSurface)
{
    // check to see if we have a video surface in any of our 
    // sites 
    IHXVideoSurface* pTempSurface;

    for (CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	 i!= m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
	    if (HXR_OK == pSiteUser->m_pSite2->GetVideoSurface(pTempSurface))
	    {
		// so we have a video sufrface in one of our sites 
		// good enough return HXR_OK.
		HX_RELEASE(pTempSurface);
		QueryInterface(IID_IHXVideoSurface, (void**) &pSurface);
		return HXR_OK;
	    }
	}
     }
	    
    return HXR_FAIL;
}

/************************************************************************
 *  Method:
 *    IHXSite2::GetNumberOfChildSites
 */
STDMETHODIMP_(UINT32)
CHXMultiInstanceSiteUserSupplier::GetNumberOfChildSites()
{
    UINT32 ulChildSites = 0;

    CHXSimpleList::Iterator i = m_SiteUsers.Begin();
    for ( ; i != m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = 
		    (CHXMultiInstanceSiteUser*)(*i);
	ulChildSites += pSiteUser->m_pSite2->GetNumberOfChildSites();
    }

    return ulChildSites;
}

/************************************************************************
 *  Method:
 *    IHXSite2::AddPassiveSiteWatcher
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::AddPassiveSiteWatcher
(
    IHXPassiveSiteWatcher* pWatcher
)
{
    HX_ASSERT(pWatcher);

    pWatcher->AddRef();
    m_PassiveSiteWatchers.AddTail(pWatcher);
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSite2::DetachPassiveSiteWatcher
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::RemovePassiveSiteWatcher
(
    IHXPassiveSiteWatcher* pWatcher
)
{
    LISTPOSITION pos = m_PassiveSiteWatchers.GetHeadPosition();
    while(pos)
    {
	IHXPassiveSiteWatcher* pThisWatcher = 
	    (IHXPassiveSiteWatcher*)m_PassiveSiteWatchers.GetAt(pos);
	if(pWatcher == pThisWatcher)
	{
	    pWatcher->Release();
	    m_PassiveSiteWatchers.RemoveAt(pos);
	    break;
	}
	m_PassiveSiteWatchers.GetNext(pos);
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSite2::SetCursor
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::SetCursor
(
    HXxCursor cursor,
    REF(HXxCursor) oldCursor
)
{
    return HXR_NOTIMPL;
}

/*
 * IHXSiteEnumerator methods 
 */

STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::GetFirstSite  
(
    REF(IHXSite*) /* OUT */ pFirstSite,
    REF(IHXSiteEnumerator::SitePosition) /* OUT */ nextPosition
)
{
    nextPosition = m_SiteUsers.GetHeadPosition();  
    if (nextPosition)
    {
        pFirstSite = ((CHXMultiInstanceSiteUser*) m_SiteUsers.GetNext(nextPosition))->m_pSite; 

        if (pFirstSite)
        {        
            HX_ADDREF(pFirstSite);
            return HXR_OK;
        }
    }

    return HXR_FAIL;
}

STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::GetNextSite   
(
    REF(IHXSite*) pNextSite,
    REF(IHXSiteEnumerator::SitePosition) /* IN/OUT */ nextPosition
)
{
    if (nextPosition)
    {
        pNextSite = ((CHXMultiInstanceSiteUser*) m_SiteUsers.GetNext(nextPosition))->m_pSite; 

        if (pNextSite)
        {
            HX_ADDREF(pNextSite);
            return HXR_OK;
        }
    }

    return HXR_FAIL;
}


/************************************************************************
 *  Method:
 *    IHXVideoSurface::Blt
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::Blt
(UCHAR*			/*IN*/	pImageBits, 
HXBitmapInfoHeader*    /*IN*/	pBitmapInfo,			
REF(HXxRect)		/*IN*/	rDestRect, 
REF(HXxRect)		/*IN*/	rSrcRect) 
{           
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}

/************************************************************************
 *  Method:
 *    IHXVideoSurface::BeginOptimizedBlt
 */
STDMETHODIMP
CHXMultiInstanceSiteUserSupplier::BeginOptimizedBlt
(
HXBitmapInfoHeader*    /*IN*/	pBitmapInfo
) 
{
    if (!pBitmapInfo)
	return HXR_FAIL;

    HX_DELETE(m_pSurfaceBitmapInfo);
    m_pSurfaceBitmapInfo = new HXBitmapInfoHeader;
    memcpy(m_pSurfaceBitmapInfo, pBitmapInfo, sizeof(HXBitmapInfoHeader)); /* Flawfinder: ignore */
    
    HX_RESULT retVal = HXR_FAIL;
    IHXVideoSurface* pTempSurface;

    for (CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	 i!= m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
	    if (HXR_OK == pSiteUser->m_pSite2->GetVideoSurface(pTempSurface))
	    {
		// so we have a video sufrface in one of our sites 
		// good enough return HXR_OK.
		if (HXR_OK == pTempSurface->BeginOptimizedBlt(pBitmapInfo))
		{
		    retVal = HXR_OK;
		}
		HX_RELEASE(pTempSurface);
	    }
	}
     }
    return retVal;
}

/************************************************************************
 *  Method:
 *    IHXVideoSurface::OptimizedBlt
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::OptimizedBlt	
(
UCHAR*			/*IN*/	pImageBits,			
REF(HXxRect)		/*IN*/	rDestRect, 
REF(HXxRect)		/*IN*/	rSrcRect
)
{
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}

/************************************************************************
 *  Method:
 *    IHXVideoSurface::EndOptimizedBlt
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::EndOptimizedBlt ()
{
    HX_RESULT retVal = HXR_FAIL;
    IHXVideoSurface* pTempSurface;

    for (CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	 i!= m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
	    if (HXR_OK == pSiteUser->m_pSite2->GetVideoSurface(pTempSurface))
	    {
		// so we have a video sufrface in one of our sites 
		// good enough return HXR_OK.
		if (HXR_OK == pTempSurface->EndOptimizedBlt())
		{
		    HX_RELEASE(pTempSurface);
		    retVal = HXR_OK;
		    break;
		}
		HX_RELEASE(pTempSurface);
	    }
	}
     }
    return retVal;
}

/************************************************************************
 *  Method:
 *    IHXVideoSurface::GetOptimizedFormat
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::GetOptimizedFormat
(
    REF(HX_COMPRESSION_TYPE) /*OUT*/ ulType
)
{
    if (m_SiteUsers.IsEmpty())
	return HXR_FAIL;

    HX_RESULT retVal = HXR_FAIL;
    IHXVideoSurface* pTempSurface;

    for (CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	 i!= m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
	    if (HXR_OK == pSiteUser->m_pSite2->GetVideoSurface(pTempSurface))
	    {
		// so we have a video sufrface in one of our sites 
		// good enough return HXR_OK.
		if (HXR_OK == pTempSurface->GetOptimizedFormat(ulType))
		{
		    retVal = HXR_OK;
		}
		HX_RELEASE(pTempSurface);
	    }
	}
     }
    return retVal;
}


/************************************************************************
 *  Method:
 *    IHXVideoSurface::GetPreferredFormat
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::GetPreferredFormat
(
REF(HX_COMPRESSION_TYPE) /*OUT*/ ulType
)
{
    if (m_SiteUsers.IsEmpty())
	return HXR_FAIL;

    HX_RESULT retVal = HXR_FAIL;
    IHXVideoSurface* pTempSurface;

    for (CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	 i!= m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
	    if (HXR_OK == pSiteUser->m_pSite2->GetVideoSurface(pTempSurface))
	    {
		// so we have a video sufrface in one of our sites 
		// good enough return HXR_OK.
		if (HXR_OK == pTempSurface->GetPreferredFormat(ulType))
		{
		    HX_RELEASE(pTempSurface);
		    retVal = HXR_OK;
		    break;
		}
		HX_RELEASE(pTempSurface);
	    }
	}
     }
    return retVal;
}


/*
 * IHXVideoSurface2 methods 
 */
STDMETHODIMP 
CHXMultiInstanceSiteUserSupplier::SetProperties
(
    HXBitmapInfoHeader *bmi, REF(UINT32) ulNumBuffers, IHXRenderTimeLine *pClock
)
{
    HX_RESULT result = HXR_OK;

    for (CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	 i!= m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
            IHXVideoSurface* pVideoSurface;
	    if (SUCCEEDED(pSiteUser->m_pSite2->GetVideoSurface(pVideoSurface)))
	    {
                IHXVideoSurface2* pVideoSurface2;
                if (SUCCEEDED(pVideoSurface->QueryInterface(IID_IHXVideoSurface2,
                                                            (void**) &pVideoSurface2)))
                {
                    // Propagate all failures:
                    HX_RESULT localResult = pVideoSurface2->SetProperties(bmi,
                                                                          ulNumBuffers,
                                                                          pClock);
                    if (SUCCEEDED(result))
                    {
                        result = localResult;    
                    }
                    
                    HX_RELEASE(pVideoSurface2);
                }
                HX_RELEASE(pVideoSurface);
            }
        }
    }

    return result;
}


STDMETHODIMP_(void) 
CHXMultiInstanceSiteUserSupplier::Flush()    
{
    for (CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	 i!= m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
            IHXVideoSurface* pVideoSurface;
	    if (SUCCEEDED(pSiteUser->m_pSite2->GetVideoSurface(pVideoSurface)))
	    {
                IHXVideoSurface2* pVideoSurface2;
                if (SUCCEEDED(pVideoSurface->QueryInterface(IID_IHXVideoSurface2,
                                                            (void**) &pVideoSurface2)))
                {
                    pVideoSurface2->Flush();
                    HX_RELEASE(pVideoSurface2);
                }
                HX_RELEASE(pVideoSurface);
            }
        }
    }
}

STDMETHODIMP   
CHXMultiInstanceSiteUserSupplier::ReleaseVideoMem(VideoMemStruct* pVidMem)
{
    HX_RESULT result = HXR_FAIL;

    for (CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	 i!= m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
            IHXVideoSurface* pVideoSurface;
	    if (SUCCEEDED(pSiteUser->m_pSite2->GetVideoSurface(pVideoSurface)))
	    {
                IHXVideoSurface2* pVideoSurface2;
                if (SUCCEEDED(pVideoSurface->QueryInterface(IID_IHXVideoSurface2,
                                                            (void**) &pVideoSurface2)))
                {
                    // We just need one success:
                    result = pVideoSurface2->ReleaseVideoMem(pVidMem);

                    HX_RELEASE(pVideoSurface2);
                    if (SUCCEEDED(result))
                    {
                        break;   
                    }
                }
                HX_RELEASE(pVideoSurface);
            }
        }
    }

    return result;
}

                                 
STDMETHODIMP   
CHXMultiInstanceSiteUserSupplier::ColorConvert
(
    INT32 cidIn, 
    HXxSize *pSrcSize,
    HXxRect *prSrcRect,
    SourceInputStruct *pInput,
    INT32 cidOut,
    UCHAR *pDestBuffer, 
    HXxSize *pDestSize, 
    HXxRect *prDestRect, 
    int nDestPitch
)
{
    HX_RESULT result = HXR_FAIL;

    for (CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	 i!= m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
            IHXVideoSurface* pVideoSurface;
	    if (SUCCEEDED(pSiteUser->m_pSite2->GetVideoSurface(pVideoSurface)))
	    {
                IHXVideoSurface2* pVideoSurface2;
                if (SUCCEEDED(pVideoSurface->QueryInterface(IID_IHXVideoSurface2,
                                                            (void**) &pVideoSurface2)))
                {
                    // We just need one success:
                    result = pVideoSurface2->ColorConvert( cidIn,
                                                           pSrcSize,
                                                           prSrcRect,
                                                           pInput,
                                                           cidOut,
                                                           pDestBuffer,
                                                           pDestSize,
                                                           prDestRect,
                                                           nDestPitch
                                                           );
                    
                    HX_RELEASE(pVideoSurface2);
                    if (SUCCEEDED(result))
                    {
                        break;   
                    }
                }
                HX_RELEASE(pVideoSurface);
            }
        }
    }

    return result;
}

                                 
STDMETHODIMP   
CHXMultiInstanceSiteUserSupplier::GetVideoMem
(
    VideoMemStruct* pVidMem,
    UINT32 ulFlags
)
{
    HX_RESULT result = HXR_FAIL;

    for (CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	 i!= m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
            IHXVideoSurface* pVideoSurface;
	    if (SUCCEEDED(pSiteUser->m_pSite2->GetVideoSurface(pVideoSurface)))
	    {
                IHXVideoSurface2* pVideoSurface2;
                if (SUCCEEDED(pVideoSurface->QueryInterface(IID_IHXVideoSurface2,
                                                            (void**) &pVideoSurface2)))
                {
                    // We just need one success:
                    result = pVideoSurface2->GetVideoMem(pVidMem, ulFlags);

                    HX_RELEASE(pVideoSurface2);
                    if (SUCCEEDED(result))
                    {
                        break;   
                    }
                }
                HX_RELEASE(pVideoSurface);
            }
        }
    }

    return result;
}
                                 
STDMETHODIMP   
CHXMultiInstanceSiteUserSupplier::Present
(
    VideoMemStruct* pVidMem,
    INT32 lTime,
    UINT32 ulFlags,
    HXxRect* prDestRect,
    HXxRect* prSrcRect
)
{
    HX_RESULT result = HXR_OK;

    for (CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	 i!= m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
            IHXVideoSurface* pVideoSurface;
	    if (SUCCEEDED(pSiteUser->m_pSite2->GetVideoSurface(pVideoSurface)))
	    {
                IHXVideoSurface2* pVideoSurface2;
                if (SUCCEEDED(pVideoSurface->QueryInterface(IID_IHXVideoSurface2,
                                                            (void**) &pVideoSurface2)))
                {
                    // Propagate all failures:
                    HX_RESULT localResult = pVideoSurface2->Present(pVidMem,
                                                                    lTime,
                                                                    ulFlags,
                                                                    prDestRect,
                                                                    prSrcRect);
                    if (SUCCEEDED(result))
                    {
                        result = localResult;    
                    }
                    
                    HX_RELEASE(pVideoSurface2);
                }
                HX_RELEASE(pVideoSurface);
            }
        }
    }

    return result;
}
                                 
STDMETHODIMP   
CHXMultiInstanceSiteUserSupplier::PresentIfReady()
{
    HX_RESULT result = HXR_OK;

    for (CHXSimpleList::Iterator i = m_SiteUsers.Begin();
	 i!= m_SiteUsers.End(); ++i)
    {
	CHXMultiInstanceSiteUser* pSiteUser = (CHXMultiInstanceSiteUser*)(*i);
	if (pSiteUser->m_pSite2)
	{
            IHXVideoSurface* pVideoSurface;
	    if (SUCCEEDED(pSiteUser->m_pSite2->GetVideoSurface(pVideoSurface)))
	    {
                IHXVideoSurface2* pVideoSurface2;
                if (SUCCEEDED(pVideoSurface->QueryInterface(IID_IHXVideoSurface2,
                                                            (void**) &pVideoSurface2)))
                {
                    // Propagate all failures:
                    HX_RESULT localResult = pVideoSurface2->PresentIfReady();
                    if (SUCCEEDED(result))
                    {
                        result = localResult;    
                    }
                    HX_RELEASE(pVideoSurface2);
                }
                HX_RELEASE(pVideoSurface);
            }
        }
    }     

    return result;
}

/*
 *  IHXInterruptSafe methods
 */

/************************************************************************
 *	Method:
 *	    IHXInterruptSafe::IsInterruptSafe
 *	Purpose:
 *	    This is the function that will be called to determine if
 *	    interrupt time execution is supported.
 */
STDMETHODIMP_(HXBOOL)
CHXMultiInstanceSiteUserSupplier::IsInterruptSafe(void)
{
    return m_bIsInterrupSafe;
}

/************************************************************************
 *  Method:
 *    Constructor
 */
CHXMultiInstanceSiteUser::CHXMultiInstanceSiteUser
(
    CHXMultiInstanceSiteUserSupplier* pMISUS
)
    : m_lRefCount(0)
    , m_pMISUS(pMISUS)
    , m_pSite(NULL)
    , m_pSite2(NULL)
{
};

/************************************************************************
 *  Method:
 *    Destructor
 */
CHXMultiInstanceSiteUser::~CHXMultiInstanceSiteUser()
{
}

/************************************************************************
 *  Method:
 *    IUnknown::QueryInterface
 */
STDMETHODIMP 
CHXMultiInstanceSiteUser::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXSiteUser))
    {
	AddRef();
	*ppvObj = (IHXSiteUser*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXSiteUser*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/************************************************************************
 *  Method:
 *    IUnknown::AddRef
 */
STDMETHODIMP_(ULONG32) 
CHXMultiInstanceSiteUser::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/************************************************************************
 *  Method:
 *    IUnknown::Release
 */
STDMETHODIMP_(ULONG32) 
CHXMultiInstanceSiteUser::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *  Method:
 *    IHXSiteUser::AttachSite
 */
STDMETHODIMP 
CHXMultiInstanceSiteUser::AttachSite(IHXSite* /*IN*/ pSite)
{
    if (m_pSite) return HXR_UNEXPECTED;
    if (!pSite)  return HXR_INVALID_PARAMETER;
    m_pSite = pSite;
    m_pSite->AddRef();


    IHXSite2* pSite2 = NULL;
    m_pSite->QueryInterface(IID_IHXSite2, (void**)&pSite2);
    if (pSite2)
    {
	m_pSite2 = pSite2;
    }

    if (!m_pMISUS->m_bIsAttached)
    {
	IHXInterruptSafe* pInterrupSafe = NULL;

	if (m_pSite->QueryInterface(IID_IHXInterruptSafe, 
				    (void**) &pInterrupSafe) == HXR_OK)
	{
	    m_pMISUS->m_bIsInterrupSafe = pInterrupSafe->IsInterruptSafe();
	    pInterrupSafe->Release();
	}

	m_pMISUS->m_pSingleUser->AttachSite((IHXSite*)m_pMISUS);
	m_pMISUS->m_bIsAttached = TRUE;
    }

    if (m_pMISUS->m_bSetSizeHasBeenCalled)
    {
	m_pSite->SetSize(m_pMISUS->m_size);
    }
    if (m_pMISUS->m_bSetPositionHasBeenCalled)
    {
	m_pSite->SetPosition(m_pMISUS->m_position);
    }
    if (m_pMISUS->m_bSetZOrderHasBeenCalled)
    {
	m_pSite2->SetZOrder(m_pMISUS->m_zorder);
    }


    HXBitmapInfoHeader* pHeader = m_pMISUS->GetBitmapInfoHeader();
    if (pHeader && m_pSite2)
    {
	IHXVideoSurface* pTempSurface;
	if (HXR_OK == m_pSite2->GetVideoSurface(pTempSurface))
	{
	    // so we have a video sufrface in one of our sites 
	    // good enough return HXR_OK.
	    pTempSurface->BeginOptimizedBlt(pHeader);
	    HX_RELEASE(pTempSurface);
	}
    }

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSiteUser::DetachSite
 */
STDMETHODIMP 
CHXMultiInstanceSiteUser::DetachSite()
{
    HX_ASSERT(m_pSite);
    HX_ASSERT(m_pMISUS);

    m_pMISUS->DestroySiteUser((IHXSiteUser*)this);

    HX_RELEASE(m_pSite);
    HX_RELEASE(m_pSite2);

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSiteUser::HandleEvent
 */
STDMETHODIMP 
CHXMultiInstanceSiteUser::HandleEvent(HXxEvent* /*IN*/ pEvent)
{
    if (m_pMISUS)
    {
	if (m_pMISUS->m_pSingleUser)
	{
	    return m_pMISUS->m_pSingleUser->HandleEvent(pEvent);
	}
    }
    return HXR_NOTIMPL;
}

/************************************************************************
 *  Method:
 *    IHXSiteUser::NeedsWindowedSites
 */
STDMETHODIMP_(HXBOOL)
CHXMultiInstanceSiteUser::NeedsWindowedSites()
{
    return FALSE;
}

