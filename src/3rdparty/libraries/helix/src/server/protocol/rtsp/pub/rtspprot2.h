/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspprot2.h,v 1.10 2007/08/18 00:21:16 dcollins Exp $
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
#ifndef _RTSPPROT2_H_
#define _RTSPPROT2_H_

#include "hxcomm.h"             // IHXFastAlloc
#include "hxrtspprot2.h"        // IHXRTSPProtocol2

class Transport;
_INTERFACE IHXTransport;

class CRTSPProtocol : public IHXRTSPProtocol2,
                      public IHXResolveResponse,
                      public IHXSocketResponse
{
public:
    CRTSPProtocol( IHXFastAlloc* pFastAlloc );
    ~CRTSPProtocol( void );

    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXRTSPPRotocol2
    STDMETHOD(Init)                 (THIS_ IHXRTSPProtocolResponse* pResponse,
                                           IUnknown* punkContext);
    STDMETHOD(GetSocket)            (THIS_ REF(IHXSocket*) pSock);
    STDMETHOD(SetTransport)         (THIS_ BYTE byChan, IHXTransport* pTran);
    STDMETHOD(Connect)              (THIS_ const char* szHost, const char* szPort);
    STDMETHOD(Accept)               (THIS_ IHXSocket* pSock);
    STDMETHOD(Close)                (THIS);
    STDMETHOD(SendRequest)          (THIS_ IHXRTSPRequestMessage* pReq);
    STDMETHOD(SendResponse)         (THIS_ IHXRTSPResponseMessage* pRsp);
    STDMETHOD(SendPacket)           (THIS_ IHXRTSPInterleavedPacket* pPkt);

    // IHXResolveResponse
    STDMETHOD(GetAddrInfoDone)      (THIS_ HX_RESULT status, UINT32 nVecLen, IHXSockAddr** ppAddrVec);
    STDMETHOD(GetNameInfoDone)      (THIS_ HX_RESULT status, const char* pszNode, const char* pszService);


    // IHXSocketResponse
    STDMETHOD(EventPending)         (THIS_ UINT32 uEvent, HX_RESULT status);

    STDMETHOD(ConnectDone)          (THIS_ HX_RESULT status);
    STDMETHOD(ReadDone)             (THIS_ HX_RESULT status, IHXBuffer* pbuf);
    STDMETHOD(WriteReady)           (THIS_ HX_RESULT status);
    STDMETHOD(Closed)               (THIS_ HX_RESULT status);

private:
    struct request_tag
    {
        UINT32   m_cseq;
        UINT32   m_verb;
    };

private:
    void DispatchMessage( void );

protected:
    ULONG32                     m_ulRefCount;
    IUnknown*                   m_punkContext;
    IHXCommonClassFactory*      m_pCCF;
    IHXRTSPProtocolResponse*    m_pResponse;
    IHXTransport*               m_ppTran[256];
    IHXSocket*                  m_pSock;
    IHXRTSPConsumer*            m_pConsumer;
    IHXBuffer*                  m_pbufFrag;
    UINT32                      m_cseqSend;     // Our next cseq
    UINT32                      m_cseqRecv;     // Peer's last cseq

    UINT32                      m_ntagcount;
    UINT32                      m_ntagalloc;
    request_tag*                m_ptags;
    IHXFastAlloc*               m_pFastAlloc;
    IHXScheduler*               m_pScheduler;
    IHXResolve*                 m_pResolver;
};

#endif /* ndef _RTSPPROT2_H_ */
