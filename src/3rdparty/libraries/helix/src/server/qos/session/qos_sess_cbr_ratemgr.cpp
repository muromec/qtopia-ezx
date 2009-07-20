#include "hxcom.h"
#include "hxpiids.h"
#include "hxslist.h"
#include "hxtypes.h"
#include "hxerror.h"
#include "hxstats.h"
#include "ihxpckts.h"
#include "servpckts.h"
#include "transport.h"
#include "asmrulep.h"
#include "bwcalc.h"
#include "pcktstrm.h"
#include "pcktflowmgr.h"
#include "server_engine.h"
#include "timeval.h"
#include "source.h"
#include "hxqossess.h"
#include "hxpcktflwctrl.h"
#include "qos_sess_cbr_ratemgr.h"

CCBRRateMgr::CCBRRateMgr(Process* proc):m_pProc(proc), m_ulRefCount(0)
             ,m_ulActualDeliveryRate(0), m_ulSubscribedRate(0),
              m_unStreamCount(0), m_pFlowMgr(NULL), m_pStats(NULL), m_pbufSessionId(NULL)
{
    m_tNextSendTime = 0.0;    
}


CCBRRateMgr::CCBRRateMgr(Process* proc, UINT16 unStreamCount, 
              PacketFlowManager* pFlowMgr):m_pProc(proc), m_ulRefCount(0),
                 m_ulActualDeliveryRate(0), m_ulSubscribedRate(0),
                 m_unStreamCount(unStreamCount), m_pStats(NULL), m_pbufSessionId(NULL)
{
    m_pFlowMgr = pFlowMgr;
    m_tNextSendTime = 0.0;
    m_ppStreams = new PacketStream*[m_unStreamCount];
    memset(m_ppStreams, 0, sizeof(PacketStream*) * m_unStreamCount);
}

CCBRRateMgr::~CCBRRateMgr()
{
/*
    if (m_ulRecalculatedRate)
    {
	HXAtomicSubUINT32(g_pAggregateRequestedBitRate, m_ulRecalculatedRate);
    }
*/
    HX_RELEASE(m_pbufSessionId);
    delete[] m_ppStreams;
    HX_RELEASE(m_pStats);
}

STDMETHODIMP CCBRRateMgr::QueryInterface(THIS_ REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXQoSRateManager*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXQoSRateManager))
    {
        AddRef();
        *ppvObj = (IHXQoSRateManager*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPacketFlowControl))
    {
        AddRef();
        *ppvObj = (IHXPacketFlowControl*)this;
        return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) CCBRRateMgr::AddRef(THIS)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32) CCBRRateMgr::Release(THIS)
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP CCBRRateMgr::Play (THIS)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CCBRRateMgr::StartSeek (THIS_
                          UINT32 ulTime)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CCBRRateMgr::Activate(THIS)
{
    for (UINT16 i = 0; i < m_unStreamCount; i++)
    {
        PacketStream* pStreamData = m_ppStreams[i];
        if (pStreamData && pStreamData->m_bStreamRegistered && pStreamData->m_bSentStreamDone)
        {
            for (INT32 lRule = 0; lRule < pStreamData->m_lNumRules; lRule++)
            {
	        if (pStreamData->m_pRules[lRule].m_bRuleOn &&
	            !pStreamData->m_pRules[lRule].m_bBitRateReported)
	        {
	            // Update delivery rate
	            m_ulSubscribedRate += pStreamData->m_pRules[lRule].m_ulAvgBitRate;
                    if (pStreamData->m_pRules[lRule].m_bTimeStampDelivery)
	            {
		        pStreamData->m_ulVBRAvgBitRate += pStreamData->m_pRules[lRule].m_ulAvgBitRate;
	            }
                }
            }
        }
    }
    return HXR_OK;
}

STDMETHODIMP CCBRRateMgr::WantWouldBlock(THIS)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CCBRRateMgr::SeekDone (THIS)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CCBRRateMgr::SetStartingTimestamp(THIS_
                            UINT32 ulStartingTimestamp)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CCBRRateMgr::SetEndPoint (THIS_
                           UINT32 ulEndPoint, 
                           BOOL bPause)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CCBRRateMgr::RegisterStream(THIS_
                              Transport* pTransport,
                              UINT16 uStreamNumber,
                              ASMRuleBook* pRuleBook,
                              IHXValues* pHeader)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CCBRRateMgr::GetSequenceNumber (THIS_
                                 UINT16 uStreamNumber,
                                 UINT16& uSequenceNumber)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CCBRRateMgr::Pause (THIS_
                     UINT32 ulPausePoint)
{
    return HXR_NOTIMPL;
}


STDMETHODIMP CCBRRateMgr::StreamDone (THIS_
                          UINT16 ulStreamNumber, BOOL bForce /* = FALSE */)
{
    PacketStream* pStreamData = m_ppStreams[ulStreamNumber];

    if (!pStreamData || !pStreamData->m_bStreamRegistered)
    {
	return HXR_UNEXPECTED;
    }

    for (INT32 lRule = 0; lRule < pStreamData->m_lNumRules; lRule++)
    {
        if (!pStreamData->m_pRules[lRule].m_bBitRateReported)
            continue;

        // Update delivery rate
        m_ulSubscribedRate -= pStreamData->m_pRules[lRule].m_ulAvgBitRate;
        if (pStreamData->m_pRules[lRule].m_bTimeStampDelivery)
        {
            pStreamData->m_ulVBRAvgBitRate -=
                pStreamData->m_pRules[lRule].m_ulAvgBitRate;
        }
    }
    return HXR_OK;
}

STDMETHODIMP CCBRRateMgr::SetDropRate (THIS_
                           UINT16 uStreamNumber, 
                           UINT32 uDropRate)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CCBRRateMgr::SetDropToBandwidthLimit (THIS_
                                       UINT16 uStreamNumber,
                                       UINT32 ulBandwidthLimit)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CCBRRateMgr::HandleSubscribe (THIS_
                               INT32 lRuleNumber,
                               UINT16 unStreamNumber)
{
    PacketStream* pStreamData = m_ppStreams[unStreamNumber];

    m_ulSubscribedRate += 
        pStreamData->m_pRules[lRuleNumber].m_ulAvgBitRate;
    
    if (pStreamData->m_pRules[lRuleNumber].m_bTimeStampDelivery)
    {
        pStreamData->m_ulVBRAvgBitRate += 
            pStreamData->m_pRules[lRuleNumber].m_ulAvgBitRate;
    }
    return HXR_OK;
}

STDMETHODIMP CCBRRateMgr::HandleUnSubscribe (THIS_
                                 INT32 lRuleNumber,
                                 UINT16 unStreamNumber)
{
    PacketStream* pStreamData = m_ppStreams[unStreamNumber];

    m_ulSubscribedRate -= 
        pStreamData->m_pRules[lRuleNumber].m_ulAvgBitRate; 

    if (pStreamData->m_pRules[lRuleNumber].m_bTimeStampDelivery)
    {
        pStreamData->m_ulVBRAvgBitRate -=
                pStreamData->m_pRules[lRuleNumber].m_ulAvgBitRate;
    }
    return HXR_OK;
}

STDMETHODIMP CCBRRateMgr::ControlDone (THIS)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP_(float) CCBRRateMgr::SetSpeed (THIS_
                                float fSpeed)
{
    m_ulActualDeliveryRate = 0;
//    m_pFlowMgr->RecalcConstantBitRate();
    return fSpeed;
}

// IHXPacketFlowControl methods
STDMETHODIMP CCBRRateMgr::Init(IHXSessionStats* pSessionStats, IHXBuffer* pbufsessionId, 
                               UINT16 unStreamCount, IHXServerPacketSource*  pSource)
{
    m_unStreamCount = unStreamCount;

    if (pbufsessionId)
    {
        m_pbufSessionId = pbufsessionId;
        m_pbufSessionId->AddRef();
    }

    m_ppStreams = new PacketStream*[m_unStreamCount];
    memset(m_ppStreams, 0, sizeof(PacketStream*) * m_unStreamCount); 
 
    m_pStats = pSessionStats; 
    m_pStats->AddRef();

    return HXR_OK;
}

STDMETHODIMP CCBRRateMgr::SetDeliveryBandwidth(UINT32 ulBackOff, 
                                               UINT32 ulBandwidth)
{
    HX_RESULT hr = HXR_FAIL;

    if (m_ulActualDeliveryRate != ulBandwidth)
    {
	//bReschedule = TRUE;
        hr = HXR_OK;
    }

    m_bDeliveryBandwidthSet = TRUE;
    m_ulActualDeliveryRate = ulBandwidth;
//    m_pFlowMgr->RecalcConstantBitRate();

    return hr;
}

STDMETHODIMP_(ULONG32) CCBRRateMgr::GetActualDeliveryRate (THIS) const
{
    return m_bDeliveryBandwidthSet?m_ulActualDeliveryRate:0;
}

STDMETHODIMP CCBRRateMgr::RegisterStream(PacketStream* pStreamData, UINT16 ulStreamNumber)
{
    HX_ASSERT(pStreamData);
    m_ppStreams[ulStreamNumber] = pStreamData;
    m_ppStreams[ulStreamNumber]->m_bStreamRegistered = TRUE;
    return HXR_OK;
}

STDMETHODIMP CCBRRateMgr::VerifyDelivery (THIS_ ServerPacket* pPacket, Timeval& tNextSendTime)
{
    BOOL bMoreThan1SecBehind = FALSE;
    Timeval tNow = m_pProc->pc->engine->now;
    Timeval tNextTime;
    
    // are we more than 1 second behind? or is this the first packet
    // (next send time == 0)
    if (m_tNextSendTime.tv_sec == 0 || tNow >= m_tNextSendTime + Timeval(1,0))
    {
        // XXXSMP This is basically a hack that should be reconsidered.
//        ulMsecSinceLastSend = 250;
        bMoreThan1SecBehind = TRUE;
    }
    
    HX_ASSERT(pPacket);


    UINT32 ulPacketSize = pPacket->GetSize();

    // convert packetsize (in bits) and delivery rate (in bps) to
    // useconds per bit so we know how long to wait before sending
    // again
    UINT32 ulDeliveryRate = m_bDeliveryBandwidthSet ? m_ulActualDeliveryRate :
                                                      m_ulSubscribedRate;

    INT32 ulSendDelay = (INT32)(1000000.0 * (double)ulPacketSize * 8.0 /
                                (double)ulDeliveryRate);

    // more than 1 second behind or this is the first packet
    if (bMoreThan1SecBehind)
        tNextTime = tNow + ulSendDelay;
    else
        tNextTime = m_tNextSendTime + ulSendDelay;

    // are we behind?
    if (tNow > tNextTime)
    {
        // yes.  by more than a second?
        if (tNow > tNextTime + Timeval(1,0))
        {
            // yes.  so increase next time to be 1/2 number of seconds behind
            // XXXtbradley why do we do this?  we're still setting it to be
            // in the past.  I guess so we don't try to catch up all at once
            UINT32 ulSecondsBehind = (tNow - tNextTime).tv_sec;
            tNextTime += Timeval(0.5 * ulSecondsBehind);
            (*g_pOverloads)++;
            m_pProc->pc->loadinfo->Overload(); 
        }
        else
        {
            (*g_pBehind)++;
        }
        tNextSendTime.tv_sec = 0;
        tNextSendTime.tv_usec = 0;
    }
    else
    {
        tNextSendTime = tNextTime;
    }
    m_tNextSendTime = tNextTime;
    return HXR_OK;
}

STDMETHODIMP CCBRRateMgr::Done()
{
    m_ulSubscribedRate = 0;  
    return HXR_OK;
}

STDMETHODIMP CCBRRateMgr::TimeLineStateChanged (BOOL bStart)
{
    return HXR_NOTIMPL;
}
