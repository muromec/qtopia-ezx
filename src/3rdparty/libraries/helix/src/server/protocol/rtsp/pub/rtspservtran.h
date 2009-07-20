/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspservtran.h,v 1.3 2006/12/21 19:05:06 tknox Exp $
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

#ifndef _RTSP_TRAN_H_
#define _RTSP_TRAN_H_

#include "hxtypes.h"
#include "hxcom.h"

#include "source.h"
#include "transport.h"
#include "rtspserv.h"
#include "hxrtsp2.h"

#include "hxclientprofile.h"

#include "sink.h"
#include "source.h"
#include "hxservpause.h"

#include "hxstreamadapt.h"

class HXProtocol;
class RTSPTransportHack;
class RTSPServerMulticastTransport;
struct IHXValues;
struct IHXPacketResend;
struct IHXRTSPProtocolResponse;
_INTERFACE IHXServerRDTTransport;

class ServerPacket;
_INTERFACE IHXServerPacketSource;
_INTERFACE IHXServerPacketSink;
_INTERFACE IHXQoSCongestionControl;

class RTSPServerTransport: public Transport,
                           public IHXServerPacketSink,
                           public IHXServerPacketSource,
                           public IHXServerPauseAdvise
{
public:

    RTSPServerTransport(IUnknown* pContext,
                        Transport* pTransport,
                        const char* pSessionID,
                        UINT16 streamNumber,
                        UINT32 ulReliability,
                        HXBOOL bUseRegistryForStats,
                        IHXSessionStats* pSessionStats);

    virtual ~RTSPServerTransport();

    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)		(THIS);
    STDMETHOD_(ULONG32,Release)		(THIS);

    /*
     *  IHXStatistics methods
     */

    STDMETHOD (InitializeStatistics)    (THIS_
                                        UINT32  /*IN*/ ulRegistryID);

    STDMETHOD (UpdateStatistics)        (THIS);

    /* IHXServerPacketSink */
    STDMETHOD(PacketReady) (ServerPacket* pPacket);
    STDMETHOD(SetSource) (IHXServerPacketSource* pSource);
    STDMETHOD(Flush) ();
    STDMETHOD(SourceDone)();

    /* IHXServerPacketSource */
    STDMETHOD(SinkBlockCleared)(THIS_ UINT32 ulStream);
    STDMETHOD(EnableTCPMode) (THIS);
    STDMETHOD(SetSink)(THIS_ IHXServerPacketSink* pSink) {return HXR_OK;}
    STDMETHOD(StartPackets) (THIS) {return HXR_OK;}
    STDMETHOD(GetPacket) (THIS) {return HXR_OK;}

    /* IHXServerPauseAdvise */
    STDMETHOD(OnPauseEvent) (THIS_ BOOL bPause);

    /* Other Methods */

    STDMETHOD_(BOOL,isNullSetup)        (THIS);

    STDMETHOD_(BOOL,isRTSPMulticast)    (THIS);

    STDMETHOD_(BOOL,isRTP)              (THIS) { return m_pTransport->isRTP(); }

    STDMETHOD_(BOOL,isReflector)        (THIS) { return m_pTransport->isReflector(); }

    STDMETHOD (getRTCPRule)           (THIS_ REF(UINT16) unRTCPRule)
                                { return m_pTransport->getRTCPRule (unRTCPRule); }

    STDMETHOD_(HXBOOL,IsInitialized)    (THIS);
    BOOL                IsUpdated();
    HX_RESULT           sendPacket(BasePacket* pPacket);
    HX_RESULT           sendPackets(BasePacket** pPacket);
    HX_RESULT           streamDone(UINT16 uStreamNumber,
                           UINT32 uReasonCode = 0,
                           const char* pReasonText = NULL);
    void                setSequenceNumber(UINT16 uStreamNumber,
                                          UINT16 uSequenceNumber);
    void                setTimeStamp(UINT16 uStreamNumber, UINT32 ulTS,
                                     BOOL bIsRaw = FALSE);

    STDMETHOD(SetDeliveryBandwidth)(UINT32 ulBackOff, UINT32 ulBandwidth);

    UINT32              wrapSequenceNumber()
                            {return m_pTransport->wrapSequenceNumber();}
    CHXString           m_sessionID;
    UINT16              m_uStreamNumber;
    BOOL                SupportsPacketAggregation();

private:
    STDMETHOD(InitRDTTransport)();

    IUnknown*           m_pContext;
    Transport*      m_pTransport;
    UINT32              m_ulReliability;
    UINT32              m_ulRegistryID;
    HXBOOL              m_bUseRegistryForStats;
    IHXServerPacketSource*    m_pSource;
    IHXServerPacketSink*    m_pSink;
    IHXServerRDTTransport* m_pRDTTransport;
    IHXSessionStats*    m_pSessionStats;
    IHXServerPauseAdvise*  m_pPauseAdvise;

    friend class RTSPServerMulticastTransport;
};

inline HX_RESULT
RTSPServerTransport::sendPacket(BasePacket* pPacket)
{
    m_pTransport->sendPacket(pPacket);
    return HXR_OK;
}

inline HX_RESULT
RTSPServerTransport::sendPackets(BasePacket** pPacket)
{
    m_pTransport->sendPackets(pPacket);
    return HXR_OK;
}

inline HX_RESULT
RTSPServerTransport::streamDone(UINT16 uStreamNumber,
        UINT32 uReasonCode /* = 0 */, const char* pReasonText /* = NULL */)
{
    m_pTransport->streamDone(uStreamNumber, uReasonCode, pReasonText);
    return HXR_OK;
}

inline void
RTSPServerTransport::setSequenceNumber(UINT16 uStreamNumber,
    UINT16 ulSequenceNumber)
{
    m_pTransport->setFirstSeqNum(uStreamNumber, ulSequenceNumber);
}

inline void
RTSPServerTransport::setTimeStamp(UINT16 uStreamNumber, UINT32 ulTS,
                                  BOOL bIsRaw)
{
    m_pTransport->setFirstTimeStamp(uStreamNumber, ulTS, bIsRaw);
}

inline BOOL
RTSPServerTransport::IsInitialized()
{
    return m_pTransport->IsInitialized();
}

inline BOOL
RTSPServerTransport::IsUpdated()
{
    return m_pTransport->IsUpdated();
}

inline BOOL
RTSPServerTransport::isNullSetup()
{
    return m_pTransport->isNullSetup();
}

inline BOOL
RTSPServerTransport::isRTSPMulticast()
{
    return m_pTransport->tag() == RTSP_TR_RDT_MCAST;
}

inline BOOL
RTSPServerTransport::SupportsPacketAggregation()
{
    return m_pTransport->SupportsPacketAggregation();
}

#endif /* _RTSP_TRAN_H_ */
