/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspmcast.cpp,v 1.13 2007/03/08 00:17:42 tknox Exp $
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

#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"

#include "platform.h"
#include "hxassert.h"
#include "debug.h"
#include "hxstrutl.h"
#include "hxspriv.h"
#include "hxinetaddr.h"
#include "cbqueue.h"
#include "fio.h"
#include "sio.h"
#include "url.h"
#include "netdrv.h"
#include "hxtypes.h"
#include "servchallenge.h"
#include "player.h"
#include "hxdeque.h"
#include "hxstring.h"
#include "chxpckts.h"
#include "hxslist.h"
#include "mimehead.h"
#include "servpckts.h"
#include "rtsputil.h"
#include "rtspmsg.h"
#include "rtspmdsc.h"
#include "rtsppars.h"
#include "rtspserv.h"
#include "plgnhand.h"
#include "timerep.h"
#include "rtspif.h"
#include "rtspservtran.h"
#include "server_request.h"
#include "base_errmsg.h"
#include "hxpiids.h"
#include "hxreg.h"
#include "multicast_mgr.h"
#include "rtsptran.h"
#include "rdt_base.h"
#include "rdt_tcp.h"
#include "rdt_udp.h"
#include "rtspmcast.h"
#include "udpio.h"
#include "hxmap.h"

STDMETHODIMP
RTSPServerMulticastTransport::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXStatistics*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXStatistics))
    {
        AddRef();
        *ppvObj = (IHXStatistics*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP
RTSPServerMulticastTransport::InitializeStatistics(UINT32 ulRegistryID)
{
    return HXR_NOTIMPL;
    //return m_pTransport->initializeStatistics(ulRegistryID);
}

STDMETHODIMP
RTSPServerMulticastTransport::UpdateStatistics()
{
    return HXR_NOTIMPL;
    //return m_pTransport->updateStatistics();
}

RTSPServerMulticastTransport::RTSPServerMulticastTransport(
    IUnknown* pContext,
    Transport* pTrans, UINT32 ulTTL, UINT32 ulAddress,
    MulticastManager* pMgr)
    : m_ulRegistryID(0)
    , m_pMgr(pMgr)
    , m_unSourcePort(0)
    , m_pUDPMcastSocket(0)
    , m_pContext(pContext)
{
    m_pContext->AddRef();

    m_pTransports = new CHXMapStringToOb;
    m_pTransMutex = HXMutexCreate();

    m_pStreamHandler = pTrans->GetStreamHandler();

    SetupMulticastSocket(ulTTL);

    IHXDescriptorRegistration* pDescReg = NULL;
    m_pContext->QueryInterface(IID_IHXDescriptorRegistration, (void**)&pDescReg);
    if (pDescReg)
    {
        pDescReg->RegisterSockets(1);
    }
    HX_RELEASE( pDescReg );
}

RTSPServerMulticastTransport::RTSPServerMulticastTransport(IUnknown* pContext, CHXMapStringToOb* pTransports)
    : m_pStreamHandler(NULL)
    , m_pTransports(pTransports)
    , m_ulRegistryID(0)
    , m_unSourcePort(0)
    , m_pMgr(NULL)
    , m_pUDPMcastSocket(NULL)
    , m_pContext(pContext)
{
    m_pContext->AddRef();
    m_pTransMutex = HXMutexCreate();
}


RTSPServerMulticastTransport::~RTSPServerMulticastTransport()
{
    if (m_pStreamHandler)
    {
        m_pStreamHandler->Release();
        m_pStreamHandler = NULL;
    }

    CHXMapStringToOb::Iterator i;

    for (i = m_pTransports->Begin(); i != m_pTransports->End(); ++i)
    {
        RTSPMcastInfo* pInfo = (RTSPMcastInfo*)(*i);
        pInfo->m_pTransport->Done();
        HX_RELEASE(pInfo->m_pTransport);
        delete pInfo;
    }
    HXMutexDestroy(m_pTransMutex);

    IHXThreadSafeScheduler* pScheduler = NULL;
    m_pContext->QueryInterface(IID_IHXThreadSafeScheduler, (void**)&pScheduler);
    if (pScheduler)
    {
        IHXCallback* pCB = new SockCloseCallback(m_pContext, m_pUDPMcastSocket);
        pScheduler->RelativeEnter(pCB, 0);
    }
    HX_RELEASE( pScheduler );
    HX_RELEASE( m_pContext );
}

void RTSPServerMulticastTransport::SetupMulticastSocket(UINT32 ulTTL)
{
    //Create a upd/multicast socket using networkservices.
    IHXNetServices *pNetSvc = NULL;
    m_pContext->QueryInterface(IID_IHXNetServices, (void**)&pNetSvc);
    if ( pNetSvc )
    {
	pNetSvc->CreateSocket(&m_pUDPMcastSocket);
    }

    if ( m_pUDPMcastSocket )
    {
	HX_RESULT hxsocketerror = 0;
	
	//currently only do multicasting to ipv4 address.
	hxsocketerror = m_pUDPMcastSocket->Init(HX_SOCK_FAMILY_IN4, HX_SOCK_TYPE_MCAST, HX_SOCK_PROTO_ANY);
	HX_ASSERT(SUCCEEDED(hxsocketerror));

	//Set TTL value.
	m_pUDPMcastSocket->SetOption(HX_SOCKOPT_IN4_MULTICAST_TTL, ulTTL);

	//Set re-use address option.
	m_pUDPMcastSocket->SetOption(HX_SOCKOPT_REUSEADDR, TRUE);

	//Create a IPV4 SockAddr object using networkservices.
	IHXSockAddr *pIAddress = 0;
        pNetSvc->CreateSockAddr( HX_SOCK_FAMILY_IN4, &pIAddress);

	if (pIAddress)
	{
	    m_pUDPMcastSocket->Bind(pIAddress);
	}

	//Get Port number.
	IHXSockAddr *pILocalAddress = NULL;
	m_pUDPMcastSocket->GetLocalAddr(&pILocalAddress);
	if ( pILocalAddress )
	{
	    m_unSourcePort = pILocalAddress->GetPort();
	}
	HX_RELEASE(pILocalAddress);
	HX_RELEASE(pIAddress);
    }

    HX_RELEASE(pNetSvc);

    return;
}

STDMETHODIMP
RTSPServerMulticastTransport::SockCloseCallback::Func()
{
    IHXDescriptorRegistration* pDescReg = NULL;
    m_pContext->QueryInterface(IID_IHXDescriptorRegistration, (void**)&pDescReg);
    if (pDescReg)
    {
        pDescReg->UnRegisterSockets(1);
    }
    HX_RELEASE( pDescReg );

    //Socket is closed by RDTUDPTransport class.
    HX_RELEASE( m_pUDPMcastSocket );
    HX_RELEASE( m_pContext );

    return HXR_OK;
}


HX_RESULT
RTSPServerMulticastTransport::SubscriptionDone(BYTE* bRuleOn,
                                               REF(UINT32)ulSourcePort,
                                               REF(UINT32)ulPort,
                                               REF(UINT32)ulAddr,
                                               REF(TransportStreamHandler*)pHandler)
{
    char pSub[4096] = { 0 };
    int offset = 0;
    int i, j;

    for (j = 0; j < RULE_TABLE_HEIGHT; j++)
    {
        for (i = 0; i < RULE_TABLE_WIDTH; i++)
        {
            if (bRuleOn[i * RULE_TABLE_HEIGHT + j])
            {
                if (offset < 4000)
                {
                    offset += sprintf(pSub+offset, "s%dr%d;", j, i);
                }
                else
                {
                    HX_ASSERT(0); // pSub is too small
                    break;
                }
            }
        }
    }

    ulSourcePort = m_unSourcePort;

    void* pVoid = 0;
    HXMutexLock(m_pTransMutex);
    m_pTransports->Lookup(pSub, pVoid);

    if (pVoid)
    {
        HXMutexUnlock(m_pTransMutex);
        struct RTSPMcastInfo* pInfo = (struct RTSPMcastInfo*)pVoid;
        pInfo->m_ulPlayerCount++;
        ulPort = m_pMgr->m_ulRTSPPort;

        // this tranport contains the right resend buffer...
        pHandler = pInfo->m_pTransport->GetStreamHandler();

        /*
         * XXXSMPNOW We never reduce the m_ulPlayerCount which causes
         * to send multicasts of substreams that may not be needed
         * until all players disconnect form the whole stream.
         * This should be fixed.
         */

        if (pInfo->m_bAddrAllocated)
        {
            ulAddr = pInfo->m_ulAddress;
            return HXR_OK;
        }
        else
        {
            ulAddr = 0;
            return HXR_FAIL;
        }
    }
    else
    {
        RTSPMcastInfo* pInfo = new RTSPMcastInfo;

        if (FAILED(m_pMgr->GetNextAddress(pInfo->m_ulAddress, m_pMgr->m_ulRTSPPort)))
        {
            pInfo->m_bAddrAllocated = FALSE;
        }
        else
        {
            pInfo->m_bAddrAllocated = TRUE;
        }

        RDTUDPTransport* pTransport;
        pTransport = new RDTUDPTransport(TRUE, 1, RTSP_TR_RDT_MCAST);
        pTransport->AddRef();

        pTransport->MulticastSetup(m_pStreamHandler);

	pTransport->init(m_pContext, m_pUDPMcastSocket, 0);

#if NOTYET
        pTransport->setForeignAddress(pInfo->m_ulAddress, (UINT16)m_pMgr->m_ulRTSPPort);
#endif
	SetAddressOnTransport( pTransport, pInfo->m_ulAddress, m_pMgr->m_ulRTSPPort );

        ulPort = m_pMgr->m_ulRTSPPort;

        pInfo->m_pTransport = pTransport;
        pInfo->m_ulPlayerCount = 1;

        pHandler = pInfo->m_pTransport->GetStreamHandler();

        m_pTransports->SetAt(pSub, (void *)pInfo);
        HXMutexUnlock(m_pTransMutex);

        memcpy(pInfo->m_bRuleOn, bRuleOn,
               RULE_TABLE_WIDTH * RULE_TABLE_HEIGHT * sizeof(BYTE));
        memset(pInfo->m_ulSequence, 0, 64 * sizeof(UINT32));

        if (pInfo->m_bAddrAllocated)
        {
            ulAddr = pInfo->m_ulAddress;
            return HXR_OK;
        }
        else
        {
            ulAddr = 0;
            return HXR_FAIL;
        }
    }
}

void
RTSPServerMulticastTransport::SetAddressOnTransport( RDTUDPTransport* pTransport, UINT32 ulAddress, UINT32 ulPort )
{
    //Create a IPV4 SockAddr object using networkservices.
    IHXSockAddr *pIAddress = 0;
    IHXNetServices *pNetSvc = 0;
    IHXBuffer* pAddrBuf = 0;
    m_pContext->QueryInterface(IID_IHXNetServices, (void**)&pNetSvc);

    IHXCommonClassFactory *pCCF = 0;
    m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&pCCF);

    if ( pNetSvc )
    {
	pNetSvc->CreateSockAddr( HX_SOCK_FAMILY_IN4, &pIAddress);
	if (pIAddress && pCCF)
	{
	    pCCF->CreateInstance(IID_IHXBuffer, (void**)&pAddrBuf);
	    if (pAddrBuf)
	    {
		in_addr sdestAddr;
		sdestAddr.s_addr = hx_ntohl(ulAddress);
		const UCHAR* pStrAddress = (const UCHAR*)hx_inet_ntoa( sdestAddr );
		if ( pStrAddress )
		{
		    pAddrBuf->Set( pStrAddress, strlen((const char*)pStrAddress) + 1 );

		    pIAddress->SetAddr(pAddrBuf);
    	    
		    pIAddress->SetPort((UINT16)ulPort );

		    //Set Address on transport.
		    pTransport->setPeerAddr(pIAddress);
		}
	    }
	}
    }

    HX_RELEASE(pAddrBuf);
    HX_RELEASE(pCCF);
    HX_RELEASE(pNetSvc);
    HX_RELEASE(pIAddress);

    return;
}


HX_RESULT
RTSPServerMulticastTransport::sendPacket(BasePacket* pPacket)
{
    HX_ASSERT(pPacket);

    CHXMapStringToOb::Iterator i;

    for (i = m_pTransports->Begin(); i != m_pTransports->End(); ++i)
    {
        RTSPMcastInfo* pInfo = (RTSPMcastInfo*)(*i);

        // XXXGo Don't send any packet if failed to allocate an addr
        if (pInfo->m_bAddrAllocated &&
            pInfo->m_bRuleOn
            [((ServerPacket *)pPacket)->m_uASMRuleNumber * RULE_TABLE_HEIGHT +
              pPacket->GetStreamNumber()])
        {
            pPacket->m_uSequenceNumber = (UINT16)
                pInfo->m_ulSequence[pPacket->GetStreamNumber()]++;

            if (pInfo->m_ulSequence[pPacket->GetStreamNumber()] >=
                pInfo->m_pTransport->wrapSequenceNumber())
            {
                pInfo->m_ulSequence[pPacket->GetStreamNumber()] = 0;
            }
            pInfo->m_pTransport->sendPacket(pPacket);
        }
    }
    return HXR_OK;
}

HX_RESULT
RTSPServerMulticastTransport::streamDone(UINT16 uStreamNumber,
        UINT32 uReasonCode /* = 0 */, const char* pReasonText /* = NULL */)
{
    CHXMapStringToOb::Iterator i;

    for (i = m_pTransports->Begin(); i != m_pTransports->End(); ++i)
    {
        RTSPMcastInfo* pInfo = (RTSPMcastInfo*)(*i);
        pInfo->m_pTransport->streamDone(uStreamNumber, uReasonCode, pReasonText);
    }
    return HXR_OK;
}

void
RTSPServerMulticastTransport::setSequenceNumber(UINT16 uStreamNumber,
    UINT16 ulSequenceNumber)
{
    /* Let the Resend Buffer worry about this */
}

BOOL
RTSPServerMulticastTransport::IsInitialized()
{
    return TRUE;
}

BOOL
RTSPServerMulticastTransport::IsUpdated()
{
    return TRUE;
}

BOOL
RTSPServerMulticastTransport::isNullSetup()
{
    return FALSE;
}

BOOL
RTSPServerMulticastTransport::isRTSPMulticast()
{
    return TRUE;
}

UINT32
RTSPServerMulticastTransport::wrapSequenceNumber()
{
    return TNG_WRAP_SEQ_NO;
}
