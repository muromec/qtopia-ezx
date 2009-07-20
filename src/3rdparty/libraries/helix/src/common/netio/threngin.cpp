/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: threngin.cpp,v 1.13 2006/08/16 17:28:19 gwright Exp $
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

#include <stdio.h>
#include "hxresult.h"
#include "hxassert.h"
#include "hxmap.h"
#include "hxcom.h"
#include "hxengin.h"
#include "hxthread.h"
#include "conn.h"
#include "thrdconn.h"
#include "threngin.h"
#include "hxtick.h"
#include "pckunpck.h"

#if defined(_UNIX) && (defined( _UNIX_THREADED_NETWORK_IO ) || defined(THREADS_SUPPORTED))
#include "platform/unix/UnixThreads.h"
#include "platform/unix/unix_net.h"
#endif
 
#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)
#include "carbthrd.h"
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif                                                                   

#define ALLFS       0xFFFFFFFF
#define LOCAL_LOOPBACK_ADDR 0x7F000001 // 127.0.0.1
#define LOCAL_LOOPBACK_NAME "127.0.0.1"
#define LOCAL_LOOPBACK_PORT 0x1234

void* NetworkThreadMainLoop(void* pArg);

ThreadEngine*   ThreadEngine::m_pzThreadEngine = NULL;

/* Global destructor will be called at DLL shutdown and thread engine
 * will be destroyed
 */ 
DestructEngine selfDestructor;
    
#if defined(_UNIX_THREADED_NETWORK_IO)
void UnixMessageLoop(void* pARG)
{
    HX_ASSERT(pARG);
    ThreadEngine* pEngine = (ThreadEngine*)pARG;
    HX_ASSERT( pEngine );
    IHXThread* pMainAppThread = pEngine->GetMainAppThread();
    HX_ASSERT( pMainAppThread );
    struct HXThreadMessage msg;
    while(pMainAppThread->PeekMessage(&msg, 0, 0, TRUE )==HXR_OK)
    {
        if( msg.m_ulMessage != 0 )
        {
            // select state events for app thread to recevie
            ThreadedConn* pThat = (ThreadedConn*)msg.m_pParam1;
            if (pThat != NULL)
            {
                switch(msg.m_ulMessage)
                {
                   case HXMSG_ASYNC_DNS:
                       pThat->OnAsyncDNS((HXBOOL)msg.m_pParam2);
                       break;
                   case HXMSG_ASYNC_CONNECT:
                       pThat->OnConnect((HXBOOL)msg.m_pParam2);
                       break;
                   case HXMSG_ASYNC_READ:
                       pThat->OnReadNotification();
                       break;
                   case HXMSG_ASYNC_WRITE:
                       pThat->OnWriteNotification();
                       break;
                   case HXMSG_ASYNC_ACCEPT:
                       pThat->OnAcceptNotification();
                       break;
                   default:
                       HX_ASSERT("Unknown message" == NULL );
                       break;
                }
            }
        }
    }
    HX_RELEASE(pMainAppThread);
    pEngine->m_pUnixMessageLoop->ScheduleRelative(pEngine->m_pScheduler, 10);
}

#endif

ThreadEngine*    
ThreadEngine::GetThreadEngine(IUnknown* pContext)
{
    if (!m_pzThreadEngine)
    {
        m_pzThreadEngine = new ThreadEngine(pContext); 
    }
    return m_pzThreadEngine;
}

ThreadEngine::ThreadEngine(IUnknown* pContext)
    : m_pNetworkThread(0)
    , m_pMainAppThread(0)
    , m_pMutex(0)
    , m_pSockMap(0)
    , m_pContext(pContext)
    , m_pQuitEvent(NULL)
    , m_bInDestructor(FALSE)
#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)
    , m_ThreadedNetworkingCarbonTimerUPP(NULL)
    , m_ThreadedNetworkingCarbonTimerRef(NULL)
#endif
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
    , m_pLocalListener(NULL)
    , m_pLocalReader(NULL)
    , m_pLocalWriter(NULL)
#if defined(_UNIX_THREADED_NETWORK_IO)
    , m_pScheduler(NULL);
    , m_pUnixMessageLoop(NULL)
#endif

#endif // HELIX_FEATURE_NETWORK_USE_SELECT
{
    HX_ADDREF(m_pContext);

    m_pzThreadEngine = this;

#if !defined(THREADS_SUPPORTED) && defined(_UNIX_THREADED_NETWORK_IO)
    // THREAD_SUPPORTED should be defined whenever _UNIX_THREADED_NETWORK_IO
    // is defined, please check the config file of the platform,
    // Otherwise, the mutex created below would be a stub mutex.
    HX_ASSERT(FALSE);
#endif
    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);
    CreateInstanceCCF(CLSID_IHXThread, (void**)&m_pNetworkThread, m_pContext);
    /* This is just a wrapper thread for the main application 
     * It is really a hack. Will do it the right way sometime soon*/
    CreateInstanceCCF(CLSID_IHXThread, (void**)&m_pMainAppThread, m_pContext);

    CreateEventCCF((void**)&m_pQuitEvent, m_pContext, NULL, TRUE);

    m_pNetworkThread->CreateThread(NetworkThreadMainLoop, (void*) this, 0);
    m_pNetworkThread->SetThreadName("Old NetServices Network Thread");
    m_pSockMap = new CHXMapPtrToPtr;
    
    m_pMainAppThread->YieldTimeSlice();

#if defined(_UNIX_THREADED_NETWORK_IO)
    
    m_pContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
    HX_ASSERT(m_pScheduler);
    m_pUnixMessageLoop = new CHXGenericCallback((void*)this, UnixMessageLoop);
    HX_ADDREF(m_pUnixMessageLoop);
    m_pUnixMessageLoop->ScheduleRelative(m_pScheduler, 10);
    
#endif
    
    
#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)
    HX_ASSERT(!m_ThreadedNetworkingCarbonTimerUPP);
    HX_ASSERT(!m_ThreadedNetworkingCarbonTimerRef);
    
    m_ThreadedNetworkingCarbonTimerUPP = ::NewEventLoopTimerUPP(
                (EventLoopTimerProcPtr)FauxMainAppCarbonTimer);
    InstallEventLoopTimer(GetCurrentEventLoop(), 0, 50 * kEventDurationMillisecond,
                m_ThreadedNetworkingCarbonTimerUPP, this, &m_ThreadedNetworkingCarbonTimerRef);

#endif
}

ThreadEngine::~ThreadEngine()
{
    m_pMutex->Lock();

    m_bInDestructor = TRUE;
    if (m_pSockMap && m_pSockMap->GetCount() > 0)
    {
        CHXMapPtrToPtr::Iterator ndxConn = m_pSockMap->Begin();
        for (; ndxConn != m_pSockMap->End(); ++ndxConn)
        {
            ThreadedConn* pConn = (ThreadedConn*) (*ndxConn);
            pConn->AddRef();
        }
    }

    m_pMutex->Unlock();

    if (m_pSockMap)
    {
        //HX_ASSERT(m_pSockMap->GetCount() == 0);
        if (m_pSockMap->GetCount() > 0)
        {
            CHXMapPtrToPtr::Iterator ndxConn = m_pSockMap->Begin();
            for (; ndxConn != m_pSockMap->End(); ++ndxConn)
            {
                ThreadedConn* pConn = (ThreadedConn*) (*ndxConn);
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
                if (pConn == m_pLocalReader ||
                    pConn == m_pLocalWriter ||
                    pConn == m_pLocalListener)
                    continue;
#endif // HELIX_FEATURE_NETWORK_USE_SELECT
                pConn->finaldone();
                pConn->Release();
            }
        }   
    }

    if (m_pNetworkThread)
    {
        HXThreadMessage msg(HXMSG_QUIT, NULL, NULL);

        if (m_pNetworkThread->PostMessage(&msg, NULL) == HXR_OK)
        {
            m_pQuitEvent->Wait(ALLFS);
        }
        
        m_pNetworkThread->Exit(0);
        HX_RELEASE(m_pNetworkThread);
    }

#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
    m_pLocalReader->Release();
    m_pLocalWriter->Release();
    m_pLocalListener->Release();
#endif // HELIX_FEATURE_NETWORK_USE_SELECT

#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)
    ::RemoveEventLoopTimer(m_ThreadedNetworkingCarbonTimerRef);
    m_ThreadedNetworkingCarbonTimerRef;
    
    ::DisposeEventLoopTimerUPP(m_ThreadedNetworkingCarbonTimerUPP);
    m_ThreadedNetworkingCarbonTimerUPP;
#endif


#if defined(_UNIX_THREADED_NETWORK_IO)
    
    m_pUnixMessageLoop->Cancel(m_pScheduler);
    HX_RELEASE(m_pUnixMessageLoop);
    HX_RELEASE(m_pScheduler);
    
#endif

    HX_RELEASE(m_pMainAppThread);
    HX_RELEASE(m_pQuitEvent);

    if (m_pSockMap)
    {
        m_pSockMap->RemoveAll();
        delete m_pSockMap;
        m_pSockMap = 0;
    }

    HX_RELEASE(m_pMutex);
    HX_RELEASE(m_pContext);
}

void 
ThreadEngine::DestroyThreadEngine(void)
{
    if (m_pzThreadEngine)
    {
        ThreadEngine* pThreadEngine = m_pzThreadEngine;
        m_pzThreadEngine = NULL;
        delete pThreadEngine;
    }
}

HX_RESULT
ThreadEngine::AttachSocket(ThreadedConn* pConn)
{
    void* pVoid = 0;
    HX_RESULT theErr = HXR_OK;

    m_pMutex->Lock();
    if (m_pSockMap->Lookup(pConn, pVoid))
    {
        theErr = HXR_FAIL;
    }
    else
    {
        pConn->AddRef();
        m_pSockMap->SetAt((void*) pConn, (void*) pConn);
    }

    if (m_pSockMap->GetCount() == 1)
    {
#ifdef WIN32_PLATFORM_PSPC
        HXThreadMessage msg(HXMSG_ASYNC_RESUME, (void*) 200, NULL);
#else
        HXThreadMessage msg(HXMSG_ASYNC_RESUME, (void*) 20, NULL);
#endif
        m_pNetworkThread->PostMessage(&msg, NULL);
    }

    m_pMutex->Unlock();

    return theErr;
}

HX_RESULT
ThreadEngine::DetachSocket(ThreadedConn* pConn)
{
    HX_RESULT theErr = HXR_OK;

    m_pMutex->Lock();

    void* blah;
    if (!m_pSockMap->Lookup((void*) pConn, (void*&) blah))
    {
        theErr = HXR_FAIL;
    }
    else
    {
        /* if we are in the destructor, no need to remove from the map */
        if (!m_bInDestructor)
        {
            m_pSockMap->RemoveKey((void*) pConn);
        }

        pConn->Detached();
        pConn->Release();
    }

    if (m_pSockMap->GetCount() == 0)
    {
        HXThreadMessage msg(HXMSG_ASYNC_STOP, NULL, NULL);
        m_pNetworkThread->PostMessage(&msg, NULL);
    }

    m_pMutex->Unlock();

    return theErr;
}

//-----------------------------------------------------------
void
ThreadEngine::DoAsyncCallback(ThreadedConn* pConn, IHXCallback* pCallback)
{
    void* pVoid = 0;

    m_pMutex->Lock();

    /* Fire callback ONLY if the connected socket is still active*/
    if (m_pSockMap->Lookup(pConn, pVoid))
    {
        pCallback->Func();
    }
    pCallback->Release();

    m_pMutex->Unlock();
}
//-----------------------------------------------------------
void
ThreadEngine::DoNetworkIO(ThreadedConn* pConn)
{
    CHXMapPtrToPtr::Iterator ndxConn;

    m_pMutex->Lock();

    if (m_bInDestructor)
    {
        goto exit;
    }

    if (pConn)
    {
        void* pVoid = 0;
        if (m_pSockMap->Lookup(pConn, pVoid))
        {
            pConn->DoNetworkIO();
        }
        goto exit;
    }

    ndxConn = m_pSockMap->Begin();
    for (; ndxConn != m_pSockMap->End(); ++ndxConn)
    {
        ThreadedConn* pConn = (ThreadedConn*) (*ndxConn);
        pConn->DoNetworkIO();
    }

exit:
    m_pMutex->Unlock();
}
//-----------------------------------------------------------
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
HX_RESULT 
ThreadEngine::WaitforSelect()
{
    HX_RESULT theErr = HXR_FAIL;
    ThreadedConn *pConn = NULL;

    if (m_bInDestructor)
        pConn = m_pLocalReader;

    if (m_pLocalReader && m_pLocalReader->GetActualConn())
        theErr = m_pLocalReader->GetActualConn()->WaitforSelect(this,pConn);
    return theErr;
}

#endif // HELIX_FEATURE_NETWORK_USE_SELECT
    



//-----------------------------------------------------------
void* NetworkThreadMainLoop(void* pArg)
{
    ThreadEngine*   pEngine  = (ThreadEngine*) pArg;
    IUnknown*       pContext = pEngine->m_pContext;
    IHXThread*      pThread  = pEngine->m_pNetworkThread;
    HXThreadMessage msg;
    HXBOOL          bDone   = FALSE;
    UINT32          ulSleepTime = 0;
    UINT32          ulLastTimerCallback = HX_GET_TICKCOUNT();
    UINT32          ulTimerId = 0;

#if defined( _WIN32 ) || defined( _UNIX_THREADED_NETWORK_IO ) || defined(THREADS_SUPPORTED)
    IHXAsyncTimer* pAsyncTimer=NULL;
    CreateInstanceCCF(CLSID_IHXAsyncTimer, (void**)&pAsyncTimer, pContext);
#endif /*_WIN32 || _UNIX_THREADED_NETWORK_IO */
    
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
    unsigned short iSize;
    HX_RESULT   theErr = HXR_FAIL;
    UINT32      ulPlatformData  = 0;
    HXBOOL      bRWStartup = FALSE;
    HXBOOL      bMoreToRead = FALSE;
    HXBOOL      bDebug = FALSE;

    pEngine->m_pLocalListener  = new ThreadedConn(HX_TCP_SOCKET);
    pEngine->m_pLocalWriter  = new ThreadedConn(HX_TCP_SOCKET);
    pEngine->m_pLocalReader  = NULL;

    pThread->SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);
//    pThread->SetPriority(THREAD_PRIORITY_HIGHEST);

#if defined (_WIN32)
    ulPlatformData = (UINT32)GetModuleHandle(NULL);
#elif defined (_WIN16)
    ulPlatformData = (UINT32)(int)g_hInstance;
#endif

    if (pEngine->m_pLocalListener && pEngine->m_pLocalWriter)
    {
        theErr = pEngine->m_pLocalListener->listen(
                    LOCAL_LOOPBACK_ADDR
                    , LOCAL_LOOPBACK_PORT
                    , 4 // backlog
                    , 0 // blocking
                    , ulPlatformData // ulPlatform
                    );
    }
    if (SUCCEEDED(theErr))
    {
    // writer connects and reader accepts
        theErr = pEngine->m_pLocalWriter->connect(
                    LOCAL_LOOPBACK_NAME
                    , LOCAL_LOOPBACK_PORT
                    , 0 // blocking
                    , ulPlatformData // ulPlatform
                    );
    }
    
    if (SUCCEEDED(theErr) && !bDebug)
    {
        // writer connects and reader accepts
        theErr = pEngine->m_pLocalListener->accept(LOCAL_LOOPBACK_ADDR);
    }
    

    if (FAILED(theErr))
    {
        if (pEngine->m_pLocalListener)
        {
            pEngine->DetachSocket(pEngine->m_pLocalListener);
            pEngine->m_pLocalListener = NULL;
        }
        if (pEngine->m_pLocalWriter)
        {
            pEngine->DetachSocket(pEngine->m_pLocalWriter);
            pEngine->m_pLocalWriter = NULL;
        }
        if (pEngine->m_pLocalReader)
        {
            pEngine->DetachSocket(pEngine->m_pLocalReader);
            pEngine->m_pLocalReader = NULL;
        }
    }

    while (!bDone)
    {
        if (bDebug || FAILED(theErr) 
            || !pEngine->m_pLocalWriter 
            || !pEngine->m_pLocalReader 
            || !pEngine->m_pLocalWriter->connection_really_open()
            || !pEngine->m_pLocalReader->connection_really_open()
            )
        {
            // process connect and accept messages, then use select
            pEngine->m_pNetworkThread->m_bUseReaderWriter = FALSE;
            if( 0 == ulTimerId )
            {
                ulSleepTime = 20;
                ulTimerId = pAsyncTimer->SetTimer(ulSleepTime, pThread);
            }
            if (pThread->GetMessage(&msg, 0, 0) != HXR_OK)
            {
                break;
            }
        }
        else
        {
            if (!pEngine->m_pNetworkThread->m_bUseReaderWriter || bRWStartup)
            { // process messages in queue
                if (!bRWStartup)
                {
                    if(ulTimerId>0)
                    {
                        pAsyncTimer->KillTimer(ulTimerId);
                        ulTimerId = 0;
                    }
                    msg.m_ulMessage = HXMSG_ASYNC_START_READERWRITER;
                    theErr = pEngine->m_pNetworkThread->PostMessage(&msg, NULL);
                }
                pEngine->m_pNetworkThread->m_bUseReaderWriter = TRUE;
                pEngine->m_pLocalWriter->set_callback(pEngine->m_pLocalWriter->get_callback()); // for post network msg
                pThread->SetNetworkMessageConnection(pEngine->m_pLocalWriter);
                bRWStartup = TRUE;
                if (pThread->GetMessage(&msg, 0, 0) != HXR_OK)
                    break;
            }
            else
            {
                if (!bMoreToRead)
                {
                    if (HXR_FAIL == pEngine->WaitforSelect())
                        break;
                }
                msg.m_ulMessage = 0;
                iSize = sizeof(HXThreadMessage);
                pEngine->m_pLocalReader->m_bNetworkIOPending = TRUE;
                theErr = pEngine->m_pLocalReader->read(&msg,&iSize);
                if (HXR_OK != theErr)
                {
                    if (HXR_WOULD_BLOCK == theErr)
                    {
                        bMoreToRead = FALSE;
                        theErr = HXR_OK;
                        continue;
                    }
                    // reader failed, must be disconnected, return to msg loop
                    break;
                }
                if (iSize != sizeof(HXThreadMessage)) // fixme
                {
                    bMoreToRead = FALSE;
                    continue;
                }
                else
                    bMoreToRead = TRUE;
            }
        }
#else
    while (!bDone && pThread->GetMessage(&msg, 0, 0) == HXR_OK)
    {
#endif // HELIX_FEATURE_NETWORK_USE_SELECT
        switch (msg.m_ulMessage)
        {
#if defined( _WIN32 ) || defined( _UNIX_THREADED_NETWORK_IO ) || defined(THREADS_SUPPORTED)
           case HXMSG_ASYNC_TIMER: //Look in hxmsgs.h (WM_TIMER under win32)
           {
#if defined(_UNIX_THREADED_NETWORK_IO) || (defined(THREADS_SUPPORTED) && defined(_UNIX))
               unix_TCP::process_idle();
#endif                    
               ULONG32 ulCurrentTime = HX_GET_TICKCOUNT();
               
               if (CALCULATE_ELAPSED_TICKS(ulLastTimerCallback, 
                                           ulCurrentTime) >= ulSleepTime)
               {
                   ulLastTimerCallback = ulCurrentTime;
                   pEngine->DoNetworkIO(); 
               }
           }
           break;
#endif /* _WIN32 || _UNIX_THREADED_NETWORK_IO */
           case HXMSG_ASYNC_NETWORKIO:
           {
               ThreadedConn* pConn = (ThreadedConn*) msg.m_pParam1; 
               pEngine->DoNetworkIO(pConn);
           }
           break;
           case HXMSG_ASYNC_CALLBACK:
           {
               ThreadedConn* pConn = (ThreadedConn*) msg.m_pParam1; 
               IHXCallback* pCallback = (IHXCallback*) msg.m_pParam2;
               pEngine->DoAsyncCallback(pConn, pCallback);
           }
           break;
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
           case HXMSG_ASYNC_SETREADER_CONNECTION:
           {
               pEngine->m_pLocalReader = (ThreadedConn*) msg.m_pParam2; 
           }
           break;
           case HXMSG_ASYNC_START_READERWRITER:
           {
                pEngine->m_pNetworkThread->m_bUseReaderWriter = TRUE;
                bRWStartup = FALSE;
           }
           break;
#endif //HELIX_FEATURE_NETWORK_USE_SELECT
           case HXMSG_ASYNC_DETACH:
           {
               ThreadedConn* pConn = (ThreadedConn*) msg.m_pParam1; 
               pEngine->DetachSocket(pConn); 
           }
           break;
           case HXMSG_QUIT:
           {
               bDone    = TRUE;
           }
           
           break;
           case HXMSG_ASYNC_RESUME:
#if defined( _WIN32 ) || defined( _UNIX_THREADED_NETWORK_IO ) || defined(THREADS_SUPPORTED)
           {
               if (ulTimerId > 0)
               {
                   pAsyncTimer->KillTimer(ulTimerId);
                   ulTimerId = 0;
               }
               
               ulSleepTime = (UINT32)(PTR_INT)msg.m_pParam1;
               ulTimerId = pAsyncTimer->SetTimer( ulSleepTime, pThread );
           }
#endif /* _WIN32 || _UNIX_THREADED_NETWORK_IO */
           break;
           
           case HXMSG_ASYNC_STOP:
#if defined( _WIN32 ) || defined( _UNIX_THREADED_NETWORK_IO ) || defined(THREADS_SUPPORTED)
           {
               if( ulTimerId > 0 )
               {
                   pAsyncTimer->KillTimer( ulTimerId );
                   ulTimerId = 0;
               }
           }
#endif /* _WIN32 || _UNIX_THREADED_NETWORK_IO */
           break;
           default:
               pThread->DispatchMessage(&msg);
               break;
        }
    }
#if defined( _WIN32 ) || defined( _UNIX_THREADED_NETWORK_IO ) || defined(THREADS_SUPPORTED)
#endif
    
    pEngine->m_pQuitEvent->SignalEvent();
    
    HX_RELEASE(pAsyncTimer);
    
    return (void*) 0;
}

#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)

void ThreadEngine::FauxMainAppCarbonTimer(EventLoopTimerRef, void* /* unused */)
{
    // xxxbobclark this is essentially just ripped off from Greg Wright's
    // similar message handling loop for Unix.
    //
    // It's been moved to pnio because more places than just rmacore
    // (i.e. rmacleng) use the threaded implementation of networking,
    // viz. rnqueue for Auto-Update. Using a Carbon Timer to get time
    // periodically can luckily be implemented in pnio; it's sure to
    // get actual System Time.
    
    ThreadEngine* pEngine = ThreadEngine::GetThreadEngine();
    HX_ASSERT(pEngine);
    IHXThread* pMainAppThread = pEngine->GetMainAppThread();
    HX_ASSERT(pMainAppThread);
    HXThreadMessage msg;
    
    while (pMainAppThread->PeekMessage(&msg, 0, 0, TRUE) == HXR_OK)
    {
        if (msg.m_ulMessage != 0)
        {
            ThreadedConn* pThreadedConn = (ThreadedConn*)msg.m_pParam1;
            if (pThreadedConn)
            {
                switch (msg.m_ulMessage)
                {
                    case HXMSG_ASYNC_DNS:
                        pThreadedConn->OnAsyncDNS((HXBOOL)msg.m_pParam2);
                        break;

                    case HXMSG_ASYNC_CONNECT:
                        pThreadedConn->OnConnect((HXBOOL)msg.m_pParam2);
                        break;

                    case HXMSG_ASYNC_READ:
                        pThreadedConn->OnReadNotification();
                        break;

                    case HXMSG_ASYNC_WRITE:
                        pThreadedConn->OnWriteNotification();
                        break;

                    case HXMSG_ASYNC_ACCEPT:
                        pThreadedConn->OnAcceptNotification();
                        break;

                    default:
                        HX_ASSERT(!"Unknown message in threaded networking Carbon Timer");
                        break;

                }
            }
        }
    }
    HX_RELEASE(pMainAppThread);
}

#endif
