/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_info.cpp,v 1.18 2007/03/30 19:08:37 tknox Exp $ 
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "ihxpckts.h"
#include "servbuffer.h"
#include "hxtypes.h"
#include "proc.h"
#include "proc_container.h"
#include "bcast_defs.h"
#include "server_info.h"
#include "server_engine.h"
#include "server_context.h"
#include "servreg.h"
#include "loadinfo.h"
#include "shmem.h"
#include "globals.h"
#include "platform.h"
#ifdef _WINDOWS
#include "hxwinver.h"
#endif
#ifdef _UNIX
#include "shmem.h"
#endif
#include "server_version.h"
#include "sysinfo.h"
#include "rtspstats.h"

static sicFunc sicInc[SIC_LAST_COUNT] =
{
    &ServerInfo::IncrementStreamCount,
    &ServerInfo::IncrementMidBoxCount,
    &ServerInfo::IncrementRTSPClientCount,
    &ServerInfo::IncrementHTTPClientCount,
    &ServerInfo::IncrementMMSClientCount,
    &ServerInfo::IncrementTCPTransportCount,
    &ServerInfo::IncrementUDPTransportCount,
    &ServerInfo::IncrementMulticastTransportCount,
    &ServerInfo::IncrementCloakedClientCount,
};

static sicFunc sicDec[SIC_LAST_COUNT] =
{
    &ServerInfo::DecrementStreamCount,
    &ServerInfo::DecrementMidBoxCount,
    &ServerInfo::DecrementRTSPClientCount,
    &ServerInfo::DecrementHTTPClientCount,
    &ServerInfo::DecrementMMSClientCount,
    &ServerInfo::DecrementTCPTransportCount,
    &ServerInfo::DecrementUDPTransportCount,
    &ServerInfo::DecrementMulticastTransportCount,
    &ServerInfo::DecrementCloakedClientCount,
};

ServerInfoFacade::ServerInfoFacade(Process* pProc, ServerInfo* pServerInfo)
    : m_pProc (pProc)
    , m_pServerInfo (pServerInfo)
    , m_ulRefCount (0)
{
}

ServerInfoFacade::~ServerInfoFacade()
{
}

STDMETHODIMP
ServerInfoFacade::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXServerInfo))
    {
        AddRef();
        *ppvObj = (IHXServerInfo*) this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else
    {
        *ppvObj = NULL;
        return HXR_NOINTERFACE;
    }
}

STDMETHODIMP_(UINT32)
ServerInfoFacade::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
ServerInfoFacade::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

void
ServerInfoFacade::UpdateServerInfoCounter(ServerInfoCounters sicSelector, sicFunc* sicFuncArray)
{
    if (sicSelector < 0 || sicSelector >= SIC_LAST_COUNT)
    {
        return;
    }

    sicFunc pFunc = sicFuncArray[sicSelector];
    (m_pServerInfo->*pFunc) (m_pProc);
}

STDMETHODIMP_(void)
ServerInfoFacade::IncrementServerInfoCounter(ServerInfoCounters sicSelector)
{
    UpdateServerInfoCounter (sicSelector, sicInc);
}

STDMETHODIMP_(void)
ServerInfoFacade::DecrementServerInfoCounter(ServerInfoCounters sicSelector)
{
    UpdateServerInfoCounter (sicSelector, sicDec);
}

STDMETHODIMP_(UINT32*)
ServerInfoFacade::GetServerGlobalPointer(ServerGlobalSelector sgSelector)
{
    UINT32* pGlobal = NULL;

    if (sgSelector < 0 || sgSelector >= SG_LAST_GLOBAL)
    {
        return pGlobal; // XXX:TDK If we get a bogus selector, should we log an error?
    }

    switch (sgSelector)
    {
        case SG_BYTES_SERVED: 
            pGlobal = g_pBytesServed;
            break;
        case SG_PPS: 
            pGlobal = g_pPPS;
            break;
        default:
            break;  // XXX:TDK We added a new global selector, but didn't add it here. Log an error?
    }

    return pGlobal;
}

extern CSysInfo* g_pSysInfo;

ServerInfo::ServerInfo(Process* pProc)
	  : m_TotalClientCount(0)
          , m_pUsageCallback(NULL)
          , m_ulNewClients(0)
          , m_ulLeavingClients(0)
          , m_ulLastBytesServed(0)
          , m_ulLastForcedSelects(0)
          , m_ulLastPPS(0)
          , m_ulLastOverloads(0)
          , m_ulLastNoBufs(0)
          , m_ulLastOtherUDPErrs(0)
          , m_ulLastNewClients(0)
          , m_ulLastMutexCollisions(0)
          , m_ulLastMemOps(0)
          , m_ulLastSchedulerItems(0)
          , m_ulLastISchedulerItems(0)
          , m_PercentUserCPUUsage(0)
          , m_PercentKernCPUUsage(0)
          , m_PercentCPUUsage(0)
          , m_TotalCPUUsage(0)
          , m_pBroadcastRecvrStats(NULL)
          , m_pBroadcastDistStats(NULL)
#if ENABLE_LATENCY_STATS
          , m_ulCorePassCBTime(0)
          , m_ulCorePassCBCnt(0)
          , m_ulCorePassCBMax(0)
          , m_ulDispatchTime(0)
          , m_ulDispatchCnt(0)
          , m_ulDispatchMax(0)
          , m_ulStreamerTime(0)
          , m_ulStreamerCnt(0)
          , m_ulStreamerMax(0)
          , m_ulFirstReadTime(0)
          , m_ulFirstReadCnt(0)
          , m_ulFirstReadMax(0)
#endif
{
    IHXBuffer* buf = new ServerBuffer(TRUE);
    buf->Set((BYTE *)ServerVersion::VersionString(), 
	     strlen(ServerVersion::VersionString())+1);
    pProc->pc->registry->AddStr("Server.version", buf, pProc);
    HX_RELEASE(buf);

    buf = new ServerBuffer(TRUE);
#ifdef _WINDOWS
    char pWinVer[256];
    strcpy(pWinVer, HXGetOSName(HXGetWinVer(NULL)));
    buf->Set((BYTE *)pWinVer, strlen(pWinVer)+1);
#else
    buf->Set((BYTE *)ServerVersion::Platform(), 
	     strlen(ServerVersion::Platform())+1);
#endif
    pProc->pc->registry->AddStr("Server.platform", buf, pProc);
    HX_RELEASE(buf);

    pProc->pc->registry->AddInt("server.PacketAggLevel", 0, pProc);
    pProc->pc->registry->AddInt("server.RTSPAttemptedStreamed", 0, pProc);

    m_ClientCount = 0;
    m_RegID[PLYR_COUNT] = pProc->pc->registry->AddInt("Server.ClientCount", 
						     m_ClientCount, pProc);

    /*
     * for dist lic and ad serving/smil to work together. 
     * now the number of licenses given out depend on the number of 
     * streams being played by each player (realplayer, realsim, etc)
     * this use of a new var instead of 'server.clientCount' also fixes
     * an issue with the publisher's monitor which showed that the publisher's
     * client count was inclusive of all the subscriber's it was connected to.
     * sub1 = 10 conns, 
     * sub2 = 15 conns, 
     * pub = 5 connections (clients directly conencted to the pub)
     * the dist lic monitor would show that the pub had 30 (5+10+15) conns
     * 
     * for the other bug that is solved using this new var refer to the 
     * player.cpp file in the NewSession() and NewSessionWithID() methods
     */
    m_StreamCount = 0;
    m_RegID[STRM_COUNT] = pProc->pc->registry->AddInt("Server.StreamCount", 
						     m_StreamCount, pProc);
    m_MidBoxCount = 0;
    m_RegID[MIDBOX_COUNT] = pProc->pc->registry->AddInt("Server.MidBoxCount",
						     m_MidBoxCount, pProc);
    m_PNAClientCount = 0;
    m_RegID[PNA_CLNT_COUNT] = pProc->pc->registry->
					AddInt("Server.PNAClientCount",
					       m_PNAClientCount,
					       pProc);
    m_RTSPClientCount = 0;
    m_RegID[RTSP_CLNT_COUNT] = pProc->pc->registry->
					 AddInt("Server.RTSPClientCount",
						 m_RTSPClientCount,
					         pProc);
    m_MMSClientCount = 0;
    m_RegID[MMS_CLNT_COUNT] = pProc->pc->registry->
					 AddInt("Server.MMSClientCount",
						 m_MMSClientCount,
					         pProc);
    m_HTTPClientCount = 0;
    m_RegID[HTTP_CLNT_COUNT] = pProc->pc->registry->
					 AddInt("Server.HTTPClientCount",
						 m_HTTPClientCount,
					         pProc);
    m_TCPTransCount = 0;
    m_RegID[TCP_TRANS_COUNT] = pProc->pc->registry->
					 AddInt("Server.TCPTransportCount",
						 m_TCPTransCount,
						 pProc);
    m_UDPTransCount = 0;
    m_RegID[UDP_TRANS_COUNT] = pProc->pc->registry->
					 AddInt("Server.UDPTransportCount",
						 m_UDPTransCount,
						 pProc);
    m_MulticastTransCount = 0;
    m_RegID[MC_TRANS_COUNT] = pProc->pc->registry->
					 AddInt("Server.MulticastTransportCount",
						 m_MulticastTransCount,
						 pProc);
    m_CloakedClientCount = 0;
    m_RegID[CLOAKED_COUNT] = pProc->pc->registry->
					 AddInt("Server.CloakedClientCount",
						 m_CloakedClientCount,
						 pProc);
    m_BandwidthUsage = 0;
    m_RegID[TOTAL_BW_USAGE] = pProc->pc->registry->
					 AddInt("Server.BandwidthUsage",
						 m_BandwidthUsage,
						 pProc);

    m_PercentUserCPUUsage = 0;
    m_RegID[USER_CPU_USAGE] = pProc->pc->registry->
					 AddInt("Server.PercentUserCPUUsage",
						 m_PercentUserCPUUsage,
						 pProc);

    m_PercentKernCPUUsage = 0;
    m_RegID[KERN_CPU_USAGE] = pProc->pc->registry->
					 AddInt("Server.PercentKernCPUUsage",
						 m_PercentKernCPUUsage,
						 pProc);

    m_PercentCPUUsage = 0;
    m_RegID[SERVER_CPU_USAGE] = pProc->pc->registry->
					 AddInt("Server.PercentCPUUsage",
						 m_PercentCPUUsage,
						 pProc);

    m_TotalCPUUsage = 0;
    m_RegID[TOTAL_AGGCPU_USAGE] = pProc->pc->registry->
					 AddInt("Server.PercentAggCPUUsage",
						 m_TotalCPUUsage,
						 pProc);


    m_BytesMemoryUsage = 0;
    m_RegID[TOTAL_MEM_USAGE] = pProc->pc->registry->
					 AddInt("Server.BytesMemoryUsage",
						 m_BytesMemoryUsage,
						 pProc);
    m_FileObjectCount = 0;
    m_RegID[FILE_OBJECT_COUNT] = pProc->pc->registry->
					 AddInt("Server.FileObjectCount",
						 m_FileObjectCount,
						 pProc);
    m_ulTotalCAs = *g_pNumCrashAvoids;
    m_RegID[TOTAL_CAS] = pProc->pc->registry->
					 AddInt("Server.TotalCAs",
						 m_ulTotalCAs,
						 pProc);
#ifdef _UNIX
    pProc->pc->registry->AddInt("Server.BytesMemoryPool",
			SharedMemory::BytesInPool(),
			pProc);
#endif

    m_TotalClientCount = 0;
    m_RegID[TOTAL_CLNT_COUNT] = pProc->pc->registry->
	AddInt("Server.LastRegistryIDUsed", m_TotalClientCount, pProc);
    m_RegID[UPTIME] = pProc->pc->registry->
	AddInt("Server.Uptime", 0, pProc);
    m_RegID[BYTES_IN_USE] = pProc->pc->registry->
	AddInt("Server.BytesInUse", 0, pProc);
    m_RegID[BANDWIDTH_OUTPUT] = pProc->pc->registry->
	AddInt("Server.BandwidthOutput", 0, pProc);
    m_RegID[BYTES_OUTPUT_10SECS] = pProc->pc->registry->
	AddInt("Server.Last10SecondsBytesServed", 0, pProc);
    m_RegID[BANDWIDTH_OUTPUT_PER_PLAYER] = pProc->pc->registry->
	AddInt("Server.BandwidthOutputPerPlayer", 0, pProc);
    m_RegID[BYTES_OUTPUT_PER_PLAYER_10SECS] = pProc->pc->registry->
	AddInt("Server.Last10SecondsBytesServedPerPlayer", 0, pProc);
    m_RegID[PACKETS_OUTPUT_10SECS] = pProc->pc->registry->
	AddInt("Server.Last10SecondsPackets", 0, pProc);
    m_RegID[PACKET_OVERLOADS_10SECS] = pProc->pc->registry->
	AddInt("Server.Last10SecondsOverloads", 0, pProc);
    m_RegID[ENOBUFS_10SECS] = pProc->pc->registry->
	AddInt("Server.Last10SecondsENoBufs", 0, pProc);
    m_RegID[OTHER_UDP_ERRORS_10SECS] = pProc->pc->registry->
	AddInt("Server.Last10SecondsOtherUDPErrors", -1, pProc);
    IHXBuffer* pBuffer = new ServerBuffer(TRUE);
    pBuffer->Set((UINT8*)"Unknown", strlen("Unknown")+ 1);
    m_RegID[LOAD_STATE] = pProc->pc->registry->
	AddStr("Server.LoadState", pBuffer, pProc);
    pBuffer->Release();
    m_RegID[FORCED_SELECTS_10SECS] = pProc->pc->registry->
	AddInt("Server.Last10SecondsForcedSelects", 0, pProc);
    m_RegID[NEW_CLIENTS_10SECS] = pProc->pc->registry->
	AddInt("Server.Last10SecondsNewClients", 0, pProc);

    m_RegID[MAIN_LOOP_ITERS_10SECS] = pProc->pc->registry->
	AddInt("Server.Last10SecondsMainLoopIters", 0, pProc);
    m_RegID[MUTEX_COLLISIONS_10SECS] = pProc->pc->registry->
	AddInt("Server.Last10SecondsMutexCollisions", 0, pProc);
    m_RegID[MEM_OPS_10SECS] = pProc->pc->registry->
	AddInt("Server.Last10SecondsMemoryOps", 0, pProc);
    m_RegID[SCHEDULER_ITEMS_10SECS] = pProc->pc->registry->
	AddInt("Server.Last10SecondsSchedulerItems", 0, pProc);
    m_RegID[SCHEDULER_ITEMS_WITH_MUTEX_10SECS] = pProc->pc->registry->
	AddInt("Server.Last10SecondsSchedulerItemsWithMutex", 0, pProc);

    /* This callback is automatically run by OncePerSecond::Func() */
    m_pUsageCallback = new ServerInfo::UsageCallback(this);

    m_ulStartTime = pProc->pc->engine->now.tv_sec;
    m_ulUptime = 0;
    memset(m_pLastMainLoops, 0, sizeof(UINT32) * MAX_THREADS);
}

ServerInfo::~ServerInfo()
{
    PANIC(("Can't destruct ServerInfo"));

    HX_DELETE(m_pUsageCallback);
}

void
ServerInfo::Func(Process* pProc)
{
    m_pUsageCallback->Func(pProc);
}


ServerInfo::UsageCallback::UsageCallback(ServerInfo* pServerInfo)
    : m_pServerInfo(pServerInfo)
    , m_ulUpdateCounter(10)
{
    g_pSysInfo -> InitCPUCalc();
}

ServerInfo::UsageCallback::~UsageCallback()
{
}

/* This Func() is automatically run by OncePerSecond::Func() */
HX_RESULT
ServerInfo::UsageCallback::Func(Process* pProc)
{
    SharedMemory::UpdateBytesInUse();

    UINT32 ulInterval = 10;

    if (m_ulUpdateCounter >= ulInterval)
    {
	m_ulUpdateCounter = 1;
    }
    else
    {
	m_ulUpdateCounter++;
	return HXR_OK;
    }

    UINT32 ulMainLoops = 0;

    for (int i = 0; i < pProc->numprocs(); i++)
    {
        ulMainLoops += CounterDifference(&(m_pServerInfo->m_pLastMainLoops[i]), g_pMainLoops[i]);
    }

    UINT32 ulBytesServed   = CounterDifference(&(m_pServerInfo->m_ulLastBytesServed),
                                               *g_pBytesServed);
    UINT32 ulForcedSelects = CounterDifference(&(m_pServerInfo->m_ulLastForcedSelects),
                                               *g_pForcedSelects);
    UINT32 ulPPS           = CounterDifference(&(m_pServerInfo->m_ulLastPPS), *g_pPPS);
    UINT32 ulOverloads     = CounterDifference(&(m_pServerInfo->m_ulLastOverloads),
                                               *g_pOverloads);
    UINT32 ulNoBufs        = CounterDifference(&(m_pServerInfo->m_ulLastNoBufs), *g_pNoBufs);
    UINT32 ulOtherUDPErrs  = CounterDifference(&(m_pServerInfo->m_ulLastOtherUDPErrs),
                                               *g_pOtherUDPErrs);
    UINT32 ulMutexCollisions = CounterDifference(&(m_pServerInfo->m_ulLastMutexCollisions),
                                                 *g_pConcurrentOps);
    UINT32 ulMemOps        = CounterDifference(&(m_pServerInfo->m_ulLastMemOps),
                                               *g_pConcurrentMemOps);
    UINT32 ulSchedulerElems  = CounterDifference(&(m_pServerInfo->m_ulLastSchedulerItems),
                                                 *g_pSchedulerElems);
    UINT32 ulISchedulerElems = CounterDifference(&(m_pServerInfo->m_ulLastISchedulerItems),
                                                 *g_pISchedulerElems);

    UINT32 ulNewClients = CounterDifference(&(m_pServerInfo->m_ulLastNewClients), m_pServerInfo->m_ulNewClients);
    
    UINT32 ulBandwidthOutput = (UINT32) ((8.0) * ulBytesServed / ulInterval);
    UINT32 ulBytesPerPlayer = m_pServerInfo->m_ClientCount ? 
        ulBytesServed / m_pServerInfo->m_ClientCount : 0;
    UINT32 ulBandwidthPerPlayer = m_pServerInfo->m_ClientCount ?
        ulBandwidthOutput / m_pServerInfo->m_ClientCount : 0;

    m_pServerInfo->m_ulTotalCAs = *g_pNumCrashAvoids;

    // Update usage stats
#ifdef _UNIX
    m_pServerInfo->UpdateMemoryUsage(SharedMemory::BytesAllocated(), pProc);    
#elif defined _WINDOWS
    UINT32 ulMemUsage = 0;
    g_pSysInfo->GetMemUsage(ulMemUsage);
    m_pServerInfo->UpdateMemoryUsage(ulMemUsage, pProc);
#endif

    INT32 lUserCPUUsage = 0;
    INT32 lKernCPUUsage = 0;
    INT32 lAggCPUUsage = 0;
    g_pSysInfo->GetCPUUsage(lUserCPUUsage, lKernCPUUsage, lAggCPUUsage);
    m_pServerInfo->UpdateCPUUsage(lUserCPUUsage, lKernCPUUsage, lAggCPUUsage, pProc);

    m_pServerInfo->m_ulUptime = (pProc->pc->engine->now.tv_sec -
	m_pServerInfo->m_ulStartTime);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[UPTIME],
	m_pServerInfo->m_ulUptime, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	BYTES_IN_USE], SharedMemory::BytesInUse(), pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	BANDWIDTH_OUTPUT], ulBandwidthOutput, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	BYTES_OUTPUT_10SECS], ulBytesServed, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	BANDWIDTH_OUTPUT_PER_PLAYER], ulBandwidthPerPlayer, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	BYTES_OUTPUT_PER_PLAYER_10SECS], ulBytesPerPlayer, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	PACKETS_OUTPUT_10SECS], ulPPS, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	PACKET_OVERLOADS_10SECS], ulOverloads, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	ENOBUFS_10SECS], ulNoBufs, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	OTHER_UDP_ERRORS_10SECS], ulOtherUDPErrs, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	FORCED_SELECTS_10SECS], ulForcedSelects, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	NEW_CLIENTS_10SECS], ulNewClients, pProc);
	
    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	MAIN_LOOP_ITERS_10SECS], ulMainLoops, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	MUTEX_COLLISIONS_10SECS], ulMutexCollisions, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	MEM_OPS_10SECS], ulMemOps, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	SCHEDULER_ITEMS_10SECS], ulSchedulerElems, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	SCHEDULER_ITEMS_WITH_MUTEX_10SECS], ulISchedulerElems, pProc);

    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	TOTAL_CAS], m_pServerInfo->m_ulTotalCAs, pProc);


    //Using "LoadState" is depreciated since it's misleading and freaks out our users
    const char* pHealth;
    switch (pProc->pc->loadinfo->GetLoadState())
    {
    case NormalLoad:
        pHealth = "Normal";
        break;
    case HighLoad:  
        pHealth = "High Load";
        break;
    case ExtremeLoad:
        pHealth = "Extreme Load";
        break;
    default:
        pHealth = "Unknown";
        break;
    }
    int nPktAggLevel = pProc->pc->loadinfo->GetLoadState() + 1;
    pProc->pc->registry->SetInt("server.PacketAggLevel", nPktAggLevel, pProc);

    IHXBuffer* pBuf;

    if (!m_pServerInfo->m_pBroadcastRecvrStats && HXR_OK == 
           pProc->pc->registry->GetBuf(BRCV_REGISTRY_STATISTICS, &pBuf, pProc))
    {
        m_pServerInfo->m_pBroadcastRecvrStats = 
                                        *((BrcvStatistics**)pBuf->GetBuffer());
	HX_RELEASE(pBuf);
    }

    if (!m_pServerInfo->m_pBroadcastDistStats && HXR_OK == 
           pProc->pc->registry->GetBuf(BDST_REGISTRY_STATISTICS, &pBuf, pProc))
    {
        m_pServerInfo->m_pBroadcastDistStats = 
                                        *((BdstStatistics**)pBuf->GetBuffer());
	HX_RELEASE(pBuf);
    }

    IHXBuffer* pBuffer = new ServerBuffer(TRUE);
    pBuffer->Set((UINT8*)pHealth, strlen(pHealth)+1);
    pProc->pc->registry->SetStr(m_pServerInfo->m_RegID[
	LOAD_STATE], pBuffer, pProc);
    pBuffer->Release();

    m_pServerInfo->m_FileObjectCount = *g_pFileObjs;
    pProc->pc->registry->SetInt(m_pServerInfo->m_RegID[
	FILE_OBJECT_COUNT], m_pServerInfo->m_FileObjectCount, pProc);
        
    return HXR_OK;
}
