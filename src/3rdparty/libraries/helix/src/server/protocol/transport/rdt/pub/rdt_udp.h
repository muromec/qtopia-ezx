/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rdt_udp.h,v 1.4 2007/01/06 00:12:04 seansmith Exp $
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

#ifndef _RDT_UDP_H_
#define _RDT_UDP_H_

#include "hxcom.h"
#include "hxcomptr.h"

#include "rdt_base.h"       // For RDTBaseTransport
#include "hxservpause.h"    // For IHXServerPauseAdvise

_INTERFACE IHXScheduler;
_INTERFACE IHXBuffer;
_INTERFACE IHXQoSSignalBus;
_INTERFACE IHXSocket;
_INTERFACE IHXRTSPTransportResponse;
_INTERFACE IHXServerPacketSource;
_INTERFACE IHXSockAddr;

class TNGLatencyReportPacket;
class QoSRDTMetrics;
class SeekCallback;
class TransportInfoCallback;

class RDTUDPTransport: public RDTBaseTransport,
                       public IHXServerPauseAdvise
{
public:
    RDTUDPTransport                     (BOOL bIsServer, BOOL bIsSource,
                                         RTSPTransportTypeEnum lType,
                                         UINT32 uFeatureLevel = 0,
                                         BOOL bDisableResend = FALSE);
    virtual ~RDTUDPTransport            (void);

    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)		(THIS);
    STDMETHOD_(ULONG32,Release)		(THIS);

    STDMETHOD_(void, Done)              (THIS);
    virtual void Reset                  (void);
    virtual void Restart                (void);
    virtual void addStreamInfo          (RTSPStreamInfo* pStreamInfo,
                                         UINT32 ulBufferDepth = TRANSPORT_BUF_DURATION_UNDEF);
    virtual HX_RESULT sendPacket        (BasePacket* pPacket);
    virtual HX_RESULT handlePacket      (IHXBuffer* pBuffer);
    virtual HX_RESULT streamDone        (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL);

    // BCM
    virtual TransportStreamHandler* GetStreamHandler(void);
    virtual void SetStreamHandler       (TransportStreamHandler* pHandler);
    virtual void MulticastSetup         (TransportStreamHandler* pHandler);
    virtual void JoinMulticast          (IHXSockAddr* pAddr, IHXSocket* pSock);
    virtual BOOL isBCM                  (void) { return m_bBCM; }
    // end BCM

    virtual HX_RESULT pauseBuffers      (void);
    virtual HX_RESULT resumeBuffers     (void);

    virtual BOOL SupportsPacketAggregation(void) { return (m_uFeatureLevel >= 2); }
    virtual HX_RESULT sendPackets       (BasePacket** pPacket);

    /*
     * IHXServerPauseAdvise methods
     */
    STDMETHOD(OnPauseEvent) (THIS_ BOOL bPause);

    /* IHXQoSSignalSourceResponse */
    STDMETHOD (SignalBusReady)(THIS_ HX_RESULT hResult,
                                IHXQoSSignalBus* pBus,
                               IHXBuffer* pSessionId);

    // IHXSocketResponse
    STDMETHOD(EventPending)             (THIS_ UINT32 uEvent, HX_RESULT status);

    HX_RESULT init                      (IUnknown* pContext,
                                         IHXSocket* pSocket,
                                         IHXRTSPTransportResponse* pResp);
    void setPeerAddr                    (IHXSockAddr* pAddr);
    HX_RESULT sendPacket                (ServerPacket* pPacket);
    HX_RESULT sendMulticastPacket       (BasePacket* pPacket);
    HX_RESULT sendRTTResponsePacket     (UINT32 secs, UINT32 uSecs);
    HX_RESULT sendCongestionPacket      (INT32 xmitMultiplier,
                                        INT32 recvMultiplier);
    virtual HX_RESULT sendBWProbingPackets(UINT32 ulCount, UINT32 ulSize, REF(INT32) lSeqNo);

    HX_RESULT sendPackets               (ServerPacket** pPacket);
    STDMETHOD(PacketReady) (ServerPacket* pPacket);
    STDMETHOD(SetSource) (IHXServerPacketSource* pSource);
    STDMETHOD(SourceDone) (void);

    void handleACKTimeout               (void);
    STDMETHOD(SetDeliveryBandwidth)(UINT32 ulBackOff, UINT32 ulBandwidth);
    STDMETHOD(SetAggregationLimits) (THIS_   UINT32* pAggregStat,
                                     UINT32 ulAggregateTo, UINT32 ulAggregateHighest);

    class SeekCallback : public IHXCallback
    {
    public:
        SeekCallback                    (RDTUDPTransport* pTransport);

        /*
         *      IUnknown methods
         */
        STDMETHOD(QueryInterface)       (THIS_
                                        REFIID riid,
                                        void** ppvObj);

        STDMETHOD_(ULONG32,AddRef)      (THIS);

        STDMETHOD_(ULONG32,Release)     (THIS);

        /*
         *      IHXCallback methods
         */
        STDMETHOD(Func)                 (THIS);

    private:
        friend class RDTUDPTransport;
        RDTUDPTransport*                m_pTransport;
        LONG32                          m_lAckRefCount;
        BOOL                            m_bIsCallbackPending;
        CallbackHandle                  m_Handle;
                                        ~SeekCallback();
    };
    friend class SeekCallback;

    class TransportInfoCallback : public IHXCallback
    {
    public:
        TransportInfoCallback           (RDTUDPTransport* pTransport);

        ~TransportInfoCallback          (void);

        /*
         *      IUnknown methods
         */
        STDMETHOD(QueryInterface)       (THIS_
                                        REFIID riid,
                                        void** ppvObj);

        STDMETHOD_(ULONG32,AddRef)      (THIS);

        STDMETHOD_(ULONG32,Release)     (THIS);

        /*
         *      IHXCallback methods
         */
        STDMETHOD(Func)                 (THIS);

        void Start(UINT32 ulFrequency);
        void Stop(void);

        void Suspend(void);
        void Resume(void);

    private:
        friend class RDTUDPTransport;
        RDTUDPTransport*                m_pTransport;
        IHXScheduler*                   m_pScheduler;
        LONG32                          m_lRefCount;
        CallbackHandle                  m_Handle;
        UINT32                          m_ulFrequency;
    };
    friend class TransportInfoCallback;

    class UDPOutputCallback : public IHXCallback
    {
    public:
        UDPOutputCallback               (RDTUDPTransport* pTransport,
                                         IHXSockAddr* pPeerAddr,
                                         IHXBuffer* pBuffer,
                                         IHXSocket* pUDPSocket);

        virtual ~UDPOutputCallback      (void);

        /* IUnknown methods  */
        STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
        STDMETHOD_(ULONG32,AddRef)      (THIS);
        STDMETHOD_(ULONG32,Release)     (THIS);

        /* IHXCallback methods */
        STDMETHOD(Func)                 (THIS);

    private:
        friend class RDTUDPTransport;
        LONG32                          m_lRefCount;
        RDTUDPTransport*                m_pTransport;
        IHXSockAddr*                    m_pPeerAddr;
        IHXBuffer*                      m_pBuffer;
        IHXSocket*                      m_pSocket;
        BOOL                            m_bDone;
    };
    friend class UDPOutputCallback;

    friend class RDTUDPVectorPacketTransport;    // For unit testing.
    friend class CUTRDTUDPTransportTestDriver;    // For unit testing.

protected:
    // For unit testing
    HX_RESULT virtual VectorPacket (BasePacket* pPacket,
                                    IHXBuffer** ppCur = NULL);

private:
    HX_RESULT m_udpOutputStatus;
    typedef struct _BitmapDataItem
    {
        BYTE* m_pData;
        UINT32 m_dataLen;
    } BitmapDataItem;

    typedef struct _BwDetectionData
    {
        UINT32      m_ulSize;
        UINT32      m_ulTimeStamp;
        UINT32      m_ulATime;
    } BwDetectionData;

    HX_RESULT MakeRDTHeader (BasePacket* pPacket, IHXBuffer* pHeader,
                             BOOL bAggregating);

    HX_RESULT sendACKPacket             (void);

    HX_RESULT handleMulticastDataPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                        UINT32* pLen, UINT32 ulTimeStamp);
    HX_RESULT handleASMActionPacket     (IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen);
    HX_RESULT handleACKPacket           (IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen);
    HX_RESULT handleRTTRequestPacket    (IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen);
    HX_RESULT handleRTTResponsePacket    (IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen);
    HX_RESULT handleCongestionPacket    (IHXBuffer* pBuffer, UINT32* pPos,
                                         UINT32* pLen);
    HX_RESULT handleBandwidthReportPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                          UINT32* pLen, UINT32 ulTime);
    void setRTTInfo(UINT32 uReqTimeMs);

    HX_RESULT writePacket               (IHXBuffer* pSendBuffer);

    // Template functions to make common code that differs only in the packet
    // pointer type (BasePacket v. ServerPacket).

    template<typename PacketT>
    HX_RESULT sendPacketImpl            (PacketT* pPacket);

    template<typename PacketT>
    HX_RESULT sendPacketsImpl           (PacketT** pPacket);

    typedef HXCOMPtr<SeekCallback> SPSeekCallback;
    typedef HXCOMPtr<TransportInfoCallback> SPTransportInfoCallback;

    IHXSockAddr*                        m_pPeerAddr;
    SeekCallback*                       m_pSeekCallback;
    TransportInfoCallback*              m_pTransportInfoCallback;
    BOOL                                m_bCallbackPending;
#ifdef DEBUG
    BOOL                                m_drop_packets;
    UINT32                              m_packets_since_last_drop;
#endif /* DEBUG */
    UINT32                              m_ulBackToBackTime;
    IHXSocket*                          m_pMCastUDPSocket;
    UINT32                              m_ulCurrentMulticastAddress;
    UINT32                              m_ulCurrentMulticastPort;
    BOOL                                m_bDisableResend;

    BOOL                                m_bBCM;
    UINT32                              m_ulRTTProbeFrequency;
    BOOL                                m_bSuspendRTTProbeOnPause;

    /* Media Delivery Pipeline: */
    double                              m_fRTT;
    QoSRDTMetrics*                      m_pQoSMetrics;
    BOOL                                m_bFirstPacket;
    BOOL                                m_bPktAggregationEnabled;
};

#endif /* ndef _RDT_UDP_H_ */
