/* ***** BEGIN LICENSE BLOCK *****
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxausvc.h"
#include "hxengin.h"
#include "timeval.h"
#include "hxtick.h"
#include "hxassert.h"
#include "fakeaudiodevice.h"

#include "hxheap.h"
#if defined(_DEBUG)
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

CHXFakeAudioDevice::CHXFakeAudioDevice(IUnknown* pContext, UINT32 ulGranularity)
    : m_pContext(pContext)
    , m_pScheduler(NULL)
    , m_lRefCount(0)
    , m_pResponse(NULL)
    , m_usVolume(0)
    , m_usMinVolume(0)
    , m_usMaxVolume(0)
    , m_pCallbackTime(NULL)
    , m_ulLastCallbackTime(0)
    , m_ulLastCallbackTick(0)
    , m_ulGranularity(ulGranularity)
    , m_CallbackID(0)
    , m_eState(HXFakeAudioDeviceStateClosed)
{
    HX_ADDREF(m_pContext);
    memset(&m_AudioFormat, 0, sizeof(HXAudioFormat));
}

CHXFakeAudioDevice::~CHXFakeAudioDevice()
{
    Close(TRUE);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pScheduler);
    HX_DELETE(m_pCallbackTime);
}

STDMETHODIMP CHXFakeAudioDevice::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal = HXR_NOINTERFACE;

    if (ppvObj)
    {
        // Set default
        *ppvObj = NULL;
        // Switch on riid
        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = (IUnknown*) (IHXAudioDevice*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXAudioDevice))
        {
            *ppvObj = (IHXAudioDevice*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXCallback))
        {
            *ppvObj = (IHXCallback*) this;
            retVal  = HXR_OK;
        }
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CHXFakeAudioDevice::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXFakeAudioDevice::Release()
{
    HX_ASSERT(m_lRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_lRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP CHXFakeAudioDevice::Open(const HXAudioFormat* pAudioFormat, IHXAudioDeviceResponse* pStreamResponse)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (pAudioFormat && pStreamResponse && m_pContext)
    {
        // Copy the audio format
        m_AudioFormat = *pAudioFormat;
        // Save the response
        HX_RELEASE(m_pResponse);
        m_pResponse = pStreamResponse;
        m_pResponse->AddRef();
        // Reset the callback time and callback tick
        Reset();
        // Allocate a Timeval
        HX_DELETE(m_pCallbackTime);
        m_pCallbackTime = new Timeval;
        if (m_pCallbackTime)
        {
            // QI our context for IHXScheduler
            HX_RELEASE(m_pScheduler);
            retVal = m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
            if (SUCCEEDED(retVal))
            {
                // Set the state
                m_eState = HXFakeAudioDeviceStateOpened;
            }
        }
    }

    return retVal;
}

STDMETHODIMP CHXFakeAudioDevice::Close(const HXBOOL bFlush)
{
    // Clear any pending callbacks
    ClearCallback();
    // Release the response interface
    HX_RELEASE(m_pResponse);
    // Set the state
    m_eState = HXFakeAudioDeviceStateClosed;

    return HXR_OK;
}

STDMETHODIMP CHXFakeAudioDevice::Resume()
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pScheduler && m_pResponse && m_pCallbackTime && m_eState != HXFakeAudioDeviceStateClosed)
    {
        // Start up the timeline if we are in the opened or
        // paused states. If we are already in the resumed state,
        // then this is a no-op.
        if (m_eState == HXFakeAudioDeviceStateOpened || m_eState == HXFakeAudioDeviceStatePaused)
        {
            // Compute the callback times
            HXTimeval lTime          = m_pScheduler->GetCurrentSchedulerTime();
            m_pCallbackTime->tv_sec  = lTime.tv_sec;
            m_pCallbackTime->tv_usec = lTime.tv_usec;
            *m_pCallbackTime        += (int) (m_ulGranularity * 1000);
            m_ulLastCallbackTick     = HX_GET_BETTERTICKCOUNT();
            // Set the state
            m_eState = HXFakeAudioDeviceStateResumed;
            // Give an immediate callback with the current time
	    AddRef();	// Make sure OnTimeSync does not destroy us
            m_pResponse->OnTimeSync(m_ulLastCallbackTime);
            // Since the call to OnTimeSync() can change our state,
            // then we should check the state before scheduling the callback
            if (m_eState == HXFakeAudioDeviceStateResumed)
            {
                m_CallbackID = m_pScheduler->AbsoluteEnter(this, *((HXTimeval*) m_pCallbackTime));
            }
	    Release();
        }
        // Clear the return value
        retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CHXFakeAudioDevice::Pause()
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_eState != HXFakeAudioDeviceStateClosed)
    {
        if (m_eState == HXFakeAudioDeviceStateResumed)
        {
            // Clear any pending callback
            ClearCallback();
            // Get the current tick
            UINT32 ulTick = HX_GET_BETTERTICKCOUNT();
            // Update the time
            m_ulLastCallbackTime += (ulTick - m_ulLastCallbackTick);
        }
        // Set the state
        m_eState = HXFakeAudioDeviceStatePaused;
        // Clear the return value
        retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CHXFakeAudioDevice::Write(const HXAudioData* pAudioData)
{
    // Not expecting to get called with Write
    HX_ASSERT(FALSE && "CHXFakeAudioDevice::Write");
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL) CHXFakeAudioDevice::InitVolume(const UINT16 uMinVolume, const UINT16 uMaxVolume)
{
    m_usMinVolume = uMinVolume;
    m_usMaxVolume = uMaxVolume;
    return TRUE;
}

STDMETHODIMP CHXFakeAudioDevice::SetVolume(const UINT16 uVolume)
{
    UINT16 usVolume = uVolume;
    if (usVolume < m_usMinVolume)
    {
        usVolume = m_usMinVolume;
    }
    else if (usVolume > m_usMaxVolume)
    {
        usVolume = m_usMaxVolume;
    }
    m_usVolume = usVolume;

    return HXR_OK;
}

STDMETHODIMP_(UINT16) CHXFakeAudioDevice::GetVolume()
{
    return m_usVolume;
}

STDMETHODIMP CHXFakeAudioDevice::Reset()
{
    ClearCallback();
    m_ulLastCallbackTime = 0;
    m_ulLastCallbackTick = 0;
    // Reset the state. If we are closed,
    // stay closed. If we are opened stay opened.
    // If we are paused or resumed, go to open.
    if (m_eState == HXFakeAudioDeviceStatePaused ||
        m_eState == HXFakeAudioDeviceStateResumed)
    {
        m_eState = HXFakeAudioDeviceStateOpened;
    }

    return HXR_OK;
}

STDMETHODIMP CHXFakeAudioDevice::Drain()
{
    // Not expecting to get called
    HX_ASSERT(FALSE && "CHXFakeAudioDevice::Drain");
    return HXR_OK;
}

STDMETHODIMP CHXFakeAudioDevice::CheckFormat(const HXAudioFormat* pAudioFormat)
{
    // We accept all formats!
    return HXR_OK;
}

STDMETHODIMP CHXFakeAudioDevice::GetCurrentAudioTime(REF(ULONG32) ulCurrentTime)
{
    // Get the time at the last callback
    UINT32 ulTime = m_ulLastCallbackTime;
    // Are we in a resumed state?
    if (m_eState == HXFakeAudioDeviceStateResumed)
    {
        // In this state, the clock should still be running,
        // so we need to interpolate between the time at
        // the last callback and now
        //
        // Get the current tick
        UINT32 ulTick = HX_GET_BETTERTICKCOUNT();
        // Compute the difference between the last callback
        // tick and now and add this to the time
        ulTime += (ulTick - m_ulLastCallbackTick);
    }
    // Set the out parameter
    ulCurrentTime = ulTime;

    return HXR_OK;
}

STDMETHODIMP CHXFakeAudioDevice::Func()
{
    // Clear the callback handle, so we know
    // we don't have a pending callback
    m_CallbackID = 0;
    // Are we still in a resumed state?
    if (m_eState == HXFakeAudioDeviceStateResumed && m_pResponse && m_pScheduler)
    {
        // Get the current tick
        UINT32 ulTick = HX_GET_BETTERTICKCOUNT();
        // Add this to the current time
        m_ulLastCallbackTime += (ulTick - m_ulLastCallbackTick);
        // Save the current tick
        m_ulLastCallbackTick  = ulTick;
        // Call the response's OnTimeSync()
	AddRef();   // Make sure the OnTimeSync does not destroy us
        m_pResponse->OnTimeSync(m_ulLastCallbackTime);
        // Since a call to OnTimeSync could change our state
        // then we need to check the state again before scheduling
        // another callback
        if (m_eState == HXFakeAudioDeviceStateResumed)
        {
            *m_pCallbackTime += (int) (m_ulGranularity * 1000);
            m_CallbackID      = m_pScheduler->AbsoluteEnter(this, *((HXTimeval*) m_pCallbackTime));
        }
	Release();
    }

    return HXR_OK;
}

HX_RESULT STDAPICALLTYPE CHXFakeAudioDevice::HXCreateFakeAudioDevice(IUnknown*            pContext,
                                                                     UINT32               ulGranularity,
                                                                     REF(IHXAudioDevice*) rpAudioDevice)
{
    HX_RESULT retVal = HXR_FAIL;

    HX_ASSERT(rpAudioDevice == NULL);

    if (pContext && ulGranularity)
    {
        CHXFakeAudioDevice* pObj = new CHXFakeAudioDevice(pContext, ulGranularity);
        if (pObj)
        {
            retVal = pObj->QueryInterface(IID_IHXAudioDevice, (void**) &rpAudioDevice);
        }
        if (FAILED(retVal))
        {
            HX_DELETE(pObj);
        }
    }

    return retVal;
}

void CHXFakeAudioDevice::ClearCallback()
{
    // Do we have a pending callback?
    if (m_pScheduler && m_CallbackID)
    {
        // Remove the callback
        m_pScheduler->Remove(m_CallbackID);
        // Clear the callback ID
        m_CallbackID = 0;
    }
}

