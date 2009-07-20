/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxsocketselector.cpp,v 1.4 2006/02/23 22:31:01 ping Exp $ 
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


#include "hxtypes.h"
#include "nettypes.h"
#include "hxcom.h"
#include "hxmap.h"
#include "hxset.h"
#include "hxthread.h"
#include "thrdutil.h"
#include "hxsocketselector.h"

#include "debug.h"
#include "hxassert.h"
#include "hxtlogutil.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif									 

#include "chxmaplongtoobj.h"

// maps per-thread instances
static CHXMapLongToObj m_instances;

HX_RESULT HXSocketSelector::GetThreadInstance(HXSocketSelector*& pInstance, IUnknown* pContext)
{
    HX_RESULT hr = HXR_FAIL;
    
    UINT32 id = HXGetCurrentThreadID();
    HXSocketSelector*& pStored = reinterpret_cast<HXSocketSelector*&> (m_instances[id]);
    if (pStored)
    { 
        HX_ASSERT(pStored->GetTID() == id);

        // already created
        hr = HXR_OK;
        pInstance = pStored;
        pInstance->AddRef();
    }
    else
    {
        // create instance for this thread
        hr =  HXSocketSelector::Create(pStored, pContext);
        if (SUCCEEDED(hr))
        {
            HXLOGL3(HXLOG_NETW, "HXSocketSelector::GetThreadInstance() created instance [%p]...", pStored);
            HX_ASSERT(pStored->GetTID() == id);
        }
        pInstance = pStored; // pass on reference
    }

    return hr;
}

void HXSocketSelector::FinalRelease()
{
    // reset instance in the collection
    UINT32 id = this->GetTID();
    HXSocketSelector*& pStored = reinterpret_cast<HXSocketSelector*&> (m_instances[id]);
    HX_ASSERT(pStored == this);

    HXLOGL3(HXLOG_NETW, "HXSocketSelector::FinalRelease() destroying instance [%p]...", this);
    
    // indicate instance no longer exists
    pStored = 0;

    // continue final release (delete this)
    HXRefCounted::FinalRelease();
}

HX_RESULT HXSocketSelector::Create(HXSocketSelector*& pSelector, IUnknown* pContext)
{
    HX_RESULT hr = HXR_FAIL;

    // create derived (platform) instance
    pSelector = HXSocketSelector::AllocInstance();
    if(pSelector)
    {
        pSelector->AddRef();
        hr = pSelector->Init(pContext);
        if(HXR_OK != hr)
        {
            HX_DELETE(pSelector);
        }
    }
    else 
    {
        hr = HXR_OUTOFMEMORY;
    }
    return hr;
}

HXSocketSelector::HXSocketSelector()
: m_tid(HXGetCurrentThreadID())
, m_pContext(NULL)
{
}

HXSocketSelector::~HXSocketSelector()
{
    if(!m_pool.IsEmpty())
    {
        CHXMapPtrToPtr::Iterator end = m_pool.End();
        for(CHXMapPtrToPtr::Iterator begin = m_pool.Begin(); begin != end; ++begin)
        {
            sockobj_t fd = reinterpret_cast<sockobj_t>(begin.get_key());
            RemoveSocket(fd);
        }

        m_pool.RemoveAll();
    }
    HX_RELEASE(m_pContext);
}


HX_RESULT HXSocketSelector::Init(IUnknown* pContext)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);
    return HXR_OK;
}

//
// Remove a set of sockets from the select pool.
// 
void
HXSocketSelector::RemoveSocketSet(const CHXSet& removeSet)
{
    // cast away const (container lacks const_iterator)
    CHXSet& removeSet_ = const_cast<CHXSet&>(removeSet);
    CHXSet::Iterator end = removeSet_.End();
    for(CHXSet::Iterator begin = removeSet_.Begin(); begin != end; ++begin)
    {
        sockobj_t fd = reinterpret_cast<sockobj_t>(*begin);
        RemoveSocket(fd);
    }
}

//
// Remove all sockets associated with the given handler
// from the select pool.
//
void
HXSocketSelector::RemoveHandler(EventHandler* pHandler)
{
    HXLOGL4(HXLOG_NETW, "HXSocketSelector[%p]::RemoveHandler(): de-selecting fd's for handler [%p]", this, pHandler);
    
    CHXSet removeSet;
    CHXMapPtrToPtr::Iterator end = m_pool.End();
    for(CHXMapPtrToPtr::Iterator begin = m_pool.Begin(); begin != end; ++begin)
    {
        EventHandler* pThisHandler = reinterpret_cast<EventHandler*>(*begin);
        HX_ASSERT(pThisHandler);
        if (pThisHandler == pHandler)
        {
            // add match to set of fd's we will remove (container lacks erase(*iter))
            removeSet.Add(begin.get_key());
        }
    }

    RemoveSocketSet(removeSet);
}


//
// Select socket already in pool.
//
HX_RESULT HXSocketSelector::UpdateSocket(sockobj_t fd, UINT32 eventMask)
{
    HX_ASSERT(INVALID_SOCKET != fd);
    HX_ASSERT(0 != m_pool.Lookup(reinterpret_cast<void*>(fd)));
    return HXR_OK;
}

//
// Ensure socket is selected after re-enabling event. Some platforms
// such as windows will auto-reenable select events when you call 
// certain API. For example, calling recv() auto-selects for FD_READ.
//
HX_RESULT HXSocketSelector::ReEnableSocket(sockobj_t fd, UINT32 eventMask)
{
    // some platforms may wish to re-set FDs for basic select
    HX_ASSERT(INVALID_SOCKET != fd);
    HX_ASSERT(0 != m_pool.Lookup(reinterpret_cast<void*>(fd)));
    return HXR_OK;
}

//
// Add socket to select pool and select.
//
HX_RESULT HXSocketSelector::AddSocket(sockobj_t fd, 
                                      UINT32 eventMask, 
                                      EventHandler* pHandler)
{
    HXLOGL3(HXLOG_NETW, "HXSocketSelector[%p]::AddSocket(): adding fd %lu for handler [%p]", this, fd, pHandler);

    HX_ASSERT(INVALID_SOCKET != fd);
    HX_ASSERT(0 == m_pool.Lookup(reinterpret_cast<void*>(fd)));
    HX_ASSERT(pHandler);

    m_pool.SetAt(reinterpret_cast<void*>(fd), pHandler);
    return UpdateSocket(fd, eventMask);
}

bool HXSocketSelector::HasSocket(sockobj_t fd) const
{
    return 0 != m_pool.Lookup(reinterpret_cast<void*>(fd));
}

//
// Remove socket from select pool.
//
void HXSocketSelector::RemoveSocket(sockobj_t fd)
{
    HXLOGL4(HXLOG_NETW, "HXSocketSelector[%p]::RemoveSocket(): de-selecting and removing fd %lu", this, fd);

    HX_ASSERT(INVALID_SOCKET != fd);
    HX_ASSERT(0 != m_pool.Lookup(reinterpret_cast<void*>(fd)));

    m_pool.RemoveKey(reinterpret_cast<void*>(fd));
}







