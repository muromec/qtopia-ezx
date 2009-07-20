/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_tran_cc.cpp,v 1.73 2007/04/25 00:56:47 darrick Exp $ 
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
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxengin.h"
#include "hxassert.h"
#include "hxmon.h"
#include "hxccf.h"

#include "errdbg.h"
#include "timeval.h"
#include "hxtick.h"
#include "ntptime.h"
#include "bufnum.h"
#include "rtppkt.h"
#include "rtpwrap.h"
#include "rtcputil.h"
#include "source.h"
#include "sink.h"
#include "servpckts.h"

#include "hxqossig.h"
#include "hxqos.h"
#include "hxqosinfo.h"
#include "hxqostran.h"

#include "qos_tran_cc.h"
#include "qos_tran_aimd.h"
#include "qos_cfg_names.h"
#include "qos_tran_rdt_metrics.h"

#define QOS_CC_MIN_SCHED_GRANULARITY 10 /* msec */
#define QOS_CC_RTP_UDP_IP_OVERHEAD   36 /* bytes */
#define QOS_CC_RTP_TCP_IP_OVERHEAD   48 /* bytes */
#define QOS_CC_RDT_UDP_IP_OVERHEAD   36 /* bytes */
#define QOS_CC_RDT_TCP_IP_OVERHEAD   48 /* bytes */

QoSCongestionCtl::QoSCongestionCtl(IUnknown* pContext) :
    m_lRefCount(0),
    m_pSignalBus(NULL),
    m_pThroughputSignal (NULL),
    m_pRTTSignal(NULL),
    m_pContext(pContext),
    m_pScheduler(NULL),
    m_pTSScheduler(NULL),
    m_pCongestionEqn(NULL),
    m_pSessionId(NULL),
    m_pTimeout(NULL),
    /* Rate Shaping: */
    m_ulTokens(0),
    m_ulMaxBurst(QOS_CC_DEFAULT_BURST_LIMT),
    m_bPassThrough (FALSE),
    m_bCallbacksStarted (FALSE),
    m_ulStartRate (QOS_CC_DEFAULT_START_RATE),
    m_ulAvgPacketSize (QOS_CC_DEFAULT_PKT_SIZE << AVG_PKT_SZ_FILTER),
    m_nProtocolOverhead (QOS_CC_RTP_UDP_IP_OVERHEAD),
    m_fMaxOversendRate (QOS_CC_DEFAULT_SCALAR),
    m_ulChannelRate (QOS_CC_DEFAULT_MAX),
    m_ulMaxRate (QOS_CC_DEFAULT_MAX),
    m_ulSDBRate (0),
    m_bUseSDB (FALSE),
    m_ulMediaRate(0),
    m_ulTotalRate (0),
    m_unStreamNumber(0),
    m_bBlocked (FALSE),
    m_ulCallbackHandle(0),
    m_ulTotalBandwidth(0),
    m_pAggregator (NULL),
    /* TCP Mode: */
    m_bConfigureTCPMode (FALSE),
    m_bTCP(FALSE),
    /* RTCP Metrics Analysis: */
    m_bFirstRR (FALSE),
    m_unLastSeq(0),
    m_unCycleCnt (0),
    m_ulLastLoss(0),
    m_ulNumLost (0),
    m_fRTT(0.0),
    m_fRRIntvl (0.0),
    m_fLastLoss(0.0),
    m_ulTotalAckd (0),
    m_pSource(NULL),
    m_pSink (NULL),
    m_pTransportInfo (NULL),
    m_pLinkCharParams (NULL),
    /* Outbound Rate: */
    m_fAvgOutboundRate (0.0),
    m_ulBitAccumulator (0),
    /* Debug: */
    m_ulSessionStartTime(HX_GET_BETTERTICKCOUNT()),
    m_bShowMaxSendRateDebugInfo(FALSE)
{
    if (m_pContext)
    {
        IHXCommonClassFactory* pCCF = NULL;
        IHXBuffer*             pSigBuf = NULL;
        IHXBuffer*             pRTTSigBuf = NULL;
        
        m_pContext->AddRef();
        HX_VERIFY(SUCCEEDED(m_pContext->
                            QueryInterface(IID_IHXThreadSafeScheduler, (void**)&m_pTSScheduler)));
        
        m_pContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);

        HX_VERIFY(SUCCEEDED(m_pContext->QueryInterface(IID_IHXCommonClassFactory, 
            (void**)&pCCF)));

        if (pCCF && 
            SUCCEEDED(pCCF->CreateInstance(CLSID_IHXQoSSignal, 
                        (void**)&m_pThroughputSignal)) &&
            SUCCEEDED(pCCF->CreateInstance(CLSID_IHXQoSSignal, 
                        (void**)&m_pRTTSignal)) &&
            SUCCEEDED(pCCF->CreateInstance(CLSID_IHXBuffer, 
                        (void**)&pSigBuf)) &&
            SUCCEEDED(pCCF->CreateInstance(CLSID_IHXBuffer, 
                        (void**)&pRTTSigBuf)))
        {
            pSigBuf->SetSize(sizeof(RateSignal));
            pRTTSigBuf->SetSize(sizeof(RTTSignal));

            m_pThroughputSignal->SetId(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                                             HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                                             HX_QOS_SIGNAL_COMMON_THRUPUT));

            m_pRTTSignal->SetId(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_APPLICATION,
                                                             HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                                             HX_QOS_SIGNAL_COMMON_RTT));

            m_pThroughputSignal->SetValue(pSigBuf);
            m_pRTTSignal->SetValue(pRTTSigBuf);
        }

        IHXRegistry2* pRegistry = NULL;

        if (SUCCEEDED(m_pContext->QueryInterface(IID_IHXRegistry2,
                                                (void**)&pRegistry)))
        {
            INT32 nFlag = 0;
            if (SUCCEEDED(pRegistry->GetIntByName("config.MediaDelivery.ShowChannelRateDebugInfo", nFlag)))
            {
                m_bShowMaxSendRateDebugInfo = (nFlag == 1) ? TRUE : FALSE;
            }
        }

        HX_RELEASE(pRegistry);
        HX_RELEASE(pCCF);
        HX_RELEASE(pSigBuf);
        HX_RELEASE(pRTTSigBuf);
    }

    m_tRateTimer.tv_sec  = 0;
    m_tRateTimer.tv_usec = 0;
}

QoSCongestionCtl::~QoSCongestionCtl()
{
    if (m_pSignalBus)
    {
        m_pSignalBus->DettachListener(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                                            0,0),
                                      (IHXQoSSignalSink*)this);
        m_pSignalBus->Release();
        m_pSignalBus = NULL;
    }

    if (m_pTSScheduler && m_ulCallbackHandle)
    {
        m_pTSScheduler->Remove(m_ulCallbackHandle);
        m_ulCallbackHandle = 0;
    }

    if (m_pTimeout)
    {
        m_pTimeout->Stop();
        m_pTimeout->Release();
        m_pTimeout = NULL;
    }

    if (m_pAggregator)
    {
        m_pAggregator->RemoveRateShaper((IHXQoSRateShaper*)this);
        m_pAggregator->Release();
        m_pAggregator = NULL;
    }

    HX_RELEASE(m_pTransportInfo);
    HX_RELEASE(m_pSessionId);
    HX_RELEASE(m_pTSScheduler);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pCongestionEqn);
    HX_RELEASE(m_pSource);
    HX_RELEASE(m_pSink);
    HX_RELEASE(m_pThroughputSignal);
    HX_RELEASE(m_pRTTSignal);

    HX_DELETE(m_pLinkCharParams);
}

/* IHXQoSCongestionCtl */
STDMETHODIMP
QoSCongestionCtl::Init(IHXBuffer* pSessionId, 
                       UINT16             unStrmNumber,
                       UINT32 /* bytes */ ulMediaRate,  
                       UINT32 /* bytes */ ulPacketSize)
{
    if ((!pSessionId))
    {
        HX_ASSERT(0);
        return HXR_INVALID_PARAMETER;
    }

    m_unStreamNumber = unStrmNumber;
    
    if (ulMediaRate)
    {
        m_ulMediaRate = ulMediaRate;
    }
    else
    {
        m_ulMediaRate = m_ulStartRate;
    }

    if (ulPacketSize)
    {
        m_ulAvgPacketSize = ulPacketSize << AVG_PKT_SZ_FILTER;
    }

    HX_RELEASE(m_pSessionId);
    m_pSessionId = pSessionId;
    m_pSessionId->AddRef();

    IHXQoSSignalSource* pSignalSrc = NULL;

    if (pSessionId && 
        SUCCEEDED(m_pContext->QueryInterface(IID_IHXQoSSignalSource,
                                             (void**) &pSignalSrc)))
    {
        pSignalSrc->GetSignalBus(pSessionId, (IHXQoSSignalSourceResponse*)this);
        HX_RELEASE(pSignalSrc);
    }
    else
    {
        m_pSignalBus = NULL;
    }
    
    return HXR_OK;
}

STDMETHODIMP
QoSCongestionCtl::EnableTCPMode()
{
    m_bTCP = TRUE;
    m_bConfigureTCPMode = TRUE;
    ConfigureTCPMode();

    return (m_pSource) ? m_pSource->EnableTCPMode () : HXR_OK;
}

/* IHXQoSSignalSourceResponse */
STDMETHODIMP
QoSCongestionCtl::SignalBusReady (HX_RESULT hResult, 
                                     IHXQoSSignalBus* pBus, 
                                     IHXBuffer* pSessionId)
{
    if (FAILED(hResult))
    {
        return HXR_OK;
    }

    IHXQoSProfileConfigurator* pConfig = NULL;
    INT32 lTemp                        = 0;

    HX_RELEASE(m_pSignalBus);
    m_pSignalBus = pBus;
    m_pSignalBus->AddRef();
    
    /* Connect to the signal bus: */
    m_pSignalBus->AttachListener(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,0,0),
                                 (IHXQoSSignalSink*)this);

    /* Configure congestion control: */
    HX_VERIFY(SUCCEEDED(m_pSignalBus->QueryInterface(IID_IHXQoSProfileConfigurator, 
                                                     (void**)&pConfig)));

    if (SUCCEEDED(pConfig->GetConfigInt(QOS_CFG_CC_MAX_BURST, lTemp)))
    {
        m_ulMaxBurst = (UINT32)lTemp;
    }
    else
    {
        m_ulMaxBurst = QOS_CC_DEFAULT_BURST_LIMT;
    }

    //Initialize Channel rate from the Config only if Link Characteristics
    // are not explicitly passed by clients using 3GPP Link-Char header
    if (!m_pLinkCharParams)
    {
            lTemp = 0;
            if (SUCCEEDED(pConfig->GetConfigInt(QOS_CFG_CC_MAX_SENDRATE, lTemp)))
            {
            m_ulChannelRate = (UINT32)lTemp;
            }
            else
            {
            m_ulChannelRate = QOS_CC_DEFAULT_MAX;
            }
    }

    PrintMaxSendRateDebugInfo();

    lTemp = 0;
    if (SUCCEEDED(pConfig->GetConfigInt(QOS_CFG_CC_MAX_OSR, lTemp)))
    {
        m_fMaxOversendRate = (double)((double)lTemp/100.0);
    }
    else
    {
        m_fMaxOversendRate = QOS_CC_DEFAULT_SCALAR;
    }

    m_ulStartRate = QOS_CC_DEFAULT_START_RATE;

    m_bPassThrough = FALSE;
        
    lTemp = 0;
    if (SUCCEEDED(pConfig->GetConfigInt(QOS_CFG_CC_INIT_PKTSZ, lTemp)))
    {
        m_ulAvgPacketSize = lTemp << AVG_PKT_SZ_FILTER;
    }
    else
    {
        m_ulAvgPacketSize = QOS_CC_DEFAULT_PKT_SIZE  << AVG_PKT_SZ_FILTER;
    }

    lTemp = 0;
    if (SUCCEEDED(pConfig->GetConfigInt(QOS_CFG_CC_USE_SDB, lTemp)))
    {
        m_bUseSDB = (BOOL)lTemp;
    }
    else
    {
        m_bUseSDB = FALSE;
    }

    UpdateMaxRate();

    /* Don't set up congestion control or rate shaping if we are in pass through or TCP mode: */
    if (m_bPassThrough)
    {
        HX_RELEASE(pConfig);
        return HXR_OK;
    }

    if (m_bConfigureTCPMode)
    {
        ConfigureTCPMode();
        HX_RELEASE(pConfig);
        return HXR_OK;
    }

    /* Set up rate shaping: */
    m_ulTokens = m_ulMaxBurst;

    /* Setup congestion control algorithm if we are not honoring client SetDeliveryBandwidth requests */
    if (!m_bUseSDB)
    {
        IHXQoSClassFactory* pQoSFactory = NULL;
        if(SUCCEEDED(m_pContext->QueryInterface(IID_IHXQoSClassFactory,
                                                (void**)&pQoSFactory)))
        {
            HX_RELEASE(m_pCongestionEqn);
            if (SUCCEEDED(pQoSFactory->CreateInstance(pConfig,
                                                      CLSID_IHXQoSCongestionEquation, 
                                                      (void**)&m_pCongestionEqn)))
            {
                m_pCongestionEqn->Init(m_pContext, m_pSessionId, m_unStreamNumber);
                m_pCongestionEqn->SetMediaRate(m_ulMediaRate, TRUE);
                m_pCongestionEqn->SetMediaPacketSize(m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER);
                m_pCongestionEqn->SetMaximumRate(m_ulMaxRate);

                m_pSignalBus->QueryInterface(IID_IHXQoSTransportAdaptationInfo, 
                                             (void**)&m_pTransportInfo);

            
                m_pTimeout = new CongestionFeedbackTimeout(this, pConfig);
                m_pTimeout->AddRef();
            }
            HX_RELEASE(pQoSFactory);
        }
    }

    /* if we have an aggregator, set it up: */
    if (m_pAggregator)
    {
        HX_VERIFY(SUCCEEDED(m_pAggregator->AddRateShaper((IHXQoSRateShaper*) this)));
        m_pAggregator->AddTokens((UINT16)(m_ulMaxBurst * (m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER)), m_unStreamNumber);
    }

    HX_RELEASE(pConfig);
    return HXR_OK;
}

 /* IHXServerPacketSink */
STDMETHODIMP
QoSCongestionCtl::SetSource (IHXServerPacketSource* pSource)
{
    if (!pSource)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_pSource);
    m_pSource = pSource;
    m_pSource->AddRef();
    
    return HXR_OK;
}

STDMETHODIMP
QoSCongestionCtl::PacketReady (ServerPacket* pPacket)
{
    if (!pPacket)
    {
        HX_ASSERT(0);
        return HXR_INVALID_PARAMETER;
    }

    if (!m_pSink)
    {
        HX_ASSERT(0);
        return HXR_NOT_INITIALIZED;
    }

    if (m_ulAvgPacketSize != 0)
    {
        INT16 nDelta = 0;
                                                                                                                                                
        nDelta = (pPacket->GetSize() + m_nProtocolOverhead) - 1 - (m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER);
                                                                                                                                                
        if ((m_ulAvgPacketSize += nDelta) <= 0)
        {
            m_ulAvgPacketSize = 1;
        }
    }
    else
    {
        m_ulAvgPacketSize = (pPacket->GetSize() + m_nProtocolOverhead) << AVG_PKT_SZ_FILTER;
    }

    // If passthrough enabled, this callback is only scheduled to refresh media rate for TCP
    // (for accurate rateshifting).
    
    if (!m_bPassThrough || m_bTCP)
    {
        if ((!m_bCallbacksStarted) && (!m_ulCallbackHandle))
        {
            m_bCallbacksStarted = TRUE;
            UINT32 ulInitialRate = (m_ulMediaRate) ? m_ulMediaRate : m_ulStartRate;
        
            if (m_pCongestionEqn)
            {
                UINT32 ulCongestionEqnInitialRate = 0;
                m_pCongestionEqn->GetRate(ulCongestionEqnInitialRate);

                ulInitialRate = (ulCongestionEqnInitialRate) ? 
                                 ulCongestionEqnInitialRate : ulInitialRate;
            }

            /* adjust initial transmission rate for protocol overhead */
            ulInitialRate = AdjustRateForOverhead(ulInitialRate);

            m_ulCallbackHandle = m_pTSScheduler->
                                     RelativeEnter((IHXCallback*)this, 
                                                   TransmitInterval(m_ulTokens, ulInitialRate,
                                                                    (m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER)));
        }
    }
    
    if (!m_bPassThrough)
    {
        if (!m_pAggregator)
        {
            m_bBlocked = (!m_ulTokens);
        }
        else
        {
            m_bBlocked = (HXR_BLOCKED == m_pAggregator->RemoveTokens ((UINT16)(pPacket->GetSize()), m_unStreamNumber));
        }

        if (m_bBlocked)
        {
            pPacket->m_bTransportBlocked = TRUE;
            return HXR_BLOCKED;
        }
        
        if (m_ulTokens > 0)
        {
            m_ulTokens--;
        }
    }
        
    /* Measure Outbound Rate: */
    m_ulBitAccumulator += pPacket->GetSize() * 8;

    HXTimeval rmatv     = m_pScheduler->GetCurrentSchedulerTime();
    Timeval tvCurrentTime((INT32) rmatv.tv_sec, (INT32)rmatv.tv_usec);

    if ((m_tRateTimer.tv_sec) || (m_tRateTimer.tv_usec))
    {
        Timeval tvDiff = tvCurrentTime - m_tRateTimer;

        if (tvDiff.tv_sec)
        {
            UINT32 ulOutboundRate = 0;
            UINT32 ulDiff_ms = (tvDiff.tv_sec * 1000) + (tvDiff.tv_usec / 1000);
            HX_ASSERT(ulDiff_ms);
        
            ulOutboundRate = (ulDiff_ms) ? m_ulBitAccumulator / ulDiff_ms : 0;
            ulOutboundRate *= 1000;

            if (m_fAvgOutboundRate)
            {
                m_fAvgOutboundRate = (double)((1 - QOS_CC_OUT_RATE_FILTER) * m_fAvgOutboundRate + 
                                              QOS_CC_OUT_RATE_FILTER * (double)ulOutboundRate); 
            }
            else if (m_bTCP)
            {
                // It takes some time before the TCP stack blocks.
                // This causes the outbound rate to be high initially.
                // Register the average outbound rate as the media
                // rate initially for TCP to prevent unwanted upshifts.
                m_fAvgOutboundRate = (double)m_ulMediaRate;
            }
            else
            {
                m_fAvgOutboundRate = (double)ulOutboundRate;
            }
           
            m_tRateTimer = tvCurrentTime;
            m_ulBitAccumulator = 0;
        }
    }
    else
    {
        m_tRateTimer = tvCurrentTime;
    }
        
    return m_pSink->PacketReady(pPacket);
}

STDMETHODIMP
QoSCongestionCtl::Flush ()
{
    return (m_pSink) ? m_pSink->Flush() : HXR_NOT_INITIALIZED;
}

STDMETHODIMP
QoSCongestionCtl::SourceDone ()
{
    if (m_pAggregator)
    {
        m_pAggregator->RemoveRateShaper((IHXQoSRateShaper*)this);
        m_pAggregator->Release();
        m_pAggregator = NULL;
    }

    HX_RELEASE(m_pSource);
    if (m_ulCallbackHandle)
    {
        m_pTSScheduler->Remove(m_ulCallbackHandle);
        m_ulCallbackHandle = 0;
    }
    return (m_pSink) ? m_pSink->SourceDone() : HXR_OK;
}

 /* IHXServerPacketSource */
STDMETHODIMP
QoSCongestionCtl::SetSink (IHXServerPacketSink* pSink)
{
    if (!pSink)
    {
        HX_ASSERT(0);
        return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_pSink);
    m_pSink = pSink;
    m_pSink->AddRef();

    return m_pSink->SetSource((IHXServerPacketSource*)this);
}
 
STDMETHODIMP
QoSCongestionCtl::StartPackets ()
{
    return (m_pSource) ? m_pSource->StartPackets () : HXR_NOT_INITIALIZED;
}

STDMETHODIMP
QoSCongestionCtl::GetPacket ()
{
    return (m_pSource) ? m_pSource->GetPacket () : HXR_NOT_INITIALIZED;
}
 
STDMETHODIMP
QoSCongestionCtl::SinkBlockCleared (UINT32 ulStream)
{
    return (m_pSource) ? m_pSource->SinkBlockCleared (ulStream) : HXR_NOT_INITIALIZED;
}

/* IHXQoSRateShaper */
STDMETHODIMP
QoSCongestionCtl::Init (THIS_ IHXQoSRateShapeAggregator* pAggregator)
{
    HX_ASSERT(!m_pAggregator);

    HX_RELEASE(m_pAggregator);
    m_pAggregator = pAggregator;
    m_pAggregator->AddRef();

    /* we will be added to the aggregator in the SignalBusReady() method*/

    return HXR_OK;
}

STDMETHODIMP
QoSCongestionCtl::StreamCleared (THIS_ UINT16 nStreamNumber)
{
    HX_ASSERT(nStreamNumber == m_unStreamNumber);

    return (m_pSource) ? m_pSource->SinkBlockCleared ((UINT32)nStreamNumber) : HXR_NOT_INITIALIZED;
}

STDMETHODIMP
QoSCongestionCtl::GetStreamNumber (THIS_ REF(UINT16) /*OUT*/ nStreamNumber)
{
    nStreamNumber = m_unStreamNumber;
    return HXR_OK;
}

STDMETHODIMP
QoSCongestionCtl::GetMaxTokens (THIS_ REF(UINT32) /*OUT*/ ulMaxTokens)
{
    if (!m_ulMaxBurst)
    {
        return HXR_NOT_INITIALIZED;
    }

    ulMaxTokens = (UINT32)(m_ulMaxBurst * QOS_CC_MAX_PKT_SZ);
    return HXR_OK;
}

/* IHXCallback method */
STDMETHODIMP
QoSCongestionCtl::Func()
{
    if (!m_pTSScheduler)
    {
        m_ulCallbackHandle = 0;
        return HXR_FAIL;
    }
    
    // Reported and actual rates only differ for TCP w/ HonorMaxSendRate.

    UINT32 ulReportedRate = 0;  // Reported to the rate manager for determining rateshift.
    UINT32 ulAttemptedRate = 0; // Actual rate that we are trying to send at.

    UpdateMaxRate();

    // Report accurate rate for rateshifting.
    if (m_bTCP)
    {
        ulReportedRate = (m_ulSDBRate) ? m_ulSDBRate : AdjustRateForOverhead((UINT32)m_fAvgOutboundRate);
  
        // Similar to other congestion eqn behavior. 
        if (ulReportedRate == 0)
        {
            ulReportedRate = m_bPassThrough ? m_ulMediaRate : m_ulMaxRate; 
        }

        // Always try and write to socket at max rate if TCP limitrate on. 
        // Passthrough ignores tokens anyway, so this doesn't matter.
        ulAttemptedRate = m_bPassThrough ? ulReportedRate : m_ulMaxRate;

    }
    else if (m_pCongestionEqn)
    {
        m_pCongestionEqn->SetMediaPacketSize((m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER));
        m_pCongestionEqn->GetRate(ulAttemptedRate);
        ulAttemptedRate = (ulAttemptedRate > m_ulMaxRate) ? m_ulMaxRate : ulAttemptedRate;
        ulReportedRate = ulAttemptedRate;
    }
    else
    {
        ulAttemptedRate = (m_ulSDBRate) ? m_ulSDBRate : m_ulMaxRate;
        ulReportedRate = ulAttemptedRate;
    }
     
    //fprintf(stderr, "cc::func outbnd %u med %u chan %u rep %u act %u max %u avgpktsize %u\n", (UINT32)m_fAvgOutboundRate, m_ulMediaRate, m_ulChannelRate, ulReportedRate, ulAttemptedRate, m_ulMaxRate, m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER);
    /* Notify the stack of the new rate */
    if (m_pSignalBus)
    {
        IHXBuffer* pTmp = NULL;

        if (SUCCEEDED(m_pThroughputSignal->GetValue(pTmp)))
        {
            RateSignal* pSignal = (RateSignal*)pTmp->GetBuffer();

            if (pSignal)
            {
                pSignal->m_unStreamNumber = m_unStreamNumber;
                pSignal->m_ulRate         = ulReportedRate;
            }
        }

        HX_RELEASE(pTmp);

        m_pThroughputSignal->SetValueUINT32(ulReportedRate);
        m_pSignalBus->Send(m_pThroughputSignal);
    }

    if (!m_bPassThrough)
    {
        if (m_pAggregator)
        {
            /* this method may call sink block cleared on this stream */
            HX_VERIFY(SUCCEEDED(m_pAggregator->AddTokens ((UINT16)(m_ulMaxBurst * (m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER)), m_unStreamNumber)));
        }
        else
        {
            m_ulTokens = m_ulMaxBurst;
        }
    
        if (m_bBlocked)
        {
            m_bBlocked = FALSE;

            /*
              if we don't have an aggregator
              the we must clear the stream ourselves
            */

            if (!m_pAggregator)
            {
                if (m_pSource)
                {
                    m_pSource->SinkBlockCleared(m_unStreamNumber);
            
                }
            }
        }
    }

    /* Schedule the next callback */
    m_ulCallbackHandle = m_pTSScheduler->
        RelativeEnter((IHXCallback*)this, 
                      TransmitInterval(m_ulMaxBurst, ulAttemptedRate/8, 
                                       m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER));

    return HXR_OK;
}

/* IHXQoSSignalSink */
STDMETHODIMP
QoSCongestionCtl::Signal(IHXQoSSignal* pSignal, IHXBuffer* pSessionId)
{
    if (!pSignal)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_QOS_SIGNAL ulId     = 0;
    IHXBuffer*    pBuffer = NULL;

    pSignal->GetId(ulId);

    switch (ulId & HX_QOS_SIGNAL_ID_MASK)
    {
    case HX_QOS_SIGNAL_RTCP_RR:
    {
        pSignal->GetValue(pBuffer);
        
        m_nProtocolOverhead = QOS_CC_RTP_UDP_IP_OVERHEAD;
        
        if (SUCCEEDED(HandleRTCPFeedback(pBuffer)))
        {
            UpdateMaxRate();

            if (m_pCongestionEqn)
            {
                m_pCongestionEqn->SetMaximumRate(m_ulMaxRate);
                m_pCongestionEqn->Update(m_ulLastRecvd,m_ulNumLost,
                                         m_fRecvdRate, m_fAvgOutboundRate,
                                         m_fLastLoss, m_fRTT);
            }
            
            if (m_pTimeout)
            {
                m_pTimeout->OnFeedback();
            }
        }
  
         HX_RELEASE(pBuffer);
         break;
    }
    case HX_QOS_SIGNAL_RDT_METRICS:
    {
         pSignal->GetValue(pBuffer);
 
         RDTTransportMetrics* pMetrics = (RDTTransportMetrics*)(pBuffer->GetBuffer());

        m_nProtocolOverhead = QOS_CC_RDT_UDP_IP_OVERHEAD;
 
         if (pMetrics && (pMetrics->m_nStreamNumber == m_unStreamNumber))
         {
            UpdateMaxRate();
            
            if (m_pCongestionEqn)
            {
                m_pCongestionEqn->SetMaximumRate(m_ulMaxRate);
                m_pCongestionEqn->Update(pMetrics->m_ulNumRecvd, 
                                         pMetrics->m_ulNumLost,
                                         (double)m_ulMediaRate,
                                         m_fAvgOutboundRate,
                                         pMetrics->m_fLoss,
                                         pMetrics->m_fRTT);
            }
        }
            
        HX_RELEASE(pBuffer);
        break;
    }
    case HX_QOS_SIGNAL_RDT_RTT:
    {
        m_nProtocolOverhead = QOS_CC_RDT_UDP_IP_OVERHEAD;

         pSignal->GetValue(pBuffer);
         double* pRTT = (double*)(pBuffer->GetBuffer());
 
         if (pRTT)
         {
            if (m_pCongestionEqn)
            {
                m_pCongestionEqn->Update(0, 0, (double)m_ulMediaRate,
                                         m_fAvgOutboundRate, 0, *pRTT);
            }

            if (m_pTimeout)
            {
                m_pTimeout->OnFeedback();
            }
        }
            
        HX_RELEASE(pBuffer); 
        break;
    }
    case HX_QOS_SIGNAL_COMMON_INIT_MEDIA_RATE:
    case HX_QOS_SIGNAL_COMMON_MEDIA_RATE:
    {        
        pBuffer = NULL;
        pSignal->GetValue(pBuffer);

        if (pBuffer)
        {
            RateSignal* pRateSignal = (RateSignal*)pBuffer->GetBuffer();
            
            if (pRateSignal && (pRateSignal->m_unStreamNumber == m_unStreamNumber))
            {
                m_ulMediaRate = (pRateSignal->m_ulRate) ? pRateSignal->m_ulRate : 
                    QOS_CC_DEFAULT_START_RATE;
                m_ulTotalRate = pRateSignal->m_ulCumulativeRate;

                UpdateMaxRate();
                
                if (m_pCongestionEqn)
                {
                    if (HX_QOS_SIGNAL_COMMON_INIT_MEDIA_RATE == (ulId&HX_QOS_SIGNAL_ID_MASK))
                    {
                            m_pCongestionEqn->SetMediaRate(m_ulMediaRate, TRUE);
                    }
                    else
                    {
                            m_pCongestionEqn->SetMediaRate(m_ulMediaRate, FALSE);                        
                    }
                    m_pCongestionEqn->SetMaximumRate(m_ulMaxRate);
                }
            }
        }
        
        HX_RELEASE(pBuffer);
        break;
    }
    case HX_QOS_SIGNAL_COMMON_SDB:
    {
        if (!m_pCongestionEqn && m_bUseSDB)
        {
            pSignal->GetValue(pBuffer);
            
            if (pBuffer)
            {
                RateSignal* pRateSignal = (RateSignal*)pBuffer->GetBuffer();
                
                if (pRateSignal && (pRateSignal->m_unStreamNumber == m_unStreamNumber))
                {
                    UINT32 ulRate = AdjustRateForOverhead (pRateSignal->m_ulRate);
                    UpdateMaxRate();
                    m_ulSDBRate = (m_ulMaxRate > ulRate) ? ulRate : m_ulMaxRate;
                }
            }

            HX_RELEASE(pBuffer);
        }
        break;
    }
    case HX_QOS_SIGNAL_COMMON_BANDWIDTH:
    {
        pSignal->GetValue(pBuffer);
        
        if (pBuffer)
        {
            RateSignal* pRateSignal = (RateSignal*)pBuffer->GetBuffer();
            
            if (pRateSignal)
            {
                UINT32 ulRate = AdjustRateForOverhead (pRateSignal->m_ulRate);
                m_ulTotalBandwidth = ulRate;
            }
        }

        HX_RELEASE(pBuffer);
        break;
    }
    case HX_QOS_SIGNAL_COMMON_LINK_CHAR_HDR:
    {
        pSignal->GetValue(pBuffer);

        if (pBuffer)
        {
            LinkCharSignalData* pLinkCharSigData = NULL;

            pLinkCharSigData = (LinkCharSignalData*)pBuffer->GetBuffer();
            if (pLinkCharSigData && (pLinkCharSigData->m_bSessionAggregate
                                    || (pLinkCharSigData->m_unStreamNum == m_unStreamNumber)))
            {
                SetLinkCharParams((LinkCharParams *)pLinkCharSigData);
            }
            HX_RELEASE(pBuffer);
        }

        break;
    }
    default:
    {
        break;
    }
    }
    return HXR_OK;
}

STDMETHODIMP
QoSCongestionCtl::ChannelClosed(IHXBuffer* pSessionId)
{
    HX_RELEASE(m_pSignalBus);

    /* no more feedback, revert to pass through filter */
    if (m_pTSScheduler && m_ulCallbackHandle)
    {
        m_pTSScheduler->Remove(m_ulCallbackHandle);
        m_ulCallbackHandle = 0;
    }

    if (m_pTimeout)
    {
        m_pTimeout->Stop();
        m_pTimeout->Release();
        m_pTimeout = NULL;
    }

    HX_RELEASE(m_pTSScheduler);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pCongestionEqn);
    return HXR_OK;
}

/* QoSCongestionCtl method */
HX_RESULT
QoSCongestionCtl::HandleRTCPFeedback(IHXBuffer* pRTCPReceiverReport)
{
    if ((!pRTCPReceiverReport) || (!(pRTCPReceiverReport->GetBuffer())))
    {
        return HXR_INVALID_PARAMETER;
    }
    
    ReceptionReport* pRR = (ReceptionReport*)(pRTCPReceiverReport->GetBuffer());

    if(pRR->ssrc != m_unStreamNumber)
    {
        return HXR_INVALID_PARAMETER;
    }
    
    UINT32  ulRTT = 0;
    UINT32  ulNowTrunc = 0;
    double  fPPS = 0.0;
    UINT16 unPktCnt = 0;

    /* determine the number of packets received: */
    UINT16 unHighSeq  = (pRR->last_seq & 0xffff);
    UINT16 unCycleCnt = ((pRR->last_seq & 0xffff0000) >> 16);  

    if (m_bFirstRR)
    {
        unPktCnt  = ((unCycleCnt <= m_unCycleCnt) || (m_unLastSeq > unHighSeq)) ? unHighSeq - m_unLastSeq : 
            unHighSeq + (0xffff - m_unLastSeq);
    }
    else
    {
        /* not enough information to deduce received packet count */
        m_bFirstRR = TRUE;
    }

    m_unLastSeq = unHighSeq;
    m_unCycleCnt = unCycleCnt;
    
    /* determine the number of packets lost: */
    m_ulNumLost = (pRR->lost > m_ulLastLoss) ? pRR->lost - m_ulLastLoss : 0;

    /* use same time version steps as report generation: */
    HXTimeval rmatv     = m_pScheduler->GetCurrentSchedulerTime();
    Timeval tvArrivalTime((INT32) rmatv.tv_sec, (INT32)rmatv.tv_usec);

    /* update the history */
    m_ulLastRecvd = (m_ulNumLost < unPktCnt) ? unPktCnt - m_ulNumLost : 0;
    m_ulTotalAckd += m_ulLastRecvd;

    /* get an RTT estimate sec 6.3.1 of RFC 1889 */
    if (pRR->lsr != 0)
    {
        NTPTime ntpNow = NTPTime (tvArrivalTime);
        ulNowTrunc     = ntpNow.m_ulSecond  << 16;
        ulNowTrunc    |= (ntpNow.m_ulFraction >> 16);
   
        ulRTT = abs((INT32)(ulNowTrunc - pRR->lsr - pRR->dlsr));
        NTPTime ntpRTT = NTPTime( ((ulRTT & 0xffff0000) >> 16), 
                                  ((ulRTT & 0xffff) << 16));
            
        m_fRTT = (ntpRTT.m_ulSecond * 1000.0);
        m_fRTT += ((double) ntpRTT.m_ulFraction / (double) MAX_UINT32) * 1000.0;

        /* Notify the stack of the new RTT */
        if (m_pSignalBus)
        {
            IHXBuffer* pTmp = NULL;

            if (SUCCEEDED(m_pRTTSignal->GetValue(pTmp)))
            {
                RTTSignal* pSignal = (RTTSignal*)pTmp->GetBuffer();

                if (pSignal)
                {
                    pSignal->m_unStreamNumber = m_unStreamNumber;
                    pSignal->m_fRTT = m_fRTT;
                }
            }

            HX_RELEASE(pTmp);

            m_pSignalBus->Send(m_pRTTSignal);
        }
    }

    /* compute the estimated received throughput */
    if (unPktCnt != 0)
    {
        Timeval tTimeDiff = tvArrivalTime - m_tLastRR;
        fPPS = (double)((((double)
                          (m_ulLastRecvd))/
                         ((double)(tTimeDiff.tv_sec*MILLISECOND) + 
                          (tTimeDiff.tv_usec/MILLISECOND)))*1000);

        UINT32 ulIntvlSample = ((tTimeDiff.tv_sec*MILLISECOND) + 
                                ((tTimeDiff.tv_usec/MILLISECOND)*1000));

        m_fRRIntvl = (m_fRRIntvl) ? 
            (double)((1 - AVG_PKT_SZ_FILTER) * m_fRRIntvl +
                     AVG_PKT_SZ_FILTER * ((double)(ulIntvlSample))) : (double)ulIntvlSample;

        m_fRecvdRate = fPPS*(m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER);
    }

    /* update loss: */
    m_ulLastLoss = (pRR->lost > 0) ? pRR->lost : 0;

    m_tLastRR = tvArrivalTime;

    if (m_ulTotalAckd)
    {
        m_fLastLoss = (double) ((double)pRR->lost)/((double)m_ulTotalAckd);
    }

    return HXR_OK;
}

void                          
QoSCongestionCtl::FeedbackTimeout(BOOL bDisable)
{
    if (bDisable) /* no more feedback, disable congestion control */
    {
        if (m_pTimeout)
        {
            m_pTimeout->Stop();
            m_pTimeout->Release();
            m_pTimeout = NULL;
        }

        if (m_pTSScheduler)
        {
            if (m_ulCallbackHandle)
            {
                m_pTSScheduler->Remove(m_ulCallbackHandle);
                m_ulCallbackHandle = 0;
            }

            
            m_ulCallbackHandle = m_pTSScheduler->
                RelativeEnter((IHXCallback*)this, 
                              TransmitInterval(m_ulMaxBurst, m_ulMaxRate,
                                               (m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER)));
        }
        
        HX_RELEASE(m_pCongestionEqn);
    }
    else
    {
        if (m_pCongestionEqn)
        {
            m_pCongestionEqn->FeedbackTimeout();
        }
    }
}

UINT32
QoSCongestionCtl::AdjustRateForOverhead(UINT32 ulRate)
{
    /* offset the base media rate by the protocol overhead: */
    return (((m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER)) ? 
            (ulRate + (ulRate * (m_nProtocolOverhead/(m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER)))) :
             ulRate);
}

UINT32
QoSCongestionCtl::TransmitInterval(UINT32 ulTokens, UINT32 ulRate,
                                   UINT32 ulPacketSize)
{
    // Calculate how many ms in the future to call Func() again.
    // The goal is to send MAX_BURST packets per call to Func().

    // NOTE: Minimum scheduler granularity is ~8.2ms-- that means
    // that Func() may be called up to ~8.2ms early or late. This
    // matters when the interval is low!

    // Formula: ulInterval = ulPacketSize (BYTES) / ulRate (BYTES/s) * 1000 (ms/s) *
    //                       ulTokens (packets)
    // (# of ms to wait to send another ulTokens worth of packets)

    //XXXDPL should we just not do this at all for extremely low
    // intervals?

    UINT32 ulInterval;

    if (ulRate == 0)
    {
        ulInterval = 1000;
        return ulInterval;
    }

    ulInterval = ulPacketSize * 1000 * ulTokens;
    ulInterval += (ulRate >> 1); // So division rounds to nearest int.
    ulInterval /= ulRate;

    if (ulInterval > 1000)
    {
        ulInterval = 1000;
    }
    else if (ulInterval < QOS_CC_MIN_SCHED_GRANULARITY)
    {
        ulInterval = QOS_CC_MIN_SCHED_GRANULARITY;
    }

    return ulInterval;
}

void
QoSCongestionCtl::UpdateMaxRate()
{
    if (m_pCongestionEqn ||
        m_bUseSDB ||
        m_pLinkCharParams ||
        (m_bTCP && !m_bPassThrough))
    {
        UINT32 ulChannelRate = m_ulChannelRate;

        //Channel Rate need not be apportioned when Link Characteristics have been provided
        // for individual media stream.
        //
        //If Link Characteristics are absent or are provided as a Session Aggregate 
        // the channel rate is divided based on the Media Rate of the stream
        if (m_ulTotalRate && (!m_pLinkCharParams || m_pLinkCharParams->m_bSessionAggregate))
        {
            double fRatio = (double)((double)m_ulMediaRate / (double)m_ulTotalRate);
            ulChannelRate = (UINT32)(m_ulChannelRate * fRatio);
        }

        if (m_ulMediaRate && m_fMaxOversendRate != 0.0)
        {
            UINT32 ulMaxMediaRate = 
                AdjustRateForOverhead((UINT32)((double)(m_ulMediaRate) * m_fMaxOversendRate));

            m_ulMaxRate = (ulChannelRate > ulMaxMediaRate) ? ulMaxMediaRate : ulChannelRate;
        }
        else
        {
            m_ulMaxRate = ulChannelRate;
        }
    }
    else //No congestion control.  Send at the media rate:
    {
        m_ulMaxRate = AdjustRateForOverhead(m_ulMediaRate);
    }
}

HX_RESULT
QoSCongestionCtl::ConfigureTCPMode()
{
    if (!m_pSignalBus)
    {
        return HXR_NOT_INITIALIZED;
    }

    if (m_bConfigureTCPMode)
    {
        IHXQoSProfileConfigurator* pConfig = NULL;
        INT32 lTemp                        = 0;
        m_bConfigureTCPMode                = FALSE;

        /* Configure TCP Mode: */
        HX_VERIFY(SUCCEEDED(m_pSignalBus->QueryInterface(IID_IHXQoSProfileConfigurator, 
                                                         (void**)&pConfig)));

        if (SUCCEEDED(pConfig->GetConfigInt(QOS_CFG_TCP_HONOR_MAX_SENDRATE, lTemp)))
        {
            m_bPassThrough = !((BOOL)lTemp);
        }
        else
        {
            m_bPassThrough = TRUE;
        }
        HX_RELEASE(pConfig);

        m_nProtocolOverhead = QOS_CC_RDT_TCP_IP_OVERHEAD;    

        //Only schedule token bucket updates if we are not in pass through mode
        if (m_ulCallbackHandle)
        {
            m_pTSScheduler->Remove(m_ulCallbackHandle);
            m_ulCallbackHandle = 0;
        }

        if (!m_bPassThrough)
        {
            m_ulCallbackHandle = m_pTSScheduler->
                RelativeEnter((IHXCallback*)this,  
                              TransmitInterval(m_ulTokens, m_ulMaxRate,
                                               (m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER)));
        }

        //Don't expect feedback in TCP mode
        if (m_pTimeout)
        {
            m_pTimeout->Stop();
            m_pTimeout->Release();
            m_pTimeout = NULL;
        }
        
        //Don't attempt to do congestion control over TCP
        HX_RELEASE(m_pCongestionEqn);
    }

    return HXR_OK;
}

STDMETHODIMP
QoSCongestionCtl::SetLinkCharParams (THIS_ LinkCharParams* /* IN */ pLinkCharParams)
{
    if (!m_pLinkCharParams)
    {
        m_pLinkCharParams = new LinkCharParams;
    }

    //accept new Link Characteristics
    //
    //Set Channel Rate to Maximum Bandwidth of the link
    *m_pLinkCharParams = *pLinkCharParams;

    PrintLinkCharDebugInfo(pLinkCharParams);

    //XXXDPL If this is aggregate shouldn't we use maxbw * media rate / total rate?
    UINT32 ulOldChannelRate = m_ulChannelRate;
    m_ulChannelRate = m_pLinkCharParams->m_ulMaxBW;

    if (ulOldChannelRate != m_ulChannelRate)
    {
        PrintMaxSendRateDebugInfo();        
    }

    //Re-calculate Max Rate
    UpdateMaxRate();

    return HXR_OK;
}

STDMETHODIMP
QoSCongestionCtl::GetLinkCharParams (THIS_ UINT16 unStreamNum,
                                 REF(LinkCharParams) /* OUT */ linkCharParams)
{
    if (m_pLinkCharParams)
    {
        linkCharParams = *m_pLinkCharParams;
        return HXR_OK;
    }

    return HXR_FAIL;
}

void
QoSCongestionCtl::PrintMaxSendRateDebugInfo()
{
    if (!m_bShowMaxSendRateDebugInfo)
    {
        return;
    }

    printf("| CongestionControl: Session=%s; MaxSendRate=%u\n",
           m_pSessionId ? (const char*)m_pSessionId->GetBuffer() : "UNKNOWN",
           m_ulChannelRate);
    fflush(stdout);

}
void
QoSCongestionCtl::PrintLinkCharDebugInfo(LinkCharParams* pParams)
{
    if (!m_bShowMaxSendRateDebugInfo)
    {
        return;
    }

    printf("| 3GPP-Link-Char: Session=%s; MBW=%u GBW=%u\n",
           m_pSessionId ? (const char*)m_pSessionId->GetBuffer() : "UNKNOWN",
           pParams->m_ulMaxBW,
           pParams->m_ulGuaranteedBW);
    fflush(stdout);
}

/* IUnknown */
STDMETHODIMP
QoSCongestionCtl::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXQoSCongestionControl*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSCongestionControl))
    {
        AddRef();
        *ppvObj = (IHXQoSCongestionControl*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSSignalSink))
    {
        AddRef();
        *ppvObj = (IHXQoSSignalSink*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSSignalSourceResponse))
    {
        AddRef();
        *ppvObj = (IHXQoSSignalSourceResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSRateShaper))
    {
        AddRef();
        *ppvObj = (IHXQoSRateShaper*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerPacketSink))
    {
        AddRef();
        *ppvObj = (IHXServerPacketSink*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerPacketSource))
    {
        AddRef();
        *ppvObj = (IHXServerPacketSource*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSLinkCharSetup))
    {
        AddRef();
        *ppvObj = (IHXQoSLinkCharSetup*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
QoSCongestionCtl::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
QoSCongestionCtl::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    delete this;
    return 0;
}

/* Feedback Timeout Timer */
CongestionFeedbackTimeout::CongestionFeedbackTimeout(QoSCongestionCtl* pOwner, 
                                                     IHXQoSProfileConfigurator* pConfig) :
    m_lRefCount (0),
    m_ulCallbackHandle (0),
    m_pOwner (NULL),
    m_pSched (NULL),
    m_ulInitialFeedback (0),
    m_ulLastFeedback (0),
    m_ulFeedbackInterval(0),
    m_ulTimeoutEventCount (QOS_CC_DEFAULT_MAX_TIMEO),
    m_ulInitialTimeout (QOS_CC_DEFAULT_TIMEOUT),
    m_ulLastTimeout (QOS_CC_DEFAULT_TIMEOUT),
    m_ulTimeoutIntervalCount (QOS_CC_DEFAULT_INTVL_CNT),
    m_ulMaxTimeouts (QOS_CC_DEFAULT_MAX_TIMEO),
    m_ulMinTimeoutInterval (QOS_CC_DEFAULT_MIN_TIMEO_INTVL)
{
    HX_ASSERT(pOwner);
    HX_ASSERT(pConfig);
    HX_ASSERT(pOwner->m_pTSScheduler);
    
    if ((pOwner) && (pOwner->m_pTSScheduler))
    {
        m_pOwner = pOwner;
        m_pOwner->AddRef();

        m_pSched = m_pOwner->m_pTSScheduler;
        m_pSched->AddRef();
    }

    if (pConfig)
    {
        INT32   lTemp        = 0;
        if (SUCCEEDED(pConfig->GetConfigInt(QOS_CFG_CC_TIMEO_DISABLE, lTemp)))
        {
            if (lTemp)
            {
                Stop();
                return;
            }
        }

        if (SUCCEEDED(pConfig->GetConfigInt(QOS_CFG_CC_TIMEO_INIT, lTemp)))
        {
            m_ulInitialTimeout = (UINT32)lTemp;
        }
        else
        {
            m_ulInitialTimeout = QOS_CC_DEFAULT_TIMEOUT;
        }
        
        lTemp = 0;
        if (SUCCEEDED(pConfig->GetConfigInt(QOS_CFG_CC_TIMEO_INTVL_COUNT, lTemp)))
        {
            // Make sure this is not 0
            m_ulTimeoutIntervalCount = lTemp ? (UINT32)lTemp : 1;
        }
        else
        {
            m_ulTimeoutIntervalCount = QOS_CC_DEFAULT_INTVL_CNT;
        }

        lTemp = 0;
        if (SUCCEEDED(pConfig->GetConfigInt(QOS_CFG_CC_TIMEO_MAX_TIMEOUTS, lTemp)))
        {
            m_ulMaxTimeouts = m_ulTimeoutEventCount = (UINT32)lTemp;
        }
        else
        {
            m_ulMaxTimeouts = m_ulTimeoutEventCount = QOS_CC_DEFAULT_MAX_TIMEO;
        }
    }

    if (m_pSched)
    {
        m_ulCallbackHandle = m_pSched->RelativeEnter((IHXCallback*)this, m_ulInitialTimeout);
        m_ulLastTimeout = m_ulInitialTimeout;
    }
}
 
CongestionFeedbackTimeout::~CongestionFeedbackTimeout()
{
    Stop();
}

void
CongestionFeedbackTimeout::Stop()
{
    if (m_pSched && m_ulCallbackHandle)
    {
        m_pSched->Remove(m_ulCallbackHandle);
        m_ulCallbackHandle = 0;
    }

    HX_RELEASE(m_pSched);
    HX_RELEASE(m_pOwner);
}

void
CongestionFeedbackTimeout::OnFeedback()
{
    UINT32 ulNow = HX_GET_BETTERTICKCOUNT();
   
    HX_ASSERT(m_ulTimeoutIntervalCount);

    // m_ulFeedbackInterval is the average time expected for
    // TimeoutIntervalCount intervals between feedback packets
    if (m_ulInitialFeedback > m_ulTimeoutIntervalCount)
    {
        // Update the feedback interval with the current interval -
        // Average out the last TimeoutIntervalCount - 1 intervals
        // and add the current interval.
        m_ulFeedbackInterval = m_ulFeedbackInterval - 
            (m_ulFeedbackInterval / m_ulTimeoutIntervalCount) +
            (ulNow - m_ulLastFeedback);

        if (m_ulFeedbackInterval < m_ulMinTimeoutInterval)
        {
            m_ulFeedbackInterval = m_ulMinTimeoutInterval;
        }
        m_ulLastTimeout = m_ulFeedbackInterval;
    }
    else 
    {
        // Sum the first FeedbackInterval packet intervals.
        // Use the default until we've gotten m_ulTimeoutIntervalCount
        // feedback packets to get a reasonable approximation of the
        // next expected packet time
        if (m_ulInitialFeedback)
        {
            m_ulFeedbackInterval += (ulNow - m_ulLastFeedback);
        }
        m_ulInitialFeedback++;
    }
        
    m_ulLastFeedback = ulNow;
    m_ulTimeoutEventCount = m_ulMaxTimeouts;

    if (m_pSched)
    {
        if (m_ulCallbackHandle)
        {
            m_pSched->Remove(m_ulCallbackHandle);
            m_ulCallbackHandle = 0;
        }

        m_ulCallbackHandle = m_pSched->RelativeEnter((IHXCallback*)this, 
                m_ulLastTimeout);
    }
}

STDMETHODIMP
CongestionFeedbackTimeout::Func()
{
    m_ulCallbackHandle = 0;
    
    if (m_ulTimeoutEventCount)
    {
        m_ulTimeoutEventCount--;
    }

    /* Disable congestion control if there was no initial feedback or 
       we timed out too many times 
     */
    if (((!m_ulInitialFeedback) || (!m_ulTimeoutEventCount)))
    {
        m_pOwner->FeedbackTimeout(TRUE); //Disable congestion control on timeout.
    }
    else
    {
        m_pOwner->FeedbackTimeout(FALSE); 

        m_ulLastTimeout = m_ulInitialTimeout;
        m_ulInitialFeedback = 0;
        m_ulFeedbackInterval = 0;
        
        if ((m_pSched) && m_ulLastTimeout)
        {
            m_ulCallbackHandle = m_pSched->RelativeEnter((IHXCallback*)this, m_ulLastTimeout);
        }
    }

    return HXR_OK;
}

/* IUnknown */
STDMETHODIMP
CongestionFeedbackTimeout::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXCallback*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
CongestionFeedbackTimeout::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
CongestionFeedbackTimeout::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    delete this;
    return 0;
}

  

