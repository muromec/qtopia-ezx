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

#include "hxcom.h"
#include "hxnet.h"

#include "hxsymbiannet.h"
#include "hxsymbianresolver.h"
#include "hxsymbiansocket.h"
#include "hxsymbiansockaddr.h"
#include "hxtlogutil.h"

static const ULONG32 DefaultRetryCount = 2;

CHXResolver::CHXResolver(IHXNetServices* pServices, IUnknown* pContext):
    CActive(EPriorityStandard)
    ,m_ulRefCount(0)
    ,m_pContext(pContext)
    ,m_pResponse(NULL)
    ,m_pNetServices(pServices)
    ,m_pAPManager(NULL)
    ,m_pAPResponse(NULL)
    ,m_bInitialized(FALSE)
    ,m_ulRetryCount(0)
    ,m_pRequest(NULL)
{
    HX_ASSERT(m_pNetServices);
    HX_ASSERT(m_pContext);
    HX_ADDREF(m_pNetServices);
    HX_ADDREF(m_pContext);
    CActiveScheduler::Add(this);
}

CHXResolver::~CHXResolver()
{
    Close();
    HX_RELEASE(m_pAPManager);
    HX_RELEASE(m_pNetServices);
    HX_RELEASE(m_pContext);
    HX_DELETE(m_pRequest);
    HX_RELEASE(m_pResponse);
    if (m_pAPResponse)
    {
        m_pAPResponse->ClearPointers();
    }
    HX_RELEASE(m_pAPResponse);
}

STDMETHODIMP CHXResolver::QueryInterface(THIS_
               REFIID riid,
               void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXResolve*)this },
        { GET_IIDHANDLE(IID_IHXResolve), (IHXResolve*) this },
    };
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) CHXResolver::AddRef(THIS)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)CHXResolver::Release(THIS)
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
CHXResolver::Init(IHXResolveResponse* pResponse)
{
    HX_RESULT hxr = HXR_OK;

    if(pResponse)
    {
        HX_RELEASE(m_pResponse);
        m_pResponse = pResponse;
        HX_ADDREF(m_pResponse);
    }

    m_pNetServices->QueryInterface(IID_IHXAccessPointManager,
            (void**)&m_pAPManager);
    if (!m_pAPManager)
    {
        hxr = HXR_FAILED;
    }
    else if ( (m_pAPResponse = new HXAccessPointConnectResp(this,
                   static_APConnectDone)) == NULL)
    {
        hxr = HXR_OUTOFMEMORY;
    }
    else
    {
        HX_ADDREF(m_pAPResponse);
    }

    return hxr;
}

STDMETHODIMP
CHXResolver::Close()
{
    if(m_bInitialized)
    {
        if (IsActive())
            Cancel();
        m_bInitialized = FALSE;
        m_resolver.Close();
    }
    return HXR_OK;
}

STDMETHODIMP
CHXResolver::GetAddrInfo(const char* pNode,
                        const char* pServ,
                        IHXAddrInfo* pHints)
{
    HXLOGL4(HXLOG_NETW, "CHXResolver::GetAddrInfo this=%x", this);
    HX_RESULT res = HXR_OK;
    UINT16 port = 0;

      // Convert the service to native order
    if (pServ)
    {
      port = (UINT16) atoi(pServ);
    }

    HBufC *node = HBufC::New(strlen(pNode));
    if(!node)
        return HXR_OUTOFMEMORY;

    TInt len = strlen(pNode);
    TPtr8 ptr8((TUint8*)pNode, len, len);
    node->Des().Copy(ptr8);

    HX_DELETE(m_pRequest);
    m_pRequest = new Request();
    if(!m_pRequest)
    {
        delete node;
        return HXR_OUTOFMEMORY;
    }
    else
    {
        m_pRequest->m_pNode  = node;
        m_pRequest->m_uPort  = port;		
    }

    m_type = EResolvingByName;
    // connect to the access point
    res = m_pAPManager->Connect(m_pAPResponse);

    return res;
}


STDMETHODIMP
CHXResolver::GetNameInfo(IHXSockAddr* pAddr, UINT32 uFlags)
{

    HX_RESULT res = HXR_OK;

    HX_DELETE(m_pRequest);
    m_pRequest = new Request();
    if(!m_pRequest)
    {
        return HXR_OUTOFMEMORY;
    }
    else
    {
        IHXSockAddr *addr;
        pAddr->Clone(&addr);
        m_pRequest->m_ppAddress[0]  = addr;
        m_pRequest->m_uVecLen   = 1;
    }

    m_type = EResolvingByAddress;
    // connect to the access point
    res = m_pAPManager->Connect(m_pAPResponse);
    return res;
}


void CHXResolver::static_APConnectDone(void* pObj, HX_RESULT status)
{
    HXLOGL3(HXLOG_NETW, "CHXResolver::static_APConnectDone Start %d", status);
    CHXResolver* pResolver = (CHXResolver*)pObj;

    if (pResolver)
    {
        pResolver->OnAccessPointConnect(status);
    }
}


void CHXResolver::OnAccessPointConnect(HX_RESULT status)
{
    if (HXR_OK == status)
    {
        if(!m_bInitialized)
        {
            status = HXR_DNR;
            // XXXAG better to include in com i/f
            if(m_resolver.Open( ((HXSymbianAccessPointManager*)m_pAPManager)->
                                GetRSocketServ(),
                                KAfInet, 
                                KProtocolInetUdp, 
                                ((HXSymbianAccessPointManager*)m_pAPManager)->
                                GetRConnection()) == KErrNone)
            {
                status = HXR_OK;
                m_bInitialized = TRUE;
            }
        }
    }
    if(status == HXR_OK)
    {
        m_ulRetryCount = DefaultRetryCount;
        StartResolve();
    }
    else
    {
        DispatchResponse(status);
    }
}


void CHXResolver::DispatchResponse(HX_RESULT status)
{
    HX_RESULT  hxr = HXR_OK;
    if(!m_pResponse)
    {
        return ;
    }

    // add ref so that this object exists even if response handler removes
    // its ref.
    AddRef();
    if(m_type == EResolvingByName)
    {
        IHXSockAddr *addr = NULL;
        if(status == HXR_OK)
        {
            // create the IHXSockAddr
            if(m_nameEntry().iAddr.Family() == KAfInet)
            {
                hxr = m_pNetServices->CreateSockAddr(HX_SOCK_FAMILY_IN4, &addr);
                HXLOGL3(HXLOG_NETW, "CHXResolver::DispatchResponse IN4 hxr=%d", hxr);
            }
            else
            {
                hxr = m_pNetServices->CreateSockAddr(HX_SOCK_FAMILY_IN6, &addr);
                HXLOGL3(HXLOG_NETW, "CHXResolver::DispatchResponse IN6 hxr=%d", hxr);
            }
            // set the resolved address in IHXSockAddr.
            if(hxr == HXR_OK)
            {
                TInetAddr inetAddr = TInetAddr::Cast(m_nameEntry().iAddr);
                ((CHXInetSockAddr *)addr)->SetNative(inetAddr);
                addr->SetPort(m_pRequest->m_uPort);
            }
            else
            {
               status = hxr; 
            }
        }
        m_pResponse->GetAddrInfoDone(status, 1, &addr);		
        HX_RELEASE(addr);
        HX_DELETE(m_pRequest);
    }
    else if(m_type == EResolvingByAddress)
    {
        HBufC8 *hostName = NULL;
        TBuf8<8> port(0);
        const unsigned char *hostStr=NULL;
        const unsigned char *portStr=NULL;
        if(status == HXR_OK)
        {
            HBufC8 *hostName = HBufC8::New(m_nameEntry().iName.Length()+1);
            if(hostName)
            {
                hostStr = hostName->Des().PtrZ();
                port.Num((int)m_pRequest->m_ppAddress[0]->GetPort());
                portStr = port.PtrZ();
            }
            else
            {
                status = HXR_OUTOFMEMORY;
            }
        }
        m_pResponse->GetNameInfoDone(status, 
                                    (const char *)hostStr, 
                                    (const char *)portStr); 
        delete hostName;
        HX_DELETE(m_pRequest);
    }
    Release();
}


void
CHXResolver::RunL()
{
    HXLOGL3(HXLOG_NETW, "CHXResolver::RunL iStatus=%d", iStatus.Int());
    HX_RESULT status = HXR_DNR;
    HXBOOL bDoneResolving = TRUE;
    if (iStatus.Int() == KErrNone)
    {
        status = HXR_OK;
    }
    else if ((iStatus.Int() == KErrTimedOut) && (m_ulRetryCount > 0))
    {
        // Decrement retry count and try again
        m_ulRetryCount--;
        StartResolve();
        bDoneResolving = FALSE;
    }

    if (bDoneResolving)
    {
        DispatchResponse(status);
    }
}

void
CHXResolver::DoCancel()
{
    // do nothing
}

void 
CHXResolver::StartResolve()
{
    if(m_type == EResolvingByName)
    {
        m_resolver.GetByName(m_pRequest->m_pNode->Des(), m_nameEntry, iStatus);
    }
    else if(m_type == EResolvingByAddress)
    {
        TInetAddr addr;
        ((CHXInetSockAddr *) m_pRequest->m_ppAddress[0])->GetNative(addr);
        m_resolver.GetByAddress(addr, m_nameEntry, iStatus);
    }
    SetActive();
}

CHXResolver::Request::Request()
{
}

CHXResolver::Request::~Request()
{
    HX_DELETE(m_pNode);
    UINT32 counter;
    for(counter=0; counter < m_uVecLen; counter++)
    {
        HX_DELETE(m_ppAddress[counter]);
    }
}

