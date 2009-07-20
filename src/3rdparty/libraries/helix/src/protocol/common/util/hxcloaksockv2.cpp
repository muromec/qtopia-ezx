/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcloaksockv2.cpp,v 1.10 2006/05/12 01:48:24 atin Exp $
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
#include "hlxclib/stdio.h"   
#include "hxcom.h"
#include "hxnet.h"
#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxccf.h" 
#include "hxmon.h" 
#include "hxthread.h"
#include "hxbuffer.h"
#include "dbcs.h" 
#include "conn.h"    
#include "md5.h"
#include "hxtick.h"  
#include "rtsputil.h"
#include "hxurlrep.h"
#include "hxcloaksockv2.h"
#include "hlxclib/sys/socket.h"
#include "httppars.h"
#include "hxauthn.h"
#include "hxplgns.h"
#include "hxfiles.h"


//Return a random number in [beg, int].
static inline int RandBetween(int beg, int end )
{
    return beg + ((end-beg+1)*rand())/RAND_MAX;
}


const char* const HXCloakedV2TCPSocket::zm_HashName = "RTSPviaHTTP1.1";
const int   HXCloakedV2TCPSocket::zm_nNameCount = 10;
const char* const HXCloakedV2TCPSocket::zm_NameList[] =
        { "name",
          "href",
          "id",
          "rel",
          "type",
          "id_article",
          "sid",
          "hl",
          "FamilyID",
          "Action"
        };

//size of the queue to hold data off of the wire.
const int HXCloakedV2TCPSocket::zm_QueueSize = 5120;

HXCloakedV2TCPSocket::HXCloakedV2TCPSocket(IUnknown* pContext)
    : m_pContext(pContext),
      m_pCloakContext(NULL),
      m_pScheduler(NULL),
      m_pNetworkServices(NULL),
      m_lRefCount(0),
      m_pTCPResponse(NULL),
      m_bResponseIsInterruptSafe(FALSE),
      m_pTCPSocket(NULL),
      m_nProxyPort(0),
      m_pszProxyName(NULL),
      m_pDestAddr(NULL),
      m_pProxyAddr(NULL),
      m_pResolver(NULL),
      m_CloakingState(csDisconnected),
      m_ResourceName(""),
      m_NVPairValue(""),
      m_NVPairName(""),
      m_ReadQueue(zm_QueueSize),
      m_RTSPQueue(zm_QueueSize)
{
    if(m_pContext)
    {
        m_pContext->AddRef();
        m_pContext->QueryInterface(IID_IHXScheduler,   (void**) &m_pScheduler);
        m_pContext->QueryInterface(IID_IHXNetServices, (void**) &m_pNetworkServices);
    }
    HX_ASSERT(m_pScheduler);
    HX_ASSERT(m_pNetworkServices);

    srand((unsigned)time(NULL));
}

HXCloakedV2TCPSocket::~HXCloakedV2TCPSocket()
{
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pNetworkServices);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pTCPResponse);
    HX_RELEASE(m_pTCPSocket);
    HX_RELEASE(m_pDestAddr);
    HX_RELEASE(m_pProxyAddr);
    HX_RELEASE(m_pResolver);
    HX_RELEASE(m_pCloakContext);
               
    HX_VECTOR_DELETE(m_pszProxyName);
}

STDMETHODIMP HXCloakedV2TCPSocket::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXSocket),           (IHXSocket*)this },
            { GET_IIDHANDLE(IID_IHXCloakedTCPSocket), (IHXCloakedTCPSocket*)this },
            { GET_IIDHANDLE(IID_IHXHTTPProxy),        (IHXHTTPProxy*)this },
            { GET_IIDHANDLE(IID_IHXInterruptState),   (IHXInterruptSafe*)this },
            { GET_IIDHANDLE(IID_IHXResolveResponse),  (IHXResolverResponse*)this },
            { GET_IIDHANDLE(IID_IHXSocketResponse),   (IHXSocketResponse*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXSocket*)this }
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXCloakedV2TCPSocket::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXCloakedV2TCPSocket::Release()
{
    if(InterlockedDecrement(&m_lRefCount) <= 0)
    {
        delete this;
    }
    return m_lRefCount;
}

HXBOOL HXCloakedV2TCPSocket::_GetInterruptSafe(IHXSocketResponse* pResponseObject)
{
    HXBOOL bRetVal = FALSE;
    //Find out if the response object is interrupt safe or not.
    IHXInterruptSafe* pTCPResponseState = NULL;
    pResponseObject->QueryInterface(IID_IHXInterruptSafe,
                                   (void**) &pTCPResponseState);
    if( pTCPResponseState )
    {
        bRetVal = pTCPResponseState->IsInterruptSafe();
        HX_RELEASE(pTCPResponseState);
    }
    return bRetVal;
}


STDMETHODIMP HXCloakedV2TCPSocket::Init( HXSockFamily   family,
                                         HXSockType     type,
                                         HXSockProtocol protocol)
{
    HX_RESULT res = HXR_FAIL;

    m_family = family;
    m_type = type;
    m_protocol = protocol;
    
    //Create the TCP socket to transport our HTTP requests.
    HX_ASSERT(m_pNetworkServices);
    if(m_pNetworkServices)
    {
        if( m_pTCPSocket )
        {
            m_pTCPSocket->Close();
            HX_RELEASE( m_pTCPSocket );
        }
        res = m_pNetworkServices->CreateSocket(&m_pTCPSocket);
        if(m_pTCPSocket && SUCCEEDED(res) )
        {
            res = m_pTCPSocket->SetResponse(this);
            if( SUCCEEDED(res) )
            {
                res = m_pTCPSocket->Init(family, type, protocol);
                m_pTCPSocket->SetOption( HX_SOCKOPT_APP_BUFFER_TYPE,
                                         HX_SOCKBUF_DEFAULT);
                m_pTCPSocket->SelectEvents(HX_SOCK_EVENT_READ|
                                           HX_SOCK_EVENT_CONNECT|
                                           HX_SOCK_EVENT_CLOSE);
        

            }
        }
        if( m_pNetworkServices && SUCCEEDED(res) && !m_pResolver )
        {
            res = m_pNetworkServices->CreateResolver(&m_pResolver);
            if( SUCCEEDED(res) && m_pResolver )
            {
                res = m_pResolver->Init(this);
            }
        }
    }
    
    if( !SUCCEEDED(res) )
    {
        if( m_pTCPSocket )
        {
            m_pTCPSocket->Close();
            HX_RELEASE(m_pTCPSocket);
        }
        
        HX_RELEASE(m_pResolver);
    }

    return res;
}

STDMETHODIMP HXCloakedV2TCPSocket::SetResponse(IHXSocketResponse* pTCPResponse)
{
    HX_RESULT res = HXR_FAIL;
    
    HX_ASSERT(pTCPResponse);
    if( pTCPResponse)
    {
        HX_RELEASE(m_pTCPResponse);
        m_pTCPResponse = pTCPResponse;
        m_pTCPResponse->AddRef();
        m_bResponseIsInterruptSafe = _GetInterruptSafe(m_pTCPResponse);
        res = HXR_OK;
    }
    
    return res;
}

STDMETHODIMP HXCloakedV2TCPSocket::Bind(IHXSockAddr* pAddr)
{
    return HXR_NOTIMPL;
}


//Network addresses and ports are in native byte order
STDMETHODIMP HXCloakedV2TCPSocket::SetProxy( const char* pszProxyName,
                                             UINT16 nProxyPort)
{
    HX_RESULT res = HXR_UNEXPECTED;

    //SetProxy must be called before 'ConnectToOne'.
    HX_ASSERT(csDisconnected==m_CloakingState);
    HX_ASSERT(NULL!=pszProxyName);
    HX_ASSERT(0!=nProxyPort);

    if( NULL!=pszProxyName && 0!=nProxyPort && csDisconnected==m_CloakingState)
    {
        res = HXR_FAIL;
        HX_VECTOR_DELETE(m_pszProxyName);

        m_pszProxyName = new char[::strlen(pszProxyName)+1];
        if(!m_pszProxyName)
        {
            res = HXR_OUTOFMEMORY;
        }
        else
        {
            ::strcpy(m_pszProxyName, pszProxyName); /* Flawfinder: ignore */
            m_nProxyPort = nProxyPort;
            HX_RELEASE(m_pProxyAddr);
            res = HXR_OK;
        }
    }
    
    return res;
}

STDMETHODIMP HXCloakedV2TCPSocket::InitCloak(IHXValues* pValues,
                                             IUnknown* pUnknown)
{
    HX_RELEASE(m_pCloakContext);
    m_pCloakContext = pUnknown;
    m_pCloakContext->AddRef();
    return HXR_OK;
}


STDMETHODIMP HXCloakedV2TCPSocket::GetLocalAddr(IHXSockAddr** ppAddr)
{
    HX_RESULT res = HXR_FAIL;
    
    HX_ASSERT(m_pTCPSocket);
    HX_ASSERT(csDisconnected != m_CloakingState );
    
    if( m_pTCPSocket && csDisconnected!=m_CloakingState )
    {
        res = m_pTCPSocket->GetLocalAddr(ppAddr);
    }
    return res;
}

STDMETHODIMP HXCloakedV2TCPSocket::GetPeerAddr(IHXSockAddr** ppAddr)
{
    //XXXgfw decide whether or not to return the proxy or server addr
    //here. We really need just the server one I think.
    HX_RESULT res = HXR_FAIL;
    
    HX_ASSERT(m_pTCPSocket);
    HX_ASSERT(csDisconnected!=m_CloakingState);
    
    if( m_pTCPSocket && csDisconnected!=m_CloakingState )
    {
        res = m_pTCPSocket->GetPeerAddr(ppAddr);
    }
    return res;
}

STDMETHODIMP HXCloakedV2TCPSocket::Read(IHXBuffer** ppBuf)
{
    HX_RESULT  res     = HXR_WOULD_BLOCK;
    ULONG32    ulBytes = 0;
    IHXBuffer* pNew    = NULL;
    
//    HX_ASSERT( csReady == m_CloakingState );

    *ppBuf = NULL;
    ulBytes = m_RTSPQueue.GetQueuedItemCount();
    
    if( ulBytes > 0 )
    {
        res = HXR_OUTOFMEMORY;
	CreateBufferCCF(pNew, m_pContext);
        if( pNew )
        {
            res = pNew->SetSize(ulBytes);
            HX_ASSERT(SUCCEEDED(res));
            if( SUCCEEDED(res) )
            {
                m_RTSPQueue.DeQueue(pNew->GetBuffer(), ulBytes );
                *ppBuf = pNew;
            }
        }

        if( !SUCCEEDED(res) )
        {
            HX_RELEASE(pNew);
        }
    }
    
    return res;
}

// HX_RESULT HXCloakedV2TCPSocket::_ChunkBuffer(IHXBuffer* pBuffer, IHXBuffer*& pNew)
// {
//     char      szBuf[12];
//     ULONG32   ulSize = 0;
//     HX_RESULT res    = HXR_FAIL;
    
//     HX_ASSERT(pBuffer);
//     HX_ASSERT(pBuffer->GetSize()!=0 );

//     if( pBuffer && pBuffer->GetSize()!=0 )
//     {
//         pBuffer->AddRef();
//         ULONG32 ulSize = pBuffer->GetSize();
    
//         sprintf( szBuf, "%X\r\n", ulSize ); //Hex size of chunked.

//         res = HXR_OUTOFMEMORY;
//	   CreateBufferCCF(pNew, m_pContext);
//         HX_ASSERT(pNew);
//         if(pNew)
//         {
//             //Size is FFFFFFFF\r\n + trailing \r\n. Chunked headers.
//             res = pNew->SetSize(ulSize+strlen(szBuf)+2);
//             if( SUCCEEDED(res) )
//             {
//                 memcpy( pNew->GetBuffer(), szBuf, strlen(szBuf) );
//                 memcpy( pNew->GetBuffer()+strlen(szBuf), pBuffer->GetBuffer(), ulSize);
//                 memcpy( pNew->GetBuffer()+strlen(szBuf)+ulSize, "\r\n", 2);
//                 res = HXR_OK;
//             }
//         }
//         HX_RELEASE(pBuffer);
//     }

//     if( !SUCCEEDED(res) )
//     {
//         HX_RELEASE(pNew);
//     }
    
    
//     return res;
// }


HX_RESULT HXCloakedV2TCPSocket::_UnChunkBuffer(IHXBuffer* pBuf,
                                               IHXBuffer*& pNew,
                                               ULONG32& ulBytesUsed)
{
    HX_RESULT      res         = HXR_FAIL;
    UINT           unChunkSize = 0;
    const char*    pucData     = NULL;
    ULONG32        ulBufSize   = 0;
    
    HX_ASSERT(pBuf);
    //Min size is 3 "0\r\n": zero chunk.
    HX_ASSERT(pBuf->GetSize() >= 3 );

    if( pBuf && pBuf->GetSize()>=3 )
    {
        ulBufSize = pBuf->GetSize();
        pucData   = (const char*)pBuf->GetBuffer();

        HX_ASSERT(NULL != pucData );
        
        res = HXR_OUTOFMEMORY;
	CreateBufferCCF(pNew, m_pContext);
        HX_ASSERT(pNew);
        if( pNew )
        {   
            //check for a zero chunk.
            int nZero = strncmp(pucData, "0\r\n", 3);
            if( 0 != nZero )
            {
                //Grab the size of the chunk.
                sscanf(pucData, "%X", &unChunkSize );

                HX_ASSERT(0!=unChunkSize);
                if( unChunkSize != 0 )
                {
                    //Find where the chunk starts and make sure it is
                    //actually that long.
                    const char* pszStartOfData = HXFindString( pucData, "\r\n");

                    if( NULL != pszStartOfData )
                    {
                        //Move past the CRLF
                        pszStartOfData += 2;
                    
                        if( pszStartOfData &&
                            unChunkSize+pszStartOfData <= ulBufSize+pucData-2)
                        {
                            //Now find the very last \r\n. It better be at the end.
                            nZero = strncmp( pszStartOfData+unChunkSize, "\r\n", 2 );
                            if( 0 == nZero )
                            {
                                //Verify it occurs and occurs just
                                //after the last byte of data.
                                res = pNew->SetSize(unChunkSize);
                                HX_ASSERT(SUCCEEDED(res));
                                if( SUCCEEDED(res) )
                                {
                                    memcpy(pNew->GetBuffer(), pszStartOfData, unChunkSize);
                                    //The amount of data we are consuming from here...
                                    ulBytesUsed = (pszStartOfData+unChunkSize)-pucData+2;
                                    res = HXR_OK;
                                }
                            }                        
                        }
                    }
                }
            }
            else
            {
                res = HXR_OK;
            }
        }
        if( !SUCCEEDED(res) )
        {
            HX_RELEASE(pNew);
        }
        
    }
    
    return res;
}


STDMETHODIMP HXCloakedV2TCPSocket::Write(IHXBuffer* pBuffer)
{
    IHXBuffer* pNew      = NULL;
    HX_RESULT  res       = HXR_FAIL;
    CHXString  GetString = "";
    
    
//    HX_ASSERT( csReady == m_CloakingState && pBuffer );

//    if( csReady == m_CloakingState && pBuffer )
    if( pBuffer )
    {
        pBuffer->AddRef();
        res = _GeneratePostRequest(pBuffer, pNew);
        HX_ASSERT(SUCCEEDED(res));
        if( SUCCEEDED(res) && pNew )
        {
            res = m_pTCPSocket->Write(pNew);
        }
        
        HX_RELEASE(pBuffer);
        HX_RELEASE(pNew);

        //We always follow our POST request with a Get request.
        GetString = _GenerateGetRequest();
        _WriteString(GetString);
    }
    
    return res;
}


HX_RESULT HXCloakedV2TCPSocket::GetOption( HXSockOpt name, UINT32* pVal)
{
    HX_RESULT res = HXR_UNEXPECTED;
    HX_ASSERT( m_pTCPSocket );
    if(m_pTCPSocket)
    {
        res = m_pTCPSocket->GetOption(name, pVal);
    }
    return res;    
}



HX_RESULT HXCloakedV2TCPSocket::SetOption( HXSockOpt name, UINT32 val)
{
//     HX_RESULT res = HXR_UNEXPECTED;
//     HX_ASSERT( m_pTCPSocket );
//     if(m_pTCPSocket)
//     {
//         res = m_pTCPSocket->SetOption(name, val);
//     }
//    return res;
    //XXXgfw don't let RTSP set our socket options.
    return HXR_OK;
}

HX_RESULT HXCloakedV2TCPSocket::SelectEvents( UINT32 uEventMask )
{
//     HX_RESULT res = HXR_UNEXPECTED;
//     HX_ASSERT( m_pTCPSocket );
//     if(m_pTCPSocket)
//     {
//         res = m_pTCPSocket->SelectEvents(uEventMask);
//     }
//     return res;
    //XXXgfw don't let RTSP override the events we need from the
    //Socket. We should probably keep a list of events and not
    //send the ones it wants.
    return HXR_OK;
}

HX_RESULT HXCloakedV2TCPSocket::ConnectToOne( IHXSockAddr* pAddr)
{
    HX_RESULT res = HXR_UNEXPECTED;

    HX_ASSERT( m_pTCPSocket );
    HX_ASSERT( csDisconnected == m_CloakingState );
    HX_ASSERT(!m_pDestAddr);
    HX_ASSERT(m_pTCPSocket);
    HX_ASSERT(m_pResolver);
    
    HX_RELEASE(m_pDestAddr);
    pAddr->Clone(&m_pDestAddr);
    
    //If we have a proxy, we need to resolve that host here. The
    //process will then continue in the GetAddrDone() method. Once
    //there we connect to the proxy and then both code paths merge in
    //EventPending with a connect event.
    if( csDisconnected == m_CloakingState )
    {
        if( m_pszProxyName && strlen(m_pszProxyName) )
        {
            res = m_pResolver->GetAddrInfo( m_pszProxyName, NULL, NULL);
        }
        else
        {
            res = m_pTCPSocket->ConnectToOne(m_pDestAddr);
        }
    }
    
    return res;
}


HX_RESULT HXCloakedV2TCPSocket::ConnectToAny( UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    HX_ASSERT("should not be called"==NULL);
    return HXR_NOTIMPL;
}


HX_RESULT HXCloakedV2TCPSocket::GetAddrInfoDone(HX_RESULT status,
                                                UINT32 nVecLen,
                                                IHXSockAddr** ppAddrVec)
{
    HX_RESULT res = HXR_FAIL;

    HX_ASSERT(0!=m_nProxyPort);

    //We are done getting the addr info for the proxy host.
    if( SUCCEEDED(status) && 1<=nVecLen )
    {
        HX_ASSERT(!m_pProxyAddr);
        HX_ASSERT(m_pTCPSocket);
        HX_RELEASE(m_pProxyAddr);
        ppAddrVec[0]->Clone(&m_pProxyAddr);
        m_pProxyAddr->SetPort(m_nProxyPort);
        if( m_pTCPSocket )
        {
            res = m_pTCPSocket->ConnectToOne(m_pProxyAddr);
        }
    }

    if( !SUCCEEDED(res) )
    {
        HX_RELEASE(m_pProxyAddr);
    }

    //hardcoded because I have no idea what the IHXSocket would do
    //with any other return code. Not sure why we return one at all.
    return HXR_OK;
}

HX_RESULT HXCloakedV2TCPSocket::GetNameInfoDone(HX_RESULT status,
                                                const char* pNode,
                                                const char* pService)
{
    //Cloaking TCP sockets done use this.
    HX_RESULT res = HXR_FAIL;
    HX_ASSERT("shoudn't be called"==NULL);
    return res;
}

void HXCloakedV2TCPSocket::_ResetResourceAndHash()
{
    CHXString tmp = "";
    
    //Generate a unique file name to use for this connection.
    _CreateRandomString(m_ResourceName);
    m_ResourceName += ".html";

    //Find a name to use for the name value pair that holds the
    //MD5 hash.
    int nIndex = RandBetween(0, sizeof(zm_NameList)/sizeof(char*)-1);
    m_NVPairName = zm_NameList[nIndex];

    //Compute the hash.
    char ucHash[65];
    tmp += m_ResourceName + zm_HashName;
    MD5Data( ucHash, (const unsigned char*)(const char*)tmp, tmp.GetLength() );
    ucHash[64] = '\0';
    m_NVPairValue = ucHash;

}


void HXCloakedV2TCPSocket::_CreateRandomString(CHXString& str )
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

CHXString HXCloakedV2TCPSocket::_GetHTTPResourcePath()
{
    //Generate the resource path. Its form depends on if we are going
    //through a proxy or not.   
    if( m_pszProxyName && m_pProxyAddr )
    {
        //We are going through a proxy. We have to add the full server
        //path in there.
        HXURLRep url(HXURLRep::TYPE_NETPATH, "HTTP", _GetHTTPHost(), m_pDestAddr->GetPort(), m_ResourceName);
        HX_ASSERT(url.IsValid());
        return url.String(); 
    }
    
    return CHXString("/") + m_ResourceName;

}

CHXString HXCloakedV2TCPSocket::_GetHTTPHost()
{
    CHXString  tmp  = "";
    IHXBuffer* pBuf = NULL;
    
    m_pDestAddr->GetAddr(&pBuf);
    tmp = pBuf->GetBuffer();
    HX_RELEASE(pBuf);
    return tmp;
}

CHXString HXCloakedV2TCPSocket::_GenerateGetRequest()
{
    CHXString  tmp  = "";
    CHXString  auth = _GetProxyAuthString();
    
    //Create the GET request. Here is a sample:
    //
    //       GET /37f572a87c23.html?name= 8941a043fbb22e98cdb257792a8bc269 HTTP/1.1
    //       Host: www.real.com
    //       Pragma: no-cache
    //       [CR/LF]
    tmp += "GET "+_GetHTTPResourcePath()+"?"+m_NVPairName+"="+m_NVPairValue+" HTTP/1.1";
    tmp += "\r\n";
    tmp += "Host: " + _GetHTTPHost();
    tmp += "\r\n";
    tmp += "Pragma: no-cache";
    tmp += "\r\n";
    tmp += "Cache-Control: private";
    tmp += "\r\n";
    if( !auth.IsEmpty() )
    {
        tmp += auth;
        tmp += "\r\n";
    }
    tmp += "\r\n";
    return tmp;
}

HX_RESULT HXCloakedV2TCPSocket::_GeneratePostRequest(IHXBuffer* pBuf,
                                                     IHXBuffer*& pNew)
{
    HX_RESULT  res = HXR_OUTOFMEMORY;
    CHXString  tmp  = "";
    CHXString  auth = _GetProxyAuthString();
    char       szTmp[11];
    
    //Create the PST request. Here is a sample:
    //
    //       POST /37f572a87c23.html?name= 8941a043fbb22e98cdb257792a8bc269 HTTP/1.1
    //       Host: www.real.com
    //       Pragma: no-cache
    //       Cache-Control: private
    //       Transfer-encoding: chunked
    //       [CR/LF]
    tmp += "POST "+_GetHTTPResourcePath()+"?"+m_NVPairName+"="+m_NVPairValue+" HTTP/1.1";
    tmp += "\r\n";
    tmp += "Host: " + _GetHTTPHost();
    tmp += "\r\n";
    tmp += "Pragma: no-cache";
    tmp += "\r\n";
    tmp += "Cache-Control: private";
    tmp += "\r\n";
//     tmp += "Transfer-encoding: chunked";
//     tmp += "\r\n";
    if( !auth.IsEmpty() )
    {
        tmp += auth;
        tmp += "\r\n";
    }
    tmp += "Content-Length: ";

    snprintf( szTmp, 10, "%d", pBuf->GetSize() );
    tmp += szTmp;
    tmp += "\r\n";

    //The end of message body \r\n.
    tmp += "\r\n";
    
    //copy POST body into new buffer and then the actual message.
    CreateBufferCCF(pNew, m_pContext);
    if( pNew )
    {
        res = pNew->SetSize(pBuf->GetSize() + tmp.GetLength());
        if( SUCCEEDED(res) )
        {
            UCHAR* pPtr = pNew->GetBuffer();
            memcpy( pPtr, (const char*)tmp, tmp.GetLength());
            pPtr += tmp.GetLength();
            memcpy( pPtr, pBuf->GetBuffer(), pBuf->GetSize() );
        }
    }

    if(!SUCCEEDED(res))
    {
        HX_RELEASE(pNew);
    }

    return res;
}


HX_RESULT HXCloakedV2TCPSocket::_EstablishGetAndPost()
{
    HX_RESULT res = HXR_FAIL;
    CHXString GetString  = "";
//    CHXString PostString = "";

    //inv.
    HX_ASSERT( m_pTCPSocket );
    HX_ASSERT( csConnected == m_CloakingState );

    //Generate our unique file name and hash for this connection.
    _ResetResourceAndHash();

    //Make the GET request.
    GetString = _GenerateGetRequest();

//     //Make the POST request.
//     PostString = _GeneratePostRequest();

    HX_ASSERT( !GetString.IsEmpty() );
//    HX_ASSERT( !PostString.IsEmpty() );
    
    //Send our Get and Post requests to the server or proxy.
    res = _WriteString(GetString);
    HX_ASSERT(SUCCEEDED(res));

//     if( SUCCEEDED(res) )
//     {
//         res = _WriteString(PostString);
//         HX_ASSERT(SUCCEEDED(res));
//     }

    if( SUCCEEDED(res) )
    {
        //Move cloaking into preping state.
        m_CloakingState = csPreping;
    }
    
    return res;
}

HX_RESULT HXCloakedV2TCPSocket::_WriteString(const CHXString& str)
{
    HX_RESULT   res    = HXR_FAIL;
    int         nCount = str.GetLength();
    IHXBuffer*  pBuf   = NULL;
    const char* pStr   = (const char*)str;
    
    //inv
    CreateBufferCCF(pBuf, m_pContext);
    HX_ASSERT(pBuf);
    HX_ASSERT(!str.IsEmpty());
    
    if( pBuf && !str.IsEmpty() )
    {
        res = pBuf->Set( (const unsigned char*)pStr, nCount);
        HX_ASSERT(SUCCEEDED(res));
        if( SUCCEEDED(res) )
        {
            res = m_pTCPSocket->Write(pBuf);
            HX_ASSERT(SUCCEEDED(res));
        }
        HX_RELEASE(pBuf);
    }

    return res;
}


CHXString HXCloakedV2TCPSocket::_GetProxyAuthString()
{
    IHXBuffer*   pBuffer       = NULL;
    CHXString    retStr        = "";
    CHXString    key           = "proxy-authentication.http:";
    IHXBuffer*   pHeaderBuffer = NULL;
    HX_RESULT    res           = HXR_OK;
    IHXRegistry* pRegistry     = NULL;
    CHXString    recentProxyAuth;
    
    res = m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);
    if( SUCCEEDED(res) )
    {
        res = pRegistry->GetStrByName("proxy-authentication.http.realm.recent", pHeaderBuffer);
        if (SUCCEEDED(res))
        {
            recentProxyAuth = CHXString((const char*)pHeaderBuffer->GetBuffer(),
                                        pHeaderBuffer->GetSize());
        }
        HX_RELEASE(pHeaderBuffer);

        key += m_pszProxyName;
        key += ":";
        key += recentProxyAuth;

        res = pRegistry->GetStrByName((const char*)key, pBuffer);
        if( SUCCEEDED(res) && pBuffer )
        {
            CHXString authString((const char*)pBuffer->GetBuffer(), pBuffer->GetSize());
            retStr = "Proxy-Authorization: ";
            retStr += authString;
        }
        HX_RELEASE(pBuffer);
    }
    HX_RELEASE(pRegistry);

    return retStr;
}

STDMETHODIMP HXCloakedV2TCPSocket::ResponseReady( HX_RESULT res, IHXRequest* pRequestResponse)
{
    IHXCommonClassFactory* pCCF       = NULL;
    IHXRegistry*           pRegistry  = NULL;
    IHXValues*             pHeaders   = NULL;
    const char*            pName      = NULL;
    IHXBuffer*             pBuf       = NULL;
    CHXString              key        = "";
    IHXBuffer*             pTmpBuffer = NULL;

    if( SUCCEEDED(res) )
    {
        //Extract the authentication info, again, this is a real waste to just
        //get a username password. See comment in _DoProxyAuthentication.
        res = m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&pCCF);
        if( SUCCEEDED(res) )
        {
            res = m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);
            if( SUCCEEDED(res) )
            {
                res = pRequestResponse->GetRequestHeaders(pHeaders);
                if( SUCCEEDED(res) )
                {
                    res = pHeaders->GetFirstPropertyCString(pName, pBuf);
                    while( SUCCEEDED(res) && pBuf )
                    {
                        if( !strcasecmp(pName, "Proxy-Authorization") )
                        {
                            CHXString  key = "proxy-authentication.http:";
                            key += m_pszProxyName;
                            key += ":";
                            res = pRegistry->GetStrByName( "proxy-authentication.http.realm.recent",
                                                           pTmpBuffer);
                            if( SUCCEEDED(res) )
                            {
                                key += CHXString((const char*)pTmpBuffer->GetBuffer(),
                                                 pTmpBuffer->GetSize());
                            }
                            HX_RELEASE(pTmpBuffer);

                            res = pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pTmpBuffer);
                            if( SUCCEEDED(res) )
                            {
                                pTmpBuffer->Set(pBuf->GetBuffer(), pBuf->GetSize());
                                
                                UINT32 regid = pRegistry->GetId((const char*)key);
                                if (!regid)
                                {
                                    pRegistry->AddStr((const char*)key, pTmpBuffer);
                                }
                                else
                                {
                                    pRegistry->SetStrByName((const char*)key, pTmpBuffer);
                                }
                            }
                            HX_RELEASE(pTmpBuffer);
                            break;
                        }

                        // get next header name value line
                        HX_RELEASE(pBuf);
                        res = pHeaders->GetNextPropertyCString(pName, pBuf);
                    }
                    HX_RELEASE(pBuf);
                    HX_RELEASE(pHeaders);
                }
            }
            HX_RELEASE(pRegistry);
        }
        HX_RELEASE(pCCF);
    }
    
    HX_RELEASE(pRequestResponse);

    //We need to start over connting again....
    m_CloakingState = csDisconnected;
    if( m_pTCPSocket)
    {
        m_pTCPSocket->Close();
        HX_RELEASE(m_pTCPSocket);
    }
    
    res = m_pNetworkServices->CreateSocket(&m_pTCPSocket);
    if(m_pTCPSocket && SUCCEEDED(res) )
    {
        res = m_pTCPSocket->SetResponse(this);
        if( SUCCEEDED(res) )
        {
            res = m_pTCPSocket->Init(m_family, m_type, m_protocol);
            m_pTCPSocket->SetOption( HX_SOCKOPT_APP_BUFFER_TYPE,
                                     HX_SOCKBUF_DEFAULT);
            m_pTCPSocket->SelectEvents(HX_SOCK_EVENT_READ|
                                       HX_SOCK_EVENT_CONNECT|
                                       HX_SOCK_EVENT_CLOSE);
        
            res = m_pTCPSocket->ConnectToOne(m_pDestAddr);
        }
    }

    return res;
}

HX_RESULT HXCloakedV2TCPSocket::_DoProxyAuthentication(HTTPResponseMessage* pMess)
{
    HX_RESULT                  res              = HXR_FAIL;
    IHXCommonClassFactory*     pCCF             = NULL;
    CHXString                  getStr           = "";
    IHXKeyValueList*           pRespHeaders     = NULL;
    MIMEHeaderValue*           pHeaderValue     = NULL;
    MIMEHeader*                pHeader          = NULL;
    CHXString                  strHeader        = "";
    CHXString                  strTemp          = "";
    IHXBuffer*                 pBuffer          = NULL;
    IHXRequest*                pRequest         = NULL;
    IHXValues*                 pValues          = NULL;             
    IHXClientAuthConversation* pClientAuth      = NULL;
    IUnknown*                  pUnkConfig       = NULL;
    IHXObjectConfiguration*    pConfig          = NULL;


    //////////////////////
    //
    // XXXgfw I really don't think we need all this authentication
    // garbage.  It really seems we only need to TLC to just give us a
    // dialog to do a user/pass request. It sure seems like this code
    // below is way overkill. Just check out ResponseReady(), the only
    // thing is needs from all this is just that one string. Oh well,
    // until I have time to figure out exactly how this all works (all
    // the calls through the core and into the TLC from here) I will
    // just leave it like the original cloaking.
    //
    //////////////////////
    
    res = m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&pCCF);
    if( SUCCEEDED(res) )
    {
        //pRequest will be freed in the ResponseReady() method.
        res = pCCF->CreateInstance(CLSID_IHXRequest, (void**)&pRequest);
        if( SUCCEEDED(res) )
        {
            getStr = _GenerateGetRequest();
            pRequest->SetURL((const char*)getStr); //Always returns HXR_OK;
            
            res = pCCF->CreateInstance(CLSID_IHXKeyValueList, (void**)&pRespHeaders);
            if( SUCCEEDED(res) )
            {
                
                pHeader = pMess->getFirstHeader();
                while (pHeader)
                {
                    pHeaderValue = pHeader->getFirstHeaderValue();
                    strHeader = "";
                    while( pHeaderValue )
                    {
                        pHeaderValue->asString(strTemp);
                        strHeader += strTemp;
                        pHeaderValue = pHeader->getNextHeaderValue();
                        if (pHeaderValue)
                        {
                            strHeader += ", ";
                        }
                    }
                    pBuffer = NULL;
                    CHXBuffer::FromCharArray((const char*)strHeader, &pBuffer);
                    if( pBuffer )
                    {
                        pRespHeaders->AddKeyValue(pHeader->name(), pBuffer);
                    }
                    HX_RELEASE(pBuffer);
                    pHeader = pMess->getNextHeader();
                }

                pValues = NULL;
                if (HXR_OK == pRespHeaders->QueryInterface(IID_IHXValues, (void**)&pValues))
                {
                    pRequest->SetResponseHeaders(pValues);
                }
                HX_RELEASE(pValues);
            }

            HX_RELEASE(pRespHeaders);

            if (HXR_OK == pRequest->GetResponseHeaders(pValues))
            {
                res = pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
                if (SUCCEEDED(res))
                {
                    pBuffer->Set((UCHAR*)m_pszProxyName, strlen(m_pszProxyName)+1);
                    pValues->SetPropertyCString("_server", pBuffer);
                    HX_RELEASE(pBuffer);
                }

                // Add the protocol to the response headers because TLC needs it
                if (SUCCEEDED(pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer)))
                {
                    pBuffer->Set((UCHAR*)"http", strlen("http") + 1);
                    pValues->SetPropertyCString("_protocol", pBuffer);
                    HX_RELEASE(pBuffer);
                }

                HX_RELEASE(pValues);
            }

            //We now need a CCF from the cloaking context, not our core's context.
            HX_RELEASE(pCCF);


            res = m_pCloakContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&pCCF);
            if(SUCCEEDED(res))
            {
                res = pCCF->CreateInstance( CLSID_CHXClientAuthenticator, (void**)&pUnkConfig );
                if(SUCCEEDED(res))
                {
                    res = pUnkConfig->QueryInterface(IID_IHXClientAuthConversation, 
                                                     (void**)&pClientAuth);
                    if (SUCCEEDED(res))
                    {
                        res = pUnkConfig->QueryInterface(IID_IHXObjectConfiguration,
                                                         (void**)&pConfig);
                        if (SUCCEEDED(res))
                        {
                            pConfig->SetContext(m_pCloakContext);
                            HX_RELEASE(pConfig);
                        }

                        if ( pClientAuth && !pClientAuth->IsDone() )
                        {
                            HX_ASSERT(pRequest);
                            //After this if/else, we continue in ResponseReady() after
                            //the TLC calls us back.
                            if (pRequest)
                            {
                                res = pClientAuth->MakeResponse(this, pRequest);
                            }
                            else
                            {
                                // Auth Failed!
                                pClientAuth->Authenticated(FALSE);
                                res = HXR_NOT_AUTHORIZED;
                                res = ResponseReady(res, pRequest);
                            }
                        }
                    }
                    HX_RELEASE(pUnkConfig);
                }
                HX_RELEASE(pCCF);
            }
        }
    }

    //We return this in all cases. the connection will be started over in
    //the ResponseReady() method.
    res = HXR_NOT_AUTHORIZED;
    return res;
}




HX_RESULT HXCloakedV2TCPSocket::_ParseGetResponse()
{
    HX_RESULT            res         = HXR_FAIL;
    HTTPResponseMessage* pMess       = NULL;
    ULONG32              ulLength    = 0;
    ULONG32              ulErrorCode = 0;
    IHXBuffer*           pBuf        = NULL;
    HTTPParser           parser;

    res = _GetReadQueue(pBuf);
    
    if( pBuf && SUCCEEDED(res) )
    {
        res = HXR_FAIL;
        ulLength = pBuf->GetSize();
#if !defined HELIX_FEATURE_SERVER
        pMess = (HTTPResponseMessage*)parser.parse( (const char*)pBuf->GetBuffer(), ulLength);
#else
	BOOL bMsgTooLarge = FALSE;
        pMess = (HTTPResponseMessage*)parser.parse( (const char*)pBuf->GetBuffer(), ulLength, bMsgTooLarge);
#endif /* !HELIX_FEATURE_SERVER */
        HX_ASSERT(pMess);

        //Verify we have HTTP response message.
        if( pMess && HTTPMessage::T_UNKNOWN != pMess->tag() )
        {
            if( pMess->errorCode() && strlen(pMess->errorCode()))
            {
                ulErrorCode = atoi(pMess->errorCode());

                //Verify we have HTTP 1.1.
                if( pMess->majorVersion()==1 && pMess->minorVersion()==1)
                {
                    if( ulErrorCode == 401 || ulErrorCode == 407 )
                    {
                        //Proxy authentication is required. Close up the Socket,
                        //get authentication and try again.
                        m_pTCPSocket->Close();
                        res = _DoProxyAuthentication(pMess);
                    }

                    //Verify we have 200 OK.
                    else if( 200 == ulErrorCode )
                    {
                        res = HXR_OK;
                    }
                }
            }

            // We need to put back any used data. Find the trailing
            // \r\n and put back all data after that.
            const char* pucData      = (const char*)pBuf->GetBuffer();
            const char* pszEndOfMess = HXFindString( pucData, "\r\n\r\n");
            if( NULL != pszEndOfMess )
            {
                pszEndOfMess += 4; //skip past \r\n\r\n.
                m_ReadQueue.EnQueue( pszEndOfMess,
                                     pBuf->GetSize()-(pszEndOfMess-pucData));
            }
        }
    }
    
    HX_RELEASE(pBuf);
    return res;
}

HX_RESULT HXCloakedV2TCPSocket::_ReadFromSocket()
{
    HX_RESULT  res  = HXR_FAIL;
    IHXBuffer* pBuf = NULL;

    res = m_pTCPSocket->Read(&pBuf);
    if( pBuf && SUCCEEDED(res) )
    {
        //XXXgfw performance problem here. All data is in IHXBuffers
        //up until this point. Now we are doing a memcpy for each
        //packet received. Bad bad bad.
        res = m_ReadQueue.EnQueue(pBuf->GetBuffer(), pBuf->GetSize());
        HX_RELEASE(pBuf);
    }
     
    return res;
}


HX_RESULT HXCloakedV2TCPSocket::_GetReadQueue(IHXBuffer*& pNew )
{
    HX_RESULT res     = HXR_FAIL;
    ULONG32   ulBytes = 0;

    ulBytes = m_ReadQueue.GetQueuedItemCount();
    //3 is the minimum chunk size.
    if( ulBytes >= 3 )
    {
        res = HXR_OUTOFMEMORY;
	CreateBufferCCF(pNew, m_pContext);
        if( pNew )
        {
            res = pNew->SetSize(ulBytes);
            HX_ASSERT(SUCCEEDED(res));
            if( SUCCEEDED(res) )
            {
                m_ReadQueue.DeQueue(pNew->GetBuffer(), ulBytes);
            }
        }
        
        if( !SUCCEEDED(res) )
        {
            HX_RELEASE(pNew);
        }
    }

    return res;
}


HX_RESULT HXCloakedV2TCPSocket::_ReadChunk(ULONG32& ulSizeRead )
{
    HX_RESULT  res         = HXR_FAIL;
    IHXBuffer* pBuf        = NULL;
    IHXBuffer* pNew        = NULL;
    ULONG32    ulBytesUsed = 0;
    
    HX_ASSERT( csReady    == m_CloakingState ||
               csPostWait == m_CloakingState ||
               csGetWait  == m_CloakingState );

    res = _GetReadQueue(pBuf);
    if( pBuf && SUCCEEDED(res) )
    {
        res = _UnChunkBuffer(pBuf, pNew, ulBytesUsed );
        if( SUCCEEDED(res)  )
        {
            ulSizeRead = pNew->GetSize();
            //Enque the data onto the RTSP queue.
            res = m_RTSPQueue.EnQueue(pNew->GetBuffer(), ulSizeRead);

            //Put back any unused data
            if( ulBytesUsed < pBuf->GetSize() )
            {
                m_ReadQueue.EnQueue(pBuf->GetBuffer()+ulBytesUsed,
                                     pBuf->GetSize()-ulBytesUsed);
            }
            res = HXR_OK;
        }
        else
        {
            //We do not have a full chunk yet. Put our data back on
            //the read queue and wait for the full chunk to come in.
            m_ReadQueue.EnQueue(pBuf->GetBuffer(), pBuf->GetSize());
        }
        HX_RELEASE(pNew);
        HX_RELEASE(pBuf);
    }

    return res;
}


HX_RESULT HXCloakedV2TCPSocket::_HandleReadEvent()
{
    HX_RESULT res    = HXR_FAIL;
    ULONG32   ulSize = 0;
    
    //Read any data on the wire into our input queue.
    res = _ReadFromSocket();
    HX_ASSERT( SUCCEEDED(res) );

    if( SUCCEEDED(res) )
    {
        //One of these should always taks care of reading data off of our
        //wire queue and putting it into the RTSP queue.
        switch( m_CloakingState )
        {
           case csPreping:
               res = _ParseGetResponse();
               if( SUCCEEDED(res) )
               {
                   //Ok, send the machine to csReady.
                   m_CloakingState = csPostWait;
               }

               //If we receive a HXR_NOT_AUTHORIZED while preping,
               //then we are waiting on authorization traffic to take
               //place. So, change states again and keep going.
               if( HXR_NOT_AUTHORIZED != res )
               {
                   //Inform the RTSP layer...
                   m_pTCPResponse->EventPending( HX_SOCK_EVENT_CONNECT, res);
               }
               break;
               
           case csReady:
               //We can only get 2 things here. A good chunk read with
               //data to pass onto the RTSP layer, a zero chunk signaling
               //the end of data from the server. If we receive a zero
               //chunk before sending one on the POST channel then the
               //server is closing the connection due to error or end of
               //stream. If we receive anything other then a valid chunk,
               //it is an error.

               //Read a chunk off the wire. It will be put into the RTSP
               //queue. If we only received a partial chunk this call will
               //fail. That is ok, the next read event may bring the rest
               //of it (or next several read events). Also, we may get
               //several chunks in one TCP read.
               res = HXR_OK;
               while( SUCCEEDED(res) )
               {
                   res = _ReadChunk(ulSize);           
                   if( SUCCEEDED(res) && 0 == ulSize)
                   {
                       //We got a zero chunk.
                       m_CloakingState = csPostWait;
                   }
               }

               //If we got anything in the RTSP queue, let the RTSP
               //layer know that it can read.
               if( 0 != m_RTSPQueue.GetQueuedItemCount() )
               {
                   m_pTCPResponse->EventPending(HX_SOCK_EVENT_READ, HXR_OK );
               }
               
               break;
               
           case csPostWait:
               //We have received our zero chunk and now await the
               //post reply.
               res = _ParseGetResponse();
               if( SUCCEEDED(res) )
               {
                   //Ok, send the machine to csReady.
                   m_CloakingState = csGetWait;
               }

               break;
               
           case csGetWait:
               //We have received our POST response and now wait on
               //our GET response again.
               res = _ParseGetResponse();
               if( SUCCEEDED(res) )
               {
                   //Ok, send the machine to csReady.
                   m_CloakingState = csReady;
               }

               break;
               
           case csTearingDown:
               //Ignore all reads. Something has gone wrong and we are
               //tearing everything down...
               break;

           default:
               HX_ASSERT("invalid state for read"==NULL);
               break;
        };  
    }
    
    return res;
}


HX_RESULT HXCloakedV2TCPSocket::EventPending( UINT32 uEvent,
                                              HX_RESULT status)
{
    HX_RESULT  res  = HXR_OK;

    HX_ASSERT( SUCCEEDED(status) );
    
    switch( uEvent )
    {
       case HX_SOCK_EVENT_READ:
           
           _HandleReadEvent();
           break;
           
       case HX_SOCK_EVENT_CONNECT:
           HX_ASSERT( csDisconnected == m_CloakingState);
           if( SUCCEEDED(status) )
           {
               //We have our single TCP connection to the server. We
               //now Set up the GET and POST channels.
               m_CloakingState = csConnected;
               res = _EstablishGetAndPost();
           }
           
           if( !SUCCEEDED(res) )
           {
               //We can't establish a connection. Tell the RTSP
               //layer we are done.
               m_pTCPResponse->EventPending( HX_SOCK_EVENT_CONNECT, res);
           }
           break;

       case HX_SOCK_EVENT_CLOSE:
           break;

       default:
           HX_ASSERT("should not get this event"==NULL);
           break;
    }

    //We blindly set the return code here.
    res = HXR_OK;
    
    return res;
}


HX_RESULT HXCloakedV2TCPSocket::Close()
{
    HX_RESULT res = HXR_FAIL;

//     //Send a zero chunk to the server to let it know we are done.
//     if( csReady == m_CloakingState )
//     {
//         res = HXR_OUTOFMEMORY;
//         IHXBuffer* pBuf = NULL;
//	   CreateBufferCCF(pBuf, m_pContext);	    
//         if( pBuf )
//         {
//             res = pBuf->Set((const unsigned char*)"0\r\n", 3 );
//             HX_ASSERT(SUCCEEDED(res));
//             if( SUCCEEDED(res) )
//             {
//                 res = m_pTCPSocket->Write(pBuf);
//             }
//             HX_RELEASE(pBuf);
//         }
//     }
    
    m_CloakingState = csDisconnected;

    if( m_pTCPSocket )
    {
        //Now clean up the socket.
        m_pTCPSocket->Close();
        HX_RELEASE(m_pTCPSocket);
    }

    //Clean up resolver
    HX_RELEASE(m_pResolver);
    
    return res;
}



//
// Not timplemented....
//
HX_RESULT HXCloakedV2TCPSocket::Listen( UINT32 uBackLog)
{
    HX_RESULT res = HXR_NOTIMPL;
    return res;
}

HX_RESULT HXCloakedV2TCPSocket::Accept( IHXSocket** ppNewSock, IHXSockAddr** ppSource)
{
    HX_RESULT res = HXR_NOTIMPL;
    return res;
}

HX_RESULT HXCloakedV2TCPSocket::PeekFrom( IHXBuffer** ppBuf, IHXSockAddr** ppAddr)
{
    HX_RESULT res = HXR_NOTIMPL;
    return res;
}

HX_RESULT HXCloakedV2TCPSocket::ReadFrom( IHXBuffer** ppBuf, IHXSockAddr** ppAddr)
{
    HX_RESULT res = HXR_NOTIMPL;
    return res;
}

HX_RESULT HXCloakedV2TCPSocket::WriteTo( IHXBuffer* pBuf, IHXSockAddr* pAddr)
{
    HX_RESULT res = HXR_NOTIMPL;
    return res;
}

HX_RESULT HXCloakedV2TCPSocket::ReadV( UINT32 nVecLen, UINT32* puLenVec, IHXBuffer** ppBufVec)
{
    HX_RESULT res = HXR_NOTIMPL;
    return res;
}

HX_RESULT HXCloakedV2TCPSocket::ReadFromV( UINT32 nVecLen,
                                           UINT32* puLenVec,
                                           IHXBuffer** ppBufVec,
                                           IHXSockAddr** ppAddr)
{
    HX_RESULT res = HXR_NOTIMPL;
    return res;
}

HX_RESULT HXCloakedV2TCPSocket::WriteV( UINT32 nVecLen,
                                        IHXBuffer** ppBufVec)
{
    HX_RESULT res = HXR_NOTIMPL;
    return res;
}

HX_RESULT HXCloakedV2TCPSocket::WriteToV( UINT32 nVecLen,
                                          IHXBuffer** ppBufVec,
                                          IHXSockAddr* pAddr)
{
    HX_RESULT res = HXR_NOTIMPL;
    return res;
}

HX_RESULT HXCloakedV2TCPSocket::CreateSockAddr( IHXSockAddr** ppAddr)
{
    HX_RESULT res = HXR_NOTIMPL;
    return res;
}

HX_RESULT HXCloakedV2TCPSocket::Peek( IHXBuffer** ppBuf)
{
    HX_RESULT res = HXR_NOTIMPL;
    return res;
}

HX_RESULT HXCloakedV2TCPSocket::SetAccessControl( IHXSocketAccessControl* pControl)
{
    HX_RESULT res = HXR_NOTIMPL;
    return res;
}

HXSockFamily HXCloakedV2TCPSocket::GetFamily()
{
    return HX_SOCK_FAMILY_NONE;
}

HXSockType HXCloakedV2TCPSocket::GetType()
{
    return HX_SOCK_TYPE_NONE;
}

HXSockProtocol HXCloakedV2TCPSocket::GetProtocol()
{
    return HX_SOCK_PROTO_NONE;
}

