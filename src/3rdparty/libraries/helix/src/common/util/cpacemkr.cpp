/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cpacemkr.cpp,v 1.15 2008/04/22 15:03:34 ehyche Exp $
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

/****************************************************************************
 *  Defines
 */
#define WAIT_TIME_GRANULE	5	// in milliseconds


/****************************************************************************
 *  Includes
 */
#include "hxcom.h"
#include "hxtick.h"
#include "hxassert.h"

#include "hxcomm.h"
#include "hxthread.h"
#include "pckunpck.h"
#include "cpacemkr.h"
#define HELIX_FEATURE_LOGLEVEL_NONE // comment out to enable logging
#include "hxtlogutil.h"

#if defined(HELIX_CONFIG_NOSTATICS)
# include "globals/hxglobals.h"
#endif

/****************************************************************************
 *  CVideoPaceMaker             
 */
/****************************************************************************
 *  Statitics
 */


/****************************************************************************
 *  Constructor/Destructor          
 */
CVideoPaceMaker::CVideoPaceMaker(IUnknown* pContext)
    : m_pResponse(NULL)
    , m_pThread(NULL)
    , m_bActive(FALSE)
    , m_bThreadActive(FALSE)
    , m_bThreadIdle(FALSE)
    , m_bSuspend(FALSE)
    , m_bSuspended(FALSE)
    , m_ulBaseTime(0)
    , m_ulInterval(0)
    , m_ulId(0)
    , m_pEvent(NULL)
    , m_lRefCount(0)
    , m_pContext(pContext)
{
    HX_ENABLE_LOGGING(m_pContext);
    HXLOGL4(HXLOG_BVID, "CON CVideoPaceMaker %p", this);
    HX_ADDREF(m_pContext);
}

CVideoPaceMaker::~CVideoPaceMaker(void)
{
    HXLOGL4(HXLOG_BVID, "DES CVideoPaceMaker %p", this);
    if (m_pThread)
    {
        m_pThread->Exit(0);
    }
    HX_RELEASE(m_pThread);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pContext);
}


/****************************************************************************
 *  Main Interface               
 */
/****************************************************************************
 *  CVideoPaceMaker::Start               
 */
STDMETHODIMP CVideoPaceMaker::Start(IHXPaceMakerResponse* pResponse,
				    LONG32 lPriority,
				    ULONG32 ulInterval,
				    ULONG32 &ulId)
{
    HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::Start(,lPriority=%ld,ulInterval=%lu,)",  this, lPriority, ulInterval);
    HX_RESULT retVal = HXR_UNEXPECTED;

    m_bActive = TRUE;

    if ((m_pResponse == NULL) && (m_pThread == NULL))
    {
	retVal = HXR_OK;
    }

    if (retVal == HXR_OK)
    {
	retVal = HXR_INVALID_PARAMETER;
	if (pResponse)
	{
	    m_pResponse = pResponse;
	    pResponse->AddRef();
	    retVal = HXR_OK;
	}
    }

    if (retVal == HXR_OK)
    {
	retVal = HXR_INVALID_PARAMETER;
	if (ulInterval != 0)
	{
	    m_ulInterval = ulInterval;
	    retVal = HXR_OK;
	}
    }

    if (retVal == HXR_OK)
    {
	ulId = m_ulId = GetNextID();	
	retVal = CreateInstanceCCF(CLSID_IHXThread, (void**)&m_pThread, m_pContext);
    }

#ifdef THREADS_SUPPORTED        
    if (retVal == HXR_OK)
    {
	AddRef();
	m_bThreadActive = TRUE;
	retVal = m_pThread->CreateThread(ThreadRoutine, 
					 (void*) this, 0);
	if (FAILED(retVal))
	{
	    m_bThreadActive = FALSE;
	    Release();
	}
    }
#endif        

    if (retVal == HXR_OK)
    {
	retVal = m_pThread->SetPriority(lPriority);
    }

    if (retVal != HXR_OK)
    {
	m_bActive = FALSE;
    }

    return retVal;
}


/****************************************************************************
 *  CVideoPaceMaker::Stop               
 */
STDMETHODIMP CVideoPaceMaker::Stop(void)
{
    HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::Stop()",  this);
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_bActive)
    {
	m_bActive = FALSE;
	retVal = HXR_OK;
    }

    return retVal;
}


/****************************************************************************
 *  CVideoPaceMaker::Stop               
 */
STDMETHODIMP CVideoPaceMaker::Suspend(HXBOOL bSuspend)
{
    HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::Suspend(bSuspend=%lu)",  this, bSuspend);
    HX_RESULT retVal = HXR_UNEXPECTED;

    // Overly complicated exclusive-OR test
    if ((!(m_bSuspend && bSuspend)) && (bSuspend || m_bSuspend))
    {
        // If we are being resumed, then we need to
        // signal the event since the thread is waiting forever
        HXBOOL bSignal = m_bSuspend;
        // Assign the new value
	m_bSuspend = bSuspend;
        // Are we supposed to signal?
        if (bSignal && m_pEvent)
        {
            // Signal the event
            m_pEvent->SignalEvent();
        }
        // Clear the return value
	retVal = HXR_OK;
    }

    return retVal;
}


HX_RESULT CVideoPaceMaker::Signal(void)
{
    HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::Signal()",  this);
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pEvent)
    {
	retVal = m_pEvent->SignalEvent();
    }

    return retVal;
}


STDMETHODIMP CVideoPaceMaker::WaitForStop(void)
{
    HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::WaitForStop() enter",  this);
    HX_RESULT retVal = HXR_FAIL;
    IHXEvent* pEvent = NULL;

    CreateEventCCF((void**)&pEvent, m_pContext, NULL, TRUE); 

    if (pEvent)
    {
	retVal = HXR_OK;
	while (m_bThreadActive)
	{
	    pEvent->Wait(WAIT_TIME_GRANULE);
	}
    }

    HX_RELEASE(pEvent);

    HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::WaitForStop() returns 0x%08x", this, retVal);

    return retVal;
}


STDMETHODIMP CVideoPaceMaker::WaitForSuspend(void)
{
    HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::WaitForSuspend()", this);
    HX_RESULT retVal = HXR_FAIL;
    IHXEvent* pEvent = NULL;

    CreateEventCCF((void**)&pEvent, m_pContext, NULL, TRUE);

    if (pEvent)
    {
	retVal = HXR_OK;
	while (m_bThreadActive && (!m_bSuspended))
	{
	    pEvent->Wait(WAIT_TIME_GRANULE);
	}
    }

    HX_RELEASE(pEvent);

    HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::WaitForSuspend() returns 0x%08x", this, retVal);

    return retVal;
}


STDMETHODIMP CVideoPaceMaker::SetPriority(LONG32 lPriority)
{
    HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::SetPriority(lPriority=%ld)", this, lPriority);
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pThread)
    {
	retVal = m_pThread->SetPriority(lPriority);
    }

    return retVal;
}


STDMETHODIMP CVideoPaceMaker::SetInterval(ULONG32 ulInterval)
{
    HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::SetInterval(ulInterval=%lu)", this, ulInterval);
    m_ulInterval = ulInterval;

    return HXR_OK;
}

/****************************************************************************
 *  Private Methods             
 */
/****************************************************************************
 *  CVideoPaceMaker::OnThreadStart               
 */
void CVideoPaceMaker::OnThreadStart(void)
{
    HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::OnThreadStart()", this);
    HX_ASSERT(m_pResponse);
    HX_ASSERT(!m_pEvent);

    CreateEventCCF((void**)&m_pEvent, m_pContext, NULL, TRUE);

#ifdef THREADS_SUPPORTED    
    m_pResponse->OnPaceStart(m_ulId);
#endif    

}


/****************************************************************************
 *  CVideoPaceMaker::OnThreadEnd               
 */
void CVideoPaceMaker::OnThreadEnd(void)
{
    HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::OnThreadEnd()", this);
    HX_ASSERT(m_pResponse);

    HX_RELEASE(m_pEvent);

#ifdef THREADS_SUPPORTED    
    m_pResponse->OnPaceEnd(m_ulId);
#endif    
    m_pResponse->Release();
    m_pResponse = NULL;
    m_bThreadActive = FALSE;
}


/****************************************************************************
 *  CVideoPaceMaker::DecoderThreadRoutine               
 */
void* CVideoPaceMaker::ThreadRoutine(void* pArg)
{
    CVideoPaceMaker* pThis = (CVideoPaceMaker*) pArg;

    HX_ASSERT(pThis);

    pThis->OnThreadStart();

    do
    {
        // Call the user's OnPace method
        HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::ThreadRoutine() calling OnPace()", pThis);
	pThis->m_bThreadIdle = FALSE;
	pThis->m_pResponse->OnPace(pThis->m_ulId);
	pThis->m_bThreadIdle = TRUE;
        // Check the user's suspend flag
        pThis->m_bSuspended = pThis->m_bSuspend;
        // Get the amount to wait
        UINT32 ulWait = (pThis->m_bSuspended ? HX_EVENT_WAIT_FOREVER : pThis->m_ulInterval);
        // Wait the specified time. If we are suspended, then
        // we will wait forever, until we are resumed OR until
        // the user signals the event.
        HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::ThreadRoutine() enter Wait(%lu)", pThis, ulWait);
        if (pThis->m_pEvent->Wait(ulWait) == HXR_OK)
	    {
		pThis->m_pEvent->ResetEvent();
	    }
        HXLOGL4(HXLOG_BVID, "CVideoPaceMaker[%p]::ThreadRoutine() leave Wait(%lu)", pThis, ulWait);
    } while (pThis->IsActive());

    pThis->OnThreadEnd();
    pThis->Release();

    return NULL;
}


/****************************************************************************
*  IUnknown::AddRef                                            ref:  hxcom.h
*
*  This routine increases the object reference count in a thread safe
*  manner. The reference count is used to manage the lifetime of an object.
*  This method must be explicitly called by the user whenever a new
*  reference to an object is used.
*/
STDMETHODIMP_(ULONG32) CVideoPaceMaker::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/****************************************************************************
*  IUnknown::Release                                           ref:  hxcom.h
*
*  This routine decreases the object reference count in a thread safe
*  manner, and deletes the object if no more references to it exist. It must
*  be called explicitly by the user whenever an object is no longer needed.
*/
STDMETHODIMP_(ULONG32) CVideoPaceMaker::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}

/****************************************************************************
*  IUnknown::QueryInterface                                    ref:  hxcom.h
*
*  This routine indicates which interfaces this object supports. If a given
*  interface is supported, the object's reference count is incremented, and
*  a reference to that interface is returned. Otherwise a NULL object and
*  error code are returned. This method is called by other objects to
*  discover the functionality of this object.
*/
STDMETHODIMP CVideoPaceMaker::QueryInterface(REFIID riid, void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*) this },
		{ GET_IIDHANDLE(IID_IHXPaceMaker), (IHXPaceMaker*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);   
}

LONG32 CVideoPaceMaker::GetNextID()
{
#if defined(HELIX_CONFIG_NOSTATICS)
    const LONG32 zlLastId = 0;
    LONG32& it = HXGlobalInt32::Get(&zlLastId, 0 );
    return InterlockedIncrement(&it);
    
#else
    static LONG32 zlLastId = 0;
    return InterlockedIncrement(&zlLastId);
#endif
}

