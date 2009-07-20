/* ***** BEGIN LICENSE BLOCK *****
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

#ifndef _RTP_UDP_H_
#define _RTP_UDP_H_

#include "rtp_base.h"

/*
*   NOTE:
*   Each RTP transport represents a RTP session because transport is bounded to a pair
*   of addr and ports.

*   As it is currently implemented, RTP transport can NOT have more than one stream.

*   Currently, we do NOT support RTSP/RTP multicast.  so we don't need to do a number
*   of things that should be done if we were to support multicast.
*   1. Maintain a table of members (instread we have just one member)
*   2. Don't really need to calculate RTCP intervals (instead every 5 sec)

*   There are two assumptions due to the nature of 1 to 1 or 1 to many session
*   1. A server (server) never receives RTP.
*   2. There is only one sender (server) in a session.
*/

class ServerRTPUDPTransport : public ServerRTPBaseTransport
                            , public IHXSocketResponse
{
public:
    ServerRTPUDPTransport               (BOOL bIsSource);
    ~ServerRTPUDPTransport              ();

    /* IUnknown: */
    STDMETHOD(QueryInterface)           (THIS_
                                         REFIID riid,
                                         void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)		(THIS);
    STDMETHOD_(ULONG32,Release)		(THIS);

    /* IHXSocketResponse */
    STDMETHOD(EventPending)             (THIS_ UINT32 uEvent, HX_RESULT status);

    // RTSPTransport
    STDMETHOD_(void, Done)              (THIS);
    virtual RTSPTransportTypeEnum tag   (void);
    virtual HX_RESULT sendPacket        (BasePacket* pPacket);
    virtual void JoinMulticast          (IHXSockAddr* pAddr, IHXSocket* pSocket = 0);
    HX_RESULT init                      (IUnknown* pContext,
                                         IHXSocket* pSocket,
                                         IHXRTSPTransportResponse* pResp);

    void setPeerAddr                    (IHXSockAddr* pPeerAddr);

    /* XXXMC
     * Special-case handling for PV clients
     */
    HX_RESULT sendPVHandshakeResponse(UINT8* pPktPayload);

protected:
    IHXSocket*                          m_pUDPSocket;
    IHXSockAddr*                        m_pPeerAddr;

private:

    HX_RESULT writePacket               (IHXBuffer* pSendBuffer);

    IHXSockAddr*                        m_pMulticastAddr;
    IHXMulticastSocket*                 m_pMulticastSocket;

    friend class ServerRTCPBaseTransport;
    friend class ServerRTCPUDPTransport;

    friend class CUTServerRTPUDPTransportTestDriver;  // For unit testing
};

class ServerRTCPUDPTransport : public ServerRTCPBaseTransport
                             , public IHXSocketResponse
{
public:
    ServerRTCPUDPTransport              (BOOL bIsSource);
    ~ServerRTCPUDPTransport             ();

    /* IUnknown: */
    STDMETHOD(QueryInterface)           (THIS_
                                         REFIID riid,
                                         void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)		(THIS);
    STDMETHOD_(ULONG32,Release)		(THIS);

    /* IHXSocketResponse */
    STDMETHOD(EventPending)             (THIS_ UINT32 uEvent, HX_RESULT status);

    STDMETHOD_(void, Done)              (THIS);
    virtual RTSPTransportTypeEnum tag   (void);
    virtual HX_RESULT streamDone        (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL);

    HX_RESULT init                      (IUnknown* pContext,
                                         IHXSocket* pSocket,
                                         ServerRTPBaseTransport* pDataTransport,
                                         IHXRTSPTransportResponse* pResp,
                                         UINT16 streamNumber);
    void setPeerAddr                    (IHXSockAddr* pPeerAddr);

    virtual void JoinMulticast          (IHXSockAddr* pAddr, IHXSocket* pSocket = 0);

protected:
    IHXSocket*      m_pUDPSocket;
    IHXSockAddr*    m_pPeerAddr;

private:

    HX_RESULT reflectRTCP               (IHXBuffer* pSendBuf);
    HX_RESULT sendSenderReport          (void);
    HX_RESULT sendBye                   (void);


    IHXMulticastSocket*     m_pMulticastSocket;
    IHXSockAddr*            m_pMulticastAddr;

    friend class ServerRTPBaseTransport;
    friend class ServerRTPUDPTransport;
};

#endif /* _RTP_UDP_H_ */
