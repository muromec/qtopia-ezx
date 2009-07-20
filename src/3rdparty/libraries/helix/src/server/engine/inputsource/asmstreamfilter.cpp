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
const char* CONFIG_MIN_PREROLL = "RateAdaptation.MinPreroll";

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
    , m_bRateCommitted(FALSE)
    , m_ulPktCount(0)
    , m_initState(ASM_FILTER_IS_NONE)
    , m_pRateSelInfo(NULL)
    , m_lDebugOutput(0)
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

        m_pQoSConfig->GetConfigInt(DEBUG_OUTPUT, m_lDebugOutput);
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
        if (SUCCEEDED(res))
        {
            IHXRateDescription* pRateDesc = NULL;
            res = m_pUberMgr->GetCurrentAggregateRateDesc(pRateDesc);

            if (FAILED(res))
            {
                res = m_pStreamSelector->SelectInitialRateDesc(m_pRateSelInfo);
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
            if (m_lDebugOutput & (DUMP_ASMHANDLING | DUMP_ALL))
            {
                m_pUberMgr->Dump();
            }
            if (m_lDebugOutput & (DUMP_SELECTION | DUMP_ALL))
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
#endif
    if (DUMP_PKT == (m_lDebugOutput & DUMP_PKT))
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
    if (IsLive())
    {
        //Unsubscribe to currently subscribed streams and rules.
        UnsubscribeAllOnDone();
    }

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
    return m_pCtrlSink->FileHeaderReady(status, pHdr);
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
    return m_pCtrlSink->StreamHeaderReady(status, pHdr);
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
        if (m_lDebugOutput & (DUMP_ASMHANDLING | DUMP_ALL))
        {
            m_pUberMgr->Dump();
        }
    }

    // Dump bandwidth descriptors from rulebook, if necessary
    INT32 lTemp = 0;
    if (m_lDebugOutput & (DUMP_ALLRATE | DUMP_ALL))
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
    // XXXLY - ClientSession should eventually request this initial stream selection, instead of
    // always doing it here.
    if (SUCCEEDED(res))
    {
        DetermineSelectableStreams();
    }
    if (SUCCEEDED(res))
    {
        m_pUberMgr->SetStreamSelector(m_pStreamSelector);
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::DetermineSelectableStreams
// Purpose:
//  Determines the set of usable streams, and performs internal stream selection
//  ulBufferSize: Used to update the VPDBSize with the buffer size received in *-Adaptation headers.
//                Should be ZERO if we are not using *-Adaptation which is default value.
// Notes:
//  Does not commit the inital rate since this method may be called multiple
//  times (e.g., once during DESCRIBE, later during PLAY with client Bandwidth info)
HX_RESULT
CASMStreamFilter::DetermineSelectableStreams(const StreamAdaptationParams* pStreamAdaptationParams)
{
    HX_RESULT res = HXR_OK;

    // Create stream selector
    if (!m_pStreamSelector)
    {

        m_pStreamSelector = new CStreamSelector();
        if (!m_pStreamSelector)
            res = HXR_OUTOFMEMORY;
        else
            m_pStreamSelector->AddRef();

        // Init the stream seletor
        if (SUCCEEDED(res))
            res = m_pStreamSelector->Init(m_pUberMgr, (IUnknown*)m_pProc->pc->server_context, m_pStats, m_pQoSConfig);
    }

    // Determine set of usable streams
    if (SUCCEEDED(res))
    {
        res = m_pStreamSelector->ExcludeUnusableStreams(pStreamAdaptationParams);
    }


    INT32 lTemp = 0;
    if (m_lDebugOutput & (DUMP_SELECTION | DUMP_ALL))
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

STDMETHODIMP
CASMStreamFilter::PacketReady(HX_RESULT ulStatus, IHXPacket* pPacket)
{
    if (m_pPullPacketSink)
    {
        if (DUMP_PKT == (m_lDebugOutput & DUMP_PKT))
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
    // Subscription is primarily handled by CStreamSelector as rates are committed.

    INT32 lTemp = 0;
    if (m_bRateCommitted)
    {
        // Technically subscribe is always ignored in this call.
        // Output legacy message if initial rate selection has occurred.
        if (m_lDebugOutput & (DUMP_ASMHANDLING | DUMP_ALL))
        {
            printf("Ignoring subsequent external Subscribe: %u:%u\n", ulLogicalStreamNum, uRuleNumber);fflush(0);
        }
    }
    return HXR_OK;
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
    if (m_lDebugOutput & (DUMP_ASMHANDLING | DUMP_ALL))
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
CASMStreamFilter::SelectLogicalStream(UINT32 ulStreamGroupNum,
                                      UINT32 ulLogicalStreamNum)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->SelectLogicalStream(ulStreamGroupNum, 
                                           ulLogicalStreamNum);
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

    if (m_pStreamSelector && !m_bRateCommitted)
    {
        m_pStreamSelector->SelectInitialRateDesc(m_pRateSelInfo);
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
CASMStreamFilter::GetNextSwitchableStreamGroupRateDesc(UINT32 ulStreamGroupNum, REF(IHXRateDescription*)pRateDesc)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->GetNextSwitchableStreamGroupRateDesc(ulStreamGroupNum, pRateDesc);
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
                                     IHXStreamRateDescResponse* pResp,
                                     BOOL bIsClientInitiated)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    HX_RESULT hr = HXR_FAIL;
    hr = m_pUberMgr->UpshiftStreamGroup(ulStreamGroupNum, ulRate, pResp, bIsClientInitiated);

    // Dump adaptation info, if necessary
    if (SUCCEEDED(hr) && (m_lDebugOutput & (DUMP_ADAPTATION | DUMP_ALL)))
    {
        printf("UpshiftStreamGroup(%u): StreamGroupNum: %u. ClientInitiated: %d.\n",
            ulRate, ulStreamGroupNum, bIsClientInitiated);
        fflush(0);
    }

    return hr;
}

STDMETHODIMP
CASMStreamFilter::DownshiftStreamGroup(UINT32 ulStreamGroupNum,
                                       UINT32 ulRate,
                                       IHXStreamRateDescResponse* pResp,
                                       BOOL bIsClientInitiated)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    HX_RESULT hr = HXR_FAIL;
    hr = m_pUberMgr->DownshiftStreamGroup(ulStreamGroupNum, ulRate, pResp, bIsClientInitiated);
        
    // Dump adaptation info, if necessary
    if (SUCCEEDED(hr) && (m_lDebugOutput & (DUMP_ADAPTATION | DUMP_ALL)))
    {
        printf("DownshiftStreamGroup(%u): StreamGroupNum: %u. ClientInitiated: %d.\n",
            ulRate, ulStreamGroupNum, bIsClientInitiated);
        fflush(0);
    }

    return hr;
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

STDMETHODIMP
CASMStreamFilter::GetLowestAvgRate(UINT32 ulStreamGroupNum, REF(UINT32) ulLowestAvgRate)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->GetLowestAvgRate(ulStreamGroupNum, ulLowestAvgRate);
}

STDMETHODIMP
CASMStreamFilter::SetDownshiftOnFeedbackTimeoutFlag(BOOL bFlag)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->SetDownshiftOnFeedbackTimeoutFlag(bFlag);
}

STDMETHODIMP
CASMStreamFilter::GetNextSwitchableRateDesc(REF(IHXRateDescription*)pRateDesc)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    return m_pUberMgr->GetNextSwitchableRateDesc(pRateDesc);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CASMStreamFilter::UpshiftAggregate
// Purpose:
//  Upshifts to a higher rate
STDMETHODIMP
CASMStreamFilter::UpshiftAggregate(UINT32 ulRate, IHXRateDescResponse* pResp, BOOL bIsClientInitiated)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    HX_RESULT res = HXR_OK;

    // Perform actual uphift
    if (SUCCEEDED(res))
        res = m_pUberMgr->UpshiftAggregate(ulRate, NULL, bIsClientInitiated);

    // Dump adaptation info, if necessary
    if (m_lDebugOutput & (DUMP_ADAPTATION | DUMP_ALL))
    {
        printf("UpshiftAggregate(%u). ClientInitiated: %d.\n", ulRate, bIsClientInitiated);
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
//  CASMStreamFilter::SetRateSelectionInfoObject
// Purpose:
//  Provide initial rate selection info.

STDMETHODIMP
CASMStreamFilter::SetRateSelectionInfoObject(IHXRateSelectionInfo* pRateSelInfo)
{
    HX_RELEASE(m_pRateSelInfo);
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
CASMStreamFilter::DownshiftAggregate(UINT32 ulRate, IHXRateDescResponse* pResp, BOOL bIsClientInitiated)
{
    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    HX_RESULT res = HXR_OK;

    // Perform actual downshift
    if (SUCCEEDED(res))
        res = m_pUberMgr->DownshiftAggregate(ulRate, NULL, bIsClientInitiated);

    // Dump adaptation info, if necessary
    if (m_lDebugOutput & (DUMP_ADAPTATION | DUMP_ALL))
    {
        printf("DownshiftAggregate(%u). ClientInitiated: %d.\n", ulRate, bIsClientInitiated);
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
    if (m_lDebugOutput & (DUMP_ADAPTATION | DUMP_ALL))
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
    if (m_lDebugOutput & (DUMP_ADAPTATION | DUMP_ALL))
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

// Get the current active streams & rules and unsubscribe from those.
HX_RESULT
CASMStreamFilter::UnsubscribeAllOnDone()
{
    HX_RESULT res = HXR_OK;

    // Validate state
    if (!m_pUberMgr)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }
    
    for (UINT16 i = 0; i < m_pUberMgr->GetNumStreamGroups(); i++)
    {
        IHXRateDescription* pRateDesc = NULL;
        UINT32 ulSelectedLogiclStreamNum = MAX_UINT16;
        res = m_pUberMgr->GetSelectedLogicalStreamNum(i, ulSelectedLogiclStreamNum);

        if (SUCCEEDED(res))
        {
            res = m_pUberMgr->GetCurrentStreamGroupRateDesc(i, pRateDesc);
        }

        if (SUCCEEDED(res) && pRateDesc)
        {
            UINT32* aulRuleArray = pRateDesc->GetRuleArray();
            for (UINT16 j = 0; j < pRateDesc->GetNumRules(); j++)
            {
                m_pUberMgr->UnsubscribeLogicalStreamRule(ulSelectedLogiclStreamNum,
                    aulRuleArray[j], NULL);
            }
        }

        HX_RELEASE(pRateDesc);
    }

    return res;
}

