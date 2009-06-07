/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: rtspstats.h,v 1.7 2004/04/08 20:26:58 darrick Exp $ 
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

#ifndef _RTSPSTATS_H
#define _RTSPSTATS_H

#include "hxtypes.h"
#include "hxpiids.h"
#include "hxrtsp2.h"
#include "hxslist.h"
#include "hxmap.h"
#include "mutex.h"
#include "simple_callback.h"


#if defined _WINDOWS && !defined snprintf
#define snprintf _snprintf
#endif


class RTSPAggregateStats;

_INTERFACE IHXClientStatsManager;
_INTERFACE IHXClientStats;

class Process;


///////////////////////////////////////////////////////////////////////////////
//  Helper container structures.
///////////////////////////////////////////////////////////////////////////////

struct RTSPEventInfo
{
    UINT16 usType;
    UINT16 usSrc;
    UINT32 ulStatus;
    UINT32 ulTs;
    UINT32 ulCSeq;
};


struct RTSPEventID 
{
    UINT16 usType;
    const char *szShortEventName;
    UINT32 ulEventMask;
};
    

///////////////////////////////////////////////////////////////////////////////
//  RTSPStats - Instantiated 1 per RTSPServerProtocol.
///////////////////////////////////////////////////////////////////////////////

class RTSPStats : public IUnknown 
{

public:

    RTSPStats();
    virtual ~RTSPStats();

    enum {MAX_RTSP_METHODS = 13,  MAX_EVENT_INFO_BUFFSIZE = 38}; 

    HX_RESULT Init(IUnknown* pContext);
    
    HX_RESULT SetClientStatsObj(IHXClientStats* pClientStats);
    
    HX_RESULT UpdateRequestStats(RTSPMethod usEventType, 
                                 UINT32 ulCSeq,
                                 BOOL bServerOrigin);
    HX_RESULT UpdateResponseStats(UINT32 ulStatusCode,
                                  UINT32 ulCSeq,
                                  BOOL bServerOrigin);

    HX_RESULT DumpEvents(BOOL bDumpPendingEvents = FALSE);
    HX_RESULT BuildEventString(CHXSimpleList* pEventInfoList, 
                               IHXBuffer*& pEventBuff);

    void ReportError(const UINT8 uiSeverity,    
                     HX_RESULT result, 
                     const char* pFormat, 
                     ...);

    // IUnknown methods.

    STDMETHODIMP QueryInterface(REFIID ID, void** ppInterfaceObj);     
    STDMETHODIMP_(UINT32) AddRef();     
    STDMETHODIMP_(UINT32) Release();

    HX_RESULT ReportEvents(IHXBuffer* pBuffer, 
                           IHXSessionStats* pSessionStats,
                           IHXBuffer* pSessionID);

    static BOOL FeatureConfigured();
    static BOOL FeatureEnabled();
    static BOOL FeatureLicensed();

    enum RTSPEventSource { CLIENT_REQUEST,
                           SERVER_REQUEST,
                           CLIENT_RESPONSE,
                           SERVER_RESPONSE,
                           UNKOWN_SRC };


    struct RTSPEventCode
    {   
        UINT16 usSrcId;
        const char *szShortSrcName;
    };
    
protected:

    HX_RESULT ReadConfig();
    HX_RESULT CheckLicense();

    UINT32 m_ulRefCount;
    UINT32 m_ulMaxEvents;

    IHXRegistry2* m_pRegistry;
    IHXCommonClassFactory* m_pClassFactory;
    IHXScheduler* m_pScheduler;
    IHXErrorMessages* m_pErrorLog;
    IHXRTSPEventsManager* m_pEventsMgr;
    IHXClientStatsManager* m_pClientStatsManager;
    IHXClientStats* m_pClientStats;

    CHXMapLongToObj m_EventsPendingResponse; // Batches requests waiting for a response.
    CHXSimpleList m_OutputQueue; // Events scheduled for immediate output.

    static RTSPEventCode zm_EventCodeList[UNKOWN_SRC];
    static const char* zm_szTStatTemplate;

    static BOOL zm_bConfigRead;
    static BOOL zm_bLicenseChecked;

    static BOOL zm_bEnabled;
    static BOOL zm_bLicensed;

    static UINT32 zm_ulMaxEvents;
    static UINT32 zm_ulEventMask;
    
    friend class RTSPAggregateStats;
    friend class RTSPSessionEventsList;
    friend struct RTSPStatInfo;
};

///////////////////////////////////////////////////////////////////////////////
//  RTSPSessionEventsList - Instantiated 1 per RTSPServerProtocol::Session
///////////////////////////////////////////////////////////////////////////////

class RTSPSessionEventsList
{
public:

    RTSPSessionEventsList();
    virtual ~RTSPSessionEventsList();

    HX_RESULT Init(RTSPStats* pRTSPEventMgr,
                   const CHXString& SessionID);

    void SetSessionStats(IHXSessionStats* pSessionStats);

    HX_RESULT UpdateRequestStats(RTSPMethod usEventType, 
                                 UINT32 ulCSeq,
                                 BOOL bServerOrigin);

    HX_RESULT UpdateResponseStats(UINT32 ulStatusCode,
                                  UINT32 ulCSeq,
                                  BOOL bServerOrigin);

    HX_RESULT DumpEvents(BOOL bDumpPendingEvents = FALSE);

protected:

    IHXSessionStats* m_pSessionStats;
    CHXMapLongToObj m_EventsPendingResponse; // Batches requests waiting for a response.
    CHXSimpleList m_OutputQueue; // Events scheduled for immediate output.
    RTSPStats* m_pRTSPEventMgr;
    IHXBuffer* m_pSessionID;

    friend class RTSPStats;
};

///////////////////////////////////////////////////////////////////////////////
// RTSPEventsManager Class - Instantiated one per server
///////////////////////////////////////////////////////////////////////////////

class RTSPEventsManager 
{
public:

    RTSPEventsManager();
    virtual ~RTSPEventsManager();


    // IHXRTSPEventsManager methods   

    HX_RESULT RegisterSink(IHXRTSPEventsSink* pSink, Process* pProc);
    HX_RESULT RemoveSink(IHXRTSPEventsSink* pSink);
    HX_RESULT OnRTSPEvents(IHXClientStats* pClientStats,
                           IHXSessionStats* pSessionStats,
                           IHXBuffer* pEvents,
                           IHXBuffer* pRTSPSessionID);
    IHXRTSPAggregateEventStats* GetAggregateStats();


    HX_RESULT Init(IUnknown* pContext);
    void Cleanup();


    ///////////////////////////////////////////////////////////////////////////
    // SinkListElem - created by RegisterSink()
    ///////////////////////////////////////////////////////////////////////////

    class SinkListElem 
    {
    public:

        SinkListElem() : m_pSink(NULL), m_pProc(NULL) {}
        ~SinkListElem() { HX_RELEASE(m_pSink); }

        IHXRTSPEventsSink* m_pSink;
        Process* m_pProc;
    };



    ///////////////////////////////////////////////////////////////////////////
    // SinkNotifyCallback - created by OnRTSPEvents()
    ///////////////////////////////////////////////////////////////////////////

    class SinkNotifyCallback 
    : public SimpleCallback
    {
    protected:

        IHXClientStats* m_pClientStats;
        IHXSessionStats* m_pSessionStats;
        IHXRTSPEventsSink* m_pSink;
        Process* m_pProc;
        IHXBuffer* m_pRTSPEvents;

    public:

        SinkNotifyCallback();
        ~SinkNotifyCallback();

    
        HX_RESULT Init(IHXClientStats* pClientStats, 
                       IHXSessionStats* pSessionStats,
                       IHXRTSPEventsSink* pSink,
                       Process* pProc,
                       IHXBuffer* pRTSPEvents);

        void func(Process* pProc);
    };


protected:

    BOOL m_bInitDone;

    //XXXDPL This list is not threadsafe! If there is ever more than
    // one sink (or if a sink ever gets removed) this list will need 
    // to be made threadsafe!
    CHXSimpleList m_SinkList;
    RTSPAggregateStats* m_pAggStats;
};


///////////////////////////////////////////////////////////////////////////////
// RTSPEventsManagerProcWrapper - Instantiated whenever context QIs
// for IHXRTSPEventsManager
///////////////////////////////////////////////////////////////////////////////

class RTSPEventsManagerProcWrapper : public IHXRTSPEventsManager
{

public:

    RTSPEventsManagerProcWrapper(RTSPEventsManager* pEventsMgr,
                                 Process* pProc);
    virtual ~RTSPEventsManagerProcWrapper();

    // IUnknown methods.

    STDMETHODIMP QueryInterface(REFIID ID, void** ppInterfaceObj);     
    STDMETHODIMP_(UINT32) AddRef();     
    STDMETHODIMP_(UINT32) Release();

    // IHXRTSPEventsManager methods   

    STDMETHODIMP RegisterSink(IHXRTSPEventsSink* pSink);
    STDMETHODIMP RemoveSink(IHXRTSPEventsSink* pSink);
    STDMETHODIMP OnRTSPEvents(IHXClientStats* pClientStats,
                              IHXSessionStats* pSessionStats,
                              IHXBuffer* pEvents,
                              IHXBuffer* pRTSPSessionID);
    STDMETHODIMP_(IHXRTSPAggregateEventStats*) GetAggregateStats();

protected:

    UINT32 m_ulRefCount;
    RTSPEventsManager* m_pEventsMgr;
    Process* m_pProc;

};


///////////////////////////////////////////////////////////////////////////////
// RTSPAggregateStats Class - Instantiated one per server
///////////////////////////////////////////////////////////////////////////////

class RTSPAggregateStats : public IHXRTSPAggregateEventStats
//                         , public IHXPropWatchResponse 
{ 
public:

    RTSPAggregateStats();
    virtual ~RTSPAggregateStats();

    // IUnknown methods.

    STDMETHODIMP QueryInterface(REFIID ID, void** ppInterfaceObj);     
    STDMETHODIMP_(UINT32) AddRef();     
    STDMETHODIMP_(UINT32) Release();

    // IHXRTSPAggregateStats methods

    STDMETHODIMP UpdateClientRequestCount(INT32 lCount, 
                                          RTSPMethod ulMethodId);
    STDMETHODIMP UpdateServerRequestCount(INT32 lCount, 
                                          RTSPMethod ulMethodId);
    STDMETHODIMP UpdateClientResponseCount(INT32 lCount, 
                                           RTSPMethod ulMethodId,
                                                   UINT32 ulStatusCode);
    STDMETHODIMP UpdateServerResponseCount(INT32 lCount, 
                                           RTSPMethod ulMethodId,
                                           UINT32 ulStatusCode);

    HX_RESULT GetLicense(REF(BOOL) bIsLicensed);

    // IHXPropWatchResponse methods

    /*
    STDMETHODIMP AddedProp(const UINT32 ulId,
                           const HXPropType propType,
                           const UINT32 ulParentID);
    STDMETHODIMP ModifiedProp(const UINT32 ulId,
                              const HXPropType propType,
                              const UINT32 ulParentID);
    STDMETHODIMP DeletedProp(const UINT32 ulId,
                             const UINT32 ulParentID);
    */

    HX_RESULT Init(IUnknown* pContext);
    
    enum {NUM_AGGREGATE_RTSP_STATS = RTSPStats::MAX_RTSP_METHODS + 4};

    enum RTSPReqMethodType 
    { 
        CLIENT_REQUEST, 
        SERVER_REQUEST, 
        NUM_RTSP_REQUEST_TYPES
    };

    enum EventCounterType
    {
        REQUEST_TOTAL_COUNT,
        RESPONSE_SUCCESS_COUNT,
        RESPONSE_ERR4XX_COUNT,
        RESPONSE_ERR5XX_COUNT,
        NUM_COUNTERS
    };

    typedef struct _RTSPStatInfo
    {
        RTSPReqMethodType   ulMethodType;
        RTSPMethod          ulMethodId;
        const char*         szRegPropName;
    } RTSPStatInfo;

    typedef struct _EventCounterInfo
    {
        UINT32      ulRegId;
        INT32       nCounter;
        BOOL        bOverflow;
    } EventCounterInfo;
  

protected:

    // Protected Methods
    HX_RESULT GetResponseCounterId(UINT32 ulStatusCode, 
                                   UINT32& ulCounterId,
                                   BOOL& bIsRedirect);

    inline HX_RESULT UpdateCounter(INT32 lCount, 
                                   UINT32 ulCounterId,
                                   RTSPReqMethodType ulMethodType, 
                                   RTSPMethod ulMethodId);

    HX_RESULT ReadConfig();
    HX_RESULT SetupRegistryEntries();

    /*
    HX_RESULT GetEventId(const char* szEventName, 
                         UINT32& ulEventId);
    */
    //HX_RESULT ClearRegistryWatches();

    // Static members
    static RTSPStatInfo zm_StatInfoList[NUM_AGGREGATE_RTSP_STATS];
    static const char* zm_StatCounters[NUM_RTSP_REQUEST_TYPES][NUM_COUNTERS];

    // Instance members
    UINT32 m_ulRefCount;
    IHXRegistry2* m_pRegistry;
    IHXErrorMessages* m_pErrorLog;

    UINT32 m_ulEventMask;

    BOOL m_bLicensed;
    BOOL m_bLicenseChecked;

    BOOL m_bInitDone;
    BOOL m_bEnabled;

    //IHXPropWatch* m_pPropWatch;
    const char* m_szStatsRegRoot;

    BOOL m_bIsProxy;

    //UINT32 m_EventMaskRegIdVec[RTSPStats::MAX_RTSP_METHODS];
    EventCounterInfo m_EventCounterVec[NUM_RTSP_REQUEST_TYPES]
                                      [RTSPStats::MAX_RTSP_METHODS]
                                      [NUM_COUNTERS];
};


#endif  /* ifndef _RTSPSTATS_H */
