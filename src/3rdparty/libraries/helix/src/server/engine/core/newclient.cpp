/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: newclient.cpp,v 1.13 2007/06/19 01:57:36 seansmith Exp $ 
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxstring.h"

#include "debug.h"

#include "tcpio.h"
#include "sio.h"
#include "cbqueue.h"

#include "rbpc.h"
#include "rttp2.h"
#include "rttp2sn.h"

#include "srcerrs.h"
#include "proc.h"
#include "proc_container.h"
#include "core_proc.h"
#include "core_container.h"
#include "conn_dispatch.h"
#include "hxstrutl.h"
#include "hxprot.h"
#include "cbqueue.h"
#include "hxslist.h"
#include "mimehead.h"
#include "rtsputil.h"
#include "rtspmsg.h"
#include "rtspmdsc.h"
#include "rtsppars.h"
#include "rtspserv.h"
#include "rtspif.h"
#include "httpprot.h"
#include "http.h"
#include "client.h"
#include "server_info.h"
#include "server_engine.h"
#include "base_errmsg.h"
#include "netbyte.h"
#include "servchallenge.h"

#include "cloaked_httpprot.h"
#include "qt_cloaked_httpprot.h"
#include "newclient.h"
#ifdef HELIX_FEATURE_SERVER_WMT_MMS
#include "newclient_wmt.h"
#endif
#include "cloaksync.h"
#include "qt_cloaksync.h"

#ifdef PAULM_NCPTIMING
#include "classtimer.h"
ClassTimer g_NCPTimer("NewClientProtocol",
	0, 3600);
#endif

static UINT16 MAX_Q_MSG = 4096;
static UINT16 REAL_MAX_Q_MSG = 65500;

NewClientProtocol::NewClientProtocol(Client* pClient):
    HXProtocol(pClient),
    m_lRefCount(0),
    m_pSwitchProtocol(0),
    m_bClientAlreadyConnected(FALSE)
{
#ifdef PAULM_NCPTIMING
    g_NCPTimer.Add(this);
#endif

    m_pState = NewClientInitState::instance();
    setNRequired(4);
    m_pInQueue = new CByteQueue(MAX_Q_MSG);
    m_pInQueue->SetMaxSize(REAL_MAX_Q_MSG);
}

NewClientProtocol::~NewClientProtocol()
{
#ifdef PAULM_CLIENTAR
    REL_NOTIFY(m_pClient, 1);
#endif
#ifdef PAULM_NCPTIMING
    g_NCPTimer.Remove(this);
#endif
    if (m_pSwitchProtocol)
	HX_RELEASE(m_pSwitchProtocol);

    if (m_pInQueue)
	delete m_pInQueue;
}

STDMETHODIMP_(UINT32)
NewClientProtocol::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
NewClientProtocol::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

void
NewClientProtocol::init()
{
    m_pClient->m_pCtrl->Read(m_nRequired);
}

void 
NewClientProtocol::setAlreadyReadData(Byte* pAlreadyReadData, 
                                      UINT32 alreadyReadDataLen)
{
    if (pAlreadyReadData && alreadyReadDataLen)
	m_pInQueue->EnQueue(pAlreadyReadData, HX_SAFEUINT16(alreadyReadDataLen));
}

INT32 
NewClientProtocol::handleInput(IHXBuffer* pMsg)
{
    BYTE* pData = 0;
    UINT32 ulMsgLen = 0;

    if (pMsg && pMsg->GetSize() > 0)
    {
	pData = pMsg->GetBuffer();
	ulMsgLen = pMsg->GetSize();
    }
    if (pData && ulMsgLen > 0)
    {
	if (!m_pInQueue->EnQueue(pData, (UINT16)ulMsgLen))
	{
	    if (!m_pInQueue->Grow((UINT16)ulMsgLen))
	    {
		return -1;
	    }
	    m_pInQueue->EnQueue(pData, (UINT16)ulMsgLen);
	}
    }

    int ret = 0;
    UINT32 bytesAvail = m_pInQueue->GetQueuedItemCount();
    if (bytesAvail == 0)
    {
	return 0;
    }

    Byte* pBuf = new Byte[bytesAvail];

    for(;;)
    {
	m_pInQueue->DeQueue(pBuf, (UINT16)bytesAvail);

	// if m_nRequired is -1, pass everything we've got...
	if (-1 == m_nRequired)
	{
	    m_nRequired = bytesAvail;
	}
	
	UINT32 bytesUsed = m_nRequired;

	if(bytesUsed > bytesAvail)
        {
            DPRINTF(D_INFO, ("%lu: got %lu, wanted %d bytes\n",
                            m_pClient->conn_id, bytesAvail, m_nRequired));
	    m_pInQueue->EnQueue(pBuf, (UINT16)bytesAvail);
            ret = 0;
            break;
        }

        ret = m_pState->handleInput(pBuf, m_nRequired, this);
	m_pInQueue->EnQueue(pBuf, (UINT16)bytesAvail);

        if(ret == 0)
            continue;
        else if(ret > 0 && !m_bClientAlreadyConnected)
	{
	    m_pClient->m_pCtrl->disableRead();

	}
	else if(ret == -2)
	{
	    DPRINTF(D_INFO, ("NCP::hI -- switching to %s\n", 
		    m_pSwitchProtocol->versionStr()));
	    m_pInQueue->DeQueue(pBuf, (UINT16)bytesAvail);
	    m_pSwitchProtocol->setAlreadyReadData(pBuf, bytesAvail);
	    m_pClient->setProtocol(m_pSwitchProtocol);
            m_pSwitchProtocol->SetupProtocol();

	    m_pSwitchProtocol->Release();
	    m_pSwitchProtocol = 0;
	}

        break;
    }
    delete pBuf;

    return ret;
}

// static defines
NewClientInitState*	NewClientInitState::m_pState = 0;
NewClientRTSPState*     NewClientRTSPState::m_pState = 0;
NewClientHTTPState*     NewClientHTTPState::m_pState = 0;
NewClientIsCloakingState* NewClientIsCloakingState::m_pState = 0;
NewClientIsQTCloakingState* NewClientIsQTCloakingState::m_pState = 0;
NewClientAlivePingState* NewClientAlivePingState::m_pState = 0;

NewClientRBPState*      NewClientRBPState::m_pState = 0;

void
NewClientState::DispatchClient(Client* pClient)
{
#ifndef _SOLARIS
    CorePassClientCallback* cb = new CorePassClientCallback;
    cb->c    = pClient;
#if ENABLE_LATENCY_STATS
    pClient->TCorePassCB();
#endif
    pClient->LeavingCore();
    pClient->disconnect();
    cb->proc = pClient->m_pProc;
    pClient->m_pProc->pc->engine->ischedule.enter(0.0, cb);
#else
    pClient->LeavingCore();
    pClient->disconnect();
    ((CoreContainer *)pClient->m_pProc->pc)->cdispatch->Send(pClient);
#endif
}

int 
NewClientInitState::handleInput(Byte* pMsg,
    int nMsgLen, NewClientProtocol* pProt)
{
    int		    len = 0;
    Client*         pClient = pProt->client();


    if (!memcmp (pMsg, "!!", 2))
    {
	pProt->setState(NewClientAlivePingState::instance());
	pProt->setNRequired(1);

	if (!pProt->isClientAlreadyConnected())
	{
	    DispatchClient(pClient);
	    return 1;
	}

	return 0;
    }
    else if ((memcmp(pMsg, "ANNO", 4) == 0)
	|| (memcmp(pMsg, "DESC", 4) == 0)
	|| (memcmp(pMsg, "GET_", 4) == 0)
	|| (memcmp(pMsg, "OPTI", 4) == 0)
	|| (memcmp(pMsg, "PAUS", 4) == 0)
	|| (memcmp(pMsg, "PLAY", 4) == 0)
	|| (memcmp(pMsg, "RECO", 4) == 0)
	|| (memcmp(pMsg, "REDI", 4) == 0)
	|| (memcmp(pMsg, "SETU", 4) == 0)
	|| (memcmp(pMsg, "SET_", 4) == 0)
	|| (memcmp(pMsg, "TEAR", 4) == 0))
    {
        if ((pClient->m_protType == Client::UNKNOWN_PROT) || 
	    (pClient->m_protType == Client::RTSP_PROT))
        {
	    pProt->setState(NewClientRTSPState::instance());
	    pProt->setNRequired(15);

	    if (!pProt->isClientAlreadyConnected())
	    {
	        DispatchClient(pClient);
	        return 1;
	    }

	    return 0;
        }
        else
        {
            return -1;
        }
    }
    else if((strncasecmp((char*)pMsg, "GET", 3) == 0)
	    || (strncasecmp((char*)pMsg, "PUT", 3) == 0)
	    || (strncasecmp((char*)pMsg, "POST", 4) == 0))
    {
        if ((pClient->m_protType == Client::UNKNOWN_PROT) || 
	    (pClient->m_protType == Client::HTTP_PROT))
        {
	    DPRINTF(D_INFO, ("NewClient: start HTTP state\n"));

	    pProt->setState(NewClientIsCloakingState::instance());
	    pProt->setNRequired(16);
	    return 0;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        UINT32 opcode = (*pMsg&0x7f)<<8; 
	opcode |= pMsg[1];
	if(opcode == RBPC_HELLO || opcode == RTTPC_HELLO)
	{
	    Header	    rbp_header;
	    rbp_header.unpack(pMsg, nMsgLen);
	    if (rbp_header.opcode == RBPC_HELLO)
	    {
		pProt->setState(NewClientRBPState::instance());
    		pProt->setNRequired(nMsgLen + rbp_header.length);
	    }
	    else if (rbp_header.opcode == RTTPC_HELLO)
	    {
		RTTPSN_stream_status	err_msg;
		Byte			buf[DEFAULT_SMALL_RBP_BUF_SIZE];
		err_msg.init(buf, DEFAULT_SMALL_RBP_BUF_SIZE);
		err_msg.set(RTTPSN_ST_ERROR, 
			"The encoder you are using is obsolete. "
	    		" You should obtain a new encoder from "
			"http://www.real.com/");
		if (-1 == pClient->output(err_msg.msg, err_msg.msg_len))
		{
		    DPRINTF(D_INFO, ("%lu: failed to send RBPC error msg\n",
			    pClient->conn_id));
		}
		return -1;
	    }
	}
        else
        {
            // It might be from the Windows media load tool -
            if ((pClient->m_protType == Client::UNKNOWN_PROT) || 
		(pClient->m_protType == Client::HTTP_PROT))
            {
                pProt->setState(NewClientIsCloakingState::instance());
                pProt->setNRequired(15);
                return 0;
            }
            else
            {
                return -1;
            }
        }
    }

    return 0;
}

int NewClientRBPState::handleInput(Byte* pMsg, int nMsgLen, 
    NewClientProtocol* pProt)
{
    RBPC_hello      hello_msg;
    int             msg_len;
    Int16           type = 0;
    Client*         pClient = pProt->client();

    hello_msg.unpack((Byte*)pMsg, nMsgLen);

    ERRMSG(pClient->m_pProc->pc->error_handler,
           "%ld: RTTP entity %d not implemented\n",
           pClient->conn_id, type);
    return -1;

#if 0 //XXX this has been disabled for a long long time, delete this?

    Byte    msg[DEFAULT_SMALL_RBP_BUF_SIZE];
    hello_msg.major_version = RBP_MAJOR_VERSION;
    hello_msg.minor_version = RBP_MINOR_VERSION;
    hello_msg.id.len = 0;
    hello_msg.id.data = 0;
    msg_len = DEFAULT_SMALL_RBP_BUF_SIZE;
    hello_msg.pack(msg, msg_len);

    if (pClient->output(msg, msg_len) != msg_len)
    {
	ERRMSG(pClient->m_pProc->pc->error_handler,
	       "Write failed in newclient_rbp_hello\n");
        return -1;
    }
    return 0;
#endif
}


/*  NewClientRTSPState has been moved to newclient_rtsp.cpp for tactical
    reasons : we want it to differ in server and proxy and don't want
    major cahnges to the build system. Thank you for your patience!
 */

int
NewClientIsCloakingState::handleInput(Byte* pMsg, int nMsgLen,
				      NewClientProtocol* pProt)
{
    /*
     * Decide here if it is regular http and we can pass it
     * off to a streamer, or if it is cloaking, and we need
     * to keep going till we get a guid.
     */
    Client*         pClient = pProt->client();

    /*
     * See if it is a RM cloaked
     */             
    if((strncasecmp((const char*)pMsg, "GET /SmpDsBhgRl", 15) == 0) ||
       (strncasecmp((const char*)pMsg, "POST /SmpDsBhgRl", 16) == 0))
    {
#ifdef PAULM_CLIENTAR
	ADDR_NOTIFY(pClient, 7);
#endif
	CloakSyncProtocol* newProt = new CloakSyncProtocol(pClient);
	newProt->AddRef();
	newProt->Init();
	pProt->setSwitchProtocol(newProt);

	return -2;  // switch protocols
    }

    // read in all!!!
    pProt->setState(NewClientIsQTCloakingState::instance());
    pProt->setNRequired(-1);
    return 0;
}

int
NewClientIsQTCloakingState::handleInput(Byte* pMsg, int nMsgLen,
				      NewClientProtocol* pProt)
{
    /*
     * Decide here if it is regular http and we can pass it
     * off to a streamer, or if it is cloaking, and we need
     * to keep going till we get a guid.
     */
    Client*         pClient = pProt->client();

#ifdef HELIX_FEATURE_SERVER_WMT_MMS
    if (IsWMTHTTP(pMsg, nMsgLen, pClient))
    {
        // From a WMT player/server/load simulator
        pProt->setState(NewClientWMTHTTPState::instance());
        pProt->setNRequired(-1);

        if (!pProt->isClientAlreadyConnected())
        {
            pClient->m_bNeedToJumpStart = 1;
            DispatchClient(pClient);
            return 1;
        }

        return 0;
    }
#endif

    /*
     * See if it is a QT cloaked.
     */
    const char nugget[] = "application/x-rtsp-tunnelled";
    if ((nMsgLen > sizeof(nugget)) && StrNStr((const char*)pMsg, nugget, nMsgLen, sizeof(nugget)))
    {
	// this is the only way we know to identify that this is from QTP, and 
	// we'll assume this is cloaking.
#ifdef PAULM_CLIENTAR
	ADDR_NOTIFY(pClient, 7);
#endif
	CloakSyncProtocolQT* newProt = new CloakSyncProtocolQT(pClient);
	newProt->AddRef();
	newProt->Init();
	pProt->setSwitchProtocol(newProt);

	return -2;  // switch protocols
    }

    /*
     * We know it is not cloaking.
     */
    pProt->setState(NewClientHTTPState::instance());

    if (!pProt->isClientAlreadyConnected())
    {
	pClient->m_bNeedToJumpStart = 1;
	DispatchClient(pClient);
	return 1;
    }

    return -1;
}

int
NewClientHTTPState::handleInput(Byte* pMsg, int nMsgLen,
    NewClientProtocol* pProt)
{
    Client* pClient = pProt->client();
    DPRINTF(D_INFO, ("NCHTTPS: hI -- %*.*s\n", nMsgLen, nMsgLen, pMsg));

    if((strncasecmp((const char*)pMsg, "GET", 3) == 0) ||
            (strncasecmp((const char*)pMsg, "PUT", 3) == 0) ||
            (strncasecmp((const char*)pMsg, "POST", 4) == 0))
    {
	pClient->role = new HTTP(pClient);
	HTTPProtocol* newProt = new HTTPProtocol(pClient);
	newProt->AddRef();
	pProt->setSwitchProtocol(newProt);

	return -2;  // switch protocols
    }
    else
    {
	ERRMSG(pClient->m_pProc->pc->error_handler,
	       "%ld: Unrecognized protocol\n", pClient->conn_id);
	return -1;
    }

}

int NewClientAlivePingState::handleInput(Byte* pMsg, int nMsgLen, 
    NewClientProtocol* pProt)
{
    Client* 	pClient = pProt->client();

    if (nMsgLen < 1)
    {
	return -1;
    }

    if (pMsg[0] != '!')
    {
	return -1;
    }

    const char pMessage[] = "200 Looks like I'm fine!\r\n";
    pClient->output((unsigned char *)pMessage, sizeof(pMessage) - 1);

    return -1;
}
