/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: thrdconn.cpp,v 1.22 2007/07/06 20:43:53 jfinnecy Exp $
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

#include "hlxclib/time.h"
#include <stdio.h>
#include <string.h>
#include "hxcom.h"
#include "conn.h"

#if defined (_WIN32) || defined (WIN32)
#include "platform/win/win_net.h"
#include "platform/win/casynthr.h"
#endif

#if defined(_UNIX) && (defined( _UNIX_THREADED_NETWORK_IO ) || defined(THREADS_SUPPORTED))
#include "platform/unix/UnixThreads.h"
#endif /* _UNIX_THREADED_NETWORK_IO */

#ifdef _CARBON
#include "carbthrd.h"
#endif


#include "hxslist.h"
#include "growingq.h"

#include "hxengin.h"
#include "ihxpckts.h"
#include "hxbuffer.h"
#include "timebuff.h"
#include "hxtick.h"
#include "hxthread.h"
#include "threngin.h"
#include "conn.h"
#include "thrdconn.h"
#include "pckunpck.h"
#define HELIX_FEATURE_LOGLEVEL_NONE
#include "hxtlogutil.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define QUEUE_START_SIZE    512

ThreadedConn*
ThreadedConn::new_socket(IUnknown* pContext, UINT16 type)
{
    return new ThreadedConn(pContext, type);
}

#ifdef THREADS_SUPPORTED
#ifdef HELIX_FEATURE_ADD_NETWORK_THREAD_SLEEP
#define DEFAULT_NETWORK_THREAD_SLEEP	50
#endif //HELIX_FEATURE_ADD_NETWORK_THREAD_SLEEP
#endif //THREADS_SUPPORTED

/*
HXThread*
ThreadedConn::GetNetworkThread(void)
{
#if defined (_WIN32)
    return win_net::GetNetworkThread();
#else
    HX_ASSERT(FALSE);
    return HXR_UNEXXPECTED;
#endif
}

void
ThreadedConn::DestroyNetworkThread(void)
{
#if defined (_WIN32)
    win_net::DestroyNetworkThread();
#else
    HX_ASSERT(FALSE);
    return P NR_UNEXXPECTED;
#endif
}
*/

ThreadedConn::ThreadedConn(IUnknown* pContext, UINT16 type)
    : conn(pContext)
    , m_lRefCount(0)
    , m_pActualConn(NULL)
    , m_uSocketType(type)
    , m_pNetworkThread(NULL)
    , m_pMainAppThread(NULL)
    , m_pMutex(NULL)
    , m_pNetCallback(0)
    , m_pSendTCP (0)
    , m_pReceiveTCP (0)
    , m_pTempBuffer (0)
    , m_ulUserHandle(0)
    , m_pInternalWindowHandle(NULL)
    , m_bConnected(FALSE)
    , m_bIsDone(FALSE)
    , m_bDetachPending(TRUE)
    , m_pInitEvent(0)
    , m_pQuitEvent(0)
    , m_pListenEvent(NULL)
    , m_pDetachEvent(0)
    , m_bInitialized(FALSE)
    , m_bOutstandingReadNotification(FALSE)
    , m_bOutstandingWriteNotification(FALSE)
    , m_bWriteFlushPending(FALSE)
    , m_bNetworkIOPending(FALSE)
    , m_bReadNowPending(FALSE)
    , m_bReadPostPendingWouldBlock(FALSE)
    , m_pIncommingConnections(NULL)
    , m_bIgnoreWSAECONNRESET(FALSE)
#if defined (_WIN32) || defined (WIN32)
    , m_pNotifier(NULL)
#endif
#ifdef THREADS_SUPPORTED
#ifdef HELIX_FEATURE_ADD_NETWORK_THREAD_SLEEP
    , m_ulNetworkThreadSleep(DEFAULT_NETWORK_THREAD_SLEEP)
#endif //HELIX_FEATURE_ADD_NETWORK_THREAD_SLEEP
#endif //THREADS_SUPPORTED
{
    HXLOGL4(HXLOG_NETW, "CON ThreadedConn this=%p", this);
    
    m_pActualConn = conn::actual_new_socket(m_pContext, type);
    m_pActualConn->AddRef();
    conn::add_connection_to_list (m_pContext, m_pActualConn);

    ThreadEngine* pEngine = ThreadEngine::GetThreadEngine(m_pContext);
    // NOTE: pEngine allocation is not checked for success.
    pEngine->AttachSocket(this);
    m_pNetworkThread    = pEngine->GetNetworkThread();
    m_pMainAppThread    = pEngine->GetMainAppThread();
    m_pNetCallback      = new ThrConnSocketCallback(this);

#if !defined(THREADS_SUPPORTED) && defined(_UNIX_THREADED_NETWORK_IO)
    // THREAD_SUPPORTED should be defined whenever _UNIX_THREADED_NETWORK_IO
    // is defined, please check the config file of the platform,
    // Otherwise, the mutex created below would be a stub mutex.
    HX_ASSERT(FALSE);
#endif

    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);
    
    CreateEventCCF((void**)&m_pInitEvent, m_pContext, NULL, FALSE);
    CreateEventCCF((void**)&m_pQuitEvent, m_pContext, NULL, TRUE);
    CreateEventCCF((void**)&m_pDetachEvent, m_pContext, NULL, TRUE);
    CreateEventCCF((void**)&m_pListenEvent, m_pContext, NULL, FALSE);

    m_pTempBuffer = new char[TCP_BUF_SIZE];

    /* Allocate byte queues ONLY if it is a TCP socket */
    if (m_uSocketType == HX_TCP_SOCKET)
    {
        // allocate TCP send and receive queue
        m_pSendTCP = new CByteGrowingQueue(QUEUE_START_SIZE,1);
        if (!m_pSendTCP || !m_pSendTCP->IsQueueValid())
        {
            mLastError = HXR_OUTOFMEMORY;
        }
        m_pSendTCP->SetMaxSize(TCP_BUF_SIZE);

        m_pReceiveTCP = new CByteGrowingQueue(QUEUE_START_SIZE,1);
        if (!m_pReceiveTCP || !m_pReceiveTCP->IsQueueValid())
        {
            mLastError = HXR_OUTOFMEMORY;
        }
        m_pReceiveTCP->SetMaxSize(TCP_BUF_SIZE);
    }
}

ThreadedConn::~ThreadedConn()
{
    HXLOGL4(HXLOG_NETW, "DES ThreadedConn this=%p", this);

    if (m_pNetCallback)
    {
        m_pNetCallback->m_pContext = 0;
    }

    if (m_pActualConn)
    {
        m_pActualConn->done();
        m_pActualConn->Release();
        m_pActualConn = 0;
    }

    HX_DELETE(m_pNetCallback);
    HX_VECTOR_DELETE(m_pTempBuffer);
    HX_DELETE(m_pSendTCP);
    HX_DELETE(m_pReceiveTCP);

    while (m_WriteUDPBuffers.GetCount() > 0)
    {
        UDPPacketInfo* pPacket = (UDPPacketInfo*) m_WriteUDPBuffers.RemoveHead();
        pPacket->m_pBuffer->Release();
        delete pPacket;
    }

    while (m_ReadUDPBuffers.GetCount() > 0)
    {
        UDP_PACKET* pPacket = (UDP_PACKET*) m_ReadUDPBuffers.RemoveHead();
        HX_RELEASE(pPacket->pBuffer);
        HX_DELETE(pPacket);
    }

    HX_RELEASE(m_pMutex);

    HX_RELEASE(m_pInitEvent);
    HX_RELEASE(m_pQuitEvent);
    HX_RELEASE(m_pDetachEvent);
    HX_RELEASE(m_pListenEvent);
    HX_DELETE(m_pIncommingConnections);

    mCallBack = NULL;
#ifdef _UNIX_THREADED_NETWORK_IO
    if( m_bNetworkThreading )
    {
        //Remove any messages from the main app thread for us.
        HX_RESULT res = HXR_OK;
        HX_ASSERT( m_pMainAppThread );
        HXThreadMessage msgBack;
        HXThreadMessage msgMatch(0, (void*)this, NULL, NULL);
        while( HXR_OK == res )
        {
            res = m_pMainAppThread->PeekMessageMatching(&msgBack, &msgMatch, TRUE );
        }
    }
#elif defined(_CARBON) && defined(THREADS_SUPPORTED)
    // remove any messages from the main app thread for us.
    HX_ASSERT(m_pMainAppThread);
    HX_RESULT res = HXR_OK;
    HXThreadMessage msgBack;
    HXThreadMessage msgMatch(0, (void*)this, NULL, NULL);
    while( HXR_OK == res )
    {
        res = m_pMainAppThread->PeekMessageMatching(&msgBack, &msgMatch, TRUE );
    }
#endif
    HX_RELEASE(m_pNetworkThread);
    HX_RELEASE(m_pMainAppThread);
}

ULONG32 ThreadedConn::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG32 ThreadedConn::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT
ThreadedConn::dns_find_ip_addr(const char* host, UINT16 blocking)
{
    ThrdConnGenericCallback* pCallback  = new ThrdConnGenericCallback(this, DNS_CALLBACK_TYPE);
    pCallback->m_HostName   = host;
    pCallback->m_bBlocking  = (HXBOOL) blocking;
    /* Will be released by the thread engine */
    pCallback->AddRef();
    HXThreadMessage msg(HXMSG_ASYNC_CALLBACK, this, pCallback);
    return m_pNetworkThread->PostMessage(&msg, NULL);
}


HX_RESULT
ThreadedConn::ActualDnsFindIpAddr(const char* host, UINT16 blocking)
{
    HX_RESULT theErr = HXR_UNEXPECTED;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        theErr = m_pActualConn->dns_find_ip_addr(host, blocking);
        m_pMutex->Unlock();
    }

    return theErr;
}

HXBOOL
ThreadedConn::dns_ip_addr_found(HXBOOL* valid, ULONG32* addr)
{

    HXBOOL bResult = FALSE;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        bResult = m_pActualConn->dns_ip_addr_found(valid, addr);
        m_pMutex->Unlock();
    }

    return bResult;
}

void
ThreadedConn::finaldone (void)
{
    if (!m_bIsDone)
    {
        done();
    }

    /* final attempt to cleanup */
    PostDoneAndDetach();

    if (m_pQuitEvent)
    {
        m_pQuitEvent->Wait(ALLFS);
    }

    if (m_pDetachEvent)
    {
        m_pDetachEvent->Wait(ALLFS);
    }
}

void
ThreadedConn::Detached (void)
{
    if (m_pDetachEvent)
    {
        m_pDetachEvent->SignalEvent();
    }
}

void
ThreadedConn::done (void)
{
    /* Do not pass any more callbacks to the client above */
    mCallBack   = NULL;
    m_bIsDone   = TRUE;
    /* Actual message to release the socket will be posted in DoWrite() */

#if (defined (_WIN32) || defined (WIN32)) && !defined(WIN32_PLATFORM_PSPC)
    if (!m_pNotifier)
    {
        m_pNotifier =
            CAsyncNetThread::GetCAsyncNetThreadNotifier(m_pContext,
							(HINSTANCE)m_ulUserHandle,
                                                        FALSE);
    }
    if (m_pNotifier)
    {
        m_pNotifier->DetachSocket(this);
        m_pNotifier = NULL;
    }
#endif /*defined (_WINDOWS) || defined (_WIN32)*/

    if (!m_bConnected)
    {
        PostDoneAndDetach();
    }
}

void
ThreadedConn::PostDoneAndDetach()
{
    // If we are out of memory, let's just get out of here. Ideally, we should
    // not ever get to this point, but lots of functions here have void return
    // types, so it is possible.
    if( mLastError == HXR_OUTOFMEMORY )
    {
        return;
    }

    m_pMutex->Lock();

    if (m_bDetachPending)
    {
        m_bDetachPending = FALSE;
        ThrdConnGenericCallback* pCallback      = new ThrdConnGenericCallback(this, DONE_CALLBACK_TYPE);
        /* Will be released by the thread engine */
        pCallback->AddRef();
        HXThreadMessage msg(HXMSG_ASYNC_CALLBACK, this, pCallback);
        m_pNetworkThread->PostMessage(&msg, NULL);
        HXThreadMessage msg1(HXMSG_ASYNC_DETACH, this, NULL);
        m_pNetworkThread->PostMessage(&msg1, NULL);
    }

    m_pMutex->Unlock();
}

void
ThreadedConn::ActualDone (void)
{
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        m_bConnected = FALSE;
        m_pActualConn->done();
        m_pActualConn->Release();
        m_pActualConn = 0;
        m_pMutex->Unlock();
    }

    if (m_pQuitEvent)
    {
        m_pQuitEvent->SignalEvent();
    }
}

HX_RESULT
ThreadedConn::init  (UINT32 local_addr,UINT16 port, UINT16 blocking)
{
    ThrdConnGenericCallback* pCallback  = new ThrdConnGenericCallback(this, INIT_CALLBACK_TYPE);
    pCallback->m_ulLocalAddr    = local_addr;
    pCallback->m_uPort          = port;
    pCallback->m_bBlocking      = (HXBOOL) blocking;
    /* Will be released by the thread engine */
    pCallback->AddRef();

    HXThreadMessage msg(HXMSG_ASYNC_CALLBACK, this, pCallback);
    m_pNetworkThread->PostMessage(&msg, NULL);

    /* Wait for the actual Initialization to complete. This is the only
       function we wait for the networking thread to complete before passing
       the result to the user since there is no async interface to pass the result
       back and it very much possible that socket binding may fail during
       initialization
     */
    m_pInitEvent->Wait(ALLFS);

    if (m_bInitialized)
    {
        return HXR_OK;
    }
    else
    {
        return HXR_FAIL;
    }
}

HX_RESULT
ThreadedConn::ActualInit(UINT32 local_addr,UINT16 port, UINT16 blocking)
{
    HX_RESULT theErr = HXR_UNEXPECTED;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        theErr = m_pActualConn->init(local_addr, port, blocking);
        if (!theErr && m_uSocketType == HX_UDP_SOCKET)
        {
            m_bConnected = TRUE;
        }

        if (!theErr)
        {
            m_bInitialized = TRUE;
        }

        /* Signal the main thread that intialization is complete */
        m_pInitEvent->SignalEvent();
        m_pMutex->Unlock();
    }

    return theErr;
}

HX_RESULT
ThreadedConn::listen(ULONG32 ulAddr,  UINT16 port, UINT16 backlog,
                     UINT16 blocking, ULONG32 ulPlatform)
{
    HX_RESULT theErr = HXR_OK;

#if (defined (_WIN32) || defined (WIN32)) && !defined(WIN32_PLATFORM_PSPC)
    m_ulUserHandle  = ulPlatform;

    if (!m_pNotifier)
    {
        m_pNotifier =
            CAsyncNetThread::GetCAsyncNetThreadNotifier(m_pContext,
						        (HINSTANCE)ulPlatform,
                                                        TRUE);
    }
    if (m_pNotifier)
    {
        m_pInternalWindowHandle = (void*) m_pNotifier->GetWindowHandle();
        m_pNotifier->AttachSocket(this);
    }
    else
    {
        theErr = HXR_OUTOFMEMORY;
    }
#endif /*defined (_WIN32) || defined (WIN32)*/

    HX_DELETE(m_pIncommingConnections);
    m_pIncommingConnections = new CHXSimpleList();

    if (!theErr)
    {
        ThrdConnGenericCallback* pCallback      =
                new ThrdConnGenericCallback(this, LISTEN_CALLBACK_TYPE);
        pCallback->m_ulLocalAddr = ulAddr;
        pCallback->m_uPort      = port;
        pCallback->m_uBacklog   = backlog;
        pCallback->m_bBlocking  = (HXBOOL) blocking;
        pCallback->m_ulHandle   = ulPlatform;
        /* Will be released by the thread engine */
        pCallback->AddRef();
        HXThreadMessage msg(HXMSG_ASYNC_CALLBACK, this, pCallback);
        theErr = m_pNetworkThread->PostMessage(&msg, NULL);

        if ( SUCCEEDED(theErr) )
        {
            /*
             * Wait for the actual Listen to complete.
             */
	    // listen is called from the network thread, so this will wait forever
#ifdef HELIX_FEATURE_NETWORK_USE_SELECT
	    m_bListenning = TRUE;
#else
            m_pListenEvent->Wait(ALLFS);
#endif

            if ( m_bListenning )
            {
                theErr = HXR_OK;
            }
            else
            {
                theErr = HXR_FAIL;
            }
        }
    }
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
    set_callback(m_pNetCallback); // for accept msg
#endif //HELIX_FEATURE_NETWORK_USE_SELECT
    return theErr;
}

HX_RESULT
ThreadedConn::ActualListen( ULONG32     ulAddr,
                            UINT16      port,
                            UINT16      backlog,
                            UINT16      blocking,
                            ULONG32     ulPlatform)
{
    HX_RESULT err = HXR_UNEXPECTED;
    HX_ASSERT(m_pActualConn);
    if ( m_pActualConn )
    {
        m_pMutex->Lock();
        err = m_pActualConn->listen(ulAddr, port, backlog, blocking, ulPlatform);

        if ( SUCCEEDED(err) )
        {
            m_bListenning = TRUE;
        }
        else
        {
            m_bListenning = FALSE;
        }

        m_pListenEvent->SignalEvent();
        m_pMutex->Unlock();
    }
    return err;
}

#if     defined (_WINDOWS) || defined (_WIN32)
// we need it for dns_find_ip_addr since async stuff needs a window handle...
HX_RESULT
ThreadedConn::SetWindowHandle(ULONG32 handle)
{
    HX_RESULT theErr = HXR_OK;

    m_pMutex->Lock();
    m_ulUserHandle  = handle;
#if (defined (_WIN32) || defined (WIN32)) && !defined(WIN32_PLATFORM_PSPC)
    if (!m_pNotifier)
    {
        m_pNotifier =
            CAsyncNetThread::GetCAsyncNetThreadNotifier(m_pContext,
							(HINSTANCE)handle,
                                                        TRUE);
    }
    if (m_pNotifier)
    {
        m_pInternalWindowHandle = (void*)m_pNotifier->GetWindowHandle();
        m_pNotifier->AttachSocket(this);
    }
    else
    {
        theErr = HXR_OUTOFMEMORY;
    }
#endif /*defined (_WIN32) || defined (WIN32)*/

    if (!theErr)
    {
        ThrdConnGenericCallback* pCallback      =
                new ThrdConnGenericCallback(this, SETWINDOWHANDLE_CALLBACK_TYPE);
        pCallback->m_ulHandle   = handle;
        /* Will be released by the thread engine */
        pCallback->AddRef();
        HXThreadMessage msg(HXMSG_ASYNC_CALLBACK, this, pCallback);
        theErr = m_pNetworkThread->PostMessage(&msg, NULL);
    }

    m_pMutex->Unlock();

    return theErr;
}


HX_RESULT
ThreadedConn::ActuallSetWindowHandle(ULONG32 handle)
{
    HX_RESULT theErr = HXR_UNEXPECTED;

    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        theErr = m_pActualConn->SetWindowHandle(handle);
        if (!theErr)
        {
        }
        m_pMutex->Unlock();
    }

    return theErr;
}

#endif /* defined (_WINDOWS) || defined (_WIN32)*/

HX_RESULT
ThreadedConn::connect(const char*   host,
                     UINT16         port,
                     UINT16         blocking,
                     ULONG32        ulPlatform)
{
    HX_RESULT theErr = HXR_OK;

#if (defined (_WIN32) || defined (WIN32)) && !defined(WIN32_PLATFORM_PSPC)
    m_ulUserHandle  = ulPlatform;

    if (!m_pNotifier)
    {
        m_pNotifier =
            CAsyncNetThread::GetCAsyncNetThreadNotifier(m_pContext,
							(HINSTANCE)ulPlatform,
                                                        TRUE);
    }
    if (m_pNotifier)
    {
        m_pInternalWindowHandle = (void*) m_pNotifier->GetWindowHandle();
        m_pNotifier->AttachSocket(this);
    }
    else
    {
        theErr = HXR_OUTOFMEMORY;
    }
#endif /*defined (_WIN32) || defined (WIN32)*/

    if (!theErr)
    {
        ThrdConnGenericCallback* pCallback      =
                new ThrdConnGenericCallback(this, CONNECT_CALLBACK_TYPE);
        pCallback->m_HostName   = host;
        pCallback->m_uPort              = port;
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
	blocking = 1; // we don't get fd_connect when doing loopback
#endif //defined(HELIX_FEATURE_NETWORK_USE_SELECT)
        pCallback->m_bBlocking  = (HXBOOL) blocking;
        pCallback->m_ulHandle   = ulPlatform;
        /* Will be released by the thread engine */
        pCallback->AddRef();
        HXThreadMessage msg(HXMSG_ASYNC_CALLBACK, this, pCallback);
        theErr = m_pNetworkThread->PostMessage(&msg, NULL);
    }

    return theErr;
}

HX_RESULT
ThreadedConn::ActualConnect(const char* host,
                            UINT16      port,
                            UINT16      blocking,
                            ULONG32     ulPlatform)
{
    HX_RESULT theErr = HXR_UNEXPECTED;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        theErr = m_pActualConn->connect(host, port, blocking, ulPlatform);
        m_pMutex->Unlock();
    }

    return theErr;
}

#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
HX_RESULT
ThreadedConn::accept (ULONG32 ulAddr)
{
    HX_RESULT theErr = HXR_OK;

#if (defined (_WIN32) || defined (WIN32)) && !defined(WIN32_PLATFORM_PSPC)
    m_ulUserHandle  = NULL; //ulPlatform;

    if (!m_pNotifier)
    {
        m_pNotifier =
            CAsyncNetThread::GetCAsyncNetThreadNotifier((HINSTANCE)m_ulUserHandle,
                                                        TRUE);
    }
    if (m_pNotifier)
    {
        m_pInternalWindowHandle = (void*) m_pNotifier->GetWindowHandle();
        m_pNotifier->AttachSocket(this);
    }
    else
    {
        theErr = HXR_OUTOFMEMORY;
    }
#endif /*defined (_WIN32) || defined (WIN32)*/

    if (!theErr)
    {
        ThrdConnGenericCallback* pCallback      =
                new ThrdConnGenericCallback(this, ACCEPT_CALLBACK_TYPE);
        pCallback->m_ulHandle   = m_ulUserHandle;
	pCallback->m_ulLocalAddr = ulAddr;
        /* Will be released by the thread engine */
        pCallback->AddRef();
        HXThreadMessage msg(HXMSG_ASYNC_CALLBACK, this, pCallback);
        theErr = m_pNetworkThread->PostMessage(&msg, NULL);
    }

    return theErr;
}

HX_RESULT
ThreadedConn::ActualAccept(ULONG32 ulAddr,
                            ULONG32     ulPlatform)
{
    HX_RESULT theErr = HXR_UNEXPECTED;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        theErr = m_pActualConn->CheckForConnection();
        m_pMutex->Unlock();
	if (theErr == HXR_WOULD_BLOCK)
	{
	    ThrdConnGenericCallback* pCallback      =
		    new ThrdConnGenericCallback(this, ACCEPT_CALLBACK_TYPE);
	    pCallback->m_ulHandle   = ulPlatform;
	    pCallback->m_ulLocalAddr = ulAddr;
	    /* Will be released by the thread engine */
	    pCallback->AddRef();
	    HXThreadMessage msg(HXMSG_ASYNC_CALLBACK, this, pCallback);
	    theErr = m_pNetworkThread->PostMessage(&msg, NULL);
	}
    }

    return theErr;
}
#endif //defined(HELIX_FEATURE_NETWORK_USE_SELECT)

HX_RESULT
ThreadedConn::blocking(void)
{
    ThrdConnGenericCallback* pCallback  = new ThrdConnGenericCallback(this, BLOCKING_CALLBACK_TYPE);
    /* Will be released by the thread engine */
    pCallback->AddRef();
    HXThreadMessage msg(HXMSG_ASYNC_CALLBACK, this, pCallback);
    return m_pNetworkThread->PostMessage(&msg, NULL);
}

HX_RESULT
ThreadedConn::ActualBlocking(void)
{
    HX_RESULT theErr = HXR_UNEXPECTED;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        theErr = m_pActualConn->blocking();
        m_pMutex->Unlock();
    }

    return theErr;
}

HX_RESULT
ThreadedConn::nonblocking(void)
{
    ThrdConnGenericCallback* pCallback  = new ThrdConnGenericCallback(this, NONBLOCKING_CALLBACK_TYPE);
    /* Will be released by the thread engine */
    pCallback->AddRef();
    HXThreadMessage msg(HXMSG_ASYNC_CALLBACK, this, pCallback);
    return m_pNetworkThread->PostMessage(&msg, NULL);
}

HX_RESULT
ThreadedConn::ActualNonBlocking(void)
{
    HX_RESULT theErr = HXR_UNEXPECTED;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        theErr = m_pActualConn->nonblocking();
        m_pMutex->Unlock();
    }

    return theErr;
}


HX_RESULT
ThreadedConn::read (void* buf, UINT16* size)
{
    HX_RESULT theErr = HXR_OK;
    UINT16 uCount = 0;

    m_pMutex->Lock();
    m_bOutstandingReadNotification = FALSE;

    if (m_uSocketType != HX_TCP_SOCKET)
    {
        theErr = HXR_NET_SOCKET_INVALID;
        goto cleanup;
    }

    uCount = m_pReceiveTCP->GetQueuedItemCount();
    if (uCount > 0)
    {
        uCount = (uCount <= *size ? uCount : *size);
        m_pReceiveTCP->DeQueue(buf, uCount);
        *size = uCount;
    }
    else
    {
        *size = 0;
        if (!mLastError && m_pActualConn)
        {
            theErr = HXR_WOULD_BLOCK;
        }
        else
        {
            theErr = mLastError;
        }
    }

cleanup:

    if (!mLastError && !m_bNetworkIOPending)
    {
        theErr = PostIOMessage();
    }

    m_pMutex->Unlock();

    return theErr;
}

HX_RESULT
ThreadedConn::readfrom (REF(IHXBuffer*)    pBuffer,
                        REF(UINT32)         ulAddress,
                        REF(UINT16)         ulPort)
{
    HXLOGL4(HXLOG_NETW, "%p::ThreadedConn::readfrom()", this);
    
    HX_RESULT theErr = HXR_OK;
    UDP_PACKET* pPacket = NULL;

    m_pMutex->Lock();
    m_bOutstandingReadNotification = FALSE;

    pBuffer = NULL;
    ulAddress = 0;
    ulPort = 0;

    if (m_uSocketType != HX_UDP_SOCKET)
    {
        theErr = HXR_NET_SOCKET_INVALID;
        goto cleanup;
    }

    if (m_ReadUDPBuffers.GetCount() > 0)
    {
        pPacket = (UDP_PACKET*)m_ReadUDPBuffers.RemoveHead();;

        pBuffer = pPacket->pBuffer;
        ulAddress = pPacket->ulAddress;
        ulPort = pPacket->ulPort;

        HX_DELETE(pPacket);
    }
    else
    {
        if (!mLastError)
        {
            theErr = HXR_WOULD_BLOCK;
        }
        else
        {
            theErr = mLastError;
        }
    }

cleanup:

    if (!m_bReadPostPendingWouldBlock && !mLastError && !m_bNetworkIOPending)
    {
        theErr = PostIOMessage();
    }

    m_pMutex->Unlock();

    if( mLastError == HXR_OUTOFMEMORY )
    {
        theErr = mLastError;
    }
    return theErr;
}

HX_RESULT
ThreadedConn::write (void* buf, UINT16* size)
{
    HX_RESULT theErr = HXR_OK;
    HX_ASSERT(m_pActualConn && m_uSocketType == HX_TCP_SOCKET);

    m_pMutex->Lock();
    m_bOutstandingWriteNotification = FALSE;
    UINT16 uCount = m_pSendTCP->GetMaxAvailableElements();
    if (uCount > 0)
    {
        uCount = (uCount <= *size ? uCount : *size);
        m_pSendTCP->EnQueue(buf, uCount);
        *size = uCount;
    }
    else
    {
        *size = 0;
        if (!mLastError)
        {
            theErr = HXR_WOULD_BLOCK;
        }
        else
        {
            theErr = mLastError;
        }
    }

    if (!m_bWriteFlushPending && m_pSendTCP->GetQueuedItemCount() > 0 && m_bConnected)
    {
        m_bWriteFlushPending    = TRUE;
        AddRef();
    }

    if (!mLastError && !m_bNetworkIOPending)
    {
        theErr = PostIOMessage();
    }

    m_pMutex->Unlock();

    return theErr;
}

HX_RESULT
ThreadedConn::writeto(void* buf,UINT16* len, ULONG32 addr, UINT16 port)
{
    HX_RESULT theErr = HXR_OK;
    HX_ASSERT(m_pActualConn && m_uSocketType == HX_UDP_SOCKET);

    m_pMutex->Lock();
    m_bOutstandingWriteNotification = FALSE;
    if (!mLastError)
    {
        UDPPacketInfo* pPacket  = new UDPPacketInfo;
	if (HXR_OK == CreateAndSetBufferCCF(pPacket->m_pBuffer, (UCHAR*) buf, 
					    (ULONG32) *len, m_pContext))
	{
	    pPacket->m_ulAddr   = addr;
	    pPacket->m_uPort    = port;
	    m_WriteUDPBuffers.AddTail((void*) pPacket);
	}
	else
	{
	    HX_DELETE(pPacket);
	    theErr = HXR_OUTOFMEMORY;
	}
    }
    else
    {
        theErr = mLastError;
    }

    if (!theErr && !m_bWriteFlushPending && m_WriteUDPBuffers.GetCount() > 0 && m_bConnected)
    {
        m_bWriteFlushPending    = TRUE;
        AddRef();
    }

    if (!mLastError && !m_bNetworkIOPending)
    {
        theErr = PostIOMessage();
    }

    m_pMutex->Unlock();

    return theErr;
}

ULONG32
ThreadedConn::get_addr(void)
{
    ULONG32 ulAddr = 0;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        ulAddr = m_pActualConn->get_addr();
        m_pMutex->Unlock();
    }

    return ulAddr;
}

UINT16
ThreadedConn::get_local_port(void)
{
    UINT16 nPort = 0;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        nPort = m_pActualConn->get_local_port();
        m_pMutex->Unlock();
    }

    return nPort;
}


/* join_multicast_group() has this socket join a multicast group */
HX_RESULT
ThreadedConn::join_multicast_group(ULONG32 addr, ULONG32 if_addr)
{
    HX_RESULT theErr = HXR_UNEXPECTED;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        theErr = m_pActualConn->join_multicast_group(addr, if_addr);
        m_pMutex->Unlock();
    }

    return theErr;
}

HX_RESULT
ThreadedConn::leave_multicast_group(ULONG32 addr, ULONG32 if_addr)
{
    HX_RESULT theErr = HXR_UNEXPECTED;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        theErr = m_pActualConn->leave_multicast_group(addr, if_addr);
        while (!m_ReadUDPBuffers.IsEmpty())
        {
            UDP_PACKET* pPacket = (UDP_PACKET*)m_ReadUDPBuffers.RemoveHead();

            HX_RELEASE(pPacket->pBuffer);
            HX_DELETE(pPacket);
        }
        m_pMutex->Unlock();
    }

    return theErr;
}

HX_RESULT
ThreadedConn::reuse_addr(HXBOOL enabled)
{
    HX_RESULT theErr = HXR_UNEXPECTED;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        theErr = m_pActualConn->reuse_addr(enabled);
        m_pMutex->Unlock();
    }
    return theErr;
}

HX_RESULT
ThreadedConn::reuse_port(HXBOOL enabled)
{
    HX_RESULT theErr = HXR_UNEXPECTED;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        theErr = m_pActualConn->reuse_port(enabled);
        m_pMutex->Unlock();
    }
    return theErr;
}

HX_RESULT
ThreadedConn::set_broadcast(HXBOOL enable)
{
        HX_RESULT theErr = HXR_UNEXPECTED;
        HX_ASSERT(m_pActualConn);
        if(m_pActualConn)
        {
        m_pMutex->Lock();
        theErr = m_pActualConn->set_broadcast(enable);
        m_pMutex->Unlock();
        }
        return theErr;
}

HX_RESULT
ThreadedConn::set_multicast_if(ULONG32 ulInterface)
{
    HX_RESULT theErr = HXR_UNEXPECTED;
    HX_ASSERT(m_pActualConn);
    if(m_pActualConn)
    {
        m_pMutex->Lock();
        theErr = m_pActualConn->set_multicast_if(ulInterface);
        m_pMutex->Unlock();
    }
    return theErr;
}

void
ThreadedConn::IgnoreWSAECONNRESET(HXBOOL b)
{
    HX_RESULT theErr = HXR_UNEXPECTED;
    HX_ASSERT(m_pActualConn);
    if(m_pActualConn)
    {
        m_pMutex->Lock();
        m_pActualConn->IgnoreWSAECONNRESET(b);
        m_pMutex->Unlock();
    }
}

HX_RESULT
ThreadedConn::last_error(void)
{
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        mLastError = m_pActualConn->last_error();
        m_pMutex->Unlock();
    }

    return mLastError;
}

void
ThreadedConn::set_callback(HXAsyncNetCallback* pCallback)
{
    m_pMutex->Lock();
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        /* Set our callback as the callback */
        m_pActualConn->set_callback(m_pNetCallback);
    }

    mCallBack = pCallback;
    m_pMutex->Unlock();
}


UINT16
ThreadedConn::connection_open(void)
{
    UINT16 uConnOpen = 0;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        uConnOpen = m_pActualConn->connection_open();
        m_pMutex->Unlock();
    }

    return uConnOpen;
}

UINT16
ThreadedConn::connection_really_open(void)
{
    UINT16 uConnOpen = 0;
    //HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        uConnOpen = m_pActualConn->connection_really_open();
        m_pMutex->Unlock();
    }

    return uConnOpen;
}

int
ThreadedConn::get_sock(void)
{
    int iSockNum = -1;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        iSockNum =  m_pActualConn->get_sock();
        m_pMutex->Unlock();
    }
    return iSockNum;
}

void
ThreadedConn::set_sock(int theSock)
{
    m_pMutex->Lock();
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pActualConn->set_sock(theSock);
    }

    mSock = theSock;
    m_pMutex->Unlock();
}

HXBOOL
ThreadedConn::set_receive_buf_size(int DesiredSize)
{
    ThrdConnGenericCallback* pCallback  = new ThrdConnGenericCallback(this, SET_BUFFER_SIZE_CALLBACK_TYPE);
    pCallback->m_ulBufferSize = (UINT32) DesiredSize;
    /* Will be released by the thread engine */
    pCallback->AddRef();
    HXThreadMessage msg(HXMSG_ASYNC_CALLBACK, this, pCallback);
    m_pNetworkThread->PostMessage(&msg, NULL);
    return TRUE;
}


HXBOOL
ThreadedConn::ActualSetReceiveBufSize(UINT32 ulBufferSize)
{
    HXBOOL bResult = FALSE;
    HX_ASSERT(m_pActualConn);
    if (m_pActualConn)
    {
        m_pMutex->Lock();
        bResult = m_pActualConn->set_receive_buf_size((int) ulBufferSize);
        m_pMutex->Unlock();
    }

    return bResult;
}

void
ThreadedConn::OnAsyncDNS(HXBOOL bResult)
{
    if (mCallBack)
    {
        mCallBack->Func(DNS_NOTIFICATION, bResult);
    }
}

void
ThreadedConn::OnReadNotification(void)
{
    if (mCallBack)
    {
        mCallBack->Func(READ_NOTIFICATION);
    }
}

void
ThreadedConn::OnWriteNotification(void)
{
    if (mCallBack)
    {
        mCallBack->Func(WRITE_NOTIFICATION);
    }
}

void
ThreadedConn::OnConnect(HXBOOL bResult)
{
    if (mCallBack)
    {
        mCallBack->Func(CONNECT_NOTIFICATION, bResult);
    }
}

void
ThreadedConn::OnAcceptNotification()
{
    if( m_pIncommingConnections )
    {
        conn* pConn = (conn*)m_pIncommingConnections->RemoveHead();
	// accept_notification is posted in win_net:checkforconnection
	// callback->func(accept_notification..) calls this function
#ifndef HELIX_FEATURE_NETWORK_USE_SELECT
        if (mCallBack)
        {
            mCallBack->Func(ACCEPT_NOTIFICATION, TRUE, pConn);
        }
        if (pConn)
        {
            pConn->Release();
            pConn = NULL;
        }
#else
	// at this point we need to set pengine->reader = pconn
        HXThreadMessage msg(HXMSG_ASYNC_SETREADER_CONNECTION, this, pConn);
        m_pNetworkThread->PostMessage(&msg, NULL);
#endif

    }
}

void
ThreadedConn::HandleDNSNotification(HXBOOL bSuccess)
{
#if !defined( WIN32_PLATFORM_PSPC ) && !defined(HELIX_FEATURE_NETWORK_USE_SELECT)
    
    HXThreadMessage msg(HXMSG_ASYNC_DNS, this, (void*) bSuccess);
    m_pMainAppThread->PostMessage(&msg, m_pInternalWindowHandle);
    
#else
    OnAsyncDNS(bSuccess);
#endif
}

void
ThreadedConn::HandleConnectNotification(HXBOOL bSuccess)
{
    if (bSuccess)
    {
        m_bConnected = TRUE;
    }

#if !defined( WIN32_PLATFORM_PSPC ) && !defined(HELIX_FEATURE_NETWORK_USE_SELECT)
    HXThreadMessage msg(HXMSG_ASYNC_CONNECT, this, (void*) bSuccess);
    m_pMainAppThread->PostMessage(&msg, m_pInternalWindowHandle);
#else // No notifier on CE
    OnConnect(bSuccess);
#endif
}

void
ThreadedConn::HandleAcceptNotification(conn* pConn)
{
    ThreadedConn* pTConn = (ThreadedConn*)conn::new_socket(m_pContext, HX_TCP_SOCKET);
    pTConn->SetActualConn(pConn, m_ulUserHandle);
    m_pIncommingConnections->AddHead((conn*)pTConn);

#if !defined( WIN32_PLATFORM_PSPC ) && !defined(HELIX_FEATURE_NETWORK_USE_SELECT)
    HXThreadMessage msg(HXMSG_ASYNC_ACCEPT, this, NULL);
    m_pMainAppThread->PostMessage(&msg, m_pInternalWindowHandle);
#else // No notifier on CE
    OnAcceptNotification();
#endif
}

void
ThreadedConn::HandleCloseNotification()
{
    /* make one more read call to get back the actual error */
    m_bReadPostPendingWouldBlock = FALSE;
}

HX_RESULT
ThreadedConn::SetActualConn(conn* pConn, ULONG32 ulPlatform)
{
    if ( m_pActualConn )
    {
        m_pActualConn->done();
        m_pActualConn->Release();
    }
    m_pActualConn = pConn;
    m_pActualConn->AddRef();
    m_ulUserHandle = ulPlatform;
    m_bConnected = TRUE;
    return HXR_OK;
}

HX_RESULT
ThreadedConn::DoRead(HXBOOL bFromReadNotification)
{
#ifdef _CARBON
    AddRef(); // ensure that this object doesn't encounter its dtor before routine completes
#endif
    HX_RESULT theErr = HXR_OK;
    m_pMutex->Lock();

    /* Reset reading heuristing if we just got data. */
    if( bFromReadNotification )
    {
        m_bReadNowPending = m_bReadPostPendingWouldBlock = FALSE;
    }

    /* If the socket done has already been called, do not attempt to read
     * any more data
     */
    if (m_bIsDone)
    {
        goto exit;
    }

#ifdef THREADS_SUPPORTED
#ifdef HELIX_FEATURE_ADD_NETWORK_THREAD_SLEEP
    Sleep( m_ulNetworkThreadSleep );	// gives other threads a quote to run
#endif //HELIX_FEATURE_ADD_NETWORK_THREAD_SLEEP
#endif //THREADS_SUPPORTED

    if (m_uSocketType == HX_TCP_SOCKET)
    {
        UINT16 uCount = m_pReceiveTCP->GetMaxAvailableElements();

        if (uCount > 0)
        {
            UINT32 ulBytesToRead = conn::bytes_to_preparetcpread(this);

            if (ulBytesToRead > 0)
            {
                if ((UINT32)uCount > ulBytesToRead)
                {
                    uCount = (UINT16)ulBytesToRead;
                }

                if ( m_bReadPostPendingWouldBlock )
                {
                    /* fake a call return */
                    theErr = HXR_WOULD_BLOCK;
                }
                else
                {
                    /* call read and do heuristinc bookkeeping */
                    theErr = m_pActualConn->read(m_pTempBuffer, &uCount);
#ifndef HELIX_FEATURE_NETWORK_USE_SELECT
                    m_bReadPostPendingWouldBlock = (m_bReadNowPending && theErr == HXR_WOULD_BLOCK);
#endif
                    m_bReadNowPending = (theErr == HXR_WOULD_BLOCK ? TRUE : FALSE);
                }
                if (!theErr && uCount > 0)
                {
                    conn::bytes_to_actualtcpread(this, uCount);
                    m_pReceiveTCP->EnQueue(m_pTempBuffer, uCount);
                }
            }
        }

#ifndef _WINCE
        if (!m_bOutstandingReadNotification && m_pReceiveTCP->GetQueuedItemCount() > 0)
        {
#ifdef HELIX_FEATURE_NETWORK_USE_SELECT
            m_bOutstandingReadNotification  = TRUE;
            HXThreadMessage msg(HXMSG_ASYNC_READ, this, NULL);
            theErr = m_pMainAppThread->PostMessage(&msg, m_pInternalWindowHandle);
	    if ( theErr == HXR_NOT_INITIALIZED )
	    {
		theErr = HXR_OK;
	    }
#endif
        }
#endif /* _WINCE */
    }
    else /*if (m_uSocketType == HX_UDP_SOCKET)*/
    {
        UINT32 ulAddress = 0;
        UINT16 ulPort = 0;

        /* Read as much UDP data as possible */
        while (!theErr)
        {
            IHXBuffer* pBuffer = NULL;

            if ( m_bReadPostPendingWouldBlock )
            {
                /* fake a call return */
                theErr = HXR_WOULD_BLOCK;
            }
            else
            {
                /* call read and do heuristinc bookkeeping */
                theErr = m_pActualConn->readfrom(pBuffer, ulAddress, ulPort);
                if( theErr == HXR_OUTOFMEMORY )
                {
                    mLastError = HXR_OUTOFMEMORY;
                }
                /* If this is a single WOULDBLOCK, ReadNowPending gets set.  If this is the second
                   consecutive blocking call, ReadPostPendingWouldBlock gets set.
                   Feel free to suggest better variable names. */
#ifndef HELIX_FEATURE_NETWORK_USE_SELECT
                m_bReadPostPendingWouldBlock = (m_bReadNowPending && theErr == HXR_WOULD_BLOCK);
#endif
                m_bReadNowPending = (theErr == HXR_WOULD_BLOCK ? TRUE : FALSE);
            }

            if (!theErr && pBuffer)
            {
                UDP_PACKET* pPacket = new UDP_PACKET;

                if(pPacket)
                {
                    pPacket->pBuffer = pBuffer;
                    pPacket->ulAddress = ulAddress;
                    pPacket->ulPort = ulPort;
                    m_ReadUDPBuffers.AddTail((void*)pPacket);
                }
                else
                {
                    theErr = HXR_OUTOFMEMORY;
                }
            }
        }


#ifndef HELIX_FEATURE_NETWORK_USE_SELECT
        if ( !theErr && !m_bOutstandingReadNotification && m_ReadUDPBuffers.GetCount() > 0)
        {
            m_bOutstandingReadNotification  = TRUE;
            HXThreadMessage msg(HXMSG_ASYNC_READ, this, NULL);
            m_pMainAppThread->PostMessage(&msg, m_pInternalWindowHandle);
        }
#endif
    }


    if (!mLastError && theErr)
    {
        mLastError = ConvertNetworkError(theErr);
    }

    /* If there is an error, issue a Read Available message
     * so that error can be reported back on next Read
     */
    if (!m_bOutstandingReadNotification && mLastError && theErr != HXR_OUTOFMEMORY)
    {
        m_bOutstandingReadNotification  = TRUE;
        HXThreadMessage msg(HXMSG_ASYNC_READ, this, NULL);
#ifndef HELIX_FEATURE_NETWORK_USE_SELECT
        m_pMainAppThread->PostMessage(&msg, m_pInternalWindowHandle);
#else
	m_pNetworkThread->PostMessage(&msg, m_pInternalWindowHandle);
#endif
    }

exit:
    m_pMutex->Unlock();
#ifdef _CARBON
    Release();
#endif
    return theErr;
}

void
ThreadedConn::DoWrite()
{
    // If we are out of memory, let's just get out of here. Ideally, we should
    // not ever get to this point, but lots of functions here have void return
    // types, so it is possible to lose an OOM error.
    if( mLastError == HXR_OUTOFMEMORY )
    {
        return;
    }
#ifdef _CARBON
    AddRef();
#endif
    HX_RESULT theErr = HXR_OK;
    m_pMutex->Lock();
    if (m_uSocketType == HX_TCP_SOCKET)
    {
        UINT16 uCount = m_pSendTCP->GetQueuedItemCount();
        if (uCount > 0)
        {
            m_pSendTCP->DeQueue(m_pTempBuffer, uCount);
            UINT16 uActualCount = uCount;
            theErr = m_pActualConn->write(m_pTempBuffer, &uActualCount);
            switch(theErr)
            {
                case HXR_AT_INTERRUPT:
                case HXR_WOULD_BLOCK:
                case HXR_OK:
                    // enqueue the data that was not sent
                    if(uActualCount != uCount)
                    {
                        m_pSendTCP->EnQueue(m_pTempBuffer + uActualCount,
                                            uCount - uActualCount);
                    }

                    // mask out these errors
                    theErr = HXR_OK;
                    break;

                default:
                    break;
            }
        }

#ifndef HELIX_FEATURE_NETWORK_USE_SELECT
        if (!m_bIsDone && !m_bOutstandingWriteNotification && m_pSendTCP->GetMaxAvailableElements() > 0)
        {
            m_bOutstandingWriteNotification = TRUE;
            HXThreadMessage msg(HXMSG_ASYNC_WRITE, this, NULL);
            m_pMainAppThread->PostMessage(&msg, m_pInternalWindowHandle);
        }
#endif
    }
    else /*if (m_uSocketType == HX_UDP_SOCKET)*/
    {
        while (!theErr && m_WriteUDPBuffers.GetCount() > 0)
        {
            UDPPacketInfo* pPacket = (UDPPacketInfo*) m_WriteUDPBuffers.GetHead();
            UINT16 uLength = (UINT16) pPacket->m_pBuffer->GetSize();
            theErr = m_pActualConn->writeto(pPacket->m_pBuffer->GetBuffer(),   // sendto
                                        &uLength,
                                        pPacket->m_ulAddr,
                                        pPacket->m_uPort);
            if (!theErr)
            {
                pPacket->m_pBuffer->Release();
                delete pPacket;
                m_WriteUDPBuffers.RemoveHead();
            }
        }

#ifndef HELIX_FEATURE_NETWORK_USE_SELECT
        if (!m_bIsDone && !m_bOutstandingWriteNotification && m_WriteUDPBuffers.GetCount() == 0)
        {
            m_bOutstandingWriteNotification = TRUE;
            HXThreadMessage msg(HXMSG_ASYNC_WRITE, this, NULL);
            m_pMainAppThread->PostMessage(&msg, m_pInternalWindowHandle);
        }
#endif
    }

    if (!mLastError && theErr)
    {
        mLastError = ConvertNetworkError(theErr);
    }

#ifndef HELIX_FEATURE_NETWORK_USE_SELECT
    if (!mLastError && !m_bNetworkIOPending &&
        ((m_uSocketType == HX_TCP_SOCKET && m_pSendTCP->GetQueuedItemCount() > 0) ||
         (m_uSocketType == HX_UDP_SOCKET && m_WriteUDPBuffers.GetCount() > 0)))
    {
        theErr = PostIOMessage();
    }
#endif

    m_pMutex->Unlock();

    if (m_bWriteFlushPending &&
        ((m_uSocketType == HX_TCP_SOCKET && m_pSendTCP->GetQueuedItemCount() == 0) ||
         (m_uSocketType == HX_UDP_SOCKET && m_WriteUDPBuffers.GetCount() == 0)))
    {
        m_bWriteFlushPending    = FALSE;
        Release();
    }

    /* We are done and there is no more data pending to bw written out */
    /* This is the time socket actually gets destroyed */
    if (m_bIsDone && !m_bWriteFlushPending)
    {
        m_bConnected = FALSE;
        PostDoneAndDetach();
    }
#ifdef _CARBON
    Release();
#endif
}

//--------------------------------------------------
void
ThreadedConn::DoNetworkIO(void)
{
    m_bNetworkIOPending = FALSE;
    if (m_bConnected)
    {
        // DoRead now has a return type, but since DoNetworkIO does not we
        // are going to ignore any errors returned. This may not be a good idea.
#if !defined( WIN32_PLATFORM_PSPC ) /*&& !defined( _UNIX )*/
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
	if (!m_pNetworkThread->m_bUseReaderWriter)
#endif //defined(HELIX_FEATURE_NETWORK_USE_SELECT)
        DoRead();
        DoWrite();
#else
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
	if (!m_pNetworkThread->m_bUseReaderWriter)
#endif //defined(HELIX_FEATURE_NETWORK_USE_SELECT)
        DoRead(TRUE);
        DoWrite();
#endif
    }
}
HX_RESULT
ThreadedConn::ConvertNetworkError(HX_RESULT theErr)
{
    HX_RESULT lResult = theErr;
    if (!IS_SERVER_ALERT(theErr))
    {
        switch(theErr)
        {
            case HXR_AT_INTERRUPT:                      // mask out these errors
            case HXR_WOULD_BLOCK:
            case HXR_NO_DATA:
            case HXR_OK:
                lResult = HXR_OK;
                break;

            case HXR_DNR:
            case HXR_NET_CONNECT:
            case HXR_DOC_MISSING:
            case HXR_OUTOFMEMORY:
            case HXR_ADVANCED_SERVER:
            case HXR_BAD_SERVER:
            case HXR_OLD_SERVER:
            case HXR_INVALID_FILE:
            case HXR_REDIRECTION:
            case HXR_PROXY:
            case HXR_PROXY_RESPONSE:
            case HXR_ADVANCED_PROXY:
            case HXR_OLD_PROXY:
            case HXR_PERFECTPLAY_NOT_SUPPORTED:
            case HXR_NO_LIVE_PERFECTPLAY:
            case HXR_PERFECTPLAY_NOT_ALLOWED:
                break;

            default:
                lResult = HXR_FAIL;
                break;
        }
    }

    return lResult;
}

HX_RESULT
ThreadedConn::PostIOMessage(void)
{
    m_bNetworkIOPending = TRUE;
    HXThreadMessage msg(HXMSG_ASYNC_NETWORKIO, this, NULL);
    return m_pNetworkThread->PostMessage(&msg, NULL);
}


HX_RESULT ThreadedConn::ThrConnSocketCallback::Func(NotificationType Type,
                                               HXBOOL bSuccess, conn* pConn)
{
    ThreadedConn* pContext = m_pContext;
    // It would be nice to set theErr for all of the calls below, but the
    // effects of that are unknown to this developer. XXXJHHB
    HX_RESULT theErr = HXR_OK;
    if(pContext)
    {
        switch (Type)
        {
        case READ_NOTIFICATION:
            theErr = pContext->DoRead(TRUE);
            break;
        case WRITE_NOTIFICATION:
            pContext->DoWrite();
            break;
        case CONNECT_NOTIFICATION:
            pContext->HandleConnectNotification(bSuccess);
            break;
        case DNS_NOTIFICATION:
            pContext->HandleDNSNotification(bSuccess);
            break;
        case ACCEPT_NOTIFICATION:
            pContext->HandleAcceptNotification(pConn);
#ifdef HELIX_FEATURE_NETWORK_USE_SELECT
            break;
#endif
        case CLOSE_NOTIFICATION:
            pContext->HandleCloseNotification();
            break;
#ifdef HELIX_FEATURE_NETWORK_USE_SELECT
        case SEND_BUFFER_NOTIFICATION:
	    UINT16 len;
	    len = sizeof(HXThreadMessage);
	    pContext->m_bNetworkIOPending = TRUE;
            pContext->write((char *)bSuccess,&len);
            pContext->DoWrite();
	    pContext->m_bNetworkIOPending = FALSE;
            break;
#endif
        default:
            break;
        }
    }

    return theErr;
}



ThreadedConn::ThrdConnGenericCallback::ThrdConnGenericCallback(ThreadedConn* pConn, UINT16 uCallbackType)
    : m_lRefCount (0)
    , m_uCallbackType (uCallbackType)
    , m_pConn (pConn)
    , m_bBlocking (FALSE)
    , m_ulLocalAddr (0)
    , m_uPort (0)
    , m_ulBufferSize (0)
    , m_uBacklog(0)
//    , m_pNewConn(NULL)
{
}

ThreadedConn::ThrdConnGenericCallback::~ThrdConnGenericCallback()
{
//    HX_RELEASE(m_pNewConn);
}

/*
 * IUnknown methods
 */

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your
//              object.
//
STDMETHODIMP ThreadedConn::ThrdConnGenericCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::AddRef
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) ThreadedConn::ThrdConnGenericCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::Release
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) ThreadedConn::ThrdConnGenericCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *      IHXCallback methods
 */
STDMETHODIMP ThreadedConn::ThrdConnGenericCallback::Func(void)
{
    if (m_pConn)
    {
        switch (m_uCallbackType)
        {
            case DNS_CALLBACK_TYPE:
                m_pConn->ActualDnsFindIpAddr(m_HostName, m_bBlocking);
                break;
            case INIT_CALLBACK_TYPE:
                m_pConn->ActualInit(m_ulLocalAddr, m_uPort, m_bBlocking);
                break;
            case SETWINDOWHANDLE_CALLBACK_TYPE:
#if defined (_WIN32) || defined (_WINDOWS)
                m_pConn->ActuallSetWindowHandle(m_ulHandle);
#endif /*defined (_WIN32) || defined (_WINDOWS)*/
                break;
            case CONNECT_CALLBACK_TYPE:
                m_pConn->ActualConnect(m_HostName, m_uPort, m_bBlocking,
                                        m_ulHandle);
                break;
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
            case ACCEPT_CALLBACK_TYPE:
                m_pConn->ActualAccept(m_ulLocalAddr,
                                        m_ulHandle);
                break;
#endif //defined(HELIX_FEATURE_NETWORK_USE_SELECT)
            case BLOCKING_CALLBACK_TYPE:
                m_pConn->ActualBlocking();
                break;
            case NONBLOCKING_CALLBACK_TYPE:
                m_pConn->ActualNonBlocking();
                break;
            case DONE_CALLBACK_TYPE:
                m_pConn->ActualDone();
                break;
            case SET_BUFFER_SIZE_CALLBACK_TYPE:
                m_pConn->ActualSetReceiveBufSize(m_ulBufferSize);
                break;
            case LISTEN_CALLBACK_TYPE:
                m_pConn->ActualListen(m_ulLocalAddr, m_uPort, m_uBacklog,
                                m_bBlocking, m_ulHandle);
                break;
            default:
                break;
        }
    }
    return HXR_OK;
}
