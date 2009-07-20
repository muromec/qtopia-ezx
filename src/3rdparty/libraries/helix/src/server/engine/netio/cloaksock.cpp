/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cloaksock.cpp,v 1.5 2007/05/23 18:59:27 seansmith Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "fsio.h"
#include "tcpio.h"
#include "udpio.h"
#include "sockio.h"
#include "netbyte.h"
#include "cbqueue.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "chxpckts.h"
#include "hxprot.h"
#include "server_engine.h"
#include "inetwork_acceptor.h"
#include "resolver_dispatch.h"

#include "proc.h"
#include "client.h"
#include "server_inetwork.h"
#include "cloaksock.h"

#ifdef PAULM_CSTCPSCTIMING
#include "classtimer.h"
ClassTimer g_CSTCPSCTimer("CloakedStreamTCPSocketContext", 0, 3600);
#endif

#ifdef PAULM_STORERECTIMING
#include "classtimer.h"
ClassTimer g_STORERECTimer("StoredReceiver", 0, 3600);
#endif

/*
 * CloakedStreamTCPSocketContext methods
 *
 * works now for the single GET -- POST case only
 */
CloakedStreamTCPSocketContext::CloakedStreamTCPSocketContext(Process* proc,
    IHXResolver* pResolver)
    : IHXTCPSocketContext(proc->pc->engine, pResolver),
      m_proc(proc), m_pToClient(0), m_pToClientResponse(0), m_pReceiver(0),
      m_pToClientReadCb(0), m_nNumPosts(0), m_pClient(0)
{
#ifdef PAULM_CSTCPSCTIMING
    g_CSTCPSCTimer.Add(this);
#endif
    m_pReceiverQueue = new CHXSimpleList;
    m_BufLen = 0;
    m_pEncodedBuf = new Byte[m_BufLen];
    m_pDecodedBuf = new Byte[m_BufLen];
    m_LeftoverLen = 0;
    m_DecodedLeftoverLen = 0;
    m_bSupportsBufferedSocket = FALSE;
}

CloakedStreamTCPSocketContext::CloakedStreamTCPSocketContext(Process* proc,
    IHXResolver* pResolver, TCPIO* sock, UINT32 lForeignAddr, UINT16 nPort)
    : IHXTCPSocketContext(proc->pc->engine, pResolver, sock,
        lForeignAddr, nPort),
      m_proc(proc), m_pToClient(0), m_pToClientResponse(0), m_pReceiver(0),
      m_pToClientReadCb(0), m_nNumPosts(0), m_pClient(0)
{
#ifdef PAULM_CSTCPSCTIMING
    g_CSTCPSCTimer.Add(this);
#endif
    m_pReceiverQueue = new CHXSimpleList;
    m_BufLen = 0;
    m_pEncodedBuf = new Byte[m_BufLen];
    m_pDecodedBuf = new Byte[m_BufLen];
    m_LeftoverLen = 0;
    m_DecodedLeftoverLen = 0;
}

CloakedStreamTCPSocketContext::~CloakedStreamTCPSocketContext()
{
    int nNumPosts = m_pReceiverQueue->GetCount();
#ifdef PAULM_CSTCPSCTIMING
    g_CSTCPSCTimer.Remove(this);
#endif

    DPRINTF(D_INFO, ("ClkSTCPSC(%p)::~ClkSTCPSC() start\n", this));

    if (!m_pReceiverQueue->IsEmpty())
    {
        CHXSimpleList::Iterator     i;
        StoredReceiver*             receiver;

        for (i = m_pReceiverQueue->Begin(); i != m_pReceiverQueue->End(); ++i)
        {
            receiver = (StoredReceiver *)(*i);
            // ask the response object to do cleanup when Closed
            receiver->m_pReceiverResp->Closed(HXR_OK);
            delete receiver;
            receiver = 0;
        }
    }
    delete m_pReceiverQueue;
    m_pReceiverQueue = 0;

    if (m_pToClientResponse)
    {
        m_pToClientResponse->Closed(HXR_OK);
#ifdef PAULM_CLIENTAR
        REL_NOTIFY(m_pToClientResponse, 12);
#endif
        m_pToClientResponse->Release();
        m_pToClientResponse = 0;
    }
    if (m_pToClient)
    {
#ifdef PAULM_IHXTCPSCAR
        REL_NOTIFY(m_pToClient, 10);
#endif
        m_pToClient->Release();
        m_pToClient = 0;
    }
    HX_RELEASE(m_pClient);
    if (m_pToClientReadCb)
    {
        m_pToClientReadCb->Release();
        m_pToClientReadCb = 0;
    }

    if (m_pEncodedBuf)
    {
        delete [] m_pEncodedBuf;
        m_pEncodedBuf = 0;
    }
    if (m_pDecodedBuf)
    {
        delete [] m_pDecodedBuf;
        m_pDecodedBuf = 0;
    }

    DPRINTF(D_INFO, ("ClkSTCPSC(%p)::~ClkSTCPSC() end\n", this));
}

STDMETHODIMP
CloakedStreamTCPSocketContext::Read(UINT16 Size)
{
    if (!m_pReceiverQueue->IsEmpty())
    {
        IHXTCPSocketContext* currRcvr = CurrentReceiver();
        DPRINTF(0x00200000, ("ClkSTCPSC(%p)::Read -- proc(%p), "
               "currRcvr(%p)->Read()\n",
               this, ((Client *)m_pTCPResponse)->m_pProc, currRcvr));
        return currRcvr->Read(Size);
    }
    return 0;
}

STDMETHODIMP
CloakedStreamTCPSocketContext::Write(IHXBuffer* pBuffer)
{
    DPRINTF(0x00200000, ("ClkSTCPSC(%p)::Write -- proc(%p), "
           "m_pToClient(%p)->Write()\n",
           this, ((Client *)m_pTCPResponse)->m_pProc, m_pToClient));
    if (m_pToClient->SupportsBufferedSocket())
    {
        HX_RESULT res = m_pToClient->BufferedWrite(pBuffer);
        m_pToClient->FlushWrite();
        return res;
    }
    else
    {
        return m_pToClient->Write(pBuffer);
    }
}

STDMETHODIMP
CloakedStreamTCPSocketContext::BufferedWrite(IHXBuffer* pBuffer)
{
    DPRINTF(0x00200000, ("ClkSTCPSC(%p)::Write -- proc(%p), "
           "m_pToClient(%p)->BufferedWrite()\n",
           this, ((Client *)m_pTCPResponse)->m_pProc, m_pToClient));
    return m_pToClient->BufferedWrite(pBuffer);
}

STDMETHODIMP
CloakedStreamTCPSocketContext::FlushWrite()
{
    DPRINTF(0x00200000, ("ClkSTCPSC(%p)::Write -- proc(%p), "
           "m_pToClient(%p)->FlushWrite()\n",
           this, ((Client *)m_pTCPResponse)->m_pProc, m_pToClient));
    return m_pToClient->FlushWrite();
}

STDMETHODIMP
CloakedStreamTCPSocketContext::SetDesiredPacketSize(UINT32 size)
{
    DPRINTF(0x00200000, ("ClkSTCPSC(%p)::Write -- proc(%p), "
           "m_pToClient(%p)->FlushWrite()\n",
           this, ((Client *)m_pTCPResponse)->m_pProc, m_pToClient));
    return m_pToClient->SetDesiredPacketSize(size);
}

STDMETHODIMP
CloakedStreamTCPSocketContext::WantWrite()
{
    return m_pToClient->WantWrite();
}

void
CloakedStreamTCPSocketContext::enableRead()
{
    DPRINTF(0x10000000, ("ClkSTCPSC::eR -- this(%p)\n", this));
    if (!m_pReceiverQueue->IsEmpty())
    {
        StoredReceiver* currRcvr
            = (StoredReceiver *)m_pReceiverQueue->GetHead();
        currRcvr->m_pReceiver->enableRead();
    }
}

void
CloakedStreamTCPSocketContext::disableRead()
{
    DPRINTF(0x10000000, ("ClkSTCPSC::dR -- this(%p)\n", this));
    if (!m_pReceiverQueue->IsEmpty())
        CurrentReceiver()->disableRead();
}

int
CloakedStreamTCPSocketContext::readUndo(BYTE* pData, UINT32 nDataLen)
{
    if (!m_pReceiverQueue->IsEmpty())
        return CurrentReceiver()->readUndo(pData, nDataLen);
    return 0;
}

void
CloakedStreamTCPSocketContext::disconnect()
{
    m_pToClient->disconnect();
}

void
CloakedStreamTCPSocketContext::reconnect(Engine* pEngine)
{
    if (!m_pReceiverQueue->IsEmpty())
        CurrentReceiver()->reconnect(pEngine);
}

int
CloakedStreamTCPSocketContext::write_flush()
{
    return m_pToClient->write_flush();
}

TCPIO*
CloakedStreamTCPSocketContext::getReadTCPIO()
{
    if (!m_pReceiverQueue->IsEmpty())
    {
        IHXTCPSocketContext* currRcvr = CurrentReceiver();
        return currRcvr->getReadTCPIO();
    }
    return 0;
}

TCPIO*
CloakedStreamTCPSocketContext::getWriteTCPIO()
{
    return m_pToClient->getWriteTCPIO();
}

SIO*
CloakedStreamTCPSocketContext::getReadSIO()
{
    if (!m_pReceiverQueue->IsEmpty())
        return CurrentReceiver()->getReadSIO();
    return 0;
}

SIO*
CloakedStreamTCPSocketContext::getWriteSIO()
{
    return m_pToClient->getWriteSIO();
}

STDMETHODIMP
CloakedStreamTCPSocketContext::AddSender(IHXTCPSocketContext* sender,
    IHXTCPResponse* senderResponse, Client* pClient)
{
    m_pToClient = sender;
#ifdef PAULM_IHXTCPSCAR
    ADDR_NOTIFY(m_pToClient, 10);
#endif
    m_pToClient->AddRef();
    m_pToClientResponse = senderResponse;
#ifdef PAULM_CLIENTAR
    ADDR_NOTIFY(m_pToClientResponse, 11);
#endif
    m_pToClientResponse->AddRef();

    m_pClient = pClient;
    m_pClient->AddRef();

    return HXR_OK;
}

STDMETHODIMP
CloakedStreamTCPSocketContext::AddReceiver(IHXTCPSocketContext* fromClient,
                                     IHXTCPResponse* fromClientResponse,
                                     Byte* already_read_data,
                                     UINT32 already_read_data_len,
                                     Client* c)
{
    m_nNumPosts++;
    StoredReceiver* new_receiver = new StoredReceiver(
        fromClient->m_pReadCallback, fromClient, fromClientResponse,
        already_read_data, already_read_data_len);
    m_pReceiverQueue->AddTail(new_receiver);

    if (m_pReceiverQueue->GetCount() > 1)
    {
        RotateReceiver();
    }
    return HXR_OK;
}

void
CloakedStreamTCPSocketContext::RotateReceiver()
{
    StoredReceiver*     old_receiver;

    old_receiver = (StoredReceiver *)m_pReceiverQueue->RemoveHead();

    DPRINTF(0x48000000, ("ClkTCPSC::RR() is killing me!\n"));
    // remove the receiver from the READERS/WRITERS queue
    old_receiver->m_pReceiverResp->Closed(HXR_OK);
    delete old_receiver;
    old_receiver = 0;

    if (!m_pReceiverQueue->IsEmpty())
    {
        m_proc->pc->engine->callbacks.add(HX_READERS,
            CurrentReceiver()->getReadTCPIO(),
            CurrentReadCallback());
    }
}

IHXTCPSocketContext::TCPSocketReadCallback*
CloakedStreamTCPSocketContext::CurrentReadCallback()
{
    if (m_pReceiverQueue->IsEmpty())
    {
        return 0;
    }
    return ((StoredReceiver *)m_pReceiverQueue->GetHead())->m_pReadCb;
}

IHXTCPSocketContext*
CloakedStreamTCPSocketContext::CurrentReceiver()
{
    if (m_pReceiverQueue->IsEmpty())
    {
        return 0;
    }
    StoredReceiver* head = (StoredReceiver *)m_pReceiverQueue->GetHead();
    if (!head)
        return 0;

    DPRINTF(0x00200000, ("CurrentReceiver(%p), remainingDataLen(%lu)\n",
            head->m_pReceiver, head->RemainingDataLen()));
    return head->m_pReceiver;
}

HX_RESULT
CloakedStreamTCPSocketContext::DoRead()
{
    if (m_pReceiverQueue->IsEmpty())
        return HXR_FAIL;

    StoredReceiver* head = (StoredReceiver *)m_pReceiverQueue->GetHead();
    if (!head)
        return HXR_FAIL;

    IHXTCPSocketContext* currRcvr = head->m_pReceiver;
    UINT32 len = head->RemainingDataLen();
    DPRINTF(0x00200000, ("%lu: ClkSTCPSC::DR -- currRcvr(%p), "
            "remainingDataLen(%lu)\n",
            ((Client *)m_pTCPResponse)->m_ulConnId, currRcvr, len));

    return m_pTCPResponse->ReadDone(HXR_OK, 0);
}

void
CloakedStreamTCPSocketContext::DoWrite()
{
    DPRINTF(0x00200000, ("ClkSTCPSC::DW -- m_pToClient(%p)\n", m_pToClient));
    m_pToClient->DoWrite();
}

CloakedStreamTCPSocketContext::StoredReceiver::StoredReceiver
(
    IHXTCPSocketContext::TCPSocketReadCallback* pReadCb,
    IHXTCPSocketContext* receiver, IHXTCPResponse* receiverResp,
    Byte* already_read_data, UINT32 already_read_data_len
)
{
#ifdef PAULM_STORERECTIMING
    g_STORERECTimer.Add(this);
#endif
    m_pReadCb = pReadCb;
    m_pReadCb->AddRef();
    m_pReceiver = receiver;
#ifdef PAULM_IHXTCPSCAR
    ADDR_NOTIFY(m_pReceiver, 9);
#endif
    m_pReceiver->AddRef();
    m_pReceiverResp = receiverResp;
#ifdef PAULM_CLIENTAR
    ADDR_NOTIFY(m_pReceiverResp, 10);
#endif
    m_pReceiverResp->AddRef();
    m_lAlreadyReadDataLen = already_read_data_len;
    if (m_lAlreadyReadDataLen > 0)
    {
        m_pAlreadyReadData = already_read_data;
    }
    else
    {
        m_pAlreadyReadData = 0;
    }
    m_pRemainingData = m_pAlreadyReadData;
}

CloakedStreamTCPSocketContext::StoredReceiver::~StoredReceiver()
{
#ifdef PAULM_STORERECTIMING
    g_STORERECTimer.Remove(this);
#endif
    if (m_pReadCb)
    {
        m_pReadCb->Release();
        m_pReadCb = 0;
    }
    if (m_pReceiverResp)
    {
#ifdef PAULM_CLIENTAR
        REL_NOTIFY(m_pReceiverResp, 11);
#endif
        m_pReceiverResp->Release();
        m_pReceiverResp = 0;
    }
    if (m_pReceiver)
    {
#ifdef PAULM_IHXTCPSCAR
        REL_NOTIFY(m_pReceiver, 9);
#endif
        m_pReceiver->Release();
        m_pReceiver = 0;
    }
    if (m_lAlreadyReadDataLen && m_pAlreadyReadData)
    {
        delete [] m_pAlreadyReadData;
        m_pAlreadyReadData = 0;
        m_lAlreadyReadDataLen = 0;
    }
}

UINT32
CloakedStreamTCPSocketContext::StoredReceiver::RemainingDataLen()
{
    if (m_pAlreadyReadData == 0)
    {
        return 0;
    }
    DPRINTF(0x00200000, ("ClkSTCPSC::RDL -- m_pAlreadyReadData(%p), "
            "m_lAlreadyReadDataLen(%lu), m_pRemainingData(%p)\n",
            m_pAlreadyReadData, m_lAlreadyReadDataLen, m_pRemainingData));
    return m_pAlreadyReadData + m_lAlreadyReadDataLen - m_pRemainingData;
}
