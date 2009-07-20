/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxthreadedsocket.cpp,v 1.21 2008/12/02 14:00:10 gahluwalia Exp $ 
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


/*

HXThreadedSocket adapts CHXSocket so inbound socket data is automatically
and continually buffered at the application layer. 

Read buffering begins as soon as SelectEvents() is called with HX_SOCK_EVENT_READ.
Reads will continue until the buffer limit is exceeded, at which point reading pauses.
The socket user can specify the buffer limit via a socket option.

Reading is resumed (after pausing) once the buffer drains below the max limit
as a result of the socket user calling a read method on the socket (on the
parent thread).

All function calls (except construction) for the underlying socket are performed
on the net thread. This is done even though in some cases it may be safe to call
underlying socket methods on the parent thread directly. We make no assumption
about the thread safety of the underlying socket.
    
*/


#include "hxtypes.h"
#include "hxassert.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxscope_lock.h"
#include "hxnet.h"
#include "hxprefutil.h"
#include "hxclassdispatchtasks.h"
#include "hxthreadtaskdriver.h"
#include "hxthreadedsocket.h"

#include "hxtlogutil.h"
#include "debug.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

// com interface implementation
BEGIN_INTERFACE_LIST(HXThreadedSocket)
    INTERFACE_LIST_ENTRY(IID_IHXSocket, IHXSocket)
#ifdef HELIX_FEATURE_SECURE_SOCKET
    INTERFACE_LIST_ENTRY(IID_IHXSecureSocket, IHXSecureSocket)
#endif
    INTERFACE_LIST_ENTRY(IID_IHXSocketResponse, IHXSocketResponse)
    INTERFACE_LIST_ENTRY(IID_IHXMulticastSocket, IHXMulticastSocket)
    INTERFACE_LIST_ENTRY(IID_IHXCallback, IHXCallback)
    INTERFACE_LIST_ENTRY(IID_IHXInterruptSafe, IHXInterruptSafe)
END_INTERFACE_LIST


HX_RESULT 
HXThreadedSocket::Create(IUnknown* pContext,
                         IHXSocket* pActualSock,
                         IHXSocket*& pThreadedSock)
{
    HX_ASSERT(pActualSock);

    HX_RESULT hr = HXR_FAIL;
        
    HXThreadedSocket* pSock = new HXThreadedSocket();
    if(pSock)
    {
        pSock->AddRef();
        hr = pSock->DoConstructInit(pContext, pActualSock);
        if( SUCCEEDED(hr))
        {
            hr = pSock->QueryInterface(IID_IHXSocket, reinterpret_cast<void**>(&pThreadedSock));
        }
        HX_RELEASE(pSock);
    }
    else
    {
        hr = HXR_OUTOFMEMORY;
    }
   
    return hr;
}

#ifdef HELIX_FEATURE_SECURE_SOCKET
HX_RESULT 
HXThreadedSocket::CreateSecureSocket(IUnknown* pContext,
                         IHXSecureSocket* pActualSock,
                         IHXSecureSocket*& pThreadedSock)
{
    HX_ASSERT(pActualSock);

    HX_RESULT hr = HXR_FAIL;
        
    HXThreadedSocket* pSock = new HXThreadedSocket();
    if(pSock)
    {
        pSock->AddRef();
        hr = pSock->DoConstructInit(pContext, pActualSock);
        if( SUCCEEDED(hr))
        {
            hr = pSock->QueryInterface(IID_IHXSecureSocket, reinterpret_cast<void**>(&pThreadedSock));
        }
        HX_RELEASE(pSock);
    }
    else
    {
        hr = HXR_OUTOFMEMORY;
    }
   
    return hr;
}

HX_RESULT HXThreadedSocket::DoConstructInit(IUnknown* pContext, IHXSecureSocket* pSecSock)
{
    HX_RESULT hr = HXR_OUTOFMEMORY;
    HX_ASSERT(pContext);
    HX_ASSERT(pSecSock);
    IHXSocket* pSock = NULL;
    pSecSock->QueryInterface(IID_IHXSocket, reinterpret_cast<void**>(&pSock));
    if(pSock)
    {
        hr = DoConstructInit(pContext, pSock);
    }
    HX_RELEASE(pSock);
    return hr;
}

#endif

HXThreadedSocket::HXThreadedSocket() 
: m_pSock(0)
, m_pResponse(0)
, m_pDriver(0)
, m_inReadEventPostingLoop(FALSE)
, m_state(TS_OPEN)
, m_selectedEvents(0)
, m_cbMaxInbound(0)
, m_pScheduler(0)
, m_hCallback(0)
, m_isResponseSafe(FALSE)
, m_pSchedulerMutex(0)
#ifdef HELIX_FEATURE_SECURE_SOCKET
,m_pSecureSock(0)
#endif
{
}


HXThreadedSocket::~HXThreadedSocket()
{
    HX_ASSERT(!m_pSock); // you should call Close()
    HX_RELEASE(m_pDriver);
    Close();
}


HX_RESULT HXThreadedSocket::DoConstructInit(IUnknown* pContext, IHXSocket* pSock)
{
    HX_ASSERT(pContext);
    HX_ASSERT(pSock);

    m_pContext = pContext;
    m_pContext->AddRef();

    m_pInbound = new HXSocketData(pContext);
    m_pOutbound = new HXSocketData(pContext);

    m_pContext->QueryInterface(IID_IHXScheduler, reinterpret_cast<void**>(&m_pScheduler));
    HX_ASSERT(m_pScheduler);

    // the underlying socket
    m_pSock = pSock;
    m_pSock->AddRef();
    m_pSock->SetResponse(this);

    HXSockType type = pSock->GetType();
    if (HX_SOCK_TYPE_NONE != type)
    {
        // we can go ahead and do this now (otherwise wait until Init() called)
        InitBufferLimits(type);
    }
    
    // get task driver for communication between threads
    HX_RESULT hr = HXThreadTaskDriver::GetInstance(m_pDriver, m_pContext); 
    if (SUCCEEDED(hr))
    {
        hr = m_events.Init();
        if (SUCCEEDED(hr))
        {
            // for synchronizing access to the pointer and m_hCallback (the scheduler itself is thread safe)
	    hr = CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pSchedulerMutex, m_pContext);	
        }
    }
   
    return hr;
}

// send message to network thread; don't wait for completion
HX_RESULT HXThreadedSocket::Send(HXClassDispatchTask<HX_RESULT>* pTask)
{
    HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::Send()", this);
    HX_RESULT hr = HXR_FAIL;
    HX_ASSERT(pTask);
    if (pTask)
    {
        pTask->AddRef();
        HX_ASSERT(m_pDriver);
        if (m_pDriver)
        {
            hr = m_pDriver->Send(pTask);
        }
        else
        {
            HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::Send(): no driver (closed?)", this);
            hr = HXR_UNEXPECTED;
        }
        HX_RELEASE(pTask);
    }
    else
    {
        hr = HXR_OUTOFMEMORY;
    }
    return hr;
}

// send message to network thread and wait for completion
//
// for tasks that return value that is not an HX_RESULT
HX_RESULT HXThreadedSocket::SendWaitHelper(HXThreadTask* pTask)
{
    // send message to network thread and wait for completion
    HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::SendWaitHelper()", this);
    
    HX_RESULT hr = HXR_FAIL;
    HX_ASSERT(pTask);
    if (pTask)
    {
        pTask->AddRef();
        HX_ASSERT(m_pDriver);
        if (m_pDriver)
        {
            hr = m_pDriver->SendAndWait(pTask);
        }
        else
        {
            HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::SendWaitHelper(): no driver (closed?)", this);
            hr = HXR_UNEXPECTED;
        }
        HX_RELEASE(pTask);
    }
    else
    {
        hr = HXR_OUTOFMEMORY;
    }
    return hr;
}

// send message to network thread and wait for completion
//
// for tasks that return HX_RESULT
HX_RESULT HXThreadedSocket::SendWait(HXClassDispatchTask<HX_RESULT>* pTask)
{
    HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::SendWait()", this);

    HX_RESULT hr = HXR_FAIL;
    HX_ASSERT(pTask);
    if (pTask)
    {
        pTask->AddRef();
        HX_ASSERT(m_pDriver);
        if (m_pDriver)
        {
            // send message to net thread and wait for completion
            hr = m_pDriver->SendAndWait(pTask);
            if (HXR_OK == hr)
            {
                // get result from completed task
                hr = pTask->GetReturnValue();
            }
        }
        else
        {
            HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::SendWait(): no driver (closed?)", this);
            hr = HXR_UNEXPECTED;
        }
        HX_RELEASE(pTask);
    }
    else
    {
        hr = HXR_OUTOFMEMORY;
    }
    return hr;
}


STDMETHODIMP_(HXSockFamily)
HXThreadedSocket::GetFamily()
{
    HXClassDispatchTask<HXSockFamily>* pTask =  AllocDispatch0(m_pSock, &IHXSocket::GetFamily);
    HXSockFamily ret = HXSockFamily();
    if(pTask)
    {
        pTask->AddRef();
        SendWaitHelper(pTask);
        ret = pTask->GetReturnValue();
        pTask->Release();
    }
    return ret;
}

STDMETHODIMP_(HXSockType)
HXThreadedSocket::GetType()
{
    HXClassDispatchTask<HXSockType>* pTask =  AllocDispatch0(m_pSock, &IHXSocket::GetType);
    HXSockType ret = HXSockType();
    if(pTask)
    {
        pTask->AddRef();
        SendWaitHelper(pTask);
        ret = pTask->GetReturnValue();
        pTask->Release();
    }
    return ret;
}

STDMETHODIMP_(HXSockProtocol)
HXThreadedSocket::GetProtocol()
{
    HXClassDispatchTask<HXSockProtocol>* pTask =  AllocDispatch0(m_pSock, &IHXSocket::GetProtocol);
    HXSockProtocol ret = HXSockProtocol();
    if(pTask)
    {
        pTask->AddRef();
        SendWaitHelper(pTask);
        ret = pTask->GetReturnValue();
        pTask->Release();
    }
    return ret;
}

// set buffer limits based on family
void HXThreadedSocket::InitBufferLimits(HXSockType type)
{
    const char* pKey = 0;
    UINT32 cbDefaultMaxInbound = 0;
    if (HX_SOCK_TYPE_TCP == type)
    {
        // inbound buffer is not really needed for TCP (reliable/windowed)
        pKey = "ThreadedSockTCPInboundBufSize";
        cbDefaultMaxInbound = 8 * 1024; // 8K
    }
    else
    {
        HX_ASSERT(HX_SOCK_TYPE_UDP == type);
        pKey = "ThreadedSockUDPInboundBufSize";
        cbDefaultMaxInbound = 2 * 1024 * 1024; // 2M
    }

    m_cbMaxInbound = cbDefaultMaxInbound;
    ReadPrefUINT32(m_pContext, pKey, m_cbMaxInbound);
    if (0 == m_cbMaxInbound )
    {
        // 0 explicitly specified in prefs; use app default
        m_cbMaxInbound = cbDefaultMaxInbound;
    }

    HXLOGL3(HXLOG_NETW, "HXThreadedSocket[%p]::InitBufferLimits(): inbound buffer limit = %lu bytes", this, m_cbMaxInbound); 
}


STDMETHODIMP
HXThreadedSocket::Init(HXSockFamily f, HXSockType t, HXSockProtocol p)
{
    HXLOGL3(HXLOG_NETW, "HXThreadedSocket[%p]::Init()", this);
    InitBufferLimits(t);
    return SendWait( AllocDispatch3(m_pSock, &IHXSocket::Init, f,t,p) ); 
}

STDMETHODIMP
HXThreadedSocket::SetResponse(IHXSocketResponse* pResponse)
{
    HX_RELEASE(m_pResponse);
    m_pResponse = pResponse;
    if(m_pResponse)
    {
        // take it
        m_pResponse->AddRef();

        // see if response is thread safe; we'll report 'safe' based on response
        IHXInterruptSafe* pSafe = 0;
        
        // by default we will process on app thread only
        HXBOOL useIntSafe = FALSE;
        ReadPrefBOOL(m_pContext, "ThreadedSockIntSafe", useIntSafe);
        HXLOGL3(HXLOG_NETW, "HXThreadedSocket::SetResponse(): PREF: threaded socket use int safe = '%s'", useIntSafe ? "yes" : "no");

        if (useIntSafe)
        {
            m_pResponse->QueryInterface(IID_IHXInterruptSafe, reinterpret_cast<void**> (&pSafe));
        }
        if (pSafe)
        {
            // if 'safe' we get callbacks on both app and core threads; processing on the core
            // thread can interfere with the ui (app) thread because it runs at a higher priority
            m_isResponseSafe = pSafe->IsInterruptSafe();
            HX_RELEASE(pSafe);
        }
        else
        {
            m_isResponseSafe = FALSE;
        }
    }

    return HXR_OK;
}

STDMETHODIMP
HXThreadedSocket::CreateSockAddr(IHXSockAddr** ppAddr)
{
    return SendWait( AllocDispatch1(m_pSock, &IHXSocket::CreateSockAddr, ppAddr) ); 
}


STDMETHODIMP
HXThreadedSocket::Bind(IHXSockAddr* pAddr)
{
    HXLOGL3(HXLOG_NETW, "HXThreadedSocket[%p]::Bind()", this);
    return SendWait( AllocDispatch1(m_pSock, &IHXSocket::Bind, pAddr) ); 
}


STDMETHODIMP
HXThreadedSocket::ConnectToOne(IHXSockAddr* pAddr)
{
    HXLOGL3(HXLOG_NETW, "HXThreadedSocket[%p]::ConnectToOne()", this);
    return SendWait( AllocDispatch1(m_pSock, &IHXSocket::ConnectToOne, pAddr) ); 
}

STDMETHODIMP
HXThreadedSocket::ConnectToAny(UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    HXLOGL3(HXLOG_NETW, "HXThreadedSocket[%p]::ConnectToAny()", this);
    return SendWait( AllocDispatch2(m_pSock, &IHXSocket::ConnectToAny, nVecLen, ppAddrVec) ); 
}


STDMETHODIMP
HXThreadedSocket::GetLocalAddr(IHXSockAddr** ppAddr)
{
    return SendWait( AllocDispatch1(m_pSock, &IHXSocket::GetLocalAddr, ppAddr) ); 
}

STDMETHODIMP
HXThreadedSocket::GetPeerAddr(IHXSockAddr** ppAddr)
{
    return SendWait( AllocDispatch1(m_pSock, &IHXSocket::GetPeerAddr, ppAddr) ); 

}

STDMETHODIMP
HXThreadedSocket::SelectEvents(UINT32 uEventMask)
{
    HXLOGL3(HXLOG_NETW, "HXThreadedSocket[%p]::SelectEvents(%lu)", this, uEventMask); 
    m_selectedEvents = uEventMask;
    if(m_pResponse && (uEventMask & HX_SOCK_EVENT_READ))
    {
        // ensure read event goes out if we have already inbound 
        // data; unlikely but possible for certain legal cases
        EnsureReadEventPosted();
    }

    return SendWait( AllocDispatch1(m_pSock, &IHXSocket::SelectEvents, uEventMask) ); 
}


STDMETHODIMP_(INT32)
HXThreadedSocket::Peek(IHXBuffer** ppBuf)
{
    if (m_pInbound && (m_pInbound->GetCount() > 0))
    {
        m_pInbound->PeekHead(*ppBuf);
        return HXR_OK;
    }
    return HXR_FAIL;
}

STDMETHODIMP
HXThreadedSocket::PeekFrom(IHXBuffer** ppBuf, IHXSockAddr** ppAddr)
{
    HX_ASSERT(ppBuf);
    HX_ASSERT(ppAddr);
    if (m_pInbound && (m_pInbound->GetCount() > 0))
    {
        m_pInbound->PeekHead(*ppBuf, *ppAddr);
        return HXR_OK;
    }
    return HXR_FAIL;
}


HXBOOL
HXThreadedSocket::IsReadable() const
{
    // return 'true' if read methods can succeed
    return ( (m_pInbound && m_pInbound->GetCount() > 0 && m_state <= TS_ENDSTREAM) || m_state == TS_ENDSTREAM);
}


STDMETHODIMP_(HXBOOL)
HXThreadedSocket::IsInterruptSafe()
{
    return m_isResponseSafe;
}


STDMETHODIMP_(INT32)
HXThreadedSocket::Read(IHXBuffer** ppBuf)
{    
    return ReadFrom(ppBuf, 0); 
}


STDMETHODIMP
HXThreadedSocket::ReadFrom(IHXBuffer** ppBuf, IHXSockAddr** ppAddr)
{
    HX_RESULT hr = HXR_FAIL;

    HX_ASSERT(ppBuf);

    if (m_pInbound)
    {
	hr = HXR_OK;
	// ensure read event is set to indicate that response needs further read events posted
	m_selectedEvents |= HX_SOCK_EVENT_READ;

	if (m_pInbound->GetCount() > 0 && m_state <= TS_ENDSTREAM)
	{
	    if (ppAddr)
	    {
		m_pInbound->RemoveHead(*ppBuf, *ppAddr);
	    }
	    else
	    {
		m_pInbound->RemoveHead(*ppBuf);
	    }

	    HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::ReadFrom(): got next buffer: buffer count = %lu", this, m_pInbound->GetCount());

	    // inbound buffer may be full at this point if:
	    //  (a) writer added since we removed; or 
	    //  (b) mixed size buffers in queue (last removed relatively small to recently added)
	    //
	    if (m_pInbound->TryResetFullSignal())
	    {
		// full signal was re-set; space is now available; notify writer (net thread) to resume
		HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::ReadFrom(): full signal detected", this);
		HXClassDispatchTask<HX_RESULT>* pTask =  AllocDispatch0(this, &HXThreadedSocket::HandleRead);
		hr = Send(pTask);
	    }

	    if (HXR_OK == hr)
	    {
		if(IsReadable() && !m_inReadEventPostingLoop)
		{ 
		    // we must unwind (return from this function) before notifying response
		    AddPendingEvent(HX_SOCK_EVENT_READ, HXR_OK);
		}
	    }
	    else
	    {
		HX_ASSERT(FALSE);
		m_state = TS_ERROR;
		AddPendingEvent(HX_SOCK_EVENT_ERROR, hr);
	    }
	}
	else 
	{
	    // no inbound data (or possibly error/bad state)...
	    if (m_state != TS_OPEN)
	    {
		if (TS_ENDSTREAM == m_state)
		{
		    // graceful close (remote side sent all data and closed connection)
		    HXLOGL3(HXLOG_NETW, "HXThreadedSocket[%p]::ReadFrom(): returning HXR_SOCK_ENDSTREAM", this);
		    hr = HXR_SOCK_ENDSTREAM;
		}
		else
		{
		    HXLOGL3(HXLOG_NETW, "HXThreadedSocket[%p]::ReadFrom(): returning HXR_FAIL", this);
		    hr = HXR_FAIL;
		}

		m_state = TS_CLOSED;

		// we must unwind (return from this function) before notifying response
		AddPendingEvent(HX_SOCK_EVENT_CLOSE, hr);
	    }
	    else
	    {
		HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::ReadFrom(): buffer empty; returning HXR_SOCK_WOULDBLOCK", this);
		hr = HXR_SOCK_WOULDBLOCK;
	    }
	}
    }

    return hr;
}

STDMETHODIMP
HXThreadedSocket::Write(IHXBuffer* pBuf)
{
    return WriteTo(pBuf, 0);
}

STDMETHODIMP
HXThreadedSocket::WriteTo(IHXBuffer* pBuf, IHXSockAddr* pAddr)
{
    HX_RESULT hr = HXR_FAIL;

    if (m_pOutbound)
    {
	hr = HXR_OK;

	if (!m_pOutbound->IsInitialized())
	{
	    // allocate space for holding pending outbound buffers
	    // until they are are processed by the network thread.
	    const UINT32 OUTBOUND_SLOT_COUNT = 512;
	    hr = m_pOutbound->Init(OUTBOUND_SLOT_COUNT);
	    if (FAILED(hr))
	    {
		return hr;
	    }
	}

	// outbound buffer ensures this function returns quickly
	if (m_pOutbound->IsFull())
	{
	    HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::WriteTo(): outbound buffers full (%lu writes pending)", this, m_pOutbound->GetCount());

	    // special case: wait for net thread to process write buffers so we have space
	    hr = SendWait(AllocDispatch0(this, &HXThreadedSocket::HandleWrite));
	    if (FAILED(hr))
	    {
		return hr;
	    }

	    HX_ASSERT(!m_pOutbound->IsFull());
	}

	m_pOutbound->AddTail(pBuf, pAddr);

	HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::WriteTo(): now %lu writes pending", this, m_pOutbound->GetCount());
	if (m_pOutbound->TryResetEmptySignal())
	{
	    HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::WriteTo(): empty signal detected", this);
	    // we just added the first item; notify net thread to process outbound buffer
	    hr = Send(AllocDispatch0(this, &HXThreadedSocket::HandleWrite));
	}
    }

    return hr;
}


STDMETHODIMP
HXThreadedSocket::WriteV(UINT32 nVecLen, IHXBuffer** ppVec)
{
    HX_ASSERT(FALSE);
    return HXR_NOTIMPL; 
}

STDMETHODIMP
HXThreadedSocket::WriteToV(UINT32 nVecLen, IHXBuffer** ppVec, IHXSockAddr* pAddr)
{
    HX_ASSERT(FALSE);
    return HXR_NOTIMPL; 
}


// [net thread]
STDMETHODIMP
HXThreadedSocket::HandleWrite()
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pOutbound)
    {
	retVal = HXR_OK;

	while (m_pOutbound->GetCount() > 0 && m_state <= TS_ENDSTREAM)
	{
	    // process pending outbound data
	    HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::HandleWrite(): writing next buffer; pending = %lu", this, m_pOutbound->GetCount());

	    IHXBuffer* pBuf = 0;
	    IHXSockAddr* pAddr = 0;
	    m_pOutbound->RemoveHead(pBuf, pAddr);

	    HX_RESULT hr;
	    if (pAddr)
	    {
		hr = m_pSock->WriteTo(pBuf, pAddr);
	    }
	    else
	    {
		hr = m_pSock->Write(pBuf);
	    }

	    HX_RELEASE(pBuf);
	    HX_RELEASE(pAddr);

	    if (FAILED(hr))
	    {
		HX_ASSERT(HXR_SOCK_WOULDBLOCK != hr); // unexpected (should be handled by socket write buffer logic)
		HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::HandleWrite(): write failed; code = %08x", this, hr);
		m_state = TS_ERROR;
		AddPendingEvent(HX_SOCK_EVENT_ERROR, hr);
	    }
	}
    }

    return retVal;
}

// [net thread]
HX_RESULT HXThreadedSocket::HandleRead()
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pInbound)
    {
	HX_RESULT hr = HXR_OK;

	retVal = HXR_OK;

	// ensure inbound data initialized (SetOption must be called by now)
	if (!m_pInbound->IsInitialized())
	{
	    // The size of the read buffers that are returned from the 
	    // socket is unpredictable. Here we pick a small avg packet
	    // size such that we are highly likely to have more than
	    // sufficient slots allocated in the inbound buffer.
	    const UINT32 AVG_READ_BUF_SIZE = 50;

	    hr = m_pInbound->Init(m_cbMaxInbound, AVG_READ_BUF_SIZE);
	    if (FAILED(hr))
	    {
		m_state = TS_ERROR;
		AddPendingEvent(HX_SOCK_EVENT_ERROR, hr);
		return HXR_OK;
	    }
	}

	const UINT32 READ_COUNT = 1000; // arbitrary

	UINT32 count = 0;
	for(count = 0; count < READ_COUNT; ++count)
	{
	    // don't read if not or no longer open
	    if (m_state != TS_OPEN)
	    {
		HXLOGL3(HXLOG_NETW, "HXThreadedSocket[%p]::HandleRead(): ignoring (not TS_OPEN)", this);
		break;
	    }

	    // check buffer limit
	    if (m_pInbound->IsFull())
	    {
		HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::HandleRead(): buffer limit exceeded (%lu > %lu); not reading", this, m_pInbound->GetBytes(), m_cbMaxInbound);
		// possible, but unexpected (we handle this)
		HX_ASSERT(m_pInbound->GetFree() > 0); 

		// stop loop
		break;
	    }

	    // read another buffer
	    HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::HandleRead(): reading next buffer", this);
	    IHXBuffer* pBuf = 0; 
	    IHXSockAddr* pAddr = 0;
	    if (m_pSock->GetType() == HX_SOCK_TYPE_TCP)
	    {
		hr = m_pSock->Read(&pBuf);
	    }
	    else
	    {
		HX_ASSERT(HX_SOCK_TYPE_UDP == m_pSock->GetType());
		hr = m_pSock->ReadFrom(&pBuf, &pAddr);
	    }

	    if (HXR_OK == hr)
	    {
		HX_ASSERT(pBuf);
		HX_ASSERT(pBuf->GetSize() != 0);

		// transfer buffer to inbound data
		hr = m_pInbound->AddTail(pBuf, pAddr);
		if (FAILED(hr))
		{
		    HX_ASSERT(FALSE); // should be impossible

		    // stop loop
		    break;
		}

		if (m_pInbound->TryResetEmptySignal())
		{
		    // empty signal was set and we just added data; notify reader to resume reading
		    HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::HandleRead(): empty signal detected", this);
		    AddPendingEvent(HX_SOCK_EVENT_READ, HXR_OK);
		}

		HX_RELEASE(pBuf);
		HX_RELEASE(pAddr);   

	    }
	    else if (HXR_SOCK_ENDSTREAM == hr)
	    {
		// graceful close
		HXLOGL3(HXLOG_NETW, "HXThreadedSocket[%p]::HandleRead(): read returns HXR_SOCK_ENDSTREAM", this);
		m_state = TS_ENDSTREAM;

		// exepected and non-critical (we handle)
		hr = HXR_OK;

		if (0 == m_pInbound->GetCount())
		{
		    // notify reader to ensure it sees the endstream event
		    HXLOGL3(HXLOG_NETW, "HXThreadedSocket[%p]::HandleRead(): empty signal detected (HXR_SOCK_ENDSTREAM)", this);
		    AddPendingEvent(HX_SOCK_EVENT_READ, HXR_OK);
		}

		// stop loop
		break;
	    }
	    else 
	    {
		if (HXR_SOCK_WOULDBLOCK == hr)
		{ 
		    // nothing to read for the moment
		    HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::HandleRead(): read fails; err = HXR_SOCK_WOULDBLOCK", this);

		    // exepected and non-critical (we handle)
		    hr = HXR_OK;
		}
		else
		{
		    HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::HandleRead(): read fails; err = 0x%08x", this, hr);
		}

		// stop loop
		break;
	    }
	}
	HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::HandleRead(): %lu reads completed; buffer count = %lu (%lu bytes)", this, count, m_pInbound->GetCount(), m_pInbound->GetBytes());

	if (FAILED(hr))
	{
	    // unexpected critical error (we don't/can't handle)
	    HX_ASSERT(hr != HXR_SOCK_WOULDBLOCK);
	    HX_ASSERT(hr != HXR_SOCK_ENDSTREAM);
	    m_state = TS_ERROR;
	    AddPendingEvent(HX_SOCK_EVENT_ERROR, hr);
	}
    }

    return retVal;
}

// [net thread]
STDMETHODIMP
HXThreadedSocket::EventPending(UINT32 uEvent, HX_RESULT status)
{
    HX_RESULT hr = HXR_OK;
    
    switch (uEvent)
    {
    case HX_SOCK_EVENT_READ:
        HandleRead();
        break;
    case HX_SOCK_EVENT_WRITE:
        HandleWrite();
        AddPendingEvent(uEvent, status);
        break;
    case HX_SOCK_EVENT_CLOSE:
        // fall through...
    default:
        AddPendingEvent(uEvent, status);
        break;
    }

    return HXR_OK;
}

// [net/parent threads]
void HXThreadedSocket::ScheduleCallback()
{
    // Scheduling ensures that response notifications: 
    // 
    //   (a) are always done under core mutex
    //   (b) occur on core as well as app thread
    //

    HXScopeLock lock(m_pSchedulerMutex);
    HX_ASSERT(m_pScheduler);
    if (m_pScheduler)
    {
        HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::ScheduleCallback(): handle = %lu", this, m_hCallback);
        if (0 == m_hCallback)
        {
            m_hCallback = m_pScheduler->RelativeEnter(this, 0);
        }
    }
    
}

void HXThreadedSocket::DestroyScheduler()
{
    HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::DestroyScheduler(): handle = %lu", this, m_hCallback);
    HXScopeLock lock(m_pSchedulerMutex);
    if (m_pScheduler)
    {
        if (m_hCallback)
        {
            m_pScheduler->Remove(m_hCallback);
            m_hCallback = 0;
        }
        HX_RELEASE(m_pScheduler);
    }
}

// callback from scheduler
STDMETHODIMP
HXThreadedSocket::Func()
{
    HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::Func()", this);
   
    // locking prevents bad return value assignment in ScheduleCallback()
    m_pSchedulerMutex->Lock();
    m_hCallback = 0;
    m_pSchedulerMutex->Unlock();

    while (m_events.GetCount() > 0)
    {
        UINT32 event;
        HX_RESULT status;
        m_events.RemoveHead(event, status);
        // read events use special case handling
        HX_ASSERT(event != HX_SOCK_EVENT_READ);
        // other code should prevent this (error event only used internally)
        HX_ASSERT(event != HX_SOCK_EVENT_ERROR);
        HX_ASSERT(m_pResponse);
        m_pResponse->EventPending(event, status);
        if (event == HX_SOCK_EVENT_CLOSE)
        {
            // ensure we close after close event posted (no more events)
            Close();
            return HXR_OK;
        }
    }
    
    // special-case handling for read events
    EnsureReadEventPosted();
 
    return HXR_OK;
}


// ensure that response is posted HX_SOCK_EVENT_READ if needed
void HXThreadedSocket::EnsureReadEventPosted()
{
    // do nothing if:
    //  (a) read events not selected;
    //  (b) we are called recursively from loop below
    //  (c) we are in closed/error state
    if (!(m_selectedEvents & HX_SOCK_EVENT_READ) || m_inReadEventPostingLoop || m_state > TS_ENDSTREAM)
    {
        HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::EnsureReadEventPosted(): ignoring (selected events = 0x%08x)", this, m_selectedEvents);
        return;
    }
   
    /*

    Typically the response will immediately read in response
    to HX_SOCK_EVENT_READ. We should avoid calling EventPending()
    in response to Read() because each Read() must complete and
    return the buffer before the next EventPending() is sent. Also,
    recursion problems would occur. Here we do this:

    1)HX_SOCK_EVENT_READ msg in parent thread msg queue ->
    2)  Call to this function (assuming inbound data ready)
    2)       post HX_SOCK_EVENT_READ to response ->
    3)           response gets EventPending() ->
    4)                response calls Read() ->
    5)       post HX_SOCK_EVENT_READ to response ->
    6)           response gets EventPending() ->
    7)                response calls Read() ->
    8)       post HX_SOCK_EVENT_READ to response, etc.

    */

    m_inReadEventPostingLoop = TRUE;

    // this determines how much processing we do per scheduler callback
    UINT32 iterCount = 1;

    // loop in case Read() is called in response to read event...
    while ( (m_selectedEvents & HX_SOCK_EVENT_READ) && iterCount--)
    {
        if (!IsReadable())
        {
            HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::EnsureReadEventPosted(): not readable; breaking", this);
            break;
        }

        // read event will be re-selected once user calls a read method (or possibly SelectEvents)
        HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::EnsureReadEventPosted(): HX_SOCK_EVENT_READ to response", this);
        m_selectedEvents &= ~HX_SOCK_EVENT_READ;
        HX_ASSERT(m_pResponse);
        m_pResponse->EventPending(HX_SOCK_EVENT_READ, HXR_OK);
    }

    // yield; ensure an immediate callback is scheduled if we still have data
    if (IsReadable() && m_selectedEvents & HX_SOCK_EVENT_READ)
    {
        ScheduleCallback();
    }
    
    m_inReadEventPostingLoop = FALSE;
}


// Queue up an event for handling under scheduler callback. All events
// are dispatched on app/core thread under scheduler callback so response
// is guaranteed safe call semantics.
//
// [parent or net thread]
void
HXThreadedSocket::AddPendingEvent(UINT32 event, HX_RESULT status)
{        
    HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::AddPendingEvent(%lu; status = %08x); response = %p", this, event, status, m_pResponse);
    HX_ASSERT(event != 0);

     // translate error events; HX_SOCK_EVENT_ERROR is only used internally (and may go away)
    if (HX_SOCK_EVENT_ERROR == event)
    {
        event = HX_SOCK_EVENT_CLOSE;

        // just in case
        HX_ASSERT(FAILED(status));
        if (SUCCEEDED(status))
        {
            status = HXR_FAIL;
        }
    }

    // don't add read events; read events are handled separately
    if (HX_SOCK_EVENT_READ != event)
    {
        if (HX_SOCK_EVENT_CLOSE == event)
        {
            if(SUCCEEDED(status))
            {
                // swallow close events that have no error; normal-case
                // close events are generated in ReadFrom after all data 
                // is read in in order to conform with Helix close event 
                // semantics
                HXLOGL4(HXLOG_NETW, "HXThreadedSocket[%p]::AddPendingEvent(): swallowing event", this);
                return;
            }
            else
            {
                // severe error; abandon notification for other events
                m_events.Clear();
            }
        }

        m_events.AddTail(event, status);
    } 

    // ensure immediate callback scheduled
    ScheduleCallback();
}

STDMETHODIMP
HXThreadedSocket::Close()
{
    HXLOGL3(HXLOG_NETW, "HXThreadedSocket[%p]::Close()", this);
    
    HX_RESULT hr = HXR_OK;
    if (m_pDriver)
    {
        // wait for actual socket to close on network thread
        HX_ASSERT(m_pSock);
        hr = SendWait( AllocDispatch0(m_pSock, &IHXSocket::Close) );
    }

    if (m_pInbound)
    {
	m_pInbound->CleanUp();
    }

    if (m_pOutbound)
    {
	m_pOutbound->CleanUp();
    }

    HX_DELETE(m_pInbound);
    HX_DELETE(m_pOutbound);

    m_events.Clear();
    DestroyScheduler();

    HX_RELEASE(m_pSchedulerMutex);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pSock);
#ifdef HELIX_FEATURE_SECURE_SOCKET
    HX_RELEASE(m_pSecureSock);
#endif
    HX_RELEASE(m_pDriver);
    HX_RELEASE(m_pResponse);
    m_state = TS_CLOSED;
    m_selectedEvents = 0;

    return hr;
}


STDMETHODIMP
HXThreadedSocket::Listen(UINT32 uBackLog)
{
    return SendWait( AllocDispatch1(m_pSock, &IHXSocket::Listen, uBackLog) ); 
}


STDMETHODIMP
HXThreadedSocket::Accept(IHXSocket** ppNewSock, IHXSockAddr** ppSource)
{
    HX_ASSERT(ppNewSock);
    HX_ASSERT(ppSource);

    // get connected socket
    IHXSocket* pConnectedSock = 0;
    HX_RESULT hr = SendWait( AllocDispatch2(m_pSock, &IHXSocket::Accept, &pConnectedSock, ppSource) ); 
    if (SUCCEEDED(hr))
    {
        // Accept() returns a plain socket (not an HXThreadedSocket); now wrap the actual socket
        hr = HXThreadedSocket::Create(m_pContext, pConnectedSock, *ppNewSock);
        HX_RELEASE(pConnectedSock);
    }
    return hr;
}


STDMETHODIMP
HXThreadedSocket::GetOption(HXSockOpt name, UINT32* pval)
{
    if (name == HX_SOCKOPT_APP_RCVBUF)
    {
        return m_cbMaxInbound;
    }
    return SendWait( AllocDispatch2(m_pSock, &IHXSocket::GetOption, name, pval) ); 
}

STDMETHODIMP
HXThreadedSocket::SetOption(HXSockOpt name, UINT32 val)
{
    // HXThreadedSocket-specific socket options
    HX_RESULT retVal = HXR_FAIL;
    
    if (m_pInbound)
    {
	retVal = HXR_OK;

	if (name == HX_SOCKOPT_APP_RCVBUF)
	{
	    // HX_SOCKOPT_APP_RCVBUF only makes sense for HXThreadedSocket.
	    m_cbMaxInbound = val;

	    // SetOption must be called before we init the inbound data buffer
	    HX_ASSERT(!m_pInbound->IsInitialized());
	}
	else
	{
	    retVal = SendWait( AllocDispatch2(m_pSock, &IHXSocket::SetOption, name, val) );
	}
    }

    return retVal;
}

STDMETHODIMP
HXThreadedSocket::ReadV(UINT32 nVecLen, UINT32* puLenVec,
                                           IHXBuffer** ppBufVec)
{
    HX_ASSERT(FALSE);
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXThreadedSocket::ReadFromV(UINT32 nVecLen, UINT32* puLenVec,
                                           IHXBuffer** ppBufVec,
                                           IHXSockAddr** ppAddr)
{
    HX_ASSERT(FALSE);
    return HXR_NOTIMPL;
}


STDMETHODIMP
HXThreadedSocket::JoinGroup(IHXSockAddr* pGroupAddr, 
                              IHXSockAddr* pInterface)
{
    HXLOGL3(HXLOG_NETW, "HXThreadedSocket[%p]::JoinGroup", this);
    IHXMulticastSocket* pMulti = 0;
    HX_RESULT hr = m_pSock->QueryInterface(IID_IHXMulticastSocket, reinterpret_cast<void**>(&pMulti));
    if (SUCCEEDED(hr))
    {
        hr = SendWait( AllocDispatch2(pMulti, &IHXMulticastSocket::JoinGroup, pGroupAddr, pInterface) );
        HX_RELEASE(pMulti);
    }

    return hr;
}


STDMETHODIMP
HXThreadedSocket::LeaveGroup(IHXSockAddr* pGroupAddr, 
                               IHXSockAddr* pInterface)
{
    HXLOGL3(HXLOG_NETW, "HXThreadedSocket[%p]::LeaveGroup", this);
  
    IHXMulticastSocket* pMulti = 0;
    HX_RESULT hr = m_pSock->QueryInterface(IID_IHXMulticastSocket, reinterpret_cast<void**>(&pMulti));
    if (SUCCEEDED(hr))
    {
        hr = SendWait( AllocDispatch2(pMulti, &IHXMulticastSocket::LeaveGroup, pGroupAddr, pInterface) );
        HX_RELEASE(pMulti);
    }

    return hr;
}

STDMETHODIMP
HXThreadedSocket::SetSourceOption(HXMulticastSourceOption flag,
                                           IHXSockAddr* pSourceAddr,
                                           IHXSockAddr* pGroupAddr,
                                           IHXSockAddr* pInterface)
{
    
    IHXMulticastSocket* pMulti = 0;
    HX_RESULT hr = m_pSock->QueryInterface(IID_IHXMulticastSocket, reinterpret_cast<void**>(&pMulti));
    if (SUCCEEDED(hr))
    {
        hr = SendWait( AllocDispatch4(pMulti, &IHXMulticastSocket::SetSourceOption, flag, pSourceAddr, pGroupAddr, pInterface) );
        HX_RELEASE(pMulti);
    }
    return hr;

}

STDMETHODIMP
HXThreadedSocket::SetAccessControl(IHXSocketAccessControl* pControl)
{
    return SendWait( AllocDispatch1(m_pSock, &IHXSocket::SetAccessControl, pControl) ); 
}

#ifdef HELIX_FEATURE_SECURE_SOCKET
STDMETHODIMP 
HXThreadedSocket::SetClientCertificate(IHXList* pCertChain, IHXBuffer* pPrvKey)
{
   return SendWait( AllocDispatch2(m_pSecureSock, &IHXSecureSocket::SetClientCertificate, pCertChain, pPrvKey));
}

STDMETHODIMP
HXThreadedSocket::InitSSL(IHXCertificateManager* pContext)
{
    return SendWait( AllocDispatch1(m_pSecureSock, &IHXSecureSocket::InitSSL, pContext));
}

STDMETHODIMP
HXThreadedSocket::SetSessionID(IHXBuffer* pBuff)
{
    return SendWait( AllocDispatch1(m_pSecureSock, &IHXSecureSocket::SetSessionID, pBuff));
}

STDMETHODIMP
HXThreadedSocket::GetSessionID(IHXBuffer** pBuff)
{
    return SendWait( AllocDispatch1(m_pSecureSock, &IHXSecureSocket::GetSessionID, pBuff));
}
#endif

