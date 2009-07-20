/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxpropw.cpp,v 1.5 2009/05/30 19:09:56 atin Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#include <stdio.h>
#include <string.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "debug.h"
#include "regdb_misc.h"
#include "hxmon.h"
#include "proc.h"
#include "proc_container.h"
#include "dispatchq.h"
#include "simple_callback.h"
#include "servreg.h"
#include "hxpropw.h"
#include "base_errmsg.h"


/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXPRopWatch::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
HXPropWatch::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXPropWatch))
    {
        AddRef();
        *ppvObj = (IHXPropWatch*)this;
        return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXPropWatch::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
HXPropWatch::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXPropWatch::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
HXPropWatch::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   

HXPropWatch::HXPropWatch(ServerRegistry* registry, Process* proc)
       : m_lRefCount(0), m_response(0), m_registry(registry), 
         m_proc(proc)
{
    if (proc)
    m_procnum = proc->procnum();
}

HXPropWatch::~HXPropWatch()
{
    if (m_response)
    m_response->Release();
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:      HXPropWatch::Init
 *  Input Params:       IHXPropWatchResponse* pResponse, 
 *  Return Value:       STDMETHODIMP
 *  Description:
 *      Initialize with the response object and the registry so that
 *  Watch notifications can be sent back to the respective plugins.
 */
STDMETHODIMP
HXPropWatch::Init(IHXPropWatchResponse* pResponse)
{
    if (pResponse)
    {
    m_response = pResponse;
    m_response->AddRef();
    return HXR_OK;
    }

    return HXR_FAIL;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:      SetWatchOnRoot
 *  Input Params:
 *  Return Value:       UINT32
 *  Description:
 *      set a watch point at the root of the registry hierarchy.
 *  to be notified if any property at this level gets added/modified/deleted.
 */
STDMETHODIMP_(UINT32)
HXPropWatch::SetWatchOnRoot()
{
    DPRINTF(D_REGISTRY&D_ENTRY, ("HXPropWatch::SetWatchOnRoot()\n"));
    ServerPropWatch* cb = new ServerPropWatch;
    cb->m_pResponse = m_response;
    cb->proc = m_proc;
    cb->procnum = m_procnum;
    UINT32 h;
    if ((h = m_registry->SetWatch(cb)))
    return h;
    
    delete cb;
    DPRINTF(D_REGISTRY, ("SetWatchOnRoot() failed\n"));
    return 0;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:      SetWatchByName
 *  Input Params:       const char* prop_name
 *  Return Value:       UINT32
 *  Description:
 *      set a watch point on any Property. if the Property gets
 *  modified/deleted a notification will be sent by the registry.
 */
STDMETHODIMP_(UINT32)
HXPropWatch::SetWatchByName(const char* prop_name)
{
    DPRINTF(D_REGISTRY&D_ENTRY, ("HXPropWatch::SetWatchByName(prop_name(%s)\n)",
        prop_name));
    ServerPropWatch* cb = new ServerPropWatch;
    cb->m_pResponse = m_response;
    cb->proc = m_proc;
    cb->procnum = m_procnum;
    UINT32 h;
    if ((h = m_registry->SetWatch(prop_name, cb)))
    return h;
    
    delete cb;
    DPRINTF(D_REGISTRY, ("SetWatchByName(%s) failed\n", prop_name));
    return 0;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:      SetWatchById
 *  Input Params:       const UINT32 id
 *  Return Value:       UINT32
 *  Description:
 *      set a watch point on any Property. if the Property gets
 *  modified/deleted a notification will be sent by the registry.
 */
STDMETHODIMP_(UINT32)
HXPropWatch::SetWatchById(const UINT32 id)
{
    DPRINTF(D_REGISTRY&D_ENTRY, ("HXPropWatch::SetWatchByName(prop_name(%lu)\n)",
        id));
    ServerPropWatch* cb = new ServerPropWatch;
    cb->m_pResponse = m_response;
    cb->proc = m_proc;
    cb->procnum = m_procnum;
    UINT32 h;
    if ((h = m_registry->SetWatch(id, cb)))
    return h;

    delete cb;
    DPRINTF(D_REGISTRY, ("SetWatchById(%lu) failed\n", id));
    return 0;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:      ClearWatchOnRoot
 *  Input Params:
 *  Return Value:       HX_RESULT
 *  Description:
 *      clear a watch point from the root of the DB hierarchy
 */
STDMETHODIMP
HXPropWatch::ClearWatchOnRoot()
{
    return m_registry->ClearWatch(m_response, m_proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:      ClearWatchByName
 *  Input Params:       const char* prop_name
 *  Return Value:       HX_RESULT
 *  Description:
 *      clear a watch point on a property.
 */
STDMETHODIMP
HXPropWatch::ClearWatchByName(const char* prop_name)
{
    return m_registry->ClearWatch(prop_name, m_response, m_proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:      ClearWatchById
 *  Input Params:       const UINT32 id
 *  Return Value:       HX_RESULT
 *  Description:
 *      clear a watch point on a property.
 */
STDMETHODIMP
HXPropWatch::ClearWatchById(const UINT32 id)
{
    return m_registry->ClearWatch(id, m_response, m_proc);
}

void
PropWatchCallback::func(Process* _proc)
{
    DPRINTF(0x00800000, ("%d: PropWatchCallback(%p)::func(): from m_proc->procnum(%d))"
        " --> _proc->procnum(%d), m_pPlugin(%p)\n", Process::get_procnum(),
        this, m_proc->procnum(), _proc->procnum(), m_pPlugin));

    switch(m_event)
    {
    case DBE_ADDED:
        m_pPlugin->AddedProp(m_hash, m_type, m_parentHash);
        DPRINTF(0x00800000, ("PropWatchCallback(%p)::func -- ADD "
            "m_hash(%lu, %lu)\n", this, m_hash, m_parentHash));
        break;

    case DBE_MODIFIED:
        m_pPlugin->ModifiedProp(m_hash, m_type, m_parentHash);
        DPRINTF(0x00800000, ("PropWatchCallback(%p)::func -- MODIFY "
            "m_hash(%lu, %lu)\n", this, m_hash, m_parentHash));
        break;

    case DBE_DELETED:
            if (m_pDeletedPropCB)
            {
                switch (m_type)
                {
                case PT_COMPOSITE:
                    m_pDeletedPropCB->DeletedComposite(
                        m_hash, m_parentHash, m_bParentBeingNotified, m_pKey);
                    HX_RELEASE(m_pKey);
                    break;

                //XXX others unimplemented for now
                default:
                    m_pPlugin->DeletedProp(m_hash, m_parentHash);
                }
            }
            else
            {
                m_pPlugin->DeletedProp(m_hash, m_parentHash);
            }
        /*
             * Clear the watch so that the registry entry may be deleted.
         * Clear the watch ONLY when this callback is for the property
             * being deleted and NOT for a watch on the parent property,
             * being notified of the child's extermination ;-)
         */
        if (!m_bParentBeingNotified)
            {
                DPRINTF(0x00800000, ("PropWatchCallback(%p)::func -- "
                        "DELETE m_hash(%lu, %lu)\n", 
                        this, m_hash, m_parentHash));
                m_registry->ClearWatch(m_hash, m_pPlugin, _proc);
            }
            else
                DPRINTF(0x00800000, ("PropWatchCallback(%p)::func -- "
                        "PARENT notification -- DELETE "
                        "m_hash(%lu, %lu)\n", this, m_hash, m_parentHash));

        break;

    default:
        ERRMSG(m_proc->pc->error_handler,
               "invalid operation(%d) on registry property with key(%lu, %lu)\n",
           m_event, m_hash, m_parentHash);
        break;
    }
    delete this;
}


PropWatchCallback::PropWatchCallback()
    : m_pPlugin(0)
    , m_pDeletedPropCB(0)
    , m_proc(0)
    , m_procnum(0)
    , m_hash(0)
    , m_type(PT_UNKNOWN)
    , m_event(DBE_ERROR)
    , m_parentHash(0)
    , m_bParentBeingNotified(FALSE)
    , m_registry(0)
    , m_pKey(0)
{
}


PropWatchCallback::~PropWatchCallback()
{
    BOOL bLocked = m_registry->MutexLockIfNeeded(m_proc);

    HX_RELEASE(m_pPlugin);
    HX_RELEASE(m_pDeletedPropCB);
    HX_RELEASE(m_pKey);

    if (bLocked)
    {
        m_registry->MutexUnlock(m_proc);
    }
}
