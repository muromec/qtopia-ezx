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
#ifndef _STREAM_FILTER_ASM_H_
#define _STREAM_FILTER_ASM_H_

#include "hxasm.h"
#include "ispifs.h"
#include "source.h"
#include "sink.h"

_INTERFACE IHXServerPacketSource;
_INTERFACE IHXServerPacketSink;
_INTERFACE IHXPSourceControl;
_INTERFACE IHXPSinkControl;
_INTERFACE IHXASMSource;
_INTERFACE IHXSessionStats;
_INTERFACE IHXQoSProfileConfigurator;
_INTERFACE IHXRateDescription;
_INTERFACE IHXRateDescEnumerator;
_INTERFACE IHXRateDescVerifier;
_INTERFACE IHXQoSApplicationAdaptationInfo;
_INTERFACE IHXQoSProfileConfigurator;

class Process;
class ASMRuleBook;
class CUberStreamManager;
class CStreamSelector;

/*
 * This class puts a pretty face
 */
class CASMStreamFilter : public IHXServerPacketSource
		       , public IHXServerPacketSink
		       , public IHXPSourceControl
		       , public IHXPSinkControl

		       , public IHXPSourcePackets
		       , public IHXPSinkPackets

		       , public IHXASMSource
		       , public IHXUberStreamManager
		       , public IHXUberStreamManagerConfig
{
public:
    CASMStreamFilter(Process* pProc, IHXSessionStats* pStats, IHXQoSProfileConfigurator* pQoSConfig);

    /*
     * MDP Support - IHXServerPacketSource and IHXServerPacketSink
     */
    /* IHXServerPacketSource */
    STDMETHOD(SetSink)		(THIS_ IHXServerPacketSink* pSink);
    STDMETHOD(StartPackets)	(THIS);
    STDMETHOD(GetPacket)	(THIS);
    STDMETHOD(SinkBlockCleared)	(THIS_ UINT32 ulStream);
    STDMETHOD(EnableTCPMode)    (THIS);

    /* IHXServerPacketSink */
    STDMETHOD(SetSource)	(THIS_ IHXServerPacketSource* pSource);
    STDMETHOD(PacketReady)	(THIS_ ServerPacket* pPacket);
    STDMETHOD(Flush)		(THIS);
    STDMETHOD(SourceDone)       (THIS);

    /*
     * Contorol Interfaces - IHXPSourceControl and IHXPSinkControl
     */
    /* IHXPSourceControl */
    STDMETHOD(Init)             (THIS_ IHXPSinkControl* pSink);
    STDMETHOD(Done)		(THIS);
    STDMETHOD(GetFileHeader)    (THIS_ IHXPSinkControl* pSink);
    STDMETHOD(GetStreamHeader)  (THIS_ IHXPSinkControl* pSink, UINT16 unStreamNumber);
    STDMETHOD(Seek)             (THIS_ UINT32 ulSeekTime);
    STDMETHOD_(BOOL,IsLive)     (THIS);
    STDMETHOD(SetLatencyParams) (THIS_
	UINT32 ulLatency, BOOL bStartAtTail, BOOL bStartAtHead);

    /* IHXPSinkControl methods */
    STDMETHOD(InitDone)		(THIS_ HX_RESULT ulStatus);
    STDMETHOD(FileHeaderReady)  (THIS_ HX_RESULT ulStatus, IHXValues* pHeader);
    STDMETHOD(StreamHeaderReady)(THIS_ HX_RESULT ulStatus, IHXValues* pHeader);
    STDMETHOD(StreamDone)	(THIS_ UINT16 unStreamNumber);
    STDMETHOD(SeekDone)		(THIS_ HX_RESULT ulStatus);


    /*
     * Legacy Mode  - IHXPSourcePackets and IHXPSinkPackets
     */
    /* IHXPSourcePackets */
    STDMETHOD(Init)             (THIS_ IHXPSinkPackets* pSink);
    STDMETHOD(GetPacket)        (THIS_ UINT16 unStreamNumber);

    /* IHXPSinkPackets */
    STDMETHOD(PacketReady)  	(THIS_ HX_RESULT ulStatus, IHXPacket* pPacket);


    /* IHXASMSource */
    STDMETHOD(Subscribe)	(THIS_ UINT16 uStreamNumber, UINT16 uRuleNumber);
    STDMETHOD(Unsubscribe)	(THIS_ UINT16 uStreamNumber, UINT16 uRuleNumber);
    
    /* IUnknown */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);

    // we need a way to set IHXPSourceControl...
    HX_RESULT SetControlSource(IHXPSourceControl* pCtrlSource);

    // IHXRateDescEnumerator methods
    STDMETHOD_(UINT32,GetNumRateDescriptions) (THIS);
    STDMETHOD(GetRateDescription) (THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc);
    STDMETHOD_(UINT32,GetNumSelectableRateDescriptions) (THIS);
    STDMETHOD(GetSelectableRateDescription)(THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc);
    STDMETHOD_(UINT32,GetNumSwitchableRateDescriptions) (THIS);
    STDMETHOD(GetSwitchableRateDescription)(THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc);
    
    STDMETHOD(FindRateDescByRule)(THIS_ UINT32 ulRuleNumber, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc);
    STDMETHOD(FindRateDescByExactAvgRate)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc);
    STDMETHOD(FindRateDescByClosestAvgRate)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc);
    STDMETHOD(FindRateDescByMidpoint)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc);

    // IHXUberStreamManager methods
    STDMETHOD_(UINT32,GetNumStreamGroups) (THIS);
    STDMETHOD(GetStreamGroup)(THIS_ UINT32 ulIndex, REF(IHXRateDescEnumerator*)pStreamGroupMgr);
    STDMETHOD(FindStreamGroupByLogicalStream)(THIS_ UINT32 ulLogicalStream, REF(UINT32)ulStreamGroup);
    STDMETHOD_(UINT32,GetNumLogicalStreams) (THIS);
    STDMETHOD(GetLogicalStream)(THIS_ UINT32 ulIndex, REF(IHXRateDescEnumerator*)pLogicalStreamEnum);
    STDMETHOD(GetSelectedLogicalStreamNum)(THIS_ UINT32 ulStreamGroupNum, REF(UINT32)ulSelectedLogicalStreamNum);

    STDMETHOD(SetAggregateRateDesc)(THIS_ IHXRateDescription* pRateDesc, IHXRateDescResponse* pResp);
    STDMETHOD(GetCurrentAggregateRateDesc)(THIS_ REF(IHXRateDescription*)pRateDesc);
    STDMETHOD(CommitInitialAggregateRateDesc) (THIS);
    STDMETHOD_(BOOL,IsInitalAggregateRateDescCommitted) (THIS);
    STDMETHOD(UpshiftAggregate)(THIS_ UINT32 ulRate, IHXRateDescResponse* pResp, BOOL bIsClientInitiated);
    STDMETHOD(DownshiftAggregate)(THIS_ UINT32 ulRate, IHXRateDescResponse* pResp, BOOL bIsClientInitiated);

    STDMETHOD(SelectLogicalStream)(THIS_ UINT32 ulStreamGroupNum,
                                         UINT32 ulLogicalStreamNum);
    STDMETHOD(SetStreamGroupRateDesc)(THIS_ UINT32 ulStreamGroupNum, 
                                        UINT32 ulLogicalStreamNum, 
                                        IHXRateDescription* pRateDesc, 
                                        IHXStreamRateDescResponse* pResp);

    STDMETHOD(GetCurrentStreamGroupRateDesc)(THIS_ UINT32 ulStreamGroupNum, REF(IHXRateDescription*)pRateDesc);
    STDMETHOD(CommitInitialStreamGroupRateDesc) (THIS_ UINT32 ulStreamGroupNum);
    STDMETHOD_(BOOL,IsInitalStreamGroupRateDescCommitted) (THIS_ UINT32 ulStreamGroupNum);
    STDMETHOD(GetNextSwitchableRateDesc)(THIS_ REF(IHXRateDescription*)pRateDesc);
    STDMETHOD(GetNextSwitchableStreamGroupRateDesc)(THIS_ UINT32 ulStreamGroupNum, REF(IHXRateDescription*)pRateDesc);

    STDMETHOD(UpshiftStreamGroup)   (THIS_ UINT32 ulStreamGroupNum, 
                                    UINT32 ulRate, 
                                    IHXStreamRateDescResponse* pResp,
                                    BOOL bIsClientInitiated);
    STDMETHOD(DownshiftStreamGroup) (THIS_ UINT32 ulStreamGroupNum, 
                                    UINT32 ulRate, 
                                    IHXStreamRateDescResponse* pResp,
                                    BOOL bIsClientInitiated);

    STDMETHOD(SubscribeLogicalStreamRule)   (THIS_ UINT32 ulLogicalStreamNum, 
                                            UINT32 ulRuleNum, 
                                            IHXStreamRateDescResponse* pResp);
    STDMETHOD(UnsubscribeLogicalStreamRule)(THIS_ UINT32 ulLogicalStreamNum, 
                                            UINT32 ulRuleNum, 
                                            IHXStreamRateDescResponse* pResp);
    STDMETHOD(GetLowestAvgRate)(THIS_ UINT32 ulStreamGroupNum, REF(UINT32) ulLowestAvgRate);
    STDMETHOD(SetDownshiftOnFeedbackTimeoutFlag)(THIS_ BOOL bFlag);

    // IHXUberStreamManagerConfig methods
    STDMETHOD(SetRateSelectionInfoObject) (THIS_ IHXRateSelectionInfo* pRateSelInfo);

    void SetInitial(UINT32 type, UINT32 ulVal);
    
protected:    
    ~CASMStreamFilter();

    void    _Done(void);
    HX_RESULT HandleFileHeader(HX_RESULT status, IHXValues* pHdr);
    HX_RESULT HandleStreamHeader(HX_RESULT status, IHXValues* pHdr);
    HX_RESULT CopyHeaders(IHXValues* pOldVal, REF(IHXValues*) pNewVal);
    HX_RESULT ModifyStreamHeaders(IHXValues* pHdr);

    HX_RESULT CommitSubscription();

    /* initial external subscription support */    
    HX_RESULT CommitExternalSubscription(void);


    HX_RESULT InitASMRuleHandler(IHXSyncHeaderSource* pHdrSrc);
    HX_RESULT DoInit(HX_RESULT ulStatus);

    STDMETHOD(DetermineSelectableStreams)(THIS_ const StreamAdaptationParams* pStreamAdaptationParams = NULL);
    HX_RESULT PrepareExternalSubscription(void);
    
    void    ShiftResponse(HX_RESULT status);
    void    ShiftResponse(HX_RESULT status, UINT32 ulStreamGroup);
    void    UpdateStats(IHXRateDescription* pRateDesc);

    BOOL    IsLegacyMode(void) {return m_pPullPacketSink || m_pPullPacketSource;}

    void    DumpSelection(HX_RESULT theErr);
    HX_RESULT UnsubscribeAllOnDone(void);
    
protected:
    UINT32			m_ulRefCount;
    Process*			m_pProc;

    /* MDP Support */
    IHXServerPacketSink*	m_pPacketSink;
    IHXServerPacketSource*	m_pPacketSource;

    /* PPM Support */
    IHXPSinkPackets*		m_pPullPacketSink;
    IHXPSourcePackets*		m_pPullPacketSource;

    IHXPSinkControl*		m_pCtrlSink;
    IHXPSourceControl*		m_pCtrlSource;

    IHXASMSource*		m_pASMSource;
    IHXSessionStats*		m_pStats;
    IHXQoSProfileConfigurator*  m_pQoSConfig;

    enum
    {
        ASM_FILTER_ST_NONE,
        ASM_FILTER_ST_UP,
        ASM_FILTER_ST_DOWN
    }                           m_shiftType;

    IHXQoSApplicationAdaptationInfo* m_pQoSStats;

    
    CUberStreamManager*         m_pUberMgr;
    IHXRateDescResponse*        m_pRateDescResp;

    CStreamSelector*		m_pStreamSelector;
        
    UINT32                      m_bRateCommitted; // TRUE if stream selection has been committed
    IHXRateSelectionInfo*       m_pRateSelInfo;

    enum
    {
	ASM_FILTER_IS_NONE,		    
	ASM_FILTER_IS_CONTROL_INITED,		// IHX*Control Interface is Inited	
	ASM_FILTER_IS_RATE_SELECTION_INITED	// Rate Selection Objects Inited
    }				m_initState;

    /*
     * These two flags represents two types of stream selection that can be performed
     * during initialization.
     *  m_bAllowExternalStreamSelection: for microclient that issues ASM subscription
     *   at the startup and locks into the codec that was chosen.  We need to lock 
     *   audio stream to the subscribed bitrate to avoid potential codec switching.
     *  m_bAllowInternalStreamSelection: enables stream selection (verification) that
     *   uses Profile/QoSConfig which marks certain bitrates as non selectable/switchable    
     */ 
    BOOL			m_bAllowExternalStreamSelection;		
    BOOL			m_bAllowInternalStreamSelection;

    // debug
    UINT32			m_ulShiftAfter;
    UINT32			m_ulPktCount;
    INT32                       m_lDebugOutput;

};

#endif /* _STREAM_FILTER_ASM_H_ */
