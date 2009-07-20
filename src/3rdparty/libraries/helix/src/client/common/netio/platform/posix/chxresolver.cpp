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

#include "netdrv.h"
#include "hxnet.h"
#include "sockimp.h"
#include "hxtaskmanager.h"
#include "hxmsgs.h"
#include "hxposixsockutil.h"
#include "chxresolver.h"

#include "hxtlogutil.h"
#include "hxassert.h"
#include "debug.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif


static
HX_RESULT TranslateResolveErr(int err)
{
    switch (err)
    {
    case RSLVERR_NODATA:                  return HXR_RSLV_NODATA;
    case RSLVERR_NONAME:                  return HXR_RSLV_NONAME;
    default:
        HX_ASSERT(false);
    }

    return HXR_FAIL;
}

// class ResolverTask
class ResolverTask
: public HXTaskManager::Task
{
public:
    ResolverTask(IHXNetServices* pNetServices, CHXResolver* pResolver);
    virtual ~ResolverTask();
    HX_RESULT Init(const char* pszNode, 
                    const char* pszService, 
                    IHXAddrInfo* pHints);

    HX_RESULT GetResult(IHXSockAddr**& ppAddr, UINT32& count) const;

    //HXTaskManager::Task()
    void OnTaskComplete(HX_RESULT hr);

protected:
    void SaveAddrInfoHelper(CHXSimpleList& list);

protected:

    // task input
    CHXString m_node;
    CHXString m_service;
    IHXAddrInfo* m_pHints;

    // task results
    IHXSockAddr**   m_ppAddr;
    UINT32          m_infoCount;
    HX_RESULT       m_result;

    IHXNetServices* m_pNetServices;
    CHXResolver* m_pResolver;
};

void ResolverTask::OnTaskComplete(HX_RESULT hr)
{
    m_pResolver->OnResolverTaskComplete(hr, this);
}


HX_RESULT ResolverTask::GetResult(IHXSockAddr**& ppAddr, 
                                  UINT32& count) const
{
    ppAddr = m_ppAddr;
    count = m_infoCount;
    return m_result;
}

void ResolverTask::SaveAddrInfoHelper(CHXSimpleList& list)
{
    HX_ASSERT(HXR_OK == m_result);
    m_infoCount = list.GetCount();
    m_ppAddr = new IHXSockAddr*[m_infoCount];
    if(!m_ppAddr)
    {
        m_infoCount = 0;
        m_result = HXR_OUTOFMEMORY;
    }

    INT32 idx = 0;
    CHXSimpleList::Iterator end = list.End();
    for(CHXSimpleList::Iterator begin = list.Begin(); begin != end; ++begin)
    {
        IHXSockAddr* pAddr = reinterpret_cast<IHXSockAddr*>(*begin);
        if(m_ppAddr)
        {
            // transfer ownership
            m_ppAddr[idx++] = pAddr;
        }
        else
        {
            // oom case
            pAddr->Release();
        }
    }
    
    list.RemoveAll();
}

ResolverTask::ResolverTask(IHXNetServices* pNetServices, CHXResolver* pResolver)
: m_pHints(0)
, m_ppAddr(0) 
, m_infoCount(0)       
, m_result(HXR_FAIL)
, m_pNetServices(pNetServices)
, m_pResolver(pResolver)
{
    HX_ASSERT(m_pNetServices);
    HX_ASSERT(m_pResolver);
    m_pNetServices->AddRef();
    m_pResolver->AddRef();
}
  

HX_RESULT ResolverTask::Init(const char* pszNode, 
                         const char* pszService, 
                         IHXAddrInfo* pHints)
{
    HX_ASSERT(pszNode);

    m_node = pszNode;
    m_service = pszService; // port

    m_pHints = pHints;
    if(m_pHints)
    {
        m_pHints->AddRef();
    }

    HXLOGL3(HXLOG_NETW, "ResolverTask[%p]::Init(): '%s', '%s'", this, (const char*)m_node, (const char*)m_service);

    return HXR_OK;
}

ResolverTask::~ResolverTask()
{
    HX_RELEASE(m_pHints);
    for(UINT32 idx = 0; idx < m_infoCount; ++idx)
    {
        IHXSockAddr* pAddr = m_ppAddr[idx];
        HX_ASSERT(pAddr);
        pAddr->Release();
    }
    HX_VECTOR_DELETE(m_ppAddr);

    HX_RELEASE(m_pNetServices);
    HX_RELEASE(m_pResolver);
}

// class AddrInfoTask
class AddrInfoTask
: public ResolverTask
{
public:
    AddrInfoTask(IHXNetServices* pNetServices, CHXResolver* pResolver);

//HXTaskManager::Task
    void Execute();

private:
    void SaveAddrInfo(const struct addrinfo* pRawInfo);
};

AddrInfoTask::AddrInfoTask(IHXNetServices* pNetServices, CHXResolver* pResolver) 
: ResolverTask(pNetServices, pResolver) 
{
}

void AddrInfoTask::Execute()
{
    HXLOGL3(HXLOG_NETW, "AddrInfoTask[%p]::Execute(): start", this);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(const struct addrinfo));

    HX_ASSERT(!m_pHints); // XXXLCM implement socktype and protocol; update IHXAddrInfo
    //hints.ai_family = (m_pHints ? HXFamilyToAF(m_pHints->GetFamily()) : AF_UNSPEC);
    hints.ai_family = AF_UNSPEC;

    // hints.ai_socktype == //SOCK_RAW, SOCK_STREAM, or SOCK_DGRAM; 0 == any
    // hints.ai_protocol == //IP only (IPPROTO_TCP or IPPROTO_UDP); 0 == any
    // everything else must be 0

    struct addrinfo* pRawInfo = 0;
    int res = hx_getaddrinfo(m_node, m_service, &hints, &pRawInfo);
    if (0 == res)
    {
        m_result = HXR_OK;
        SaveAddrInfo(pRawInfo);
        hx_freeaddrinfo(pRawInfo);
    }
    else
    {
        HXLOGL3(HXLOG_NETW, "AddrInfoTask[%p]::Execute(): failed; sock err = %ld",this, res);
        m_result = TranslateResolveErr(res);
    }

    HXLOGL3(HXLOG_NETW, "AddrInfoTask[%p]::Execute(): done; result = %08x", this, m_result);
}

void AddrInfoTask::SaveAddrInfo(const struct addrinfo* pRawInfo)
{
    HXLOGL3(HXLOG_NETW, "AddrInfoTask[%p]::SaveAddrInfo()", this);

    HX_ASSERT(pRawInfo);
    HX_ASSERT(0 == m_ppAddr);
    HX_ASSERT(0 == m_infoCount);

    //
    // ADDRINFO => IHXSockAddr
    //
    CHXSimpleList list;
    m_result = HXPosixSockUtil::CreateAddrInfo(m_pNetServices, pRawInfo, list);
    if(HXR_OK == m_result)
    {
        SaveAddrInfoHelper(list);
    }
}






// class GetHostByNameTask
class GetHostByNameTask
: public ResolverTask
{
public:
    GetHostByNameTask(IHXNetServices* pNetServices, CHXResolver* pResolver);
    HX_RESULT Init(const char* pszHostName);

//HXTaskManager::Task
    void Execute();

private:
    void SaveAddrInfo(const struct hostent* pEntry);
};

GetHostByNameTask::GetHostByNameTask(IHXNetServices* pNetServices, CHXResolver* pResolver)
: ResolverTask(pNetServices, pResolver)
{
}


void GetHostByNameTask::Execute()
{
    HXLOGL3(HXLOG_NETW, "GetHostByNameTask[%p]::Execute()", this);

    const struct hostent* pEntry = hx_gethostbyname(m_node);
    if(pEntry)
    {
        m_result = HXR_OK;
        SaveAddrInfo(pEntry);
    }
    else
    {
        int res = hx_lastsockerr();
        HXLOGL3(HXLOG_NETW, "GetHostByNameTask[%p]::Execute(): failed; sock err = %ld", this, res);
        m_result = TranslateResolveErr(res);
    }

    HXLOGL3(HXLOG_NETW, "GetHostByNameTask[%p]::Execute(): done; result = %08x", this, m_result);
    
}




void GetHostByNameTask::SaveAddrInfo(const struct hostent* pEntry)
{
    HXLOGL3(HXLOG_NETW, "GetHostByNameTask[%p]::SaveAddrInfo()", this);

    HX_ASSERT(pEntry);
    HX_ASSERT(0 == m_ppAddr);
    HX_ASSERT(0 == m_infoCount);

    // service should be port number string
    HX_ASSERT(strspn(m_service, "0123456789") == m_service.GetLength());
    UINT32 port = strtoul(m_service, 0, 0);
    HX_ASSERT(port <= 0xffff);

    // convert from ADDRINFO format to collection of IHXSockAddr*
    CHXSimpleList list;
    m_result = HXPosixSockUtil::CreateAddrInfo(m_pNetServices, pEntry, UINT16(port), list);
    if(HXR_OK == m_result)
    {
       SaveAddrInfoHelper(list); 
    }
}


// class NameInfoTask
class NameInfoTask
: public HXTaskManager::Task
{
public:
    NameInfoTask(CHXResolver* pResolver);
    ~NameInfoTask();
    HX_RESULT Init(IHXSockAddr* pAddr, UINT32 uFlags);
    HX_RESULT GetResult(CHXString& node, CHXString& service) const;

//HXTaskManager::Task
    void Execute();
    void OnTaskComplete(HX_RESULT hr);
private:
    CHXResolver* m_pResolver;

    // input
    IHXSockAddr* m_pAddr;
    UINT32  m_flags;

    // output
    CHXString m_node;
    CHXString m_service;
    HX_RESULT m_result;
};

HX_RESULT NameInfoTask::GetResult(CHXString& node, CHXString& service) const
{
    node = m_node;
    service = m_service;
    return m_result;
}


NameInfoTask::NameInfoTask(CHXResolver* pResolver)
: m_pResolver(pResolver)
, m_result(HXR_FAIL)
, m_pAddr(0)
, m_flags(0)
{
    HX_ASSERT(m_pResolver);
}

NameInfoTask::~NameInfoTask()
{
    HX_RELEASE(m_pAddr);
}

HX_RESULT NameInfoTask::Init(IHXSockAddr* pAddr, UINT32 uFlags)
{
    HX_ASSERT(pAddr);
    m_pAddr = pAddr;
    m_pAddr->AddRef();

    m_flags = uFlags;
    return HXR_OK;
}

// Execute helper
inline
int TranlateHXFlag(UINT32 mask, UINT32 test, int transValue )
{
    if( (mask & test) == test )
    {
        return transValue;
    }
    return 0;
}

void NameInfoTask::Execute()
{
    HXLOGL3(HXLOG_NETW, "NameInfoTask[%p]::Execute()", this);
    struct sockaddr_storage sa;
    HXPosixSockUtil::GetSockAddr(m_pAddr, sa);

    char* pNode = m_node.GetBuffer(NI_MAXHOST);
    char* pServ = m_service.GetBuffer(NI_MAXSERV);
    if(pNode && pServ)
    {
#if(1) //XXXLCM this is unecessary if we can assume HX_NI_XXX and NI_XXX are equivalent
        int flags = 0;
        flags |= TranlateHXFlag(m_flags, HX_NI_NOFQDN,       NI_NOFQDN);
        flags |= TranlateHXFlag(m_flags, HX_NI_NUMERICHOST,  NI_NUMERICHOST);
        flags |= TranlateHXFlag(m_flags, HX_NI_NAMEREQD,     NI_NAMEREQD);
        flags |= TranlateHXFlag(m_flags, HX_NI_NUMERICSERV,  NI_NUMERICSERV);
        flags |= TranlateHXFlag(m_flags, HX_NI_DGRAM,        NI_DGRAM);
#else
        int flags = m_flags;
#endif

        // NI_NOFQDN - returns 'www' for 'www.foo.com' (useful!)
        // NI_NUMERICSERV - if not supplied and port does not resolve to well-known service GetNameInfo() will fail
        // HX_NI_NUMERICHOST - returns IP ('xxx.xxx.xxx.xx') instead of name
        int res = hx_getnameinfo(reinterpret_cast<struct sockaddr*>(&sa), sizeof(struct sockaddr_storage), pNode, NI_MAXHOST, 
                                pServ, NI_MAXSERV, flags);
        if (res != 0)
        {
            res = hx_lastsockerr();
            HXLOGL3(HXLOG_NETW, "NameInfoTask[%p]::Execute(): sock err = %ld", this, res);
            m_result = TranslateResolveErr(res);
        }
    }
    else
    {
        m_result = HXR_OUTOFMEMORY;
    }
    m_node.ReleaseBuffer(-1);
    m_service.ReleaseBuffer(-1);

    HXLOGL3(HXLOG_NETW, "NameInfoTask[%p]::Execute(): done; result = %08x", this, m_result);
}

void NameInfoTask::OnTaskComplete(HX_RESULT hr)
{
    m_pResolver->OnNameInfoTaskComplete(hr, this);
}


//
// class HXResolverTaskManager 
// 
// enables us to detect when the last HXTaskManager instance is destroyed
//
class HXResolverTaskManager 
: public HXTaskManager
{
public:
    static HX_RESULT GetInstance(HXTaskManager*& pTaskMgr, IUnknown* pContext);
protected:
    void FinalRelease();
    HXResolverTaskManager(UINT32 taskDoneMsg) 
        : HXTaskManager(taskDoneMsg) {}
};

// ref-counted singleton
static HXResolverTaskManager* g_pTaskMgrInstance;

void HXResolverTaskManager::FinalRelease()
{
    HXLOGL3(HXLOG_NETW, "HXResolverTaskManager::FinalRelease(): destroying instance %p", this);

    // indicate that no more instances exist
    g_pTaskMgrInstance = 0;

    // continue final release (delete this)
    HXTaskManager::FinalRelease();
}

HX_RESULT HXResolverTaskManager::GetInstance(HXTaskManager*& pTaskMgr, IUnknown* pContext)
{
    HX_RESULT hr = HXR_OK;

    // arbitrary thread pool size for the resolver
    const UINT32 RESOLVER_THREAD_POOL_COUNT = 3;

    if (g_pTaskMgrInstance)
    {
        pTaskMgr = g_pTaskMgrInstance;
        pTaskMgr->AddRef();
    }
    else
    {
        g_pTaskMgrInstance = new HXResolverTaskManager(HXMSG_TASKDONE_1);
        if( g_pTaskMgrInstance )
        {
            HXLOGL3(HXLOG_NETW, "HXResolverTaskManager::GetInstance(): created instance %p", g_pTaskMgrInstance);
            g_pTaskMgrInstance->AddRef();
#ifndef _WINCE			
            hr = g_pTaskMgrInstance->Init(pContext, RESOLVER_THREAD_POOL_COUNT);
#else
            hr = g_pTaskMgrInstance->Init(pContext,RESOLVER_THREAD_POOL_COUNT, THREAD_PRIORITY_NORMAL);
#endif
            if (FAILED(hr))
            {
                HX_ASSERT(false);
                HX_RELEASE(g_pTaskMgrInstance);
            }
            else
            {
                pTaskMgr = g_pTaskMgrInstance;
            }
        }
        else 
        {
            hr = HXR_OUTOFMEMORY;
        }
    }

    return hr;
}



// IUnknown
BEGIN_INTERFACE_LIST_NOCREATE(CHXResolver)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXResolve)
END_INTERFACE_LIST


CHXResolver::CHXResolver(IUnknown* pContext, IHXNetServices* pServices) 
: m_pResponse(0)
, m_pTaskManager(0)
, m_pContext(NULL)
{
    m_pNetServices = pServices;
    HX_ASSERT(m_pNetServices);
    m_pNetServices->AddRef();

    m_pContext = pContext;
    HX_ADDREF(m_pContext);
}

CHXResolver::~CHXResolver()
{
    Close();
}

STDMETHODIMP
CHXResolver::Init(IHXResolveResponse* pResponse)
{
    HX_ASSERT(!m_pResponse);
    HX_ASSERT(pResponse);

    if( m_pResponse)
    {
        return HXR_UNEXPECTED;
    }
    if(!pResponse)
    {
        return HXR_INVALID_PARAMETER;
    }

    // ensure socket library is loaded
    HX_RESULT hr = m_netDrvLoader.EnsureLoaded(m_pContext);
    if(FAILED(hr))
    {
        return hr;
    }

    m_pResponse = pResponse;
    m_pResponse->AddRef();

    // for executing blocking resolver calls on a thread pool worker thread
    return HXResolverTaskManager::GetInstance(m_pTaskManager, m_pContext);
}

STDMETHODIMP
CHXResolver::Close()
{
    HXLOGL3(HXLOG_NETW, "CHXResolver[%p]::Close()", this);
    
    // release the response before the task manager so we don't
    // call response when outstanding tasks are aborted
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pNetServices);

    // task manager will abort outstanding resolver tasks 
    // associated with this instance
    HX_RELEASE(m_pTaskManager);
    HX_RELEASE(m_pContext);
    return HXR_OK;
}

void CHXResolver::OnNameInfoTaskComplete(HX_RESULT hr, NameInfoTask* pTask)
{
    HXLOGL3(HXLOG_NETW, "CHXResolver[%p]::OnNameInfoTaskComplete() (response = %p)", this, m_pResponse);
    HX_ASSERT(pTask);
    HX_ASSERT(m_pResponse || HXR_ABORT == hr);

    CHXString node, service;
    if(HXR_OK == hr)
    {
        hr = pTask->GetResult(node, service);
    }

    if (m_pResponse)
    {
        m_pResponse->GetNameInfoDone(hr, node, service);
    }
}

void CHXResolver::OnResolverTaskComplete(HX_RESULT hr, ResolverTask* pResolverTask)
{
    HXLOGL3(HXLOG_NETW, "CHXResolver[%p]::OnResolverTaskComplete(): task = 0x%p (response = %p)", this, pResolverTask, m_pResponse);
    HX_ASSERT(pResolverTask);
    HX_ASSERT(m_pResponse || HXR_ABORT == hr);

    IHXSockAddr** ppInfo = 0;
    UINT32 count = 0;
    if(HXR_OK == hr)
    {
        hr = pResolverTask->GetResult(ppInfo, count);

#if defined(HELIX_FEATURE_LOGLEVEL_4) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
        if (SUCCEEDED(hr))
        {
            for (UINT32 idx = 0; idx < count; ++idx)
            {
                IHXSockAddr* pAddr = ppInfo[idx];
                HX_ASSERT(pAddr);
                if (pAddr)
                {
                    IHXBuffer* pBuf = 0;
                    pAddr->GetAddr(&pBuf);
                    HX_ASSERT(pBuf);
                    if (pBuf)
                    {
                        HXLOGL4(HXLOG_NETW, "CHXResolver[%p]::OnResolverTaskComplete(): addr = '%s'", this, pBuf->GetBuffer());
                        HX_RELEASE(pBuf);
                    }
                }
            }
        }
#endif
        //XXXLCM copy ppInfo, add to cache
    }

    if (m_pResponse)
    {
        m_pResponse->GetAddrInfoDone(hr, count, ppInfo);
    }
   
}


STDMETHODIMP
CHXResolver::GetAddrInfo(const char* pNode, 
                        const char* pServ, 
                        IHXAddrInfo* pHints)
{
    HXLOGL3(HXLOG_NETW, "CHXResolver[%p]::GetAddrInfo(): starting task...", this);

    if (pHints)
    {
        HX_ASSERT(false); // XXXLCM see AddrInfoTask::Execute()
        return HXR_INVALID_PARAMETER;
    }

    HX_ASSERT(m_pResponse); // call Init() first
    HX_ASSERT(m_pTaskManager);
    HX_ASSERT(pNode);
    if(!pNode)
    {
        return HXR_INVALID_PARAMETER;
    }

    // XXXLCM check pNode to see if it is an IP string ("xxx.xxx.xxx.xxx")

    ResolverTask* pTask = 0;
    if(hx_netdrv_has_ipv6_name_resolution())
    {
        pTask = new AddrInfoTask(m_pNetServices, this);
    }
    else
    {
        // IPv6 API not available. Use gethostbyname() for underlying resolver.
        pTask = new GetHostByNameTask(m_pNetServices, this);
    }

    HX_RESULT hr = HXR_OUTOFMEMORY;
    if(pTask)
    {
        pTask->AddRef();
        hr = pTask->Init(pNode, pServ, pHints);
        if(HXR_OK == hr)
        {
            // wait for OnTaskComplete()...
            m_pTaskManager->AddTask(pTask);
        }
        pTask->Release();
        
    }
    
    return hr;

}


STDMETHODIMP
CHXResolver::GetNameInfo(IHXSockAddr* pAddr, UINT32 uFlags)
{
    HXLOGL3(HXLOG_NETW, "CHXResolver[%p]::GetNameInfo(): starting task...", this);

    HX_ASSERT(pAddr);
    if(!pAddr)
    {
        return HXR_INVALID_PARAMETER;
    }

    NameInfoTask* pTask = 0;
    if(hx_netdrv_has_ipv6_name_resolution())
    {
        pTask = new NameInfoTask(this);
    }
    else
    {
        HX_ASSERT(false);
        return HXR_NOTIMPL; //XXXLCM implement for ipv4?
    }

    HX_RESULT hr = HXR_OUTOFMEMORY;
    if(pTask)
    {
        pTask->AddRef();
        HX_RESULT hr = pTask->Init(pAddr, uFlags);
        if(HXR_OK == hr)
        {
            // wait for OnNameInfoTaskComplete()...
            m_pTaskManager->AddTask(pTask);
        }
        pTask->Release();
    }
 
    return hr;
}

