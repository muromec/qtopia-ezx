/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxnetapi.cpp,v 1.42 2007/07/06 21:58:22 jfinnecy Exp $
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
#include "hxresult.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxprefs.h"
#include "hxerror.h"

#if defined(_MACINTOSH)
#include "hx_moreprocesses.h"
#endif /* _MACINTOSH. */

#include "ihxpckts.h"
#include "hxfiles.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "hxengin.h"
#include "hxpnets.h"

#include "hxtick.h"
#include "hxstrutl.h"

#include "chxuuid.h"
#include "rtsputil.h"

#include "hxslist.h"
#include "cbqueue.h"
#include "growingq.h"
#include "dbcs.h"
#include "conn.h"
#include "hxmarsh.h"
#include "hxthread.h"
#include "hxcore.h"
#include "hxmutex.h"

#if defined (_UNIX)
#include "unix_net.h"
#include "platform/unix/hxsignal.h"
#elif defined (_WIN32) || defined (_WINDOWS)
#include "win_net.h"
#elif defined(__TCS__)
#include "platform/tm1/tm1_net.h"
#elif defined (_MACINTOSH)
#ifndef _MAC_MACHO
#include "OpenTptInternet.h" //for AF_INET
#endif
#include "mac_net.h"
#include "macsockets.h"
#endif

#include "threngin.h"

#include "netbyte.h"
#include "hxnetapi.h"
#include "hxnetutil.h"

#include "hxheap.h"

#include "hxprefs.h"
#include "hxprefutil.h"
#define HELIX_FEATURE_LOGLEVEL_NONE
#include "hxtlogutil.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifdef _WIN16
extern HINSTANCE g_hInstance;       // initialized inside DLLMAIN.CPP(core.dll)
#endif

const int UDP_CHUNK_SIZE = 1024;

#define QUEUE_START_SIZE    512

#if defined _WINDOWS && !defined _WIN32  // for win16
#define DESIRED_RCV_BUFSIZE 0xFFFF  // ~ 525 KBits
#else
#define DESIRED_RCV_BUFSIZE 0x0002FFFF	// ~ 1.5 MBits
#endif //defined _WINDOWS && !defined _WIN32

#ifdef _WINCE
#define SCHED_GRANULARITY 10
#else
#define SCHED_GRANULARITY 50
#endif

// same for all the platforms...may need to tweak it, if necessary
#define MAX_ITERATION_COUNT	200

#if defined( _UNIX )
HXBOOL ReadAsyncDNSPref( IUnknown* pContext )
{
    static HXBOOL bNoAsyncDNS  = FALSE;
    static HXBOOL bNeedToLoad  = TRUE;

    if( bNeedToLoad && NULL!=pContext)
    {
        IHXPreferences* pPreferences = NULL;

        bNeedToLoad = FALSE;
        pContext->QueryInterface(IID_IHXPreferences, (void **)&pPreferences);

        IHXBuffer *pBuffer = NULL;
        if( NULL!=pPreferences )
        {
            pPreferences->ReadPref("NoAsyncDNS", pBuffer);
            if (pBuffer)
            {
                bNoAsyncDNS = (atoi((const char*)pBuffer->GetBuffer()) == 1);
                HX_RELEASE(pBuffer);
            }
            HX_RELEASE( pPreferences );
        } // NULL!=pPreferences
    }//bNeedToLoad && NULL!=pContext

    return bNoAsyncDNS;
}

HXBOOL ReadThreadedDNSPref( IUnknown* pContext )
{
    static HXBOOL bThreadedDNS = TRUE;
    static HXBOOL bNeedToLoad  = TRUE;

    if( bNeedToLoad && NULL != pContext )
    {
        ReadPrefBOOL( pContext, "ThreadedDNS", bThreadedDNS );
        bNeedToLoad = FALSE;
    }

    return bThreadedDNS;
}

#endif

#if !defined(HELIX_CONFIG_NOSTATICS)
UINT16 HXNetworkServices::z_muNumDriverInstance = 0;
#else
#include "globals/hxglobals.h"
const UINT16 HXNetworkServices::z_muNumDriverInstance = 0;
#endif

/* HXNetworkServices */
HXNetworkServices::HXNetworkServices(IUnknown* pContext)
{
    m_bNeedToCleanupDrivers = FALSE;
    m_lRefCount = 0;
    m_pContext	= pContext;

    if (m_pContext)
    {
	m_pContext->AddRef();
    }
}

HXNetworkServices::~HXNetworkServices()
{
    Close();
}

STDMETHODIMP HXNetworkServices::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXNetworkServices), (IHXNetworkServices*)this },
            { GET_IIDHANDLE(IID_IHXNetworkInterfaceEnumerator), (IHXNetworkInterfaceEnumerator*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXNetworkServices*)this },
        };

    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXNetworkServices::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXNetworkServices::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

void
HXNetworkServices::UseDrivers()
{
    if (!m_bNeedToCleanupDrivers)
    {
	m_bNeedToCleanupDrivers = TRUE;

#if defined(HELIX_CONFIG_NOSTATICS)
	UINT16& z_muNumDriverInstance = (UINT16&)
	    HXGlobalInt16::Get(&HXNetworkServices::z_muNumDriverInstance);
#endif
	z_muNumDriverInstance++;
    }
}

void
HXNetworkServices::Close()
{
    HX_RELEASE(m_pContext);

    if (m_bNeedToCleanupDrivers)
    {
	m_bNeedToCleanupDrivers = FALSE;

#if defined(HELIX_CONFIG_NOSTATICS)
	UINT16& z_muNumDriverInstance = (UINT16&)
	    HXGlobalInt16::Get(&HXNetworkServices::z_muNumDriverInstance);
#endif

	z_muNumDriverInstance--;

	if (z_muNumDriverInstance == 0)
	{
#if defined(THREADS_SUPPORTED)
            ThreadEngine::DestroyThreadEngine();
#elif defined(_UNIX_THREADED_NETWORK_IO)
            if( ReadNetworkThreadingPref((IUnknown*)m_pContext) )
                ThreadEngine::DestroyThreadEngine();
#endif
	    conn::close_drivers(NULL);
	}
    }
}

STDMETHODIMP
HXNetworkServices::CreateTCPSocket(IHXTCPSocket** ppTCPSocket)
{
    *ppTCPSocket = new HXTCPSocket(m_pContext, this);
    if (*ppTCPSocket == NULL)
    {
	return HXR_OUTOFMEMORY;
    }

    (*ppTCPSocket)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
HXNetworkServices::CreateUDPSocket(IHXUDPSocket** ppUDPSocket)
{
    *ppUDPSocket = new HXUDPSocket(m_pContext, this);
    if (*ppUDPSocket == NULL)
    {
	return HXR_OUTOFMEMORY;
    }

    (*ppUDPSocket)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
HXNetworkServices::CreateListenSocket(IHXListenSocket** ppListenSocket)
{
    *ppListenSocket = new HXListenSocket(m_pContext, this);
    if (*ppListenSocket == NULL)
    {
	return HXR_OUTOFMEMORY;
    }

    (*ppListenSocket)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
HXNetworkServices::CreateResolver(IHXResolver** ppResolver)
{
    *ppResolver = new HXResolver(this, m_pContext);
    if(*ppResolver == NULL)
    {
	return HXR_OUTOFMEMORY;
    }
    (*ppResolver)->AddRef();
    return HXR_OK;
}



/* IHXNetworkInterfaceEnumerator */
STDMETHODIMP
HXNetworkServices::EnumerateInterfaces
    (REF(UINT32*) pulInterfaces, REF(UINT32) ulNumInterfaces)
{
    return conn::EnumerateInterfaces(pulInterfaces, ulNumInterfaces);
}

/* HXResolver */
HXResolver::HXResolver(HXNetworkServices* pNetworkServices):
     m_lRefCount(0)
    ,m_pCallback(0)
    ,m_pResp(0)
    ,m_bResolverPending(FALSE)
    ,m_pData(0)
    ,m_pNetworkServices(NULL)
{
    m_pNetworkServices = pNetworkServices;
    m_pNetworkServices->AddRef();
}

HXResolver::HXResolver( HXNetworkServices* pNetworkServices,
                          IUnknown*           pContext):
     m_lRefCount(0)
    ,m_pCallback(0)
    ,m_pResp(0)
    ,m_bResolverPending(FALSE)
    ,m_pData(0)
    ,m_pNetworkServices(NULL)
    ,m_pContext( pContext )
{
    m_pNetworkServices = pNetworkServices;
    m_pNetworkServices->AddRef();
}

HXResolver::~HXResolver()
{
    if (m_pData)
    {
	m_pData->done();
	m_pData->Release();
	m_pData = 0;
    }

    if (m_pCallback)
    {
	delete m_pCallback;
	m_pCallback = 0;
    }

    if (m_pResp)
    {
	m_pResp->Release();
	m_pResp = 0;
    }

#if defined( _WIN32 ) || defined( _WINDOWS )
    win_net::ReleaseWinsockUsage(this);
#endif
    HX_RELEASE(m_pNetworkServices);
}

STDMETHODIMP
HXResolver::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXResolver), (IHXResolver*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXResolver*)this },
        };

    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXResolver::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXResolver::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP HXResolver::Init(IHXResolverResponse* pResp)
{
    if (!pResp)
    {
	return HXR_UNEXPECTED;
    }

    if (m_pResp)
    {
	m_pResp->Release();
	m_pResp = 0;
    }

    m_pResp = pResp;
    m_pResp->AddRef();
    return HXR_OK;
}

STDMETHODIMP HXResolver::GetHostByName(const char* pHostName)
{
    HX_RESULT theErr = HXR_OK;

    if (!pHostName || m_bResolverPending)
    {
	return HXR_UNEXPECTED;
    }

    if (!m_pCallback)
    {
	m_pCallback = new HXResolverCallback;
	if (!m_pCallback)
	{
	    return HXR_OUTOFMEMORY;
	}
	m_pCallback->m_pContext = this;
    }

    /* Is this the first time we are calling gethostbyname */
    if (!m_pData)
    {
#if defined( _WIN32 ) || defined( _WINDOWS )
	//	Have we been able to load and initialize the winsock stuff yet?
	if (!win_net::IsWinsockAvailable(this))
	{
	    return HXR_FAIL; // HXR_GENERAL_NONET;
	}
#endif
	m_pNetworkServices->UseDrivers();

	conn::init_drivers(NULL);
    }

    if (m_pData)
    {
	m_pData->done();
	m_pData->Release();
	m_pData = 0;
    }

#ifdef _UNIX
    //This one has to be set before we create a new socket.
    conn::SetNetworkThreadingPref( ReadNetworkThreadingPref((IUnknown*)m_pContext) );
    conn::SetThreadedDNSPref( ReadThreadedDNSPref((IUnknown*)m_pContext) );
#endif

    m_pData = conn::new_socket(m_pContext, HX_UDP_SOCKET);
    if (!m_pData)
    {
	return HXR_OUTOFMEMORY;
    }

#ifdef _UNIX
    m_pData->SetAsyncDNSPref( ReadAsyncDNSPref((IUnknown*)m_pContext) );
#endif

    // XXXAAK -- local addr binding stuff
    theErr = m_pData->init(INADDR_ANY, 0);

    m_pData->nonblocking();
    m_pData->set_callback(m_pCallback);

#ifdef _WINDOWS
#if defined (_WIN32)
    ULONG32 ulPlatformData = (ULONG32)GetModuleHandle(NULL);
#elif defined (_WIN16)
    ULONG32 ulPlatformData = (ULONG32)(int)g_hInstance;
#endif
    m_pData->SetWindowHandle(ulPlatformData);
#endif /* defined (_WINDOWS) */

    m_bResolverPending = TRUE;
#ifndef _WINCE
    m_pData->dns_find_ip_addr(pHostName);
#else
    // Only blocking DNS
    m_pData->dns_find_ip_addr(pHostName, 1);
#endif

    return HXR_OK;
}

void
HXResolver::DNSDone(HXBOOL bSuccess)
{
    ULONG32 ulAddr = 0;
    HXBOOL bIsValid = TRUE;
    char*   pDottedIP = 0;

    m_bResolverPending = FALSE;

    AddRef();

    if (bSuccess)
    {
	m_pData->dns_ip_addr_found(&bIsValid, &ulAddr);
	UINT32 ulHostAddr = DwToHost(ulAddr);
	m_pResp->GetHostByNameDone(HXR_OK, ulHostAddr);
    }
    else
    {
	m_pResp->GetHostByNameDone(HXR_DNR, 0);
    }

    Release();
}


HX_RESULT
HXResolver::HXResolverCallback::Func(NotificationType Type,
					       HXBOOL bSuccess, conn* pConn)
{
    if(m_pContext)
    {
	switch (Type)
	{
	case DNS_NOTIFICATION:
	    m_pContext->DNSDone(bSuccess);
	    break;
	case READ_NOTIFICATION:
	case WRITE_NOTIFICATION:
	case CONNECT_NOTIFICATION:
	default:
	    break;
	}
    }
    return HXR_OK;
}


/* HXTCPSocket */
HXTCPSocket::HXTCPSocket(IUnknown* pContext, HXNetworkServices* pNetworkServices):
     m_lRefCount(0)
    ,m_pTCPResponse(0)
    ,m_pCtrl(0)
    ,m_lForeignAddress(0)
    ,m_nForeignPort(0)
    ,m_bReadPending(FALSE)
    ,m_nRequired(0)
    ,mSendTCP(0)
    ,mReceiveTCP(0)
    ,m_pBuffer(0)
    ,m_pCallback(0)
    ,m_bConnected(FALSE)
    ,m_bWantWritePending(FALSE)
    ,m_bInitComplete(FALSE)
    ,m_bWriteFlushPending(FALSE)
    ,m_pScheduler(0)
    ,m_pSchedulerReadCallback(0)
    ,m_pSchedulerWriteCallback(0)
    ,m_bInRead(FALSE)
    ,m_bInWrite(FALSE)
    ,m_bInDoRead(FALSE)
    ,m_pInterruptState(NULL)
    ,m_pResponseInterruptSafe(NULL)
    ,m_pMutex(NULL)
    ,m_bInDestructor(FALSE)
    ,m_pNonInterruptReadCallback(NULL)
    ,m_pNetworkServices(NULL)
    ,m_pPreferences(NULL)
    ,m_bReuseAddr(FALSE)
    ,m_bReusePort(FALSE)
    ,m_pContext(pContext)
    ,m_bSecureSocket(FALSE)
{
    m_pNetworkServices = pNetworkServices;
    m_pNetworkServices->AddRef();

#ifdef _MACINTOSH
	m_pInterruptSafeMacWriteQueue = new InterruptSafeMacQueue();
	HX_ASSERT(m_pInterruptSafeMacWriteQueue != NULL);
#endif

    if (pContext)
    {
	pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
	pContext->QueryInterface(IID_IHXInterruptState, (void**) &m_pInterruptState);
	pContext->QueryInterface(IID_IHXPreferences, (void**) &m_pPreferences);
    }

    if (m_pScheduler)
    {
	m_pSchedulerReadCallback = new ScheduledSocketCallback(this, TRUE);
	m_pSchedulerReadCallback->AddRef();

	m_pSchedulerWriteCallback = new ScheduledSocketCallback(this, TRUE);
	m_pSchedulerWriteCallback->AddRef();

	m_pNonInterruptReadCallback = new ScheduledSocketCallback(this, FALSE);
	m_pNonInterruptReadCallback->AddRef();

#ifdef _MACINTOSH
	m_pMacCommandCallback = new ScheduledSocketCallback(this, FALSE);
	m_pMacCommandCallback->AddRef();
#endif
    }

#if defined(_UNIX) && defined(HELIX_FEATURE_IGNORE_SIGPIPE)
        // When the connection is closed by the server, SIGPIPE will be thrown
        // in next write() and terminates the program abruptly.
        //
        // In order to gracefully exists the program, it's recommended to:
        // - ignore the SIGPIPE and
        // - checks the return code(errno) from write()
        //
        // for read(), it simply returns 0 when the connection is closed.
        SIGNAL(SIGPIPE, SIG_IGN);
#endif /* _UNIX  && HELIX_FEATURE_IGNORE_SIGPIPE */

    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);  
}

HXTCPSocket::~HXTCPSocket()
{
    m_bInDestructor = TRUE; // set it early
    m_pMutex->Lock();

#ifdef _MACINTOSH
	HX_DELETE(m_pInterruptSafeMacWriteQueue); // will release any objects in its nodes
#endif

   if (m_pSchedulerReadCallback)
		m_pSchedulerReadCallback->Unschedule(m_pScheduler);

   if (m_pSchedulerWriteCallback)
		m_pSchedulerWriteCallback->Unschedule(m_pScheduler);

   if (m_pNonInterruptReadCallback)
		m_pNonInterruptReadCallback->Unschedule(m_pScheduler);

#ifdef _MACINTOSH
	if (m_pMacCommandCallback)
		m_pMacCommandCallback->Unschedule(m_pScheduler);
#endif

    /*
     * XXX...While handling the m_pCtrl->done it's possible for the
     *       DispatchMessage call in CancelSelect to cause an
     *       asynchronous DoRead to occur. The resulting AddRef/Release
     *       would cause this object to be deleted again, so to prevent
     *       this we set the m_pCallback->m_pContext = 0
     */

   if (m_pCallback)
    {
	m_pCallback->m_pContext = 0;
    }

//#ifndef _UNIX
    /* XXXJR I feel certain this is related to the above comment somehow.
     *       Deleting the ctrl here wreaks havoc on the encoder.
     *       This is a bad solution, but I don't really know
     *       what the right one is.  This at least prevents random crashes
     *       in the encoder.
     *
     * XXXGH commented out the #ifndef because it was breaking my
     *       connectionless control stuff
     *
     */
    if (m_pCtrl)
    {
	m_pCtrl->done();
	m_pCtrl->Release(); // A deleted (0xdddddddd) pointer was used here.
	m_pCtrl = 0;
    }
//#endif

    HX_RELEASE(m_pTCPResponse);
    HX_DELETE(m_pCallback);
    HX_DELETE(mSendTCP);
    HX_DELETE(mReceiveTCP);
    HX_VECTOR_DELETE(m_pBuffer);

    while (m_PendingWriteBuffers.GetCount() > 0)
    {
	IHXBuffer* pBuffer =
		    (IHXBuffer*) m_PendingWriteBuffers.RemoveHead();
	pBuffer->Release();
    }

    if (m_pSchedulerReadCallback)
    {
    	m_pSchedulerReadCallback->m_pSocket = NULL;
    	m_pSchedulerReadCallback->Release();
    	m_pSchedulerReadCallback = NULL;
    }

    if (m_pSchedulerWriteCallback)
    {
    	m_pSchedulerWriteCallback->m_pSocket = NULL;
    	m_pSchedulerWriteCallback->Release();
    	m_pSchedulerWriteCallback = NULL;
    }

    if (m_pNonInterruptReadCallback)
    {
    	m_pNonInterruptReadCallback->m_pSocket = NULL;
    	m_pNonInterruptReadCallback->Release();
    	m_pNonInterruptReadCallback = NULL;
    }

#ifdef _MACINTOSH
   if (m_pMacCommandCallback)
    {
		m_pMacCommandCallback->m_pSocket = NULL;
		m_pMacCommandCallback->Release();
		m_pMacCommandCallback = NULL;
    }
#endif

    HX_RELEASE(m_pInterruptState);
    HX_RELEASE(m_pResponseInterruptSafe);
    HX_RELEASE(m_pScheduler);

    m_pMutex->Unlock();
    HX_RELEASE(m_pMutex);

#if defined( _WIN32 ) || defined( _WINDOWS )
    win_net::ReleaseWinsockUsage(this);
#endif

    HX_RELEASE(m_pNetworkServices);
    HX_RELEASE(m_pPreferences);
}

STDMETHODIMP HXTCPSocket::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXTCPSocket), (IHXTCPSocket*)this },
            { GET_IIDHANDLE(IID_IHXSetSocketOption), (IHXSetSocketOption*)this },
            { GET_IIDHANDLE(IID_IHXTCPSecureSocket), (IHXTCPSecureSocket*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXTCPSocket*)this },
        };

    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXTCPSocket::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXTCPSocket::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    else if (m_lRefCount < 0)
    {
    	// double delete
    	return 0;
    }

    delete this;
    return 0;
}

STDMETHODIMP HXTCPSocket::Init(IHXTCPResponse* pTCPResponse)
{
    if (!pTCPResponse)
    {
	return HXR_UNEXPECTED;
    }

    m_pTCPResponse = pTCPResponse;
    m_pTCPResponse->AddRef();

    m_pTCPResponse->QueryInterface(IID_IHXInterruptSafe,
				   (void**) &m_pResponseInterruptSafe);

    // allocate TCP send and receive queue
    mSendTCP = new CByteGrowingQueue(QUEUE_START_SIZE,1);
    if (!mSendTCP || !mSendTCP->IsQueueValid())
    {
	return HXR_OUTOFMEMORY;
    }
    mSendTCP->SetMaxSize(TCP_BUF_SIZE);

    mReceiveTCP = new CByteGrowingQueue(QUEUE_START_SIZE,1);
    if (!mReceiveTCP || !mReceiveTCP->IsQueueValid())
    {
	return HXR_OUTOFMEMORY;
    }
    mReceiveTCP->SetMaxSize(TCP_BUF_SIZE);

    m_pBuffer = new char[TCP_BUF_SIZE];
    if (!m_pBuffer)
    {
	return HXR_OUTOFMEMORY;
    }

    return HXR_OK;
}

STDMETHODIMP HXTCPSocket::SetResponse(IHXTCPResponse* pTCPResponse)
{
    m_pMutex->Lock();
    HX_RELEASE(m_pTCPResponse);
    m_pTCPResponse = pTCPResponse;
    m_pTCPResponse->AddRef();

    HX_RELEASE(m_pResponseInterruptSafe);
    m_pTCPResponse->QueryInterface(IID_IHXInterruptSafe,
				   (void**) &m_pResponseInterruptSafe);
    m_pMutex->Unlock();
    return HXR_OK;
}

STDMETHODIMP HXTCPSocket::Bind(UINT32 ulLocalAddr, UINT16 nPort)
{
    UINT32	ulMaxBandwidth = 0;
    HXBOOL	bEnforceMaxBandwidth = TRUE;
    HXBOOL        bLoadTest = FALSE;
    IHXBuffer* pBuffer = NULL;

    if (m_bInitComplete)
	return HXR_UNEXPECTED;

    m_nLocalPort = nPort;

#if defined( _WIN32 ) || defined( _WINDOWS )
    //	Have we been able to load and initialize the winsock stuff yet?
    if (!win_net::IsWinsockAvailable(this))
    {
	return HXR_FAIL; // HXR_GENERAL_NONET;
    }
#endif

    m_pNetworkServices->UseDrivers();

    HX_RESULT theErr = conn::init_drivers(NULL);

#ifdef _UNIX
    //This one has to be set before we create a new socket.
    conn::SetNetworkThreadingPref( ReadNetworkThreadingPref((IUnknown*)m_pContext) );
    conn::SetThreadedDNSPref( ReadThreadedDNSPref((IUnknown*)m_pContext) );
#endif


    m_pCtrl = NULL;

#if defined(HELIX_FEATURE_SECURECONN)
    if (m_bSecureSocket)
    {
	IHXSSL* pHXSSL = NULL;
	IHXCommonClassFactory* pCCF = NULL;

	if (m_pContext)
	{
	    m_pContext->AddRef();

	    // get the CCF
	    m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&pCCF);

	    HX_RELEASE(m_pContext);
	}

	if (pCCF)
	{
	    pCCF->CreateInstance(IID_IHXSSL, (void**) &pHXSSL);
	    HX_RELEASE(pCCF);
	}

	if (pHXSSL)
	{
	    m_pCtrl = new secureconn(m_pContext, pHXSSL);
	    pHXSSL->Release();
	}
    }
    else
#endif /* HELIX_FEATURE_SECURECONN */
    {
	m_pCtrl  = conn::new_socket(m_pContext, HX_TCP_SOCKET);
    }


    if (!m_pCtrl)
    {
	return HXR_OUTOFMEMORY;
    }

    // XXXGo - As it is implemented, this is the only way...
    if (m_bReuseAddr)
    {
	if (m_pCtrl->reuse_addr(m_bReuseAddr) != HXR_OK)
	{
	    // err...what do we need to do?
	    HX_ASSERT(!"reuse_addr() failed");
	}
    }
    if (m_bReusePort)
    {
	if (m_pCtrl->reuse_port(m_bReusePort) != HXR_OK)
	{
	    // err...what do we need to do?
	    HX_ASSERT(!"reuse_port() failed");
	}
    }

#ifdef _UNIX
    m_pCtrl->SetAsyncDNSPref( ReadAsyncDNSPref((IUnknown*)m_pContext) );
#endif

    m_pCtrl->nonblocking();

    m_pCallback = new TCPSocketCallback;
    if (!m_pCallback)
    {
	return HXR_OUTOFMEMORY;
    }
    m_pCallback->m_pContext = this;
    m_pCtrl->set_callback(m_pCallback);
    m_bInitComplete = TRUE;

    if (m_pPreferences)
    {
	/* Get MaxBandwidth from Prefs */
	ReadPrefUINT32(m_pPreferences, "MaxBandwidth", ulMaxBandwidth);
	ReadPrefBOOL(m_pPreferences, "LoadTest", bLoadTest);
	ReadPrefBOOL(m_pPreferences, "EnforceMaxBandwidth", bEnforceMaxBandwidth);

	//If we are in load test mode, never enforce the MaxBandwidth.
	bEnforceMaxBandwidth = bEnforceMaxBandwidth&&!bLoadTest;

	if (ulMaxBandwidth && bEnforceMaxBandwidth)
	{
	    conn::m_ulMaxBandwidth = ulMaxBandwidth / 8;
	}
	else if (!bEnforceMaxBandwidth)
	{
	    conn::m_ulMaxBandwidth = MAX_UINT32;
	}
    }

    return HXR_OK;
}

STDMETHODIMP HXTCPSocket::Connect(const char*	pDestination,
				   UINT16	nPort)
{
    if (!m_bInitComplete)
    {
	HX_RESULT ret = Bind(HXR_INADDR_ANY, 0);
	if (HXR_OK != ret)
		return ret;
    }

    HX_RESULT	theErr		= HXR_OK;
    UINT32	ulPlatformData	= 0;

#if defined (_WIN32)
    ulPlatformData = (UINT32)GetModuleHandle(NULL);
#elif defined (_WIN16)
    ulPlatformData = (UINT32)(int)g_hInstance;
#endif

    m_nForeignPort = nPort;

#ifndef _WINCE
    theErr = m_pCtrl->connect(pDestination,nPort,0,ulPlatformData);
#else
    theErr = m_pCtrl->connect(pDestination,nPort,1,ulPlatformData);
#endif

    theErr = ConvertNetworkError(theErr);
    return theErr;
}

STDMETHODIMP HXTCPSocket::Read(UINT16 uSize)
{
    HX_RESULT	theErr	= HXR_OK;
    HX_RESULT	lResult = HXR_OK;

    if (m_bReadPending)
    {
	return HXR_UNEXPECTED;
    }

    m_bReadPending  = TRUE;
    m_nRequired	    = uSize;

    m_pMutex->Lock();
    theErr  = DoRead();
    m_pMutex->Unlock();
    lResult = ConvertNetworkError(theErr);

    return lResult;
}

STDMETHODIMP HXTCPSocket::Write(IHXBuffer* pBuffer)
{
    HX_RESULT	theErr	= HXR_OK;
    HX_RESULT	lResult = HXR_OK;

#ifdef _MACINTOSH
	if (m_pInterruptSafeMacWriteQueue)
		m_pInterruptSafeMacWriteQueue->AddTail(pBuffer); // AddRef called inside
#else
	pBuffer->AddRef();
	m_PendingWriteBuffers.AddTail((void*) pBuffer);
    /* Transfer pending buffers to TCP send queue */
    TransferBuffers();
#endif

	m_pMutex->Lock();
    theErr = DoWrite();
    m_pMutex->Unlock();
    lResult = ConvertNetworkError(theErr);

    return lResult;
}

STDMETHODIMP HXTCPSocket::WantWrite()
{
    if (mSendTCP->GetQueuedItemCount() == 0)
    {
	m_pTCPResponse->WriteReady(HXR_OK);
    }
    else
    {
	m_bWantWritePending = TRUE;
    }

    return HXR_OK;
}

STDMETHODIMP HXTCPSocket::GetLocalAddress(ULONG32& lAddress)
{
    HX_RESULT retVal = HXR_FAIL;
    
    if(m_bConnected)
    {
        lAddress = (127 << 24) | 1;
        retVal   = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP HXTCPSocket::GetForeignAddress(ULONG32& lAddress)
{
    if(m_bConnected && m_lForeignAddress)
    {
	lAddress = m_lForeignAddress;
	return HXR_OK;
    }
    return HXR_FAIL;
}

STDMETHODIMP HXTCPSocket::GetLocalPort(UINT16& port)
{
    HX_RESULT retVal = HXR_FAIL;
    
    if (m_bConnected)
    {
        port   = m_nLocalPort;
        retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP HXTCPSocket::GetForeignPort(UINT16& port)
{
    if(m_bConnected)
    {
	port = m_nForeignPort;
	return HXR_OK;
    }
    return HXR_FAIL;
}

// the tcp socket will still need to be inited
STDMETHODIMP
HXTCPSocket::AcceptConnection(conn* pNewConn)
{
    HX_ASSERT(!m_bConnected);
    HX_ASSERT(!m_bInitComplete);
    HX_ASSERT(m_pCtrl == NULL);
    m_pCtrl = pNewConn;
    m_pCtrl->AddRef();
    m_pCtrl->nonblocking();

    if ( m_pCallback )
    {
	HX_DELETE(m_pCallback);
    }
    m_pCallback = new TCPSocketCallback;
    if ( !m_pCallback )
    {
	return HXR_OUTOFMEMORY;
    }
    m_pCallback->m_pContext = this;
    m_pCtrl->set_callback(m_pCallback);
    m_lForeignAddress = DwToHost(m_pCtrl->get_addr());
    m_nLocalPort = WToHost(m_pCtrl->get_local_port());

    m_bInitComplete = TRUE;
    m_bConnected = TRUE;

    return HXR_OK;
}

HX_RESULT
HXTCPSocket::DoRead()
{
#ifdef _MACINTOSH
	if (m_bInDoRead)
	{
		return HXR_OK; // whatever needs to be done will be done by the caller that's already here.

		// xxxbobclark the m_bInDoRead flag is hacked around calling ReadDone(), because
		//             ReadDone() may call Read() which in turn calls us here, and we do
		//             not want to bail out in that instance. (Otherwise we only remove
		//             one packet at a time, which, given the scheduler granularity and
		//             high bit rates, implies that our bandwidth would be too low.)
	}
#endif
	m_bInDoRead = TRUE;

    HX_RESULT theErr = HXR_OK;
    // check how much room we have in TCP receive queue
    UINT16 count = mReceiveTCP->GetMaxAvailableElements();

    if (count > 0)
    {
#if !defined(THREADS_SUPPORTED) && !defined(_UNIX_THREADED_NETWORK_IO)
	UINT32 ulBytesToRead = conn::bytes_to_preparetcpread(m_pCtrl);

	if (ulBytesToRead > 0)
	{
	    if ((UINT32)count > ulBytesToRead)
	    {
		count = (UINT16)ulBytesToRead;
	    }

	    // attempt to read data from TCP link
	    theErr = m_pCtrl->read(m_pBuffer, &count);
	    if (!theErr && count > 0)
	    {
		conn::bytes_to_actualtcpread(m_pCtrl, (UINT32)count);
		mReceiveTCP->EnQueue(m_pBuffer, count);
	    }
	    else if (theErr)
	    {
		theErr = ConvertNetworkError(theErr);
	    }
	}
#elif defined(_UNIX_THREADED_NETWORK_IO)
        //XXXgfw duplicated code. Clean this up...
        if( ReadNetworkThreadingPref((IUnknown*)m_pContext) )
        {
            // in THREADS_SUPPORTED mode, this will be taken care by the thrdconn.cpp
            // attempt to read data from TCP link
            theErr = m_pCtrl->read(m_pBuffer, &count);
            if (!theErr && count > 0)
            {
                mReceiveTCP->EnQueue(m_pBuffer, count);
            }
            else if (theErr)
            {
                theErr = ConvertNetworkError(theErr);
            }
        }
        else
        {
            UINT32 ulBytesToRead = conn::bytes_to_preparetcpread(m_pCtrl);

            if (ulBytesToRead > 0)
            {
                if ((UINT32)count > ulBytesToRead)
                {
                    count = (UINT16)ulBytesToRead;
                }

                // attempt to read data from TCP link
                theErr = m_pCtrl->read(m_pBuffer, &count);
                if (!theErr && count > 0)
                {
                    conn::bytes_to_actualtcpread(m_pCtrl, (UINT32)count);
                    mReceiveTCP->EnQueue(m_pBuffer, count);
                }
                else if (theErr)
                {
                    theErr = ConvertNetworkError(theErr);
                }
            }

        }
#else
	// in THREADS_SUPPORTED mode, this will be taken care by the thrdconn.cpp
	// attempt to read data from TCP link
	theErr = m_pCtrl->read(m_pBuffer, &count);
	if (!theErr && count > 0)
	{
	    mReceiveTCP->EnQueue(m_pBuffer, count);
	}
	else if (theErr)
	{
	    theErr = ConvertNetworkError(theErr);
	}
#endif /* !THREADS_SUPPORTED */
    }

    count = mReceiveTCP->GetQueuedItemCount();
    if (m_bReadPending && count > 0)
    {
	/* If we are at interrupt time and the response object is not interrupt safe,
	 * schedule a callback to return back the data
	 */
	if (!IsSafe())
	{
		m_bInDoRead = FALSE;
	    return HXR_OK;
	}

	m_bReadPending = FALSE;
	if (m_nRequired < count)
	{
	    // XXXAAK -- UINT32 down to UINT16 - possible truncation???
	    count = (UINT16)m_nRequired;
	}

	IHXBuffer* pBuffer = NULL;
	if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
	{
	    mReceiveTCP->DeQueue(m_pBuffer, count);
	    pBuffer->Set((UCHAR*) m_pBuffer, count);

	    m_bInDoRead = FALSE;
	    theErr = m_pTCPResponse->ReadDone(HXR_OK, pBuffer);
	    m_bInDoRead = TRUE;

	    HX_RELEASE(pBuffer);
	}

	/* mask any kind of errors */
        // Huh??? Don't mask OUTOFMEMORY errors!
        if( theErr != HXR_OUTOFMEMORY )
        {
	    theErr = HXR_OK;
        }
    }

    if (theErr && m_bReadPending)
    {
	/* If we are at interrupt time and the response object is not interrupt safe,
	 * schedule a callback to return back the data
	 */
	if (!IsSafe())
	{
		m_bInDoRead = FALSE;
	    return HXR_OK;
	}

#ifdef _MACINTOSH
	if (m_pMacCommandCallback && m_pMacCommandCallback->ScheduleCallback(TCP_READ_DONE_COMMAND, m_pScheduler, 0, theErr))
	{
	    m_bReadPending = FALSE;
    	    m_bInDoRead = FALSE;
	    return HXR_OK;
	}
	else
	{
	    // failed to schedule a callback, notify the responser with error directly
	    m_bReadPending = FALSE;
	    m_pTCPResponse->ReadDone(theErr, NULL);
	}
#else
	m_bReadPending = FALSE;
	m_pTCPResponse->ReadDone(theErr, NULL);
#endif
    }

    if (!theErr		    &&
	m_bReadPending	    &&
	m_pSchedulerReadCallback)
	{
	    m_pSchedulerReadCallback->ScheduleCallback(TCP_READ_COMMAND, m_pScheduler, SCHED_GRANULARITY);
    }

 	m_bInDoRead = FALSE;
    return theErr;
}

HX_RESULT
HXTCPSocket::DoWrite()
{
    HX_RESULT theErr = HXR_OK;

    if (m_bInWrite) return HXR_OK;
    m_bInWrite = TRUE;

#ifdef _MACINTOSH
	if (m_pInterruptSafeMacWriteQueue)
		m_pInterruptSafeMacWriteQueue->TransferToSimpleList(m_PendingWriteBuffers);
	TransferBuffers(); // PENDING_BUFFERS_ARE_EMPTIED_AT_START_OF_DO_WRITE
#endif

    // check how data we have in TCP send queue
    UINT16 count    = mSendTCP->GetQueuedItemCount();
    UINT16 actual   = count;

    if(count > 0)
    {
	mSendTCP->DeQueue(m_pBuffer,count);
	theErr = m_pCtrl->write(m_pBuffer, &actual);
    }

    switch(theErr)
    {
	case HXR_AT_INTERRUPT:
	case HXR_WOULD_BLOCK:
	case HXR_OK:
	    // enqueue the data that was not sent
	    if(actual != count)
	    {
		mSendTCP->EnQueue(m_pBuffer + actual, count - actual);
	    }

	    // mask out these errors
	    theErr = HXR_OK;
	    break;
	default:
	    theErr = ConvertNetworkError(theErr);
	    break;
    }

    if (!theErr && m_bWantWritePending && mSendTCP->GetQueuedItemCount() == 0)
    {
	m_bWantWritePending = FALSE;
	m_pTCPResponse->WriteReady(HXR_OK);
    }

#ifndef _MACINTOSH
	// m_PendingWriteBuffers will always be empty due to the full buffer transfer at the top of this routine.
	// see PENDING_BUFFERS_ARE_EMPTIED_AT_START_OF_DO_WRITE
    if (!theErr && m_PendingWriteBuffers.GetCount()  > 0)
    {
	TransferBuffers();
    }
#endif

    if (!theErr &&
	((mSendTCP && mSendTCP->GetQueuedItemCount() > 0) ||
	m_PendingWriteBuffers.GetCount() > 0)) // see PENDING_BUFFERS_ARE_EMPTIED_AT_START_OF_DO_WRITE
    {
	if (m_pSchedulerWriteCallback)
	{
	    m_pSchedulerWriteCallback->ScheduleCallback(TCP_WRITE_COMMAND, m_pScheduler, SCHED_GRANULARITY);
	}
    }

    if (m_bWriteFlushPending &&
	((mSendTCP->GetQueuedItemCount() == 0 &&
	 m_PendingWriteBuffers.GetCount() == 0)	||
	 theErr))
    {
	m_bWriteFlushPending	= FALSE;
	Release();
    }
    else if (!theErr && !m_bWriteFlushPending &&
	(mSendTCP->GetQueuedItemCount() > 0 || m_PendingWriteBuffers.GetCount() > 0))
    {
	m_bWriteFlushPending	= TRUE;
	AddRef();
    }

    m_bInWrite = FALSE;
    return theErr;
}

/* If we are at interrupt time and the response object is not interrupt safe,
 * schedule a callback to return back the data
 */
HXBOOL
HXTCPSocket::IsSafe()
{
    if (m_pInterruptState && m_pInterruptState->AtInterruptTime() &&
	(!m_pResponseInterruptSafe ||
	 !m_pResponseInterruptSafe->IsInterruptSafe()))
    {
	if (m_pNonInterruptReadCallback)
	{
	    m_pNonInterruptReadCallback->ScheduleCallback(TCP_READ_COMMAND, m_pScheduler, 0);
	}

	return FALSE;
    }

    return TRUE;
}


void
HXTCPSocket::ConnectDone(HXBOOL bResult)
{
    AddRef();
    if (bResult == TRUE)
    {
	m_bConnected = TRUE;
	//XXX need to set m_lForeignAddr here
	//XXXJR hack!
	m_lForeignAddress = DwToHost(m_pCtrl->get_addr());
	m_pTCPResponse->ConnectDone(HXR_OK);
    }
    else
    {
#ifdef _MACINTOSH
	if (!(m_pMacCommandCallback && m_pMacCommandCallback->ScheduleCallback(TCP_CONNECT_DONE_COMMAND, m_pScheduler, 0, HXR_NET_CONNECT)))
	{
		//note: only happens when there's a problem (e.g. macleod 1/2 server problem)
		m_pTCPResponse->ConnectDone(HXR_NET_CONNECT); // couldn't use the delayed callback... take our chances.
	}
#else
	m_pTCPResponse->ConnectDone(HXR_NET_CONNECT);
#endif
    }
    Release();
}

void
HXTCPSocket::CloseDone()
{
    m_pTCPResponse->Closed(HXR_OK);
}

void
HXTCPSocket::DNSDone(HXBOOL bSuccess)
{
    AddRef();
    if (!bSuccess)
    {
	m_pTCPResponse->ConnectDone(HXR_DNR);
    }
    Release();
}

void
HXTCPSocket::TransferBuffers()
{
    IHXBuffer* pBuffer = 0;

    while (m_PendingWriteBuffers.GetCount() > 0)
    {
	pBuffer = (IHXBuffer*) m_PendingWriteBuffers.GetHead();
	if ((UINT16) pBuffer->GetSize() < mSendTCP->GetMaxAvailableElements())
	{
	    mSendTCP->EnQueue(	pBuffer->GetBuffer(),
				(UINT16) pBuffer->GetSize());
	    pBuffer->Release();
	    m_PendingWriteBuffers.RemoveHead();
	}
	else
	{
	    break;
	}
    }
}

STDMETHODIMP
HXTCPSocket::SetOption(HX_SOCKET_OPTION option, UINT32 ulValue)
{
    HX_RESULT res = HXR_OK;

    switch(option)
    {
    case HX_SOCKOPT_REUSE_ADDR:
	m_bReuseAddr = (HXBOOL)ulValue;
    break;
    case HX_SOCKOPT_REUSE_PORT:
	m_bReusePort = (HXBOOL)ulValue;
    break;
    case HX_SOCKOPT_MULTICAST_IF:
	res = HXR_UNEXPECTED;
    break;
    default:
	HX_ASSERT(!"I don't know this option");
	res = HXR_FAIL;
    }

    return res;
}

STDMETHODIMP
HXTCPSocket::SetSecure(HXBOOL bSecure)
{
    HX_RESULT res = HXR_OK;

    m_bSecureSocket = bSecure;

    return res;
}

STDMETHODIMP HXTCPSocket::HandleCallback(INT32 theCommand, HX_RESULT theError)
{
    HX_RESULT theErr = HXR_OK;
    if (!m_bInDestructor)
    {
	AddRef();
	m_pMutex->Lock();
	if (!m_bInDestructor)
	{
		switch(theCommand)
		{
			case TCP_READ_COMMAND:
			    theErr = DoRead();
			    break;

			case TCP_WRITE_COMMAND:
			    DoWrite(); // protected from re-entry by m_bInWrite
			    break;

			case TCP_READ_DONE_COMMAND:
		    	m_bReadPending = FALSE;
				m_pTCPResponse->ReadDone(theError, NULL);
			    break;

			case TCP_CONNECT_DONE_COMMAND:
				m_pTCPResponse->ConnectDone(theError);
				break;

			case TCP_BIND_COMMAND:
			default:
			    theErr = DoRead();
			    DoWrite();
			    break;
	   	}
	}
	m_pMutex->Unlock();

        // we want out of memory errors to be reported immediately
        // because fiddling around waiting for the error to propagate
        // normally will just make the situation worse; mask out all
        // other errors, as they will eventually get dealt with in
        // ReadDone() or similar functions.
        
        if (theErr != HXR_OUTOFMEMORY)
        {
            theErr = HXR_OK;
        }
        
	if( theErr )
	{
	    IHXErrorMessages * pErrorNotifier = NULL;
	    IUnknown * pPlayer = NULL;
	    IHXClientEngine* pEngine = NULL;
	    UINT32 nNumPlayers = 0;

	    m_pContext->QueryInterface(IID_IHXClientEngine, (void**)&pEngine);
	    if( pEngine )
	    {
		nNumPlayers = pEngine->GetPlayerCount();
		for( int ii=0; ii<nNumPlayers; ii++ )
		{
		    pEngine->GetPlayer(ii,pPlayer);
		    if( pPlayer )
		    {
			pPlayer->QueryInterface( IID_IHXErrorMessages, (void**)&pErrorNotifier );
		    }
		    if( pErrorNotifier )
		    {
			pErrorNotifier->Report( HXLOG_ERR, theErr, 0, NULL, NULL );
			pErrorNotifier->Release();
		    }
		    HX_RELEASE( pPlayer );
		}
	    }
	    HX_RELEASE( pEngine );
	}
	Release();
    }
    return theErr;
}

HX_RESULT HXTCPSocket::TCPSocketCallback::Func(NotificationType Type,
						   HXBOOL bSuccess, conn* pConn)
{
    if(m_pContext)
    {

	switch (Type)
	{
	case READ_NOTIFICATION:
//
//  This clears up a problem on the Macintosh where we were getting
//  interrupt callbacks from the Network device, and could possibly
//  collide when adding/removing data the same time from the same
//  socket, at interrupt time, and at system level time.
//
#if defined(_UNIX_THREADED_NETWORK_IO)
            if( !ReadNetworkThreadingPref((IUnknown*)(m_pContext->m_pContext) ))
            {
                m_pContext->AddRef();
                m_pContext->m_pMutex->Lock();
                m_pContext->DoRead();
                m_pContext->m_pMutex->Unlock();
                m_pContext->Release();
            }
#elif !defined (THREADS_SUPPORTED) && !defined(_MACINTOSH)
	    m_pContext->AddRef();
	    m_pContext->m_pMutex->Lock();
	    m_pContext->DoRead();
	    m_pContext->m_pMutex->Unlock();
	    m_pContext->Release();
#endif
	    break;
	case WRITE_NOTIFICATION:
#if defined(_UNIX_THREADED_NETWORK_IO)
            if( !ReadNetworkThreadingPref((IUnknown*)(m_pContext->m_pContext)) )
            {
                m_pContext->AddRef();
                m_pContext->m_pMutex->Lock();
                m_pContext->DoWrite();
                m_pContext->m_pMutex->Unlock();
                m_pContext->Release();
            }
#elif !defined (THREADS_SUPPORTED) && !defined(_MACINTOSH)
	    m_pContext->AddRef();
	    m_pContext->m_pMutex->Lock();
	    m_pContext->DoWrite();
	    m_pContext->m_pMutex->Unlock();
	    m_pContext->Release();
#endif
	    break;
	case CONNECT_NOTIFICATION:
	    m_pContext->ConnectDone(bSuccess);
	    break;
	case CLOSE_NOTIFICATION:
	    m_pContext->CloseDone();
	    break;
	case DNS_NOTIFICATION:
	    m_pContext->DNSDone(bSuccess);
	    break;
	default:
	    break;
	}
    }
    return HXR_OK;
}

HXListenSocket::HXListenSocket(IUnknown* pContext,
				 HXNetworkServices* pNetworkServices)
: m_pListenConn(NULL)
, m_pCallback(NULL)
, m_pContext(NULL)
  , m_bReuseAddr(FALSE)
  , m_bReusePort(FALSE)
{
    m_pContext = pContext;
    m_pContext->AddRef();

    m_pNetworkServices = pNetworkServices;
    m_pNetworkServices->AddRef();

    m_lRefCount = 0;
    m_pListenResponse = 0;

}

HXListenSocket::~HXListenSocket()
{
    if (m_pCallback)
    {
	m_pCallback->m_pContext = 0;
    }

    if (m_pListenConn)
    {
	m_pListenConn->done();
        m_pListenConn->Release();
        m_pListenConn = NULL;
    }

    HX_RELEASE(m_pContext);
    HX_DELETE(m_pCallback);

    HX_RELEASE(m_pListenResponse);
    HX_RELEASE(m_pNetworkServices);
}

STDMETHODIMP HXListenSocket::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXListenSocket), (IHXListenSocket*)this },
            { GET_IIDHANDLE(IID_IHXSetSocketOption), (IHXSetSocketOption*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXListenSocket*)this },
        };

    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXListenSocket::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXListenSocket::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return HXR_OK;
}

STDMETHODIMP HXListenSocket::SetOption(HX_SOCKET_OPTION option,
					UINT32 ulValue)
{
    HX_RESULT  ret = HXR_OK;

    switch (option)
    {
    case HX_SOCKOPT_REUSE_ADDR:
	m_bReuseAddr = ulValue;
	break;

    case HX_SOCKOPT_REUSE_PORT:
	m_bReusePort = ulValue;
	break;

    default:
	ret = HXR_NOTIMPL;
    }

    return ret;
}

STDMETHODIMP HXListenSocket::Init(UINT32 ulLocalAddr, UINT16 port,
				   IHXListenResponse* pListenResponse)
{
    if (!pListenResponse)
    {
	return HXR_UNEXPECTED;
    }

    HX_RELEASE(m_pListenResponse);
    m_pListenResponse = pListenResponse;
    m_pListenResponse->AddRef();

#if defined( _WIN32 ) || defined( _WINDOWS )
    //	Have we been able to load and initialize the winsock stuff yet?
    if (!win_net::IsWinsockAvailable(this))
    {
	return HXR_FAIL; // HXR_GENERAL_NONET;
    }
#endif

    m_pNetworkServices->UseDrivers();

#ifdef _UNIX
    //This one has to be set before we create a new socket.
    conn::SetNetworkThreadingPref( ReadNetworkThreadingPref((IUnknown*)m_pContext) );
    conn::SetThreadedDNSPref( ReadThreadedDNSPref((IUnknown*)m_pContext) );
#endif

    HX_RESULT ret = conn::init_drivers(NULL);
    m_pListenConn = conn::new_socket(m_pContext, HX_TCP_SOCKET);
    if ( m_pListenConn == NULL )
    {
	return HXR_OUTOFMEMORY;
    }

#ifdef _UNIX
    m_pListenConn->SetAsyncDNSPref( ReadAsyncDNSPref((IUnknown*)m_pContext) );
#endif

    m_pListenConn->nonblocking();
    m_pListenConn->reuse_addr(m_bReuseAddr);
    m_pListenConn->reuse_port(m_bReusePort);

    if ( m_pCallback == NULL)
    {
	m_pCallback = new ListenSocketCallback();
	m_pCallback->m_pContext = this;
    }
    m_pListenConn->set_callback(m_pCallback);

    UINT32	ulPlatformData	= 0;

#if defined (_WIN32)
    ulPlatformData = (UINT32)GetModuleHandle(NULL);
#elif defined (_WIN16)
    ulPlatformData = (UINT32)(int)g_hInstance;
#endif
    return m_pListenConn->listen(ulLocalAddr, port, 2, 0, ulPlatformData);
}


HX_RESULT HXListenSocket::ListenSocketCallback::Func(NotificationType Type,
						   HXBOOL bSuccess, conn* pConn)
{
    if(m_pContext)
    {
	switch (Type)
	{
	case ACCEPT_NOTIFICATION:

	    if ( bSuccess )
	    {
		HXTCPSocket* pSock = new HXTCPSocket(m_pContext->m_pContext,
		    m_pContext->m_pNetworkServices);
	       	if ( pSock )
		{
		    pSock->AddRef();
		    if ( SUCCEEDED(pSock->AcceptConnection(pConn)) )
		    {
			m_pContext->m_pListenResponse->NewConnection(HXR_OK,
			    (IHXTCPSocket*)pSock);
		    }
		    HX_RELEASE(pSock);
		}
	    }
	    break;
	case CONNECT_NOTIFICATION:
	    break;
	case READ_NOTIFICATION:
	    break;
	case CLOSE_NOTIFICATION:
	    break;
	case DNS_NOTIFICATION:
	default:
	    break;
	}
    }
    return HXR_OK;
}


HXUDPSocket::HXUDPSocket(IUnknown* pContext, HXNetworkServices* pNetworkServices):
    m_lRefCount(0),
    m_pCallback(0),
    m_pUDPResponse(0),
    m_pData(0),
    m_bReadPending(FALSE),
    m_bInRead(FALSE),
    m_bInDoRead(FALSE),
    m_bInWrite(FALSE),
    m_nRequired(0),
    m_pSchedulerReadCallback(NULL),
    m_pSchedulerWriteCallback(NULL),
    m_pNonInterruptReadCallback(NULL),
    m_pScheduler(0),
    m_nDestPort(0),
    m_bInitComplete(FALSE),
    m_pInterruptState(NULL),
    m_pResponseInterruptSafe(NULL),
    m_pMutex(NULL),
    m_bReuseAddr(FALSE),
    m_bReusePort(FALSE),
    m_bInDestructor(FALSE),
    m_pContext(pContext)
{
    HXLOGL4(HXLOG_NETW, "CON HXUDPSocket this=%p", this);
    
#ifdef _MACINTOSH
	m_pInterruptSafeMacWriteQueue = new InterruptSafeMacQueue();
	HX_ASSERT(m_pInterruptSafeMacWriteQueue != NULL);
#endif

    m_pNetworkServices = pNetworkServices;
    m_pNetworkServices->AddRef();

    if (pContext)
    {
	pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
	pContext->QueryInterface(IID_IHXInterruptState, (void**) &m_pInterruptState);
    }

    if (m_pScheduler)
    {
	m_pSchedulerReadCallback = new ScheduledSocketCallback(this, TRUE);
	m_pSchedulerReadCallback->AddRef();

	m_pSchedulerWriteCallback = new ScheduledSocketCallback(this, TRUE);
	m_pSchedulerWriteCallback->AddRef();

	m_pNonInterruptReadCallback = new ScheduledSocketCallback(this, FALSE);
	m_pNonInterruptReadCallback->AddRef();
    }

    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, pContext);  
}


HXUDPSocket::~HXUDPSocket()
{
    HXLOGL4(HXLOG_NETW, "DES HXUDPSocket this=%p", this);
    m_bInDestructor = TRUE;
	m_pMutex->Lock();

#ifdef _MACINTOSH
	HX_DELETE(m_pInterruptSafeMacWriteQueue); // will release any objects in its nodes
#endif

    if (m_pSchedulerReadCallback)
   		m_pSchedulerReadCallback->Unschedule(m_pScheduler);

    if (m_pSchedulerWriteCallback)
   		m_pSchedulerWriteCallback->Unschedule(m_pScheduler);

    if (m_pNonInterruptReadCallback)
   		m_pNonInterruptReadCallback->Unschedule(m_pScheduler);

    /*
     * XXX...While handling the m_pData->done it's possible for the
     *       DispatchMessage call in CancelSelect to cause an
     *       asynchronous DoRead to occur. The resulting AddRef/Release
     *       would cause this object to be deleted again, so to prevent
     *       this we set the m_pCallback->m_pContext = 0
     */

    if (m_pCallback)
    {
	m_pCallback->m_pContext = 0;
    }

    if (m_pData)
    {
	m_pData->done();
	m_pData->Release();
	m_pData = 0;
    }

    if(m_pUDPResponse)
    {
	m_pUDPResponse->Release();
	m_pUDPResponse = 0;
    }

    if (m_pCallback)
    {
	delete m_pCallback;
	m_pCallback = 0;
    }

    if (m_pScheduler)
    {
	m_pScheduler->Release();
	m_pScheduler = 0;
    }

    while (!m_ReadBuffers.IsEmpty())
    {
	UDP_PACKET* pPacket = (UDP_PACKET*)m_ReadBuffers.RemoveHead();

	HX_RELEASE(pPacket->pBuffer);
	HX_DELETE(pPacket);
    }

    HX_RELEASE(m_pInterruptState);
    HX_RELEASE(m_pResponseInterruptSafe);

    if (m_pSchedulerReadCallback)
    {
    	m_pSchedulerReadCallback->m_pSocket = NULL;
    	m_pSchedulerReadCallback->Release();
    	m_pSchedulerReadCallback = NULL;
    }

    if (m_pSchedulerWriteCallback)
    {
    	m_pSchedulerWriteCallback->m_pSocket = NULL;
    	m_pSchedulerWriteCallback->Release();
    	m_pSchedulerWriteCallback = NULL;
    }

    if (m_pNonInterruptReadCallback)
    {
    	m_pNonInterruptReadCallback->m_pSocket = NULL;
    	m_pNonInterruptReadCallback->Release();
    	m_pNonInterruptReadCallback = NULL;
    }

    m_pMutex->Unlock();
    HX_DELETE(m_pMutex);

#if defined( _WIN32 ) || defined( _WINDOWS )
    win_net::ReleaseWinsockUsage(this);
#endif

    HX_RELEASE(m_pNetworkServices);
}

STDMETHODIMP HXUDPSocket::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXUDPSocket), (IHXUDPSocket*)this },
            { GET_IIDHANDLE(IID_IHXSetSocketOption), (IHXSetSocketOption*)this },
            { GET_IIDHANDLE(IID_IHXUDPMulticastInit), (IHXUDPMulticastInit*)this },
            { GET_IIDHANDLE(IID_IHXSetPrivateSocketOption), (IHXSetPrivateSocketOption*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXUDPSocket*)this },
        };

    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXUDPSocket::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXUDPSocket::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
HXUDPSocket::InitMulticast(UINT8 uTTL)
{
    HXLOGL4(HXLOG_NETW, "HXUDPSocket::InitMulticast(uTTL=%u)", uTTL);
    
    if (HXR_OK != m_pData->set_multicast())
    {
	return HXR_FAIL;
    }

    if (HXR_OK != m_pData->set_multicast_ttl(uTTL))
    {
	return HXR_FAIL;
    }

    return HXR_OK;
}

STDMETHODIMP HXUDPSocket::Init(ULONG32 ulAddr, UINT16 nPort,
				IHXUDPResponse* pUDPResponse)
{
    HXLOGL4(HXLOG_NETW, "HXUDPSocket::Init(ulAddr=%lu,nPort=%u,pUDPResponse=%p)", ulAddr, nPort, pUDPResponse);
    
    if (!pUDPResponse && !m_pUDPResponse)
    {
        /*
         * if the response object hasn't been set up yet, then
         * require a response object (i.e. the first call to Init
         * must always specify a response object
         */
	return HXR_UNEXPECTED;
    }

    HX_RESULT theErr = HXR_OK;
    UINT32 ulPlatformData = 0;

    if (pUDPResponse != NULL)
    {
        HX_RELEASE(m_pUDPResponse);
        m_pUDPResponse = pUDPResponse;
        m_pUDPResponse->AddRef();
    }

    HX_RELEASE(m_pResponseInterruptSafe);
    m_pUDPResponse->QueryInterface(IID_IHXInterruptSafe,
				   (void**) &m_pResponseInterruptSafe);

    m_sockAddr.sin_family	= AF_INET;
    m_sockAddr.sin_addr.s_addr	= DwToNet(ulAddr);  //*(long*)&ulAddr;
    m_sockAddr.sin_port		= WToNet(nPort);

    m_nDestPort = nPort;

    return HXR_OK;
}

STDMETHODIMP HXUDPSocket::Bind(UINT32 ulLocalAddr, UINT16 nPort)
{
    HXLOGL4(HXLOG_NETW, "HXUDPSocket::Bind(ulLocalAddr%lu,nPort=%u)", ulLocalAddr, nPort);
    
    if (m_bInitComplete)
	return HXR_UNEXPECTED;

#if defined( _WIN32 ) || defined( _WINDOWS )
    //	Have we been able to load and initialize the winsock stuff yet?
    if (!win_net::IsWinsockAvailable(this))
    {
	return HXR_FAIL; // HXR_GENERAL_NONET;
    }
#endif

    m_pNetworkServices->UseDrivers();

    HX_RESULT theErr = conn::init_drivers(NULL);
    if (theErr)
    {
	return (theErr);
    }

    theErr = HXR_OK;
    UINT32 ulPlatformData = 0;

#ifdef _UNIX
    //This one has to be set before we create a new socket.
    conn::SetNetworkThreadingPref( ReadNetworkThreadingPref((IUnknown*)m_pContext) );
    conn::SetThreadedDNSPref( ReadThreadedDNSPref((IUnknown*)m_pContext) );
#endif

    m_pData = conn::new_socket(m_pContext, HX_UDP_SOCKET);
    if (!m_pData)
    {
	return HXR_OUTOFMEMORY;
    }

    // XXXGo - As it is implemented, this is the only way...
    if (m_bReuseAddr)
    {
	if (m_pData->reuse_addr(m_bReuseAddr) != HXR_OK)
	{
	    // err...what do we need to do?
	    HX_ASSERT(!"reuse_addr() failed");
	}
    }
    if (m_bReusePort)
    {
	if (m_pData->reuse_port(m_bReusePort) != HXR_OK)
	{
	    // err...what do we need to do?
	    HX_ASSERT(!"reuse_port() failed");
	}
    }

#ifdef _UNIX
    m_pData->SetAsyncDNSPref( ReadAsyncDNSPref((IUnknown*)m_pContext) );
#endif


    // XXXST -- local addr binding stuff, removed dependency to m_nLocalPort
    // 0 for local port will make the system choose a free port
    theErr = m_pData->init(ulLocalAddr, nPort);

    if (theErr)
    {
	theErr = ConvertNetworkError(theErr);
	if (theErr)
	{
	    m_pData->done();
	    m_pData->Release();
	    m_pData = 0;
	    return theErr;
	}
    }

    m_pData->nonblocking();

    m_pData->set_receive_buf_size(DESIRED_RCV_BUFSIZE);
    if (!m_pCallback)
    {
	m_pCallback = new UDPSocketCallback;
	m_pCallback->m_pContext = this;
    }
    m_pData->set_callback(m_pCallback);

#ifdef _WINDOWS
#if defined (_WIN32)
    ulPlatformData = (UINT32)GetModuleHandle(NULL);
#elif defined (_WIN16)
    ulPlatformData = (UINT32)(int)g_hInstance;
#endif
    m_pData->SetWindowHandle(ulPlatformData);
#endif /* defined (_WINDOWS) */

    if (m_pSchedulerReadCallback)
    {
	    m_pSchedulerReadCallback->ScheduleCallback(UDP_BIND_COMMAND, m_pScheduler, SCHED_GRANULARITY);
    }

    m_bInitComplete = TRUE;
    return theErr;
}

STDMETHODIMP HXUDPSocket::Read(UINT16 nBytes)
{
    HXLOGL4(HXLOG_NETW, "%p::HXUDPSocket::Read(%u) m_bInRead=%lu", this, nBytes, m_bInRead);
    
    if (!m_bInitComplete)
    {
	HX_RESULT ret = Bind(HXR_INADDR_ANY, 0);
	if (HXR_OK != ret)
	    return HXR_UNEXPECTED;
    }

    HX_RESULT theErr = HXR_OK;

    if(m_bReadPending)
    {
	return HXR_UNEXPECTED;
    }

    m_bReadPending = TRUE;

    if (m_bInRead)
    {
	return HXR_OK;
    }

    m_bInRead = TRUE;

    m_pMutex->Lock();
    UINT16 uNumIterations = 0;
    do
    {
	theErr = DoRead();
	uNumIterations++;
    } while (m_bReadPending && !theErr && m_ReadBuffers.GetCount() > 0 && uNumIterations < MAX_ITERATION_COUNT);

    m_pMutex->Unlock();
    theErr = ConvertNetworkError(theErr);

    if (m_bReadPending && m_pSchedulerReadCallback)
	    m_pSchedulerReadCallback->ScheduleCallback(UDP_READ_COMMAND, m_pScheduler, SCHED_GRANULARITY);

    m_bInRead = FALSE;

    return theErr;
}

STDMETHODIMP HXUDPSocket::Write(IHXBuffer* pBuffer)
{
    HXLOGL4(HXLOG_NETW, "HXUDPSocket::Write(pBuffer=%p)", pBuffer);
    
    if (!m_bInitComplete)
    {
	HX_RESULT ret = Bind(HXR_INADDR_ANY, 0);
	if (HXR_OK != ret)
	    return HXR_UNEXPECTED;
    }
    HX_RESULT theErr = HXR_OK;
    HX_RESULT lResult = HXR_OK;

#if 0
    struct in_addr in;
    in.s_addr = m_sockAddr.sin_addr.s_addr;
    char* address = inet_ntoa(in);
    printf("address = %s:", address);
    UINT32 port = ntohl(m_sockAddr.sin_port);
    printf("%d\n", port);
#endif /* 0 */

#ifdef _MACINTOSH
	if (m_pInterruptSafeMacWriteQueue)
		m_pInterruptSafeMacWriteQueue->AddTail(pBuffer); // AddRef called inside
#else
	pBuffer->AddRef();
	m_WriteBuffers.AddTail((void*) pBuffer);
#endif


    m_pMutex->Lock();
    theErr = DoWrite();
    m_pMutex->Unlock();

    lResult = ConvertNetworkError(theErr);

    return lResult;
}

STDMETHODIMP HXUDPSocket::WriteTo(ULONG32 ulAddr,
    UINT16 nPort, IHXBuffer* pBuffer)
{
    HXLOGL4(HXLOG_NETW, "HXUDPSocket::WriteTo(ulAddr=%lu,nPort=%u,pBuffer=%p)", ulAddr, nPort, pBuffer);
    
    if (!m_bInitComplete)
    {
	HX_RESULT ret = Bind(HXR_INADDR_ANY, 0);
	if (HXR_OK != ret)
	    return HXR_UNEXPECTED;
    }

    m_sockAddr.sin_family	= AF_INET;
    m_sockAddr.sin_addr.s_addr	= DwToNet(ulAddr);   //*(long*)&ulAddr;
    m_sockAddr.sin_port		= WToNet(nPort);

    return (Write(pBuffer));
}

STDMETHODIMP HXUDPSocket::GetLocalPort(UINT16& nPort)
{
    // Get the local port from the socket info
    nPort = m_pData->get_local_port();

    return (INT16)nPort >= 0 ? HXR_OK : HXR_FAIL;
}

STDMETHODIMP HXUDPSocket::JoinMulticastGroup(
    ULONG32	    ulMulticastAddr,
    ULONG32	    ulInterfaceAddr)
{
    HXLOGL4(HXLOG_NETW, "HXUDPSocket::JoinMulticastGroup(ulMulticastAddr=%lu,ulInterfaceAddr=%lu)", ulMulticastAddr, ulInterfaceAddr);
    
#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
    HX_RESULT theErr = HXR_OK;
    HX_RESULT lResult = HXR_OK;

    m_pMutex->Lock();
    theErr = m_pData->join_multicast_group(ulMulticastAddr, ulInterfaceAddr);
    lResult = ConvertNetworkError(theErr);
    m_pMutex->Unlock();

    return lResult;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */
}

STDMETHODIMP HXUDPSocket::LeaveMulticastGroup(
    ULONG32	    ulMulticastAddr,
    ULONG32	    ulInterfaceAddr)
{
    HXLOGL4(HXLOG_NETW, "HXUDPSocket::LeaveMulticastGroup(ulMulticastAddr=%lu,ulInterfaceAddr=%lu)", ulMulticastAddr, ulInterfaceAddr);
#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
    HX_RESULT theErr = HXR_OK;
    HX_RESULT lResult = HXR_OK;

    m_pMutex->Lock();
    theErr = m_pData->leave_multicast_group(ulMulticastAddr, ulInterfaceAddr);
    while (!m_ReadBuffers.IsEmpty())
    {
	UDP_PACKET* pPacket = (UDP_PACKET*)m_ReadBuffers.RemoveHead();

	HX_RELEASE(pPacket->pBuffer);
	HX_DELETE(pPacket);
    }
    lResult = ConvertNetworkError(theErr);
    m_pMutex->Unlock();

    return lResult;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */
}

HX_RESULT
HXUDPSocket::DoRead()
{
    HXLOGL4(HXLOG_NETW, "%p::HXUDPSocket::DoRead()", this);
    
#ifdef _MACINTOSH
	if (m_bInDoRead)
		return HXR_OK; // whatever needs to be done will be
                               // done by the caller that's already
                               // here.

	// xxxbobclark the m_bInDoRead flag is hacked around calling
	// ReadDone(), because ReadDone() may call Read() which in
	// turn calls us here, and we do not want to bail out in that
	// instance. (Otherwise we only remove one packet at a time,
	// which, given the scheduler granularity and high bit rates,
	// implies that our bandwidth would be too low.)
#endif
	m_bInDoRead = TRUE;

    HX_RESULT theErr = HXR_OK;
    IHXBuffer* pBuffer = 0;
    UINT32  ulAddress = 0;
    UINT16  ulPort = 0;
    UINT16  count = 0;


    do
    {
	/*
	 * Must reset count before every read
	 */
	count = TCP_BUF_SIZE;

	theErr = m_pData->readfrom(pBuffer, ulAddress, ulPort);

#if defined(HELIX_FEATURE_LOGLEVEL_4) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
    UINT16 usRTPSeq = 0;
    UINT32 ulRTPTimestamp = 0;
    if (pBuffer && pBuffer->GetSize() >= 8)
    {
        BYTE* pTmp = pBuffer->GetBuffer();
        usRTPSeq       = ((pTmp[2] << 8) | pTmp[3]);
        ulRTPTimestamp = ((pTmp[4] << 24) | (pTmp[5] << 16) | (pTmp[6] << 8)  | pTmp[7]);
    }
    HXLOGL4(HXLOG_NETW, "%p::HXUDPSocket::DoRead()\tconn::readfrom(pBuffer=%p,ulAddress=%lu,ulPort=%u) (rtpSeq=%u,rtpTS=%lu) returned 0x%08x",
            this, pBuffer, ulAddress, ulPort, usRTPSeq, ulRTPTimestamp, theErr);
#endif /* #if defined(HELIX_FEATURE_LOGLEVEL_4) || defined(HELIX_FEATURE_LOGLEVEL_ALL) */
	if (!theErr && pBuffer)
	{
	    UDP_PACKET* pPacket = new UDP_PACKET;

	    pPacket->pBuffer = pBuffer;
	    pPacket->ulAddress = ulAddress;
	    pPacket->ulPort = ulPort;

	    m_ReadBuffers.AddTail((void*)pPacket);
	}
	else
	{
	    count = 0;
	}
    } while (!theErr && count > 0);

    if (m_bReadPending && m_ReadBuffers.GetCount() > 0)
    {
	/* If we are at interrupt time and the response object is not
	 * interrupt safe, schedule a callback to return back the data
	 */
	if (!IsSafe())
	{
		m_bInDoRead = FALSE;
	    return HXR_AT_INTERRUPT;
	}

	m_bReadPending = FALSE;

	UDP_PACKET* pPacket = (UDP_PACKET*)m_ReadBuffers.RemoveHead();
	pBuffer = pPacket->pBuffer;
	ulAddress = pPacket->ulAddress;
	ulPort = pPacket->ulPort;

	AddRef();


	m_bInDoRead = FALSE;
#if defined(HELIX_FEATURE_LOGLEVEL_4) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
    UINT16 usRTPSeq = 0;
    UINT32 ulRTPTimestamp = 0;
    if (pBuffer && pBuffer->GetSize() >= 8)
    {
        BYTE* pTmp = pBuffer->GetBuffer();
        usRTPSeq       = ((pTmp[2] << 8) | pTmp[3]);
        ulRTPTimestamp = ((pTmp[4] << 24) | (pTmp[5] << 16) | (pTmp[6] << 8)  | pTmp[7]);
    }
    HXLOGL4(HXLOG_NETW, "%p::HXUDPSocket::DoRead()\tcalling ReadDone(HXR_OK,pBuffer=%p,ulAddress=%lu,ulPort=%u) (rtpSeq=%u,rtpTS=%lu)",
            this, pBuffer, ulAddress, ulPort, usRTPSeq, ulRTPTimestamp);
#endif /* #if defined(HELIX_FEATURE_LOGLEVEL_4) || defined(HELIX_FEATURE_LOGLEVEL_ALL) */
	m_pUDPResponse->ReadDone(HXR_OK, pBuffer, ulAddress, ulPort);
	m_bInDoRead = TRUE;

	HX_RELEASE(pBuffer);
	HX_DELETE(pPacket);

	Release();

	m_bInDoRead = FALSE;
	return HXR_OK;
    }

    /* if we called from within Read(), we will schedule a callback there, if necessary */
    if (!m_bInRead && m_pSchedulerReadCallback)
	    m_pSchedulerReadCallback->ScheduleCallback(UDP_READ_COMMAND, m_pScheduler, SCHED_GRANULARITY);

	m_bInDoRead = FALSE;
	return theErr;
}

HX_RESULT
HXUDPSocket::DoWrite()
{
    HX_RESULT theErr = HXR_OK;

    if (m_bInWrite) return HXR_OK;
    m_bInWrite = TRUE;

#ifdef _MACINTOSH
	if (m_pInterruptSafeMacWriteQueue)
		m_pInterruptSafeMacWriteQueue->TransferToSimpleList(m_WriteBuffers);
#endif

    while (!theErr && m_WriteBuffers.GetCount() > 0)
    {
	IHXBuffer* pBuffer = (IHXBuffer*) m_WriteBuffers.GetHead();
	UINT16 uLength = (UINT16) pBuffer->GetSize();
	theErr = m_pData->writeto(  pBuffer->GetBuffer(),   // sendto
				    &uLength,
				    (UINT32) m_sockAddr.sin_addr.s_addr,
				    WToHost(m_sockAddr.sin_port));
	if (!theErr)
	{
	    pBuffer->Release();
	    m_WriteBuffers.RemoveHead();
	}
    }

    if (m_pSchedulerWriteCallback &&
	m_WriteBuffers.GetCount() > 0)
    {
	    m_pSchedulerWriteCallback->ScheduleCallback(UDP_WRITE_COMMAND, m_pScheduler, SCHED_GRANULARITY);
    }

    m_bInWrite = FALSE;

    return theErr;
}

/* If we are at interrupt time and the response object is not interrupt safe,
 * schedule a callback to return back the data
 */
HXBOOL
HXUDPSocket::IsSafe()
{
    if (m_pInterruptState && m_pInterruptState->AtInterruptTime() &&
	(!m_pResponseInterruptSafe ||
	 !m_pResponseInterruptSafe->IsInterruptSafe()))
    {
	if (m_pNonInterruptReadCallback){
		m_pNonInterruptReadCallback->ScheduleCallback(UDP_READ_COMMAND, m_pScheduler, 0);
	}

	return FALSE;
    }

    return TRUE;
}

STDMETHODIMP HXUDPSocket::HandleCallback(INT32  theCommand, HX_RESULT theError)
{
    HX_RESULT theErr = HXR_OK;
    if (!m_bInDestructor)
    {
	AddRef();
	m_pMutex->Lock();
	if (!m_bInDestructor)
	{
		switch(theCommand)
		{
			case UDP_READ_COMMAND:
			    theErr = DoRead();
			    break;

			case UDP_WRITE_COMMAND:
			    theErr = DoWrite();  // protected from re-entry by m_bInWrite
			    break;

			default:
			    theErr = DoRead();
                            if( theErr == HXR_OK )
                            {
			        theErr = DoWrite();
                            }
			    break;
	   	}
	}
	m_pMutex->Unlock();
	Release();
    }

    return theErr;
}


HX_RESULT HXUDPSocket::UDPSocketCallback::Func(NotificationType Type,
					       HXBOOL bSuccess, conn* pConn)
{
    if(m_pContext)
    {
	switch (Type)
	{
	case READ_NOTIFICATION:
#if defined(_UNIX_THREADED_NETWORK_IO)
            if( !ReadNetworkThreadingPref((IUnknown*)(m_pContext->m_pContext)) )
            {
                m_pContext->AddRef();
                m_pContext->m_pMutex->Lock();
                m_pContext->DoRead();
                m_pContext->m_pMutex->Unlock();
                m_pContext->Release();
            }
#elif !defined (THREADS_SUPPORTED) && !defined(_MACINTOSH)
	    m_pContext->AddRef();
	    m_pContext->m_pMutex->Lock();
	    m_pContext->DoRead();
	    m_pContext->m_pMutex->Unlock();
	    m_pContext->Release();
#endif
	    break;
	case WRITE_NOTIFICATION:
#if defined(_UNIX_THREADED_NETWORK_IO)
            if( !ReadNetworkThreadingPref((IUnknown*)(m_pContext->m_pContext)) )
            {
                m_pContext->AddRef();
                m_pContext->m_pMutex->Lock();
                m_pContext->DoWrite();
                m_pContext->m_pMutex->Unlock();
                m_pContext->Release();
            }
#elif !defined (THREADS_SUPPORTED) && !defined(_MACINTOSH)
	    m_pContext->AddRef();
	    m_pContext->m_pMutex->Lock();
	    m_pContext->DoWrite();
	    m_pContext->m_pMutex->Unlock();
	    m_pContext->Release();
#endif
	    break;
	case CONNECT_NOTIFICATION:
	default:
	    break;
	}
    }
    return HXR_OK;
}


STDMETHODIMP
HXUDPSocket::SetOption(HX_SOCKET_OPTION option, UINT32 ulValue)
{
    HX_RESULT res = HXR_OK;

    switch(option)
    {
    case HX_SOCKOPT_REUSE_ADDR:
	m_bReuseAddr = (HXBOOL)ulValue;
    break;
    case HX_SOCKOPT_REUSE_PORT:
	m_bReusePort = (HXBOOL)ulValue;
    break;
	case HX_SOCKOPT_BROADCAST:
	{
	HX_RESULT theErr = HXR_OK;
	if(m_pData)
	{
		m_pMutex->Lock();
		theErr = m_pData->set_broadcast(ulValue);
		res = ConvertNetworkError(theErr);
		m_pMutex->Unlock();
	}
	}
	break;
    case HX_SOCKOPT_MULTICAST_IF:
	if (m_pData)
	{
	    res = m_pData->set_multicast_if(ulValue);
	}
	break;
    case HX_SOCKOPT_SET_SENDBUF_SIZE:
    {
      if (m_pData)
      {
          HX_RESULT theErr = HXR_OK;

           m_pMutex->Lock();
           theErr = m_pData->set_send_size(ulValue);
           res = ConvertNetworkError(theErr);
           m_pMutex->Unlock();
      }
      break;
    }
    default:
	HX_ASSERT(!"I don't know this option");
	res = HXR_FAIL;
    }

    return res;
}

STDMETHODIMP
HXUDPSocket::SetOption(HX_PRIVATE_SOCKET_OPTION option, UINT32 ulValue)
{
    HX_RESULT res = HXR_OK;

    switch(option)
    {
    case HX_PRIVATE_SOCKOPT_IGNORE_WSAECONNRESET:
    {
	if (m_pData)
	{
	    m_pMutex->Lock();
	    m_pData->IgnoreWSAECONNRESET((HXBOOL)ulValue);
	    m_pMutex->Unlock();
	}
	break;
    }
    default:
	HX_ASSERT(!"I don't know this option");
	res = HXR_FAIL;
    }
    return HXR_OK;
}

#ifdef _MACINTOSH

/////////////////////////////////////////////////////////////////////////
//
//	InterruptSafeMacQueue
//
//	For passing data between an interrupt and anything else (mac only).
//
InterruptSafeMacQueue::InterruptSafeMacQueue()
{
 	mQueueHeader.qFlags=0;
	mQueueHeader.qHead=0;
	mQueueHeader.qTail=0;
	mDestructing = FALSE; // just a safety check
}

/////////////////////////////////////////////////////////////////////////
//
HX_RESULT InterruptSafeMacQueue::AddTail(IUnknown* pObject)
{
	if (pObject && !mDestructing)
	{
	    IhxQueueElement * theElement = new IhxQueueElement();

	    if (theElement)
	    {
		    theElement->mNextElementInQueue = NULL;
		    theElement->mObject = pObject;
	   		theElement->mObject->AddRef();
		    ::Enqueue((QElem *)theElement, &mQueueHeader);

		    //
		    // If someone interrupts and enters the destructor while we're in here,
		    // then the pObject and the new node will be leaked.  This shouldn't
		    // happen since we should have shut down all interrupts that would
		    // be adding items to the queue long before we start destructing it.
		    //

		    HX_ASSERT(!mDestructing); // if we DID enter the destructor, let the programmer know...
	    }

	    return HXR_OK;
    }

    return HXR_FAILED;
}

/////////////////////////////////////////////////////////////////////////
//
IUnknown * InterruptSafeMacQueue::RemoveHead()
{
	//
    // POINT A
    //
    // You can look at the qHead anytime you want, but you can't USE a
    // pointer unless it's OFF of the queue.  Basically you do a Dequeue,
    // and if it succeeds then you know nobody else has it.  If it fails,
    // an error is returned and you don't mess with it.
    //

	if (mQueueHeader.qHead)
    {
 		IhxQueueElement * theElement = (IhxQueueElement *) mQueueHeader.qHead;

		if (theElement)
		{
			OSErr e = ::Dequeue( (QElemPtr) theElement, &mQueueHeader );

			//
			// Between points A and D, we can't be guaranteed that the queue header and theElement are valid.  But
			// Dequeue will TELL us if that pointer is still valid by its return code.  If it can't remove the item
			// from the queue, then somebody else did and the pointer is no longer ours.  If no error was returned
			// from dequeue, then it's ours to mess with.
			//

			if (e == noErr)
			{
				// at this point we know that we can do whatever we need to with the object.
				IUnknown * theObj = theElement->mObject;
				delete theElement; // delete the node
				return theObj;
			}
		}
   }

   return NULL;
}

/////////////////////////////////////////////////////////////////////////
//
HX_RESULT InterruptSafeMacQueue::TransferToSimpleList(CHXSimpleList &simpleList)
{
	IUnknown * theObject;

	// we did an AddRef in the AddTail call -- just pass it along to the simplelist
	while ((theObject = RemoveHead()) != 0)
	{
		simpleList.AddTail((void *)theObject);
	}

   return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//
InterruptSafeMacQueue::~InterruptSafeMacQueue()
{
	mDestructing = TRUE; // don't add anything else to the queue

	IUnknown * theObject;

	while ((theObject = RemoveHead()) != 0)
    {
		theObject->Release();
    }

    // and just to be safe...
	mQueueHeader.qHead=0;
	mQueueHeader.qTail=0;
}

#endif // _MACINTOSH
