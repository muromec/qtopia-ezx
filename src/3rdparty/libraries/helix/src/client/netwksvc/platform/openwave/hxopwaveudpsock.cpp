/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxopwaveudpsock.cpp,v 1.6 2007/07/06 21:58:22 jfinnecy Exp $
 * 
 * * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
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

#include "hxopwaveudpsock.h"
#include "debug.h"
#include "hxassert.h"
#include "ihxpckts.h"
#include "smartptr.h"

#define D_UDPSOCKET 0x10000000

HXOpwaveUDPSocket::HXOpwaveUDPSocket(IHXCommonClassFactory* pCCF)
                                    : OpSocket(kUDP)
                                    , m_lRefCount(0)
                                    , m_pResponse(0)
                                    , m_pCCF(0)
                                    , m_state(udpNotInitialized)
                                    , m_ulRemoteAddr(0)
                                    , m_nRemotePort(0)
                                    , m_nLocalPort(0)
                                    , m_ulLocalAddr(0)
                                    , m_ulReadSize(0)
                                    , m_pWriteBuffer(NULL)
                                    , m_ulBytesLeftToWrite(0)
                                    , m_pReadBuffer(NULL)
{
    
    if (pCCF)
    {
        m_pCCF = pCCF;
        m_pCCF->AddRef();
    }
}


HXOpwaveUDPSocket::~HXOpwaveUDPSocket()
{
    
    DPRINTF(D_UDPSOCKET, ("HXOpwaveUDPSocket::~HXOpwaveUDPSocket()\n"));
    
    CloseSocket(HXR_OK);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pCCF);
}



/*
*  IUnknown methods
*/
STDMETHODIMP
HXOpwaveUDPSocket::QueryInterface(THIS_
                                  REFIID riid,
                                  void** ppvObj)
{
    
    DPRINTF(D_UDPSOCKET, ("HXOpwaveUDPSocket::QueryInterface()\n"));
    
    if (IsEqualIID(riid, IID_IHXUDPSocket))
    {
        
        AddRef();
        *ppvObj = (IHXUDPSocket*)this;
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
        *ppvObj = (IUnknown*)(IHXUDPSocket*)this;
        return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


STDMETHODIMP_(ULONG32)
HXOpwaveUDPSocket::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}



STDMETHODIMP_(ULONG32)
HXOpwaveUDPSocket::Release(THIS)
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
*  
*/
STDMETHODIMP
HXOpwaveUDPSocket::Init(THIS_
                        ULONG32			ulAddr,
                        UINT16			nPort,
                        IHXUDPResponse*	pUDPResponse)
{
    DPRINTF(D_UDPSOCKET, ("HXOpwaveUDPSocket::Init(%08lx, %u)\n",
        ulAddr, nPort));
    
    HX_RESULT res = HXR_OK;

    if (m_state != udpNotInitialized)
    {
        return res;
    }
    if (pUDPResponse)
    {
        HX_RELEASE(m_pResponse);
        m_pResponse = pUDPResponse;
        m_pResponse->AddRef();
    }
    m_ulRemoteAddr = ulAddr;
    m_nRemotePort = nPort;
    
    if (m_ulRemoteAddr != 0 && m_nRemotePort != 0)
    {
        m_state = udpInitialized;
        if (m_state != udpBound)
        {
 
            /// Openwave sdk does bind inside connect
            /// we do binding here due to when Bind is called, the Init is not called
            /// with valid m_nRemotePort and m_ulRemoteAddr.
            connect(m_nLocalPort, m_nRemotePort, m_ulRemoteAddr);
        
            //OpDPRINTF("UDP::Init: this=%p, ulLocalAddr=%d, localport=%d, remotePort=%d, remoteaddr=%d\n", this, 
            //            m_ulLocalAddr,m_nLocalPort, m_nRemotePort, m_ulRemoteAddr);
            m_state = udpBound;        
            m_bWritable = FALSE;
            m_bReadable = FALSE;
        }
    }

    return res;
}


STDMETHODIMP 
HXOpwaveUDPSocket::Bind(THIS_
                        UINT32 ulLocalAddr,
                        UINT16 nPort)
{
    
    DPRINTF(D_UDPSOCKET, ("HXOpwaveUDPSocket::Bind(%08lx, %u)\n", ulLocalAddr, nPort));
    
    /// Until OpSocket implement the DNS layer, this is nothting but cache address and
    /// port values
    /// OpSocket has bind function built into connect
  
    HX_RESULT res = HXR_OK;
    
    m_ulLocalAddr = ulLocalAddr;
    m_nLocalPort = nPort;
    if (m_state == udpInitialized && m_state != udpBound && m_nLocalPort != 0)
    {
        /// Only do binding if it is initialized 
        /// Openwave sdk does bind inside connect
        connect(m_nLocalPort, m_nRemotePort, m_ulRemoteAddr);
        
        //OpDPRINTF("UDP::Bind: this=%p, ulLocalAddr=%d, localport=%d, remotePort=%d, remoteaddr=%d\n", this, 
        //           m_ulLocalAddr,m_nLocalPort, m_nRemotePort, m_ulRemoteAddr);
        m_state = udpBound;        
        m_bWritable = FALSE;
        m_bReadable = FALSE;

    }
    return res;
}


STDMETHODIMP 
HXOpwaveUDPSocket::Read(THIS_
                        UINT16 Size)
{
    
    DPRINTF(D_UDPSOCKET, ("HXOpwaveUDPSocket::Read(%u)\n", Size));
    
    HX_RESULT res = HXR_OK;
    m_ulReadSize = Size;

    //OpDPRINTF("UDP:Read: this=%p, size=%d, bread=%d, state=%d, bwrite=%d\n", this, Size, m_bReadable, m_state, m_bWritable);
    if (m_state == udpBound)
    {
        if (m_bReadable && m_ulReadSize)
        {
            /// avoid recursion that might occur because 
            /// OnReadDone call in DoRead invoke ::Read again
            m_bReadable = FALSE;
            res = DoRead();
        }
    }
    return res;
    
}

/// Work horse for reading. As long as we are initilized once 
/// to read, we always try to read something of the socket.
HX_RESULT
HXOpwaveUDPSocket::DoRead()
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
 
        //OpDPRINTF("UDP::DoRead: this=%p, read=%d, askfor=%d\n", this, nRead, m_ulReadSize);
        if (nRead > 0)
        {
            pReadBuffer->SetSize(nRead);
            m_pReadBuffer = pReadBuffer;
            // Don't ADDREF, m_pReadBuffer will be released in the callback OnReadDone
        
            if (m_pResponse)
            {
                /// still to be found that address and port arguments
                /// should be put here (remote or local)
                m_pResponse->ReadDone(res, m_pReadBuffer,
                    m_ulRemoteAddr, m_nRemotePort);
            }
            
        }
    }
    HX_RELEASE(m_pReadBuffer);
    return res;
}

STDMETHODIMP 
HXOpwaveUDPSocket::Write(THIS_
                         IHXBuffer*	pBuffer)
{
    
    DPRINTF(D_UDPSOCKET, ("HXOpwaveUDPSocket::Write()\n"));

    HX_RESULT res = HXR_OK;
    if (!pBuffer)
    {
    	res = HXR_INVALID_PARAMETER;
    }
  
    //OpDPRINTF("UDP:Write: this=%p, lwsize=%d, bread=%d, newwritesize=%d, bwrite=%d\n", this, m_ulBytesLeftToWrite, m_bReadable, pBuffer->GetSize(), m_bWritable);
    if (SUCCEEDED(res) && (m_state == udpBound))
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
HX_RESULT HXOpwaveUDPSocket::DoWrite()
{
    HX_RESULT res = HXR_OK;
 
    UCHAR* pBufData = m_pWriteBuffer->GetBuffer(); 
    size_t ulActualWritten = write(pBufData, m_ulBytesLeftToWrite);
   
    HX_ASSERT(m_ulBytesLeftToWrite >= ulActualWritten);
 
    //OpDPRINTF("UDP::DoWrite, this=%p, written%d, write=%d\n\n",this, ulActualWritten, m_ulBytesLeftToWrite);
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

STDMETHODIMP
HXOpwaveUDPSocket::GetLocalPort(THIS_
                                REF(UINT16) port)
{
    
    DPRINTF(D_UDPSOCKET, ("HXOpwaveUDPSocket::GetLocalPort()\n"));
    
    HX_RESULT res = HXR_OK;
    port = m_nLocalPort;
    return res;
}



STDMETHODIMP 
HXOpwaveUDPSocket::SetOption(THIS_ 
                             HX_SOCKET_OPTION option,
                             UINT32 ulValue)
{
    
    DPRINTF(D_UDPSOCKET, ("HXOpwaveUDPSocket::SetOption(%d, %lu)\n", option, ulValue));
    HX_RESULT res = HXR_FAILED;
    
    if (m_state == udpNotInitialized)
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


STDMETHODIMP
HXOpwaveUDPSocket::WriteTo(THIS_ ULONG32	ulAddr,	UINT16 nPort,
                           IHXBuffer*		pBuffer)
{
    HX_RESULT res = HXR_OK;
    return res;
}

STDMETHODIMP
HXOpwaveUDPSocket::JoinMulticastGroup(THIS_	ULONG32	    ulMulticastAddr,
                                      ULONG32	    ulInterfaceAddr)
{
    HX_RESULT res = HXR_OK;
    return res;
}

STDMETHODIMP
HXOpwaveUDPSocket::LeaveMulticastGroup	(THIS_	ULONG32	    ulMulticastAddr,
                                         ULONG32	    ulInterfaceAddr)
{
    HX_RESULT res = HXR_OK;
    return res;
}


void HXOpwaveUDPSocket::OnWriteDone(HX_RESULT status)
{
    
    DPRINTF(D_UDPSOCKET, ("HXOpwaveUDPSocket::OnWriteDone(%ld)\n", status));
    DECLARE_SMART_POINTER_UNKNOWN scopeRef((IHXUDPSocket*)this);
    
    if (status == HXR_OK)
    {
        HX_RELEASE(m_pWriteBuffer);
        if (m_writeList.GetCount() > 0)
        {
            m_pWriteBuffer = (IHXBuffer*)m_writeList.RemoveHead();
            m_ulBytesLeftToWrite = m_pWriteBuffer->GetSize();     
        }
      
    }
    else
    {
        CloseSocket(status);
    }
}

void HXOpwaveUDPSocket::CloseSocket(HX_RESULT status)
{
    
    DPRINTF(D_UDPSOCKET, ("HXOpwaveUDPSocket::CloseConnection(%ld)\n", status));
    
    if (m_state != udpNotInitialized)
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
    m_state = udpNotInitialized;
    
}

void HXOpwaveUDPSocket::onReadable(OpSocketEvent *pSocketEvent)
{
    
    HX_RESULT res = HXR_OK;
    //OpDPRINTF("UDP:onReadable: this=%p, mrsize=%d, bread=%d, state=%d, bwrite=%d\n", this, m_ulReadSize, m_bReadable, m_state, m_bWritable);
  
    if (m_ulReadSize > 0)
    {
        /// avoid recursion that might occur because 
        /// OnReadDone call in DoRead invoke ::Read again
        m_bReadable = FALSE;
        DoRead();
    }
}

void HXOpwaveUDPSocket::onWritable(OpSocketEvent *pSocketEvent)
{
    HX_RESULT res = HXR_OK;

    m_bWritable = TRUE;
    //OpDPRINTF("UDP:onWritable: this=%p, mrsize=%d, bread=%d, lwsize=%d, bwrite=%d\n", this, m_ulReadSize, m_bReadable, m_ulBytesLeftToWrite, m_bWritable);
  
  
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

void HXOpwaveUDPSocket::onException(OpSocketEvent *pSocketEvent)
{
    static int num = 0;
    DPRINTF(D_UDPSOCKET, ("HXOpwaveUDPSocket::OnException(%ld)\n", ++num));
}


