/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtp_tcp.h,v 1.11 2007/03/22 19:16:57 tknox Exp $
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

#ifndef _RTP_TCP_H_
#define _RTP_TCP_H_

#include "rtp_base.h"
#include "sink.h"

_INTERFACE IHXServerPacketSink;
_INTERFACE IHXWouldBlockResponse;

class ServerPacket;
class IHXServerPacketSource;

class ServerRTPTCPTransport : public ServerRTPBaseTransport
                            , public IHXWouldBlock
                            , public IHXSocketResponse
{
public:
    ServerRTPTCPTransport               (BOOL bIsSource);
    virtual ~ServerRTPTCPTransport      ();

    /* IUnknown: */
    STDMETHOD(QueryInterface)           (THIS_
                                         REFIID riid,
                                         void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    /* IHXServerPacketSink */
    STDMETHOD(PacketReady) (ServerPacket* pPacket);
    STDMETHOD(SetSource) (IHXServerPacketSource* pSource);
    STDMETHOD(Flush) ();
    STDMETHOD(SourceDone) ();

    /* IHXWouldBlock */
    STDMETHOD(WantWouldBlock)   (THIS_
                                 IHXWouldBlockResponse* pResp,
                                 UINT32 ulId);

    /* IHXSocketResponse */
    STDMETHOD(EventPending)     (THIS_ UINT32 uEvent, HX_RESULT status);

    STDMETHOD_(void, Done)              (THIS);

    RTSPTransportTypeEnum tag           (void);
    HX_RESULT init                      (IUnknown* pContext,
                                        IHXSocket* pSocket,
                                        IHXRTSPTransportResponse* pResp);

    void setInterleaveChannel           (INT8 tcpInterleave)
    {
        m_tcpInterleave = tcpInterleave;
    }
    HX_RESULT sendPacket                (BasePacket* pPacket);

protected:
    IHXSocket*                          m_pTCPSocket;
    INT8                                m_tcpInterleave;
    HX_RESULT writePacket               (IHXBuffer* pBuf);

    friend class ServerRTCPBaseTransport;
    friend class ServerRTCPTCPTransport;

    friend class CUTServerRTPTCPTransportTestDriver; // for unit testing

private:
    /* Server Media Delivery Pipeline: */
    IHXServerPacketSource*              m_pSource;

    /* TCP write handling: */
    IHXWouldBlockResponse*              m_pWouldBlockResponse;
    BOOL                                m_bBlocked;
};

class ServerRTCPTCPTransport: public ServerRTCPBaseTransport
{
public:
    ServerRTCPTCPTransport              (HXBOOL bIsSender);
    ~ServerRTCPTCPTransport             (void);
    STDMETHOD_(void, Done)              (THIS);

    HX_RESULT init                      (IUnknown* pContext,
                                        IHXSocket* pSocket,
                                        ServerRTPTCPTransport* pDataTransport,
                                        IHXRTSPTransportResponse* pResp,
                                        UINT16 streamNumber);
    void setInterleaveChannel           (INT8 tcpInterleave)
    {
        m_tcpInterleave = tcpInterleave;
    }
    RTSPTransportTypeEnum tag           (void);
    HX_RESULT streamDone                (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL);

private:
    HX_RESULT reflectRTCP               (IHXBuffer* pSendBuf);
    HX_RESULT sendSenderReport          (void);
    HX_RESULT sendBye                   (void);
    HX_RESULT writePacket               (IHXBuffer* pBuf);

    IHXSocket*                          m_pTCPSocket;
    INT8                                m_tcpInterleave;
//    friend class ServerRTPBaseTransport;
//    friend class ServerRTPTCPTransport;
    friend class CUTServerRTCPTCPTransportTestDriver; // for unit testing
};

#endif /* _RTP_TCP_H_ */
