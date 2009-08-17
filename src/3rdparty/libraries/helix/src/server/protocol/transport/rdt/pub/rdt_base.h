/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rdt_base.h,v 1.6 2007/03/22 19:16:56 tknox Exp $
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

#ifndef _RDT_BASE_H_
#define _RDT_BASE_H_

#include "hxcom.h"
#include "hxcomptr.h"

#include "transport.h"       // For Transport
#include "hxservrdttran.h"      // For IHXServerRDTTransport
#include "hxqos.h"              // For IHXQoSSignalSourceResponse
#include "hxnet.h"              // For IHXSocketResponse
#include "hxtick.h"

_INTERFACE IHXBuffer;
_INTERFACE IHXAccurateClock;
_INTERFACE IHXQoSTransportAdaptationInfo;
_INTERFACE IHXServerPacketSource;

static const UINT32 LATENCY_REPORT_INTERVAL_MS = 1000;

#define PACKET_QUEUE_SIZE 12
#define RDT_RTT_GAIN 0.1
#define RDT_DEFAULT_RTT_FREQ 250
#define RDT_DEFAULT_PKTS_PER_BUF_REQ 20

#define RDTV4 4

static inline UINT32
GenerateTimeStamp(IHXBuffer* pBuffer)
{
    UINT32 ulReturn;
    /*
     *  See if this buffer was marked with a timestamp.  If not, use now.
     */
    IHXTimeStampedBuffer* pTBuff;
    if(HXR_OK == pBuffer->QueryInterface(IID_IHXTimeStampedBuffer,
                                        (void **)&pTBuff))
    {
        ulReturn = pTBuff->GetTimeStamp();
        pTBuff->Release();
    }
    else
    {
        ulReturn = HX_GET_TICKCOUNT();
    }
    return ulReturn;
}

class RDTBaseTransport : public Transport
                       , public IHXServerRDTTransport
                       , public IHXQoSSignalSourceResponse
                       , public IHXSocketResponse
{
public:
    RDTBaseTransport                     (BOOL bIsSource,
                                         UINT32 uFeatureLevel = 0);
    virtual ~RDTBaseTransport            (void);

    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);


    // Transport
    STDMETHOD_(void, Done)              (THIS);
    virtual void Reset                  (void) {}
    virtual void Restart                (void) {}
    virtual RTSPTransportTypeEnum tag   (void);
    virtual void setSessionID           (const char* pSessionID);
    virtual HX_RESULT handlePacket      (IHXBuffer* pBuffer) = 0;
    virtual HX_RESULT streamDone        (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL) = 0;

    /* IHXQoSSignalSourceResponse */
    STDMETHOD (SignalBusReady)(THIS_ HX_RESULT hResult,
                                IHXQoSSignalBus* pBus,
                               IHXBuffer* pSessionId) = 0;

    HX_RESULT init                      (IUnknown* pContext,
                                        IHXSocket* pSocket,
                                        IHXRTSPTransportResponse* pResp);
    virtual HX_RESULT sendPackets       (BasePacket** pPacket) = 0;
    virtual HX_RESULT sendPacket        (BasePacket* pPacket) = 0;
    virtual HX_RESULT sendPackets       (ServerPacket** pPacket) = 0;
    virtual HX_RESULT sendBWProbingPackets(UINT32 ulCount, UINT32 ulSize, REF(INT32) lSeqNo) = 0;
    HX_RESULT sendStreamEndPacket       (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL);

    /* IHXServerPacketSink */
    STDMETHOD(PacketReady) (ServerPacket* pPacket) = 0;
    STDMETHOD(SetSource) (IHXServerPacketSource* pSource) = 0;
    STDMETHOD(Flush) (void);
    STDMETHOD(SourceDone) (void) = 0;

    /* IHXSocketResponse */
    STDMETHOD(EventPending)             (THIS_ UINT32 uEvent, HX_RESULT status) = 0;

    STDMETHOD(SetDeliveryBandwidth)(UINT32 ulBackOff, UINT32 ulBandwidth) = 0;
    STDMETHOD(SetAggregationLimits) (THIS_   UINT32* pAggregStat,
                                     UINT32 ulAggregateTo, UINT32 ulAggregateHighest) = 0;

    void SetRDTSessionID                (UINT32 uRSID);
    void UseExtendedPackets             (BOOL bExt);
    
    void setLatencyPeriod               (UINT32 ulMs);

protected:
    BOOL createLatencyReportPacket(TNGLatencyReportPacket* pRpt);
    void ReleasePacketQueue             ();
    UINT32 SetBandwidthAndLatency       (UINT32 ulBandwidth);

    virtual HX_RESULT handleLatencyReportPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                        UINT32* pLen, UINT32 ulTime);
    HX_RESULT handleTransportInfoReqPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                           UINT32* pLen, UINT32 ulTime);
    HX_RESULT handleTransportInfoRespPacket(IHXBuffer* pBuffer, UINT32* pPos,
                                            UINT32* pLen);
    virtual void setRTTInfo(UINT32 uReqTimeMs) {};

    UINT32 SendPacketQueue(void);
    void   QueuePacket(ServerPacket* pPacket, UINT32 ulPacketSize);

    virtual HX_RESULT writePacket       (IHXBuffer* pSendBuffer) = 0;
    
    void updateQoSInfo                  (UINT64 ulBytesSent);

    IUnknown*                           m_pContext;

    UINT32                              m_uFeatureLevel;
    IHXAccurateClock*                   m_pAccurateClock;

    /* Media Delivery Pipeline: */
    IHXQoSSignalBus*                    m_pSignalBus;
    IHXQoSTransportAdaptationInfo*      m_pQoSInfo;
    IHXBuffer*                          m_pSessionId;

    BOOL                                m_bDone;

    UINT32                              m_lastLatencyReportTime;
    UINT32                              m_LatencyReportStartTime;
    UINT32                              m_ulLatencyReportPeriod;

    UINT32                              m_ulBackToBackFreq;
    UINT32                              m_ulBackToBackCounter;

    BOOL                                m_bAttemptingBackToBack;
    IHXServerPacketSource*              m_pSource;
    UINT8                               m_ucPacketQueuePos;
    UINT16                              m_ulPacketQueueSize;
    ServerPacket**                      m_pPacketQueue;
    UINT32                              m_ulBandwidth;

    UINT32                              m_ulAggregateTo;
    UINT32                              m_ulAggregateHighest;
    UINT32*                             m_pAggregStat;

    IHXSocket*                          m_pSocket;
    RTSPTransportTypeEnum               m_lTransportType;

    BOOL                                m_bUseExtPkts;
    UINT32                              m_ulRDTSessionID;

    friend class CUTRDTBaseTransportTestDriver;    // For unit testing.
    friend class CUTRDTTCPTransportTestDriver;     // For unit testing.
    friend class CUTRDTUDPTransportTestDriver;     // For unit testing.
};

#endif /* ndef _RDT_BASE_H_ */
