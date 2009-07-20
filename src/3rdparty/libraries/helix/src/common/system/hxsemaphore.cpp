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
#include "hxassert.h"
#include "debug.h"
#include "hxthread.h"
#include "pckunpck.h"
#include "hxscope_lock.h"
#include "hxsemaphore.h"

HX_RESULT HXSemaphore::MakeSemaphore(HXSemaphore*& pSem, IUnknown* pContext, UINT32 count, const char* pName)
{
    HX_RESULT hr;
    if ((pSem = new HXSemaphore()))
    {
        hr = pSem->Init(pContext, count, pName);
        if (FAILED(hr))
        {
            HX_ASSERT(FALSE);
            HX_DELETE(pSem);
        }
    }
    else
    {
        hr = HXR_OUTOFMEMORY;
    }
    
    return hr;
}
    

HXSemaphore::HXSemaphore() 
: m_pEvent(0)
, m_count(0)
, m_pMutex(0)
{
}

HX_RESULT HXSemaphore::Init(IUnknown* pContext, UINT32 count, const char* pName)
{
    if (pName)
    {
        HX_ASSERT(FALSE); // XXXLCM needs testing/further consideration
        return HXR_NOTIMPL;
    }

    HX_RESULT hr = CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, pContext);  
    if (SUCCEEDED(hr))
    {
        const HXBOOL isManualReset = FALSE; // we want an auto-reset event
	hr = CreateEventCCF((void**)&m_pEvent, pContext, pName, isManualReset);
        if (SUCCEEDED(hr))
        {
            if( count > 0 )
            {
                // Semaphore is available to start.
                Signal(count);
            }
            else
            {
                // Just in case...
                m_pEvent->ResetEvent();
            }
        }
        
    }
    
    return hr;
}
	
HXSemaphore::~HXSemaphore()
{
    HX_RELEASE(m_pEvent);
    HX_RELEASE(m_pMutex);
}

// 0            = locked (unset)
// positive     = available (set)
//
UINT32 HXSemaphore::GetCount() const
{
    HXScopeLock lock(m_pMutex);
    return m_count;
}

void HXSemaphore:: Signal(UINT32 count)
{ 
    HX_ASSERT(count > 0);
    if (count > 0)
    {
        HXScopeLock lock(m_pMutex);
        HXBOOL doSignal = (0 == m_count);
        m_count += count;
        if (doSignal)
        {
            // We transitioned to set. Signal waiter(s)....
            m_pEvent->SignalEvent();
        }
    }
}



HX_RESULT HXSemaphore::Wait(UINT32 msWait) 
{
    // Wait for event that indicates our count is > 0 (set).
    HX_RESULT hr = m_pEvent->Wait(msWait);
    if (SUCCEEDED(hr))
    {
        HXScopeLock lock(m_pMutex);
        HX_ASSERT(m_count > 0);
        m_count -= 1;
        if (m_count > 0)
        {
            // Auto-reset event was reset.Semaphore is still
            // available.Signal next waiter.
            m_pEvent->SignalEvent();
        }
    }

    return hr;
}

#if(0)
HX_RESULT HXSemaphore::Wait(UINT32 msWait) 
{
    HX_RESULT hr = HXR_OK;
    m_pMutex->Lock();
    if(m_count > 0)
    {
        // No need to wait. Decrement and continue.
        m_count -= 1;
        if (0 == m_count)
        {
            // No longer available....
            m_pEvent->ResetEvent();
        }
    }
    else
    {
        // Count is 0. We need to wait.
        m_pMutex->Unlock();
        HX_RESULT hr = m_pEvent->Wait(msWait);
        m_pMutex->Lock();
        if (SUCCEEDED(hr))
        {
            HX_ASSERT(m_count > 0);
            m_count -= 1;
            if (m_count > 0)
            {
                // Auto-reset event was reset.Semaphore is still
                // available. Signal next waiter.
                m_pEvent->SignalEvent();
            }
        }
    }
    m_pMutex->Unlock();
    return hr;
}
#endif