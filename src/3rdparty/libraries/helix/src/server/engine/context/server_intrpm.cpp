/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: server_intrpm.cpp,v 1.5 2004/06/02 17:30:30 tmarshall Exp $
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

#include "hxtypes.h"
#include "hxcom.h"

#include "intrpm.h"
#include "hxengin.h"

#include "proc.h"
#include "dispatchq.h"
#include "mem_cache.h"
#include "server_intrpm.h"


PluginMessageCallback::PluginMessageCallback(IHXCallback* cb)
{
    m_pCallback = cb;
    if (m_pCallback)
        m_pCallback->AddRef();
}

PluginMessageCallback::~PluginMessageCallback()
{
    HX_RELEASE(m_pCallback);
}

void
PluginMessageCallback::func(Process* p)
{
    if (m_pCallback)
    {
        m_pCallback->Func();
    }
    delete this;
}

ServerInterPluginMessenger::ServerInterPluginMessenger(Process* p,
                                                       MemCache* pMalloc)
    : m_lRefCount(0)
    , m_pProc(p)
    , m_pMalloc(pMalloc)
{
}

ServerInterPluginMessenger::~ServerInterPluginMessenger()
{
}

void
PluginMessageCallbackUserDeleted::func(Process* p)
{
    if (m_pCallback)
        m_pCallback->Func();
}

/*
 *      IUnknown methods
 */
STDMETHODIMP
ServerInterPluginMessenger::QueryInterface(REFIID riid,
                                           void** ppvObj)
{
    if (IsEqualIID(IID_IUnknown, riid))
    {
        AddRef();
        *ppvObj = (IHXInterPluginMessenger*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(IID_IHXInterPluginMessenger, riid))
    {
        AddRef();
        *ppvObj = (IHXInterPluginMessenger*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(IID_IHXInterPluginMessenger2, riid))
    {
        AddRef();
        *ppvObj = (IHXInterPluginMessenger2*)this;
        return HXR_OK;
    }

    *ppvObj = 0;
    return HXR_NOINTERFACE;
}


STDMETHODIMP_(ULONG32)
ServerInterPluginMessenger::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}


STDMETHODIMP_(ULONG32)
ServerInterPluginMessenger::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *      Method:
 *          IHXInterPluginMessenger::GetPluginContextID
 *      Purpose:
 *          Get a HXPluginContextID for the current server process/thread
 *      for use in the call to IHXInterPluginMessenger::PostCallback.
 */
STDMETHODIMP_(HXPluginContextID)
ServerInterPluginMessenger::GetPluginContextID()
{
    return m_pProc->procnum();
}

/************************************************************************
 *      Method:
 *          IHXInterPluginMessenger::PostCallback
 *      Purpose:
 *          Request a callback to be fired in the server space coresponding
 *      to the HXPluginContextID.
 */
STDMETHODIMP
ServerInterPluginMessenger::PostCallback(HXPluginContextID id,
                                         IHXCallback* cb)
{
    PluginMessageCallback* mc = new (m_pMalloc) PluginMessageCallback(cb);
    m_pProc->pc->dispatchq->send(m_pProc, mc, id);
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXInterPluginMessenger2::PostCallbackWithHandle
 *      Purpose:
 *          Request a callback to be fired in the server space coresponding
 *      to the HXPluginContextID. If pHandle is NULL a new Handle will be
 *  created and it is up to the caller to delete it.  Note that in
 *  subsequent calls to PostCallback a Handle created by a previous call
 *  can be reused for added efficiency.
 */
STDMETHODIMP
ServerInterPluginMessenger::PostCallbackWithHandle(HXPluginContextID id,
                                                   IHXCallback* cb,
                                                   void** pHandle)
{
    PluginMessageCallbackUserDeleted* mc =
        (PluginMessageCallbackUserDeleted*)*pHandle;
    if (!mc)
    {
        mc = new (m_pMalloc) PluginMessageCallbackUserDeleted(cb);
        *pHandle = mc;
    }

    m_pProc->pc->dispatchq->send(m_pProc, mc, id);

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXInterPluginMessenger::FreeHandle
 *      Purpose:
 *          Frees the handle returned by PostCallback
 */
STDMETHODIMP
ServerInterPluginMessenger::FreeHandle(void** pHandle)
{
    //we need to check *pHandle too, since we are going to delete it.
    if (pHandle && *pHandle)
    {
        delete((PluginMessageCallbackUserDeleted*)*pHandle);
        //*pHandle = 0;
        /* comment out this line.
           In the dtor of *pHandle(PluginMessageCallback), we call PN_RELEASE(m_pCallback),
           it will causes m_pCallback being deleted if the refcount goes to 0.
           The problem is that pHandle might be the address a member of m_pCallback(the way we
           used it in asyncfs), in this case we are modifying a piece of memory already been
           released.  This causes memory corruption in other objects, which grap the memory fast
           enoght to be modified by the above line.
        */
    }

    return HXR_OK;
}
