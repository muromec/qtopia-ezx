/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pxtimer.cpp,v 1.6 2004/07/09 18:19:44 hubbe Exp $
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

// include
#include "hxtypes.h"
#include "hxcom.h"
#include "hxengin.h"

// pnmisc
#include "baseobj.h"

// pxcomlib
#include "pxtimer.h"

// pndebug
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

PXTimer::PXTimer()
{
    m_lRefCount        = 0;
    m_ulState          = kStateConstructed;
    m_ulInstance       = 0;
    m_ulInterval       = 0;
    m_pScheduler       = NULL;
    m_pResponse        = NULL;
    m_ulCallbackHandle = 0;
    m_bCallbackPending = FALSE;
    m_bInsideTimerFire = FALSE;
}

PXTimer::~PXTimer()
{
    Deallocate();
}

void PXTimer::Deallocate()
{
    if (IsCallbackPending())
    {
        RemovePendingCallback();
    }
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pResponse);
}

STDMETHODIMP PXTimer::QueryInterface(REFIID riid, void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*)this },
		{ GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);  
}

STDMETHODIMP_(UINT32) PXTimer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32) PXTimer::Release()
{
    
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;

    return 0;
}

STDMETHODIMP PXTimer::Func()
{
    HX_RESULT retVal = HXR_OK;

    if (m_pScheduler && m_pResponse)
    {
        // Schedule another callback m_ulInterval
        // milliseconds from now
        m_ulCallbackHandle = m_pScheduler->RelativeEnter(this, m_ulInterval);
        // Set the callback pending flag
        m_bCallbackPending = TRUE;
        // Make sure we don't allow re-entrancy
        // into TimerFire().
        if (!m_bInsideTimerFire)
        {
            // Set the re-entrancy flag
            m_bInsideTimerFire = TRUE;
            // Call TimerFire on the response interface
            m_pResponse->TimerFire(m_ulInstance);
            // Clear the re-entrancy flag
            m_bInsideTimerFire = FALSE;
        }
    }

    return retVal;
}

HX_RESULT PXTimer::Init(UINT32           ulInstance,
                        IUnknown*        pContext,
                        PXTimerResponse* pResponse)
{
    HX_RESULT retVal = HXR_OK;

    if (pContext && pResponse)
    {
        // Clear out everything if necessary
        Deallocate();
        // Init members
        retVal = pContext->QueryInterface(IID_IHXScheduler,
                                          (void**) &m_pScheduler);
        if (SUCCEEDED(retVal))
        {
            // Save the members
            m_ulInstance       = ulInstance;
            m_pResponse        = pResponse;
            m_ulCallbackHandle = 0;
            m_bCallbackPending = FALSE;
            m_pResponse->AddRef();
            // Set the state
            m_ulState = kStateInitialized;
        }
    }
    else
    {
        retVal = HXR_INVALID_PARAMETER;
    }

    return retVal;
}

HX_RESULT PXTimer::StartTimer(UINT32 ulInterval)
{
    HX_RESULT retVal = HXR_OK;

    if (m_ulState == kStateConstructed)
    {
        retVal = HXR_NOT_INITIALIZED;
    }
    else if (m_ulState == kStateInitialized)
    {
        if (ulInterval >= kMinInterval)
        {
            // Save the interval
            m_ulInterval = ulInterval;
            // Schedule the first callback
            m_ulCallbackHandle = m_pScheduler->RelativeEnter(this, m_ulInterval);
            // Set the callback flag
            m_bCallbackPending = TRUE;
            // Set the state
            m_ulState = kStateActive;
        }
        else
        {
            retVal = HXR_INVALID_PARAMETER;
        }
    }
    else if (m_ulState == kStateActive)
    {
        // Timer is already started, so no need to
        // do anything...
    }

    return retVal;
}

void PXTimer::StopTimer()
{
    if (m_ulState == kStateConstructed ||
        m_ulState == kStateInitialized)
    {
        // Timer has not been started; therefore,
        // we don't need to do anything to stop it.
    }
    else if (m_ulState == kStateActive)
    {
        // Remove any pending callback
        if (IsCallbackPending())
        {
            RemovePendingCallback();
        }
        // Reset the state
        m_ulState = kStateInitialized;
    }
}
