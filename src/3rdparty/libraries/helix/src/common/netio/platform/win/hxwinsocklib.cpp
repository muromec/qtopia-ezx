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
#include "hlxclib/windows.h"
#include "hxassert.h"
#include "pckunpck.h"
#include "hxthread.h"
#include "hxscope_lock.h"
#include "debug.h"
#include "hxtlogutil.h"
#define WINSOCKLIB_NO_SOCKAPI_DEFINES
#include "hxwinsocklib.h"


#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if !defined(HELIX_CONFIG_SOCKLIB_EXPLICIT_LOAD)
#error should not be included in build
#endif

HXWinSockLib& HXWinSockLib::Lib(IUnknown* pContext)
{
    //XXXLCM global manager?
    static HXWinSockLib instance(pContext);
    return instance;
}


HXWinSockLib::HXWinSockLib(IUnknown* pContext)
: m_wsHighVersion(0)
, m_wsVersion(0)
, m_hLib(NULL)
, m_userCount(0)
, m_pLoadMutex(0)
{
    memset(&m_api, 0, sizeof(m_api));

    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pLoadMutex, pContext);
    HX_ASSERT(m_pLoadMutex);
}

HXWinSockLib::~HXWinSockLib()
{
    HX_RELEASE(m_pLoadMutex);
    //XXXLCM fixme: 
    // this will assert if other static objects use us and this static object
    // is destroyed before others during runtime static cleanup
    // HX_ASSERT(0 == m_userCount); // calls to Load()/UnLoad() must be balanced
}

namespace
{
// these help reduce clutter and facilitate typing in Load()
template<class T>
inline
bool HXSetProcAddress(HMODULE hMod, T& pfn, const TCHAR* pszFunctionName) 
{
    HX_ASSERT(!pfn);
    pfn = reinterpret_cast<T>(::GetProcAddress(hMod, pszFunctionName) );
    return (pfn != 0);
}
#define LOAD_PROC(name) if( !HXSetProcAddress(m_hLib, m_api.m_ ## name, TEXT(#name))) { hr = HXR_FAIL; }
}

//
// Attempt to load winsock DLL and obtain entry points to required API.
//
//
HX_RESULT HXWinSockLib::Load()
{
    HXScopeLock lock((IHXMutex*)m_pLoadMutex);

    HXLOGL3(HXLOG_NETW, "HXWinSockLib::Load(): usage count going to %lu", m_userCount + 1);

    if(m_hLib)
    {
        ++m_userCount;
        return HXR_OK;
    }

#if defined(WIN32_PLATFORM_PSPC)   
    const TCHAR* const pszLibName       = "winsock.dll";
    const TCHAR* const pszAltLibName    = 0;
#elif defined(_WIN32)
    const TCHAR* const pszLibName       = "ws2_32.dll"; // 2.X
    const TCHAR* const pszAltLibName    = "winsock32.dll"; // 1.1
#else
    # error fix this
#endif

    HX_RESULT hr = HXR_FAIL;
  
    HXLOGL3(HXLOG_NETW, "HXWinSockLib::Load(): loading lib");

    m_hLib = LoadLibrary(pszLibName);
    if(!m_hLib && pszAltLibName)
    {
        m_hLib = LoadLibrary(pszAltLibName);
    }

    if (m_hLib)
    {
        ++m_userCount;
        hr = HXR_OK;

        //
        // load required set of function pointers from library
        //
        LOAD_PROC(accept);
        LOAD_PROC(bind);
        LOAD_PROC(closesocket);
        LOAD_PROC(WSAStartup);
        LOAD_PROC(WSACleanup);
        LOAD_PROC(connect);
        LOAD_PROC(ioctlsocket);
        LOAD_PROC(getpeername);
        LOAD_PROC(getsockname);
        LOAD_PROC(getsockopt);
        LOAD_PROC(htonl);
        LOAD_PROC(htons);
        LOAD_PROC(inet_addr);
        LOAD_PROC(inet_ntoa);
        LOAD_PROC(listen);
        LOAD_PROC(ntohl);
        LOAD_PROC(ntohs);
        LOAD_PROC(recv);
        LOAD_PROC(recvfrom);
        LOAD_PROC(select);
        LOAD_PROC(send);
        LOAD_PROC(sendto);
        LOAD_PROC(setsockopt);
        LOAD_PROC(shutdown);
        LOAD_PROC(socket);
        LOAD_PROC(gethostbyaddr);
        LOAD_PROC(gethostbyname);
        LOAD_PROC(gethostname);
        LOAD_PROC(__WSAFDIsSet);

#if defined(_WINCE)
        LOAD_PROC(SetLastError);
        LOAD_PROC(GetLastError);
#endif

#if !defined(_WINCE)
        LOAD_PROC(getservbyport);
        LOAD_PROC(getservbyname);
        LOAD_PROC(getprotobynumber);
        LOAD_PROC(getprotobyname);

        LOAD_PROC(WSASetLastError);
        LOAD_PROC(WSAGetLastError);
        //LOAD_PROC(WSAIsBlocking);
        //LOAD_PROC(WSAUnhookBlockingHook);
        //LOAD_PROC(WSASetBlockingHook);
        //LOAD_PROC(WSACancelBlockingCall);
        LOAD_PROC(WSAAsyncGetServByName);
        LOAD_PROC(WSAAsyncGetServByPort);
        LOAD_PROC(WSAAsyncGetProtoByName);
        LOAD_PROC(WSAAsyncGetProtoByNumber);
        LOAD_PROC(WSAAsyncGetHostByName);
        LOAD_PROC(WSAAsyncGetHostByAddr);
        LOAD_PROC(WSACancelAsyncRequest);
        LOAD_PROC(WSAAsyncSelect);
        LOAD_PROC(WSASend);
        LOAD_PROC(WSASendTo);
        LOAD_PROC(WSARecv);
        LOAD_PROC(WSARecvFrom);
        LOAD_PROC(WSAIoctl);
#endif
        
        // everything prior to this should be succeed even with ipv4-only WinSock support
        HX_ASSERT(HXR_OK == hr);

        if( HXR_OK == hr )
        {
            // optional IPv6 api
            LOAD_PROC(getaddrinfo);
            LOAD_PROC(getnameinfo);
            LOAD_PROC(freeaddrinfo);
            hr = WinsockInit();
        }

        if(FAILED(hr))
        {
            UnLoad();
        }

    }
    return hr;
}


void HXWinSockLib::UnLoad()
{
    HXScopeLock lock((IHXMutex*)m_pLoadMutex);

    HX_ASSERT(m_hLib);
    if(m_hLib) 
    {
        HXLOGL3(HXLOG_NETW, "HXWinSockLib::UnLoad(): usage count going to %lu", m_userCount - 1);
        HX_ASSERT(m_userCount > 0);
        if(0 == --m_userCount)
        {
            WinsockCleanup();
        
            ::FreeLibrary(m_hLib);
            m_hLib = NULL;
            memset(&m_api, 0, sizeof(m_api));
            HXLOGL3(HXLOG_NETW, "HXWinSockLib::UnLoad(): winsock unloaded");
        }
    }
}


void 
HXWinSockLib::WinsockCleanup()
{
    if( m_wsVersion != 0)
    {
        HX_ASSERT(m_api.m_WSACleanup);
        m_api.m_WSACleanup();
        m_wsVersion = 0;
        m_wsHighVersion = 0;
    }
}

HX_RESULT 
HXWinSockLib::TryWinsockInit(BYTE major, BYTE minor)
{
    WORD	version = MAKEWORD(major, minor);
    WSADATA	wsaData;

    HX_RESULT hr = HXR_FAIL;

    HX_ASSERT(m_api.m_WSAStartup);
    int err = m_api.m_WSAStartup( version, &wsaData );

    if(0 == err)
    {
        HXLOGL3(HXLOG_NETW, "HXWinSockLib::WinsockInit(): ver = %u.%u; highest = %u.%u; desc = '%s'",
            LOBYTE( wsaData.wVersion ), HIBYTE( wsaData.wVersion ),
            LOBYTE( wsaData.wHighVersion ), HIBYTE( wsaData.wHighVersion ),
            wsaData.szDescription);

        HXLOGL3(HXLOG_NETW, "HXWinSockLib::WinsockInit(): max datagram = %ld", wsaData.iMaxUdpDg);
            
        // for sanity's sake verify we got what we requested
        if (LOBYTE( wsaData.wVersion ) == major && HIBYTE( wsaData.wVersion ) == minor) 
	{
            hr = HXR_OK;
            m_wsHighVersion = wsaData.wHighVersion;
            m_wsVersion = wsaData.wVersion;
        }
    }

    return hr;
}



HX_RESULT 
HXWinSockLib::WinsockInit()
{
    HX_ASSERT(m_hLib);
    HX_ASSERT(0 == m_wsVersion);
    
    HX_RESULT hr = HXR_FAIL;
    
    // WinSock with IPv6 support appears to support 2.2, so we first ask for that     
    hr = TryWinsockInit(2, 2);
    if(HXR_OK != hr)
    { 
        hr = TryWinsockInit(1, 1);
        if(HXR_OK != hr)
        {
            HX_ASSERT(false);
            WinsockCleanup();
        }
    }
        
    return hr;
}




