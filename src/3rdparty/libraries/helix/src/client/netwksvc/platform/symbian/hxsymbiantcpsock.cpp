/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsymbiantcpsock.cpp,v 1.11 2007/07/06 21:58:23 jfinnecy Exp $
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

#include "platform/symbian/hxsymbiantcpsock.h"
#include "hxsymbiansockhlp.h"
#include "debug.h"
#include "hxassert.h"
#include "ihxpckts.h"
#include "smartptr.h"
#include "hxtick.h"
#include "hxtbuf.h"

#include <in_sock.h>

#define D_TCPSOCKET 0x10000000

class HXSymbianTCPResolvResp : public IHXResolverResponse
{
public:
    HXSymbianTCPResolvResp(HXSymbianTCPSocket* pParent);
    ~HXSymbianTCPResolvResp();

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
    HXSymbianTCPSocket* m_pParent;
};

HXSymbianTCPResolvResp::HXSymbianTCPResolvResp(HXSymbianTCPSocket* pParent) :
    m_lRefCount(0),
    m_pParent(pParent)
{}

HXSymbianTCPResolvResp::~HXSymbianTCPResolvResp()
{}

    /*
     *  IUnknown methods
     */
STDMETHODIMP HXSymbianTCPResolvResp::QueryInterface(THIS_
						    REFIID riid,
						    void** ppvObj)
{
    QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXResolverResponse*)this },
		{ GET_IIDHANDLE(IID_IHXResolverResponse), (IHXResolverResponse*) this },
	};
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXSymbianTCPResolvResp::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXSymbianTCPResolvResp::Release(THIS)
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

STDMETHODIMP HXSymbianTCPResolvResp::GetHostByNameDone(THIS_
						       HX_RESULT status,
						       ULONG32 ulAddr)
{
    return m_pParent->GetHostByNameDone(status, ulAddr);
}

class HXSymbianTCPConnector : public CActive
{
public:
    HXSymbianTCPConnector(HXSymbianTCPSocket* pParent);
    ~HXSymbianTCPConnector();

    void Connect(RSocket& socket, ULONG32 ulAddr, UINT16 nPort);

private:
    void RunL();
    void DoCancel();

    HXSymbianTCPSocket* m_pParent;
    TInetAddr m_addr;
};

HXSymbianTCPConnector::HXSymbianTCPConnector(HXSymbianTCPSocket* pParent) :
    CActive(EPriorityStandard),
    m_pParent(pParent)
{
    CActiveScheduler::Add(this);
}

HXSymbianTCPConnector::~HXSymbianTCPConnector()
{
    if (IsActive())
	Cancel();

    m_pParent = 0;
}

void HXSymbianTCPConnector::Connect(RSocket& socket,
				    ULONG32 ulAddr, UINT16 nPort)
{
    m_addr.SetAddress(ulAddr);
    m_addr.SetPort(nPort);

    iStatus = KRequestPending;
    socket.Connect(m_addr, iStatus);
    SetActive();
}

void HXSymbianTCPConnector::RunL()
{
    m_pParent->OnConnect((iStatus == KErrNone) ? HXR_OK : HXR_NET_CONNECT);
}

void HXSymbianTCPConnector::DoCancel()
{}

class HXSymbianTCPWriter : public CActive
{
public:
    HXSymbianTCPWriter(HXSymbianTCPSocket* pParent);
    ~HXSymbianTCPWriter();

    void Write(RSocket& socket, IHXBuffer* pBuffer);

private:
    void RunL();
    void DoCancel();

    HXSymbianTCPSocket* m_pParent;
    IHXBuffer* m_pBuffer;
    TPtrC8 m_bufDes;
};

HXSymbianTCPWriter::HXSymbianTCPWriter(HXSymbianTCPSocket* pParent) :
    CActive(EPriorityStandard),
    m_pParent(pParent),
    m_pBuffer(0)
{
    CActiveScheduler::Add(this);
}

HXSymbianTCPWriter::~HXSymbianTCPWriter()
{
    if (IsActive())
	Cancel();

    HX_RELEASE(m_pBuffer);
    m_pParent = 0;
}

void HXSymbianTCPWriter::Write(RSocket& socket, IHXBuffer* pBuffer)
{
    HX_RELEASE(m_pBuffer);
    m_pBuffer = pBuffer;

    if (m_pBuffer)
    {
	m_pBuffer->AddRef();

	m_bufDes.Set(m_pBuffer->GetBuffer(), m_pBuffer->GetSize());

	iStatus = KRequestPending;
	socket.Write(m_bufDes, iStatus);
	SetActive();
    }
}

void HXSymbianTCPWriter::RunL()
{
    HX_RESULT res = HXR_FAILED;

    if (iStatus == KErrNone)
    {
	res = HXR_OK;
    }
    else if(iStatus == KErrEof)
    {
	res = HXR_STREAM_DONE;
    }
    else if(iStatus == KErrNoMemory)
    {
	res = HXR_OUTOFMEMORY;
    }

    HX_RELEASE(m_pBuffer);

    m_pParent->OnWriteDone(res);
}

void HXSymbianTCPWriter::DoCancel()
{}

class HXSymbianTCPReader : public CActive
{
public:
    HXSymbianTCPReader(HXSymbianTCPSocket* pParent,
                       IHXCommonClassFactory* pCCF);
    ~HXSymbianTCPReader();

    HX_RESULT Read(RSocket& socket, UINT16 uSize);

private:
    void RunL();
    void DoCancel();

    HXSymbianTCPSocket* m_pParent;
    IHXBuffer* m_pBuffer;
    TPtr8 m_bufDes;
    TSockXfrLength m_amountRead;
    IHXCommonClassFactory* m_pCCF;
};

HXSymbianTCPReader::HXSymbianTCPReader(HXSymbianTCPSocket* pParent,
                                       IHXCommonClassFactory* pCCF) :
    CActive(EPriorityStandard),
    m_pParent(pParent),
    m_pBuffer(0),
    m_bufDes(0, 0),
    m_pCCF(pCCF)
{
    CActiveScheduler::Add(this);

    if (m_pCCF)
    {
        m_pCCF->AddRef();
    }
}

HXSymbianTCPReader::~HXSymbianTCPReader()
{
    if (IsActive())
	Cancel();

    HX_RELEASE(m_pBuffer);
    HX_RELEASE(m_pCCF);

    m_pParent = 0;
}

HX_RESULT HXSymbianTCPReader::Read(RSocket& socket, UINT16 uSize)
{
    HX_RESULT res = HXR_FAILED;

    if (m_pParent)
    {
        res = HXSymbianSocketHelper::ResizeOrCreate(m_pCCF, uSize, m_pBuffer);
        
        if (HXR_OK == res)
        {
            IHXTimeStampedBuffer* pTSBuffer = 0;
	      if(HXR_OK == m_pBuffer->QueryInterface(IID_IHXTimeStampedBuffer, (void **)&pTSBuffer))
	      {
	      		pTSBuffer->SetTimeStamp(HX_GET_TICKCOUNT());
			HX_RELEASE(pTSBuffer);
		}		 
            m_bufDes.Set(m_pBuffer->GetBuffer(), 0, m_pBuffer->GetSize());

            iStatus = KRequestPending;
            socket.RecvOneOrMore(m_bufDes, 0, iStatus, m_amountRead);
            SetActive();
        }
    }

    return res;
}

void HXSymbianTCPReader::RunL()
{
    HX_RESULT res = HXR_FAILED;

    if (iStatus == KErrNone)
    {
	res = HXR_OK;
    }
    else if(iStatus == KErrEof)
    {
	res = HXR_STREAM_DONE;
    }
    else if(iStatus == KErrNoMemory)
    {
	res = HXR_OUTOFMEMORY;
    }

    IHXBuffer* pBuffer = 0;
    if(res == KErrNone)
    {
        HXSymbianSocketHelper::CopyOrTransfer(m_pCCF, m_amountRead(), 
                                              m_pBuffer, pBuffer);
    }

    m_pParent->OnReadDone(res, pBuffer);

    HX_RELEASE(pBuffer);
}

void HXSymbianTCPReader::DoCancel()
{}

HXSymbianTCPSocket::HXSymbianTCPSocket(IHXCommonClassFactory* pCCF,
				       IHXResolver* pResolver) :
    m_lRefCount(0),
    m_pResponse(0),
    m_pResolver(0),
    m_state(tcpNotInitialized),
    m_pConnector(0),
    m_nConnectPort(0),
    m_pWriter(0),
    m_bWantWrite(FALSE),
    m_pReader(0)
{
    IHXResolverResponse* pResolvResp = new HXSymbianTCPResolvResp(this);

    m_pConnector = new HXSymbianTCPConnector(this);
    m_pWriter = new HXSymbianTCPWriter(this);
    m_pReader = new HXSymbianTCPReader(this, pCCF);

    if (pResolvResp)
    {
	pResolvResp->AddRef();
    }

    if (pCCF && pResolver && m_pConnector && m_pWriter && m_pReader &&
	(pResolvResp) &&
	(pResolver->Init(pResolvResp) == HXR_OK) &&
	(m_socketServ.Connect() == KErrNone) &&
	(m_socket.Open(m_socketServ, KAfInet,
		       KSockStream, KProtocolInetTcp) == KErrNone))
    {
	m_pResolver = pResolver;
	m_pResolver->AddRef();

	m_state = tcpInitialized;
    }

    HX_RELEASE(pResolvResp);
}

HXSymbianTCPSocket::~HXSymbianTCPSocket()
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::~HXSymbianTCPSocket()\n"));

    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pResolver);

    if (m_state != tcpNotInitialized)
    {
	CloseConnection(HXR_OK);

	m_socket.Close();
	m_socketServ.Close();
    }

    HX_DELETE(m_pConnector);
    HX_DELETE(m_pWriter);
    HX_DELETE(m_pReader);
}

    /*
     *  IUnknown methods
     */
STDMETHODIMP HXSymbianTCPSocket::QueryInterface(THIS_
					REFIID riid,
					void** ppvObj)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::QueryInterface()\n"));
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXTCPSocket*)this },
		{ GET_IIDHANDLE(IID_IHXTCPSocket), (IHXTCPSocket*) this },
		{ GET_IIDHANDLE(IID_IHXSetSocketOption), (IHXSetSocketOption*) this },
	};
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXSymbianTCPSocket::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)HXSymbianTCPSocket::Release(THIS)
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

STDMETHODIMP HXSymbianTCPSocket::Init(THIS_
				      IHXTCPResponse* /*IN*/ pTCPResponse)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::Init()\n"));

    HX_RELEASE(m_pResponse);

    m_pResponse = pTCPResponse;

    if (m_pResponse)
	m_pResponse->AddRef();

    return (m_pResponse) ? HXR_OK : HXR_FAILED;
}

STDMETHODIMP HXSymbianTCPSocket::SetResponse(THIS_
					     IHXTCPResponse* pTCPResponse)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::SetResponse()\n"));

    HX_RELEASE(m_pResponse);

    m_pResponse = pTCPResponse;

    if (m_pResponse)
	m_pResponse->AddRef();

    return (m_pResponse) ? HXR_OK : HXR_FAILED;
}

STDMETHODIMP HXSymbianTCPSocket::Bind(THIS_
				      UINT32 ulLocalAddr,
				      UINT16 nPort)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::Bind(%08lx, %u)\n",
			  ulLocalAddr, nPort));

    HX_RESULT res = HXR_FAILED;
    TInetAddr addr(ulLocalAddr, nPort);

    if ((m_state == tcpInitialized) &&
	(m_socket.Bind(addr) == KErrNone))
    {
	m_state = tcpBound;
	res = HXR_OK;
    }

    return res;
}

    /*
     * pDestination is a string containing host name or dotted-ip notation
     */
STDMETHODIMP HXSymbianTCPSocket::Connect(THIS_
					 const char* pDestination,
					 UINT16 nPort)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::Connect('%s', %u)\n",
			  pDestination, nPort));

    HX_RESULT res = HXR_FAILED;

    if (m_state == tcpInitialized)
	Bind(0, 0);

    if (m_state == tcpBound)
    {
	m_state = tcpResolving;
	m_nConnectPort = nPort;

	res = m_pResolver->GetHostByName(pDestination);

	if (HXR_OK != res)
	{
	    m_state = tcpBound;
	}
    }

    return res;
}

STDMETHODIMP HXSymbianTCPSocket::Read(THIS_ UINT16 Size)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::Read(%u)\n", Size));

    HX_RESULT res = HXR_FAILED;

    HX_ASSERT(m_state == tcpConnected);

    if ((m_state == tcpConnected) && (!m_pReader->IsActive()))
    {
	res = DoRead(Size);
    }

    return res;
}

STDMETHODIMP HXSymbianTCPSocket::Write(THIS_ IHXBuffer*	pBuffer)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::Write()\n"));

    HX_RESULT res = HXR_FAILED;

    HX_ASSERT(m_state == tcpConnected);

    if ((m_state == tcpConnected) && pBuffer)
    {
	if (m_pWriter->IsActive())
	{
	    // A write is in progress. Add the buffer
	    // to the write list
	    pBuffer->AddRef();
	    LISTPOSITION listRet = m_writeList.AddTail(pBuffer);
            if( listRet == NULL )
            {
                res = HXR_OUTOFMEMORY;
                HX_RELEASE(pBuffer);
            }
	}
	else
	{
	    // Do the write now
	    m_pWriter->Write(m_socket, pBuffer);
	}

	res = HXR_OK;
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
STDMETHODIMP HXSymbianTCPSocket::WantWrite(THIS)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::WantWrite()\n"));

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
STDMETHODIMP HXSymbianTCPSocket::GetForeignAddress(THIS_ REF(ULONG32) lAddress)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::GetForeignAddress()\n"));

    HX_RESULT res = HXR_FAILED;

    if (m_state == tcpConnected)
    {
	TInetAddr addr;
	m_socket.RemoteName(addr);

	lAddress = (ULONG32)addr.Address();

	res = HXR_OK;
    }

    return res;
}

STDMETHODIMP HXSymbianTCPSocket::GetLocalAddress(THIS_ REF(ULONG32) lAddress)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::GetLocalAddress()\n"));

    HX_RESULT res = HXR_FAILED;

    if ((m_state != tcpNotInitialized) &&
	(m_state != tcpInitialized))
    {
	TInetAddr addr;
	m_socket.LocalName(addr);

	lAddress = (ULONG32)addr.Address();

	res = HXR_OK;
    }

    return res;
}

    /************************************************************************
     *	Method:
     *	    IHXTCPSocket::GetForeignPort
     *	Purpose:
     *	    Returns the port of the other end of the TCP socket in local
     *      host order.
     */
STDMETHODIMP HXSymbianTCPSocket::GetForeignPort(THIS_ REF(UINT16) port)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::GetForeignPort()\n"));

    HX_RESULT res = HXR_FAILED;

    if (m_state == tcpConnected)
    {
	TInetAddr addr;
	m_socket.RemoteName(addr);

	port = addr.Port();

	res = HXR_OK;
    }

    return res;
}

STDMETHODIMP HXSymbianTCPSocket::GetLocalPort(THIS_ REF(UINT16) port)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::GetLocalPort()\n"));

    HX_RESULT res = HXR_FAILED;

    if ((m_state != tcpNotInitialized) &&
	(m_state != tcpInitialized))
    {
	port = m_socket.LocalPort();

	res = HXR_OK;
    }

    return res;
}

STDMETHODIMP HXSymbianTCPSocket::SetOption(THIS_
					   HX_SOCKET_OPTION option,
					   UINT32 ulValue)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::SetOption(%d, %lu)\n",
			  option, ulValue));
    HX_RESULT res = HXR_FAILED;

    if (m_state == tcpNotInitialized)
    {
	switch(option)
	{
	case  HX_SOCKOPT_REUSE_ADDR:
	case HX_SOCKOPT_REUSE_PORT:
	    if (m_socket.SetOpt(KSoReuseAddr, KSOLSocket, ulValue) == KErrNone)
	    {
		res = HXR_OK;
	    }
	    break;
	case HX_SOCKOPT_BROADCAST:
	case HX_SOCKOPT_SET_RECVBUF_SIZE:
	case HX_SOCKOPT_SET_SENDBUF_SIZE:
	case HX_SOCKOPT_MULTICAST_IF:
	    res = HXR_UNEXPECTED;
	    break;
	default:
	    res = HXR_FAILED;
	    break;
	};
    }

    return res;
}

HX_RESULT HXSymbianTCPSocket::GetHostByNameDone(HX_RESULT status,
						ULONG32 ulAddr)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::ResolveDone(%ld, %08lx)\n",
			  status, ulAddr));

    HX_ASSERT(m_state == tcpResolving);

    DECLARE_SMART_POINTER_UNKNOWN scopeRef((IHXTCPSocket*)this);

    if (m_state == tcpResolving)
    {
	if (status == HXR_OK)
	{
	    m_pConnector->Connect(m_socket, ulAddr, m_nConnectPort);

	    m_state = tcpConnecting;
	}
	else if (m_pResponse)
	{
	    m_pResponse->ConnectDone(status);
	    m_state = tcpBound;
	}
    }

    return HXR_OK;
}

void HXSymbianTCPSocket::OnConnect(HX_RESULT status)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::OnConnect(%ld)\n",
			  status));

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

void HXSymbianTCPSocket::OnWriteDone(HX_RESULT status)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::OnWriteDone(%ld)\n", status));

    DECLARE_SMART_POINTER_UNKNOWN scopeRef((IHXTCPSocket*)this);

    if (status == HXR_OK)
    {
	if (m_writeList.GetCount() > 0)
	{
	    // Write the next buffer in the list
	    IHXBuffer* pBuffer = (IHXBuffer*)m_writeList.RemoveHead();

	    m_pWriter->Write(m_socket, pBuffer);

	    HX_RELEASE(pBuffer);
	}

	// Signal WriteReady() if we don't have any
	// writes pending and the response object
	// wants these calls
	if (!m_pWriter->IsActive() && m_bWantWrite && m_pResponse)
	{
	    m_pResponse->WriteReady(HXR_OK);
	}
    }
    else
    {
	CloseConnection(status);
    }
}

void HXSymbianTCPSocket::OnReadDone(HX_RESULT status, IHXBuffer* pBuffer)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::OnReadDone(%ld)\n", status));

    DECLARE_SMART_POINTER_UNKNOWN scopeRef((IHXTCPSocket*)this);

    if (m_pResponse)
    {
        // We assume error reporting/handling is done in the response object
        // via the value of status passed here.
	m_pResponse->ReadDone(status, pBuffer);
    }

    if (status != HXR_OK)
    {
	CloseConnection(status);
    }
}

HX_RESULT HXSymbianTCPSocket::DoRead(UINT16 size)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::DoRead(%u)\n", size));

    HX_RESULT res = HXR_FAILED;

    if (m_pReader)
    {
        res = m_pReader->Read(m_socket, size);
    }

    return res;
}

void HXSymbianTCPSocket::CloseConnection(HX_RESULT status)
{
    DPRINTF(D_TCPSOCKET, ("HXSymbianTCPSocket::CloseConnection(%ld)\n",
			  status));

    if (m_state != tcpNotInitialized)
    {
	// Cancel any connector
	if (m_pConnector->IsActive())
	{
	    m_socket.CancelConnect();
	    m_pConnector->Cancel();
	}

	// Cancel writer
	if (m_pWriter->IsActive())
	{
	    m_socket.CancelWrite();
	    m_pWriter->Cancel();
	}

	if (m_pReader->IsActive())
	{
	    m_socket.CancelRecv();
	    m_pReader->Cancel();
	}

	// Clear the writer list
	while (m_writeList.GetCount() > 0)
	{
	    // Write the next buffer in the list
	    IHXBuffer* pBuffer = (IHXBuffer*)m_writeList.RemoveHead();

	    HX_RELEASE(pBuffer);
	}

	if ((m_state != tcpInitialized) && m_pResponse)
	{
	    m_pResponse->Closed(HXR_OK);
	}

	m_state = tcpInitialized;
    }
}
