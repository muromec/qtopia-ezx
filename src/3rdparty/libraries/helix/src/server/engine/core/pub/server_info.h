/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_info.h,v 1.13 2007/03/30 19:08:38 tknox Exp $ 
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

#ifndef _SERVER_INFO_H_
#define _SERVER_INFO_H_

#include "proc.h"
#include "servreg.h"
#include "client.h"
#include "bcast_defs.h"
#ifdef _UNIX
#include "sys/types.h"
#include "sys/time.h"
#include "sys/resource.h"
#endif

#include "hxservinfo.h"

class RSSCoreStatsReport;
class LoadInfo;
class ServerInfo;
class Process;

typedef void (ServerInfo::* sicFunc)(Process* pProc);

class ServerInfoFacade : public IHXServerInfo
{
public:
    ServerInfoFacade (Process* pProc, ServerInfo* pServerInfo);
    virtual ~ServerInfoFacade();

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *  IHXServerInfo methods
     */
    STDMETHOD_(void, IncrementServerInfoCounter)    (THIS_ ServerInfoCounters sicSelector);
    STDMETHOD_(void, DecrementServerInfoCounter)    (THIS_ ServerInfoCounters sicSelector);
    STDMETHOD_(UINT32, IncrementTotalClientCount)   (THIS);
    STDMETHOD_(void, ChangeBandwidthUsage)          (THIS_ INT32 lBandwidth);
    STDMETHOD_(UINT32*, GetServerGlobalPointer)     (THIS_ ServerGlobalSelector sgSelector);

private:
    void UpdateServerInfoCounter(ServerInfoCounters sicSelector, sicFunc*);

    Process*	m_pProc;
    ServerInfo*	m_pServerInfo;
    ULONG32     m_ulRefCount;
};


class ServerInfo
{
public:
			ServerInfo(Process* pProc);
			~ServerInfo();

    static inline UINT32 CounterDifference(UINT32* pLast, UINT32 ulCounter);
    static inline double CounterDifference(double* pLast, double fCounter);


    inline void		IncrementClientCount(Process* pProc);
    inline void		DecrementClientCount(Process* pProc);
    inline void		IncrementStreamCount(Process* pProc);
    inline void		DecrementStreamCount(Process* pProc);
    inline void         IncrementMidBoxCount(Process* pProc);
    inline void         DecrementMidBoxCount(Process* pProc);
    inline void		IncrementPNAClientCount(BOOL bIsCloaked, Process* pProc);
    inline void		DecrementPNAClientCount(BOOL bIsCloaked, Process* pProc);
    inline void		IncrementRTSPClientCount(BOOL bIsCloaked, Process* pProc);
    inline void		DecrementRTSPClientCount(BOOL bIsCloaked, Process* pProc);
    inline void		IncrementHTTPClientCount(Process* pProc);
    inline void		DecrementHTTPClientCount(Process* pProc);
    inline void		IncrementMMSClientCount(Process* pProc);
    inline void		DecrementMMSClientCount(Process* pProc);
    inline void		IncrementTCPTransportCount(Process* pProc);
    inline void		DecrementTCPTransportCount(Process* pProc);
    inline void		IncrementUDPTransportCount(Process* pProc);
    inline void		DecrementUDPTransportCount(Process* pProc);
    inline void		IncrementMulticastTransportCount(Process* pProc);
    inline void		DecrementMulticastTransportCount(Process* pProc);
    inline void		IncrementCloakedClientCount(Process* pProc);
    inline void		DecrementCloakedClientCount(Process* pProc);
    inline UINT32	IncrementTotalClientCount(Process* pProc);

#if XXXAAKTESTRTSP
    inline void		IncrementOptionsMsgParsedCount(Process* pProc);
    inline void		IncrementDescribeMsgParsedCount(Process* pProc);
    inline void		IncrementSetupMsgParsedCount(Process* pProc);
    inline void		IncrementSetParameterMsgParsedCount(Process* pProc);
    inline void		IncrementPlayMsgParsedCount(Process* pProc);
    inline void		IncrementTeardownMsgParsedCount(Process* pProc);
#endif

    inline void		ResetMainLoopCounters();

    inline void		ChangeBandwidthUsage(INT32 lBandwidth, Process* pProc);
    inline void		UpdateCPUUsage(INT32 lPercentUserCPUUsage, INT32 lPercentKernCPUUsage,
                                       INT32 lPercentAggCPUUsage, Process* pProc);
    inline void		UpdateMemoryUsage(INT32 lBytesMemUsage, Process* pProc);

#if ENABLE_LATENCY_STATS
    inline void         UpdateCorePassCBLatency(UINT32 lCorePassCBTime);
    inline void         UpdateDispatchLatency(UINT32 lDispatchTime);
    inline void         UpdateStreamerLatency(UINT32 lStreamerTime);
    inline void         UpdateFirstReadLatency(UINT32 lFirstReadTime);
#endif

    void		Func(Process* pProc);
    inline UINT32	Uptime() { return m_ulUptime; }

    class UsageCallback
    {
    public:
	UsageCallback(ServerInfo* pServerInfo);
	~UsageCallback();

        HX_RESULT	Func(Process* pProc);
    
    private:
	ServerInfo*	m_pServerInfo;
	UINT32		m_ulUpdateCounter;
    };

    // Wrappers needed *only* for ServerInfoFacade to use a func lookup table
    inline void		IncrementRTSPClientCount(Process* pProc);
    inline void		DecrementRTSPClientCount(Process* pProc);

private:

    // XXXAAK -- testing registry stuff
    enum Field
    {
	PLYR_COUNT,
	STRM_COUNT,
        MIDBOX_COUNT,
	PNA_CLNT_COUNT,
	RTSP_CLNT_COUNT,
	HTTP_CLNT_COUNT,
	MMS_CLNT_COUNT,
	TCP_TRANS_COUNT,
	UDP_TRANS_COUNT,
	MC_TRANS_COUNT,		// MultiCast
	CLOAKED_COUNT,
	TOT_BYTES_SENT,
	TOTAL_BW_USAGE,
	USER_CPU_USAGE,
	KERN_CPU_USAGE,
	SERVER_CPU_USAGE,
	TOTAL_AGGCPU_USAGE,
	TOTAL_MEM_USAGE,
	TOTAL_CLNT_COUNT,

#if XXXAAKTESTRTSP
	RTSP_OPTIONS_MSG_PARSED_COUNT,
	RTSP_DESCRIBE_MSG_PARSED_COUNT,
	RTSP_SETUP_MSG_PARSED_COUNT,
	RTSP_SET_PARAMETER_MSG_PARSED_COUNT,
	RTSP_PLAY_MSG_PARSED_COUNT,
	RTSP_TEARDOWN_MSG_PARSED_COUNT,
#endif

	UPTIME,
        FILE_OBJECT_COUNT,
	BYTES_IN_USE,
	BANDWIDTH_OUTPUT,
	BYTES_OUTPUT_10SECS,
	BANDWIDTH_OUTPUT_PER_PLAYER,
	BYTES_OUTPUT_PER_PLAYER_10SECS,
	PACKETS_OUTPUT_10SECS,
	PACKET_OVERLOADS_10SECS,
	ENOBUFS_10SECS,
	OTHER_UDP_ERRORS_10SECS,
	LOAD_STATE,
	FORCED_SELECTS_10SECS,
        NEW_CLIENTS_10SECS,
        MAIN_LOOP_ITERS_10SECS,
        MUTEX_COLLISIONS_10SECS,
        MEM_OPS_10SECS,
        SCHEDULER_ITEMS_10SECS,
        SCHEDULER_ITEMS_WITH_MUTEX_10SECS,
        TOTAL_CAS,
	MAX_FIELDS
    };

    UINT32		m_RegID[MAX_FIELDS];

    INT32		m_ClientCount;
    INT32		m_StreamCount;
    INT32               m_MidBoxCount;
    INT32		m_PNAClientCount;
    INT32		m_RTSPClientCount;
    INT32		m_HTTPClientCount;
    INT32		m_MMSClientCount;
    INT32		m_TCPTransCount;
    INT32		m_UDPTransCount;
    INT32		m_MulticastTransCount;
    INT32		m_CloakedClientCount;
    INT32		m_TotalClientCount;
    INT32		m_BandwidthUsage;
    INT32		m_PercentUserCPUUsage;
    INT32		m_PercentKernCPUUsage;
    INT32		m_PercentCPUUsage;
    INT32               m_TotalCPUUsage;
    INT32		m_BytesMemoryUsage;
    UINT32		m_ulStartTime;
    UINT32		m_ulUptime;
    INT32               m_FileObjectCount;
    UINT32              m_ulTotalCAs;
    UINT32              m_ulNewClients;
    UINT32              m_ulLastNewClients;
    UINT32              m_ulLeavingClients;

#if XXXAAKTESTRTSP
    UINT32		m_ulOptionsMsg;
    UINT32		m_ulDescribeMsg;
    UINT32		m_ulSetupMsg;
    UINT32		m_ulSetParameterMsg;
    UINT32		m_ulPlayMsg;
    UINT32		m_ulTeardownMsg;
#endif

    UINT32		m_ulLastBytesServed;
    UINT32		m_ulLastForcedSelects;
    UINT32		m_ulLastPPS;
    UINT32		m_ulLastOverloads;
    UINT32		m_ulLastNoBufs;
    UINT32		m_ulLastOtherUDPErrs;
    UINT32		m_ulLastClients;
    UINT32		m_ulLastMutexCollisions;
    UINT32		m_ulLastMemOps;
    UINT32		m_ulLastSchedulerItems;
    UINT32		m_ulLastISchedulerItems;

    UINT32		m_pLastMainLoops[MAX_THREADS];

    BrcvStatistics*     m_pBroadcastRecvrStats;
    BdstStatistics*     m_pBroadcastDistStats;


#if ENABLE_LATENCY_STATS
    INT32             m_ulCorePassCBTime;
    INT32             m_ulCorePassCBCnt;
    INT32             m_ulCorePassCBMax;
    INT32             m_ulDispatchTime;
    INT32             m_ulDispatchCnt;
    INT32             m_ulDispatchMax;
    INT32             m_ulStreamerTime;
    INT32             m_ulStreamerCnt;
    INT32             m_ulStreamerMax;
    INT32             m_ulFirstReadTime;
    INT32             m_ulFirstReadCnt;
    INT32             m_ulFirstReadMax;
#endif

    UsageCallback*	m_pUsageCallback;
    friend class RSSCoreStatsReport;
    friend class LoadInfo;
    friend class UsageCallback;
    friend class ServerInfoFacade;
};

inline void
ServerInfoFacade::ChangeBandwidthUsage (INT32 lBandwidth)
{
    m_pServerInfo->ChangeBandwidthUsage (lBandwidth, m_pProc);
}

inline UINT32
ServerInfoFacade::IncrementTotalClientCount ()
{
    return m_pServerInfo->IncrementTotalClientCount (m_pProc);
}

inline UINT32
ServerInfo::CounterDifference(UINT32* pLast, UINT32 ulCounter)
{
    UINT32 ulTempLast;
    ulTempLast = *pLast;
    *pLast = ulCounter;

    if (ulTempLast > ulCounter) 
        return (UINT32)(1 + (0xffffffff - ulTempLast) + ulCounter);
    
    return (UINT32)(ulCounter - ulTempLast);        
}

inline double
ServerInfo::CounterDifference(double* pLast, double fCounter)
{
    return fCounter - *pLast;
}

inline void
ServerInfo::IncrementClientCount(Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[PLYR_COUNT], 1,
                                   &m_ClientCount, pProc);
    HXAtomicIncUINT32(&m_ulNewClients);
}

inline void
ServerInfo::DecrementClientCount(Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[PLYR_COUNT], -1,
                                   &m_ClientCount, pProc);
    HXAtomicIncUINT32(&m_ulLeavingClients);
}

inline void
ServerInfo::IncrementStreamCount(Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[STRM_COUNT], 1,
                                   &m_StreamCount, pProc);
}

inline void
ServerInfo::DecrementStreamCount(Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[STRM_COUNT], -1,
                                   &m_StreamCount, pProc);
}

inline void
ServerInfo::IncrementMidBoxCount(Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[MIDBOX_COUNT], 1,
                                   &m_MidBoxCount, pProc);
}

inline void
ServerInfo::DecrementMidBoxCount(Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[MIDBOX_COUNT], -1,
                                   &m_MidBoxCount, pProc);
}

inline void
ServerInfo::IncrementPNAClientCount(BOOL bIsCloaked, Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[PNA_CLNT_COUNT], 1,
                                   &m_PNAClientCount, pProc);
    IncrementClientCount(pProc);
    if (bIsCloaked)
    {
        IncrementCloakedClientCount(pProc);
    }           
}

inline void
ServerInfo::DecrementPNAClientCount(BOOL bIsCloaked, Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[PNA_CLNT_COUNT], -1,
                                   &m_PNAClientCount, pProc);
    DecrementClientCount(pProc);
    if (bIsCloaked)
    {
        DecrementCloakedClientCount(pProc);
    }           
}

inline void
ServerInfo::IncrementRTSPClientCount(BOOL bIsCloaked, Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[RTSP_CLNT_COUNT], 1,
                                   &m_RTSPClientCount, pProc);
    IncrementClientCount(pProc);
    if (bIsCloaked)
    {
        IncrementCloakedClientCount(pProc);
    }           
}

inline void
ServerInfo::DecrementRTSPClientCount(BOOL bIsCloaked, Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[RTSP_CLNT_COUNT], -1,
                                   &m_RTSPClientCount, pProc);
    DecrementClientCount(pProc);
    if (bIsCloaked)
    {
        DecrementCloakedClientCount(pProc);
    }           
}

inline void
ServerInfo::IncrementRTSPClientCount(Process* pProc)
{
    IncrementRTSPClientCount(FALSE, pProc);
}

inline void
ServerInfo::DecrementRTSPClientCount(Process* pProc)
{
    DecrementRTSPClientCount(FALSE, pProc);
}

inline void
ServerInfo::IncrementMMSClientCount(Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[MMS_CLNT_COUNT], 1,
                                   &m_MMSClientCount, pProc);
    IncrementClientCount(pProc);
}

inline void
ServerInfo::DecrementMMSClientCount(Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[MMS_CLNT_COUNT], -1,
                                   &m_MMSClientCount, pProc);
    DecrementClientCount(pProc);
}

inline void
ServerInfo::IncrementHTTPClientCount(Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[HTTP_CLNT_COUNT], 1,
                                   &m_HTTPClientCount, pProc);
    IncrementClientCount(pProc);
}

inline void
ServerInfo::DecrementHTTPClientCount(Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[HTTP_CLNT_COUNT], -1,
                                   &m_HTTPClientCount, pProc);
    DecrementClientCount(pProc);
}

inline void
ServerInfo::IncrementTCPTransportCount(Process* pProc)
{
    HXAtomicIncINT32(&m_TCPTransCount);
    pProc->pc->registry->SetInt(m_RegID[TCP_TRANS_COUNT], m_TCPTransCount,
			        pProc);
}

inline void
ServerInfo::DecrementTCPTransportCount(Process* pProc)
{
    HXAtomicDecINT32(&m_TCPTransCount);
    pProc->pc->registry->SetInt(m_RegID[TCP_TRANS_COUNT], m_TCPTransCount,  
			        pProc);
}

inline void
ServerInfo::IncrementUDPTransportCount(Process* pProc)
{
    HXAtomicIncINT32(&m_UDPTransCount);
    pProc->pc->registry->SetInt(m_RegID[UDP_TRANS_COUNT], m_UDPTransCount,
			        pProc);
}

inline void
ServerInfo::DecrementUDPTransportCount(Process* pProc)
{
    HXAtomicDecINT32(&m_UDPTransCount);
    pProc->pc->registry->SetInt(m_RegID[UDP_TRANS_COUNT], m_UDPTransCount,  
			        pProc);
}

inline void
ServerInfo::IncrementMulticastTransportCount(Process* pProc)
{
    HXAtomicIncINT32(&m_MulticastTransCount);
    pProc->pc->registry->SetInt(m_RegID[MC_TRANS_COUNT], m_MulticastTransCount,
			        pProc);
}

inline void
ServerInfo::DecrementMulticastTransportCount(Process* pProc)
{
    HXAtomicDecINT32(&m_MulticastTransCount);
    pProc->pc->registry->SetInt(m_RegID[MC_TRANS_COUNT], m_MulticastTransCount,  
			        pProc);
}

inline void
ServerInfo::IncrementCloakedClientCount(Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[CLOAKED_COUNT], 1,
                                   &m_CloakedClientCount, pProc);
}

inline void
ServerInfo::DecrementCloakedClientCount(Process* pProc)
{
    pProc->pc->registry->ModifyInt(m_RegID[CLOAKED_COUNT], -1,
                                   &m_CloakedClientCount, pProc);
}

inline UINT32
ServerInfo::IncrementTotalClientCount(Process* pProc)
{
    UINT32 seq_num = (UINT32)HXAtomicIncRetINT32(&m_TotalClientCount);
    pProc->pc->registry->SetInt(m_RegID[TOTAL_CLNT_COUNT], m_TotalClientCount,  
			        pProc);
    return seq_num;
}

#if XXXAAKTESTRTSP
inline void
ServerInfo::IncrementOptionsMsgParsedCount(Process* pProc)
{
    HXAtomicIncUINT32(&m_ulOptionsMsg);
}
inline void
ServerInfo::IncrementDescribeMsgParsedCount(Process* pProc)
{
    HXAtomicIncUINT32(&m_ulDescribeMsg);
}
inline void
ServerInfo::IncrementSetupMsgParsedCount(Process* pProc)
{
    HXAtomicIncUINT32(&m_ulSetupMsg);
}
inline void
ServerInfo::IncrementSetParameterMsgParsedCount(Process* pProc)
{
    HXAtomicIncUINT32(&m_ulSetParameterMsg);
}
inline void
ServerInfo::IncrementPlayMsgParsedCount(Process* pProc)
{
    HXAtomicIncUINT32(&m_ulPlayMsg);
}
inline void
ServerInfo::IncrementTeardownMsgParsedCount(Process* pProc)
{
    HXAtomicIncUINT32(&m_ulTeardownMsg);
}
#endif

inline void
ServerInfo::ResetMainLoopCounters()
{
    memset(m_pLastMainLoops, 0, sizeof(UINT32) * MAX_THREADS);
}

inline void
ServerInfo::ChangeBandwidthUsage(INT32 lBandwidth, Process* pProc)
{
    if (lBandwidth == 0)
    {
	return;
    }
#ifdef DEBUG
    INT32 ulTemp = HXAtomicAddRetINT32(&m_BandwidthUsage, lBandwidth);
    ASSERT(ulTemp >= 0);
#else
    HXAtomicAddINT32(&m_BandwidthUsage, lBandwidth);
#endif
    pProc->pc->registry->SetInt(m_RegID[TOTAL_BW_USAGE], m_BandwidthUsage,  
			        pProc);
}

inline void
ServerInfo::UpdateCPUUsage(INT32 lPercentUserCPUUsage, INT32 lPercentKernCPUUsage,
                           INT32 lPercentAggCPUUsage, Process* pProc)
{
    m_PercentUserCPUUsage = lPercentUserCPUUsage;
    pProc->pc->registry->SetInt(m_RegID[USER_CPU_USAGE], m_PercentUserCPUUsage,  
			        pProc);

    m_PercentKernCPUUsage = lPercentKernCPUUsage;
    pProc->pc->registry->SetInt(m_RegID[KERN_CPU_USAGE], m_PercentKernCPUUsage,  
			        pProc);

    m_PercentCPUUsage = m_PercentUserCPUUsage + m_PercentKernCPUUsage;
    pProc->pc->registry->SetInt(m_RegID[SERVER_CPU_USAGE], m_PercentCPUUsage,
			        pProc);

    m_TotalCPUUsage = lPercentAggCPUUsage;
    pProc->pc->registry->SetInt(m_RegID[TOTAL_AGGCPU_USAGE], m_TotalCPUUsage,  
			        pProc);
}

inline void
ServerInfo::UpdateMemoryUsage(INT32 lBytesMemoryUsage, Process* pProc)
{
    m_BytesMemoryUsage = lBytesMemoryUsage;
    pProc->pc->registry->SetInt(m_RegID[TOTAL_MEM_USAGE], m_BytesMemoryUsage,  
			        pProc);
}


#if ENABLE_LATENCY_STATS
inline void
ServerInfo::UpdateCorePassCBLatency(UINT32 lCorePassCBTime)
{
    HXAtomicAddINT32(&m_ulCorePassCBTime, lCorePassCBTime);
    HXAtomicIncINT32(&m_ulCorePassCBCnt);
    if (lCorePassCBTime > m_ulCorePassCBMax)
    {
        m_ulCorePassCBMax = lCorePassCBTime;
    }

}
inline void
ServerInfo::UpdateDispatchLatency(UINT32 lDispatchTime)
{
    HXAtomicAddINT32(&m_ulDispatchTime, lDispatchTime);
    HXAtomicIncINT32(&m_ulDispatchCnt);
    if (lDispatchTime > m_ulDispatchMax)
    {
        m_ulDispatchMax = lDispatchTime;
    }

}
inline void
ServerInfo::UpdateStreamerLatency(UINT32 lStreamerTime)
{
    HXAtomicAddINT32(&m_ulStreamerTime, lStreamerTime);
    HXAtomicIncINT32(&m_ulStreamerCnt);
    if (lStreamerTime > m_ulStreamerMax)
    {
        m_ulStreamerMax = lStreamerTime;
    }

}
inline void
ServerInfo::UpdateFirstReadLatency(UINT32 lFirstReadTime)
{
    HXAtomicAddINT32(&m_ulFirstReadTime, lFirstReadTime);
    HXAtomicIncINT32(&m_ulFirstReadCnt);
    if (lFirstReadTime > m_ulFirstReadMax)
    {
        m_ulFirstReadMax = lFirstReadTime;
    }
}
#endif


#endif /* _SERVER_INFO_H_ */
