/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: remote_logger.cpp,v 1.5 2005/04/01 21:21:20 ehyche Exp $
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

#include "remote_logger.h"


class HXLogMesg
{
public:
    HXLogMesg(HXLogMesgType type,
	      IHXBuffer* pPayload);
    ~HXLogMesg();
    
    HXLogMesgType Type() const;
    IHXBuffer* Payload();

private:
    HXLogMesgType m_type;
    IHXBuffer* m_pPayload;
};

HXLogMesg::HXLogMesg(HXLogMesgType type,
		     IHXBuffer* pPayload) :
    m_type(type),
    m_pPayload(pPayload)
{
    if (m_pPayload)
    {
	m_pPayload->AddRef();
    }
}

HXLogMesg::~HXLogMesg()
{
    HX_RELEASE(m_pPayload);
}
    
HXLogMesgType HXLogMesg::Type() const
{
    return m_type;
}

IHXBuffer* HXLogMesg::Payload()
{
    IHXBuffer* pRet = m_pPayload;

    if (pRet)
    {
	pRet->AddRef();
    }

    return pRet;
}

HXRemoteLogger::HXRemoteLogger() :
    m_lRefCount(0),
    m_pCCF(0),
    m_pNetSvc(0),
    m_pSocket(0),
    m_pRemoteHost(0),
    m_remotePort(0),
    m_state(HXLogClosed),
    m_seq(0)
{}

HXRemoteLogger::~HXRemoteLogger()
{
    Close();
}

HX_RESULT HXRemoteLogger::Init(IUnknown* pContext, 
			       const char* pRemoteHost,
			       UINT16 nRemotePort,
			       const char* pLogFilename)
{
    HX_RESULT res = HXR_FAILED;

    if (pContext && pLogFilename && (strlen(pLogFilename) > 0))
    {
	Close();

	pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
	pContext->QueryInterface(IID_IHXNetworkServices, (void**)&m_pNetSvc);

	m_pRemoteHost = CopyBuffer(pRemoteHost, strlen(pRemoteHost) + 1);
	m_remotePort = nRemotePort;

	IHXBuffer* pFilename = CopyBuffer(pLogFilename, 
					  strlen(pLogFilename) + 1);

	if (m_pCCF && m_pNetSvc && m_pRemoteHost && pFilename)
	{
	    // Enqueue Log open message to be sent when 
	    // we are connected
	    QueueMesg(HXLogOpen, pFilename);
	    res = HXR_OK;
	}

	HX_RELEASE(pFilename);
    }

    return res;
}

void HXRemoteLogger::Log( const char* pLogStr)
{
    HXBOOL bEnqueueMesg = FALSE;

    switch(m_state) {
    case HXLogClosed:
    {
	// Try to connect
	if (m_pNetSvc &&
	    (HXR_OK == m_pNetSvc->CreateTCPSocket(&m_pSocket)) &&
	    m_pSocket &&
	    (HXR_OK == m_pSocket->Init(this)) &&
	    m_pRemoteHost &&
	    (HXR_OK == m_pSocket->Connect((const char*)m_pRemoteHost->GetBuffer(),
					  m_remotePort)))
	{
	    m_state = HXLogConnecting;
	    bEnqueueMesg = TRUE;
	}
	else
	{
	    OnError();
	}
    } break;
    case HXLogConnecting:
    {
	// We are in the process of connecting
	// Just enqueue the message
	bEnqueueMesg = TRUE;
    } break;
    case HXLogConnected:
    {
	IHXBuffer* pPayload = CopyBuffer(pLogStr, strlen(pLogStr));
	if (pPayload &&
	    (HXR_OK != SendMesg(HXLogEntry, pPayload)))
	{
	    OnError();
	}

	HX_RELEASE(pPayload);
    }
    default:
	// Ignore the string
	break;
    };

    if (bEnqueueMesg)
    {
	IHXBuffer* pPayload = CopyBuffer(pLogStr, strlen(pLogStr));

	if (pPayload)
	{
	    QueueMesg(HXLogEntry, pPayload);
	}
	HX_RELEASE(pPayload);
    }
}

void HXRemoteLogger::Close()
{
    HX_RELEASE(m_pSocket);
    HX_RELEASE(m_pNetSvc);
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pRemoteHost);

    m_seq = 0;

    ClearMesgQueue();
}

    /*
     *  IUnknown methods
     */
STDMETHODIMP HXRemoteLogger::QueryInterface(THIS_
					    REFIID riid,
					    void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXTCPResponse))
    {
        AddRef();
        *ppvObj = (IHXTCPResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) HXRemoteLogger::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXRemoteLogger::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

    /*
     *	IHXTCPResponse methods
     */

    /************************************************************************
     *	Method:
     *	    IHXTCPResponse::ConnectDone
     *	Purpose:
     *	    A Connect operation has been completed or an error has occurred.
     */
STDMETHODIMP HXRemoteLogger::ConnectDone(THIS_
					 HX_RESULT		status)
{
    if (HXR_OK == status)
    {
	m_state = HXLogConnected;
	
	SendQueuedMesgs();
    }
    else
    {
	OnError();
    }

    return HXR_OK;
}

    /************************************************************************
     *	Method:
     *	    IHXTCPResponse::ReadDone
     *	Purpose:
     *	    A Read operation has been completed or an error has occurred.
     *	    The data is returned in the IHXBuffer.
     */
STDMETHODIMP HXRemoteLogger::ReadDone(THIS_
				      HX_RESULT		status,
				      IHXBuffer*		pBuffer)
{
    if (status != HXR_OK)
    {
	OnError();
    }

    return HXR_OK;
}

    /************************************************************************
     *	Method:
     *	    IHXTCPResponse::WriteReady
     *	Purpose:
     *	    This is the response method for WantWrite.
     *	    If HX_RESULT is ok, then the TCP channel is ok to Write to.
     */
STDMETHODIMP HXRemoteLogger::WriteReady(THIS_
					HX_RESULT		status)
{
    if (status != HXR_OK)
    {
	OnError();
    }

    return HXR_OK;
}

    /************************************************************************
     *	Method:
     *	    IHXTCPResponse::Closed
     *	Purpose:
     *	    This method is called to inform you that the TCP channel has
     *	    been closed by the peer or closed due to error.
     */
STDMETHODIMP HXRemoteLogger::Closed(THIS_
				    HX_RESULT		status)
{
    HX_RELEASE(m_pSocket);
    m_state = HXLogClosed;

    return HXR_OK;
}


IHXBuffer* HXRemoteLogger::CopyBuffer(const char* pStr,
				      UINT32 ulLength) const
{
    IHXBuffer* pRet = 0;

    if (m_pCCF && pStr &&
	(HXR_OK == m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)(&pRet))))
    {
	if (HXR_OK != pRet->Set((const UCHAR*)pStr, ulLength))
	{
	    HX_RELEASE(pRet);
	}
    }

    return pRet;
}

void HXRemoteLogger::ClearMesgQueue()
{
    while(!m_mesgList.IsEmpty())
    {
	HXLogMesg* pMesg = (HXLogMesg*)m_mesgList.RemoveHead();
	
	HX_DELETE(pMesg);
    }

    m_state = HXLogClosed;
}

void HXRemoteLogger::OnError()
{
    HX_RELEASE(m_pSocket);
    m_state = HXLogError;

    ClearMesgQueue();
}

HX_RESULT HXRemoteLogger::QueueMesg(HXLogMesgType type, IHXBuffer* pPayload)
{
    HX_RESULT res = HXR_FAILED;

    HXLogMesg* pMesg = new HXLogMesg(type, pPayload);

    if (pMesg)
    {
	m_mesgList.AddTail(pMesg);
	res = HXR_OK;
    }

    return res;
}

void HXRemoteLogger::SendQueuedMesgs()
{
    while(!m_mesgList.IsEmpty())
    {
	HXLogMesg* pMesg = (HXLogMesg*)m_mesgList.RemoveHead();

	HXLogMesgType type = pMesg->Type();
	IHXBuffer* pPayload = pMesg->Payload();

	HX_DELETE(pMesg);

	if (HXR_OK != SendMesg(type, pPayload))
	{
	    OnError();
	}
	
	HX_RELEASE(pPayload);
    }
}

void HXRemoteLogger::PackUInt32(UCHAR* pBuf, UINT32 ulVal) const
{
    pBuf[0] = (UCHAR)((ulVal >> 24) & 0xff);
    pBuf[1] = (UCHAR)((ulVal >> 16) & 0xff);
    pBuf[2] = (UCHAR)((ulVal >> 8) & 0xff);
    pBuf[3] = (UCHAR)((ulVal) & 0xff);
}

HX_RESULT HXRemoteLogger::SendMesg(HXLogMesgType type, IHXBuffer* pPayload)
{
    HX_RESULT res = HXR_FAILED;
    
    IHXBuffer* pHdr = 0;

    if ((m_state == HXLogConnected) &&
	pPayload && m_pCCF && m_pSocket &&
	(HXR_OK == m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pHdr)) &&
	(HXR_OK == pHdr->SetSize(16)))
    {
	UCHAR* pHdrBuf = pHdr->GetBuffer();
	PackUInt32(pHdrBuf, 0xff4d48ff);
	PackUInt32(pHdrBuf + 4, (UINT32)type);
	PackUInt32(pHdrBuf + 8, m_seq);
	PackUInt32(pHdrBuf + 12, pPayload->GetSize());

	if ((HXR_OK == m_pSocket->Write(pHdr)) &&
	    (HXR_OK == m_pSocket->Write(pPayload)))
	{
	    res = HXR_OK;
	}
    }

    HX_RELEASE(pHdr);

    return res;
}
