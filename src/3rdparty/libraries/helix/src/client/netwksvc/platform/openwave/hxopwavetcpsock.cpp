/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxopwavetcpsock.cpp,v 1.6 2004/07/09 18:45:44 hubbe Exp $
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

#include "hxopwavetcpsock.h"
#include "debug.h"
#include "hxassert.h"
#include "ihxpckts.h"
#include "smartptr.h"

#define D_TCPSOCKET 0x10000000

class HXOpwaveTCPResolvResp : public IHXResolverResponse
{

public:

    HXOpwaveTCPResolvResp(HXOpwaveTCPSocket* pParent);
    ~HXOpwaveTCPResolvResp();

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
									REFIID riid,
									void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)		(THIS);
    STDMETHOD_(ULONG32,Release)		(THIS);

    /*
     *	IHXResolverResponse methods
     */
    STDMETHOD(GetHostByNameDone)	(THIS_
									HX_RESULT status,
									ULONG32 ulAddr);
private:

    ULONG32 m_lRefCount;
    HXOpwaveTCPSocket* m_pParent;

};



HXOpwaveTCPResolvResp::HXOpwaveTCPResolvResp(HXOpwaveTCPSocket* pParent) 
					  : m_lRefCount(0)
					  , m_pParent(pParent)
{
    HX_ADDREF(m_pParent);
}


HXOpwaveTCPResolvResp::~HXOpwaveTCPResolvResp()
{
    HX_RELEASE(m_pParent);
}

/*
 *  IUnknown methods
 */
STDMETHODIMP
HXOpwaveTCPResolvResp::QueryInterface(THIS_  REFIID riid,
											void** ppvObj)
{

    if (IsEqualIID(riid, IID_IHXResolverResponse))
    {
        AddRef();
        *ppvObj = (IHXResolverResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXResolverResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;

}

STDMETHODIMP_(ULONG32)
HXOpwaveTCPResolvResp::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}


STDMETHODIMP_(ULONG32)
HXOpwaveTCPResolvResp::Release(THIS)
{

    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;

    return 0;
}

/*
 *	IHXResolverResponse methods
 */
STDMETHODIMP
HXOpwaveTCPResolvResp::GetHostByNameDone(THIS_
			 		HX_RESULT status,
					ULONG32 ulAddr)
{
    return m_pParent->GetHostByNameDone(status, ulAddr);
}


HXOpwaveTCPSocket::HXOpwaveTCPSocket(IHXCommonClassFactory* pCCF,
			  	    IHXResolver* pResolver)
				  : OpSocket(kTCP)
				  , m_lRefCount(0)
				  , m_pResponse(0)
				  , m_pResolver(0)
				  , m_pCCF(0)
				  , m_state(tcpNotInitialized)
				  , m_nConnectPort(0)
				  , m_nLocalPort(0)
				  , m_ulLocalAddr(0)
				  , m_bWantWrite(FALSE)
				  , m_pReadBuffer(NULL)
				  , m_pWriteBuffer(NULL)
				  , m_ulBytesLeftToWrite(0)
				  , m_ulReadSize(0)
{

    IHXResolverResponse* pResolvResp = new HXOpwaveTCPResolvResp(this);
    if (pResolvResp)
    {
	pResolvResp->AddRef();
    
        if (pCCF && pResolver && (pResolver->Init(pResolvResp) == HXR_OK))
	{
	    m_pCCF = pCCF;
	    m_pCCF->AddRef();
	    m_pResolver = pResolver;
	    m_pResolver->AddRef();
	    m_state = tcpInitialized;
	}
    }

    HX_RELEASE(pResolvResp);

}


HXOpwaveTCPSocket::~HXOpwaveTCPSocket()
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::~HXOpwaveTCPSocket()\n"));

    if (m_state != tcpNotInitialized)
    {
	CloseConnection(HXR_OK);
    }

    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pResolver);
    HX_RELEASE(m_pCCF);


}



/*
 *  IUnknown methods
 */
STDMETHODIMP
HXOpwaveTCPSocket::QueryInterface(THIS_
								REFIID riid,
								void** ppvObj)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::QueryInterface()\n"));

    if (IsEqualIID(riid, IID_IHXTCPSocket))
    {

        AddRef();
        *ppvObj = (IHXTCPSocket*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSetSocketOption))
    {
        AddRef();
        *ppvObj = (IHXSetSocketOption*)this;
        return HXR_OK;
    }    
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXTCPSocket*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


STDMETHODIMP_(ULONG32)
HXOpwaveTCPSocket::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}



STDMETHODIMP_(ULONG32)
HXOpwaveTCPSocket::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *	IHXTCPSocket methods
 *
 *  Network addresses and ports are in native byte order
 *  
 */
STDMETHODIMP
HXOpwaveTCPSocket::Init(THIS_
				     IHXTCPResponse* /*IN*/ pTCPResponse)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::Init()\n"));

    HX_RELEASE(m_pResponse);
    m_pResponse = pTCPResponse;
    if (m_pResponse)
    {
	m_pResponse->AddRef();
    }

    return (m_pResponse) ? HXR_OK : HXR_FAILED;

}



STDMETHODIMP 
HXOpwaveTCPSocket::SetResponse(THIS_
			        IHXTCPResponse* pTCPResponse)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::SetResponse()\n"));

    HX_RELEASE(m_pResponse);
    m_pResponse = pTCPResponse;

    if (m_pResponse)
	m_pResponse->AddRef();

    return (m_pResponse) ? HXR_OK : HXR_FAILED;
}



STDMETHODIMP 
HXOpwaveTCPSocket::Bind(THIS_
			UINT32 ulLocalAddr,
	        	UINT16 nPort)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::Bind(%08lx, %u)\n", ulLocalAddr, nPort));

	/// Until OpSocket implement the DNS layer, this is nothting but cache address and
	/// port values
	/// OpSocket has bind function built into connect
    HX_RESULT res = HXR_OK;
    m_ulLocalAddr = ulLocalAddr;
    m_nLocalPort = nPort;
    m_state = tcpBound;
    return res;
}


/*
 * pDestination is a string containing host name or dotted-ip notation
 */
STDMETHODIMP 
HXOpwaveTCPSocket::Connect(THIS_ 
        		   const char* pDestination,
			   UINT16 nPort)    
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::Connect('%s', %u)\n", pDestination, nPort));

    HX_RESULT res = HXR_OK;

    if (m_state == tcpInitialized)
    {
         Bind(0, 0);
    }

    m_ipDest = 0;
    m_nConnectPort = nPort;
    m_state = tcpResolving;   
    res = m_pResolver->GetHostByName(pDestination);

    return res;
}



STDMETHODIMP 
HXOpwaveTCPSocket::Read(THIS_
						UINT16 Size)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::Read(%u)\n", Size));

    HX_RESULT res = HXR_OK;

    HX_ASSERT(m_state == tcpConnected);

    m_ulReadSize = Size;

    //OpDPRINTF("Read: this=%p, size=%d, bread=%d, state=%d, bwrite=%d\n", this, Size, m_bReadable, m_state, m_bWritable);
    if (m_state == tcpConnected)
    {
        if (m_bReadable &&m_ulReadSize)
        {
            /// avoid recursion that might occur because 
            /// OnReadDone call in DoRead invoke ::Read again
            m_bReadable = FALSE;
            res = DoRead();
        }
        
    }
    else
    {
    	res = HXR_FAILED;
    }
    return res;

}

/// Work horse for reading. As long as we are initilized once 
/// to read, we always try to read something of the socket.
HX_RESULT
HXOpwaveTCPSocket::DoRead()
{
    /// Create a new IHXBuffer for every read
    HX_RESULT res = HXR_FAIL;
    IHXBuffer* pReadBuffer = NULL;
    res = m_pCCF->CreateInstance(IID_IHXBuffer, (void**)&pReadBuffer);
    if (SUCCEEDED(res))
    {
        res = pReadBuffer->SetSize(m_ulReadSize);
    
        UCHAR* pData = pReadBuffer->GetBuffer();
        int nRead = read(pData, m_ulReadSize);
 
        //OpDPRINTF("DoRead: this=%p, read=%d, askfor=%d\n\n", this, nRead, m_ulReadSize);
        if (nRead > 0)
        {
            pReadBuffer->SetSize(nRead);
            m_pReadBuffer = pReadBuffer;
            // Don't ADDREF, m_pReadBuffer will be released in the callback OnReadDone
        
            OnReadDone(res, m_pReadBuffer);
        }
    }
    HX_RELEASE(m_pReadBuffer);
    return res;
}

STDMETHODIMP 
HXOpwaveTCPSocket::Write(THIS_
			 IHXBuffer*	pBuffer)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::Write()\n"));

    HX_RESULT res = HXR_OK;
    if (!pBuffer)
    {
    	res = HXR_INVALID_PARAMETER;
    }
    //OpDPRINTF("Write: this=%p, lwsize=%d, bread=%d, newwritesize=%d, bwrite=%d\n", this, m_ulBytesLeftToWrite, m_bReadable, pBuffer->GetSize(), m_bWritable);
    UCHAR* pBufData = pBuffer->GetBuffer(); 
    if (SUCCEEDED(res) && (m_state == tcpConnected))
    {
        /// First add to our list
        pBuffer->AddRef();
        m_writeList.AddTail(pBuffer);

        /// decide if we need to do a write 
        /// because last time when it is ready to write, there is no data 
        /// to be written.
        if (m_bWritable)
        {
            if (!m_pWriteBuffer)
            {
                m_pWriteBuffer = (IHXBuffer*)m_writeList.RemoveHead();
                m_ulBytesLeftToWrite = m_pWriteBuffer->GetSize();  
            }
            res =DoWrite();
            if (SUCCEEDED(res))
            {
                m_bWritable = FALSE;
            }
        }
    }
    return res;
}

/// This is working method to do actual write from
/// the m_pWriteBuffer which is guranteed to have data to be written
/// and the socket is ready to accept data
HX_RESULT HXOpwaveTCPSocket::DoWrite()
{
    HX_RESULT res = HXR_OK;
 
    UCHAR* pBufData = m_pWriteBuffer->GetBuffer(); 
    size_t ulActualWritten = write(pBufData, m_ulBytesLeftToWrite);
   
    HX_ASSERT(m_ulBytesLeftToWrite >= ulActualWritten);
 
    //OpDPRINTF("DoWrite, this=%p, written%d, write=%d\n\n",this, ulActualWritten, m_ulBytesLeftToWrite);
    m_ulBytesLeftToWrite -= ulActualWritten;
    if (m_ulBytesLeftToWrite > 0)
    {
        // more left in this m_pWriteBuffer to be written out
        UCHAR* pLeftData = new UCHAR[m_ulBytesLeftToWrite];
        if (!pLeftData)
        {
            res = HXR_OUTOFMEMORY;
            OnWriteDone(res);
            return res;
        }
            
        memcpy(pLeftData, pBufData+ulActualWritten, m_ulBytesLeftToWrite);
        m_pWriteBuffer->Set(pLeftData, m_ulBytesLeftToWrite); 
        delete pLeftData;
    }
    return res;
}

/************************************************************************
 *	Method:
 *	    IHXTCPSocket::WantWrite
 *	Purpose:
 *	    This method is called when you wish to write a large amount of
 *	    data.  If you are only writing small amounts of data, you can
 *	    just call Write (all data not ready to be transmitted will be
 *	    buffered on your behalf).  When the TCP channel is ready to be
 *	    written to, the response interfaces WriteReady method will be 
 *	    called.
 */
STDMETHODIMP
HXOpwaveTCPSocket::WantWrite(THIS)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::WantWrite()\n"));

    m_bWantWrite = TRUE;
    return HXR_OK;
}



/************************************************************************
 *	Method:
 *	    IHXTCPSocket::GetForeignAddress
 *	Purpose:
 *	    Returns the address of the other end of the TCP socket as a
 *	    ULONG32 in local host order
 */
STDMETHODIMP 
HXOpwaveTCPSocket::GetForeignAddress(THIS_ REF(ULONG32) lAddress)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::GetForeignAddress()\n"));
    
    /// OpSocket's DNS should make this better
    HX_RESULT res = HXR_OK;
    if (m_state == tcpConnected)
    {
        lAddress = m_ipDest;
    }
    return res;
}


STDMETHODIMP 
HXOpwaveTCPSocket::GetLocalAddress(THIS_ REF(ULONG32) lAddress)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::GetLocalAddress()\n"));

    HX_RESULT res = HXR_OK;
    lAddress = m_ulLocalAddr;
    return res;
}



/************************************************************************
 *	Method:
 *	    IHXTCPSocket::GetForeignPort
 *	Purpose:
 *	    Returns the port of the other end of the TCP socket in local
 *      host order.
 */
STDMETHODIMP
HXOpwaveTCPSocket::GetForeignPort(THIS_ REF(UINT16) port)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::GetForeignPort()\n"));

	/// Until OpSocket has DNS, this is not appropriately implemented
    HX_RESULT res = HXR_OK;
    if (m_state == tcpConnected)
    {
	port = m_nConnectPort;
    }
    else 
    {
	res = HXR_FAILED;
    }

    return res;
}



STDMETHODIMP
HXOpwaveTCPSocket::GetLocalPort(THIS_
								REF(UINT16) port)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::GetLocalPort()\n"));

    HX_RESULT res = HXR_OK;

    if ((m_state != tcpNotInitialized) &&
	(m_state != tcpInitialized))
    {
	port = m_nLocalPort;
    }

    return res;
}



STDMETHODIMP 
HXOpwaveTCPSocket::SetOption(THIS_ 
        		    HX_SOCKET_OPTION option,
			    UINT32 ulValue)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::SetOption(%d, %lu)\n", option, ulValue));
    HX_RESULT res = HXR_FAILED;

    if (m_state == tcpNotInitialized)
    {
        switch(option)
        {
        case  HX_SOCKOPT_REUSE_ADDR:
        case HX_SOCKOPT_REUSE_PORT:
            //// OpSocket doesn't support set socket options
            {
                res = HXR_NOTIMPL;
            }
            break;
        case HX_SOCKOPT_BROADCAST:
        case HX_SOCKOPT_SET_RECVBUF_SIZE:
        case HX_SOCKOPT_SET_SENDBUF_SIZE:
        case HX_SOCKOPT_MULTICAST_IF:
            res = HXR_UNEXPECTED;
            break;
        default:
            break;
        }
    }

    return res;
}


HX_RESULT
HXOpwaveTCPSocket::GetHostByNameDone(HX_RESULT status, 
								ULONG32 ulAddr)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::ResolveDone(%ld, %08lx)\n", status, ulAddr));

    HX_ASSERT( m_state == tcpResolving);
 
    if (SUCCEEDED(status))
    {
        m_ipDest = ulAddr;
        m_state = tcpConnecting;
   
        connect(m_nLocalPort, m_nConnectPort, m_ipDest);

        /*
        char* stopstring = NULL;
        UINT32 segVal = 0;
             
        segVal = strtoul(pDestination, &stopstring, 10);
        m_ipDest = segVal << 24;
        pDestination = stopstring + 1;
        segVal = strtoul(pDestination, &stopstring, 10);
        m_ipDest += (segVal << 16);
        pDestination = stopstring + 1;
        segVal = strtoul(pDestination, &stopstring, 10);
        m_ipDest += (segVal << 8);
        pDestination = stopstring + 1;
        segVal = strtoul(pDestination, &stopstring, 10);
        m_ipDest += segVal;
        connect(m_nLocalPort, nPort, m_ipDest);
        //OpDPRINTF("Connect: this=%p, localPort=%d,remotePort=%d,desIP=%d\n", this, m_nLocalPort, nPort, m_ipDest);
        */
        m_bWritable = FALSE;
        m_bReadable = FALSE;
 
    }
    
    return status;

}



void HXOpwaveTCPSocket::OnConnect(HX_RESULT status)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::OnConnect(%ld)\n",status));
    
    HX_ASSERT(m_state == tcpConnecting);
    DECLARE_SMART_POINTER_UNKNOWN scopeRef((IHXTCPSocket*)this);
    
    if (status == HXR_OK)
    {
        m_state = tcpConnected;
    }
    else
    {
        m_state = tcpBound;
    }
    if (m_pResponse)
    {
        m_pResponse->ConnectDone(status);
    }
}

void HXOpwaveTCPSocket::OnWriteDone(HX_RESULT status)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::OnWriteDone(%ld)\n", status));
    DECLARE_SMART_POINTER_UNKNOWN scopeRef((IHXTCPSocket*)this);
    
    if (status == HXR_OK)
    {
        HX_RELEASE(m_pWriteBuffer);
        if (m_writeList.GetCount() > 0)
        {
            m_pWriteBuffer = (IHXBuffer*)m_writeList.RemoveHead();
            m_ulBytesLeftToWrite = m_pWriteBuffer->GetSize();     
        }
      
        // Signal WriteReady() if we don't have any
        // writes pending and the response object
        // wants these calls
        if (m_bWantWrite && m_pResponse)
        {
            m_pResponse->WriteReady(HXR_OK);
        }
    }
    else
    {
        CloseConnection(status);
    }
}


void HXOpwaveTCPSocket::OnReadDone(HX_RESULT status, IHXBuffer* pBuffer)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::OnReadDone(%ld)\n", status));

    DECLARE_SMART_POINTER_UNKNOWN scopeRef((IHXTCPSocket*)this);

    if (m_pResponse)
    {
	m_pResponse->ReadDone(status, pBuffer);
    }

    if (status != HXR_OK)
    {
	CloseConnection(status);
    }
}


void HXOpwaveTCPSocket::CloseConnection(HX_RESULT status)
{

    DPRINTF(D_TCPSOCKET, ("HXOpwaveTCPSocket::CloseConnection(%ld)\n", status));

    if (m_state != tcpNotInitialized)
    {
	close(true);
    }	

    // Clear the writer list

    while (m_writeList.GetCount() > 0)
    {
        // Release all the left unsent buffers in the list
        IHXBuffer* pBuffer = (IHXBuffer*)m_writeList.RemoveHead();
        HX_RELEASE(pBuffer);
    }

    if ((m_state != tcpInitialized) && m_pResponse)
    {
        m_pResponse->Closed(HXR_OK);
    }
    m_state = tcpNotInitialized;

}

void HXOpwaveTCPSocket::onReadable(OpSocketEvent *pSocketEvent)
{

    HX_RESULT res = HXR_OK;
    m_bReadable = TRUE;
    //OpDPRINTF("onReadable: this=%p, mrsize=%d, bread=%d, state=%d, bwrite=%d\n", this, m_ulReadSize, m_bReadable, m_state, m_bWritable);
  
    if (m_ulReadSize > 0)
    {
        /// avoid recursion that might occur because 
        /// OnReadDone call in DoRead invoke ::Read again
        m_bReadable = FALSE;
        DoRead();
    }
}

void HXOpwaveTCPSocket::onWritable(OpSocketEvent *pSocketEvent)
{
    HX_RESULT res = HXR_OK;

    m_bWritable = TRUE;
    //OpDPRINTF("onWritable: this=%p, mrsize=%d, bread=%d, lwsize=%d, bwrite=%d\n", this, m_ulReadSize, m_bReadable, m_ulBytesLeftToWrite, m_bWritable);
  
    if (m_state == tcpConnecting)
    {
        /// Since Openwave OpSocket's api is designed to have kWritable as the first
        /// event sending back to clients, so we first respond to core for connection
        /// status
        OpSocketEvent::Condition sockCond = pSocketEvent->getCondition();
        HX_RESULT status =  sockCond == OpSocketEvent::kException ? HXR_NET_CONNECT : HXR_OK;
        OnConnect(status);
    }

  
    /// Normal write process
    /// Call OnWriteDone for handling the notification of last write
    if (m_ulBytesLeftToWrite == 0)
    {
        /// Notify last writing is completely done
        OnWriteDone(res);
    }
    
    /// Do the next writing
    if (m_pWriteBuffer && m_ulBytesLeftToWrite > 0)
    {
        if (DoWrite() == HXR_OK)
        {
            m_bWritable = FALSE;
        }
    }
  
}


void HXOpwaveTCPSocket::onException(OpSocketEvent *pSocketEvent)
{
    OpSocketEvent::Exception except = pSocketEvent->getException();

}

   
#if 0
// Don't need it for the time being because
// all the cases are handled by the above three callbacks.
bool HXOpwaveTCPSocket::onEvent(OpEvent& ev)
{
   
    OpSocketEvent* pSockEvt = NULL;
    if ( (pSockEvt = OpSocketEvent::Cast(&ev)) != NULL)
    {
        HX_RESULT status = pSockEvt->getCondition() == OpSocketEvent::kException ? HXR_NET_CONNECT : HXR_OK;
        OnConnect(status);
        return true;
    }
    return OpEventSink::onEvent(ev);
}
#endif

