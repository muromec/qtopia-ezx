/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id:
 *
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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

/****************************************************************************
 *  Includes  
 */
#include "hxcom.h"
#include "hxnet.h"
#include "hxmap.h"
#include "hxslist.h"
#include "rtspif.h"
#include "rtsptran.h"
#include "rtp_base.h"
#include "rtp_udp.h"
#include "bufnum.h"
#include "rtpwrap.h"
#include "pkthndlr.h"
#include "rtcp_demux.h"


ServerRTCPDemux::ServerRTCPDemux()
    : m_pTransMap(NULL)
    , m_ulRefCount(0)
    , m_spSock()
    , m_spPeerAddr()
{
    m_pTransMap = new CHXMapLongToObj;
}


ServerRTCPDemux::~ServerRTCPDemux()
{
    HX_DELETE(m_pTransMap);
}


STDMETHODIMP
ServerRTCPDemux::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


STDMETHODIMP_(UINT32)
ServerRTCPDemux::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}


STDMETHODIMP_(UINT32)
ServerRTCPDemux::Release()
{
    if(InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
ServerRTCPDemux::EventPending(UINT32 uEvent, HX_RESULT status)
{
    HX_ASSERT(m_spSock.IsValid() && m_spPeerAddr.IsValid());
    SPIHXBuffer spBuf;
    SPIHXSockAddr spFromAddr;
    UINT32 uSSRC = 0;

    switch (uEvent)
    {
    case HX_SOCK_EVENT_READ:
        if (SUCCEEDED(m_spSock->ReadFrom(spBuf.AsInOutParam(), spFromAddr.AsInOutParam()))
            && m_spPeerAddr.IsValid() && spFromAddr.IsValid()
            && m_spPeerAddr->IsEqualAddr(spFromAddr.Ptr()))
        {
            HandlePacket(spBuf);
        }

        break;

    case HX_SOCK_EVENT_CLOSE:
        HX_ASSERT(FALSE);
        break;

    default:
        HX_ASSERT(FALSE);
    }

    return HXR_OK;
}


STDMETHODIMP
ServerRTCPDemux::HandlePacket(SPIHXBuffer &spBuffer)
{
    UINT32 uSrcSSRC = 0;
    BOOL   bFoundSrcSSRC = FALSE;
    HX_RESULT hxr = HXR_OK;

    RTCPUnPacker unpacker;
    if (FAILED(unpacker.UnPack(spBuffer.Ptr())))
    {
        return HXR_FAIL;
    }

    RTCPPacket* pPkt = NULL;
    CHXSimpleList pktList;
    while (SUCCEEDED(unpacker.Get(pPkt)))
    {
        if (pPkt->packet_type == 201 && pPkt->count > 0)
        {
            uSrcSSRC = pPkt->rr_data[0].ssrc;
            bFoundSrcSSRC = TRUE;
        }
        pktList.AddTail(pPkt);
    }
    
    void* pTrans;
    if (bFoundSrcSSRC && m_pTransMap->Lookup(uSrcSSRC, (void*&)pTrans))
    {
            hxr = ((ServerRTCPBaseTransport*)pTrans)->handlePacketList(&pktList, spBuffer.Ptr());
        }
    else
    {
        // no matching transport!
        while (!pktList.IsEmpty())
        {
            pPkt = (RTCPPacket*)pktList.RemoveHead();
            HX_DELETE(pPkt);
        }
        hxr = HXR_FAIL;
    }

    return hxr;
}


STDMETHODIMP
ServerRTCPDemux::AddTransport(UINT32 uSSRC, ServerRTCPBaseTransport* pTrans)
    {
    (*m_pTransMap)[uSSRC] = pTrans;

    return HXR_OK;
}


STDMETHODIMP
ServerRTCPDemux::SetSocket(IHXSocket* pSock)
{
    if (!pSock)
    {
        return HXR_FAIL;
    }

    m_spSock = pSock;
    m_spSock->SetResponse(this);
   
    return HXR_OK;
}


STDMETHODIMP
ServerRTCPDemux::SetPeerAddr(IHXSockAddr* pAddr)
{
    HX_ASSERT(pAddr != NULL);
    m_spPeerAddr = pAddr;

    return HXR_OK;
}
