/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: qos_tran_cc.cpp,v 1.106 2009/06/04 21:12:52 jzeng Exp $
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

#include "safestring.h"

#define QOS_CC_MIN_SCHED_GRANULARITY 8 /* msec */
#define QOS_CC_RTP_UDP_IP_OVERHEAD   36 /* bytes */
#define QOS_CC_RTP_TCP_IP_OVERHEAD   48 /* bytes */
#define QOS_CC_RDT_UDP_IP_OVERHEAD   36 /* bytes */
#define QOS_CC_RDT_TCP_IP_OVERHEAD   48 /* bytes */
#define QOS_RC_SLOWSEND_MAXLIMIT     1
#define QOS_RC_SLOWSEND_MINLIMIT     0

#define QOS_DEBUG_OUTF(flag,args) ((flag) ? fprintf args : 0)

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

    /* adjust callback scheduling times */
    m_tNowTime(0),
    m_lXmitInterval(0),
    m_tXmitTime(0),
    m_ucIndex_0(0),
    m_lWindowTotal(0),
    m_lAvgDiff(0),

    /* Rate Shaping: */
    m_ulTokens(0),
    m_ulMaxBurst(QOS_CC_DEFAULT_BURST_LIMT),
    m_bPassThrough (FALSE),
    m_bCallbacksStarted (FALSE),
    m_ulStartRate (QOS_CC_DEFAULT_START_RATE),
    m_ulAvgPacketSize (QOS_CC_DEFAULT_PKT_SIZE << AVG_PKT_SZ_FILTER),
    m_nProtocolOverhead (QOS_CC_RTP_UDP_IP_OVERHEAD),
    m_fMaxOversendRate (QOS_CC_DEFAULT_SCALAR),
    m_fInitialOversendRate (QOS_CC_DEFAULT_INIT_OSR),
    m_ulChannelRate (QOS_CC_DEFAULT_MAX),
    m_ulMaxRate (QOS_CC_DEFAULT_MAX),
    m_fSlowdownRate (QOS_RC_DEFAULT_SLOWSEND_SCALAR),
    m_ulSDBRate (0),
    m_bUseSDB (FALSE),
    m_ulMediaRate(0),
    m_ulTotalRate (0),
    m_unStreamNumber(0),
    m_bBlocked (FALSE),
    m_ulCallbackHandle(0),
    m_ulTotalBandwidth(0),
    m_pAggregator (NULL),
    m_bDeliverBelowLowestMediaRate (FALSE),
    m_ulLowestMediaRate(0),
    m_bUndersendStarted(FALSE),
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
    m_pWindowedRRSignal(NULL),
    m_ulPacketsSinceLastFeedback(0),
    m_ulBytesSinceLastFeedback(0),
    m_fWindowedReceiveRate(0.0),
    m_pDebugFile (NULL),

#ifdef HELIX_FEATURE_SERVER_FCS
    m_bIsFCS(FALSE),
#endif

    /* LDA related */
    m_pLDA(NULL),
    m_pLDAResponse(NULL)
{
    if (m_pContext)
    {
        IHXCommonClassFactory* pCCF = NULL;
        IHXBuffer*             pSigBuf = NULL;
        IHXBuffer*             pRTTSigBuf = NULL;
        IHXBuffer*             pWindowedRRSigBuf = NULL;

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
            SUCCEEDED(pCCF->CreateInstance(CLSID_IHXQoSSignal,
                        (void**)&m_pWindowedRRSignal)) &&
            SUCCEEDED(pCCF->CreateInstance(CLSID_IHXBuffer,
                        (void**)&pSigBuf)) &&
            SUCCEEDED(pCCF->CreateInstance(CLSID_IHXBuffer,
                        (void**)&pRTTSigBuf)) &&
            SUCCEEDED(pCCF->CreateInstance(CLSID_IHXBuffer,
                        (void**)&pWindowedRRSigBuf)))
        {
            pSigBuf->SetSize(sizeof(RateSignal));
            pRTTSigBuf->SetSize(sizeof(RTTSignal));
            pWindowedRRSigBuf->SetSize(sizeof(WindowedRRSignal));

            m_pThroughputSignal->SetId(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                                             HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                                             HX_QOS_SIGNAL_COMMON_THRUPUT));

            m_pRTTSignal->SetId(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_APPLICATION,
                                                             HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                                             HX_QOS_SIGNAL_COMMON_RTT));

            m_pWindowedRRSignal->SetId(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_APPLICATION,
                                                             HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                                             HX_QOS_SIGNAL_WINDOWED_RECEIVE_RATE));

            m_pThroughputSignal->SetValue(pSigBuf);
            m_pRTTSignal->SetValue(pRTTSigBuf);
            m_pWindowedRRSignal->SetValue(pWindowedRRSigBuf);
        }

        HX_RELEASE(pCCF);
        HX_RELEASE(pSigBuf);
        HX_RELEASE(pRTTSigBuf);
        HX_RELEASE(pWindowedRRSigBuf);
    }

    /*
     * xxx aak:
     * keep track of the number of packets received, lost from incoming feeds
     * the info on lost packets is to b used in conjunction with packet loss
     * from client's receiver reports (rtcp) or rdt metrics (rdt) to calculate
     * the actual packet loss and used in the congestion control equation.
     *
     * RTP/RTCP:
     * m_ulCumInPktLoss = the incoming cumulative packet loss
     * m_ulLastRRCumInPktLoss = the incoming cumulative packet loss saved
     *   when the last client receiver report was received
     * m_pulCumInPktLossHistory[] = array for storing incoming pkt loss
     *   history. so at each pkt the cumulative incoming pkt loss is stored.
     *   this value (along with the other 2 vars above) is used to calculate
     *   the total outgoing pkt loss as seen by the client.
     *
     * RDT:
     * handled in class QoSMetrics. by not counting client NAKd pkts for 
     * packets that were lost incoming from the live source, the correct pkt
     * loss is calculated.
     */
    m_ulCumInPktLoss = 0;
    m_ulLastRRCumInPktLoss = 0;
    m_pulCumInPktLossHistory = new UINT32[RTP_PKT_HISTORY_SIZE];
    memset(m_pulCumInPktLossHistory, 0, sizeof(UINT32) * RTP_PKT_HISTORY_SIZE);

    m_tRateTimer.tv_sec  = 0;
    m_tRateTimer.tv_usec = 0;
    memset(m_lDiffWindow, 0, sizeof(INT32) * QOS_CC_INTERVAL_DIFF_WINDOW_SIZE);
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

    if (m_pAggregator)
    {
        m_pAggregator->RemoveRateShaper((IHXQoSRateShaper*)this);
        m_pAggregator->Release();
        m_pAggregator = NULL;
    }

    if (m_pLDA)
    {
	m_pLDA->Done();
	HX_RELEASE(m_pLDAResponse);
	m_pLDA->Release();
	m_pLDA = NULL;
    }

    if(m_pDebugFile)
    {
        fclose(m_pDebugFile);
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
    HX_RELEASE(m_pWindowedRRSignal);

    HX_DELETE(m_pLinkCharParams);
    HX_DELETE(m_pulCumInPktLossHistory);
}

/* IHXQoSCongestionCtl */
STDMETHODIMP
QoSCongestionCtl::Init(IHXBuffer* pSessionId,
                       UINT16             unStrmNumber,
                       UINT32 /* bytes */ ulMediaRate,
                       UINT32 /* bytes */ ulPacketSize,
                       BOOL bIsFCS)
{
    if ((!pSessionId))
    {
        HX_ASSERT(0);
        return HXR_INVALID_PARAMETER;
    }

#ifdef HELIX_FEATURE_SERVER_FCS
    m_bIsFCS = bIsFCS;
#endif
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

    // Send signal that TCP transport is being used
    // This is used by the rate manager
    if (m_pContext)
    {
        IHXCommonClassFactory* pCCF = NULL;
        IHXQoSSignal* pTCPTransportSignal = NULL;
        HX_VERIFY(SUCCEEDED(m_pContext->QueryInterface(IID_IHXCommonClassFactory,
            (void**)&pCCF)));

        if (m_pSignalBus && pCCF &&
            SUCCEEDED(pCCF->CreateInstance(CLSID_IHXQoSSignal,
                        (void**)&pTCPTransportSignal)))
        {
            pTCPTransportSignal->SetId(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT,
                                                             HX_QOS_SIGNAL_RELEVANCE_METRIC,
                                                             HX_QOS_SIGNAL_COMMON_TCP));
            m_pSignalBus->Send(pTCPTransportSignal);
        }
        HX_RELEASE(pCCF);
        HX_RELEASE(pTCPTransportSignal);
    }

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

    IHXQoSProfileConfigurator* pQoSConfig = NULL;
    INT32 lTemp                        = 0;
    IHXBuffer* pBuffer                 = NULL;

    HX_RELEASE(m_pSignalBus);
    m_pSignalBus = pBus;
    m_pSignalBus->AddRef();

    /* Configure congestion control: */
    HX_VERIFY(SUCCEEDED(m_pSignalBus->QueryInterface(IID_IHXQoSProfileConfigurator,
                                                     (void**)&pQoSConfig)));
    /* Connect to the signal bus: */
    m_pSignalBus->AttachListener(MAKE_HX_QOS_SIGNAL_ID(HX_QOS_SIGNAL_LAYER_FRAMING_TRANSPORT, 0,0),
                                 (IHXQoSSignalSink*)this);

    if (SUCCEEDED(pQoSConfig->GetConfigInt(DEBUG_OUTPUT, m_lDebugOutput)) &&
        (m_lDebugOutput & (DUMP_CHANNELRATE | DUMP_ALL)))
    {
        // We shouldn't ever create a debugfile if we don't enable tracing.
        HX_ASSERT(m_lDebugOutput || !m_pDebugFile);

        if (!m_pDebugFile && m_lDebugOutput != 0)
        {
            char szDebugFile[128];
            SafeSprintf(szDebugFile, 128, "CC_%s_%d.txt",
	            (const char*)pSessionId->GetBuffer(), m_unStreamNumber);

            m_pDebugFile = fopen(szDebugFile, "a+");
        }
    }

    lTemp = 0;
    if (SUCCEEDED(pQoSConfig->GetConfigInt(QOS_CFG_RC_MAX_BURST, lTemp)))
    {
        m_ulMaxBurst = (UINT32)lTemp;
    }
    else
    {
        m_ulMaxBurst = QOS_CC_DEFAULT_BURST_LIMT;
    }

    if (SUCCEEDED(pQoSConfig->GetConfigInt
                           (QOS_CFG_RA_ENABLE_DELIVER_BELOW_LOWEST_MEDIA_RATE, lTemp)))
    {
        m_bDeliverBelowLowestMediaRate = (BOOL)(lTemp);
    }

    //Initialize Channel rate from the Config only if Link Characteristics
    // are not explicitly passed by clients using 3GPP Link-Char header
    if (!m_pLinkCharParams)
    {
            lTemp = 0;
	if (SUCCEEDED(pQoSConfig->GetConfigInt(QOS_CFG_RC_MAX_SENDRATE, lTemp)))
            {
            m_ulChannelRate = (UINT32)lTemp;
            }
            else
            {
            m_ulChannelRate = QOS_CC_DEFAULT_MAX;
            }
    }

    lTemp = 0;
    IHXBuffer* pBuff = NULL;
    if (SUCCEEDED(pQoSConfig->GetConfigBuffer(QOS_CFG_RC_MAX_OSR,pBuff))
        && pBuff && pBuff->GetBuffer())
    {
        m_fMaxOversendRate = (double)(atof((const char*) pBuff->GetBuffer()))/100.0;
    }
    else if (SUCCEEDED(pQoSConfig->GetConfigInt(QOS_CFG_RC_MAX_OSR, lTemp)))
    {
        m_fMaxOversendRate = (double)((double)lTemp/100.0);
    }
    else
    {
        m_fMaxOversendRate = QOS_CC_DEFAULT_SCALAR;
    }
    HX_RELEASE(pBuff);

    m_ulStartRate = QOS_CC_DEFAULT_START_RATE;
    m_bPassThrough = FALSE;

    lTemp = 0;
    if (SUCCEEDED(pQoSConfig->GetConfigInt(QOS_CFG_RC_INIT_PKTSZ, lTemp)))
    {
        m_ulAvgPacketSize = lTemp << AVG_PKT_SZ_FILTER;
    }
    else
    {
        m_ulAvgPacketSize = QOS_CC_DEFAULT_PKT_SIZE  << AVG_PKT_SZ_FILTER;
    }

    lTemp = 0;
    if (SUCCEEDED(pQoSConfig->GetConfigInt(QOS_CFG_RC_USE_SDB, lTemp)))
    {
        m_bUseSDB = (BOOL)lTemp;
    }
    else
    {
        m_bUseSDB = FALSE;
    }

    lTemp = 0;
    if (SUCCEEDED(pQoSConfig->GetConfigBuffer(QOS_CFG_RC_INIT_OSR, pBuffer))
                  && pBuffer && pBuffer->GetBuffer())
    {
        m_fInitialOversendRate = atof((const char*)pBuffer->GetBuffer())/100.0;
        HX_RELEASE(pBuffer);
    }
    else
    {
        if (SUCCEEDED(pQoSConfig->GetConfigInt(QOS_CFG_RC_INIT_OSR, lTemp)))
        {
            m_fInitialOversendRate = (double)((double)lTemp/100.0);
        }
        else
        {
            m_fInitialOversendRate = QOS_CC_DEFAULT_INIT_OSR;
        }
    }

    lTemp = 0;
    if (SUCCEEDED(pQoSConfig->GetConfigBuffer(QOS_CFG_RC_SLOWDOWN_RATE, pBuffer))
                  && pBuffer && pBuffer->GetBuffer())
    {
        m_fSlowdownRate = atof((const char*)pBuffer->GetBuffer())/100.0;
        HX_RELEASE(pBuffer);
    }
    else
    {
        if (SUCCEEDED(pQoSConfig->GetConfigInt(QOS_CFG_RC_SLOWDOWN_RATE, lTemp)))
        {
            m_fSlowdownRate = (double)((double)lTemp/100.0);
        }
        else
        {
            m_fSlowdownRate = QOS_RC_DEFAULT_SLOWSEND_SCALAR;
        }
    }

    if (m_fSlowdownRate < QOS_RC_SLOWSEND_MINLIMIT || m_fSlowdownRate > QOS_RC_SLOWSEND_MAXLIMIT)
    {
        m_fSlowdownRate = QOS_RC_DEFAULT_SLOWSEND_SCALAR;
    }

    UpdateMaxRate();

    /* Don't set up congestion control or rate shaping if we are in pass through or TCP mode: */
    if (m_bPassThrough)
    {
        HX_RELEASE(pQoSConfig);
        return HXR_OK;
    }

    if (m_bConfigureTCPMode)
    {
        ConfigureTCPMode();
        HX_RELEASE(pQoSConfig);
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
            if (SUCCEEDED(pQoSFactory->CreateInstance(pBus,
                                                      CLSID_IHXQoSCongestionEquation,
                                                      (void**)&m_pCongestionEqn)))
            {
                m_pCongestionEqn->Init(m_pContext, m_pSessionId, m_unStreamNumber);
                m_pCongestionEqn->SetMediaRate(m_ulMediaRate, TRUE);
                m_pCongestionEqn->SetMediaPacketSize(m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER);

		BOOL bEnableLossDiscrimination = FALSE;
		lTemp = 0;
		if (SUCCEEDED(pQoSConfig->GetConfigInt(QOS_CFG_RC_LDA_ENABLE, lTemp)))
		{
		    bEnableLossDiscrimination = (BOOL)lTemp;
		}
		if (bEnableLossDiscrimination)
		{
		    if (!m_pLDA)
		    {
			HX_VERIFY(SUCCEEDED(m_pContext->QueryInterface(IID_IHXPacketLossDiscriminationAlgorithm,
			    (void**)&m_pLDA)));
			m_pLDA->Init(pQoSConfig, m_unStreamNumber, m_ulMediaRate);
		    }
		    if (SUCCEEDED(
			m_pCongestionEqn->QueryInterface(IID_IHXPacketLossDiscriminationAlgorithmResponse,
			(void**)&m_pLDAResponse)))
		    {
			m_pLDA->SetLDAResponse(m_pLDAResponse);
		    }
		    else
		    {
			m_pLDA->Release();
			m_pLDA = NULL;
		    }
		}

                m_pSignalBus->QueryInterface(IID_IHXQoSTransportAdaptationInfo,
                                             (void**)&m_pTransportInfo);
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

    QOS_DEBUG_OUTF(m_pDebugFile,
        (m_pDebugFile,  "Max Burst: %u\n"
                        "Deliver Below Lowest MediaRate: %s\n"
                        "Initial Channel Rate: %u\n"
                        "Max Oversend Rate: %.2f\n"
                        "Average Packet Size: %u\n"
                        "Use Client Rate Req: %s\n"
                        "Initial Oversend Rate: %.2f\n",
            m_ulMaxBurst,
            (m_bDeliverBelowLowestMediaRate?"Yes":"No"),
            m_ulChannelRate,
            m_fMaxOversendRate,
            m_ulAvgPacketSize,
            (m_bUseSDB?"Yes":"No"),
            m_fInitialOversendRate));

    HX_RELEASE(pQoSConfig);
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

    UINT32 ulPacketSize = pPacket->GetSize();
    UINT32 ulPacketSeqNo = pPacket->m_uSequenceNumber;
    BOOL bIsLost = pPacket->IsLost();

    m_ulCumInPktLoss += bIsLost ? 1 : 0;
    m_pulCumInPktLossHistory[ulPacketSeqNo % RTP_PKT_HISTORY_SIZE]
	= m_ulCumInPktLoss;

    ++m_ulPacketsSinceLastFeedback;
    m_ulBytesSinceLastFeedback += ulPacketSize + m_nProtocolOverhead;

    if (m_ulAvgPacketSize != 0)
    {
        INT16 nDelta = 0;
        nDelta = (ulPacketSize + m_nProtocolOverhead) - 1 - (m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER);
        if ((m_ulAvgPacketSize += nDelta) <= 0)
        {
            m_ulAvgPacketSize = 1;
        }
    }
    else
    {
        m_ulAvgPacketSize = (ulPacketSize + m_nProtocolOverhead) << AVG_PKT_SZ_FILTER;
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

            m_lXmitInterval = TransmitInterval(m_ulTokens, ulInitialRate,
                (m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER));
            HXTimeval hxtv = m_pScheduler->GetCurrentSchedulerTime();
            m_tXmitTime.tv_sec = hxtv.tv_sec;
            m_tXmitTime.tv_usec = hxtv.tv_usec;
            m_tXmitTime += (m_lXmitInterval * 1000);

            m_ulCallbackHandle = m_pTSScheduler->
                RelativeEnter((IHXCallback*)this, m_lXmitInterval);
        }
    }

    if (!m_bPassThrough || m_bUndersendStarted)
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
    m_ulBitAccumulator += ulPacketSize * 8;

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

/**
 * \brief Func - queues tokens and re-schedule this callback
 *
 * m_ulMaxBurst tokens are queued and this callback is re-scheduled. The
 * re-scheduling algorithm is based on a simple running average of the time
 * difference between the scheduled time m_tXmitTime and the actual execution
 * time m_tNowTime.
 *
 * basically, the last 8 time diffs are stored, averaged and subtracted from
 * the next scheduled time (transmit interval).
 *
 * a positive value for the diff means that the callback was delayed
 * a negative value for the diff means that the callback executed early
 *
 * so if the average is a +ve value then the next callback will be scheduled
 * early by subtracting the +ve average from the calculated scheduled time. if
 * the average is -ve then the next callback will b delayed by the absolute
 * value of the -ve amount, in other words the value will be added to the
 * calculated transmit time.
 *
 * m_lXmitInterval = TransmitInterval(...) - m_lAvgDiff;
 * ...
 * m_tXmitTime = Current scheduler time + (m_lXmitInterval * 1000)
 *
 * \param None
 *
 * \return HX_RESULT: Always HXR_OK
 */
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
        ulAttemptedRate = (!m_bDeliverBelowLowestMediaRate &&
                            ulAttemptedRate < m_ulLowestMediaRate) ?
                           m_ulLowestMediaRate : ulAttemptedRate;
        ulReportedRate = ulAttemptedRate;
        m_pCongestionEqn->SetRate(ulReportedRate);
    }
    else
    {
        ulAttemptedRate = (m_ulSDBRate) ? m_ulSDBRate : m_ulMaxRate;
        ulReportedRate = ulAttemptedRate;
    }

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

    HXTimeval hxtv = m_pScheduler->GetCurrentSchedulerTime();
    m_tNowTime.tv_sec = hxtv.tv_sec;
    m_tNowTime.tv_usec = hxtv.tv_usec;

    // find the difference between the scheduled time for this callback and
    // the current time (actual execution time) in milli-secs
    Timeval tIntervalDiff = m_tNowTime - m_tXmitTime;
    INT32 lIntervalDiff = tIntervalDiff.tv_sec * 1000 + tIntervalDiff.tv_usec / 1000;
    INT32 lOldVal = m_lDiffWindow[m_ucIndex_0 % QOS_CC_INTERVAL_DIFF_WINDOW_SIZE];

    // calculate the windowed average
    m_lDiffWindow[m_ucIndex_0 % QOS_CC_INTERVAL_DIFF_WINDOW_SIZE] = lIntervalDiff;
    m_ucIndex_0++;
    m_lWindowTotal = m_lWindowTotal - lOldVal + lIntervalDiff;
    m_lAvgDiff = m_lWindowTotal / QOS_CC_INTERVAL_DIFF_WINDOW_SIZE;

    /* Log debug statistics */
    UINT32 ulNow = HX_GET_BETTERTICKCOUNT() - m_ulSessionStartTime;

    if (!m_bPassThrough || m_bUndersendStarted)
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

    if (m_bUndersendStarted == TRUE)
    {
        // Use the slowsend rate for token bucket calculations
        UINT32 ulSlowdownRate = (UINT32)(m_ulMediaRate * m_fSlowdownRate);
        if (ulSlowdownRate < ulAttemptedRate)
        {
            // case where the bandwidth drops when undersending
            ulAttemptedRate = ulSlowdownRate;
        }
    }

    QOS_DEBUG_OUTF(m_pDebugFile,
        (m_pDebugFile,  "%10u AvgOutboundRate: %7u MediaRate: %7u ChannelRate: %7u "
        "AttemptedRate: %7u MaxRate: %7u AvgPacketSize: %4u StreamNumber: %2u WindowedAvgDff: %3d \n",
        ulNow, (UINT32)m_fAvgOutboundRate, m_ulMediaRate, m_ulChannelRate,
        ulAttemptedRate, m_ulMaxRate, m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER, m_unStreamNumber, m_lAvgDiff));

    UINT32 ulAttemptedBytesPerSec = ulAttemptedRate / 8;
    m_lXmitInterval = TransmitInterval(m_ulMaxBurst, ulAttemptedBytesPerSec, m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER) - m_lAvgDiff;

    // calculate the next scheduled time for this callback
    hxtv = m_pScheduler->GetCurrentSchedulerTime();
    m_tXmitTime.tv_sec = hxtv.tv_sec;
    m_tXmitTime.tv_usec = hxtv.tv_usec;
    if (m_lXmitInterval < 0)
        m_lXmitInterval = 0;
    else
        m_tXmitTime += (m_lXmitInterval * 1000);

    /* Schedule the next callback */
    m_ulCallbackHandle = m_pTSScheduler->RelativeEnter((IHXCallback*)this, m_lXmitInterval);

    return HXR_OK;
}

/**
 * \brief AdjustInitialSendingRate - adjust the initial sending rate of each stream based on bw info
 *
 * Calculate initial sending rate based on bw info, with priority sdb > linkchar > bandwidth, the
 * max is capped at a user specified multiple of the media rate.
 *
 * \return UINT32: Calculated initial sending rate
 */
UINT32 QoSCongestionCtl::AdjustInitialSendingRate()
{
    // priority SDB > linkchar > bandwidth
    UINT32 ulMaxStreamRate = 0;

    if (m_bUseSDB && m_ulSDBRate)
    {
        ulMaxStreamRate = m_ulSDBRate * m_ulMediaRate / m_ulTotalRate;
    }
    else if (m_pLinkCharParams)
    {
        if (m_pLinkCharParams->m_bSessionAggregate)
        {
            ulMaxStreamRate = m_pLinkCharParams->m_ulGuaranteedBW * m_ulMediaRate / m_ulTotalRate;
        }
        else
        {
            // GBW is per stream, so no need to divide by total rate to get max stream rate.
            ulMaxStreamRate = m_pLinkCharParams->m_ulGuaranteedBW * m_ulMediaRate;
        }
    }
    else if (m_ulTotalBandwidth)
    {
        ulMaxStreamRate = m_ulTotalBandwidth * ((double)m_ulMediaRate / m_ulTotalRate);
    }
    // else ulMaxStreamRate is 0 so we return the media rate.

    return (ulMaxStreamRate >= m_ulMediaRate) ? min(ulMaxStreamRate, (UINT32)(m_ulMediaRate * m_fInitialOversendRate))
                                              : m_ulMediaRate;
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
    UINT32        ulStreamNumber = MAX_UINT16;

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
                m_pCongestionEqn->Update(m_ulLastRecvd,
					 m_ulNumLost,
                                         m_fRecvdRate, m_fAvgOutboundRate,
                                         m_fLastLoss, m_fRTT);
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
	    if (m_pLDA)
		m_pLDA->LostPackets(pMetrics->m_fRTT, pMetrics->m_fLoss,
		    pMetrics->m_ulNumLost, m_fWindowedReceiveRate,
		    pMetrics->m_ulNumRecvd);

            UpdateMaxRate();
            SignalRDTWindowedReceiveRate (pMetrics);

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
                        pSignal->m_fRTT = pMetrics->m_fRTT;
                    }
                }
                HX_RELEASE(pTmp);
                m_pSignalBus->Send(m_pRTTSignal);
            }

            if (m_pCongestionEqn)
            {
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
                        pSignal->m_fRTT = *pRTT;
                    }
                }
                HX_RELEASE(pTmp);
                m_pSignalBus->Send(m_pRTTSignal);
            }

            if (m_pCongestionEqn)
            {
                m_pCongestionEqn->Update(0, 0, (double)m_ulMediaRate,
                                         m_fAvgOutboundRate, 0, *pRTT);
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
                    if (HX_QOS_SIGNAL_COMMON_INIT_MEDIA_RATE == (ulId & HX_QOS_SIGNAL_ID_MASK))
                    {
                        UINT32 ulInitialSendingRate;
                        m_pCongestionEqn->SetMediaRate(m_ulMediaRate, TRUE);
                        ulInitialSendingRate = AdjustInitialSendingRate();
                        m_pCongestionEqn->SetRate(ulInitialSendingRate);
                    }
                    else
                    {
                        m_pCongestionEqn->SetMediaRate(m_ulMediaRate, FALSE);
                    }
                }
		if (m_pLDA)
		    m_pLDA->SetMediaRate(m_ulMediaRate);
            }
        }

        HX_RELEASE(pBuffer);
        break;
    }
    case HX_QOS_SIGNAL_COMMON_LOWEST_MEDIA_RATE:
    {
        pBuffer = NULL;
        pSignal->GetValue(pBuffer);

        if (pBuffer && !m_bDeliverBelowLowestMediaRate)
        {
            RateSignal* pRateSignal = (RateSignal*)pBuffer->GetBuffer();

            if (pRateSignal && (pRateSignal->m_unStreamNumber == m_unStreamNumber))
            {
                m_ulLowestMediaRate = pRateSignal->m_ulRate;
            }
        }

        HX_RELEASE(pBuffer);
        break;
    }
    case HX_QOS_SIGNAL_COMMON_SDB:
    {
#ifdef HELIX_FEATURE_SERVER_FCS
        if(m_bIsFCS && !m_pCongestionEqn && m_bUseSDB)
        {
            pSignal->GetValue(pBuffer);
            if (pBuffer)
            {
                RateSignal* pRateSignal = (RateSignal*)pBuffer->GetBuffer();
                //If ulRate is not zero, this tells us that we have received this signal for FCS
                //Now, we have already sent data for preroll so set m_fMaxOversendRate to 1.0
                //to send now by media rate
                if (pRateSignal->m_ulRate)
                {
                    m_fMaxOversendRate = 1.0;
                }   

                // Send at the media rate:
                m_ulMaxRate = AdjustRateForOverhead(m_ulMediaRate);
            }
            HX_RELEASE(pBuffer);
        }
#endif
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
    case HX_QOS_SIGNAL_COMMON_BEGIN_UNDERSEND:
    {
        // need to change the send rate to the media rate of
        // lowest available stream rate
        // Enter into undersend mode only when the stream number
        // in the signal matches ours or is aggregate
        pSignal->GetValueUINT32(ulStreamNumber);
        if (ulStreamNumber == m_unStreamNumber ||
            ulStreamNumber == MAX_UINT16)
        {
            m_bUndersendStarted = TRUE;

            UINT32 ulNow = HX_GET_BETTERTICKCOUNT() - m_ulSessionStartTime;
            QOS_DEBUG_OUTF(m_pDebugFile, (m_pDebugFile,  "%10u Entering Undersend Mode\n", ulNow));
        }
        break;
    }
    case HX_QOS_SIGNAL_COMMON_END_UNDERSEND:
    {
        // Switch back to normal server behavior
        // Exit from undersend mode only when the stream number
        // in the signal matches ours or is aggregate
        pSignal->GetValueUINT32(ulStreamNumber);
        if (ulStreamNumber == m_unStreamNumber ||
            ulStreamNumber == MAX_UINT16)
        {
            m_bUndersendStarted = FALSE;

            UINT32 ulNow = HX_GET_BETTERTICKCOUNT() - m_ulSessionStartTime;
            QOS_DEBUG_OUTF(m_pDebugFile, (m_pDebugFile,  "%10u Exiting Undersend Mode\n", ulNow));
        }
        break;
    }
    case HX_QOS_SIGNAL_FEEDBACK_TIMEOUT:
    {
        FeedbackTimeout();
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

    HX_RELEASE(m_pTSScheduler);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pCongestionEqn);
    return HXR_OK;
}

UINT32
QoSCongestionCtl::ComputeAveragePacketSizeAndClear()
{
    UINT32 ulAvgPacketSize = 0UL;

    if (m_ulPacketsSinceLastFeedback)
    {
        ulAvgPacketSize = m_ulBytesSinceLastFeedback / m_ulPacketsSinceLastFeedback;
    }

    m_ulPacketsSinceLastFeedback = m_ulBytesSinceLastFeedback = 0UL;

    return ulAvgPacketSize;
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

    /*
     * determine the number of incoming packets lost for the current RR
     * so that it can be subtracted from the rr->lost var to give the actual
     * packets lost in the client
     */
    UINT32 ulCurrRRCumInPktLoss = m_pulCumInPktLossHistory[unHighSeq % RTP_PKT_HISTORY_SIZE];
    UINT32 ulCurrRRInPktLoss = 0;
    ulCurrRRInPktLoss = ulCurrRRCumInPktLoss - m_ulLastRRCumInPktLoss;

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

    /*
     * determine the number of packets lost taking into account incoming pkt
     * loss:
     */
    m_ulNumLost = ((pRR->lost > m_ulLastLoss) ? pRR->lost - m_ulLastLoss : 0);
    if (ulCurrRRInPktLoss > 0 && m_ulNumLost > 0)
	m_ulNumLost -= ulCurrRRInPktLoss;

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
                                  ((ulRTT & 0x0000ffff) << 16));
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

        m_fRecvdRate = fPPS * m_ulAvgPacketSize; // bits-per-sec

        SignalRTPWindowedReceiveRate(fPPS);
    }

    /* update loss: */
    m_ulLastLoss = (pRR->lost > 0) ? pRR->lost : 0;
    m_ulLastRRCumInPktLoss = ulCurrRRCumInPktLoss;

    m_tLastRR = tvArrivalTime;

    if (m_pLDA)
	m_pLDA->LostPackets(ulRTT, m_fRTT, pRR->jitter, pRR->fraction,
	    m_ulNumLost, m_fWindowedReceiveRate, m_ulLastRecvd);

    if (m_ulTotalAckd)
    {
        m_fLastLoss = (double) ((double)pRR->lost)/((double)m_ulTotalAckd);
    }

    return HXR_OK;
}

void
QoSCongestionCtl::SignalRDTWindowedReceiveRate(RDTTransportMetrics *pMetrics)
{
    m_fWindowedReceiveRate = pMetrics->m_fRecvRate * (1 << AVG_PKT_SZ_FILTER);

    //printf("TDK:QoSCongestionCtl::SignalRDTWindowedReceiveRate: this=%p m_unStreamNumber=%u m_fWindowedReceiveRate=%7.2f\n", this, m_unStreamNumber, m_fWindowedReceiveRate); fflush(stdout); //TDK:DEBUG
    SignalWindowedReceiveRate();
}

void
QoSCongestionCtl::SignalRTPWindowedReceiveRate(float fPPS)
{
    UINT32 ulAvgPacketSize = ComputeAveragePacketSizeAndClear();
    m_fWindowedReceiveRate = fPPS * ulAvgPacketSize * (1 << AVG_PKT_SZ_FILTER);
    //printf("TDK:QoSCongestionCtl::SignalRTPWindowedReceiveRate: this=%p m_unStreamNumber=%u m_fWindowedReceiveRate=%7.2f\n", this, m_unStreamNumber, m_fWindowedReceiveRate); fflush(stdout); //TDK:DEBUG

    SignalWindowedReceiveRate();
}

void
QoSCongestionCtl::SignalWindowedReceiveRate()
{
    if (!m_pSignalBus)
    {
        return;
    }

    /* Notify the stack of the new Windowed RR */
    IHXBuffer* pTmp = NULL;

    if (SUCCEEDED(m_pWindowedRRSignal->GetValue(pTmp)))
    {
        WindowedRRSignal* pSignal = (WindowedRRSignal*)pTmp->GetBuffer();

        if (pSignal)
        {
            pSignal->m_unStreamNumber = m_unStreamNumber;
            pSignal->m_fWindowedReceiveRate = m_fWindowedReceiveRate;
        }
    }

    HX_RELEASE(pTmp);
    m_pSignalBus->Send(m_pWindowedRRSignal);
}

void
QoSCongestionCtl::FeedbackTimeout()
{
    HX_RELEASE(m_pCongestionEqn);

    if (m_lDebugOutput & (DUMP_CHANNELRATE | DUMP_ALL))
    {
        /* Log debug statistics */
        UINT32 ulNow = HX_GET_BETTERTICKCOUNT() - m_ulSessionStartTime;

        QOS_DEBUG_OUTF(m_pDebugFile,
            (m_pDebugFile,  "Received Feedback Timeout Signal: (timestamp: %10u)\n",
                ulNow));
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

    return ulInterval;
}

void
QoSCongestionCtl::UpdateMaxRate()
{
    if (m_pCongestionEqn ||
        m_bUseSDB ||
        m_pLinkCharParams ||
        (m_bTCP))
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
        HX_RELEASE(pConfig);

        m_nProtocolOverhead = QOS_CC_RDT_TCP_IP_OVERHEAD;

        //Only schedule token bucket updates if we are not in pass through mode
        if (m_ulCallbackHandle)
        {
            m_pTSScheduler->Remove(m_ulCallbackHandle);
            m_ulCallbackHandle = 0;
        }

        m_lXmitInterval = TransmitInterval(m_ulTokens, m_ulMaxRate,
            (m_ulAvgPacketSize >> AVG_PKT_SZ_FILTER));

        HXTimeval hxtv = m_pScheduler->GetCurrentSchedulerTime();
        m_tXmitTime.tv_sec = hxtv.tv_sec;
        m_tXmitTime.tv_usec = hxtv.tv_usec;
        m_tXmitTime += (m_lXmitInterval * 1000);

        m_ulCallbackHandle = m_pTSScheduler->
            RelativeEnter((IHXCallback*)this,  m_lXmitInterval);

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
    m_ulChannelRate = m_pLinkCharParams->m_ulMaxBW;

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
QoSCongestionCtl::PrintLinkCharDebugInfo(LinkCharParams* pParams)
{
    if (m_lDebugOutput & (DUMP_CHANNELRATE | DUMP_ALL))
    {
        /* Log debug statistics */
        UINT32 ulNow = HX_GET_BETTERTICKCOUNT() - m_ulSessionStartTime;

        QOS_DEBUG_OUTF(m_pDebugFile,
            (m_pDebugFile,  "%10u 3GPP-Link-Char: Session=%s; MBW=%u GBW=%u\n",
            ulNow, m_pSessionId ? (const char*)m_pSessionId->GetBuffer() : "UNKNOWN",
            pParams->m_ulMaxBW,
            pParams->m_ulGuaranteedBW));
    }
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
