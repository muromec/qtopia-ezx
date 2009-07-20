/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsymbianudpsock.cpp,v 1.11 2006/12/06 10:18:17 gahluwalia Exp $
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

#include "platform/symbian/hxsymbianudpsock.h"
#include "hxsymbiansockhlp.h"

#include "debug.h"
#include "hxassert.h"
#include "ihxpckts.h"
#include "smartptr.h"
#include "hxtick.h"
#include "hxtbuf.h"

#include <in_sock.h>

#define D_UDPSOCKET 0x10000000

class HXSymbianUDPWriter : public CActive
{
public:
    HXSymbianUDPWriter(HXSymbianUDPSocket* pParent);
    ~HXSymbianUDPWriter();

    void Write(RSocket& socket, ULONG32 ulAddr, UINT16 nPort,
	       IHXBuffer* pBuffer);
    
private:
    void RunL();
    void DoCancel();

    HXSymbianUDPSocket* m_pParent;
    IHXBuffer* m_pBuffer;
    TInetAddr m_sendAddr;
    TPtrC8 m_bufDes;
};

HXSymbianUDPWriter::HXSymbianUDPWriter(HXSymbianUDPSocket* pParent) :
    CActive(EPriorityStandard),
    m_pParent(pParent),
    m_pBuffer(0)
{
    CActiveScheduler::Add(this);
}

HXSymbianUDPWriter::~HXSymbianUDPWriter()
{    
    if (IsActive())
	Cancel();

    HX_RELEASE(m_pBuffer);
    m_pParent = 0;
}
    
void HXSymbianUDPWriter::Write(RSocket& socket, 
			       ULONG32 ulAddr,
			       UINT16 nPort,
			       IHXBuffer* pBuffer)
{
    HX_RELEASE(m_pBuffer);

    m_pBuffer = pBuffer;

    if (m_pBuffer)
    {
	m_pBuffer->AddRef();
    
	m_bufDes.Set(m_pBuffer->GetBuffer(), m_pBuffer->GetSize());

	m_sendAddr.SetAddress(ulAddr);
	m_sendAddr.SetPort(nPort);

	socket.SendTo(m_bufDes, m_sendAddr, 0, iStatus);
	SetActive();
    }
}

void HXSymbianUDPWriter::RunL()
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

void HXSymbianUDPWriter::DoCancel()
{}

class HXSymbianUDPWriteInfo
{
public:
    HXSymbianUDPWriteInfo(ULONG32    ulAddr, UINT16     nPort,
			  IHXBuffer* pBuffer);
    ~HXSymbianUDPWriteInfo();

    ULONG32 Address() const;
    UINT16 Port() const;
    IHXBuffer* GetBuffer();

private:
    ULONG32 m_ulAddr;
    UINT16 m_nPort;
    IHXBuffer* m_pBuffer;
};

HXSymbianUDPWriteInfo::HXSymbianUDPWriteInfo(ULONG32    ulAddr, 
					     UINT16     nPort,
					     IHXBuffer* pBuffer) :
    m_ulAddr(ulAddr),
    m_nPort(nPort),
    m_pBuffer(pBuffer)
{
    if (m_pBuffer)
    {
	m_pBuffer->AddRef();
    }
}

HXSymbianUDPWriteInfo::~HXSymbianUDPWriteInfo()
{
    HX_RELEASE(m_pBuffer);
}

ULONG32 HXSymbianUDPWriteInfo::Address() const
{
    return m_ulAddr;
}

UINT16 HXSymbianUDPWriteInfo::Port() const
{
    return m_nPort;
}

IHXBuffer* HXSymbianUDPWriteInfo::GetBuffer()
{
    IHXBuffer* pRet = m_pBuffer;

    if (pRet)
	pRet->AddRef();

    return pRet;
}

class HXSymbianUDPReader : public CActive
{
public:
    HXSymbianUDPReader(HXSymbianUDPSocket* pParent,
                       IHXCommonClassFactory* pCCF);
    ~HXSymbianUDPReader();
    
    HX_RESULT Read(RSocket& socket, UINT16);

private:
    void RunL();
    void DoCancel();

    HXSymbianUDPSocket* m_pParent;
    IHXBuffer* m_pBuffer;
    TPtr8 m_bufDes;
    TInetAddr m_recvAddr;
    IHXCommonClassFactory* m_pCCF;
};

HXSymbianUDPReader::HXSymbianUDPReader(HXSymbianUDPSocket* pParent,
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

HXSymbianUDPReader::~HXSymbianUDPReader()
{
    if (IsActive())
	Cancel();
    
    HX_RELEASE(m_pBuffer);
    HX_RELEASE(m_pCCF);
    m_pParent = 0;
}

HX_RESULT HXSymbianUDPReader::Read(RSocket& socket, UINT16 uSize)
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
            
            socket.RecvFrom(m_bufDes, m_recvAddr, 0, iStatus);
            SetActive();
        }
    }

    return res;
}

void HXSymbianUDPReader::RunL()
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
    if( res != HXR_OUTOFMEMORY )
    {
        HXSymbianSocketHelper::CopyOrTransfer(m_pCCF, m_bufDes.Length(), 
                                              m_pBuffer, pBuffer);
    }

    // All error handling is done through the response object.
    m_pParent->OnReadDone(res,
			  (ULONG32)m_recvAddr.Address(),
			  m_recvAddr.Port(),
			  pBuffer);

    HX_RELEASE(pBuffer);
}

void HXSymbianUDPReader::DoCancel()
{}

HXSymbianUDPSocket::HXSymbianUDPSocket(IHXCommonClassFactory* pCCF) :
    m_lRefCount(0),
    m_pResponse(0),
    m_state(udpNotOpen),
    m_ulRemoteAddr(0),
    m_nRemotePort(0),
    m_pWriter(0),
    m_pReader(0)
{
    m_pWriter = new HXSymbianUDPWriter(this);
    m_pReader = new HXSymbianUDPReader(this, pCCF);

    if (pCCF && m_pWriter && m_pReader &&
	(m_socketServ.Connect() == KErrNone) &&
	(m_socket.Open(m_socketServ, KAfInet, 
		       KSockDatagram, KProtocolInetUdp) == KErrNone))
    {
	m_state = udpOpen;
    }
}

HXSymbianUDPSocket::~HXSymbianUDPSocket()
{
    if (m_state != udpNotOpen)
    {
	CloseSocket(HXR_OK);

	m_socket.Close();
	m_socketServ.Close();
    }

    HX_DELETE(m_pWriter);
    HX_DELETE(m_pReader);

    HX_RELEASE(m_pResponse);
}

    /*
     *  IUnknown methods
     */
STDMETHODIMP HXSymbianUDPSocket::QueryInterface(THIS_
						REFIID riid,
						void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXNetworkServices*)this },
		{ GET_IIDHANDLE(IID_IHXUDPSocket), (IHXUDPSocket*) this },
		{ GET_IIDHANDLE(IID_IHXSetSocketOption), (IHXSetSocketOption*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXSymbianUDPSocket::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXSymbianUDPSocket::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}

    /*
     *	IHXUDPSocket methods
     *
     *  Network addresses and ports are in native byte order
     */

STDMETHODIMP HXSymbianUDPSocket::Init(THIS_
				      ULONG32			ulAddr,
				      UINT16			nPort,
				      IHXUDPResponse*	pUDPResponse)
{
    DPRINTF(D_UDPSOCKET, ("HXSymbianUDPSocket::Init(%08lx, %u)\n",
			  ulAddr, nPort));

    HX_RESULT res = HXR_FAILED;

    if (m_state != udpNotOpen)
    {
	if (!m_pResponse && !pUDPResponse)
	{
	    res = HXR_UNEXPECTED;
	}
	else if (pUDPResponse)
	{
	    HX_RELEASE(m_pResponse);
	    m_pResponse = pUDPResponse;
	    m_pResponse->AddRef();
	}

	if (m_pResponse)
	{
	    m_ulRemoteAddr = ulAddr;
	    m_nRemotePort = nPort;
	
	    if (m_state == udpOpen)
	    {
		m_state = udpInitialized;
	    }
	    
	    res = HXR_OK;
	}
    }

    return res;
}

STDMETHODIMP HXSymbianUDPSocket::Bind(THIS_
				      UINT32  ulLocalAddr,
				      UINT16  nPort)
{
    DPRINTF(D_UDPSOCKET, ("HXSymbianUDPSocket::Bind(%08lx, %u)\n",
			  ulLocalAddr, nPort));

    HX_RESULT res = HXR_FAILED;
    TInetAddr addr(ulLocalAddr, nPort);

    if ((m_state != udpNotOpen) &&
	(m_socket.Bind(addr) == KErrNone))
    {
	m_state = udpBound;
	res = HXR_OK;
    }

    return res;
}

STDMETHODIMP HXSymbianUDPSocket::Read(THIS_ UINT16 Size)
{
    DPRINTF(D_UDPSOCKET, ("HXSymbianUDPSocket::Read(%d)\n", Size));

    HX_RESULT res = HXR_FAILED;

    if (m_state == udpBound)
    {
        // Note: We intentionally ignore the read
        //       size and use the maximum possible
        //       UDP packet size. This allows us to
        //       properly receive any UDP packet.
	res = DoRead(0xf000);
    }

    return res;
}

STDMETHODIMP HXSymbianUDPSocket::Write(THIS_ IHXBuffer*	pBuffer)
{
    DPRINTF(D_UDPSOCKET, ("HXSymbianUDPSocket::Write()\n"));

    return WriteTo(m_ulRemoteAddr, m_nRemotePort, pBuffer);
}

STDMETHODIMP HXSymbianUDPSocket::WriteTo(THIS_ 
					 ULONG32    ulAddr,
					 UINT16	    nPort,
					 IHXBuffer* pBuffer)
{
    DPRINTF(D_UDPSOCKET, ("HXSymbianUDPSocket::WriteTo(%08lx, %d)\n",
			  ulAddr, nPort));

    HX_RESULT res = HXR_FAILED;

    if ((m_state == udpBound) && pBuffer)
    {
	if (m_pWriter->IsActive())
	{
	    // A write is in process. Cache the information
	    // about this write for later use.

	    HXSymbianUDPWriteInfo* pWriteInfo = 
		new HXSymbianUDPWriteInfo(ulAddr, nPort, pBuffer);

	    if (pWriteInfo)
	    {
		LISTPOSITION listRet = m_writeList.AddTail(pWriteInfo);
                if( listRet == NULL )
                {
		    res = HXR_OUTOFMEMORY;
		    HX_DELETE(pWriteInfo);
                }
                else
                {
		    res = HXR_OK;
                }
	    }
	}
	else
	{
	    // Do the write now
	    m_pWriter->Write(m_socket, ulAddr, nPort, pBuffer);
	    res = HXR_OK;
	}
    }

    return res;
}

STDMETHODIMP HXSymbianUDPSocket::GetLocalPort(THIS_
					      REF(UINT16) port)
{
    DPRINTF(D_UDPSOCKET, ("HXSymbianUDPSocket::GetLocalPort()\n"));

    HX_RESULT res = HXR_FAILED;

    if (m_state == udpBound)
    {
	port = m_socket.LocalPort();

	res = HXR_OK;
    }

    return res;
}

STDMETHODIMP HXSymbianUDPSocket::JoinMulticastGroup(THIS_
						    ULONG32 ulMulticastAddr,
						    ULONG32 ulInterfaceAddr)
{
    DPRINTF(D_UDPSOCKET, ("HXSymbianUDPSocket::JoinMulticastGroup(%08lx %08lx)\n",
			  ulMulticastAddr, ulInterfaceAddr));
    return HXR_NOTIMPL;
}
    
STDMETHODIMP HXSymbianUDPSocket::LeaveMulticastGroup(THIS_
						     ULONG32 ulMulticastAddr,
						     ULONG32 ulInterfaceAddr)
{
    DPRINTF(D_UDPSOCKET, ("HXSymbianUDPSocket::LeaveMulticastGroup(%08lx %08lx)\n",
			  ulMulticastAddr, ulInterfaceAddr));
    return HXR_NOTIMPL;
}

STDMETHODIMP HXSymbianUDPSocket::SetOption(THIS_ 
					   HX_SOCKET_OPTION option,
					   UINT32 ulValue)
{
    DPRINTF(D_UDPSOCKET, ("HXSymbianUDPSocket::SetOption(%d, %ld)\n",
			  option, ulValue));
    HX_RESULT res = HXR_FAILED;

    if (m_state == udpNotOpen)
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

void HXSymbianUDPSocket::OnWriteDone(HX_RESULT status)
{
    DPRINTF(D_UDPSOCKET, ("HXSymbianUDPSocket::WriteDone(%ld)\n", status));

    if (status == HXR_OK)
    {
	if (m_writeList.GetCount() > 0)
	{
	    // Write the next buffer in the list
	    HXSymbianUDPWriteInfo* pWriteInfo = 
		(HXSymbianUDPWriteInfo*)m_writeList.RemoveHead();

	    IHXBuffer* pBuffer = pWriteInfo->GetBuffer();

	    m_pWriter->Write(m_socket, 
			     pWriteInfo->Address(), pWriteInfo->Port(),
			     pBuffer);

	    HX_RELEASE(pBuffer);

	    delete pWriteInfo;
	}
    }
    else
    {
	CloseSocket(status);
    }
}

void HXSymbianUDPSocket::OnReadDone(HX_RESULT status, 
				    ULONG32 ulAddr, UINT16 nPort, 
				    IHXBuffer* pBuffer)
{
    DPRINTF(D_UDPSOCKET, ("HXSymbianUDPSocket::OnReadDone(%ld, %08lx, %d)\n",
			  status, ulAddr, nPort));

    DECLARE_SMART_POINTER_UNKNOWN scopeRef((IHXUDPSocket*)this);
    
    // We assume that the reponse object is going to be responsible for error 
    // handling, so the symbian network services does not try to notify
    // anyone of errors returned here.
    if (m_pResponse)
    {
	m_pResponse->ReadDone(status, pBuffer, ulAddr, nPort);
    }

    if (status != HXR_OK)
    {
	CloseSocket(status);
    }
}

HX_RESULT HXSymbianUDPSocket::DoRead(UINT16 size)
{
    DPRINTF(D_UDPSOCKET, ("HXSymbianUDPSocket::DoRead(%d)\n", size));

    HX_RESULT res = HXR_FAILED;
    
    if (m_pReader)
    {
        res = m_pReader->Read(m_socket, size);
    }

    return res;
}

void HXSymbianUDPSocket::CloseSocket(HX_RESULT status)
{
    if (m_state != udpNotOpen)
    {
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
	    HXSymbianUDPWriteInfo* pWriteInfo = 
		(HXSymbianUDPWriteInfo*)m_writeList.RemoveHead();
	    
	    HX_DELETE(pWriteInfo);
	}

	m_state = udpNotOpen;
    }
}
