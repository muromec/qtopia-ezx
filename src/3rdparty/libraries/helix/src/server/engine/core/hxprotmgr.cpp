/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxprotmgr.cpp,v 1.8 2006/05/11 06:58:16 atin Exp $
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
#include "hxcomm.h"

#include "dict.h"
#include "debug.h"

#include "client.h"

#include "hxprotmgr.h"
#include "hxprot.h"

#include "aliveprot.h"
#include "rtsp_sniffer.h"
#include "http_sniffer.h"
#ifdef HELIX_FEATURE_SERVER_WMT_MMS
#include "mms_sniffer.h"
#endif

HXProtocolManager::HXProtocolManager(void)
{
    int n;
    for (n = 0; n < MAX_PROTOCOLS; n++)
    {
        m_sniffer_bindings[n].pt = HXPROT_UNKNOWN;
        m_sniffer_bindings[n].pSniffer = NULL;
        m_address_bindings[n].pt = HXPROT_UNKNOWN;
        m_address_bindings[n].pAddr = NULL;
    }
}

HXProtocolManager::~HXProtocolManager(void)
{
    int n;
    for (n = 0; n < MAX_PROTOCOLS; n++)
    {
        HX_RELEASE(m_address_bindings[n].pAddr);
        HX_DELETE(m_sniffer_bindings[n].pSniffer);
    }
}

HX_RESULT
HXProtocolManager::Init(void)
{
    RegisterProtocol(HXPROT_ALIVE,  new HXAliveSniffer());
    RegisterProtocol(HXPROT_RTSP,   new HXRTSPSniffer());
    RegisterProtocol(HXPROT_HTTP,   new HXHTTPSniffer());
#ifdef HELIX_FEATURE_SERVER_WMT_MMS
    RegisterProtocol(HXPROT_MMS,    new HXMMSSniffer());
#endif
    return HXR_OK;
}

HX_RESULT
HXProtocolManager::SetDefaultProtocol(HXProtocolType pt, IHXSockAddr* pAddr)
{
    int n;
    for (n = 0; n < MAX_PROTOCOLS; n++)
    {
        if (m_address_bindings[n].pt == HXPROT_UNKNOWN)
        {
            HX_ASSERT(m_address_bindings[n].pAddr == NULL);
            m_address_bindings[n].pt = pt;
            m_address_bindings[n].pAddr = pAddr;
            pAddr->AddRef();
            return HXR_OK;
        }
    }
    return HXR_FAIL;
}

HX_RESULT
HXProtocolManager::RegisterProtocol(HXProtocolType pt,
                HXProtocolSniffer* pSniffer)
{
    int n;
    for (n = 0; n < MAX_PROTOCOLS; n++)
    {
        if (m_sniffer_bindings[n].pt == HXPROT_UNKNOWN)
        {
            HX_ASSERT(m_sniffer_bindings[n].pSniffer == NULL);
            m_sniffer_bindings[n].pt = pt;
            m_sniffer_bindings[n].pSniffer = pSniffer;
            return HXR_OK;
        }
    }
    return HXR_FAIL;
}

HX_RESULT
HXProtocolManager::MatchProtocol(IHXSockAddr* pLocalAddr, IHXBuffer* pBuf,
                REF(HXProtocolType) pt)
{
    int n;
    for (n = 0; n < MAX_PROTOCOLS; n++)
    {
        if (m_sniffer_bindings[n].pt != HXPROT_UNKNOWN)
        {
            if (m_sniffer_bindings[n].pSniffer->Match(pBuf))
            {
                pt = m_sniffer_bindings[n].pt;
                return HXR_OK;
            }
        }
    }

    return HXR_FAIL;
}

HX_RESULT
HXProtocolManager::CreateProtocol(CHXServSocket* pSock,
                HXProtocolType pt, REF(HXProtocol*) pProtocol)
{
    int n;
    for (n = 0; n < MAX_PROTOCOLS; n++)
    {
        if (m_sniffer_bindings[n].pt == pt)
        {
            pProtocol = m_sniffer_bindings[n].pSniffer->Create(pSock);
            return HXR_OK;
        }
    }
    return HXR_FAIL;
}

HXSocketConnection::HXSocketConnection(IHXSocket* pSock, HXProtocolType pt) :
    m_nRefCount(0),
    m_pSock((CHXServSocket*)pSock),
    m_pt(pt),
    m_pProtocol(NULL)
{
    m_pSock->AddRef();

    m_pSock->SetResponse(this);
    m_pSock->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE);
}

HXSocketConnection::~HXSocketConnection(void)
{
    m_pProtocol = NULL; // NB: We didn't AddRef this
    HX_RELEASE(m_pSock);
}

STDMETHODIMP
HXSocketConnection::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXSocketResponse*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSocketResponse))
    {
        AddRef();
        *ppvObj = (IHXSocketResponse*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
HXSocketConnection::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
HXSocketConnection::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP
HXSocketConnection::EventPending(UINT32 uEvent, HX_RESULT status)
{
    HX_RESULT hr = HXR_OK;
    IHXBuffer* pBuf = NULL;

    AddRef();

    if (m_pSock == NULL)
    {
        HX_ASSERT(FALSE);
        Release();
        return HXR_UNEXPECTED;
    }
    switch (uEvent)
    {
    case HX_SOCK_EVENT_READ:

        // NB: We have to Peek() even if the protocol is known.  The act of
        //     reading is necessary to synthesize the CLOSE event.
        hr = m_pSock->Peek(&pBuf);
        if (SUCCEEDED(hr) && pBuf)
        {
            if (m_pt == HXPROT_UNKNOWN)
            {
                IHXSockAddr* pAddr = NULL;
                m_pSock->GetLocalAddr(&pAddr);
                g_pProtMgr->MatchProtocol(pAddr, pBuf, m_pt);
                HX_RELEASE(pAddr);
            }
            if (m_pt == HXPROT_UNKNOWN)
            {
                // Match failed
                m_pSock->Close();
		HX_RELEASE(pBuf);
                break;
            }

            g_pProtMgr->CreateProtocol(m_pSock, m_pt, m_pProtocol);
            if (m_pProtocol == NULL)
            {
                // This probably shouldn't happen
                HX_ASSERT(FALSE);
                m_pSock->Close();
		HX_RELEASE(pBuf);
                Release();
                return HXR_OK;
            }
        }
        else
        {
            m_pSock->Close();
            HX_RELEASE(m_pSock);
        }
        HX_RELEASE(pBuf);
        break;
    case HX_SOCK_EVENT_CLOSE:
	m_pSock->Close();
        HX_RELEASE(m_pSock);
        break;
    case HX_SOCK_EVENT_DISPATCH:
        if (SUCCEEDED(status))
        {
            // The protocol should take the socket response which will release
            // our only ref.
            m_pProtocol->init(m_pSock->GetProc(), m_pSock);
        }
        else
        {
            //XXXTDM: How can this fail?  Which proc are we in?
            HX_ASSERT(FALSE);
            m_pSock->Close();
        }
        break;
    default:
        HX_ASSERT(FALSE);
    }
    Release();
    return HXR_OK;
}
