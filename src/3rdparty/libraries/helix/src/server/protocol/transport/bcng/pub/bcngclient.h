/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: bcngclient.h,v 1.7 2006/12/21 19:05:16 tknox Exp $
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
#ifndef _BCNGCLIENT_H_
#define _BCNGCLIENT_H_

#include "servrtsp_piids.h"

_INTERFACE IHXRTSPProtocol2;

// Response object for BCNGRTSPClient

DECLARE_INTERFACE_(IHXBCNGRTSPClientResponse, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXBCNGRTSPClientResponse
    STDMETHOD(OnError)              (THIS_ HX_RESULT status) PURE;
    STDMETHOD(InitDone)             (THIS_ HX_RESULT status) PURE;
    STDMETHOD(ConnectDone)          (THIS_ HX_RESULT status) PURE;
    STDMETHOD(OptionsDone)          (THIS_ HX_RESULT status) PURE;
    STDMETHOD(DescribeDone)         (THIS_ HX_RESULT status,
                                           IHXSDP* psdp) PURE;
    STDMETHOD(SetupDone)            (THIS_ HX_RESULT status,
                                           IHXMIMEHeader* pTranHdr) PURE;
    STDMETHOD(OnPacket)             (THIS_ IHXBuffer* pPkt) PURE;
};

DECLARE_INTERFACE_(IHXBCNGRTSPClient, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;
    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    // IHXBCNGRTSPClient
    STDMETHOD(Init)                     (THIS_ IHXBCNGRTSPClientResponse* pResponse,
                                               IUnknown* punkContext) PURE;
    STDMETHOD(Connect)                  (THIS_ const char* szHost, const char* szPort) PURE;
    STDMETHOD(SetParentProxy)           (THIS_ const char* szHost, const char* szPort) PURE;
    STDMETHOD(Options)                  (THIS_ IHXBuffer* pbufUrl) PURE;
    STDMETHOD(Describe)                 (THIS) PURE;
    STDMETHOD(Setup)                    (THIS_ IHXList* plistHeaders, UINT32 unStreamID) PURE;
    STDMETHOD(SetParam)                 (THIS_ const char* szKey, const char* szVal) PURE;
    STDMETHOD(Play)                     (THIS) PURE;
    STDMETHOD(Teardown)                 (THIS) PURE;
    STDMETHOD(Close)                    (THIS) PURE;

    STDMETHOD(SetTransportChannel)      (THIS_ BYTE ucChannel) PURE;
    STDMETHOD(SendPacket)               (THIS_ IHXBuffer* pPkt) PURE;
};


class CBCNGRTSPClient : public IHXRTSPProtocolResponse,
                        public IHXBCNGRTSPClient,
                        public Transport
{
public:
    CBCNGRTSPClient( IHXFastAlloc* pFastAlloc );
    virtual ~CBCNGRTSPClient( void );

    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    // IHXRTSPProtocolResponse
    STDMETHOD(OnConnectDone)            (THIS_ HX_RESULT status);
    STDMETHOD(OnClosed)                 (THIS_ HX_RESULT status);
    STDMETHOD(OnError)                  (THIS_ HX_RESULT status);
    STDMETHOD(OnPacket)                 (THIS_ IHXRTSPInterleavedPacket* pPkt);

    STDMETHOD(OnOptionsRequest)         (THIS_ IHXRTSPRequestMessage* pReq);
    STDMETHOD(OnDescribeRequest)        (THIS_ IHXRTSPRequestMessage* pReq);
    STDMETHOD(OnSetupRequest)           (THIS_ IHXRTSPRequestMessage* pReq);
    STDMETHOD(OnPlayRequest)            (THIS_ IHXRTSPRequestMessage* pReq);
    STDMETHOD(OnPauseRequest)           (THIS_ IHXRTSPRequestMessage* pReq);
    STDMETHOD(OnAnnounceRequest)        (THIS_ IHXRTSPRequestMessage* pReq);
    STDMETHOD(OnRecordRequest)          (THIS_ IHXRTSPRequestMessage* pReq);
    STDMETHOD(OnTeardownRequest)        (THIS_ IHXRTSPRequestMessage* pReq);
    STDMETHOD(OnGetParamRequest)        (THIS_ IHXRTSPRequestMessage* pReq);
    STDMETHOD(OnSetParamRequest)        (THIS_ IHXRTSPRequestMessage* pReq);
    STDMETHOD(OnRedirectRequest)        (THIS_ IHXRTSPRequestMessage* pReq);
    STDMETHOD(OnExtensionRequest)       (THIS_ IHXRTSPRequestMessage* pReq);

    STDMETHOD(OnOptionsResponse)        (THIS_ IHXRTSPResponseMessage* pRsp);
    STDMETHOD(OnDescribeResponse)       (THIS_ IHXRTSPResponseMessage* pRsp);
    STDMETHOD(OnSetupResponse)          (THIS_ IHXRTSPResponseMessage* pRsp);
    STDMETHOD(OnPlayResponse)           (THIS_ IHXRTSPResponseMessage* pRsp);
    STDMETHOD(OnPauseResponse)          (THIS_ IHXRTSPResponseMessage* pRsp);
    STDMETHOD(OnAnnounceResponse)       (THIS_ IHXRTSPResponseMessage* pRsp);
    STDMETHOD(OnRecordResponse)         (THIS_ IHXRTSPResponseMessage* pRsp);
    STDMETHOD(OnTeardownResponse)       (THIS_ IHXRTSPResponseMessage* pRsp);
    STDMETHOD(OnGetParamResponse)       (THIS_ IHXRTSPResponseMessage* pRsp);
    STDMETHOD(OnSetParamResponse)       (THIS_ IHXRTSPResponseMessage* pRsp);
    STDMETHOD(OnRedirectResponse)       (THIS_ IHXRTSPResponseMessage* pRsp);
    STDMETHOD(OnExtensionResponse)      (THIS_ IHXRTSPResponseMessage* pRsp);

    // IHXBCNGRTSPClient methods
    STDMETHOD(Init)                     (THIS_ IHXBCNGRTSPClientResponse* pResponse,
                                               IUnknown* punkContext);
    STDMETHOD(Connect)                  (THIS_ const char* szHost, const char* szPort);
    STDMETHOD(SetParentProxy)           (THIS_ const char* szHost, const char* szPort);
    STDMETHOD(Options)                  (THIS_ IHXBuffer* pbufUrl);
    STDMETHOD(Describe)                 (THIS);
    STDMETHOD(Setup)                    (THIS_ IHXList* plistHeaders, UINT32 unStreamID);
    STDMETHOD(SetParam)                 (THIS_ const char* szKey, const char* szVal);
    STDMETHOD(Play)                     (THIS);
    STDMETHOD(Teardown)                 (THIS);
    STDMETHOD(Close)                    (THIS);
    STDMETHOD(SetTransportChannel)      (THIS_ BYTE ucChannel);
    STDMETHOD(SendPacket)               (THIS_ IHXBuffer* pPkt);

    /* Transport methods (for TCP interleaved data)*/
    HX_RESULT handlePacket(IHXBuffer* pBuffer);

    HX_RESULT sendPacket(BasePacket* pPacket)    { /*NO-OP*/ return HXR_OK;}
    HX_RESULT sendPackets(BasePacket** pPacket)  { /*NO-OP*/ return HXR_OK;}
    HX_RESULT streamDone(UINT16 streamNumber,
                         UINT32 uReasonCode = 0,
                         const char* pReason = NULL)
                                                 { /*NO-OP*/ return HXR_OK;}
    HX_RESULT startPackets(UINT16 uStreamNumber) { /*NO-OP*/ return HXR_OK;}
    HX_RESULT stopPackets(UINT16 uStreamNumber)  { /*NO-OP*/ return HXR_OK;}
    HX_RESULT pauseBuffers(void)                 { /*NO-OP*/ return HXR_OK;}
    HX_RESULT resumeBuffers(void)                { /*NO-OP*/ return HXR_OK;}

    RTSPTransportTypeEnum tag(void)              { /*NO-OP*/return RTSP_TR_NONE;}
    void Reset(void)                             { /*NO-OP*/ }
    void Restart(void)                           { /*NO-OP*/ }
    STDMETHOD_(void, Done)      (THIS)           { /*NO-OP*/ }

private:
    IUnknown*                   m_punkContext;
    IHXCommonClassFactory*      m_pCCF;
    IHXBCNGRTSPClientResponse*  m_pResponse;
    IHXRTSPProtocol2*           m_pProtocol;
    IHXBuffer*                  m_pbufUrl;
    IHXBuffer*                  m_pbufSession;
    IHXBuffer*                  m_pbufChallengeString;
    IHXBufferedSocket*          m_pFastTCPSocket;
    IHXSocket*                  m_pTCPSocket;
    UINT32                      m_ulCSeq;
    IHXFastAlloc*               m_pFastAlloc;
    char*                       m_pszServerHost;
    char*                       m_pszServerPort;
    char*                       m_pszParentHost;
    char*                       m_pszParentPort;
};

#endif /* ndef _BCNGCLIENT_H_ */
