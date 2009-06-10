/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcloakedsocket.cpp,v 1.19 2008/08/18 21:50:48 ping Exp $
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
 *
 * ***** END LICENSE BLOCK ***** */

/*

1) Two sockets are created

a) GET socket: for server to client data (i.e., interleaved stream and rtsp response data)
b) POST socket: for client to server data (i.e., rtsp requests). RTSP is Bin64
   encoded in the post content section (that follows the header)

2) The server determines that cloaking is selected protocol based on URL it gets. Example:

GET /SmpDsBhgRl<unique 36 byte (with hyphens) guid> HTTP/1.0\r\n
POST /SmpDsBhgRl HTTP/1.0\r\n

For the POST we first specify a content length of 32767 so we can send
a lot of rtsp data over the channel to the server. That works well with
many proxies. However, there are some that will hold on the POST message
until it receives an amount of data specified by the content length. 
Eventually these proxies time out and either forward the post to the
destination or drop the connection.

3) Two modes: single-post (original) and multi-post (added later) cloaking.

Single post mode is used when both connections are successfully made
with the server with no latency difference between them.  When the
server gets both a GET and POST connection with one second of each other
it responds with "HTTP_OK" in GET response. This client reacts by operating
in single-post mode. Both channels are kept open for the duration of the 
session. One or more RTSP messages are sent over the same POST 
channel. The content length specifief for POST messages is 32767. Stream
data goes from server to client over the GET channel.

When the server does not receive an immediate (with 1 sec) POST connection
along with a GET connection (because a proxy holds on to the POST data) it
responds with "POST_NOT_RECEIVED" in the GET request. The client then operates in
multi-post mode. The GET channel remains open for the duration of the session.
Each RTSP message from client to server is encoded in a separate POST request. 
The content length is specified exactly.
  
 
4) Load balancing sometimes results in POST and GET going to different servers. To
deal with this we encode the server IP in the response to both the GET and POST. 

"accept: application/x-rtsp-tunnelled" requests that  server stick server IP in
its response header. If the server is old (doesn't support HTTPNG) it just responds
with no server IP as before. Otherwise it responsds with "x-server-ip-address".

Once both GET and POST socket are connected, we compare the server IP to see if they 
are different. If they are we re-connect the GET socket to the IP specified in the
POST response.

When we re-connect to a server we know about (via preferrred transport) we may optimize
this connection process by connecting first only to the POST (since we know we'll get
the IP address in the response). Only after we get the POST response will we then connect
the GET socket. This saves us from an excessive create/connect/close/re-connect on the
GET socket.
*/

#include "hlxclib/stdio.h"   
#include "hxcloakedsocket.h"

#include "safestring.h"

#include "hxsockutil.h"

#include "ihxpckts.h"

#include "hxbuffer.h" // CHXBuffer
#include "conn.h"     // TCP_BUF_SIZE
#include "dbcs.h"     // HXFindString
#include "hxmarsh.h"  // getshort()
#include "chxuuid.h"  // CHXuuid
#include "hxtick.h"   // HX_GET_TICKCOUNT
#include "rtsputil.h" // BinTo64
#include "hxfiles.h"  // CLSID_IHXRequest
#include "pckunpck.h"

// necessary for authentication
#include "hxmon.h"
#include "httppars.h"
#include "hxplgns.h"
#include "hxcore.h"
#include "hxcloaksockv2.h"
#include "md5.h"
#include "httpclk.h"

#include "hxtlogutil.h"
#include "hxassert.h"
#include "debug.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif


#if defined(HELIX_FEATURE_LOGLEVEL_3) || defined(HELIX_FEATURE_LOGLEVEL_ALL)

//
// HXLOG helpers
//

#define STATE_ENTRY(x) case x: return #x


const char*
HXCloakedSocket::TransConnectState(ConnectState state)
{
    switch (state)
    {
    STATE_ENTRY(csInitialized);
    STATE_ENTRY(csResolveProxy);
    STATE_ENTRY(csConnectBoth);
    STATE_ENTRY(csNGConnectPost);
    STATE_ENTRY(csNGWaitPostResponse);
    STATE_ENTRY(csNGConnectGet);
    STATE_ENTRY(csConnected);
    STATE_ENTRY(csClosed);
    }
    HX_ASSERT(false);
    return "bad connect state";
}

const char*
HXCloakedSocket::TransGetState(GetSockState state)
{
    switch (state)
    {
        STATE_ENTRY(gsConnectPending);
        STATE_ENTRY(gsReconnectPending);
        STATE_ENTRY(gsHeaderPending);
        STATE_ENTRY(gsReconnectHeaderPending);
        STATE_ENTRY(gsHeaderResponseCode);
        STATE_ENTRY(gsReadOptions);
        STATE_ENTRY(gsReadData);
        STATE_ENTRY(gsClosed);
    }
    HX_ASSERT(false);
    return "bad get state";
}

const char*
HXCloakedSocket::TransPostState(PostSockState state)
{
    switch (state)
    {
        STATE_ENTRY(psConnectPending);
        STATE_ENTRY(psReconnectPending);
        STATE_ENTRY(psHeaderPending);
        STATE_ENTRY(psOpen);
        STATE_ENTRY(psClosed);
    }
    HX_ASSERT(false);
    return "bad post state";
}
#undef STATE_ENTRY

#else
inline const char*
HXCloakedSocket::TransConnectState(ConnectState state) { return ""; }
inline const char*
HXCloakedSocket::TransGetState(GetSockState state) { return ""; }
inline const char*
HXCloakedSocket::TransPostState(PostSockState state) { return "";}
#endif //HELIX_FEATURE_LOGLEVEL_3 || HELIX_FEATURE_LOGLEVEL_ALL

//Return a random number in [beg, int].
static inline int RandBetween(int beg, int end )
{
    return beg + rand()%(end-beg+1);
}

static inline void _CreateRandomString(CHXString& str )
{
    str = "";

    char c;
    int  nDigits = RandBetween(12, 20 );
    while( nDigits-- > 0 )
    {
        c = '\0';
        while( !isalnum(c) )
            c = RandBetween('0', 'z');
        str += c;
    }
}


#define QUEUE_START_SIZE                512
#define HTTPCLOAK_POSTRESPONSE_TIMEOUT  2000
#define MAX_HTTP_METHOD_BUFSIZE         2048
#define HXGUID_SIZE                     16 // 16 byte GUID plus \r\n
#define DEFAULT_HTTP_HEADER_LENGTH      256
#define DEFAULT_OPTION_PADDING_LENGTH   16381

const UINT32 HTTP_UNAUTHORIZED = 401;
const UINT32 HTTP_PROXY_AUTHORIZATION_REQUIRED = 407;

UINT16 HXCloakedSocket::GetBytesAvailable(CHXSimpleList& list )
{
    UINT16 res = 0;
    CHXSimpleList::Iterator it = list.Begin();
    while( it != list.End() )
    {
        IHXBuffer* pBuf = (IHXBuffer*)(*it);
        res += (UINT16)pBuf->GetSize();
        ++it;
    }
    return res;
}


// helper
static HX_RESULT EnsureDataQueueCreated(CByteGrowingQueue*& pQueueOut,
                                        UINT16 maxSize = TCP_BUF_SIZE)
{
    HX_RESULT hr = HXR_OK;
    if (!pQueueOut)
    {
        pQueueOut = new CByteGrowingQueue(QUEUE_START_SIZE,1);
        if (pQueueOut && pQueueOut->IsQueueValid())
        {
            pQueueOut->SetMaxSize(maxSize);
        }
        else
        {
            hr = HXR_OUTOFMEMORY;
        }
    }
    return hr;
}

//
// really ugly and ineffient way to get rid
// of data at front of byte growing queue (from original code)
//
// XXXLCM fix this
//
static HX_RESULT DiscardData(CByteGrowingQueue* pQueue, UINT16 dropCount)
{
  
    UINT16 count = pQueue->GetQueuedItemCount();
    HX_ASSERT(dropCount <= count);

    BYTE* pTmpBuf = new BYTE[dropCount];
    if (!pTmpBuf)
    {
        return HXR_OUTOFMEMORY;
    }

    pQueue->DeQueue(pTmpBuf, dropCount);
    delete [] pTmpBuf;
    return HXR_OK;

}


//Helper function used for both DiscardData() and DeQueue().
HX_RESULT HXCloakedSocket::_DeQueue(CHXSimpleList& list,
                                    char*  pData,
                                    UINT16 unCount )
{
    HX_RESULT res  = HXR_FAIL;
    char*     pPtr = pData;
    
    while( 0<unCount )
    {
        IHXBuffer* pBuf      = (IHXBuffer*)list.RemoveHead();
        UINT16     unBufSize = (UINT16)pBuf->GetSize();
        
        if( unBufSize <= unCount )
        {
            if( pPtr )
            {
                memcpy(pPtr, pBuf->GetBuffer(), unBufSize );
                pPtr += unBufSize;
            }
            unCount -= unBufSize;
        }
        else
        {
            //Grab what we need from this buffer.
            if( pPtr )
            {
                memcpy( pPtr, pBuf->GetBuffer(), unCount );
                pPtr += unCount;
            }
                
            //Stick the remainder back onto the list.
            IHXBuffer* pTmp = NULL;
            res = m_pCCF->CreateInstance( CLSID_IHXBuffer, (void**)&pTmp);
            if( SUCCEEDED(res) )
            {
                res = pTmp->SetSize(unBufSize-unCount);
                if( SUCCEEDED(res) )
                {
                    memcpy( pTmp->GetBuffer(),
                            pBuf->GetBuffer()+unCount,
                            unBufSize-unCount );
                    list.AddHead(pTmp);
                    res = HXR_OK;
                }
                else
                {
                    HX_RELEASE(pTmp);
                }
            }
            unCount = 0;
        }
        HX_RELEASE(pBuf);
    }
    return res;
}

//Grab unCount bytes off of the timestamped queue and pass it back.
HX_RESULT HXCloakedSocket::DeQueue(CHXSimpleList& list,
                                   char*          pData,
                                   UINT16         unCount )
{
    HX_RESULT res = HXR_FAIL;
    
    if( pData && unCount>0 && unCount<=GetBytesAvailable(list) )
    {
        res = _DeQueue(list, pData, unCount);
    }
    return res;
}

HX_RESULT HXCloakedSocket::DiscardData(CHXSimpleList& list, UINT16 unCount)
{
    HX_RESULT res = HXR_FAIL;
    UINT16    unBytesAvailable = GetBytesAvailable(list);
    
    HX_ASSERT(unCount <= unBytesAvailable);
    
    if( unCount <= unBytesAvailable )
    {
        res = _DeQueue( list, NULL, unCount );
    }

    return res;
}

//Stick unCount bytes from pBuf onto the back of our timestamped
//buffer list. Since we don't have any time stamp information on this
//data we will just use a normal buffer.
HX_RESULT HXCloakedSocket::EnQueue(CHXSimpleList& list,
                                   const char*    pData,
                                   UINT16         unCount )
{
    HX_RESULT res  = HXR_FAIL;

    if( pData && unCount>0 )
    {
        //We have no timestamp information, so just use a normal
        //buffer.
        IHXBuffer* pTmp = NULL;
        res = m_pCCF->CreateInstance( CLSID_IHXBuffer, (void**)&pTmp);
        if( SUCCEEDED(res) )
        {
            res = pTmp->SetSize(unCount);
            if( SUCCEEDED(res) )
            {
                memcpy( pTmp->GetBuffer(), pData,  unCount );
                list.AddTail(pTmp);
                res = HXR_OK;
            }
            else
            {
                HX_RELEASE(pTmp);
                res = HXR_OUTOFMEMORY;
            }
        }
                
    }
    
    return res;
}


HXCloakedSocket::HXCloakedSocket(IUnknown* pContext, HXBOOL bBlockingResistant)
:    m_lRefCount(0)
    ,m_connectState(csClosed)
    ,m_getState(gsClosed)
    ,m_postState(psClosed)
    ,m_bIsHttpNg(FALSE)
    ,m_pContext(pContext)
    ,m_pResponse(NULL)
    ,m_pNetServices(NULL)
    ,m_pGetSock(NULL)
    ,m_pPostSock(NULL)
    ,m_pGetSockResponse(NULL)
    ,m_pPostSockResponse(NULL)
    ,m_pOutboundGetData(NULL)
    ,m_pInboundPostData(NULL)
    ,m_pOutboundPreEncodedPostData(NULL)
    ,m_pOutboundPostData(NULL)
    ,m_pOutBuf(NULL)
    ,m_pOutEncodedBuf(NULL)
    ,m_pInBuf(NULL)
    ,m_pGuid(NULL)
    ,m_bEndStreamPending(FALSE)
    ,m_bMultiPostMode(FALSE)
    ,m_pHTTPHeaderBuffer(NULL)
    ,m_proxyPort(0)
    ,m_pProxyAddr(NULL)
    ,m_pServerAddr(NULL)
    ,m_bDeletePadding(FALSE)
    ,m_uPadLength(10+DEFAULT_OPTION_PADDING_LENGTH) // 10 for the opts and 16381 for the padding
    ,m_pCloakValues(NULL)
    ,m_pCloakContext(NULL)
    ,m_pPreferredTransport(NULL)
    ,m_pPreferredTransportManager(NULL)
    ,m_pResolver(NULL)
    ,m_selectedEvents(0)
    ,m_eventFlags(0)
    ,m_pRedirectBuffer(NULL)
    ,m_pClientAuthConversationAuthenticator(NULL)
    ,m_family(HX_SOCK_FAMILY_NONE)
    ,m_bBlockingResistant(bBlockingResistant)
    ,m_NVPairValue("")
    ,m_NVPairName("")
    ,m_ResourceName("")
    ,m_pCCF(NULL)
{
    HX_ASSERT(m_pContext);
    m_pContext->AddRef();
}

HXCloakedSocket::~HXCloakedSocket()
{   
}

STDMETHODIMP HXCloakedSocket::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXSocket), (IHXSocket*)this },
            { GET_IIDHANDLE(IID_IHXResolveResponse), (IHXResolveResponse*)this },
            { GET_IIDHANDLE(IID_IHXClientAuthResponse), (IHXClientAuthResponse*)this },
            { GET_IIDHANDLE(IID_IHXHTTPProxy), (IHXHTTPProxy*)this },   
            { GET_IIDHANDLE(IID_IHXCloakedTCPSocket), (IHXCloakedTCPSocket*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXSocket*)this }
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXCloakedSocket::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXCloakedSocket::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP_(HXSockFamily)
HXCloakedSocket::GetFamily()
{
    // get family from arbitrarily chosen socket
    return m_pPostSock->GetFamily();
}
STDMETHODIMP_(HXSockType)
HXCloakedSocket::GetType()
{
    return HX_SOCK_TYPE_TCP;
}
STDMETHODIMP_(HXSockProtocol)
HXCloakedSocket::GetProtocol()
{
    return HX_SOCK_PROTO_ANY;
}

HX_RESULT
HXCloakedSocket::CreateSocketHelper(HXSockFamily family, IHXSocketResponse* pResponse, IHXSocket*& pSock)
{
    HX_ASSERT(pResponse);

    HX_RESULT rc = HXSockUtil::CreateSocket(m_pNetServices, pResponse, 
                            family, HX_SOCK_TYPE_TCP, HX_SOCK_PROTO_ANY, pSock);
    if (SUCCEEDED(rc))
    {
        // create unspecified addr for bind
        IHXSockAddr* pAnyAddr = 0;
        rc = m_pNetServices->CreateSockAddr(family, &pAnyAddr);
        if (SUCCEEDED(rc))
        {
            // bind
            rc = pSock->Bind(pAnyAddr);
            if (SUCCEEDED(rc))
            {
                // register to receive event notifications
                const UINT32 events = HX_SOCK_EVENT_CONNECT | HX_SOCK_EVENT_CLOSE | 
                                      HX_SOCK_EVENT_WRITE | HX_SOCK_EVENT_READ;
                rc = pSock->SelectEvents(events); 
            }
            HX_RELEASE(pAnyAddr);
        }
    }

    if (FAILED(rc))
    {
        // failed
        pSock->Close();
        HX_RELEASE(pSock);
    }
    return rc;
}

STDMETHODIMP
HXCloakedSocket::Init(HXSockFamily f, HXSockType t, HXSockProtocol p)
{
    //
    // only family can vary; otherwise only TCP socket type supported
    //
    if (t != HX_SOCK_TYPE_TCP || p != HX_SOCK_PROTO_ANY)
    {
        return HXR_INVALID_PARAMETER;
    }
    
    //Save our socket familytype.
    m_family = f;
    
    //
    // grab interfaces from context
    //
    m_pContext->QueryInterface(IID_IHXNetServices, (void**) &m_pNetServices);
    HX_ASSERT(m_pNetServices);
    if (!m_pNetServices)
    {
        return HXR_FAIL;
    }
    
    m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF );
    HX_ASSERT(m_pCCF);
    if( !m_pCCF )
    {
        return HXR_FAIL;
    }
    

    m_pContext->QueryInterface(IID_IHXPreferredTransportManager,
                               (void**)&m_pPreferredTransportManager); 


    m_pGetSockResponse = new SockResponse(this, TRUE);
    if (!m_pGetSockResponse)
    {
        return HXR_OUTOFMEMORY;
    }
    m_pGetSockResponse->AddRef();

    m_pPostSockResponse = new SockResponse(this, FALSE);
    if (!m_pPostSockResponse)
    {
        return HXR_OUTOFMEMORY;
    }
    m_pPostSockResponse->AddRef();

    //
    // create post (rtsp channel) and get (data channel) sockets
    //
    HX_RESULT rc = CreateSocketHelper(m_family, m_pGetSockResponse, m_pGetSock);
    if (FAILED(rc))
    {
        return rc;
    }

    rc = CreateSocketHelper(m_family, m_pPostSockResponse, m_pPostSock);
    if (FAILED(rc))
    {
        return rc;
    }

    // create resolver
    HX_ASSERT(!m_pResolver);
    rc = m_pNetServices->CreateResolver(&m_pResolver);
    if (FAILED(rc))
    {
        return rc;
    }

    rc = m_pResolver->Init(this);
    if (FAILED(rc))
    {
        return rc;
    }

    // queue for GET socket outbound data
    rc = EnsureDataQueueCreated(m_pOutboundGetData);
    if (HXR_OK != rc)
    {
        return rc;
    }

    // temp queue for POST socket unencoded outbound data
    /* Approx. MAX POST header size : 3000*/
    rc = EnsureDataQueueCreated(m_pOutboundPreEncodedPostData, (TCP_BUF_SIZE-1)/2 - 3000);
    if (HXR_OK != rc)
    {
        return rc;
    }

    // queue for POST socket outbound data (after encoding)
    rc = EnsureDataQueueCreated(m_pOutboundPostData);
    if (HXR_OK != rc)
    {
        return rc;
    }
    
    //
    // allocate temp work buffers
    //
    m_pInBuf = new char[TCP_BUF_SIZE];
    if (!m_pInBuf)
    {
        return HXR_OUTOFMEMORY;
    }
    m_pOutBuf = new char[TCP_BUF_SIZE];
    if (!m_pOutBuf)
    {
        return HXR_OUTOFMEMORY;
    }

    m_pOutEncodedBuf = new char[TCP_BUF_SIZE];
    if (!m_pOutEncodedBuf)
    {
        return HXR_OUTOFMEMORY;
    }

    rc = CreateGuid();
    if (HXR_OK == rc)
    {
        SetConnectState(csInitialized);
    }

    //Initialize our resource strings and hash value for the new
    //fall back cloaking model.
    _ResetResourceAndHash();
    
    return rc;

}


STDMETHODIMP
HXCloakedSocket::SetResponse(IHXSocketResponse* pResponse)
{
    HX_RELEASE(m_pResponse);

    m_pResponse = pResponse;
    if(m_pResponse)
    {
        m_pResponse->AddRef();
    }

    return HXR_OK;
}


HX_RESULT
HXCloakedSocket::GetServerAddrString(CHXString& str, HXBOOL urlFormat)
{
    HX_ASSERT(m_pServerAddr);

    IHXBuffer* pBuf = 0;
    HX_RESULT hr = m_pServerAddr->GetAddr(&pBuf);
    if (HXR_OK == hr)
    {
        if (urlFormat && m_pServerAddr->GetFamily() == HX_SOCK_FAMILY_IN6)
        {
            // format for placement in URL
            str += "[";
            str += (const char*)pBuf->GetBuffer();
            str += "]";
        }
        else
        {
           str = (const char*)pBuf->GetBuffer();
        }
        HX_RELEASE(pBuf);
    }
    else
    {
        str.Empty();
    }
    return hr;

}

HX_RESULT
HXCloakedSocket::SetServerAddr(IHXSockAddr* pAddr)
{
    if (m_pServerAddr != pAddr)
    {
        HX_RELEASE(m_pServerAddr);
        return pAddr->Clone(&m_pServerAddr);
    }
    return HXR_OK;
}

HX_RESULT
HXCloakedSocket::UpdateServerAddr(const char* pszIP)
{
    HX_ASSERT(m_pServerAddr);
    return HXSockUtil::SetAddr(m_pServerAddr, pszIP, m_pServerAddr->GetPort());
}

STDMETHODIMP
HXCloakedSocket::ConnectToOne(IHXSockAddr* pAddr)
{
    HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::ConnectToOne():");
    HXLOGL3(HXLOG_RTSP, "conn state = '%s'; get = '%s'; post = '%s'", TransConnectState(m_connectState), TransGetState(m_getState), TransPostState(m_postState));


    HX_ASSERT(csInitialized == m_connectState);
    HX_ASSERT(pAddr);
    if (!pAddr)
    {
        return HXR_INVALID_PARAMETER;
    }

    // save address
    HX_RESULT hr = SetServerAddr(pAddr);
    if (SUCCEEDED(hr))
    {
        // m_pCloakValues is only set by RTSP
        if (m_pPreferredTransportManager && m_pCloakValues)
        {
            IHXBuffer* pAddrString = 0;
            hr = m_pServerAddr->GetAddr(&pAddrString);
            if (SUCCEEDED(hr))
            {
                m_serverName = (const char*)pAddrString->GetBuffer();

                HX_RELEASE(m_pPreferredTransport);
                m_pPreferredTransportManager->GetPrefTransport( (const char*)pAddrString->GetBuffer(),
                                                               PTP_RTSP,
                                                               m_pPreferredTransport);
                HX_RELEASE(pAddrString);

                HX_ASSERT(m_pPreferredTransport);
                if (m_pPreferredTransport)
                {
                    m_bIsHttpNg = m_pPreferredTransport->GetHTTPNG();
                }
                hr = ConnectToOneHelper();
            }
        }
        else
        {
            hr = ConnectToOneHelper();
        }
    }

    return hr;
}

STDMETHODIMP
HXCloakedSocket::ConnectToAny(UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    HX_ASSERT(false); // XXXLCM fixme; socket must be initialized with HXR_SOCK_FAMILY_INANY
    return ConnectToOne(ppAddrVec[0]);
}



STDMETHODIMP
HXCloakedSocket::SelectEvents(UINT32 uEventMask)
{
    // save selected events
    m_selectedEvents = uEventMask;

    // set flag that indicates which event notifications need forwarding to response
    m_eventFlags = uEventMask;

    // ensure close events are cleared until we connect
    if (csConnected != m_connectState)
    {
        ClearEventFlag(HX_SOCK_EVENT_CLOSE);
    }

    return HXR_OK;
}


STDMETHODIMP
HXCloakedSocket::Peek(IHXBuffer** ppBuf)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXCloakedSocket::Read(IHXBuffer** ppBuf)
{
    //
    // Read from inbound data from the GET socket
    //

    HX_ASSERT(ppBuf);

    // re-enable event notification for read events
    SetEventFlag(HX_SOCK_EVENT_READ); 

    // special case: check for redirect buffer and return that
    if (m_pRedirectBuffer)
    {
        *ppBuf = m_pRedirectBuffer;
        m_pRedirectBuffer = 0;
        return HXR_OK;
    }

    HX_RESULT hr = HXR_OK;

    // inbound GET channel provides read data
    UINT16 count = m_InboundGetData.GetCount();
    if( count > 0 )
    {
        *ppBuf = (IHXBuffer*)m_InboundGetData.RemoveHead();
        if(m_InboundGetData.IsEmpty() && m_bEndStreamPending)
        {
            // just read to end of data; ensure close event sent
            SendCloseEvent(HXR_OK);
        }
    }
    else if (m_bEndStreamPending)
    {
        m_bEndStreamPending = FALSE;
        hr = HXR_SOCK_ENDSTREAM;
    }
    else
    {
        // no data to read
        hr = HXR_SOCK_WOULDBLOCK;
    }


    return hr;
}


STDMETHODIMP
HXCloakedSocket::ReadFrom(IHXBuffer** ppBuf, IHXSockAddr** ppAddr)
{
    HX_ASSERT(false); // tcp only socket
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXCloakedSocket::PeekFrom(IHXBuffer** ppBuf, IHXSockAddr** ppAddr)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXCloakedSocket::Write(IHXBuffer* pBuf)
{
    HX_RESULT hr = HXR_OK;

    //HX_ASSERT(csConnected == m_connectState);

    // add to pending write buffer list
    pBuf->AddRef();
    m_PendingWriteBuffers.AddTail((void*) pBuf);

    if (csConnected == m_connectState)
    {   
        // try to write immediately to post socket
        hr = TryPostSocketWrite();
        if (HXR_SOCK_WOULDBLOCK == hr)
        {
            // we accepted this buffer into pending buffer list; don't pass on to caller
            hr = HXR_OK;
        }
    }
   
    return hr;

}

STDMETHODIMP
HXCloakedSocket::WriteTo(IHXBuffer* pBuf, IHXSockAddr* pAddr)
{
    HX_ASSERT(false); // tcp only socket
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXCloakedSocket::Close()
{
    /* Send a final HTTP done message */
    if (gsReadData == m_getState)
    {
        SendHTTPDone(); // XXXLCM may not work if we clean up immediately after
    }

    DoCloseSockets(HXR_OK);
    if( m_pResolver )
    {
        m_pResolver->Close();
        HX_RELEASE(m_pResolver);
    }
    
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pGetSockResponse);
    HX_RELEASE(m_pPostSockResponse);
    HX_RELEASE(m_pResponse);

    while (m_PendingWriteBuffers.GetCount() > 0)
    {
        IHXBuffer* pBuffer = (IHXBuffer*) m_PendingWriteBuffers.RemoveHead();
        pBuffer->Release();
    }

    FlushQueues();
    HX_DELETE(m_pOutboundGetData);
    
    HX_DELETE(m_pInboundPostData);
    HX_DELETE(m_pOutboundPreEncodedPostData);
    HX_DELETE(m_pOutboundPostData);
    HX_VECTOR_DELETE(m_pInBuf);
    HX_VECTOR_DELETE(m_pOutBuf);
    HX_VECTOR_DELETE(m_pOutEncodedBuf);
    HX_RELEASE(m_pHTTPHeaderBuffer);
    HX_RELEASE(m_pRedirectBuffer);

    HX_RELEASE(m_pServerAddr);
    HX_RELEASE(m_pProxyAddr);
    HX_VECTOR_DELETE(m_pGuid);
    HX_RELEASE(m_pCloakValues);
    HX_RELEASE(m_pCloakContext);
   

    HX_RELEASE(m_pPreferredTransport);
    HX_RELEASE(m_pPreferredTransportManager);
    HX_RELEASE(m_pClientAuthConversationAuthenticator);
    HX_RELEASE(m_pNetServices);
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pContext);   

    return HXR_OK;
}

STDMETHODIMP
HXCloakedSocket::ReadV(UINT32 nVecLen, UINT32* puLenVec,IHXBuffer** ppBufVec)
{
    HX_ASSERT(false);
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXCloakedSocket::ReadFromV(UINT32 nVecLen, UINT32* puLenVec,
                                           IHXBuffer** ppBufVec,
                                           IHXSockAddr** ppAddr)
{
    HX_ASSERT(false);
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXCloakedSocket::GetLocalAddr(IHXSockAddr** ppAddr)
{
    if (m_pPostSock)
    {
        return m_pPostSock->GetLocalAddr(ppAddr);
    }
    return HXR_FAIL;
}

STDMETHODIMP
HXCloakedSocket::GetPeerAddr(IHXSockAddr** ppAddr)
{
    if (m_pPostSock)
    {
        return m_pPostSock->GetPeerAddr(ppAddr);
    }
    return HXR_FAIL;
}

STDMETHODIMP
HXCloakedSocket::SetAccessControl(IHXSocketAccessControl* pControl)
{
    HX_ASSERT(false);
    return HXR_NOTIMPL;
}


STDMETHODIMP
HXCloakedSocket::Listen(UINT32 uBackLog)
{
    HX_ASSERT(false);
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXCloakedSocket::Accept(IHXSocket** ppNewSock,IHXSockAddr** ppSource)
{
    HX_ASSERT(false);
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXCloakedSocket::GetOption(HXSockOpt name, UINT32* pval)
{
    HX_ASSERT(false);
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXCloakedSocket::SetOption(HXSockOpt name, UINT32 val)
{
    // XXXLCM only certain options make sense
    if(HX_SOCKOPT_APP_BUFFER_TYPE != name)
    {
        HX_ASSERT(false);
        return HXR_FAIL;
    }

    HX_RESULT hr = HXR_FAIL;
    if (m_pGetSock)
    {
        hr = m_pGetSock->SetOption(name, val);
        if (SUCCEEDED(hr))
        {
            hr = HXR_FAIL;
            if (m_pPostSock)
            {
                hr = m_pPostSock->SetOption(name, val);
            }
        }
    }
           
    return hr;
}

STDMETHODIMP
HXCloakedSocket::WriteV(UINT32 nVecLen,IHXBuffer** ppBufVec)
{
    HX_ASSERT(false);
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXCloakedSocket::WriteToV(UINT32 nVecLen,IHXBuffer** ppBufVec,IHXSockAddr* pAddr)
{
    HX_ASSERT(false);
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXCloakedSocket::CreateSockAddr(IHXSockAddr** ppAddr)
{
    HX_ASSERT(false);
    return HXR_NOTIMPL;
}

STDMETHODIMP
HXCloakedSocket::Bind(IHXSockAddr* pAddr)
{
    HX_ASSERT(false);
    return HXR_NOTIMPL;
}



//
// IHXResolveResponse
//
// the only name we resolve is the proxy name if we have it
//
STDMETHODIMP
HXCloakedSocket::GetAddrInfoDone(HX_RESULT status, UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    HXLOGL3(HXLOG_RTSP, "GetAddrInfoDone(): status = %08x", status);
    HXLOGL3(HXLOG_RTSP, "conn state = '%s'; get = '%s'; post = '%s'", TransConnectState(m_connectState), TransGetState(m_getState), TransPostState(m_postState));

    HX_RESULT hr = status;
    if (SUCCEEDED(hr))
    {
        // save proxy address
        HX_ASSERT (!m_pProxyAddr);
        hr = ppAddrVec[0]->Clone(&m_pProxyAddr);
        if (SUCCEEDED(hr))
        {
            HX_ASSERT(m_pProxyAddr);
            HX_ASSERT(csResolveProxy == m_connectState);
            SetConnectState(csInitialized);
    
            // do connect
            hr = ConnectToOneHelper();
        }
    }
    
    if (FAILED(hr))
    {
        // connect failure
        SetConnectState(csClosed);
        SendConnectEvent(HXR_NET_CONNECT);
    }   
    
    return HXR_OK;
}

//
// IHXResolveResponse
//
STDMETHODIMP
HXCloakedSocket::GetNameInfoDone(HX_RESULT status, 
                                       const char* pszNode, 
                                       const char* pszService)
{
    return HXR_UNEXPECTED;
}

//
// Note: network addresses and ports are in native byte order
//
STDMETHODIMP HXCloakedSocket::SetProxy(const char* pProxyHostName, UINT16 nPort)
{
    HX_ASSERT(pProxyHostName);
    if(pProxyHostName == 0 || *pProxyHostName == 0) 
    {
        return HXR_FAILED;
    }
    HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::SetProxy(): '%s':%u", pProxyHostName, nPort);

    m_proxyName = pProxyHostName;
    m_proxyPort = nPort;

    return HXR_OK;
}


STDMETHODIMP
HXCloakedSocket::InitCloak(IHXValues* pValues, 
                           IUnknown* pUnknown)
{
    HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::InitCloak()");
    HX_RESULT rc = HXR_OK;

    HX_RELEASE(m_pCloakValues);
    if (pValues)
    {
        m_pCloakValues = pValues;
        m_pCloakValues->AddRef();
    }
    HX_RELEASE(m_pCloakContext);
    if (pUnknown)
    {
        m_pCloakContext = pUnknown;
        m_pCloakContext->AddRef();
    }

    return HXR_OK;
}

void HXCloakedSocket::SetEventFlag(UINT32 flag)
{
    HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::SetEventFlag(): flag = %lu", flag);
    if (m_selectedEvents & flag)
    {
        m_eventFlags |= flag;
    }
}

void HXCloakedSocket::ClearEventFlag(UINT32 flag)
{
    HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::ClearEventFlag(): flag = %lu", flag);
    m_eventFlags &= ~flag;
}

HXBOOL HXCloakedSocket::IsEventFlagSet(UINT32 flag)
{
    return ((m_eventFlags & flag) == flag);
}

void HXCloakedSocket::EnsureEventSent(UINT32 event, HX_RESULT status)
{
    if (IsEventFlagSet(event))
    {
        ClearEventFlag(event);
        m_pResponse->EventPending(event, status);
    }
}

void
HXCloakedSocket::SendConnectEvent(HX_RESULT status)
{
    HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::SendConnectEvent(): status = %lu", status);
    EnsureEventSent(HX_SOCK_EVENT_CONNECT, status);
}

void
HXCloakedSocket::SendCloseEvent(HX_RESULT status)
{
    HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::SendCloseEvent(): status = %lu", status);
    EnsureEventSent(HX_SOCK_EVENT_CLOSE, status);
}

void
HXCloakedSocket::SendReadEvent()
{
    UINT32 count = m_InboundGetData.GetCount();
    if (m_pRedirectBuffer ||  (count && m_getState == gsReadData) || m_bEndStreamPending)
    {
        EnsureEventSent(HX_SOCK_EVENT_READ, HXR_OK);
    }
}

//
// helper for write event handlers; send data out on socket
//
HX_RESULT
HXCloakedSocket::WriteSocketHelper(CByteGrowingQueue* pOutboundData, IHXSocket* pSock)
{
    HX_ASSERT(pOutboundData);
    HX_ASSERT(pSock);

    HX_RESULT hr = HXR_OK;

    UINT16 count  = pOutboundData->GetQueuedItemCount();
    if (count > 0) 
    {
        // create IHXBuffer for socket
        IHXBuffer* pBuffer = NULL;
	CreateBufferCCF(pBuffer, m_pContext);
        if (pBuffer)
        {
            // allocate space
            hr = pBuffer->SetSize(count);
            if (HXR_OK == hr)
            {
                // copy outbound data to IHXBuffer
                pOutboundData->DeQueue((void*)pBuffer->GetBuffer(), count);

                //HXLOGL3(HXLOG_RTSP, "WriteSocketHelper(): sending:\r\n%.*s",  ( count > 220 ? 220 : count), pBuffer->GetBuffer() );
                
                // send
                hr = pSock->Write(pBuffer);
                if (FAILED(hr))
                {
                    // copy back if error (writing before connect succeeds?)
                    HX_ASSERT(false);
                    pOutboundData->EnQueue(pBuffer->GetBuffer(), count);
                }
                pBuffer->Release();
            }
        }
        else
        {
            hr = HXR_OUTOFMEMORY;
        }
    }
    return hr;
}

// called when write event received for socket 
HX_RESULT
HXCloakedSocket::HandleGetSockWriteEvent(HX_RESULT status)
{
    HX_ASSERT(m_pGetSock);

    HX_RESULT hr = status;
    if (SUCCEEDED(hr))
    {
        hr = TryGetSocketWrite();
    }
    else
    {
        DoCloseSockets(hr);
    }
    return hr;
}

HX_RESULT HXCloakedSocket::TryGetSocketWrite()
{
    HX_ASSERT(IsGetSockConnected());
    return WriteSocketHelper(m_pOutboundGetData, m_pGetSock);
}

// called when write event received for socket 
HX_RESULT
HXCloakedSocket::HandlePostSockWriteEvent(HX_RESULT status)
{
    HX_ASSERT(m_pPostSock);
 
    HX_RESULT hr = status;
    if (SUCCEEDED(hr))
    {
        hr = TryPostSocketWrite();
    }
    else
    {
        DoCloseSockets(hr);
    }
    return hr;
}
HX_RESULT HXCloakedSocket::TryPostSocketWrite()
{
    HX_RESULT hr = HXR_OK;

    if (IsPostSockConnected() && gsReadData == m_getState)
    {
        // send out pre-existing post data
        if(m_pOutboundPostData->GetQueuedItemCount() > 0)
        {
            HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::TryPostSocketWrite(): sending existing POST data");
            hr = WriteSocketHelper(m_pOutboundPostData, m_pPostSock);
            if (FAILED(hr))
            {
                return hr;
            }

        }
 
        HX_ASSERT(0 == m_pOutboundPostData->GetQueuedItemCount());

        // transfer data out to post message format and try to send the rest 
        if (m_PendingWriteBuffers.GetCount())
        {
            HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::TryPostSocketWrite(): converting pending data to POST message format");
            
            // prepare additional post data, possibly re-connecting socket for next multi-post mode post
            hr = PrepareNextPostSocketWrite();

            // socket reconnect may be triggered in above call
            if (FAILED(hr) || !IsPostSockConnected())
            {
                // we normally end up here in multi-post mode from re-connect
                return hr;
            }

            HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::TryPostSocketWrite(): sending (just created) POST data");
            hr = WriteSocketHelper(m_pOutboundPostData, m_pPostSock);
        }

    }
    return hr;
}

// send first and only GET (after connect or reconnect)
HX_RESULT HXCloakedSocket::SendGet()
{
    HX_RESULT hr = PrepareGetMessage();
    if (HXR_OK == hr)
    {
        TryGetSocketWrite();
    }
    return hr;
}

// send first post
HX_RESULT HXCloakedSocket::SendPost()
{       
    HX_RESULT hr = PreparePostMessage(NULL, 0);
    if (HXR_OK == hr)
    {
        hr = WriteSocketHelper(m_pOutboundPostData, m_pPostSock);
    }
    return hr;
}

//
// helper to perform actual connect on get and post sockets based on current state
//
HX_RESULT
HXCloakedSocket::ConnectToOneHelper()
{
    HX_RESULT   rc = HXR_OK;

    HXBOOL bNeedToResolveProxyAddr = (!m_pProxyAddr && !m_proxyName.IsEmpty());

    switch (m_connectState)
    {
    case csInitialized:
        if (bNeedToResolveProxyAddr)
        {
            HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::ConnectToOneHelper(): resolving '%s' proxy", (const char*)m_proxyName);

            // resolve proxy before connecting...
            SetConnectState(csResolveProxy);

            char szPort[HX_PORTSTRLEN];
            SafeSprintf(szPort, HX_PORTSTRLEN, "%u", m_proxyPort);
            rc = m_pResolver->GetAddrInfo(m_proxyName, szPort, 0);
        }
        else
        {
            if (m_bIsHttpNg)
            {
                HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::ConnectToOneHelper(): HTTPNG: starting post connect");

                // new NG method: connect post socket first
                SetConnectState(csNGConnectPost);
                SetPostState(psConnectPending);
                rc = DoConnect(m_pPostSock);
                if (FAILED(rc))
                {
                    SetConnectState(csClosed);
                } 
            }
            else
            {
                HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::ConnectToOneHelper(): OLD: connecting post and get sockets");

                // old pre http-ng method: connect both sockets
                SetConnectState(csConnectBoth);
                SetGetState(gsConnectPending);
                rc = DoConnect(m_pGetSock);
                if (SUCCEEDED(rc))
                {
                    SetPostState(psConnectPending);
                    rc = DoConnect(m_pPostSock);
                }
                if (FAILED(rc))
                {
                    SetConnectState(csClosed); //XXXLCM what if get sock is ok, post fails
                    SetGetState(gsConnectPending);
                    SetPostState(psConnectPending);
                }
            }
        }
        break;

    case csNGWaitPostResponse:
      
        HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::ConnectToOneHelper(): HTTPNG: starting get connect");

        HX_ASSERT(!bNeedToResolveProxyAddr); // should have been done earlier
        SetConnectState(csNGConnectGet);

        // now we can connect get socket
        rc = DoConnect(m_pGetSock);
        if (FAILED(rc))
        {
            SetConnectState(csClosed);
        }
        break;
   
    default:
        HX_ASSERT(false);
        break;
    }

    return rc;
}



HX_RESULT
HXCloakedSocket::CheckAndHandleBothSocketsConnected()
{
    HX_RESULT hr = HXR_OK;

    HX_ASSERT(csConnectBoth == m_connectState);

    if (IsGetSockConnected() && IsPostSockConnected())
    {
        // both sockets are now connected
        SetConnectState(csConnected);
        // re-enable close event
        SetEventFlag(HX_SOCK_EVENT_CLOSE);

        // server type unknown so far; send both POST and GET
        hr = SendGet();
        if (HXR_OK == hr)
        {
            hr = SendPost(); //XXXLCMTEST comment out to test post-not-received handling
        }
        SendConnectEvent(hr);
    }
    return hr;
}

void HXCloakedSocket::SetConnectState(ConnectState state)
{
    if (m_connectState != state)
    {
        HXLOGL3(HXLOG_RTSP, "conn state '%s' => '%s' (get = '%s'; post = '%s')",
            TransConnectState(m_connectState), TransConnectState(state),
            TransGetState(m_getState), TransPostState(m_postState));
        m_connectState = state;
    }
}

void HXCloakedSocket::SetGetState(GetSockState state)
{
    if (m_getState != state)
    {
        HXLOGL3(HXLOG_RTSP, "get state '%s' => '%s' (post = '%s')",
            TransGetState(m_getState), TransGetState(state),
            TransPostState(m_postState));
        m_getState = state;
    }
}

void HXCloakedSocket::SetPostState(PostSockState state)
{
    if (m_postState != state)
    {
        HXLOGL3(HXLOG_RTSP, "post state '%s' => '%s' (get = '%s')",
            TransPostState(m_postState), TransPostState(state),
            TransGetState(m_getState));
        m_postState = state;
    }
}


void 
HXCloakedSocket::HandleGetSockConnectEvent(HX_RESULT status)
{
    HXLOGL3(HXLOG_RTSP, "HandleGetSockConnectEvent(): status = %08x", status);
    HXLOGL3(HXLOG_RTSP, "conn state = '%s'; get = '%s'; post = '%s'", TransConnectState(m_connectState), TransGetState(m_getState), TransPostState(m_postState));

    if (FAILED(status))
    {
        SetConnectState(csClosed);
        SendConnectEvent(HXR_NET_CONNECT);
        return;
    }

    HX_ASSERT(m_getState == gsConnectPending || m_getState == gsReconnectPending);

    switch (m_connectState)
    {
    case csConnectBoth:
        {
            // waiting for both sockets to connect (default connect case)
            SetGetState(gsHeaderPending);
            CheckAndHandleBothSocketsConnected();
        }
        break;
    case csNGConnectGet:
        {
            // http-ng server connect case: post sock already connected; now both are connected
            HX_ASSERT(psConnectPending != m_postState);
            SetGetState(gsHeaderPending);

            // using server IP returned from POST response
            HX_ASSERT(m_bIsHttpNg);
            // HTTP-NG: connect process is complete once GET socket connected 
            SetConnectState(csConnected);

            // ensure foreign addr is that we actually connected to
            HX_RELEASE(m_pServerAddr);
            HX_RESULT hr = m_pGetSock->GetPeerAddr(&m_pServerAddr);
            if (SUCCEEDED(hr))
            {
                hr = SendGet();
            }

            if (FAILED(hr))
            {
                DoCloseSockets(hr);
            }
            SendConnectEvent(hr);
        }
        break;
    case csConnected:
        {
            // must be get reconnect; handle get re-connect
            HX_ASSERT(gsReconnectPending == m_getState);
            if (gsReconnectPending == m_getState)
            {
                SetGetState(gsReconnectHeaderPending);
                // ensure foreign addr is that we actually connected to
                HX_RELEASE(m_pServerAddr);
                HX_RESULT hr = m_pGetSock->GetPeerAddr(&m_pServerAddr);
                if (SUCCEEDED(hr))
                {
                    hr = SendGet();
                }

                if (FAILED(hr))
                {
                    DoCloseSockets(hr);
                }
            }
        }
        break;
    default:
        HX_ASSERT(false);
        break;
    }
}

void 
HXCloakedSocket::HandlePostSockConnectEvent(HX_RESULT status)
{
    HXLOGL3(HXLOG_RTSP, "HandlePostSockConnectEvent(): status = %08x", status);
    HXLOGL3(HXLOG_RTSP, "conn state = '%s'; get = '%s'; post = '%s'", TransConnectState(m_connectState), TransGetState(m_getState), TransPostState(m_postState));

    if (FAILED(status))
    {
        HX_ASSERT(m_postState != psReconnectPending);
        SetConnectState(csClosed);
        SendConnectEvent(HXR_NET_CONNECT);
        return;
    }

    HX_ASSERT(m_postState == psConnectPending || m_postState == psReconnectPending);
   
    switch (m_connectState)
    {
    case csConnectBoth:
        {
            // waiting for both sockets to connect (default connect case)               
            SetPostState(psHeaderPending);
            CheckAndHandleBothSocketsConnected();
        }
        break;
    case csNGConnectPost:
        {
            HX_ASSERT(m_bIsHttpNg);
            HX_ASSERT(psConnectPending == m_postState);
            SetPostState(psHeaderPending);
            SetConnectState(csNGWaitPostResponse);        
            // HTTP-NG: send POST, wait for response to get server IP for get socket to connect
            // to. At that point we'll connect to GET socket.
            SendPost();
        }
        break;
    case csConnected:
        {
            // post connects while in connect state; this must be multi-post re-connect
            HX_ASSERT(psReconnectPending == m_postState);
            if (psReconnectPending == m_postState)
            {      
                SetPostState(psHeaderPending);

                // post message should already have been created at this point (i.e., before socket reconnect)
                HX_ASSERT(m_pOutboundPostData->GetQueuedItemCount() > 0);
                TryPostSocketWrite();
            }
        }
        break;
    default:
        HX_ASSERT(false);
        break;
    }
}

HX_RESULT HXCloakedSocket::_BuildHTTPHeaderBuffer(UINT16 count)
{
    HX_RESULT res = HXR_FAIL;
    if (!m_pHTTPHeaderBuffer)
    {
        res = m_pCCF->CreateInstance( CLSID_IHXBuffer, (void**)&m_pHTTPHeaderBuffer);
        if( SUCCEEDED(res) )
        {
            UINT16 size = count + 1;
            if( size<DEFAULT_HTTP_HEADER_LENGTH )
            {
                size = DEFAULT_HTTP_HEADER_LENGTH;
            }
            res = m_pHTTPHeaderBuffer->SetSize(size);
            if( FAILED(res) )
            {
                HX_RELEASE(m_pHTTPHeaderBuffer);
            }
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }
    return res;
}

HX_RESULT HXCloakedSocket::_FindEndOfResponse(char* pBuff,
                                              char*& pText,
                                              char*& pEnd,
                                              UINT16 count)
{
    HX_RESULT hr = HXR_FAIL;

    // look for end of header (should be double CRLF, but we also
    // look for double CR or double NL)
    const char DOUBLE_CR[]   = "\r\r";
    const char DOUBLE_LF[]   = "\n\n";
    const char NLCRLF[]      = "\n\r\n";
    const char DOUBLE_CRLF[] = "\r\n\r\n";

    pEnd = 0;
    pText = pBuff;

    if ((pEnd = HXFindString(pText, DOUBLE_LF)) ||
        (pEnd = HXFindString(pText, DOUBLE_CR)) )
    {
        pEnd += 2;
        hr = HXR_OK;
    }
    else if (count >= 3 && (pEnd = HXFindString(pText, NLCRLF)))
    {
        pEnd += 3;
        hr = HXR_OK;
    }
    else if (count >= 4 && (pEnd = HXFindString(pText, DOUBLE_CRLF)))
    {
        pEnd += 4;
        hr = HXR_OK;
    }
    else
    {
        hr = HXR_INCOMPLETE;
    }
    return hr;
}
    
//
// HandleGetSockReadEvent() and HandlePostSockReadEvent() helper
//
// returns:
//  HXR_INCOMPLETE   - not enough data to process option part of get response
//
// read from inbound data (from GET or POST socket) into working http header buffer;
// header buffer is null terminated and size includes null character
//
HX_RESULT HXCloakedSocket::ReadHTTPHeader(CHXSimpleList& list)
{
    HX_RESULT hr = HXR_INCOMPLETE;
  
    UINT16 count = GetBytesAvailable(m_InboundGetData);
    
    if (count >= 2 )
    {
        hr = _BuildHTTPHeaderBuffer(count);
        if( FAILED(hr) )
        {
            return hr;
        }
        
        // copy all inbound data to header buffer
        DeQueue(m_InboundGetData, (char*)m_pHTTPHeaderBuffer->GetBuffer(), count);
        m_pHTTPHeaderBuffer->GetBuffer()[count] = '\0';

        char* pText = NULL;
        char* pEnd  = NULL;
        
        hr = _FindEndOfResponse((char*)m_pHTTPHeaderBuffer->GetBuffer(), pText, pEnd, count);
        
        if( SUCCEEDED(hr) )
        {
            UINT16 cchHeader = pEnd - pText;
            EnQueue(m_InboundGetData, pEnd , count - cchHeader);
            *pEnd = '\0';
            m_pHTTPHeaderBuffer->SetSize(pEnd - pText + 1);
        }
        else
        {
            EnQueue(m_InboundGetData, (char*)m_pHTTPHeaderBuffer->GetBuffer(), count);
        }
    }
    
    return hr;
}

HX_RESULT HXCloakedSocket::ReadHTTPHeader(CByteGrowingQueue* pInboundData)
{
    HX_RESULT hr = HXR_INCOMPLETE;
  
    UINT16 count = pInboundData->GetQueuedItemCount();
    if (count >= 2 )
    {
        hr = _BuildHTTPHeaderBuffer(count);
        if( FAILED(hr) )
        {
            return hr;
        }
        
        // copy all inbound data to header buffer
        pInboundData->DeQueue(m_pHTTPHeaderBuffer->GetBuffer(), count);
        m_pHTTPHeaderBuffer->GetBuffer()[count] = '\0';


        char* pText = NULL;
        char* pEnd  = NULL;
        
        hr = _FindEndOfResponse((char*)m_pHTTPHeaderBuffer->GetBuffer(), pText, pEnd, count);
        
        if( SUCCEEDED(hr) )
        {
            UINT16 cchHeader = pEnd - pText;
            pInboundData->EnQueue(pEnd , count - cchHeader);
            *pEnd = '\0';
            m_pHTTPHeaderBuffer->SetSize(pEnd - pText + 1);
        }
        else
        {
            pInboundData->EnQueue(m_pHTTPHeaderBuffer->GetBuffer(), count);
        }
    }
    return hr;
}

//
// HandleGetSockReadEvent() helper
//
// returns:
//  HXR_ABORT   - redirect process started
//
HX_RESULT
HXCloakedSocket::ProcessGetHeaderResponseCode()
{
    HX_ASSERT(gsHeaderResponseCode == m_getState);
    HX_ASSERT(m_pHTTPHeaderBuffer);
    char* pText = (char*)m_pHTTPHeaderBuffer->GetBuffer();

    const char* pFound = HXFindString(pText, "HTTP/1.0 200 OK");
    if (!pFound)
    {
        pFound = HXFindString(pText, "200 OK");
    }

    if (!pFound)
    {
        // check for redirect (m_pCloakValues exists only for rtsp;
        // used to exclude pnm cloaking redirects)
        if (m_pCloakValues)
        {
            pFound = HXFindString(pText, "HTTP/1.0 302");
            if (pFound)
            {
                // overwrite HTTP with RTSP so protocol sees this and does redirect
                strncpy((char*)pFound, "RTSP", 4); /* Flawfinder: ignore */

                // copy header to inbound read buffer
                HX_ASSERT(!m_pRedirectBuffer);
                HX_RELEASE(m_pRedirectBuffer);

		CreateAndSetBufferCCF(m_pRedirectBuffer, m_pHTTPHeaderBuffer->GetBuffer(),
				      m_pHTTPHeaderBuffer->GetSize(), m_pContext);
                if (!m_pRedirectBuffer)
                {
                    return HXR_OUTOFMEMORY;
                }

                SendReadEvent();
                return HXR_ABORT;
            }
        }

        // unable to handle pnm redirect or other error
        return HXR_DOC_MISSING;
    }

    return HXR_OK;
}



//
// HandleGetSockReadEvent() helper
//
// returns:
//  HXR_INCOMPLETE   - not enough data to process option part of get response
//  HXR_ABORT        - guid regenerated
//
//
HX_RESULT
HXCloakedSocket::ProcessGetOptions()
{
    HX_ASSERT (gsReadOptions == m_getState);

    // inbound data starts right after end of HTTP header
    UINT16 count = GetBytesAvailable(m_InboundGetData);
    
    /* There needs to be atleast 4 bytes to have the response
     * reponse_opcode(HTTP_RV_RESPONSE), response len, 
     * actual response(minimum one byte)
     */
    if (count < 3)
    {
        return HXR_INCOMPLETE;
    }

    HX_RESULT hr = HXR_FAIL;

    // copy inbound data to working buffer
    DeQueue(m_InboundGetData, m_pInBuf, count);
        
    if (m_pInBuf[0] == HTTP_RESPONSE)
    {
        UINT16 nOptionLen = m_pInBuf[1];
        /* Place any data after the response back in Receive queue.
         * This data needs to be sent to the user*/
        if (count > 2 + nOptionLen)
        {
            EnQueue(m_InboundGetData, m_pInBuf+2+nOptionLen, (count - (2+nOptionLen)));
        }

        hr = ProcessGetHTTPResponseOpcode((UCHAR) m_pInBuf[2]);
    }
    else if (m_pInBuf[0] == HTTP_RV_RESPONSE)
    {
        if (count < 9)
        {
            // not enough data read; copy from working buffer back to inbound data
            EnQueue(m_InboundGetData, m_pInBuf, count);
            return HXR_INCOMPLETE;
        }

        /*
         * this involves receiving a message from the server of the format:
         * HTTP/1.0 200 OK
         * ...
         * Content-length:2147483000
         * r...O?...16k padding of ZEROs...e
         *
         * r -- HTTP_RV_RESPONSE (same as the 5.0 player)
         * O -- HTTP_OPTION_RESPONSE
         * e -- HTTP_OPT_RESPONSE_END
         */
        BYTE* ptr = (BYTE *)m_pInBuf;
        
        ptr += 2;
        UINT16 nOptionLen = *ptr;
        ptr++;
        
        hr = ProcessGetHTTPResponseOpcode((UCHAR)*ptr);
        ptr++;

        if (HXR_OK == hr)
        {
            ptr++;  // get past the HTTP_OPTION_RESPONSE char 'O'
            m_uPadLength = (UINT16)getshort(ptr); 
            ptr += 2;

            // skip over the short padding option
            m_uPadLength += 1; // for the 'e' at the end of the 16k padding

            EnQueue(m_InboundGetData, (char*)ptr, 
                (m_uPadLength <= (count - (ptr - (BYTE *)m_pInBuf)) 
                ? m_uPadLength : (count - (ptr - (BYTE *)m_pInBuf))));

            count = GetBytesAvailable(m_InboundGetData);

            // get rid of the padding sent with the first response
            UINT16 len = (m_uPadLength <= count ? m_uPadLength : count); 
            DiscardData(m_InboundGetData, len);
            m_uPadLength -= len;
            if (m_uPadLength > 0)
            {
                m_bDeletePadding = TRUE;
            }
            // if we got more than the 16k initial buffer, put it back
            if (len && len < count)
            {
                EnQueue(m_InboundGetData, &m_pInBuf[len], count-len);
            }
        }
    }
    else
    {
        hr = HXR_BAD_SERVER;
    }
    return hr;
}


void
HXCloakedSocket::DoCloseSockets(HX_RESULT hr)
{
    HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::DoCloseSockets(): hr = %08x", hr);

    SetGetState(gsClosed);
    if (m_pGetSock)
    {
        m_pGetSock->Close();
        HX_RELEASE(m_pGetSock);
    }
    SetPostState(psClosed);
    if (m_pPostSock)
    {
        m_pPostSock->Close();
        HX_RELEASE(m_pPostSock);
    }

    SetConnectState(csClosed);

    if (FAILED(hr))
    {
        // error; send close notification now (otherwise wait for user to read to end of data)
        SendCloseEvent(hr);
    }

}

void
HXCloakedSocket::HandleGetSockCloseEvent(HX_RESULT status)
{
    HXLOGL3(HXLOG_RTSP, "HandleGetSockCloseEvent(): status = %lu", status);
    return;
}

void
HXCloakedSocket::HandlePostSockCloseEvent(HX_RESULT status)
{
    HXLOGL3(HXLOG_RTSP, "HandleGetSockCloseEvent(): status = %lu", status);
    return;
}

HX_RESULT
HXCloakedSocket::HandleReadEventHelper(HX_RESULT status, IHXSocket* pSock, IHXBuffer*& pBufOut)
{
    HX_RESULT hr = status;

    if (HXR_OK == hr)
    {
        hr = pSock->Read(&pBufOut);
        if (FAILED(hr))
        {
            HX_ASSERT(hr == HXR_SOCK_ENDSTREAM || hr == HXR_SOCK_WOULDBLOCK); // investigate other errors
            if (HXR_SOCK_ENDSTREAM == hr)
            {
                // end of stream data reached after graceful close
                m_bEndStreamPending = TRUE;
                DoCloseSockets(HXR_OK);
            }
            else if (HXR_SOCK_WOULDBLOCK != hr)
            {
                // fatal/severe error
                DoCloseSockets(hr);
            }
        }
    }
    else
    {
        DoCloseSockets(hr);
    }

    return hr;
}

// called when GET socket has inbound data available for reading
HX_RESULT HXCloakedSocket::HandleGetSockReadEvent(HX_RESULT status)
{
    // read a buffer
    IHXBuffer* pBuf = NULL;
    HX_RESULT  hr = HandleReadEventHelper(status, m_pGetSock, pBuf);
    char*      pBufferContents = NULL;
    ULONG32    nMsgLen = 0;
    HTTPResponseMessage* pMessage = NULL;
    HTTPParser Parser(m_pContext);
    ULONG32    ulHTTPStatus = 0;
    
    if (HXR_OK != hr)
    {
        return HXR_OK;
    }
      
    HX_ASSERT(pBuf);

    // check for and possibly begin handling http authentication
    if( m_getState != gsReadData && StartAuthenticationIfNeeded(pBuf) )
    {
        // authentication process started
        hr = HXR_OK;
        HX_RELEASE(pBuf);
        return hr;
    }

    // copy buffer to inbound data received from GET socket
    m_InboundGetData.AddTail(pBuf);
    
    /*
     * if the 16k padding was not received in one big packet
     * delete the remaining padding
     */
    if( m_bDeletePadding && m_uPadLength > 0 )
    {
        UINT16 count =  GetBytesAvailable(m_InboundGetData);
        UINT16 len   = (m_uPadLength <= count ? m_uPadLength : count);
        DiscardData(m_InboundGetData, len);
        m_uPadLength -= len;
        if (m_uPadLength == 0)
        {
            m_bDeletePadding = FALSE;
        }
    }

    HXBOOL loop;
    do
    {
        loop = FALSE;
        switch (m_getState)
        {
           case gsHeaderPending:
               // try to read header
               hr = ReadHTTPHeader(m_InboundGetData);
               if (HXR_OK == hr)
               {
                   if( m_bBlockingResistant)
                   {
                       pBufferContents = (char*)(const char*)m_pHTTPHeaderBuffer->GetBuffer();
                       nMsgLen = m_pHTTPHeaderBuffer->GetSize();
#if !defined HELIX_FEATURE_SERVER
                       pMessage = (HTTPResponseMessage*)Parser.parse( pBufferContents, nMsgLen );
#else
                       BOOL bMsgTooLarge = FALSE;
                       pMessage = (HTTPResponseMessage*)Parser.parse( pBufferContents, nMsgLen, bMsgTooLarge );
#endif /* !HELIX_FEATURE_SERVER */

                       hr  = HXR_FAIL;
                       if( pMessage && HTTPMessage::T_UNKNOWN != pMessage->tag())
                       {
                           if (strlen(pMessage->errorCode()) > 0)
                           {
                               ulHTTPStatus = atoi(pMessage->errorCode());
                               //404 is the error we get back from the
                               //server, in the fallback mode, tells us it
                               //doesn't support this mode.
                               hr = HXR_DOC_MISSING;
                               if( 404 != ulHTTPStatus )
                               {
                                   hr = HXR_OK;
                               }
                           }
                           HX_DELETE(pMessage);
                       }
                       
                       if( FAILED(hr) )
                       {
                           break;
                       }
                   }
                   
                   
                   GetServerIPFromResponse(m_getServerIP, 
                                           (const char*)m_pHTTPHeaderBuffer->GetBuffer());

                   hr = ReconnectGetSocketIfNeeded();
                   if (HXR_OK == hr)
                   {
                       // no reconnect needed; continue...
                       SetGetState(gsHeaderResponseCode);
                       hr = ProcessGetHeaderResponseCode();
                       if (HXR_ABORT == hr)
                       {
                           // this means redirect; don't bother moving on to check options
                           hr = HXR_OK;
                       }
                       else if (HXR_OK == hr)
                       {
                           SetGetState(gsReadOptions);
                           loop = TRUE;
                       }
                   }
                   else if(HXR_ABORT == hr)
                   {
                       // doing get socket reconnect
                       HX_ASSERT(gsReconnectPending == m_getState);
                       hr = HXR_OK;
                   }
               }
               else if(HXR_INCOMPLETE == hr)
               {
                   // not enough data
                   hr = HXR_OK;
               }
               break;

           case gsReconnectHeaderPending:
               hr = ReadHTTPHeader(m_InboundGetData);
               if(HXR_OK == hr)
               {
                   SetGetState(gsReadOptions);
                   loop = TRUE;
               }
               else if(HXR_INCOMPLETE == hr)
               {
                   // not enough data
                   hr = HXR_OK;
               }
               break;
  
           case gsReadOptions:
               hr = ProcessGetOptions();
               if (HXR_OK == hr)
               {
                   SetGetState(gsReadData);
                   SendReadEvent();

                   // try sending pending post data
                   TryPostSocketWrite();
               } 
               else if (HXR_INCOMPLETE == hr)
               {
                   // not enough data
                   hr = HXR_OK;
               }
               else if (HXR_ABORT == hr)
               {
                   // guid regenerated
                   HX_ASSERT(gsHeaderPending == m_getState);
                   hr = HXR_OK;
               }
               break;
           case gsReadData:
               // incoming protocol data; ensure response knows there is data available
               SendReadEvent();
               break;
           default:
               HX_ASSERT(false);
               break;
        } 
    } while (loop && (HXR_OK == hr));
    

    if (FAILED(hr))
    {
                
        DoCloseSockets(hr);
    }
    return hr;
}


//
// called when POST socket sends notification that it has inbound data pending
//
HX_RESULT                       
HXCloakedSocket::HandlePostSockReadEvent(HX_RESULT status)
{
    // read a buffer
    IHXBuffer* pBuf = NULL;
    HX_RESULT hr = HandleReadEventHelper(status, m_pPostSock, pBuf);
    if (HXR_OK != hr)
    {
        return HXR_OK;
    }
      
    HX_ASSERT(pBuf);

    // try parsing in buffer as an http response
    HTTPParser           Parser(m_pContext);
    char*                pBufferContents = NULL;
    ULONG32              nMsgLen         = 0;
    HTTPResponseMessage* pMessage        = NULL;
    ULONG32              ulHTTPStatus    = 0;
    
    pBufferContents = (char*)(const char*)pBuf->GetBuffer();
    nMsgLen         = pBuf->GetSize();
#if !defined HELIX_FEATURE_SERVER
    pMessage        = (HTTPResponseMessage*)Parser.parse( pBufferContents, nMsgLen );
#else
    BOOL bMsgTooLarge = FALSE;
    pMessage        = (HTTPResponseMessage*)Parser.parse( pBufferContents, nMsgLen, bMsgTooLarge );
#endif /* !HELIX_FEATURE_SERVER */
    ulHTTPStatus    = atoi(pMessage->errorCode());

    HX_DELETE(pMessage);

    if( HTTP_UNAUTHORIZED                 == ulHTTPStatus ||
        HTTP_PROXY_AUTHORIZATION_REQUIRED == ulHTTPStatus )
    {
        hr = HXR_FAIL;
        goto exit;
    }
    
    // transfer buffer to inbound POST data queue 
    hr = EnsureDataQueueCreated(m_pInboundPostData);
    if (HXR_OK != hr)
    {
        goto exit;
    }
    m_pInboundPostData->EnQueue(pBuf->GetBuffer(), (UINT16) pBuf->GetSize());


    HX_ASSERT(m_postState == psHeaderPending);
    if(m_postState == psHeaderPending)
    {
        hr = ReadHTTPHeader(m_pInboundPostData);
        if (HXR_OK == hr)
        {
            SetPostState(psOpen);
            GetServerIPFromResponse(m_postServerIP, (const char*)m_pHTTPHeaderBuffer->GetBuffer());

            // check if we are doing HTTP-NG connect
            if (csNGWaitPostResponse == m_connectState)
            {
                // now we can proceed to connect to get socket based on post server ip
                hr = DoConnect(m_pGetSock);
                if (FAILED(hr))
                {
                    goto exit;
                }
            }
            else
            {
                if( !m_postServerIP.IsEmpty() && m_pPreferredTransport)
                {
                    // record that this is an http-ng server
                    m_pPreferredTransport->SetHTTPNG(TRUE);
                }
    
                ReconnectGetSocketIfNeeded();
            }
        }
    }

exit:
    if (FAILED(hr))
    {
        DoCloseSockets(hr);
    }
    HX_RELEASE(pBuf);
    return HXR_OK;      
}

HX_RESULT 
HXCloakedSocket::DoConnect(IHXSocket* pSocket)
{
    // XXXLCM eventually we should use addr vector and ConnectToAny

    // connect to proxy if we have a proxy address; otherwise connect to server
    HX_ASSERT(pSocket);
    HX_ASSERT(m_pServerAddr);
    HX_ASSERT(m_pProxyAddr || m_proxyName.IsEmpty());
    IHXSockAddr* pConnectAddr = (m_pProxyAddr ? m_pProxyAddr : m_pServerAddr);
    return pSocket->ConnectToOne(pConnectAddr);
}

HX_RESULT
HXCloakedSocket::ReconnectPostSocket()
{
    // only call this in multi-post mode
    HX_ASSERT(m_bMultiPostMode);

    HX_RESULT hr = HXR_OK;
    
    // close
    if (m_pPostSock)
    {
        m_pPostSock->Close();
        HX_RELEASE(m_pPostSock);
    }
    SetPostState(psClosed);

    // re-create
    hr = CreateSocketHelper(m_family, m_pPostSockResponse, m_pPostSock);
    if (SUCCEEDED(hr))
    {
        // re-connect
        SetPostState(psReconnectPending);
        hr = DoConnect(m_pPostSock);
        if (FAILED(hr))
        {
            HX_ASSERT(false);
            SetPostState(psClosed);
        }
    }

    return hr;
}

HX_RESULT
HXCloakedSocket::PrepareNextPostSocketWrite()
{
    HX_RESULT hr = HXR_OK;

    // transfer write buffers to outbound encoded POST data queue
    while (m_PendingWriteBuffers.GetCount() > 0)
    {
        IHXBuffer* pBuffer = (IHXBuffer*) m_PendingWriteBuffers.GetHead();
        if ((UINT16) pBuffer->GetSize() < m_pOutboundPreEncodedPostData->GetMaxAvailableElements())
        {
            m_pOutboundPreEncodedPostData->EnQueue(pBuffer->GetBuffer(), 
                                                   (UINT16) pBuffer->GetSize());
            pBuffer->Release();
            m_PendingWriteBuffers.RemoveHead();
        }
        else
        {
            break;
        }
    }

    UINT16 nCount = m_pOutboundPreEncodedPostData->GetQueuedItemCount();
    if (nCount > 0)
    {
        if (m_bMultiPostMode)
        {
            // append HTTP_POSTDONE to post data
            const UINT16 amt = sizeof(UCHAR);
            const UCHAR ch = HTTP_POSTDONE;
            m_pOutboundPreEncodedPostData->EnQueue(&ch, amt);
            nCount += amt;
        }
        
        // encode post data
        m_pOutboundPreEncodedPostData->DeQueue(m_pOutBuf, nCount);
        UINT16 nEncodedCount = TCP_BUF_SIZE;

        EncodeBase64((UCHAR*) m_pOutBuf, nCount, (UCHAR*) m_pOutEncodedBuf, nEncodedCount);
        HX_ASSERT(nEncodedCount <= TCP_BUF_SIZE);

        if (m_bMultiPostMode)
        {
            // create a new post request
            hr = PreparePostMessage((UCHAR*) m_pOutEncodedBuf, nEncodedCount);
            if (SUCCEEDED(hr))
            {
                // re-connect socket for next post
                hr = ReconnectPostSocket();
                if (FAILED(hr))
                {
                    return hr;
                }
            }
        }
        else
        {
            // queue data for sending out on existing post socket connection
            m_pOutboundPostData->EnQueue(m_pOutEncodedBuf, nEncodedCount);
        }
    }

    return hr;
}


void HXCloakedSocket::_ResetResourceAndHash()
{
    CHXString tmp = "";
    
    //Generate a unique file name to use for this connection.
    _CreateRandomString(m_ResourceName);
    m_ResourceName += ".html"; 

    //Find a name to use for the name value pair that holds the
    //MD5 hash.
    int nIndex = RandBetween(0, HXCloakedV2TCPSocket::zm_nNameCount-1);
    m_NVPairName = HXCloakedV2TCPSocket::zm_NameList[nIndex];

    //Compute the hash.
    char ucHash[65];
    tmp += m_ResourceName + "RTSPviaHTTP1.1";
    MD5Data( ucHash, (const unsigned char*)(const char*)tmp, tmp.GetLength() );
    ucHash[64] = '\0';
    m_NVPairValue = ucHash;
}


HX_RESULT
HXCloakedSocket::PreparePostMessage(const UCHAR *inData, UINT16 inLength) 
{
    
    HX_ASSERT(m_pOutboundPostData->GetQueuedItemCount() == 0);

    HX_RESULT theErr = HXR_OK;
    UINT16 postLength = inLength;
    
    // create a temp buffer to help build the HTTP POST message
    char* s = new char[MAX_HTTP_METHOD_BUFSIZE];
    if (s == NULL)
    {
        return HXR_OUTOFMEMORY;
    }

    int count = FormatMethodLine(s, MAX_HTTP_METHOD_BUFSIZE, "POST");
    
    m_pOutboundPostData->EnQueue(s,count);

    // enqueue the remainder of the POST line
    count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE," HTTP/1.0\r\n"); /* Flawfinder: ignore */
    m_pOutboundPostData->EnQueue(s,count);
    
    count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"Pragma: no-cache\r\n"); /* Flawfinder: ignore */
    m_pOutboundPostData->EnQueue(s,count);

    if( !m_bBlockingResistant)
    {
        count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"Expires: Mon, 18 May 1974 00:00:00 GMT\r\n"); /* Flawfinder: ignore */
        m_pOutboundPostData->EnQueue(s,count);

        count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"User-Agent: RealPlayer G2\r\n"); /* Flawfinder: ignore */
        m_pOutboundPostData->EnQueue(s,count);

        count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"Accept: application/x-rtsp-tunnelled, */*\r\n"); /* Flawfinder: ignore */
        m_pOutboundPostData->EnQueue(s,count);

        count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"Content-type: application/x-pncmd\r\n"); /* Flawfinder: ignore */
        m_pOutboundPostData->EnQueue(s,count);
    }
    
    CHXString strAuth;
    GetAuthenticationInformation(strAuth);
    
    if (!strAuth.IsEmpty())
    {
        strAuth += "\r\n";    
        m_pOutboundPostData->EnQueue((const char*)strAuth, 
                     (UINT16)strAuth.GetLength());
    }

    UINT16 guidSize = ::strlen(m_pGuid);
        
    if(m_bMultiPostMode && !m_bBlockingResistant)
    {
        // add in the size of the GUID (which was not base64 encoded)
        postLength += guidSize + 2;     // 2 is for the \r\n at the end of the GUID!
        
        // we must report the exact size of the post data in the Content-length parameter
        count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"Content-length: %hu\r\n",postLength); /* Flawfinder: ignore */
    }
    else
    {
        count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"Content-length: 32767\r\n"); /* Flawfinder: ignore */
    }

    m_pOutboundPostData->EnQueue(s,count);

    // enqueue the CR LF to indicate end of the POST header
    count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"\r\n"); /* Flawfinder: ignore */
    m_pOutboundPostData->EnQueue(s,count);


    if( !m_bBlockingResistant )
    {
    // enqueue the GUID (Must be sent with every POST and not base64 encoded)
    m_pOutboundPostData->EnQueue(&m_pGuid[0],guidSize);
    
    // enqueue the CR LF to indicate end of the GUID
    count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"\r\n"); /* Flawfinder: ignore */
    m_pOutboundPostData->EnQueue(s,count);
    }

    if (inLength > 0)
    {
        // enqueue the actual POST data
        m_pOutboundPostData->EnQueue((char *)inData,inLength);
    }
    
    // clean up allocated buffers
    HX_VECTOR_DELETE(s);

    //HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::PreparePostMessage(): POST header to send:\n%.200s", m_pOutboundPostData);

    return theErr;
}

int HXCloakedSocket::FormatMethodLine(char* pBuf, int cchBuf, const char* pszMethod /*get or post*/)
{
    int       count = 0;
    UINT16    port  = 0;
    CHXString strServer = "";
    
    // must bet "GET" or "POST"
    HX_ASSERT(pszMethod);

    //Original 1.0 cloaking solution.
    if(m_proxyName.IsEmpty())
    {
        // not using proxy
        if( !m_bBlockingResistant )
        {
            count = SafeSprintf(pBuf,cchBuf,"%s /SmpDsBhgRl", pszMethod); /* Flawfinder: ignore */
        }
        else
        {
            //use new MD5 hash algo instead of special string to tell server
            //we are a cloaking request.
            count = SafeSprintf( pBuf, cchBuf, "%s /%s?%s=%s",
                                 pszMethod,
                                 (const char*)m_ResourceName,
                                 (const char*)m_NVPairName,
                                 (const char*)m_NVPairValue
                                 );
        }
    }
    else
    {
        // using proxy...
        HX_ASSERT(m_pCloakValues);

        UINT32 uiPort = 0;
        IHXBuffer* pBuffer = NULL;
        if (m_pCloakValues && 
            (HXR_OK == m_pCloakValues->GetPropertyULONG32("ServerPort", uiPort)) &&
            (HXR_OK == m_pCloakValues->GetPropertyCString("ServerAddress", pBuffer)))
        {
            strServer = (const char*)pBuffer->GetBuffer();
            port = (UINT16) uiPort;
        }

        HX_RELEASE(pBuffer);
        
        if( !m_bBlockingResistant )
        {
            //Original HTTP Cloaking 1.0
            if (port)
            {
                count = SafeSprintf(pBuf,cchBuf,"%s http://%s:%d/SmpDsBhgRl", pszMethod, (const char*)strServer, port);
            }
            else
            {
                count = SafeSprintf(pBuf,cchBuf,"%s http://%s/SmpDsBhgRl", pszMethod, (const char*)strServer);
            }
        }
        else
        {
            //New RTSPvHTTP 1.1 fallback code.
            if( !port )
            {
                count = SafeSprintf( pBuf, cchBuf, "%s HTTP://%s/%s?%s=%s",
                                     pszMethod,
                                     (const char*)strServer,
                                     (const char*)m_ResourceName,
                                     (const char*)m_NVPairName,
                                     (const char*)m_NVPairValue
                                     );
            }
            else
            {
                count = SafeSprintf( pBuf, cchBuf, "%s HTTP://%s:%d/%s?%s=%s",
                                     pszMethod,
                                     (const char*)strServer,
                                     port,
                                     (const char*)m_ResourceName,
                                     (const char*)m_NVPairName,
                                     (const char*)m_NVPairValue
                                     );
            }
        }
    }
    
    HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::FormatMethodLine(): %s", pBuf);

    return count;
}

HX_RESULT
HXCloakedSocket::PrepareGetMessage() 
{
    HX_RESULT   theErr = HXR_OK;
    IHXBuffer* pBuffer = NULL;

    // create a temp buffer for the HTTP GET message
    char* s = new char[MAX_HTTP_METHOD_BUFSIZE];
    
    if(s == NULL)
    {
        theErr = HXR_OUTOFMEMORY;

    }
    
    /* Flush any prior data in the send queue */
    m_pOutboundGetData->FlushQueue();

    /* Create a fresh GUID */
    CreateGuid();

    // format the HTTP GET message
    if(!theErr)
    {
        int count = FormatMethodLine(s, MAX_HTTP_METHOD_BUFSIZE , "GET");

        m_pOutboundGetData->EnQueue(s,count);

        if( !m_bBlockingResistant )
        {
            // enqueue the GUID directly after the SmpDsBhgRl tag
            m_pOutboundGetData->EnQueue(m_pGuid,::strlen(m_pGuid));
        }
        
        if (!m_proxyName.IsEmpty() )
        {
            /* 
             * enqueue dummy option to tell the server to send a padding
             * of 16k of ZEROs with the first response
             */
            if( !m_bBlockingResistant )
            {
                //old way.
                count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"?1=\"1\""); /* Flawfinder: ignore */
                m_pOutboundGetData->EnQueue(s,count);
            }
            else
            {
                //new way (1.1 fallback). Choose a random name value
                //pair and put it in there. If the server sees a 2nd
                //NV pair it knows that it means 1=1.
                int nIndex      = RandBetween(0, HXCloakedV2TCPSocket::zm_nNameCount-1);
                int nRnd        = RandBetween(0, 20947); //Just a random range.
                const char* tmp = HXCloakedV2TCPSocket::zm_NameList[nIndex];
                
                count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"&%s=%d", tmp, nRnd); 
                m_pOutboundGetData->EnQueue(s,count);
            }
            
        }

        // enqueue the HTTP 1.0 and CR LF
        count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE," HTTP/1.0\r\n"); /* Flawfinder: ignore */
        m_pOutboundGetData->EnQueue(s,count);

        count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"Pragma: no-cache\r\n"); /* Flawfinder: ignore */
        m_pOutboundGetData->EnQueue(s,count);

        if( !m_bBlockingResistant )
        {
            count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"User-Agent: RealPlayer G2\r\n"); /* Flawfinder: ignore */
            m_pOutboundGetData->EnQueue(s,count);

            count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"Expires: Mon, 18 May 1974 00:00:00 GMT\r\n"); /* Flawfinder: ignore */
            m_pOutboundGetData->EnQueue(s,count);

            count = SafeSprintf(s,MAX_HTTP_METHOD_BUFSIZE,"Accept: application/x-rtsp-tunnelled, */*\r\n"); /* Flawfinder: ignore */
            m_pOutboundGetData->EnQueue(s,count);
        }
        
        CHXString strAuth;
        GetAuthenticationInformation(strAuth);

        if (!strAuth.IsEmpty())
        {
            strAuth += "\r\n";    
            m_pOutboundGetData->EnQueue((const char*)strAuth, 
                         (UINT16)strAuth.GetLength());
        }

        // send client information so that GoldPass Admin
        // can generate redirect URL via HTTPCloaking
        if(m_pCloakValues && !m_bBlockingResistant)
        {
            if (HXR_OK == m_pCloakValues->GetPropertyCString("ClientID", pBuffer))
            {
                UINT32 ulNewSize = pBuffer->GetSize()+25;
                s = (char*)realloc(s, ulNewSize);
                if(s)
                {
                    count = SafeSprintf(s,ulNewSize,"ClientID: %s\r\n", pBuffer->GetBuffer()); /* Flawfinder: ignore */
                    m_pOutboundGetData->EnQueue(s,count);
                }
                else
                {
                    theErr = HXR_OUTOFMEMORY;
                }
                
            }
            HX_RELEASE(pBuffer);

            if (HXR_OK == m_pCloakValues->GetPropertyCString("Cookie", pBuffer))
            {
                UINT32 ulNewSize = pBuffer->GetSize()+25;
                s = (char*)realloc(s, ulNewSize);
                if(s)
                {
                    count = SafeSprintf(s,ulNewSize,"Cookie: %s\r\n", pBuffer->GetBuffer()); /* Flawfinder: ignore */
                    m_pOutboundGetData->EnQueue(s,count);
                }
                else
                {
                    theErr = HXR_OUTOFMEMORY;
                }
            }
            HX_RELEASE(pBuffer);

            if (HXR_OK == m_pCloakValues->GetPropertyCString("url", pBuffer))
            {
                UINT32 ulNewSize = pBuffer->GetSize()+25;
                s = (char*)realloc(s, ulNewSize);
                if(s)
                {
                    count = SafeSprintf(s,ulNewSize,"X-Actual-URL: %s\r\n", pBuffer->GetBuffer()); /* Flawfinder: ignore */
                    m_pOutboundGetData->EnQueue(s,count);
                }
                else
                {
                    theErr = HXR_OUTOFMEMORY;
                }
            }
            HX_RELEASE(pBuffer);
        }

        // enqueue the CR LF to indicate the end of the HTTP GET header
        s = (char*)realloc(s, 25);
        if(s)
        {
            count = SafeSprintf(s,25,"\r\n"); /* Flawfinder: ignore */
            m_pOutboundGetData->EnQueue(s,count);
        }
        else
        {
            theErr = HXR_OUTOFMEMORY;
        }
    }
    
    // clean up
    HX_DELETE(s);
    
    return theErr;
}

HX_RESULT
HXCloakedSocket::CreateGuid()
{
    CHXuuid theGuid;
    uuid_tt uuid;
    
    if (m_pGuid)
    {
        return HXR_OK;
    }

    HX_VECTOR_DELETE(m_pGuid);
    
    // generate a new GUID
    HX_RESULT theErr = theGuid.GetUuid(&uuid);

    if (HXR_OK == theErr)
    {
        CHXString theString;
        
        CHXuuid::HXUuidToString((const uuid_tt*)&uuid,&theString);
        
        int length = theString.GetLength();
        
        m_pGuid = new char[length + 1];
        if (!m_pGuid)
        {
            return HXR_OUTOFMEMORY;
        }
        
        ::strcpy(m_pGuid,(const char *)theString); /* Flawfinder: ignore */
        m_pGuid[length] = '\0';
    }
    else
    {
     // use our own GUID generator
        
        ULONG32 temp = HX_GET_TICKCOUNT();
    
        m_pGuid = new char[HXGUID_SIZE + 1];
        if (!m_pGuid)
        {
            return HXR_OUTOFMEMORY;
        }

        UINT16 length = SafeSprintf(m_pGuid,HXGUID_SIZE + 1,"%ld",temp);
        
        while(length < HXGUID_SIZE)
        {
            m_pGuid[length++] = '1';
        }
        
        m_pGuid[HXGUID_SIZE] = '\0';
    }

    return HXR_OK;
}

HX_RESULT 
HXCloakedSocket::EncodeBase64(const UCHAR* inData, UINT16 inLength, UCHAR* outData, UINT16& outLength)
{
    HX_RESULT theErr = HXR_OK;
    
    HX_ASSERT(inData != NULL);
    HX_ASSERT(outData != NULL);
    HX_ASSERT(inLength != 0);
    
    // first base64 encode the buffer
    outLength = (UINT16) BinTo64((const UCHAR*) inData, (ULONG32) inLength,(char *)outData);

    HX_ASSERT(outLength >= inLength);

    return HXR_OK;
}


HX_RESULT
HXCloakedSocket::ProcessGetHTTPResponseOpcode(UCHAR response)
{
    HX_RESULT hr = HXR_OK;
    
    enum
    {
        HTTP_OK = 0,
        HTTP_GENERAL_ERROR,     // for any error that is not defined below
        POST_NOT_RECEIVED,      // POST message was not received
        INVALID_GUID            // sent only if the GUID from the Player is already in use
    };

    switch(response)
    {
        case HTTP_OK:
        {
            HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::ProcessGetHTTPResponseOpcode(): HTTP_OK (single-post mode)");
            m_bMultiPostMode = FALSE;   
        }
        break;
                
        case POST_NOT_RECEIVED: // POST message was not received
        {
            HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::ProcessGetHTTPResponseOpcode(): POST_NOT_RECEIVED (multi-post mode)");
            if (m_getServerIP)
            {
                // use serverIP from GET response for multi-POST
                UpdateServerAddr(m_getServerIP);
            }
            m_bMultiPostMode = TRUE;

        }
        break;
                
        case INVALID_GUID:                      
        {
            HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::ProcessGetHTTPResponseOpcode(): INVALID_GUID");
            /* sent only if the GUID from the Player is already in use
             * Need to regenerate GUID and send everything again
             */     
            HX_VECTOR_DELETE(m_pGuid);
            hr = SendGet(); //XXXLCM from original logic; can we send another GET without re-connect?
            if (SUCCEEDED(hr))
            {
                // use HXR_ABORT to signal caller to continue in current state
                hr = HXR_ABORT;
            }
            else
            {
                DoCloseSockets(hr);
            }
            
        }
        break;

        default:
        {
            // shut this clip down and report an appropriate error
            hr = HXR_HTTP_CONTENT_NOT_FOUND;
        }
        break;
    }
    
    return hr;
}


void
HXCloakedSocket::FlushQueues()
{
    if (m_pOutboundGetData)
    {
        m_pOutboundGetData->FlushQueue();
    }

    //Empty the inbound GET queue.
    while( m_InboundGetData.GetCount() )
    {
        IHXBuffer* pBuf = (IHXBuffer*)m_InboundGetData.RemoveHead();
        HX_RELEASE(pBuf);
    }

    if (m_pInboundPostData)
    {
        m_pInboundPostData->FlushQueue();
    }

    if (m_pOutboundPreEncodedPostData)
    {
        m_pOutboundPreEncodedPostData->FlushQueue();
    }

    if (m_pOutboundPostData)
    {
        m_pOutboundPostData->FlushQueue();
    }
}

void                    
HXCloakedSocket::SendHTTPDone()
{
    IHXBuffer* pBuffer = NULL;
    BYTE http_done = HTTP_DONE;
    if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (UCHAR*) &http_done, 1, m_pContext))
    {
        // XXXLCM will this work in multi post mode (re-connect may not be immediate)?
        m_PendingWriteBuffers.AddTail((void*) pBuffer);
        TryPostSocketWrite();
    }
}



void
HXCloakedSocket::GetServerIPFromResponse(CHXString& serverIP, 
                                         const char* pszInBuffer)
{
    HX_ASSERT(pszInBuffer);

    serverIP = "";

    const char SERVER_IP_ADDR[] = "x-server-ip-address:";
    const size_t SERVER_IP_ADDR_COUNT = (sizeof SERVER_IP_ADDR / sizeof SERVER_IP_ADDR[0]);

    // look for 'x-server-ip-address'
    const char* pszServerIPStart = HXFindString(pszInBuffer, SERVER_IP_ADDR);
    if (pszServerIPStart)
    {
        pszServerIPStart += SERVER_IP_ADDR_COUNT;

        // removing leading spaces
        while (*pszServerIPStart == ' ')
        {
            pszServerIPStart++;
        }

        const char* pszServerIPEnd = HXFindString(pszServerIPStart, "\r\n");    
        if (pszServerIPEnd)
        {
            UINT32 nLength = pszServerIPEnd - pszServerIPStart;
            serverIP = CHXString(pszServerIPStart, nLength);
         
        }
    }
}


//
// called after we get authentication response after HandleAuthentication
//
HX_RESULT
HXCloakedSocket::StartOverAfterAuth()
{
    HX_RESULT rc = HXR_FAIL;
   
    SetConnectState(csClosed);
    SetPostState(psClosed);
    SetGetState(gsClosed);

    m_eventFlags = m_selectedEvents;

    // we already sent a connect to the user; this reconnect is transparent to response
    ClearEventFlag(HX_SOCK_EVENT_CONNECT);

    //Empty the inbound GET queue.
    while( m_InboundGetData.GetCount() )
    {
        IHXBuffer* pBuf = (IHXBuffer*)m_InboundGetData.RemoveHead();
        HX_RELEASE(pBuf);
    }
    

    SendHTTPDone();
    
    // close existing sockets
    DoCloseSockets(HXR_OK);
    
    // create send and recv (put and get) sockets
    rc = CreateSocketHelper(m_family, m_pGetSockResponse, m_pGetSock);
    if (SUCCEEDED(rc))
    {
        rc = CreateSocketHelper(m_family, m_pPostSockResponse, m_pPostSock);
        if (SUCCEEDED(rc))
        {
            SetConnectState(csInitialized);
            rc = ConnectToOne(m_pServerAddr);
        }
    }

    if (FAILED(rc))
    {
        DoCloseSockets(rc);
    }
    return rc;
}

//
// HandleGetSockReadEvent() and HandlePostSockReadEvent() helper
//
// returns:
//  HXR_ABORT - get socket reconnect has started
//  HXR_OK    - no need to reconnect get socket (proceed with current)
//
HX_RESULT
HXCloakedSocket::ReconnectGetSocketIfNeeded()
{
    HX_RESULT hr = HXR_OK;

    // we only do this if not HTTP-NG and both sockets are connected
    if (!m_bIsHttpNg && (csConnected == m_connectState))
    {
        // compare GET and POST socket IP addresses
        if (!m_getServerIP.IsEmpty() && !m_postServerIP.IsEmpty() &&
            (0 != m_getServerIP.CompareNoCase(m_postServerIP)))
        {
            // need to reconnect get socket...

            // use serverIP from POST response for GET reconnect
            UpdateServerAddr(m_postServerIP);

            //Empty the inbound GET queue.
            while( m_InboundGetData.GetCount() )
            {
                IHXBuffer* pBuf = (IHXBuffer*)m_InboundGetData.RemoveHead();
                HX_RELEASE(pBuf);
            }

            SendHTTPDone();
            
            HX_ASSERT(m_pGetSockResponse); // re-use

            // re-create socket and connect again
            HX_RELEASE(m_pGetSock);
            hr = CreateSocketHelper(m_family, m_pGetSockResponse, m_pGetSock);
            if (FAILED(hr))
            {
                DoCloseSockets(hr);
                return hr;
            }

            SetConnectState(csConnectBoth);
            SetGetState(gsReconnectPending);

            hr = DoConnect(m_pGetSock);
            if (FAILED(hr))
            {
                DoCloseSockets(hr);
            }
            else
            {
                hr = HXR_ABORT;
            }
        }
    }
    return hr;
}



// * parse buffer to see if it holds http response message
// * if no, return FALSE
// * if yes and HTTP_UNAUTHORIZED or HTTP_PROXY_AUTHORIZATION_REQUIRED, handle authentication
HXBOOL
HXCloakedSocket::StartAuthenticationIfNeeded(IHXBuffer* pBuf)
{
    HX_RESULT retVal = HXR_OK;
    
    HX_ASSERT(pBuf);
    
    /* start of authenticated proxy logic */

    // try parsing in buffer as an http response
    HTTPParser Parser(m_pContext);
    char* pBufferContents = (char*)(const char*)pBuf->GetBuffer();
    ULONG32 nMsgLen = pBuf->GetSize();
#if !defined HELIX_FEATURE_SERVER
    HTTPResponseMessage* pMessage = (HTTPResponseMessage*)Parser.parse( pBufferContents, nMsgLen );
#else
    BOOL bMsgTooLarge = FALSE;
    HTTPResponseMessage* pMessage = (HTTPResponseMessage*)Parser.parse( pBufferContents, nMsgLen, bMsgTooLarge );
#endif /* !HELIX_FEATURE_SERVER */

    //
    // ignore non-HTTP responses; thoseh will be processed by the
    // response object, i.e., RTSPClientProtocol
    //
    if( !pMessage || HTTPMessage::T_UNKNOWN == pMessage->tag())
    {
        HX_DELETE(pMessage);
        return FALSE;
    }

    // get HTTP error code
    ULONG32 ulHTTPStatus = 0;
    if (strlen(pMessage->errorCode()) > 0)
    {
        ulHTTPStatus = atoi(pMessage->errorCode());
    }

    if (ulHTTPStatus == HTTP_UNAUTHORIZED || 
        ulHTTPStatus == HTTP_PROXY_AUTHORIZATION_REQUIRED)
    {
        // we require authentication
        IHXRequest* pRequest = NULL;

        //At this point we know we need to authenticate or not get
        //through.  Also, if we keep the socket open we can sometimes
        //get another socket read event in the same stack. This
        //confuses the old cloaking to no end. So, we close the socket
        //here to keep that from happening.  The socket is closed as
        //soon as we get authentication back anyway.
        DoCloseSockets(HXR_OK);
        
        retVal = m_pCCF->CreateInstance(CLSID_IHXRequest, (void**)&pRequest);

        if(retVal == HXR_OK)
        {
            // form GET URL and assign to request object
            PrepareGetMessage();
            UINT16 count = m_pOutboundGetData->GetQueuedItemCount();
            HX_ASSERT(count >0);
            m_pOutboundGetData->DeQueue(m_pOutBuf,count);
            retVal = pRequest->SetURL(m_pOutBuf);
            HX_ASSERT(HXR_OK == retVal); //XXXLCM handle

            //
            // create headers for response to authentication request
            // from server by copying http message header into response
            // headers object
            //
            IHXKeyValueList* pResponseHeaders = NULL;
            retVal = m_pCCF->CreateInstance(CLSID_IHXKeyValueList, (void**)&pResponseHeaders);
            HX_ASSERT(HXR_OK == retVal); //XXXLCM handle

            MIMEHeaderValue* pHeaderValue = NULL;
            MIMEHeader* pHeader = pMessage->getFirstHeader();
            while (pHeader)
            {
                pHeaderValue = pHeader->getFirstHeaderValue();
                CHXString strHeader;
                while (pHeaderValue)
                {
                    CHXString strTemp;
                    pHeaderValue->asString(strTemp);
                    strHeader += strTemp;
                    pHeaderValue = pHeader->getNextHeaderValue();
                    if (pHeaderValue)
                    {
                        strHeader += ", ";
                    }
                }
                IHXBuffer* pBuffer = NULL;
                CHXBuffer::FromCharArray((const char*)strHeader, &pBuffer);
                pResponseHeaders->AddKeyValue(pHeader->name(), pBuffer);
                HX_RELEASE(pBuffer);

                pHeader = pMessage->getNextHeader();
            }

            IHXValues* pResponseValues = NULL;             

            if (HXR_OK == pResponseHeaders->QueryInterface(IID_IHXValues, (void**)&pResponseValues))
            {
                pRequest->SetResponseHeaders(pResponseValues);
            }

            // send authentication
            HandleAuthentication(pRequest, pMessage, 
                                 m_serverName, m_proxyName);

            HX_RELEASE(pResponseValues);
            HX_RELEASE(pResponseHeaders);

        }
        HX_DELETE(pMessage);
        return TRUE;
    }
    HX_DELETE(pMessage);
    return FALSE;
}

#define CLOAKED_WWW_AUTHENTICATION_RECENT_KEY "authentication.http.realm.recent"
#define CLOAKED_PROXY_AUTHENTICATION_RECENT_KEY "proxy-authentication.http.realm.recent"

void
HXCloakedSocket::GetAuthenticationInformation(CHXString& strAuth)
{
    IHXBuffer* pBuffer = NULL;

    CHXString key("no-authentication-information");

    CHXString recentAuthRealmInfo;
    CHXString recentProxyAuthRealmInfo;

    IHXBuffer* pHeaderBuffer = NULL;
    
    HX_RESULT theErr = HXR_OK;

    IHXRegistry* pRegistry = NULL;
    
#if defined(HELIX_FEATURE_REGISTRY)
    m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);
    HX_ASSERT(pRegistry);
#endif
    
    if (!pRegistry) return;
    
    theErr = pRegistry->GetStrByName(CLOAKED_WWW_AUTHENTICATION_RECENT_KEY, pHeaderBuffer);
    if (SUCCEEDED(theErr))
    {
        HX_ASSERT(pHeaderBuffer);
        recentAuthRealmInfo = CHXString((const char*)pHeaderBuffer->GetBuffer(), pHeaderBuffer->GetSize());
        HX_RELEASE(pHeaderBuffer);
    }

    theErr = pRegistry->GetStrByName(CLOAKED_PROXY_AUTHENTICATION_RECENT_KEY, pHeaderBuffer);
    if (SUCCEEDED(theErr))
    {
        HX_ASSERT(pHeaderBuffer);
        recentProxyAuthRealmInfo = CHXString((const char*)pHeaderBuffer->GetBuffer(), pHeaderBuffer->GetSize());
        HX_RELEASE(pHeaderBuffer);
    }

    key = "proxy-authentication.http:";
    key += m_proxyName;
    key += ":";
    key += recentProxyAuthRealmInfo;

    if (HXR_OK == pRegistry->GetStrByName((const char*)key, pBuffer) )
    {
        if (pBuffer)
        {
            CHXString authString((const char*)pBuffer->GetBuffer(), pBuffer->GetSize());

            strAuth = "Proxy-Authorization: ";
            strAuth += (const char*)authString;
        }
    }
    HX_RELEASE(pBuffer);

    HX_RELEASE(pRegistry);
}

HX_RESULT
HXCloakedSocket::HandleAuthentication(IHXRequest* pRequest, HTTPResponseMessage* pMessage,
                const char* pHost, const char* pProxyHost)
{
    HX_RESULT   status = HXR_OK;
    UINT32      ulAltURL = 0;
    CHXString   sConnection;
    IHXValues* pNewHeaders = NULL;

    if (!pRequest)
    {
        return HXR_UNEXPECTED;
    }

    HX_RESULT retVal = HXR_OK;
    IHXRegistry* pRegistry = NULL;
    retVal = m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);
    if (SUCCEEDED(retVal))
    {
        IHXValues* pResponseHeaders = NULL;
            
        HX_ASSERT(pRequest);
        if (HXR_OK == pRequest->GetResponseHeaders(pResponseHeaders))
        {
            IHXBuffer* pServerHeaderBuffer = NULL;

            HX_ASSERT(pHost);
            if (pHost)
            {
                retVal = m_pCCF->CreateInstance(CLSID_IHXBuffer,
                                                (void**)&pServerHeaderBuffer);
                if (SUCCEEDED(retVal))
                {
                    UINT32 ulHTTPStatus = atoi(pMessage->errorCode());
                    if (ulHTTPStatus == HTTP_PROXY_AUTHORIZATION_REQUIRED && pProxyHost)
                    {
                        pServerHeaderBuffer->Set((UCHAR*)pProxyHost, strlen(pProxyHost)+1);
                    }
                    else
                    {
                        pServerHeaderBuffer->Set((UCHAR*)pHost, strlen(pHost)+1);
                    }
                    pResponseHeaders->SetPropertyCString("_server", pServerHeaderBuffer);
                    HX_RELEASE(pServerHeaderBuffer);
                }
            }

            // Add the protocol to the response headers because TLC needs it
            IHXBuffer* pProtocol = NULL;
            if (SUCCEEDED(m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pProtocol)))
            {
                pProtocol->Set((UCHAR*)"http", strlen("http") + 1);
                pResponseHeaders->SetPropertyCString("_protocol", pProtocol);
                HX_RELEASE(pProtocol);
            }
        }

        if (!m_pClientAuthConversationAuthenticator)
        {
            // Starting conversation
                
                
            IHXCommonClassFactory* pFactory = 0;
            m_pCloakContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&pFactory);
                
            IUnknown* pUnkConfig = 0;
            status = pFactory->CreateInstance( CLSID_CHXClientAuthenticator,
                                               (void**)&pUnkConfig);
            if (SUCCEEDED(status))
            {
                status = pUnkConfig->QueryInterface(IID_IHXClientAuthConversation,
                                                    (void**)&m_pClientAuthConversationAuthenticator);
                if (SUCCEEDED(status))
                {
                    IHXObjectConfiguration* pConfig = 0;
                    status = pUnkConfig->QueryInterface(IID_IHXObjectConfiguration,
                                                        (void**)&pConfig);
                    if (SUCCEEDED(status))
                    {
                        pConfig->SetContext(m_pCloakContext);
                        HX_RELEASE(pConfig);
                    }
                }
                HX_RELEASE(pUnkConfig);
            }

            HX_RELEASE(pFactory);

        }

        if ( m_pClientAuthConversationAuthenticator
             && !m_pClientAuthConversationAuthenticator->IsDone() )
        {
            HX_ASSERT(pRequest);
            if (pRequest)
            {
                status=m_pClientAuthConversationAuthenticator->MakeResponse(this, pRequest);
                // Flow continues in ResponseReady()
            }
            else
            {
                // Auth Failed!
                m_pClientAuthConversationAuthenticator->Authenticated(FALSE);
                ResponseReady(HXR_NOT_AUTHORIZED, pRequest);
            }
        }

        HX_RELEASE(pRegistry);
    }
    

    return status;
}

// IHXClientAuthResponse
STDMETHODIMP 
HXCloakedSocket::ResponseReady(HX_RESULT status, IHXRequest* pRequestResponse)
{
    HX_RESULT hr = HXR_OK;
    
    if (FAILED(status))
    {
        return HXR_OK;
    }

    HX_ASSERT(m_pContext);
    HX_ASSERT(pRequestResponse);

    IHXRegistry* pRegistry = NULL;
    hr = m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);
    if (FAILED(hr))
    {
        return hr;
    }
    
    //
    // iterate each line in the response header and look for "Proxy-Authorization"
    //
    IHXValues* pHeaders = NULL;
    if (HXR_OK == pRequestResponse->GetRequestHeaders(pHeaders))
    {
        const char* pName;
        IHXBuffer* pBuf;
        
        // get first header name value line
        HX_RESULT hrIter = pHeaders->GetFirstPropertyCString(pName, pBuf);
        while (hrIter == HXR_OK)
        {
            if (!strcasecmp(pName, "Proxy-Authorization"))
            {
                // form registry key for storing this value
                CHXString key = "proxy-authentication.http:";
                key += m_proxyName;
                key += ":";

                // add recent realm
                IHXBuffer* pHeaderBuffer = NULL;
                if (HXR_OK == pRegistry->GetStrByName(CLOAKED_PROXY_AUTHENTICATION_RECENT_KEY,
                        pHeaderBuffer))
                {
                    HX_ASSERT(pHeaderBuffer);
                    key += CHXString((const char*)pHeaderBuffer->GetBuffer(), pHeaderBuffer->GetSize());
                    HX_RELEASE(pHeaderBuffer);
    
                }

                // create a buffer to hold value associated with reg key
                IHXBuffer* pBuffer = NULL;
                hr = m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
                if (FAILED(hr))
                {
                    HX_RELEASE(pBuf);
                    break;
                }
                pBuffer->Set(pBuf->GetBuffer(), pBuf->GetSize());

                // set reg key/value; these will be picked up when we form POST/GET messages (via GetAuthenticationInformation)
                UINT32 regid = pRegistry->GetId((const char*)key);
                if (!regid)
                {
                    pRegistry->AddStr((const char*)key, pBuffer);
                }
                else
                {
                    pRegistry->SetStrByName((const char*)key, pBuffer);
                }

                HX_RELEASE(pBuffer);
            }

            // get next header name value line
            HX_RELEASE(pBuf);
            hrIter = pHeaders->GetNextPropertyCString(pName, pBuf);
        }
        HX_RELEASE(pHeaders);
    }

    HX_RELEASE(pRegistry);

    if (SUCCEEDED(hr))
    {
        StartOverAfterAuth();
    }

    return hr;
}



HXCloakedSocket::SockResponse::SockResponse(HXCloakedSocket* pOwner, HXBOOL bIsGetResponse) :
     m_pOwner(pOwner)
    ,m_lRefCount(0) 
    ,m_bIsGetResponse(bIsGetResponse)
{
}

HXCloakedSocket::SockResponse::~SockResponse()
{
}

STDMETHODIMP HXCloakedSocket::SockResponse::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXSocketResponse), (IHXSocketResponse*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXSocketResponse*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}


STDMETHODIMP_(ULONG32) HXCloakedSocket::SockResponse::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}


STDMETHODIMP_(ULONG32) HXCloakedSocket::SockResponse::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}



/* IHXSocketResponse*/
STDMETHODIMP
HXCloakedSocket::SockResponse::EventPending(UINT32 uEvent, HX_RESULT status)
{
    HXLOGL3(HXLOG_RTSP, "HXCloakedSocket::EventPending(): get sock = %lu; event = %lu; status = %08x", m_bIsGetResponse, uEvent, status);
    switch (uEvent)
    {
    case HX_SOCK_EVENT_CONNECT:
        if (m_bIsGetResponse)
        {
            m_pOwner->HandleGetSockConnectEvent(status);
        }
        else
        {
            m_pOwner->HandlePostSockConnectEvent(status);
        }
        break;
    case HX_SOCK_EVENT_READ:
        if (m_bIsGetResponse)
        {
            m_pOwner->HandleGetSockReadEvent(status);
        }
        else 
        {
            m_pOwner->HandlePostSockReadEvent(status);
        }
        break;
    case HX_SOCK_EVENT_WRITE:
        if (m_bIsGetResponse)
        {
            m_pOwner->HandleGetSockWriteEvent(status);
        }
        else
        {
            m_pOwner->HandlePostSockWriteEvent(status);
        }
        break;
    case HX_SOCK_EVENT_CLOSE:
        if (m_bIsGetResponse)
        {
            m_pOwner->HandleGetSockCloseEvent(status);
        }
        else
        {
            m_pOwner->HandlePostSockCloseEvent(status);
        }
        break;
    default:
        HX_ASSERT(FALSE);
        break;
    }
    return HXR_OK;
}








