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

#include "hxtypes.h"
#include "hxthread.h"
#include "thrdutil.h"
#include "hxscope_lock.h"
#include "debug.h"
#include "hxassert.h"
#include "hxheap.h"
#include "hxtlogutil.h"
#include "hxthreadmessagesink.h"
#include "pckunpck.h"

#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif  


#include "chxmaplongtoobj.h"

// maps per-thread instances
static CHXMapLongToObj m_instances;

HX_RESULT HXThreadMessageSink::GetThreadInstance(HXThreadMessageSink*& pInstance, IUnknown* pContext)
    {
    HX_RESULT hr = HXR_FAIL;
    
    UINT32 id = HXGetCurrentThreadID();

    HXThreadMessageSink*& pStored = reinterpret_cast<HXThreadMessageSink*&> (m_instances[id]);
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
#if defined(_WINDOWS)
        // create instance for this thread
        hr =  HXThreadMessageSink::Create(pStored, pContext);
#else
        HX_ASSERT(false);
        hr = HXR_NOTIMPL; 
#endif        
        if (SUCCEEDED(hr))
        {
            HXLOGL3(HXLOG_THRD, "HXThreadMessageSink::GetThreadInstance() created instance [%p]...", pStored);
            HX_ASSERT(pStored->GetTID() == id);
        }
        pInstance = pStored; // pass on reference
    }

    return hr;
}

void HXThreadMessageSink::FinalRelease()
{
    // reset instance in the collection
    UINT32 id = this->GetTID();
    HXThreadMessageSink*& pStored = reinterpret_cast<HXThreadMessageSink*&> (m_instances[id]);
    HX_ASSERT(pStored == this);

    HXLOGL3(HXLOG_THRD, "HXThreadMessageSink::FinalRelease() destroying instance [%p]...", this);
    
    // indicate sink no longer exists
    pStored = 0;

    // continue final release (delete this)
    HXRefCounted::FinalRelease();
}

HXThreadMessageSink::HXThreadMessageSink()
: m_handle(INVALID_MSGSINK_HANDLE)
, m_pMutex(NULL)
, m_tid(HXGetCurrentThreadID())
, m_pContext(NULL)
{
}

HXThreadMessageSink::~HXThreadMessageSink()
{
    HX_ASSERT( m_handlers.IsEmpty() );
    m_handlers.RemoveAll();
    HX_RELEASE(m_pMutex);
    HX_RELEASE(m_pContext);
}

HX_RESULT HXThreadMessageSink::Init(IUnknown* pContext)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);
    if (!m_pMutex)
    {
        return HXR_OUTOFMEMORY;
    }
    return HXR_OK;
}
// 
// add handler for given message
//
HX_RESULT HXThreadMessageSink::AddHandler(UINT32 msg, MessageHandler* pHandler)
{
    HX_RESULT res = HXR_FAIL;
    
    HXLOGL3(HXLOG_THRD, "HXThreadMessageSink[%p]::AddHandler(): registering handler [%p] for msg %lu", this, pHandler, msg);
    HX_ASSERT(pHandler);

    if( pHandler )
    {
        HXScopeLock lock(m_pMutex);
        void* pTmp = NULL;
        
        if( m_handlers.Lookup(reinterpret_cast<void*>(msg), pTmp ))
        {
            //We already have a handler. Make sure it is the same
            //one. If not, it is an error.
            if( pTmp == reinterpret_cast<void*>(pHandler) )
            {
                res = HXR_UNEXPECTED;
            }
        }
        else
        {
            //We don't have a handler for this one yet.
            m_handlers.SetAt(reinterpret_cast<void*>(msg), pHandler );
            res = HXR_OK;
        }
    }
    
    return res;
}


//
// remove handler for given message 
//
HX_RESULT HXThreadMessageSink::RemoveHandler(UINT32 msg)
{
    HXLOGL3(HXLOG_THRD, "HXThreadMessageSink[%p]::RemoveHandler(): msg %lu\n", this, msg);

    HX_RESULT hr = HXR_OK;

    HXScopeLock lock(m_pMutex);
    HX_ASSERT(0 != m_handlers.Lookup(reinterpret_cast<void*>(msg)));
    if (!m_handlers.RemoveKey(reinterpret_cast<void*>(msg)))
    {
        hr = HXR_FAIL;
        HX_ASSERT(false);
    }
    
    return hr;
}

HXBOOL HXThreadMessageSink::IsHandled(UINT32 msg)
{
    HXBOOL retVal = FALSE;

    HXScopeLock lock(m_pMutex);
    if( !m_handlers.IsEmpty() )
    {
        retVal = (0 != m_handlers.Lookup(reinterpret_cast<void*>(msg)));
    }
    return retVal;
}











