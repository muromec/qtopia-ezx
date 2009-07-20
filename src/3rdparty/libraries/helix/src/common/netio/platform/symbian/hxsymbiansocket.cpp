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

#include "hxcom.h"
#include "hxnet.h"
#include "hxassert.h"
#include "hxbuffer.h"
#include "hxtbuf.h"
#include "hxtick.h"

#include "hxsymbiannet.h"
#include "hxsymbianresolver.h"
#include "hxsymbiansocket.h"
#include "hxsymbiansockaddr.h"

#include "hxtlogutil.h"

#define UDP_READ_SIZE_READCONTINUATION 4096

CHXSymbianSocket::CHXSymbianSocket(CHXNetServices* pNetSvc, IUnknown* pContext):
    m_nRefCount(0)
    ,m_pNetSvc(pNetSvc)
    ,m_pContext(pContext)
    ,m_pCCF(NULL)
    ,m_pScheduler(NULL)
    ,m_pResponse(NULL)
    ,m_pAccessControl(NULL)
    ,m_pAPManager(NULL)
    ,m_pAPResponse(NULL)
    ,m_bufType(HX_SOCKBUF_DEFAULT)
    ,m_readBufAlloc(HX_SOCK_READBUF_SIZE_DEFAULT)
    ,m_state(EClosed)
    ,m_pConnector(NULL)
    ,m_pWriter(NULL)
    ,m_pReader(NULL)
    ,m_bConnected(EFalse)
    ,m_ulSocketTaskFlags(0)
    ,m_ulEventMask(0) 
    ,m_pDataBuf(NULL)
    ,m_pReadersBuf(NULL)
    ,m_uReadSize(0)
    ,m_pUdpConnectAddr(NULL)
    ,m_bInitialized(FALSE)
{
    HX_ADDREF(m_pNetSvc);
    HX_ASSERT(m_pContext);
    if (m_pContext != NULL)
    {
        m_pContext->AddRef();
        m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
        m_pContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
    }
    HX_ASSERT(m_pCCF != NULL);
}

CHXSymbianSocket::~CHXSymbianSocket(void)
{
    // make sure the cleanup always takes place
    Close();
    HX_DELETE(m_pConnector);
    HX_DELETE(m_pWriter);
    HX_DELETE(m_pReader);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pAPManager);
    HX_RELEASE(m_pNetSvc);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pReadersBuf);
    HX_RELEASE(m_pDataBuf);
    HX_DELETE(m_pUdpConnectAddr);
    if (m_pAPResponse)
    {
        m_pAPResponse->ClearPointers();
    }
    HX_RELEASE(m_pAPResponse);
}

STDMETHODIMP
CHXSymbianSocket::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSocket))
    {
        AddRef();
        *ppvObj = (IHXSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXMulticastSocket))
    {
        AddRef();
        *ppvObj = (IHXMulticastSocket*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXSymbianSocket::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXSymbianSocket::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP_(HXSockFamily)
CHXSymbianSocket::GetFamily(void)
{
    return m_family;
}

STDMETHODIMP_(HXSockType)
CHXSymbianSocket::GetType(void)
{
    return m_type;
}

STDMETHODIMP_(HXSockProtocol)
CHXSymbianSocket::GetProtocol(void)
{
    return m_proto;
}

STDMETHODIMP
CHXSymbianSocket::SetResponse(IHXSocketResponse* pResponse)
{
    HX_RELEASE(m_pResponse);
    m_pResponse = pResponse;
    HX_ADDREF(m_pResponse);
    return HXR_OK;
}

STDMETHODIMP
CHXSymbianSocket::SetAccessControl(IHXSocketAccessControl* pControl)
{
    HX_RELEASE(m_pAccessControl);
    m_pAccessControl = pControl;
    HX_ADDREF(m_pAccessControl);
    return HXR_OK;
}

STDMETHODIMP
CHXSymbianSocket::Init(HXSockFamily f, HXSockType t, HXSockProtocol p)
{
    HX_RESULT hxr = HXR_OK;

    m_pConnector    = new CHXSymbianSocketConnector(this);
    m_pWriter       = new CHXSymbianSocketWriter(this);
    m_pReader       = new CHXSymbianSocketReader(this);
    if( !m_pConnector || !m_pWriter || !m_pReader )
    {
        hxr = HXR_OUTOFMEMORY;
    } 
    else
    {
        m_family = f;
        m_type   = t;
        m_proto  = p;
        // determine the preferred size of the read buffer
	    if(IS_STREAM_TYPE(m_type))
        {
            m_uReadSize = DEFAULT_TCP_READ_SIZE;
        }
        else
        {
            // same for udp and multicast
            m_uReadSize = UDP_READ_SIZE_READCONTINUATION;
        }

        m_pNetSvc->QueryInterface(IID_IHXAccessPointManager,
              (void**)&m_pAPManager);

        if (m_pAPManager == NULL)
        {
            hxr = HXR_FAILED;
        }
        else if( (m_pAPResponse = new HXAccessPointConnectResp(this,
                      static_APConnectDone)) == NULL
           )
        {
            hxr = HXR_OUTOFMEMORY;
        }
        else
        {
            HX_ADDREF(m_pAPResponse);
            m_state = EOpen;
        }
    }

    if(hxr != HXR_OK)
    {
        HX_DELETE(m_pConnector);
        HX_DELETE(m_pWriter);
        HX_DELETE(m_pReader);
        HX_RELEASE(m_pAPManager);
        HX_RELEASE(m_pAPResponse);
    }
    return hxr;
}

STDMETHODIMP
CHXSymbianSocket::CreateSockAddr(IHXSockAddr** ppAddr)
{
    return m_pNetSvc->CreateSockAddr(m_family, ppAddr);
}


STDMETHODIMP
CHXSymbianSocket::Bind(IHXSockAddr* pAddr)
{
    HX_RESULT hxr = HXR_OK;
    
    HX_ASSERT(pAddr);
    GetNativeAddr(pAddr, m_localAddr);
    m_state = EBindPending;
    // connect to the access point
    // XXX. This needs to become asynchronous.
    // hxr = m_pAPManager->Connect(m_pAPResponse);
    hxr = OnAccessPointConnect(HXR_OK);
    if(hxr == HXR_OK && !IS_STREAM_TYPE(m_type))
        SendEvents(HX_SOCK_EVENT_READ|
               HX_SOCK_EVENT_WRITE, HXR_OK);
    return hxr;
}

HX_RESULT
CHXSymbianSocket::OnBind(HX_RESULT hxr)
{
    HX_RESULT res=hxr;
    if(m_state == EBindPending)
    {
        if(hxr == HXR_OK)
        {
            if (m_sock.Bind(m_localAddr) != KErrNone)
            {
                HXLOGL3(HXLOG_NETW, "CHXSymbianSocket::OnBind RSocket::Bind failed");
                res = HXR_FAILED;
            }
            else
            {
                m_state = EBound;
            }
        }
        
        // Bind event sending
        /*
        if(HX_SOCK_EVENT_BIND & m_ulEventMask)
        {
            m_pResponse->EventPending(HX_SOCK_EVENT_BIND, hxr);
        }
        */
    }
    return res;
}

STDMETHODIMP
CHXSymbianSocket::ConnectToOne(IHXSockAddr* pAddr)
{

    HXLOGL4(HXLOG_NETW, "CHXSymbianSocket::ConnectToOne");
    HX_RESULT hxr=HXR_OK;
    HX_ASSERT(pAddr);
    if(IS_STREAM_TYPE(m_type))
    {
        GetNativeAddr(pAddr, m_peerAddr);
        m_state = EConnectionPending;
        // connect to the access point
        hxr = m_pAPManager->Connect(m_pAPResponse);
    }
    else
    {
        // rather than actual connect on RSocket we simulate the connect.
        // store the connect address to be used for I/O 
        m_pUdpConnectAddr = new TInetAddr();
        if(m_pUdpConnectAddr)
        {
            GetNativeAddr(pAddr, *m_pUdpConnectAddr);
            m_state = EConnected;
            SendEvents(HX_SOCK_EVENT_CONNECT|
                       HX_SOCK_EVENT_READ|
                       HX_SOCK_EVENT_WRITE, HXR_OK);
        }
        else
        {
            hxr = HXR_OUTOFMEMORY;
        }
    }
    HXLOGL3(HXLOG_NETW, "CHXSymbianSocket::ConnectToOne hxr=%d", hxr);
    return hxr;
}

STDMETHODIMP
CHXSymbianSocket::ConnectToAny(UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    HX_RESULT hxr = HXR_INVALID_PARAMETER;
    if(nVecLen >= 1)
    {
        hxr = ConnectToOne(ppAddrVec[0]);
    }
    return hxr;
}

STDMETHODIMP
CHXSymbianSocket::GetLocalAddr(IHXSockAddr** ppAddr)
{
    HX_RESULT hxr = HXR_OK;

    hxr = m_pNetSvc->CreateSockAddr(m_family, ppAddr);
    if (SUCCEEDED(hxr))
    {
        TInetAddr inetAddr;
        m_sock.LocalName(inetAddr);
        SetNativeAddr(*ppAddr, inetAddr);
    }
    return hxr;
}

STDMETHODIMP
CHXSymbianSocket::GetPeerAddr(IHXSockAddr** ppAddr)
{
    HX_RESULT hxr = m_pNetSvc->CreateSockAddr(m_family, ppAddr);
    if (SUCCEEDED(hxr))
    {
        if(m_bConnected)
        {
            TInetAddr inetAddr;
            m_sock.RemoteName(inetAddr);
            SetNativeAddr(*ppAddr, inetAddr);
        }
        else if(m_pUdpConnectAddr)
        {
            SetNativeAddr(*ppAddr, *m_pUdpConnectAddr);
        }
        else
        {
            hxr = HXR_FAILED;
        }
    }
    return hxr;
}

STDMETHODIMP
CHXSymbianSocket::SelectEvents(UINT32 uEventMask)
{
    m_ulEventMask = uEventMask;
    // send read event for UDP sockets
	if((!IS_STREAM_TYPE(m_type)) && (HX_SOCK_EVENT_READ & m_ulEventMask))
    {
        m_pResponse->EventPending(HX_SOCK_EVENT_READ, HXR_OK);
    }
    return HXR_OK;
}


STDMETHODIMP
CHXSymbianSocket::Listen(UINT32 uBackLog)
{
    return HXR_NOTIMPL;
}


STDMETHODIMP
CHXSymbianSocket::Accept(IHXSocket** ppNewSock, IHXSockAddr** ppSource)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXSymbianSocket::GetOption(HXSockOpt name, UINT32* pval)
{
    switch (name)
    {
    case HX_SOCKOPT_APP_BUFFER_TYPE:
        *pval = m_bufType;
        return HXR_OK;
    case HX_SOCKOPT_APP_READBUF_ALLOC:
        *pval = m_readBufAlloc;
        return HXR_OK;
    default:
        HXLOGL1(HXLOG_NETW, "CHXSymbianSocket::SetOption %d nothing is done", name);
        return HXR_OK;
    }
}


STDMETHODIMP
CHXSymbianSocket::SetOption(HXSockOpt name, UINT32 val)
{
    HXLOGL4(HXLOG_NETW, "CHXSymbianSocket::SetOption name=%d val=%d", name, val);
    // XXXAG not all options are implemented. add other options as required.
    switch (name)
    {
    case HX_SOCKOPT_APP_IDLETIMEOUT:
        return HXR_OK;
    case HX_SOCKOPT_APP_BUFFER_TYPE:
        if (val != HX_SOCKBUF_DEFAULT &&
            val != HX_SOCKBUF_TIMESTAMPED)
        {
            return HXR_FAIL;
        }
        m_bufType = (HXSockBufType)val;
        return HXR_OK;
    case HX_SOCKOPT_APP_READBUF_ALLOC:
        if (val != HX_SOCK_READBUF_SIZE_DEFAULT &&
            val != HX_SOCK_READBUF_SIZE_COPY)
        {
            return HXR_FAIL;
        }
        m_readBufAlloc = (HXSockReadBufAlloc)val;
        return HXR_OK;
    case HX_SOCKOPT_APP_READBUF_MAX:
        return HXR_NOTIMPL;
    case HX_SOCKOPT_APP_SNDBUF:
        return HXR_NOTIMPL;
    case HX_SOCKOPT_APP_AGGLIMIT:
        return HXR_NOTIMPL;
    default:
        HX_ASSERT(false);
        HXLOGL1(HXLOG_NETW, "CHXSymbianSocket::SetOption %d nothing is done", name);
        return HXR_NOTIMPL;
    }
    return HXR_NOTIMPL;
}

HX_RESULT
CHXSymbianSocket::DoClose(HX_RESULT hxr)
{
    if (m_state != EClosed)
    {
        // Cancel pending operations.
        if (m_pConnector->IsActive())
        {
            m_sock.CancelConnect();
            m_pConnector->Cancel();
        }

        if (m_pWriter->IsActive())
        {
            m_sock.CancelWrite();
            m_pWriter->Cancel();
        }

        if (m_pReader->IsActive())
        {
            m_sock.CancelRecv();
            m_pReader->Cancel();
        }

        // Clear the writer list
        while (m_writeList.GetCount() > 0)
        {
            CSocketWriteReq* pWriteReq = (CSocketWriteReq*)m_writeList.RemoveHead();
            delete pWriteReq;
        }
        
        if(m_bInitialized)
        {   
            m_bInitialized = FALSE;
            m_sock.Close();
        }

        m_state = EClosed;
    }
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pAccessControl);
    return HXR_OK;
}

STDMETHODIMP
CHXSymbianSocket::Peek(IHXBuffer** ppBuf)
{
    return DoRead(ppBuf, TRUE);
}

STDMETHODIMP
CHXSymbianSocket::Read(IHXBuffer** ppBuf)
{
    return DoRead(ppBuf);
}

STDMETHODIMP
CHXSymbianSocket::PeekFrom(IHXBuffer** ppBuf, IHXSockAddr** ppAddr)
{
    return DoReadFrom(ppBuf, ppAddr, TRUE);
}

STDMETHODIMP
CHXSymbianSocket::ReadFrom(IHXBuffer** ppBuf, IHXSockAddr** ppAddr)
{
    return DoReadFrom(ppBuf, ppAddr);
}


HX_RESULT
CHXSymbianSocket::DoRead(IHXBuffer** ppBuf, HXBOOL bPeek)
{
    HX_RESULT hxr = HXR_SOCK_WOULDBLOCK;
    if(m_pDataBuf)
    {
        hxr = HXR_OK;
        HX_RELEASE(*ppBuf);
        *ppBuf  = m_pDataBuf;
        (*ppBuf)->AddRef();
        // release the referrence if we are not peeking
        if (!bPeek)
        {
            m_ulSocketTaskFlags &= ~HX_SOCK_EVENT_READ;
            HX_RELEASE(m_pDataBuf);
        }
    }
    // check if a read task is already pending
    if(m_ulSocketTaskFlags & HX_SOCK_EVENT_READ)
        return hxr;

    // allocate a new readers buffer
    if( (CreateReadBuffer(m_pReadersBuf, m_uReadSize) == HXR_OK) )
    {
        m_ulSocketTaskFlags |= HX_SOCK_EVENT_READ;
        TInetAddr *pInetAddr = NULL;
        if(!m_bConnected)
            pInetAddr = &m_readFrom;
        m_pReader->Read(m_sock, m_pReadersBuf, pInetAddr);
    }
    return hxr;
}


HX_RESULT
CHXSymbianSocket::DoReadFrom(IHXBuffer** ppBuf, IHXSockAddr** ppAddr, HXBOOL bPeek)
{
    HX_RESULT hxr = HXR_SOCK_WOULDBLOCK;
    
    // some data is available
    if(m_pDataBuf)
    {
        hxr = m_pNetSvc->CreateSockAddr(m_family, ppAddr);
        if(!SUCCEEDED(hxr))
        {
            return hxr;
        }
        SetNativeAddr(*ppAddr, m_readFrom);
        HX_RELEASE(*ppBuf);
        *ppBuf  = m_pDataBuf;
        (*ppBuf)->AddRef();
        
        // release the buffer if we are not peeking
        if (!bPeek)
        {
            m_ulSocketTaskFlags &= ~HX_SOCK_EVENT_READ;
            HX_RELEASE(m_pDataBuf);
        }
    }
    // check if a read task is already pending
    if(m_ulSocketTaskFlags & HX_SOCK_EVENT_READ)
        return hxr;

    // allocate a new readers buffer
    if( (CreateReadBuffer(m_pReadersBuf, m_uReadSize) == HXR_OK) )
    {
        m_ulSocketTaskFlags |= HX_SOCK_EVENT_READ;
        m_pReader->Read(m_sock, m_pReadersBuf, &m_readFrom);
    }
    return hxr;
}

STDMETHODIMP
CHXSymbianSocket::Write(IHXBuffer* pBuf)
{
    return DoWrite(pBuf, NULL);
}

STDMETHODIMP
CHXSymbianSocket::Close()
{
    return DoClose(HXR_OK);
}

STDMETHODIMP
CHXSymbianSocket::WriteTo(IHXBuffer* pBuf, IHXSockAddr* pAddr)
{
    return DoWrite(pBuf, pAddr);
}

HX_RESULT
CHXSymbianSocket::DoWrite(IHXBuffer* pBuf, IHXSockAddr* pAddr)
{
    HX_RESULT res = HXR_OK;
    HX_ASSERT(pBuf);


    //if the socket is not open and ready do not attempt a write
    if((m_state == EClosed) || (m_state == EClosePending))
    {
        return HXR_INVALID_OPERATION;
    }

    if (m_pWriter->IsActive())
    {
        // write already in progress. queue the request.
        CSocketWriteReq* pWriteReq = CSocketWriteReq::Construct(pAddr, pBuf);
        if(pWriteReq)
        {
            LISTPOSITION listRet = m_writeList.AddTail(pWriteReq);
            if( listRet == NULL )
            {
                res = HXR_OUTOFMEMORY;
                delete pWriteReq;
            }
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }
    else
    {
        TInetAddr *dest=NULL;
        if(!m_bConnected)
        {
            if(m_pUdpConnectAddr)
            {
                dest = m_pUdpConnectAddr;
            }
            else
            {
                if(pAddr)
                {
                    GetNativeAddr(pAddr, m_writeTo);
                    dest = &m_writeTo;
                }
                else
                {
                    HX_ASSERT(false);
                    res = HXR_FAILED;
                }
            }
        }
        m_pWriter->Write(m_sock, pBuf, dest);
    }
    return res;
}

STDMETHODIMP
CHXSymbianSocket::ReadV(UINT32 nVecLen, UINT32* puLenVec,
                IHXBuffer** ppBufVec)
{
    return DoReadV(nVecLen, puLenVec, ppBufVec, NULL);
}



STDMETHODIMP
CHXSymbianSocket::ReadFromV(UINT32 nVecLen, UINT32* puLenVec,
                IHXBuffer** ppBufVec, IHXSockAddr** ppAddr)
{
    return DoReadV(nVecLen, puLenVec, ppBufVec, ppAddr);
}


HX_RESULT 
CHXSymbianSocket::DoReadV(UINT32 nVecLen, 
            UINT32* puLenVec, 
        IHXBuffer** ppBufVec,
        IHXSockAddr** ppAddr)
{
    return HXR_NOTIMPL;
}


STDMETHODIMP
CHXSymbianSocket::WriteV(UINT32 nVecLen, IHXBuffer** ppBufVec)
{
    return DoWriteV(nVecLen, ppBufVec, NULL);
}

STDMETHODIMP
CHXSymbianSocket::WriteToV(UINT32 nVecLen, IHXBuffer** ppBufVec, IHXSockAddr* pAddr)
{
    return DoWriteV(nVecLen, ppBufVec, pAddr);
}

HX_RESULT 
CHXSymbianSocket::DoWriteV(UINT32 nVecLen, IHXBuffer** ppVec,
                        IHXSockAddr* pAddr)
{
    return HXR_NOTIMPL;
}


STDMETHODIMP
CHXSymbianSocket::JoinGroup(IHXSockAddr* pGroupAddr,
                     IHXSockAddr* /* pInterface */)
{
    return DoMulticastOperation(KSoIp6LeaveGroup, pGroupAddr);
}

STDMETHODIMP
CHXSymbianSocket::LeaveGroup(IHXSockAddr* pGroupAddr,
                     IHXSockAddr* /* pInterface */)
{
    return DoMulticastOperation(KSoIp6JoinGroup, pGroupAddr);
}

STDMETHODIMP
CHXSymbianSocket::SetSourceOption(HXMulticastSourceOption flag,
                IHXSockAddr* pSourceAddr,
                IHXSockAddr* pGroupAddr,
                IHXSockAddr* pInterface)
{
    HX_ASSERT(FALSE);
    return HXR_NOTIMPL;
}

void
CHXSymbianSocket::OnConnect(HX_RESULT hxr)
{
    if (m_state == EConnectionPending)
    {
        m_ulSocketTaskFlags  &= ~HX_SOCK_EVENT_CONNECT;
        if(hxr == HXR_OK)
        {
            m_bConnected = ETrue;
            SendEvents(HX_SOCK_EVENT_CONNECT|
                       HX_SOCK_EVENT_READ|
                       HX_SOCK_EVENT_WRITE, HXR_OK);
        }
        else
        {
            SendEvents(HX_SOCK_EVENT_CONNECT, hxr);
        }
    }
}

void
CHXSymbianSocket::OnRead(HX_RESULT hxr, UINT32 bytes)
{
    if(hxr == HXR_OK)
    {
        HX_ASSERT(!m_pDataBuf);
        TransferBuffer(m_pReadersBuf, m_pDataBuf, bytes);
    }
    if(HX_SOCK_EVENT_READ & m_ulEventMask)
    {
        m_pResponse->EventPending(HX_SOCK_EVENT_READ, hxr);
    }
}

void
CHXSymbianSocket::OnWrite(HX_RESULT hxr)
{

    if (hxr == HXR_OK)
    {
        if (m_writeList.GetCount() > 0)
        {
            // Write the next buffer in the list
            CSocketWriteReq* pWriteReq = (CSocketWriteReq*)m_writeList.RemoveHead();
            IHXBuffer *pBuffer=NULL;
            IHXSockAddr *pSockAddr=NULL;
            pWriteReq->GetRequest(pSockAddr, pBuffer);

            TInetAddr *dest=NULL;
            if(!m_bConnected)
            {
                if(m_pUdpConnectAddr)
                {
                    dest = m_pUdpConnectAddr;
                }
                else
                {
                    // pSockAddr is the destination address
                    if(pSockAddr)
                    {
                        GetNativeAddr(pSockAddr, m_writeTo);
                        dest = &m_writeTo;
                    }
                }
            }
            m_pWriter->Write(m_sock, pBuffer, dest);
            delete pWriteReq;
        }
        else
        {
            // no pending writes. send a write event.
            if(HX_SOCK_EVENT_WRITE & m_ulEventMask && m_pResponse)
            {
                m_pResponse->EventPending(HX_SOCK_EVENT_WRITE, hxr);
            }
        }
    }
    else
    {		
        HXLOGL1(HXLOG_NETW, "HXSymbianSocket::OnWrite Error=%d", hxr);
        // Send a Close Event.
        if ( (HX_SOCK_EVENT_CLOSE & m_ulEventMask) && m_pResponse)
        {
            m_pResponse->EventPending(HX_SOCK_EVENT_CLOSE, hxr);
        }
    }
}

void CHXSymbianSocket::static_APConnectDone(void* pObj, HX_RESULT status)
{
    HXLOGL4(HXLOG_NETW, "HXSymbianSocket::static_APConnectDone status=%d", status);
    CHXSymbianSocket* pSocket = (CHXSymbianSocket*)pObj;

    if (pSocket)
    {
        pSocket->OnAccessPointConnect(status);
    }
}

HX_RESULT
CHXSymbianSocket::OnAccessPointConnect(HX_RESULT status)
{
	
    AddRef();
    HX_RESULT res = HXR_OK;
    if (status == HXR_OK)
    {
		TUint sockType = CHXNetServices::GetNativeSockType(m_type);
		TUint proto;
		if(m_type == HX_SOCK_TYPE_TCP)
			proto = KProtocolInetTcp;
		else
			proto = KProtocolInetUdp;

        if(m_sock.Open( ((HXSymbianAccessPointManager*)m_pAPManager)->
                        GetRSocketServ(),
                        KAfInet, sockType, proto,
                        ((HXSymbianAccessPointManager*)m_pAPManager)->
                        GetRConnection())  != KErrNone )
        {
            status = HXR_SOCKET_CREATE;
        }
        else
        {
            m_bInitialized = TRUE;
        }
    }

    if (m_state == EConnectionPending)
    {
        if(status == HXR_OK)
            m_pConnector->Connect(m_sock, m_peerAddr);
        else
            OnConnect(status);
    }
    else if(m_state == EBindPending)
    {
        res = OnBind(status);
    }
    Release();
    return res;
}

HX_RESULT
CHXSymbianSocket::DoMulticastOperation(TUint operation, IHXSockAddr *pGroupAddr)
{
    HX_RESULT hxr = HXR_FAILED;
    if(pGroupAddr)
    {
        TInetAddr dstAddr;
        GetNativeAddr(pGroupAddr, dstAddr);
        TPckgBuf<TSoInetIfQuery> ifInfo;
        ifInfo().iDstAddr = dstAddr;
        ifInfo().iIsUp = 0;

        // get the interface info for the destination address
        m_sock.GetOpt(KSoInetIfQueryByDstAddr, KSolInetIfQuery, ifInfo);

        TPckgBuf<TIp6Mreq> mreq;

        // Multicast operation requires IPv6 addresses
        if(pGroupAddr->GetFamily() == HX_SOCK_FAMILY_IN4)
            dstAddr.ConvertToV4Mapped();

        mreq().iAddr        = dstAddr.Ip6Address();
        mreq().iInterface   = ifInfo().iZone[0];
        if(m_sock.SetOpt(operation, KSolInetIp, mreq) == KErrNone)
            hxr = HXR_OK;
    }
    return hxr;
}

// helper functions

void
CHXSymbianSocket::GetNativeAddr(IHXSockAddr* pAddr, TInetAddr &inetAddr)
{
    CHXInetSockAddr *inetSock = (CHXInetSockAddr *) pAddr;
    inetSock->GetNative(inetAddr);
}

void
CHXSymbianSocket::SendEvents(INT32 events, HX_RESULT hxr)
{
    if (m_pResponse)
    {
        if ( events & m_ulEventMask & HX_SOCK_EVENT_CLOSE )
        {
            m_pResponse->EventPending(HX_SOCK_EVENT_CLOSE, hxr);
            events &= ~HX_SOCK_EVENT_CLOSE;
        }
        if ( events & m_ulEventMask & HX_SOCK_EVENT_CONNECT)
        {
            m_pResponse->EventPending(HX_SOCK_EVENT_CONNECT, hxr);
            events &= ~HX_SOCK_EVENT_CONNECT;
        }

        for (UINT32 i = HX_SOCK_EVENT_LAST; i > 0; i >>= 1)
        {
            if (events & m_ulEventMask & i)
            {
                // Send the event
                m_pResponse->EventPending(i, hxr);
            }
        }
    }
}

void
CHXSymbianSocket::SetNativeAddr(IHXSockAddr* pAddr, TInetAddr &inetAddr)
{
    CHXInetSockAddr *inetSock = (CHXInetSockAddr *) pAddr;
    inetSock->SetNative(inetAddr);
}

// CreateReadBuffer and TransferBuffer 
// manage the read buffer. Allocation and deallocation
// is done here only.

HX_RESULT
CHXSymbianSocket::CreateReadBuffer(REF(IHXBuffer *) pBuf, UINT32 size)
{
    HX_RESULT hxr = HXR_OK;
    IHXTimeStampedBuffer* pTSBuf = NULL;
    switch (m_bufType)
    {
    case HX_SOCKBUF_DEFAULT:
        hxr = m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuf);
        break;
    case HX_SOCKBUF_TIMESTAMPED:
        hxr = m_pCCF->CreateInstance(CLSID_IHXTimeStampedBuffer, (void**)&pTSBuf);
        HX_ASSERT(SUCCEEDED(hxr));
        if (SUCCEEDED(hxr))
        {
            pTSBuf->SetTimeStamp(HX_GET_TICKCOUNT_IN_USEC());
            hxr = pTSBuf->QueryInterface(IID_IHXBuffer, (void**)&pBuf);
            HX_ASSERT(hxr == HXR_OK);
            pTSBuf->Release();
        }
        break;
    default:
        HX_ASSERT(FALSE);
        HX_RELEASE(pBuf);
        hxr = HXR_UNEXPECTED;
        break;
    }
    if(pBuf)
        hxr = pBuf->SetSize(size);
    if(hxr != HXR_OK)
    {
        HXLOGL1(HXLOG_NETW, "CHXSymbianSocket::CreateReadBuffer hxr=%d", hxr);
    }
    return hxr;
}

HX_RESULT
CHXSymbianSocket::TransferBuffer(REF(IHXBuffer *)pInBuf, 
                                 REF(IHXBuffer *)pOutBuf, 
                                 UINT32 bytes)
{
    HX_ASSERT(!pOutBuf);
    HX_RESULT hxr = HXR_OK;
    if (HX_SOCK_READBUF_SIZE_COPY == m_readBufAlloc)
    {
        // allocate a new buffer so buffer does not waste unused memory space
        hxr = CreateReadBuffer(pOutBuf, bytes);
        if (HXR_OK == hxr)
        {
           memcpy(pOutBuf->GetBuffer(), pInBuf->GetBuffer(), bytes);
           HX_RELEASE(pInBuf);
        }
    }
    else
    {
        HX_ASSERT(HX_SOCK_READBUF_SIZE_DEFAULT == m_readBufAlloc);
        // no copy. 
        pOutBuf = pInBuf;
        pOutBuf->AddRef();
        HX_RELEASE(pInBuf);
        pOutBuf->SetSize(bytes);
    }
    return hxr;
}

// symbian specific async connector, reader and writer classes


CHXSymbianSocketConnector::CHXSymbianSocketConnector(CHXSymbianSocket* pParent):
    CActive(EPriorityStandard),
    m_pParent(pParent)
{
    CActiveScheduler::Add(this);
}

CHXSymbianSocketConnector::~CHXSymbianSocketConnector()
{
    Cancel();
    // m_pParent weak ref.
}

void
CHXSymbianSocketConnector::Connect(RSocket& socket, TInetAddr &inetAddr)
{
    m_addr = inetAddr;
    socket.Connect(m_addr, iStatus);
    SetActive();
}

void
CHXSymbianSocketConnector::RunL()
{
    HXLOGL3(HXLOG_NETW, "CHXSymbianSocket::RunL iStatus=%d", iStatus.Int());
    m_pParent->OnConnect((iStatus.Int() == KErrNone) ?HXR_OK : HXR_NET_CONNECT);
}


void
CHXSymbianSocketConnector::DoCancel()
{
// do nothing
}



CHXSymbianSocketWriter::CHXSymbianSocketWriter(CHXSymbianSocket* pParent) :
    CActive(EPriorityStandard)
    //CActive(EPriorityHigh)
    ,m_pParent(pParent)
    ,m_pBuffer(0)
    ,m_bufDes(0,0)
{
    CActiveScheduler::Add(this);
}

CHXSymbianSocketWriter::~CHXSymbianSocketWriter()
{
    Cancel();
    HX_RELEASE(m_pBuffer);
    // m_pParent weak ref.
}

void
CHXSymbianSocketWriter::Write(RSocket& socket, IHXBuffer* pBuffer, 
                                TInetAddr *dest)
{
    HX_RELEASE(m_pBuffer);
    m_pBuffer = pBuffer;

    if (m_pBuffer)
    {
        m_pBuffer->AddRef();
        m_bufDes.Set(m_pBuffer->GetBuffer(), 
                    m_pBuffer->GetSize(), 
                    m_pBuffer->GetSize());

        if(!dest)
        {
            socket.Write(m_bufDes, iStatus);
        }
        else
        {
            m_sendAddr = *dest;
            socket.SendTo(m_bufDes, m_sendAddr, 0, iStatus);
        }
        SetActive();
    }
}


void
CHXSymbianSocketWriter::RunL()
{
    HX_RESULT res = HXR_FAILED;
    if (iStatus.Int() == KErrNone)
    {
        res = HXR_OK;
    }
    else if(iStatus.Int() == KErrEof)
    {
        res = HXR_STREAM_DONE;
    }
    else if(iStatus.Int() == KErrNoMemory)
    {
        res = HXR_OUTOFMEMORY;
    }
     else if(iStatus.Int() == KErrDisconnected)
    {
    	res = HXR_SOCK_DISCON;
    }
    else
    {
        HXLOGL1(HXLOG_NETW, "CHXSymbianSocketWriter::RunL iStatus=%d", 
            iStatus.Int());
    }

    HX_RELEASE(m_pBuffer);

    m_pParent->OnWrite(res);
}

void
CHXSymbianSocketWriter::DoCancel()
{
// do nothing
}



CHXSymbianSocketReader::CHXSymbianSocketReader(CHXSymbianSocket* pParent) :
    CActive(EPriorityStandard)
    ,m_pParent(pParent)
    ,m_bufDes(0, 0)
{
    CActiveScheduler::Add(this);
}

CHXSymbianSocketReader::~CHXSymbianSocketReader()
{
    Cancel();
    HX_RELEASE(m_pBuffer);
    // m_pParent weak ref.
}


HX_RESULT
CHXSymbianSocketReader::Read(RSocket& socket, IHXBuffer* pBuf, 
                                TInetAddr *fromAddr)
{
    if (m_pParent)
    {
        m_pBuffer = pBuf;
        HX_ADDREF(m_pBuffer);
        m_bufDes.Set(pBuf->GetBuffer(), 0, pBuf->GetSize());
        if(fromAddr)
        {
            // passing ref. Saves extra copy and possible extra return path.
            socket.RecvFrom(m_bufDes, *fromAddr, KSockReadContinuation, iStatus);
        }
        else
        {
            socket.RecvOneOrMore(m_bufDes, 0, iStatus, m_amountRead);
        }
        SetActive();
    }
    return HXR_OK;
}

void 
CHXSymbianSocketReader::RunL()
{
    HX_RESULT res = HXR_FAILED;

    HX_RELEASE(m_pBuffer);
    if (iStatus.Int() == KErrNone)
    {
        res = HXR_OK;
    }
    else if(iStatus.Int() == KErrEof)
    {
        res = HXR_STREAM_DONE;
    }
    else if(iStatus.Int() == KErrNoMemory)
    {
        res = HXR_OUTOFMEMORY;
    }
    else if(iStatus.Int() == KErrDisconnected)
    {
    	res = HXR_SOCK_DISCON;
    }
    else
    {
        HXLOGL1(HXLOG_NETW, "CHXSymbianSocketReader::RunL iStatus=%d", 
            iStatus.Int());
    }
    
    m_pParent->OnRead(res, m_bufDes.Size());

}

void 
CHXSymbianSocketReader::DoCancel()
{
// do nothing
}

// XXXAG CHXListeningSocket is not implemented 
// completely for Symbian.

CHXListeningSocket::CHXListeningSocket(IHXSocket* pSock) :
    m_nRefCount(0),
    m_pSock(pSock)
{
    HX_ADDREF(m_pSock);
}

CHXListeningSocket::~CHXListeningSocket(void)
{
    Close();
}

STDMETHODIMP
CHXListeningSocket::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSocket))
    {
        AddRef();
        *ppvObj = (IHXSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXListeningSocket))
    {
        AddRef();
        *ppvObj = (IHXListeningSocket*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXListeningSocket::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXListeningSocket::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP_(HXSockFamily)
CHXListeningSocket::GetFamily(void)
{
    return m_pSock->GetFamily();
}

STDMETHODIMP_(HXSockType)
CHXListeningSocket::GetType(void)
{
    return m_pSock->GetType();
}

STDMETHODIMP_(HXSockProtocol)
CHXListeningSocket::GetProtocol(void)
{
    return m_pSock->GetProtocol();
}

STDMETHODIMP
CHXListeningSocket::Init(HXSockFamily f, HXSockType t, HXSockProtocol p,
        IHXListeningSocketResponse* pResponse)
{
    return HXR_NOTIMPL;
}


STDMETHODIMP
CHXListeningSocket::Listen(IHXSockAddr* pAddr)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXListeningSocket::Close(void)
{
    if (m_pSock != NULL)
    {
        m_pSock->Close();
        HX_RELEASE(m_pSock);
    }
    return HXR_OK;
}

STDMETHODIMP
CHXListeningSocket::GetOption(HXSockOpt name, UINT32* pval)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXListeningSocket::SetOption(HXSockOpt name, UINT32 val)
{
    return HXR_NOTIMPL;
}


STDMETHODIMP
CHXListeningSocket::EventPending(UINT32 uEvent, HX_RESULT status)
{
    return HXR_NOTIMPL;
}

CSocketWriteReq::CSocketWriteReq()
{
// empty
}

CSocketWriteReq::~CSocketWriteReq()
{
    HX_RELEASE(m_pDest);
    HX_RELEASE(m_pBuffer);
}

CSocketWriteReq*
CSocketWriteReq::Construct(IHXSockAddr *pAddr, IHXBuffer *pBuf)
{
    CSocketWriteReq* self = new CSocketWriteReq();
    if(self)
        self->SetRequest(pAddr, pBuf);
    return self;
}

void
CSocketWriteReq::SetRequest(IHXSockAddr *pAddr, IHXBuffer *pBuf)
{
    m_pDest     = pAddr;
    m_pBuffer   = pBuf;
    if(m_pDest)
        HX_ADDREF(m_pDest);
    if(m_pBuffer)
        HX_ADDREF(m_pBuffer);
}

void 
CSocketWriteReq::GetRequest(IHXSockAddr *&pAddr, IHXBuffer *&pBuf)
{
    pAddr = m_pDest;
    pBuf  = m_pBuffer;
}

