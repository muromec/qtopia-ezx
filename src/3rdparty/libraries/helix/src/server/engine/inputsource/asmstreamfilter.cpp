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

#include "hxcom.h"
#include "asmstreamfilter.h"
#include "proc.h"

#include "ihxpckts.h"
#include "hxccf.h"
#include "hxerror.h"
#include "hxstats.h"
#include "hxstrutl.h"

#include "hxqos.h"
#include "hxqosinfo.h"
#include "qos_cfg_names.h"

#include "ispifs.h"

#include "hxclientprofile.h"
#include "servpckts.h"

#include "uberstreammgr.h"
#include "streamselector.h"
#include "ratedescmgr.h"


#define DISABLE_EXTERNAL_ASM_SUB


// 1 sec
const char* CONFIG_MIN_PREROLL = "InputSource.MinPreroll";

/* Per User-Agent */
const char* CONFIG_ACCEPT_INIT_EXTERNAL_SUB =
    "InputSource.StaticPushSource.AllowInitialExternalSubscription";
const char* CONFIG_ACCEPT_INIT_INTERNAL_SUB =
    "InputSource.StaticPushSource.AllowInitialInternalSubscription";
const char* DUMP_ASMHANDLING = "InputSource.StaticPushSource.DumpASMHandling";
const char* DUMP_ALLRATE = "InputSource.StaticPushSource.DumpAllRates";
const char* DUMP_PKT = "InputSource.StaticPushSource.DumpPkt";
const char* DUMP_SELECTION = "InputSource.StaticPushSource.DumpSelection";
const char* DUMP_SUBSCRIPTION = "InputSource.StaticPushSource.DumpSubscription";
const char* DUMP_ADAPTATION = "InputSource.StaticPushSource.DumpAdaptation";

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::CASMStreamFilter
// Purpose:
//  Constructor.  Determines if external stream selection is allowed.
CASMStreamFilter::CASMStreamFilter(Process* pProc, IHXSessionStats* pStats, IHXQoSProfileConfigurator* pQoSConfig)
    : m_ulRefCount(0)
    , m_pProc(pProc)
    , m_pPacketSink(NULL)
    , m_pPacketSource(NULL)
    , m_pCtrlSink(NULL)
    , m_pCtrlSource(NULL)
    , m_pPullPacketSink(NULL)
    , m_pPullPacketSource(NULL)
    , m_pASMSource(NULL)
    , m_pStats(NULL)
    , m_pQoSStats(NULL)
    , m_shiftType(ASM_FILTER_ST_NONE)
    , m_pQoSConfig(NULL)
    , m_pUberMgr(NULL)
    , m_pRateDescResp(NULL)
    , m_pStreamSelector(NULL)
    , m_pMBitHandler(NULL)
    , m_bRateCommitted(FALSE)
    , m_ulPktCount(0)
    , m_ulLastTS(0)
    , m_bPacketTrace (FALSE)
    , m_bDumpAdaptationInfo(FALSE)
    , m_bAllowExternalStreamSelection(FALSE)
    , m_bAllowInternalStreamSelection(FALSE)
    , m_initState(ASM_FILTER_IS_NONE)
    , m_pRateSelInfo(NULL)
{
  //printf("%p: ASMStreamFilter::ASMStreamFilter()\n", this);fflush(0);

    // XXXGo - just pass it in here for now.
    if (m_pStats = pStats)
    {
        m_pStats->AddRef();

        m_pQoSStats = m_pStats->GetQoSApplicationAdaptationInfo();
    }


    if (m_pQoSConfig = pQoSConfig)
    {
        m_pQoSConfig->AddRef();

            INT32 lTemp = 0;
        if (m_pQoSConfig->GetConfigInt(CONFIG_ACCEPT_INIT_EXTERNAL_SUB, lTemp) == HXR_OK && lTemp)
        {
            m_bAllowExternalStreamSelection = TRUE;
            m_bAllowInternalStreamSelection = FALSE;
        }
        else
        {
            m_bAllowExternalStreamSelection = FALSE;
            m_bAllowInternalStreamSelection = TRUE;
        }

        lTemp = 0;
        if (m_pQoSConfig->GetConfigInt(CONFIG_ACCEPT_INIT_INTERNAL_SUB, lTemp) == HXR_OK)
        {
            m_bAllowInternalStreamSelection = (BOOL)lTemp;
        }

        lTemp = 0;
        if (pQoSConfig->GetConfigInt(QOS_CFG_MDP, lTemp) == HXR_OK && lTemp)
        {
            // MDP is enabled in a config file.  Take whatever the config says above
        }
        else
        {
            // Two possibilities:
            // 1) MDP will be used by Helix or 3GPP Adaptation signaling later.
            // 2) PPM will be use and MDP is OFF.
            // No stream selection of any sort should be performed.
            m_bAllowExternalStreamSelection = FALSE;
            m_bAllowInternalStreamSelection = FALSE;
        }

        lTemp = 0;
        if (m_pQoSConfig->GetConfigInt(DUMP_PKT, lTemp) == HXR_OK && lTemp)
        {
            m_bPacketTrace = TRUE;
        }

        lTemp = 0;
        if (m_pQoSConfig->GetConfigInt(DUMP_SELECTION, lTemp) == HXR_OK && lTemp)
        {
            printf("InitialExternalSubscription: %s\n",
            m_bAllowExternalStreamSelection ? "ON" : "OFF");
            printf("InitialInternalSubscription: %s\n",
            m_bAllowInternalStreamSelection ? "ON" : "OFF");
            fflush(0);
        }

        lTemp = 0;
        if (m_pQoSConfig->GetConfigInt(DUMP_ADAPTATION, lTemp) == HXR_OK && lTemp)
        {
            m_bDumpAdaptationInfo = TRUE;
        }
    }
    else
    {
        // No stream selection
        m_bAllowExternalStreamSelection = FALSE;
        m_bAllowInternalStreamSelection = FALSE;
    }
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::~CASMStreamFilter
// Purpose:
//  Destructor
CASMStreamFilter::~CASMStreamFilter()
{
    _Done();
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::_Done
// Purpose:
//  Cleans up internal objects
void
CASMStreamFilter::_Done()
{
    HX_RELEASE(m_pPacketSink);
    HX_RELEASE(m_pPacketSource);
    HX_RELEASE(m_pCtrlSink);
    HX_RELEASE(m_pCtrlSource);

    HX_RELEASE(m_pPullPacketSink);
    HX_RELEASE(m_pPullPacketSource);

    HX_RELEASE(m_pASMSource);
    HX_RELEASE(m_pStats);
    HX_RELEASE(m_pQoSStats);
    HX_RELEASE(m_pUberMgr);
    HX_RELEASE(m_pRateDescResp);
    HX_RELEASE(m_pQoSConfig);
    HX_RELEASE(m_pStreamSelector);
    HX_RELEASE(m_pRateSelInfo);
}


/* IHXServerPacketSource */
STDMETHODIMP
CASMStreamFilter::SetSink(IHXServerPacketSink* pSink)
{
    HX_ASSERT(pSink && !m_pPacketSink);
    if (pSink)
    {
        m_pPacketSink = pSink;
        m_pPacketSink->AddRef();
        return HXR_OK;
    }
    return HXR_INVALID_PARAMETER;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::StartPackets
// Purpose:
//  Starts packet flow from underlying source
STDMETHODIMP
CASMStreamFilter::StartPackets()
{
    HX_ASSERT(m_pPacketSource);

    const UINT32 ulSize = 1;
    UINT32 ulBR[ulSize] = {20000};// 28000, 32000, 56000, 64000, 128000};

    HX_RESULT theErr = CommitSubscription();

    if (HXR_OK == theErr)
    {
        HX_RESULT theErr;
        UINT32 ulCount = 0;
        CPhysicalStream** ppAltInfos = NULL;
        CPhysicalStream* pAltInfo = NULL;

#ifdef DISABLE_EXTERNAL_ASM_SUB
        theErr = m_pPacketSource->StartPackets();
#endif
    }

    HX_ASSERT(SUCCEEDED(theErr));
    return theErr;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::CommitSubscription
// Purpose:
//  Commmits internal/external subscriptions
HX_RESULT
CASMStreamFilter::CommitSubscription()
{
    HX_RESULT res = HXR_OK;
    /*
     * This function is called from the shared code path (IHXPSourceControl),
     * so add a check here.
     * We donot want to perform CommitSubscription if we are in the legacy mode
     */
    if (IsLegacyMode() || ASM_FILTER_IS_RATE_SELECTION_INITED != m_initState)
    {
        // nothing to do
        return HXR_OK;
    }

    if (!m_bRateCommitted)
    {
        // If the no initial rate has been selected yet by the client or server, perform
        // internal stream selection.
        // Otherwise, make sure we'll pick the correct initial rate.
        if (SUCCEEDED(res))
        {
            IHXRateDescription* pRateDesc = NULL;
            res = m_pUberMgr->GetCurrentAggregateRateDesc(pRateDesc);

            if (FAILED(res))
            {
                res = SelectStreams();
            }
            else
            {
                UINT32 ulRate = 0;
                if (SUCCEEDED(pRateDesc->GetAvgRate(ulRate)))
                {
                    INT32 lTemp = 0;
                    if (m_pQoSConfig->GetConfigInt(QOS_CFG_IS_DEFAULT_MEDIARATE, lTemp) == HXR_OK)
                    {
                        // highest precedence
                        m_initialTargetRate.m_ulInitRate = (UINT32)lTemp;
                    }

                    // ok if failed.
                    if (m_initialTargetRate.m_ulInitRate > ulRate)
                    {
                        UpshiftAggregate(m_initialTargetRate.m_ulInitRate, NULL);
                    }
                    else if (m_initialTargetRate.m_ulInitRate < ulRate)
                    {
                        DownshiftAggregate(m_initialTargetRate.m_ulInitRate, NULL);
                    }
                }
            }

            INT32 lTemp = 0;
            if (m_pQoSConfig->GetConfigInt(DUMP_SELECTION, lTemp) == HXR_OK && lTemp)
            {
                UINT32 ulRate = 0;
                UINT32 ulNewRate = 0;
                IHXRateDescription* pNewRateDesc = NULL;
                res = m_pUberMgr->GetCurrentAggregateRateDesc(pNewRateDesc);
                if (SUCCEEDED(res) &&
                    SUCCEEDED(pRateDesc->GetAvgRate(ulRate)) &&
                    SUCCEEDED(pNewRateDesc->GetAvgRate(ulNewRate)))
                {
                    printf("Initial Rate Change: %u -> %u\n", ulRate, ulNewRate);fflush(0);
                }
                HX_RELEASE(pNewRateDesc);
            }

            HX_RELEASE(pRateDesc);
        }

        // Commit the initial stream selection
        if (SUCCEEDED(res))
            res = m_pUberMgr->CommitInitialAggregateRateDesc();

        if (SUCCEEDED(res))
        {
            m_bRateCommitted = TRUE;

            // Log stream selection info, if necessary
            INT32 lTemp = 0;
            if (SUCCEEDED(m_pQoSConfig->GetConfigInt(DUMP_ASMHANDLING, lTemp))
                && lTemp)
            {
                m_pUberMgr->Dump();
            }
            if (SUCCEEDED(m_pQoSConfig->GetConfigInt(DUMP_SELECTION, lTemp))
                && lTemp)
            {
                DumpSelection(res);
            }
        }
    }


    HX_ASSERT(SUCCEEDED(res));
    return res;
}


STDMETHODIMP
CASMStreamFilter::GetPacket()
{
    HX_ASSERT(m_pPacketSource);
    return m_pPacketSource->GetPacket();
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::SinkBlockCleared
// Purpose:
//  Notification to restart packet flow for the given logical stream
STDMETHODIMP
CASMStreamFilter::SinkBlockCleared(UINT32 ulStream)
{
    if (m_pPacketSource)
    {
        return m_pPacketSource->SinkBlockCleared(ulStream);
    }
    //    printf("****SourceDone() called, yet CASMStreamFilter::SinkBlockCleared()\n");fflush(0);
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CASMStreamFilter::SetSource(IHXServerPacketSource* pSource)
{
    HX_ASSERT(pSource && !m_pPacketSource);
    if (pSource)
    {
        m_pPacketSource = pSource;
        m_pPacketSource->AddRef();
        return HXR_OK;
    }
    return HXR_INVALID_PARAMETER;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::PacketReady
// Purpose:
//  Handles shift pending, marker bit logic.  Passes packet to packet sink.
STDMETHODIMP
CASMStreamFilter::PacketReady(ServerPacket* pPacket)
{
    // this is the core of this class
    HX_ASSERT(m_pPacketSink && m_pUberMgr);
    if (!pPacket)
    {
        return HXR_UNEXPECTED;
    }

    UINT16 unStreamNum = pPacket->GetStreamNumber();
    UINT16 unASMRuleNum = pPacket->GetASMRuleNumber();
    UINT32 ulStreamGroup = 0;

#ifdef DISABLE_EXTERNAL_ASM_SUB
    // Handle shift pending logic
    if (SUCCEEDED(m_pUberMgr->FindStreamGroupByLogicalStream(unStreamNum,
        ulStreamGroup)) &&
        m_pUberMgr->IsShiftPending(ulStreamGroup))
    {
        m_pUberMgr->HandlePacket(ulStreamGroup, unASMRuleNum);

        // handle pending aggregate shift
        if (m_shiftType != ASM_FILTER_ST_NONE)
        {
            if (!m_pUberMgr->IsShiftPending())
            {
                ShiftResponse(HXR_OK);
            }
        }

        // handle pending stream group shift
        else if (!m_pUberMgr->IsShiftPending(ulStreamGroup))
        {
            ShiftResponse(HXR_OK, ulStreamGroup);
        }
    }

    // Handle market bit
    pPacket->EnableIHXRTPPacketInfo();

    if (NULL == m_pMBitHandler)
    {

        IHXRTPPacketInfo* pRTPPacketInfo = NULL;
        if (pPacket->QueryInterface(IID_IHXRTPPacketInfo, (void**) &pRTPPacketInfo) == HXR_OK)
        {
            m_pMBitHandler = &CASMStreamFilter::MBitRTPPktInfo;
            pRTPPacketInfo->Release();
        }
        else
        {
            m_pMBitHandler = &CASMStreamFilter::MBitASMRuleNo;
        }
    }

    // XXXGo - figure out Marker bit correctly using
    // "marker=" in an ASMRuleBook
    BOOL bMBit = unASMRuleNum % 2;

    (this->*m_pMBitHandler)(bMBit, pPacket);
#endif
    INT32 bTemp = 0;

    m_ulLastTS = pPacket->GetMediaTimeInMs();
    if (m_bPacketTrace)
    {
        printf("\t\t\tsending: %u rule: %u flag: %u MTS: %u\n",
                unStreamNum,
                unASMRuleNum,
                pPacket->GetASMFlags(),
                pPacket->GetMediaTimeInMs());
        fflush(0);
    }

    return m_pPacketSink->PacketReady(pPacket);
}

HX_RESULT
CASMStreamFilter::MBitRTPPktInfo(UINT8 bMBit, ServerPacket* pPkt)
{
//    pPkt->SetRuleNumber(0);
    return ((IHXRTPPacketInfo*)pPkt)->SetMarkerBit(bMBit);
}

HX_RESULT
CASMStreamFilter::MBitASMRuleNo(UINT8 bMBit, ServerPacket* pPkt)
{
    pPkt->SetRuleNumber(bMBit ? 1 : 0);
    return HXR_OK;
}

STDMETHODIMP
CASMStreamFilter::Flush()
{
    if (m_pPacketSink)
        return m_pPacketSink->Flush();
    return HXR_OK;
}

STDMETHODIMP
CASMStreamFilter::SourceDone()
{
    HX_RELEASE(m_pPacketSource);
    Flush();
    if (m_pPacketSink)
        return m_pPacketSink->SourceDone();
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::Init
// Purpose:
//  Initializes the underlying packet source
STDMETHODIMP
CASMStreamFilter::Init(IHXPSinkControl* pSink)
{
    HX_ASSERT(pSink && !m_pCtrlSink);
    if (pSink)
    {
        m_pCtrlSink = pSink;
        m_pCtrlSink->AddRef();

        return m_pCtrlSource->Init(this);
    }
    return HXR_INVALID_PARAMETER;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::Done
// Purpose:
//  Cleans up the IHXPSourceControl
STDMETHODIMP
CASMStreamFilter::Done()
{
    if (m_pCtrlSource)
    {
    m_pCtrlSource->Done();
    }
    _Done();
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::GetFileHeader
// Purpose:
//  Used to request the file header
STDMETHODIMP
CASMStreamFilter::GetFileHeader(IHXPSinkControl* pSink)
{
    HX_ASSERT(m_pCtrlSource);
    HX_ASSERT(m_pCtrlSink && m_pCtrlSink == pSink);

    IHXSyncHeaderSource* pHdrSrc = NULL;
    IHXValues* pHdr = NULL;

    HX_RESULT theErr =
        m_pCtrlSource->QueryInterface(IID_IHXSyncHeaderSource, (void**)&pHdrSrc);
    if (HXR_OK == theErr)
    {
        theErr = pHdrSrc->GetFileHeader(pHdr);
        pHdrSrc->Release();
        pHdrSrc = NULL;
    }

    if (HXR_OK == theErr)
    {
        theErr = HandleFileHeader(theErr, pHdr);
        pHdr->Release();
        return theErr;
    }
    else
    {
        // continue ::FileHeaderReady()
        return m_pCtrlSource->GetFileHeader(this);
    }
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::HandleFileHeader
// Purpose:
//  Propagates the file header to the packet control sink
HX_RESULT
CASMStreamFilter::HandleFileHeader(HX_RESULT status, IHXValues* pHdr)
{
    HX_ASSERT(m_pCtrlSink);
    // XXXGo - Need to modify header according to the Stream Selection
    if (m_bAllowInternalStreamSelection)
    {
    IHXValues*  pNewHdr = NULL;
    if (HXR_OK == status)
    {
        status = CopyHeaders(pHdr, pNewHdr);
    }
    status = m_pCtrlSink->FileHeaderReady(status, pNewHdr);
    HX_RELEASE(pNewHdr);
    }
    else
    {
        status = m_pCtrlSink->FileHeaderReady(status, pHdr);
    }
    return status;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::HandleStreamHeader
// Purpose:
//  Propagates the stream header to the packet control sink
HX_RESULT
CASMStreamFilter::HandleStreamHeader(HX_RESULT status,IHXValues* pHdr)
{
    HX_ASSERT(m_pCtrlSink);
    // XXXGo - Need to modify header according to the Stream Selection
    if (m_bAllowInternalStreamSelection)
    {
        IHXValues*  pNewHdr = NULL;
        if (HXR_OK == status)
        {
            status = CopyHeaders(pHdr, pNewHdr);
        }
        if (HXR_OK == status)
        {
            status = ModifyStreamHeaders(pNewHdr);
        }
        status = m_pCtrlSink->StreamHeaderReady(status, pNewHdr);
        HX_RELEASE(pNewHdr);
    }
    else
    {
        status = m_pCtrlSink->StreamHeaderReady(status, pHdr);
    }
    return status;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::GetStreamHeader
// Purpose:
//  Gets the stream header
STDMETHODIMP
CASMStreamFilter::GetStreamHeader(IHXPSinkControl* pSink, UINT16 unStreamNumber)
{
    HX_ASSERT(m_pCtrlSource);
    HX_ASSERT(m_pCtrlSink && m_pCtrlSink == pSink);

    IHXSyncHeaderSource* pHdrSrc = NULL;
    IHXValues* pHdr = NULL;

    // Get stream header synchronously, if possible
    HX_RESULT theErr =
        m_pCtrlSource->QueryInterface(IID_IHXSyncHeaderSource, (void**)&pHdrSrc);
    if (HXR_OK == theErr)
    {
        theErr = pHdrSrc->GetStreamHeader(unStreamNumber, pHdr);
        pHdrSrc->Release();
        pHdrSrc = NULL;
    }

    if (HXR_OK == theErr)
    {
        theErr = HandleStreamHeader(theErr, pHdr);
        pHdr->Release();
        return theErr;
    }

    // Otherwise, get stream header asynchronously
    else
    {
        // continue ::StreamHeaderReady()
        return m_pCtrlSource->GetStreamHeader(this, unStreamNumber);
    }


    HX_ASSERT(m_pCtrlSource);
    HX_ASSERT(m_pCtrlSink == pSink);
    return m_pCtrlSource->GetStreamHeader(this, unStreamNumber);
}
STDMETHODIMP
CASMStreamFilter::Seek(UINT32 ulSeekTime)
{
    HX_ASSERT(m_pCtrlSource);

    CommitSubscription();

    return m_pCtrlSource->Seek(ulSeekTime);
}
BOOL
CASMStreamFilter::IsLive()
{
    if (!m_pCtrlSource)
    {
        return FALSE;
    }
    return m_pCtrlSource->IsLive();
}
STDMETHODIMP
CASMStreamFilter::SetLatencyParams(UINT32 ulLatency, BOOL bStartAtTail, BOOL bStartAtHead)
{
    HX_ASSERT(m_pCtrlSource);
    return m_pCtrlSource->SetLatencyParams(ulLatency, bStartAtTail, bStartAtHead);
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::InitDone
// Purpose:
//  Notification that the underlying packet source has finished initializing.
//  Creates/initializes an ASMRuleHandler.
STDMETHODIMP
CASMStreamFilter::InitDone(HX_RESULT ulStatus)
{
    HX_ASSERT(m_pCtrlSink);
    HX_ASSERT(ASM_FILTER_IS_NONE == m_initState);
    m_initState = ASM_FILTER_IS_CONTROL_INITED;

    ulStatus = DoInit(ulStatus);
    return m_pCtrlSink->InitDone(ulStatus);
}

HX_RESULT
CASMStreamFilter::DoInit(HX_RESULT ulStatus)
{
    HX_ASSERT(ASM_FILTER_IS_NONE != m_initState);
    if (ASM_FILTER_IS_RATE_SELECTION_INITED == m_initState)
    {
        HX_ASSERT(m_pUberMgr);
        return HXR_OK;
    }
    else if (ASM_FILTER_IS_CONTROL_INITED == m_initState)
    {
        // good, initialize
    }

    IHXSyncHeaderSource* pHdrSrc = NULL;

    // Create an ASMRuleHandler
    if (HXR_OK == ulStatus)
    {
        m_pUberMgr = new CUberStreamManager(m_pProc->pc->common_class_factory, m_pQoSConfig);
        if (m_pUberMgr)
        {
            m_pUberMgr->AddRef();
        }
        else
        {
            ulStatus = HXR_OUTOFMEMORY;
        }

        if (HXR_OK == ulStatus)
        {
            ulStatus = m_pCtrlSource->QueryInterface(IID_IHXASMSource, (void**)&m_pASMSource);
        }
    }

    if (HXR_OK == ulStatus)
    {
        ulStatus = m_pCtrlSource->QueryInterface(IID_IHXSyncHeaderSource, (void**)&pHdrSrc);
    }

    // Initialize the CUberStreamManager
    if (HXR_OK == ulStatus)
    {
        ulStatus = InitASMRuleHandler(pHdrSrc);
    }
    HX_RELEASE(pHdrSrc);

    if (HXR_OK == ulStatus)
    {
        m_initState = ASM_FILTER_IS_RATE_SELECTION_INITED;
    }

    return ulStatus;

}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::InitASMRuleHandler
// Purpose:
//  Extracts file/stream headers and uses the ASMRuleHandler to parse the
//  uber/logicalstream rulebooks.
HX_RESULT
CASMStreamFilter::InitASMRuleHandler(IHXSyncHeaderSource* pHdrSrc)
{
    HX_ASSERT(pHdrSrc);
    HX_RESULT res = HXR_OK;

    // Determine the number of logical streams
    UINT32 ulNumLogicalStreams = 0;
    if (SUCCEEDED(res))
    {
        IHXValues* pFileHeader = NULL;
        res = pHdrSrc->GetFileHeader(pFileHeader);

        if (SUCCEEDED(res))
            res = pFileHeader->GetPropertyULONG32("StreamCount", ulNumLogicalStreams);

        HX_RELEASE(pFileHeader);
    }

    // Determine min preroll from server config
    INT32 lConfigPreroll = 0;
    if (m_pQoSConfig &&
                HXR_OK != m_pQoSConfig->GetConfigInt(CONFIG_MIN_PREROLL, lConfigPreroll))
    {
        lConfigPreroll = 1000;
    }

    // Enforce min preroll
    for (UINT32 i = 0; i < ulNumLogicalStreams && SUCCEEDED(res); i++)
    {
        IHXValues* pHdr = NULL;
        res = pHdrSrc->GetStreamHeader(i, pHdr);

        if (SUCCEEDED(res))
        {
            UINT32 ulHeaderPreroll = 0;
            pHdr->GetPropertyULONG32("Preroll", ulHeaderPreroll);

            // Ensure that the logical stream preroll is above CONFIG_MIN_PREROLL
            if (lConfigPreroll > (UINT32) ulHeaderPreroll)
            {
                pHdr->SetPropertyULONG32("Preroll", lConfigPreroll);
            }
        }

        HX_RELEASE(pHdr);
    }

    // Init the uber rulebook mgr
    if (SUCCEEDED(res))
        res = m_pUberMgr->Init(TRUE);

    // Set the ASM source
    if (SUCCEEDED(res))
    {
        IHXASMSource* pAsmSource = NULL;
        res = pHdrSrc->QueryInterface(IID_IHXASMSource, (void**)&pAsmSource);

        if (SUCCEEDED(res))
            res = m_pUberMgr->SetASMSource(pAsmSource);

        HX_RELEASE(pAsmSource);
    }

    // Propagate file/stream headers
    if (SUCCEEDED(res))
    {
        // Propagate file header
        if (SUCCEEDED(res))
        {
            IHXValues* pFileHeader = NULL;
            res = pHdrSrc->GetFileHeader(pFileHeader);

            if (SUCCEEDED(res))
                res = m_pUberMgr->SetFileHeader(pFileHeader);

            HX_RELEASE(pFileHeader);
        }

        // Propagate logical stream headers
        for (UINT32 i=0; i<ulNumLogicalStreams && SUCCEEDED(res); i++)
        {
            IHXValues* pStreamHeader = NULL;
            res = pHdrSrc->GetStreamHeader(i, pStreamHeader);

            UINT32 ulLogicalStreamNum = 0;
            if (SUCCEEDED(res))
                res = pStreamHeader->GetPropertyULONG32("StreamNumber", ulLogicalStreamNum);

            if (SUCCEEDED(res))
                res = m_pUberMgr->SetStreamHeader(ulLogicalStreamNum, pStreamHeader);

            HX_RELEASE(pStreamHeader);
        }
    }

    // Dump results
    if (SUCCEEDED(res))
    {
        INT32 lTemp = 0;
        if (m_pQoSConfig &&
                m_pQoSConfig->GetConfigInt(DUMP_ASMHANDLING, lTemp) == HXR_OK && lTemp)
        {
            m_pUberMgr->Dump();
        }
    }

    // Dump bandwidth descriptors from rulebook, if necessary
    INT32 lTemp = 0;
    if (m_pQoSConfig &&
                m_pQoSConfig->GetConfigInt(DUMP_ALLRATE, lTemp) == HXR_OK && lTemp)
    {
        HX_RESULT ret = HXR_OK;
        UINT32 ulCount = GetNumRateDescriptions();
        IHXRateDescription* pRateDesc = NULL;

        printf("**** IHXRateDescManager ****\n");
        printf("  Desc Count: %u\n", GetNumRateDescriptions());
        printf("  Active Desc Count: %u\n", GetNumSelectableRateDescriptions());

        for (UINT32 i = 0; i < ulCount && HXR_OK == ret; i++)
        {
            ret = GetRateDescription(i, pRateDesc);
            if (HXR_OK == ret)
            {
                // XXXLY - A bunch of casts like this.  Need to cleanup
                printf("   Desc %2u: ", i);
                ((CRateDescription*)pRateDesc)->Dump();
                HX_RELEASE(pRateDesc);
            }
            HX_ASSERT(!pRateDesc);
        }
        fflush(0);
    }

    // If external stream selection is not allowed, select an initial stream -- for
    // the sdp generation code.
    // Note that a different stream may be selected later on due to updated client
    // capability info (e.g. RTSP Bandwidth header)
    // XXXLY - Player::Session should eventually request this initial stream selection, instead of
    // always doing it here.
    if (m_bAllowInternalStreamSelection && SUCCEEDED(res))
    {
        res = SelectStreams();
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::SelectStreams
// Purpose:
//  Determines the set of usable streams, and performs internal stream selection
// Notes:
//  Does not commit the inital rate since this method may be called multiple
//  times (e.g., once during DESCRIBE, later during PLAY with client Bandwidth info)
HX_RESULT
CASMStreamFilter::SelectStreams()
{
    HX_RESULT res = HXR_OK;

    // Create stream selector
    if (!m_pStreamSelector)
    {

        INT32 lTemp = 0;
        if (m_bAllowInternalStreamSelection &&
            m_pQoSConfig->GetConfigInt(DUMP_SELECTION, lTemp) == HXR_OK && lTemp)
        {
            // take the value
        }
        else
        {
            // make sure it 0
            lTemp = 0;
        }

        m_pStreamSelector = new CStreamSelector((BOOL)lTemp);
        if (!m_pStreamSelector)
            res = HXR_OUTOFMEMORY;
        else
            m_pStreamSelector->AddRef();

        //INT32
        lTemp = 0;
        if (m_pQoSConfig->GetConfigInt(DUMP_SUBSCRIPTION, lTemp) == HXR_OK && lTemp)
        {
            m_pUberMgr->m_bDumpSub = TRUE;
            lTemp = 0;
        }

        // Init the stream seletor
        if (SUCCEEDED(res))
            res = m_pStreamSelector->Init(m_pUberMgr, m_pProc, m_pStats, m_pQoSConfig);
    }


    /*
     * Cracks of the selection logic - depends on two flags.
     */
    if (SUCCEEDED(res) &&
        (m_initialTargetRate.m_ulInitRate > 0 || m_initialTargetRate.m_bForceRate))
    {
        if (!m_bAllowExternalStreamSelection)
        {
            res = m_pStreamSelector->SetInitTargetRate(m_initialTargetRate.m_ulInitRate, m_initialTargetRate.m_bForceRate);
        }
    }

    // Determine set of usable streams
    if (SUCCEEDED(res) && m_bAllowInternalStreamSelection)
    {
        res = m_pStreamSelector->VerifyStreams();
    }


    // Select an initial rate
    if (SUCCEEDED(res) && !m_bAllowExternalStreamSelection)
        res = m_pStreamSelector->SelectInitialRateDesc();

    INT32 lTemp = 0;
    if (m_pQoSConfig->GetConfigInt(DUMP_SELECTION, lTemp) == HXR_OK && lTemp)
    {
        DumpSelection(res);
    }

    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::DumpSelection
// Purpose:
//  Dumps stream selection info to stdout
void
CASMStreamFilter::DumpSelection(HX_RESULT theErr)
{
    if (HXR_OK == theErr)
    {
        CPhysicalStream* pAltInfo = NULL;
        IHXRateDescription* pRateDesc = NULL;
        printf("******* ACTIVE *******\n");

        UINT32 ulCount = GetNumRateDescriptions();
        for (UINT32 i = 0; i < ulCount; i++)
        {
            if (m_pUberMgr->GetRateDescription(i, pRateDesc) == HXR_OK)
            {
                printf("I: %u ", i);
                ((CRateDescription*)pRateDesc)->Dump();
                pRateDesc->Release();
                pRateDesc = NULL;
            }
        }

        printf("******* FOUND *******\n");
        if (GetCurrentAggregateRateDesc(pRateDesc) == HXR_OK)
        {
            ((CRateDescription*)pRateDesc)->Dump();
            pRateDesc->Release();
            pRateDesc = NULL;
        }
    }
    else
    {
        printf("******* Not Available *******\n");
    }

    printf("\n");
    fflush(0);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::CopyHeaders
// Purpose:
//  Creates a new IHXValues and copies all properties to it -- except possibly ASMRuleBook
// Notes:
//  Replace this with a new SDP interface
HX_RESULT
CASMStreamFilter::CopyHeaders(IHXValues* pOldVal, REF(IHXValues*) pNewVal)
{
    HX_RESULT theErr = HXR_INVALID_PARAMETER;

    if (pOldVal)
    {
        theErr = m_pProc->pc->common_class_factory->
            CreateInstance(CLSID_IHXValues, (void**) &pNewVal);
    }

    const char* pc = NULL;
    UINT32  ul = 0;
    IHXBuffer* pBuf = NULL;

    if (HXR_OK == theErr)
    {
        // Copy all ULONG32 properties
        theErr = pOldVal->GetFirstPropertyULONG32(pc, ul);
        while (HXR_OK == theErr)
        {
//            printf("%s: %u\n", pc, ul);fflush(0);
            theErr = pNewVal->SetPropertyULONG32(pc, ul);
            if (HXR_OK == theErr)
            {
                theErr = pOldVal->GetNextPropertyULONG32(pc, ul);
            }
        }

        // Copy all string properties -- note that this only does a shallow copy
        theErr = pOldVal->GetFirstPropertyCString(pc, pBuf);
        while (HXR_OK == theErr)
        {
            if (strcasecmp("ASMRuleBook", pc))
            {
//                printf("%s: %s\n", pc, pBuf->GetBuffer());fflush(0);
                theErr = pNewVal->SetPropertyCString(pc, pBuf);
            }
            else
            {
#if 0
                HX_RELEASE(pBuf);

                theErr = m_pProc->pc->common_class_factory->
                    CreateInstance(CLSID_IHXBuffer, (void**)&pBuf);

                if (HXR_OK == theErr)
                {
                    UINT32 ulBW = 0;
                    theErr = pOldVal->GetPropertyULONG32("AvgBitRate", ulBW);
                    if (HXR_OK == theErr && ulBW)
                    {
                        char rulebook[256] = {0}; /* Flawfinder: ignore */

                        SafeSprintf(rulebook,256,
                            "marker=0,AverageBandwidth=%d;marker=1,AverageBandwidth=0;", ulBW);
                        theErr = pBuf->Set((BYTE*)rulebook, strlen(rulebook) + 1);
                    }
                    else
                    {
                        HX_ASSERT(!"no AvgBitRate - really bad");
                        theErr = pBuf->Set((const BYTE*)"marker=0,timestampdelivery=1;marker=1,timestampdelivery=1;", 59);
                    }
                }
#endif
                if (HXR_OK == theErr)
                {
                    theErr = pNewVal->SetPropertyCString("ASMRuleBook", pBuf);
                }
            }

            HX_RELEASE(pBuf);

            if (HXR_OK == theErr)
            {
                theErr = pOldVal->GetNextPropertyCString(pc, pBuf);
            }
        }

        // Copy all IHXBuffers -- note that this only does a shallow copy
        theErr = pOldVal->GetFirstPropertyBuffer(pc, pBuf);
        while (HXR_OK == theErr)
        {
//            printf("%s: %s\n", pc, pBuf->GetBuffer());fflush(0);
            theErr = pNewVal->SetPropertyBuffer(pc, pBuf);
            HX_RELEASE(pBuf);

            if (HXR_OK == theErr)
            {
                theErr = pOldVal->GetNextPropertyBuffer(pc, pBuf);
            }
        }

        theErr = HXR_OK;
    }

    return theErr;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::ModifyStreamHeaders
// Purpose:
//  Adds QOS params to the stream header, if necessary
HX_RESULT
CASMStreamFilter::ModifyStreamHeaders(IHXValues* pHdr)
{
    HX_ASSERT(m_pUberMgr);
    HX_ASSERT(pHdr);

    // Validate state
    if (!m_pStreamSelector)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    IHXBuffer* pMimeType = NULL;
    BOOL        bUseAnnexG = FALSE;

    // Add the Annex G QoS SDP params if it's a video stream
    // and QoS was applied
    if (SUCCEEDED(pHdr->GetPropertyCString("MimeType", pMimeType)))
    {
        if (m_pQoSConfig &&
            strncasecmp((const char*)pMimeType->GetBuffer(), "video", 5) == 0)
        {
            bUseAnnexG = TRUE;
        }
    }

    HX_RELEASE(pMimeType);
    return m_pUberMgr->ModifyHeaders(pHdr, bUseAnnexG);

}

STDMETHODIMP
CASMStreamFilter::FileHeaderReady(HX_RESULT ulStatus, IHXValues* pHeader)
{
    return HandleFileHeader(ulStatus, pHeader);
}
STDMETHODIMP
CASMStreamFilter::StreamHeaderReady(HX_RESULT ulStatus, IHXValues* pHeader)
{
    return HandleStreamHeader(ulStatus, pHeader);
}


STDMETHODIMP
CASMStreamFilter::StreamDone(UINT16 unStreamNumber)
{
  //    printf("StreamDone(%u) - LastTS: %u\n", unStreamNumber, m_ulLastTS);fflush(0);
    HX_ASSERT(m_pCtrlSink);
    return m_pCtrlSink->StreamDone(unStreamNumber);
}
STDMETHODIMP
CASMStreamFilter::SeekDone(HX_RESULT ulStatus)
{
    HX_ASSERT(m_pCtrlSink);
    return m_pCtrlSink->SeekDone(ulStatus);
}

/* IHXPSourcePackets */
STDMETHODIMP
CASMStreamFilter::Init(IHXPSinkPackets* pSink)
{
    HX_ASSERT(!m_pPullPacketSink && !m_pPullPacketSource && pSink && m_pPacketSource);

    HX_RESULT theErr = HXR_UNEXPECTED;

    if (pSink && m_pPacketSource)
    {
        m_pPullPacketSink = pSink;
        m_pPullPacketSink->AddRef();

        theErr = m_pPacketSource->QueryInterface(IID_IHXPSourcePackets,
            (void**)&m_pPullPacketSource);
    }

    if (HXR_OK == theErr)
    {
        theErr = m_pPullPacketSource->Init(this);
    }

    return theErr;
}
STDMETHODIMP
CASMStreamFilter::GetPacket(UINT16 unStreamNumber)
{
    if (m_pPullPacketSource)
    {
        return m_pPullPacketSource->GetPacket(unStreamNumber);
    }
    else
    {
        HX_ASSERT(!"missing m_pPullPacketSource");
        return HXR_UNEXPECTED;
    }
}

STDMETHODIMP_(ULONG32)
CASMStreamFilter::IsThreadSafe()
{
    if (m_pPullPacketSource)
    {
        return m_pPullPacketSource->IsThreadSafe();
    }
    HX_ASSERT(FALSE);
    return 0;
}

STDMETHODIMP
CASMStreamFilter::PacketReady(HX_RESULT ulStatus, IHXPacket* pPacket)
{
    if (m_pPullPacketSink)
    {
        if (m_bPacketTrace)
        {
            printf("\t\t\tsending: %u rule: %u flag: %u TS: %u\n",
            pPacket->GetStreamNumber(),
            pPacket->GetASMRuleNumber(),
            pPacket->GetASMFlags(),
            pPacket->GetTime());fflush(0);
        }

        return m_pPullPacketSink->PacketReady(ulStatus, pPacket);
    }
    else
    {
        HX_ASSERT(!"missing m_pPullPacketSink");
        return HXR_UNEXPECTED;
    }
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::Subscribe
// Purpose:
//  Handles external subscriptions to an ASM rule
STDMETHODIMP
CASMStreamFilter::Subscribe(UINT16 ulLogicalStreamNum, UINT16 uRuleNumber)
{
    // Ignore request if external subscriptions are not allowed
    INT32 lTemp = 0;
    if (!m_bAllowExternalStreamSelection)
    {
        if (m_pQoSConfig->GetConfigInt(DUMP_ASMHANDLING, lTemp) == HXR_OK && lTemp)
        {
            printf("Ignoring external Subscribe: %u:%u\n", ulLogicalStreamNum,
                uRuleNumber);fflush(0);
        }
        return HXR_OK;
    }
    // Otherwise, subscribe to the ASM rule
    else if (!m_bRateCommitted)
    {
        HX_RESULT theErr = HXR_OK;
        if (m_pQoSConfig->GetConfigInt(DUMP_ASMHANDLING, lTemp) == HXR_OK && lTemp)
        {
            printf("External Subscribe: %u:%u\n", ulLogicalStreamNum, uRuleNumber);fflush(0);
        }

        if (HXR_OK == theErr)
        {
            theErr = m_pUberMgr->SubscribeLogicalStreamRule(ulLogicalStreamNum, uRuleNumber, NULL);
        }

        return theErr;
    }
    // Ignore request if initial stream selection has already occurred
    else
    {
        if (m_pQoSConfig->GetConfigInt(DUMP_ASMHANDLING, lTemp) == HXR_OK && lTemp)
        {
            printf("Ignoring subsequent external Subscribe: %u:%u\n", ulLogicalStreamNum, uRuleNumber);fflush(0);
        }
        return HXR_OK;
    }
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::Unsubscribe
// Purpose:
//  No-op (ignores external unsubscriptions)
STDMETHODIMP
CASMStreamFilter::Unsubscribe(UINT16 ulLogicalStreamNum, UINT16 uRuleNumber)
{
    INT32 lTemp = 0;
    if (m_pQoSConfig->GetConfigInt(DUMP_ASMHANDLING, lTemp) == HXR_OK && lTemp)
    {
        printf("Ignoring external Unsubscribe: %u:%u\n", ulLogicalStreamNum,
            uRuleNumber);fflush(0);
    }
    return HXR_OK;
}

HX_RESULT
CASMStreamFilter::SetControlSource(IHXPSourceControl* pCtrlSource)
{
    HX_ASSERT(pCtrlSource && !m_pCtrlSource);
    if (pCtrlSource)
    {
        m_pCtrlSource = pCtrlSource;
        m_pCtrlSource->AddRef();
        return HXR_OK;
    }
    return HXR_INVALID_PARAMETER;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::_SetInitialRate
// Purpose:
// Subscribes to the physical streams associated with the initial rate
HX_RESULT
CASMStreamFilter::_SetInitialRate(UINT32 ulRate)
{
    HX_RESULT res = HXR_OK;

    // Find closest (lower or equal) matching rate that is available for selection
    IHXRateDescription* pRateDesc = NULL;
    if (SUCCEEDED(res))
        res = m_pUberMgr->FindRateDescByClosestAvgRate(ulRate, TRUE, FALSE, pRateDesc);

    if (SUCCEEDED(res))
        res = m_pUberMgr->SetAggregateRateDesc(pRateDesc, NULL);

    if (SUCCEEDED(res))
        res = m_pUberMgr->CommitInitialAggregateRateDesc();

    HX_RELEASE(pRateDesc);

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

STDMETHODIMP_(UINT32)
CASMStreamFilter::GetNumRateDescriptions()
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return 0;
    }

    return m_pUberMgr->GetNumRateDescriptions();
}


STDMETHODIMP
CASMStreamFilter::GetRateDescription(UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->GetRateDescription(ulIndex, pRateDesc);
}

STDMETHODIMP_(UINT32)
CASMStreamFilter::GetNumSelectableRateDescriptions()
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return 0;
    }

    return m_pUberMgr->GetNumSelectableRateDescriptions();
}

STDMETHODIMP
CASMStreamFilter::GetSelectableRateDescription(UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }
    return m_pUberMgr->GetSelectableRateDescription(ulIndex, pRateDesc);
}

STDMETHODIMP_(UINT32)
CASMStreamFilter::GetNumSwitchableRateDescriptions()
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return 0;
    }

   return m_pUberMgr->GetNumSwitchableRateDescriptions();
}

STDMETHODIMP
CASMStreamFilter::GetSwitchableRateDescription(UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->GetSwitchableRateDescription(ulIndex, pRateDesc);
}

STDMETHODIMP
CASMStreamFilter::FindRateDescByRule(UINT32 ulRuleNumber, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->FindRateDescByRule(ulRuleNumber, bRequireSelectable, bRequireSwitchable, pRateDesc);
}

STDMETHODIMP
CASMStreamFilter::FindRateDescByExactAvgRate(UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->FindRateDescByExactAvgRate(ulAvgRate, bRequireSelectable, bRequireSwitchable, pRateDesc);
}

STDMETHODIMP
CASMStreamFilter::FindRateDescByClosestAvgRate(UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->FindRateDescByClosestAvgRate(ulAvgRate, bRequireSelectable, bRequireSwitchable, pRateDesc);
}
STDMETHODIMP
CASMStreamFilter::FindRateDescByMidpoint(UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->FindRateDescByMidpoint(ulAvgRate, bRequireSelectable, bRequireSwitchable, pRateDesc);
}


STDMETHODIMP
CASMStreamFilter::GetCurrentAggregateRateDesc(REF(IHXRateDescription*)pRateDesc)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->GetCurrentAggregateRateDesc(pRateDesc);
}

STDMETHODIMP_(UINT32)
CASMStreamFilter::GetNumStreamGroups()
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return 0;
    }

   return m_pUberMgr->GetNumStreamGroups();
}

STDMETHODIMP
CASMStreamFilter::GetStreamGroup(UINT32 ulIndex, REF(IHXRateDescEnumerator*)pStreamGroupMgr)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->GetStreamGroup(ulIndex, pStreamGroupMgr);
}

STDMETHODIMP
CASMStreamFilter::FindStreamGroupByLogicalStream(UINT32 ulLogicalStream, REF(UINT32)ulStreamGroup)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->FindStreamGroupByLogicalStream(ulLogicalStream, ulStreamGroup);
}

STDMETHODIMP_(UINT32)
CASMStreamFilter::GetNumLogicalStreams()
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return 0;
    }

   return m_pUberMgr->GetNumLogicalStreams();
}

STDMETHODIMP
CASMStreamFilter::GetLogicalStream(UINT32 ulIndex, REF(IHXRateDescEnumerator*)pLogicalStreamEnum)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->GetLogicalStream(ulIndex, pLogicalStreamEnum);
}

STDMETHODIMP
CASMStreamFilter::GetSelectedLogicalStreamNum(UINT32 ulStreamGroupNum, REF(UINT32)ulSelectedLogicalStreamNum)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->GetSelectedLogicalStreamNum(ulStreamGroupNum, ulSelectedLogicalStreamNum);
}

STDMETHODIMP
CASMStreamFilter::SetAggregateRateDesc(IHXRateDescription* pRateDesc, IHXRateDescResponse* pResp)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->SetAggregateRateDesc(pRateDesc, pResp);
}

STDMETHODIMP
CASMStreamFilter::CommitInitialAggregateRateDesc()
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    if (m_pStreamSelector)
    {
        m_pStreamSelector->SelectBetterInitialRateDesc(m_pRateSelInfo);
    }

    return m_pUberMgr->CommitInitialAggregateRateDesc();
}

STDMETHODIMP_(BOOL)
CASMStreamFilter::IsInitalAggregateRateDescCommitted()
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return 0;
    }

   return m_pUberMgr->IsInitalAggregateRateDescCommitted();
}

STDMETHODIMP
CASMStreamFilter::SetStreamGroupRateDesc(UINT32 ulStreamGroupNum,
                                         UINT32 ulLogicalStreamNum,
                                         IHXRateDescription* pRateDesc,
                                         IHXStreamRateDescResponse* pResp)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->SetStreamGroupRateDesc(ulStreamGroupNum,
            ulLogicalStreamNum, pRateDesc, pResp);
}

STDMETHODIMP
CASMStreamFilter::GetCurrentStreamGroupRateDesc(UINT32 ulStreamGroupNum, REF(IHXRateDescription*)pRateDesc)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->GetCurrentStreamGroupRateDesc(ulStreamGroupNum, pRateDesc);
}

STDMETHODIMP
CASMStreamFilter::CommitInitialStreamGroupRateDesc(UINT32 ulStreamGroupNum)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->CommitInitialStreamGroupRateDesc(ulStreamGroupNum);
}

STDMETHODIMP_(BOOL)
CASMStreamFilter::IsInitalStreamGroupRateDescCommitted(UINT32 ulStreamGroupNum)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->IsInitalStreamGroupRateDescCommitted(ulStreamGroupNum);
}

STDMETHODIMP
CASMStreamFilter::UpshiftStreamGroup(UINT32 ulStreamGroupNum,
                                     UINT32 ulRate,
                                     IHXStreamRateDescResponse* pResp)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->UpshiftStreamGroup(ulStreamGroupNum, ulRate, pResp);
}

STDMETHODIMP
CASMStreamFilter::DownshiftStreamGroup(UINT32 ulStreamGroupNum,
                                       UINT32 ulRate,
                                       IHXStreamRateDescResponse* pResp)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->DownshiftStreamGroup(ulStreamGroupNum, ulRate, pResp);
}

STDMETHODIMP
CASMStreamFilter::SubscribeLogicalStreamRule(UINT32 ulLogicalStreamNum,
                                             UINT32 ulRuleNum,
                                             IHXStreamRateDescResponse* pResp)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->SubscribeLogicalStreamRule(ulLogicalStreamNum, ulRuleNum, pResp);
}

STDMETHODIMP
CASMStreamFilter::UnsubscribeLogicalStreamRule(UINT32 ulLogicalStreamNum,
                                               UINT32 ulRuleNum,
                                               IHXStreamRateDescResponse* pResp)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->UnsubscribeLogicalStreamRule(ulLogicalStreamNum, ulRuleNum, pResp);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::UpshiftAggregate
// Purpose:
//  Upshifts to a higher rate
STDMETHODIMP
CASMStreamFilter::UpshiftAggregate(UINT32 ulRate, IHXRateDescResponse* pResp)
{
    HX_RESULT res = HXR_OK;

    // Perform actual uphift
    if (SUCCEEDED(res))
        res = m_pUberMgr->UpshiftAggregate(ulRate, NULL);

    // Dump adaptation info, if necessary
    if (m_bDumpAdaptationInfo)
    {
        printf("UpshiftAggregate(%u): pkt: %u\n", ulRate, m_ulPktCount);
        fflush(0);
    }

    HX_ASSERT(ASM_FILTER_ST_NONE == m_shiftType);

    // Handle shift response
    m_shiftType = ASM_FILTER_ST_UP;
    HX_RELEASE(m_pRateDescResp);
    if (pResp)
    {
        m_pRateDescResp = pResp;
        m_pRateDescResp->AddRef();

        if (FAILED(res))
            ShiftResponse(res);
    }

    // OK if rate shift was ignored
    if (res == HXR_IGNORE)
        res = HXR_OK;

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::SetClientAverageBandwidth
// Purpose:
//  Set Initial Target Rate
STDMETHODIMP
CASMStreamFilter::SetClientAverageBandwidth(UINT32 ulAvgBandwidth)
{
    HX_RESULT res = HXR_OK;

    if (!m_initialTargetRate.m_bForceRate)
    {
        if (m_pStreamSelector)
                m_pStreamSelector->SetInitTargetRate(ulAvgBandwidth, FALSE);

        m_initialTargetRate.m_ulInitRate = ulAvgBandwidth;
    }
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::SetRateSelectionInfoObject
// Purpose:
//  Provide initial rate selection info.

STDMETHODIMP
CASMStreamFilter::SetRateSelectionInfoObject(IHXRateSelectionInfo* pRateSelInfo)
{
    m_pRateSelInfo = pRateSelInfo;
    HX_ADDREF(m_pRateSelInfo);
    return HXR_OK;
}
/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::DownshiftAggregate
// Purpose:
//  Downshifts to a lower rate
STDMETHODIMP
CASMStreamFilter::DownshiftAggregate(UINT32 ulRate, IHXRateDescResponse* pResp)
{
    HX_RESULT res = HXR_OK;

    // Perform actual downshift
    if (SUCCEEDED(res))
        res = m_pUberMgr->DownshiftAggregate(ulRate, NULL);

    // Dump adaptation info, if necessary
    if (m_bDumpAdaptationInfo)
    {
        printf("DownshiftAggregate(%u): pkt: %u\n", ulRate, m_ulPktCount);
        fflush(0);
    }

    HX_ASSERT(ASM_FILTER_ST_NONE == m_shiftType);

    // Handle shift response
    m_shiftType = ASM_FILTER_ST_DOWN;
    HX_RELEASE(m_pRateDescResp);
    if (pResp)
    {
        m_pRateDescResp = pResp;
        m_pRateDescResp->AddRef();

        if (FAILED(res))
            ShiftResponse(res);
    }

    // OK if rate shift was ignored
    if (res == HXR_IGNORE)
        res = HXR_OK;

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::ShiftResponse
// Purpose:
//  Notification that a shift has succeeded/failed.
void
CASMStreamFilter::ShiftResponse(HX_RESULT status)
{
    // Log adaptation info, if necessary
    if (m_bDumpAdaptationInfo)
    {
        if (HXR_OK == status)
        {
            printf("shift completed: ");
            IHXRateDescription* p = NULL;
            if (GetCurrentAggregateRateDesc(p) == HXR_OK)
            {
                UINT32 i = 0;
                p->GetAvgRate(i);
                printf("%u", i);
            }
            printf("\n");
            HX_RELEASE(p);
        }
        else if (HXR_IGNORE == status)
        {
            printf("shift ignored\n");
        }
        else
        {
            printf("shift failed (%x)\n", status);
        }
        fflush(0);
    }

    // update the state and if there is a rate desc response,
    // notify it that the shift has succeeded/failed
    HX_ASSERT(ASM_FILTER_ST_NONE != m_shiftType);
    HX_RESULT theErr = status;
    IHXRateDescription* pRateDesc = NULL;

    if (HXR_OK == theErr)
    {
        theErr = GetCurrentAggregateRateDesc(pRateDesc);
        HX_ASSERT(HXR_OK == theErr);

        UpdateStats(pRateDesc);
    }

    m_shiftType = ASM_FILTER_ST_NONE;
    if (m_pRateDescResp)
    {
        m_pRateDescResp->ShiftDone(theErr, pRateDesc);
    }

    HX_RELEASE(pRateDesc);
    HX_RELEASE(m_pRateDescResp);
}

void
CASMStreamFilter::ShiftResponse(HX_RESULT status, UINT32 ulStreamGroup)
{
    // Log adaptation info, if necessary
    if (m_bDumpAdaptationInfo)
    {
        if (HXR_OK == status)
        {
            printf("shift completed: ");
            IHXRateDescription* p = NULL;
            if (GetCurrentAggregateRateDesc(p) == HXR_OK)
            {
                UINT32 i = 0;
                p->GetAvgRate(i);
                printf("%u", i);
            }
            printf("\n");
            HX_RELEASE(p);
        }
        else if (HXR_IGNORE == status)
        {
            printf("shift ignored\n");
        }
        else
        {
            printf("shift failed (%x)\n", status);
        }
        fflush(0);
    }

    // Pass through the shift done
    m_pUberMgr->ShiftDone(status, ulStreamGroup);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::UpdateStats
// Purpose:
//  Updates client stats (bitrate, upshift/downshifts, etc.)
void
CASMStreamFilter::UpdateStats(IHXRateDescription* pRateDesc)
{
    HX_ASSERT(ASM_FILTER_ST_NONE != m_shiftType);

    if (m_pQoSStats)
    {
        if (pRateDesc)
        {
            UINT32 ulTemp = 0;
            if (HXR_OK == pRateDesc->GetAvgRate(ulTemp))
            {
                m_pQoSStats->SetCurrentBitrate(ulTemp);
            }
        }

        // XXXJDG This tracks aggregate shifts.
        // How to handle stream level shifts?
        // What are these counts used for?
        if (ASM_FILTER_ST_UP == m_shiftType)
        {
            m_pQoSStats->SetTotalUpshifts(m_pQoSStats->GetTotalUpshifts()+1);
        }
        else if (ASM_FILTER_ST_DOWN == m_shiftType)
        {
            m_pQoSStats->SetTotalDownshifts(m_pQoSStats->GetTotalDownshifts()+1);
        }

        m_pQoSStats->SetTotalBitrateAdaptations(m_pQoSStats->GetTotalBitrateAdaptations()+1);
    }
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::SetInitial
// Purpose:
//  Sets initial bandwidth (type == 0)
//  Testing purpose only
void
CASMStreamFilter::SetInitial(UINT32 type, UINT32 ulVal)
{
    if (0 == type)
    {
        m_initialTargetRate.m_bForceRate = TRUE;
        m_initialTargetRate.m_ulInitRate = ulVal;

        if (m_pStreamSelector)
            m_pStreamSelector->SetInitTargetRate(ulVal, TRUE);
    }
}

STDMETHODIMP
CASMStreamFilter::EnableTCPMode()
{
    HX_ASSERT(m_pPacketSource);
    return (m_pPacketSource) ? m_pPacketSource->EnableTCPMode() : HXR_OK;
}


STDMETHODIMP_(ULONG32)
CASMStreamFilter::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CASMStreamFilter::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
CASMStreamFilter::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXServerPacketSource*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXServerPacketSource))
    {
        AddRef();
        *ppvObj = (IHXServerPacketSource*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXServerPacketSink))
    {
        AddRef();
        *ppvObj = (IHXServerPacketSink*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSourceControl))
    {
        AddRef();
        *ppvObj = (IHXPSourceControl*) this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSinkControl))
    {
        AddRef();
        *ppvObj = (IHXPSinkControl*) this;
        return HXR_OK;
    }

    else if (IsEqualIID(riid, IID_IHXPSourcePackets))
    {
        AddRef();
        *ppvObj = (IHXPSourcePackets*) this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSinkPackets))
    {
        AddRef();
        *ppvObj = (IHXPSinkPackets*) this;
        return HXR_OK;
    }

    else if (IsEqualIID(riid, IID_IHXASMSource))
    {
        AddRef();
        *ppvObj = (IHXASMSource*) this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXRateDescEnumerator))
    {
        AddRef();
        *ppvObj = (IHXRateDescEnumerator*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXUberStreamManager))
    {
        AddRef();
        *ppvObj = (IHXUberStreamManager*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXUberStreamManagerConfig))
    {
        AddRef();
        *ppvObj = (IHXUberStreamManagerConfig*)this;
        return HXR_OK;
    }
   else
    {
        HX_RESULT theErr = HXR_FAIL;
        if (m_pPacketSource)
        {
            theErr = m_pPacketSource->QueryInterface(riid, ppvObj);
        }

        if (HXR_OK != theErr && m_pCtrlSource)
        {
            theErr = m_pCtrlSource->QueryInterface(riid, ppvObj);
        }

        if (HXR_OK == theErr)
        {
            return theErr;
        }
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

