/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: bcastmgr.h,v 1.44 2009/04/21 18:52:45 ckarusala Exp $ 
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
/*
 *  Broadcast Management & Distribution
 */

#ifndef _BCASTMGR_H_
#define _BCASTMGR_H_

#include "hxasm.h"
#include "hxformt.h"
#include "simple_callback.h"
#include "base_callback.h"
#include "source.h"
#include "sink.h"
#include "servlist.h"
#include "mutex.h"
#include "tconverter.h"
#include "ispifs.h"

/* These defines are for m_pStreamDoneTable[x] in the BroadcastGateway */
#define BCAST_STREAMDONE_TABLE_OK		0xfffffff0
#define BCAST_STREAMDONE_TABLE_DONE		0xfffffff1

// The queue can hold up to 10 sec worth of packets or 4000 (the max
// array size).
#define BCAST_MAX_QUEUE_SEC 10
#define BCAST_MAX_QUEUE_USEC 0

#define BCAST_QUEUE_HIWAT_SEC 7
#define BCAST_QUEUE_LOWAT_SEC 4

// The exact size is arbitrary, 4k is large enough but we set it just under
// cuz the queue is now alloc'ed up front and we want it to fit in a 64k chunk
#define BCAST_QUEUE_MAX_SIZE 4000

class CHXMapStringToOb;
class BroadcastStreamer;
class BroadcastPacketManager;
class BroadcastStreamQueue;
struct BroadcastPacketListEntry;
class PacketSignalCallback;
class Process;
class SwitchGroupRSDInfo;
class AudioSwitchGroupInfo;
_INTERFACE IHXSessionStats;
struct IHXBroadcastFormatObject;
class BroadcastPacketFilter;

struct RuleData
{
    RuleData (UINT16 unNumRules)
	{
	    m_unNumRules = unNumRules;
	    m_pActiveRules = new UINT16 [m_unNumRules];
	    memset(m_pActiveRules, 0, sizeof(UINT16) * m_unNumRules);
	}
    ~RuleData()
	{
	    HX_VECTOR_DELETE(m_pActiveRules);
	}

    UINT16* m_pActiveRules;
    UINT16  m_unNumRules;
};

class BroadcastInfo
{
public:
    BroadcastInfo(Process* pProc);
    ~BroadcastInfo();
    UINT32		m_ulProcnum;
    CHXMapStringToOb*	m_pCurrentBroadcasts;
    const char*		m_pType;
    /* This is the Live host's Process */
    Process*		m_pProc;

    /* synchs access to m_pCurrentBroadcasts */
    HX_MUTEX            m_GatewayLock;
};

class CPacketBufferQueue: public IUnknown
{
public:

    CPacketBufferQueue(UINT32 ulQueueSize, BOOL bQTrace, UINT32 ulReservedForAudioPackets);
    ~CPacketBufferQueue();

    //IUnknown
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)          (THIS);

    STDMETHOD_(ULONG32,Release)         (THIS);


    //IHXLivePacketBufferQueue
    STDMETHOD(GetPacket)                (THIS_
                                        UINT32 ulIndex,
                                        IHXPacket*& pPacket);
    STDMETHOD_(ULONG32,GetSize)         (THIS);

    HX_RESULT EnQueue(IHXPacket*  pPacket);
    HX_RESULT AddKeyframe(IHXPacket*  pPacket);
    UINT32 GetStartTime() { return m_ulStartTS; }
    UINT32 GetDuration();
    void AddAudioPacket(IHXPacket* pPacket);
    IHXPacket* GetKeyFramePacket();
    inline BOOL HandlePacketInvalidTimeStamp() { return (m_ulInvalidPacketCount++ > 3 ? TRUE : FALSE); };    
    inline void ResetPacketInvalidTimeStamp() { m_ulInvalidPacketCount = 0; };

//    HX_RESULT Update();

#if 0    
    HX_RESULT FindNextKeyframePackets(UINT32 ulIndex, UINT32& ulHead);
#endif

    void Dump(void);
private:
    IHXPacket** m_PacketBufferQueue;
    UINT32      m_ulQueueSize;
    UINT32      m_ulInsertPosition;
    UINT32      m_ulQueueBytes;
    UINT32      m_ulRefCount;
    UINT32      m_ulStartTS;
    BOOL        m_bTrace;
    UINT32      m_ulInvalidPacketCount;
    IHXPacket*  m_pKeyFrame;
    UINT32      m_ulStartingPosition;
};

class BroadcastGateway : public IHXFormatResponse
{
public:

    enum StateTable
    { 
    INIT,
    STREAMING,
    DONE
    };

    BroadcastGateway(BroadcastInfo* pInfo, const char* pFilename, Process* pStreamerProc);

    /* IUnknown methods */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)          (THIS);

    STDMETHOD_(ULONG32,Release)         (THIS);

    /* IHXFormatResponse methods */
    STDMETHOD(InitDone)                 (THIS_
                                        HX_RESULT       status);

    STDMETHOD(FileHeaderReady)          (THIS_
                                        HX_RESULT       status,
                                        IHXValues*     pHeader);

    STDMETHOD(StreamHeaderReady)        (THIS_
                                        HX_RESULT       status,
                                        IHXValues*     pHeader);

    STDMETHOD(PacketReady)              (THIS_
                                        HX_RESULT       status,
                                        IHXPacket*     pPacket);

    STDMETHOD(StreamDone)               (THIS_
                                        UINT16          unStreamNumber);

    STDMETHOD(SeekDone)                 (THIS_
                                        HX_RESULT       status);

    void AddBroadcastStreamer(Process* pProc, BroadcastStreamer* pStreamer);
    void RemoveBroadcastStreamer(Process* pProc, BroadcastStreamer* pStreamer);

    IHXValues* GetFileHeader();
    IHXValues* GetStreamHeaders(UINT32 ulStreamNumber);
    HX_RESULT Done();
    void LatencyCalc(UINT32 msDiff);
    INT32 GetRegEntryIndex()	{ return m_nRegEntryIndex; }

    IHXLivePacketBufferQueue* GetPacketBufferQueue(void); 
    IHXLivePacketBufferQueue* GetLongPacketBufferQueue(void); 
    HX_RESULT HandlePacketBufferQueue(IHXPacket* pPacket,
                                      UINT32 uStreamNumber,
                                      UINT16 unRule);

    StateTable          m_State;
    IHXValues*          m_pFileHeader;
    IHXValues**         m_pStreamHeaders;
    IHXValues**         m_ppPerStreamerFileHeaders;
    IHXValues***        m_pppPerStreamerStreamHeaders;
    UINT32              m_ulStreamCount;
    UINT32              m_ulStreamGroupCount;
    UINT32              m_ulSwitchGroupCount;
    UINT32*             m_aulLogicalStreamToStreamGroup;
    UINT32*             m_aulLogicalStreamToSwitchGroup;
    UINT32              m_unKeyframeStreamGroup;

    UINT32              m_ulPlayerCount;
    UINT32*             m_pStreamDoneTable;

    /* manage data flow on a per streamer basis */
    BroadcastPacketManager*    m_pBroadcastPacketManagers [MAX_THREADS];
    UINT16                     m_unNumStreamers;
    UINT16                     m_pStreamers [MAX_THREADS];
    Timeval                    m_tMaxQueueOccupancy;
    Timeval                    m_tLastPacketArrival;
#ifdef DEBUG_IAT
    float                      m_fAvgInterArrivalTime;
#endif
    float                      m_fx;
    float                      m_fy;
    float                      m_fz;

    /* LatencyRequirements set by broadcast format object */ 
    UINT32                      m_ulRequestedBackOff;
    BOOL                        m_bUsePreBuffer;
    BOOL                        m_bUseLatencyRequirements;

    /* ASM Rule Tracking */
    RuleData**                  m_ppRuleData;
    
    /* synchs access to: m_State */
    HX_MUTEX                    m_StateLock;

    /* synchs access to: active streamer map */
    HX_MUTEX                    m_BroadcastPacketManagerLock;
    /* synchs access to: packet arrival timer */
#ifdef DEBUG_IAT
    HX_MUTEX                    m_InterArrivalTimeLock;
#endif

    /* synchs access to: m_ppRuleData */
    HX_MUTEX                    m_RuleDataLock;

    HX_MUTEX                    m_pPerStreamerFileHeaderLock;
    HX_MUTEX                    m_pPerStreamerStreamHeaderLock;

    BroadcastInfo*              m_pInfo;

    IHXASMSource*               m_pASMSource;
    HXBOOL                      m_bIsSubscribed;

protected:
    virtual ~BroadcastGateway();

    INT32                       m_lRefCount;
    IHXBroadcastFormatObject*   m_pBCastObj;
    char*                       m_pFilename;
    UINT32                      m_ulStreamHeadersSeen;
    UINT32                      m_ulStreamDonesSeen;
    INT32                       m_nRegEntryIndex;
    BOOL                        m_bLowLatency;

    BOOL                        m_bDisableLiveTurboPlay;
    CPacketBufferQueue*        m_pPacketBufferQueue;
    CPacketBufferQueue*        m_pLongPacketBufferQueue;
    CPacketBufferQueue*        m_pFuturePacketBufferQueue;
    BOOL                       m_bQTrace;
    BOOL                        m_bEnableRSDDebug;
    BOOL                        m_bEnableRSDPerPacketLog;
    INT32                       m_lQueueSize;
    INT32                       m_lQueueDuration;
    INT32                       m_lMinPreroll;
    INT32                       m_lExtraPrerollPercentage;
    BOOL                        m_bQSizeTooSmallReported;
    HX_MUTEX                    m_PacketBufferQueueLock;
    HX_MUTEX                    m_LongPacketBufferQueueLock;

    UINT32                      m_ulGwayLatencyTotal;
    UINT32                      m_ulGwayLatencyReps;
    UINT32                      m_ulMaxGwayLatency;
    UINT32                      m_ulAvgGwayLatencyRegID;
    UINT32                      m_ulMaxGwayLatencyRegID;

    BOOL                        m_bSureStreamAware;
    UINT32                      m_lMaxDurationOfPacketBufferQueue;
    SwitchGroupRSDInfo**        m_ppSwitchGroupRSDInfo;
    UINT32*                     m_aulTmpSwitchGroupMap;
    UINT32                      m_ulAudioStreamGroup;
    AudioSwitchGroupInfo**      m_ppAudioSwitchGroupInfo;

private:
    void Init(Process* pProc);
    HX_RESULT FixupFileHeader(IHXValues* pFileHeader);
    HX_RESULT FixupStreamHeader(UINT32 ulLogicalStreamNum, IHXValues* pStreamHeader);
    BOOL IsKeyFrameStream(UINT32 ulStreamNumber);
    BOOL IsKeyFrameRule(UINT32 ulStreamNumber, UINT16 unRule);
    BOOL IsMinRSDQueueDuration(IHXPacket* pPacket);
    void ResetSwitchGroupRSDInfo();
    BOOL HasAllAudioPackets();
    UINT32 GetNumOfReservedAudioPackets();
    void AddPacketToQueue(CPacketBufferQueue*& pQueue,
            IHXPacket* pPacket,
            BOOL& bIncQSize,
            HX_MUTEX queueLock);

    class InitCallback : public SimpleCallback
    {
    public:
    void                func(Process* proc);
    BroadcastGateway*   m_pGateway;
    private:
    virtual             ~InitCallback() {};

    };
    friend class InitCallback;

    class DestructCallback : public BaseCallback                      
    {
    public:                                                           
        STDMETHOD(Func) (THIS);                                          
        BroadcastGateway*    m_pBG;
    };
    friend class DestructCallback;

    class IdleStopCallback : public BaseCallback                      
    {
    public:                                                           
        STDMETHOD(Func) (THIS);                                          
        BroadcastGateway*    m_pBG;
    };
    friend class IdleStopCallback;

    class LatencyCalcCallback : public BaseCallback                      
    {
    public:                                                           
        STDMETHOD(Func) (THIS);                                          
        BroadcastGateway*    m_pBG;
    };
    friend class LatencyCalcCallback;

    friend class ASMUpdateCallback;

    DestructCallback*    m_pDestructCallback;
    IdleStopCallback*    m_pIdleStopCallback;
    LatencyCalcCallback* m_pLatencyCalcCallback;
    UINT32               m_ulIdleStopCBHandle;
    UINT32               m_ulDestructCBHandle;
    UINT32               m_ulLatencyCalcCBHandle;
};


inline void
BroadcastGateway::LatencyCalc(UINT32 msDiff)
{
    if (msDiff > m_ulMaxGwayLatency)
    {
        m_ulMaxGwayLatency = msDiff;
    }

    m_ulGwayLatencyTotal += msDiff;
    m_ulGwayLatencyReps++;
}

class BroadcastStreamer : public IHXPSourceControl,
                          public IHXASMSource,
                          public IHXLivePacketBufferProvider,
                          public HXListElem,
                          public IHXPSourceLivePackets,
                          public IHXPSourceLiveResync,
                          public IHXServerPacketSource,
                          public IHXSyncHeaderSource

{
public:
    BroadcastStreamer(BroadcastInfo* pInfo, const char* pFilename,
                      Process* pStreamerProc, IHXSessionStats* pSessionStats);

    /* IUnknown */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /* IHXPSourceControl */
    STDMETHOD(Init)     	(THIS_
				IHXPSinkControl*		pSink);

    STDMETHOD(Done)		(THIS);

    STDMETHOD(GetFileHeader)	(THIS_
				IHXPSinkControl*		pSink);

    STDMETHOD(GetStreamHeader)	(THIS_
				IHXPSinkControl*		pSink,
				UINT16 unStreamNumber);

    STDMETHOD(Seek)		(THIS_
				UINT32		ulSeekTime);

    STDMETHOD_(BOOL,IsLive)	(THIS);

    STDMETHOD(SetLatencyParams) (THIS_
				 UINT32 ulLatency,
				 BOOL bStartAtTail,
				 BOOL bStartAtHead);


    /* IHXSMSource */
    STDMETHOD(Subscribe)   (THIS_ UINT16  uStreamNumber, UINT16 uRuleNumber);
    STDMETHOD(Unsubscribe)  (THIS_ UINT16 uStreamNumber, UINT16	uRuleNumber);

    /* IHXLivePacketBufferProvider */
    STDMETHOD(GetPacketBufferQueue)    (THIS_
                                        IHXLivePacketBufferQueue*& pQueue);
    STDMETHOD(GetLongPacketBufferQueue) (THIS_
                                         IHXLivePacketBufferQueue*& pQueue);

    /* IHXPSourceLivePackets */
    STDMETHOD(Init)           (THIS_ IHXPSinkPackets* pSinkPackets);
    STDMETHOD(StartPackets)   (THIS_ UINT16 unStreamNumber);
    STDMETHOD(StopPackets)    (THIS_ UINT16 unStreamNumber);
   
    /* IHXPSourceLiveResync */
    STDMETHOD(Resync)		(THIS);

    /* IHXServerPacketSource */
    STDMETHOD(SetSink)          (THIS_ IHXServerPacketSink* pSink);
    STDMETHOD(StartPackets)     (THIS);
    STDMETHOD(GetPacket)        (THIS);
    STDMETHOD(SinkBlockCleared) (THIS_ UINT32 ulStream);
    STDMETHOD(EnableTCPMode)    (THIS);

     /* IHXSyncHeaderSource */
    STDMETHOD(GetFileHeader)	(THIS_ REF(IHXValues*)pHeader);
    STDMETHOD(GetStreamHeader)	(THIS_ UINT32 ulStreamNo, REF(IHXValues*)pHeader);

    
private:
    ~BroadcastStreamer();

    void      GatewayCheck();
    HX_RESULT SendPacket(IHXPacket* pPacket);
    HX_RESULT PushPacketsFromQueue(void);

    HX_RESULT CreateRegEntries();
    HX_RESULT UpdateRegClientsLeaving();

    class GatewayCheckCallback : public BaseCallback                      
    {
    public:                                                           
        STDMETHOD(Func) (THIS);                                          
        BroadcastStreamer*    m_pBS;
    };
    friend class GatewayCheckCallback;

    class StreamDoneCallback : public BaseCallback                      
    {
    public:                                                           
        STDMETHOD(Func) (THIS);                                          
        BroadcastStreamer*    m_pBS;
    };
    friend class StreamDoneCallback;

    INT32		    m_lRefCount;
    /* The streamer's proc */
    Process*		    m_pProc;
    IHXPSinkControl*	    m_pSinkControl;
    BroadcastInfo*	    m_pInfo;
    BroadcastGateway*	    m_pGateway;
    BOOL		    m_bGatewayReady;
    HX_RESULT		    m_GatewayStatus;
    UINT32		    m_ulGatewayCheckCBHandle;
    GatewayCheckCallback*   m_pGatewayCheckCallback;
    UINT32		    m_ulStreamDoneCBHandle;
    StreamDoneCallback*     m_pStreamDoneCallback;
    BOOL		    m_pInitPending;
    BOOL		    m_bSourceAborted;
    INT32		    m_nRegEntryIndex;
    UINT8*                  m_pbPacketsStarted;
    // Broadcast dist sender and QT DESCRIBE sesssions won't have this.
    IHXSessionStats*        m_pSessionStats;
    BOOL                    m_bNeedXmit;
    
    // PPM sink
    IHXPSinkPackets*	    m_pSinkPackets;
    
    //MDP sink
    IHXServerPacketSink*    m_pServerPacketSink;
    BroadcastPacketFilter*      m_pPacketFilter;
    BOOL                    m_bIsMDPSink;
    friend class BroadcastPacketManager;
};

class BroadcastManager
{
public:
	    	    BroadcastManager();
	    	    ~BroadcastManager();

    /* Register must be called from a valid BroadcastPlugin process space */
    void	    Register(IUnknown* pPluginInstance, UINT32 ulProcnum,
			    Process* pLiveProc);

    HX_RESULT	    GetStream(const char* pType, const char* pFilename,
			      REF(IHXPSourceControl*) pControl,
			      Process* pStreamerProc, IHXSessionStats* pSessionStats=NULL);
private:
    CHXMapStringToOb*	m_pBroadcastManagers;
    Process*		m_pProc;
};

/* one per streamer process and one per packeet sink process */
class BroadcastPacketManager : public IHXCallback
{
 public:
    BroadcastPacketManager (Process* pProc, BroadcastGateway* pGateway);
			    
    ~BroadcastPacketManager();

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);
    
    STDMETHOD_(ULONG32,Release) (THIS);

    STDMETHOD(Func) (THIS); 

    void Init(Process* pProc);
    void Done();
    void QueuePacket(IHXPacket* pPacket);
    void SendStreamDone(UINT16 unStreamNumber);
    void AddBroadcastStreamer(BroadcastStreamer* pStreamer);
    void RemoveBroadcastStreamer(BroadcastStreamer* pStreamer);
    HX_RESULT DispatchPacket(UINT32 uMaxSinkSends, REF(UINT32)uTotalSinkSends);

    class JumpStartCallback : public SimpleCallback
    {
	public:
	    void                      func(Process* proc);
	    BroadcastPacketManager*   m_pPacketManager;
	private:
	    virtual		    ~JumpStartCallback() {};
	    
    };

    void SendDone(Process* pProc);
    class DestructCallback : public SimpleCallback
    {
	public:
	    void                      func(Process* proc);
	    BroadcastPacketManager*   m_pPacketManager;
	    DestructCallback(BroadcastPacketManager* pManager);
    };
    friend class JumpStartCallback;

 private:

    Process*              m_pProc;
    BroadcastGateway*     m_pGateway; 

    BroadcastStreamQueue* m_pStreamQueue;
    HXList                m_BroadcastStreamerList;

    UINT8                 m_bCleanedUp;
    UINT32                m_lRefCount;
    UINT32		  m_ulCallbackHandle;

    Timeval       m_tLastDepartureTime;
    Timeval       m_tNextSchedDepartureTime;
#ifdef DEBUG_PKTMGR
    char          m_pszDebug[256];
    UINT32        m_ulCount;
#endif
    UINT32        m_uNumCallBacks;
    float         m_fAvgPktsPerCallback;
    float         m_fFilter;

    UINT32        m_uMaxPackets;
    UINT32        m_uTotalSinkSends;

    IHXPacket*    m_pCurrentPacket;
    friend class BroadcastStreamQueue;
};

struct BroadcastPacketListEntry
{
    IHXPacket*    m_pPacket;
    Timeval	  m_tTime;
    UINT8	  m_bIsStreamDoneMarker;
    UINT16        m_unStreamNumberDone;
};


class BroadcastStreamQueue
{
 public:
    BroadcastStreamQueue();
    ~BroadcastStreamQueue();

    void        Init(BroadcastPacketManager* pParent);
    HX_RESULT   Enqueue(IHXPacket* pPacket);
    IHXPacket*  Dequeue(Timeval& tHead, UINT8& bIsStreamDoneMarker,
                        UINT16& unStreamNumberDone);
    void        TerminateQueue(UINT16 unStreamNumber);

    BOOL        Empty();
    float       QueueDepthInSeconds();
    UINT32      QueueDepth();
 private:
    BroadcastPacketManager* m_pParent;
    BroadcastPacketListEntry* m_pQueue;

    UINT32                  m_uHead;
    UINT32                  m_uTail;
    UINT32                  m_uMaxSize;
    UINT32                  m_uReaders;
    UINT32                  m_uWriters;
};


inline UINT32
BroadcastStreamQueue::QueueDepth()
{
    UINT32 uHead = m_uHead;
    UINT32 uTail = m_uTail;

    UINT32 uDepth = uTail - uHead;

    if (uDepth > m_uMaxSize)
    {
        uDepth = m_uMaxSize - uHead + uTail;
    }

    return uDepth;
}


enum ASMAction
{
    ASM_SUBSCRIBE,
    ASM_UNSUBSCRIBE
};

class ASMUpdateCallback : public SimpleCallback
{
 public:
    void                func(Process* proc);
    IHXASMSource*      m_pASMSource;
    
    enum ASMAction  m_cAction;
    UINT32          m_ulRuleQuantity;
    UINT16          m_unRuleNumber;
    UINT16          m_unStreamNumber;
 private:
    virtual		    ~ASMUpdateCallback() {};
    
};

class SwitchGroupRSDInfo
{
public:
    SwitchGroupRSDInfo(UINT16 unNumRules)
    {
        m_unNumRules = unNumRules;

        m_pbIsKeyFrameRule = new BOOL[m_unNumRules];
        m_pbKeyFrameReceived = new BOOL[m_unNumRules];
        m_plInterDependent = new INT32[m_unNumRules];

        for (int i=0; i<m_unNumRules; i++)
        {
            m_pbIsKeyFrameRule[i] = FALSE;
            m_pbKeyFrameReceived[i] = FALSE;
            m_plInterDependent[i] = -1;
        }

        m_ulLastKeyFrameTS = 0;
        m_bAllKeyFramesReceived = FALSE;
    }

    ~SwitchGroupRSDInfo()
    {
        HX_VECTOR_DELETE(m_pbIsKeyFrameRule);
        HX_VECTOR_DELETE(m_pbKeyFrameReceived);
        HX_VECTOR_DELETE(m_plInterDependent);
    }

    void SetKeyFrameRule(UINT16 ulRuleNum, BOOL bIsKeyFrameRule);
    void SetInterDependency(UINT16 ulRuleNum, INT16 lInterDepend);
    void OnKeyFramePacket(IHXPacket* pPacket, UINT16 unRule);
    BOOL IsPrerollSatisfied(IHXPacket* pPacket, INT32 lPreroll);
    void ResetInfo(void);
    BOOL IsKeyFrameRule(UINT16 unRule);

private:
    UINT16      m_unNumRules;
    BOOL*       m_pbIsKeyFrameRule;
    INT32*      m_plInterDependent;
    BOOL*       m_pbKeyFrameReceived;
    UINT32      m_ulLastKeyFrameTS;
    BOOL        m_bAllKeyFramesReceived;
};

class AudioSwitchGroupInfo
{
public:
    AudioSwitchGroupInfo(UINT32 ulNumofRules);
    ~AudioSwitchGroupInfo();

    void SetVideoKeyframePacket(IHXPacket* pVideoKeyFrame);
    void SetCurrentRSDQueue(CPacketBufferQueue* pQueue);
    void GetAudioPackets(IHXPacket**& pPackets, UINT32& ulNumOfPackets);
    void Reset();
    void OnPacket(IHXPacket* pPacket);
    void SetOnDepend(UINT16 unRuleNum, BOOL bOnDepend);
    void SetInterDepend(UINT16 ulRuleNum, UINT16 lInterDepend);
    BOOL HasAllAudioPackets();
    UINT32 GetNumOfPacket();

private:
    UINT32 m_ulNumOfRules;
    IHXPacket** m_ppPackets;
    IHXPacket* m_pVideoKeyframe;
    BOOL* m_pOnDepend;
    UINT16* m_pInterDepend;

    BOOL IsEarlier(IHXPacket* pPacket, IHXPacket* pPacketCompared);
};

#endif /* _BCASTMGR_H_ */

