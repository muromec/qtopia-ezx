/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsm.h,v 1.20 2007/07/06 21:58:16 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#ifndef _HXSM_H_
#define _HXSM_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxsmbw.h"
#include "hxslist.h"
#include "hxbsrc.h"
#include "hxmovmed.h"
#include "hxnetifsinkhlpr.h"

class CHXSimpleList;
class ASMRuleBook;
struct IHXSourceBandwidthInfo;
struct IHXErrorMessages;

class ASMStreamInfo;
class ASMSourceInfo;
class HXSM;
struct IHXValues;

#define NUM_REPORTS_NEEDED_TO_UPSHIFT 4
#define BW_DETECTION_DATA_POINTS 1024
#define TCP_BANDWIDTH_WINDOW 20000

class ASMSourceInfo : public IHXBandwidthManagerInput
{
public:
    UINT32	m_ulLastReportTime;
    UINT32	m_ulIncomingBandwidth;
    UINT32	m_ulRateBeforeDeAccel;
    INT32	m_lTimeDiffBase;  //this must be signed !!!
    UINT32	m_ulBytesBehind;
    INT32	m_lLastBehindTime;
    UINT32	m_ulLastSetDelivery;
    UINT32	m_ulMaxSubscribedBw;
    UINT32	m_ulMasterOffer;
    HX_BITFIELD	m_bBehind : 1;
    HX_BITFIELD	m_bLossBehind : 1;
    HX_BITFIELD	m_bSlightlyBehind : 1;
    HX_BITFIELD	m_bTimeStampDelivery : 1;
    HX_BITFIELD m_bPendingChill : 1;
    HX_BITFIELD m_bInvalidUpReport : 1;
    HX_BITFIELD	m_bPerfectPlay : 1;
    HX_BITFIELD m_bIsDone : 1;
    HX_BITFIELD m_bMayBeAccelerated : 1;
    HX_BITFIELD m_bTryToUpShift : 1;
    HX_BITFIELD	m_bAdjustBandwidth : 1;
    UINT32	m_ulLowestBandwidthBeforeTimeStamp;

    /*
     * The following are switches used in load testing only to control
     * whether or not any fall forward or fall back rate negotiation
     * goes on. There is a master switch, m_bDisableBothAccelDecel,
     * in the HXSM class that effects both of these swiches, globaly,
     * for ALL sources regardless of URL option switches.
     * 
     * Option switches are: "DoAccel", "DoDecel"
     *
     * NOTE: DoAccel and DoDecel control both Accelleration/Deceleration
     *       and upshifting and downshifting of SureStream sources.
     */
    HX_BITFIELD m_bSourceAccelAllowed : 1;
    HX_BITFIELD m_bSourceDecelAllowed : 1;

private:
    HX_BITFIELD m_bSlidingBwWindowReady : 1;

public:
    ASMRuleBook*    m_pMasterRuleBook;
    ASMStreamInfo** m_pStreams;
    CHXSimpleList   m_SubscriptionChanges;

    UINT32  GetBandwidthSince(UINT32 ulTime, UINT32 ulNow);
    UINT32  GetBandwidth() {return m_ulIncomingBandwidth;}

    TRANSPORT_TYPE			m_TransportType;

    ASMSourceInfo(HXSource* pSource, HXSM* pHXASM,
                  IUnknown* pUnknown);
    ~ASMSourceInfo();
    void Done();
    STDMETHOD (QueryInterface ) (THIS_ REFIID ID, void** ppInterfaceObj);
    STDMETHOD_(UINT32, AddRef ) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);

    /*
     *	IHXBandwidthManagerInput methods
     */
    STDMETHOD(ReportDataPacket)		(THIS_
					UINT32 ulTimeStamp,
					UINT32 ulArrivedTimeStamp,
					UINT32 ulSize);

    STDMETHOD(ReportUpshiftInfo)	(THIS_
					UINT32 ulTimeStamp,
					UINT32 ulSize);

    STDMETHOD(ReportLatency)		(THIS_
					UINT32 ulServerTime,
					UINT32 ulClientTime);

    STDMETHOD(SetCongestionFactor)	(THIS_
					UINT32 ulFactor);

    STDMETHOD(SetTransportType)		(THIS_
					TRANSPORT_TYPE type);


    HXSource*				m_pSource;
    IHXSourceBandwidthInfo*		m_pSBI;

    /* If the source has enough data, it may tell the bandwidth
     * manager to cut down on accelerated buffering.
     */
    void    ChangeAccelerationStatus	(HXBOOL      bMayBeAccelerated,
					 HXBOOL	   bUseAccelerationFactor,
					 UINT32	   ulAccelerationFactor);

    HX_RESULT AddStream(UINT16 i,
                        ASMStreamInfo* pInfo,
                        UINT32 ulLowestBwBeforeTS);

    void RegisterSourcesDone();

    HXBOOL HasSource(HXSource* pSource) const { return (m_pSource == pSource);}
    HXBOOL HasMasterRuleBook() const { return (m_pMasterRuleBook != 0);}
    HXBOOL IsFastStart() const { return m_pSource ? m_pSource->m_bFastStart : FALSE;}
    HXBOOL IsPerfectPlay() const {return m_bPerfectPlay;}
    HXBOOL IsLive() const { return (m_pSource ? m_pSource->IsLive() : FALSE);}
    HXBOOL IsBehind() const { return m_bBehind;}
    HXBOOL IsSlightlyBehind() const { return m_bSlightlyBehind;}
    

    UINT16 GetStreamCount() const { return m_pSource ? m_pSource->GetStreamCount() : 0;}
    UINT32 GetSubscribedBw() const {return m_ulSubscribedBw;}
    UINT32 GetMaxSubscribedBw() const {return m_ulMaxSubscribedBw;}

    void ClearSubscribedBw();
    void AddStreamSubBw(UINT32 ulCurBw, UINT32 ulCurMaxBw);
    void ChangeBW(UINT32 newBW);
    void SetMasterRuleBookOffer();
    void RecalcInit();
    void AddToMasterOffer(UINT32 ulOffer);
    void SetRuleGatherList(IHXAtomicRuleGather* pRuleGather);
    void FlushSubscriptions();
    void SetTSDelivery(HXBOOL bTSDelivery) { m_bTimeStampDelivery = bTSDelivery;}
    void GetSrcStats(REF(HXBOOL) bFastStart,
                     REF(UINT32) ulNumBehind,
                     REF(UINT32) ulNumSlightlyBehind,
                     REF(INT32) lBwUsage,
                     REF(HXBOOL) bAllZeroBw,
                     REF(INT32) lMaxNeededBW);
    void SetMaxAccelRatio(double maxAccelRatio);
    void LeaveFastStart(TurboPlayOffReason leftReason);

    void DistributeAndSetBandwidth(const UINT32 ulAggregateUsed,
                                   const UINT32 ulTotalBwAvailable,
                                   const float fAccelerationFactor,
                                   const UINT32 ulOriginalHighestBandwidthAvail,
                                   const UINT32 ulResistanceBitRate,
                                   const HXBOOL bEnableSDB);

    UINT32 ChangeBwBecauseOfCongestion(UINT32 ulNewValue,
                                       REF(HXBOOL) bLossBehind);

    UINT32 AdjustForTCP(UINT32 ulActualRate) const;
    UINT32 ApplyNonFastStartLimits(const UINT32 ulNewValue) const;
    UINT32 Apply3xUpshiftLimit(const UINT32 ulNewValue) const;

private:
    void SetupLoadTestVars(IUnknown* pUnknown);
    HX_RESULT GetStreamBW(IHXValues* pProps, UINT32 uStream,
                          REF(UINT32) ulBw);

    UINT32	m_ulSubscribedBw;
    UINT32				THRESHOLD;
    INT32				m_lOuterThreshold;

    INT32				CalcBackup(UINT32, UINT32);

    INT32				m_lRefCount;

    typedef struct _BwDetectionData
    {
	UINT32	    m_ulSize;
	UINT32	    m_ulTimeStamp;
	UINT32	    m_ulATime;
    } BwDetectionData;

    
    /* For Bandwidth Detection */
    BwDetectionData*			m_pBwDetectionData;
    UINT32				m_ulBwDetectionDataLen;
    UINT32				m_ulSlidingWindowLocation;
    UINT32				m_ulBwDetectionDataCount;
    HXSM*				m_pHXASM;
    IUnknown*				m_pContext;
    IHXValues* m_pSubscriptionVariables;

private:
    HXBOOL				AllocBWDetectionData(UINT32 ulReqSize);
};

class ASMStreamInfo
{
public:
    ASMStreamInfo(ASMSourceInfo* pASMSourceInfo,
                  IUnknown* pStream,
                  REF(UINT32) ulLowestBwBeforeTS);
    ~ASMStreamInfo();

    UINT32 GetFixedBw() const { return m_ulFixedBandwidth;}
    UINT32 GetCurrentSubBw() const;
    UINT32 GetMaxSubscribeBw() const;

    void UpdateSourceSubBw(UINT32 ulCurBw, 
                           UINT32 ulMaxBw);
    HXBOOL HasSourceInfo(ASMSourceInfo* pASMSourceInfo) const { return (m_pASMSourceInfo == pASMSourceInfo);}
    
    HXBOOL ChangeBW(UINT32 newMaxBW, 
                  HXBOOL bUpdateMaxEffeciveThresh,
                  REF(HXBOOL) bDownShift, REF(HXBOOL) bTryToUpshift);
    
    void SetMasterRuleBookOffer(UINT32 ulOffer);
    HX_RESULT GetBiasFactor(REF(INT32)lBias);
    
    UINT32 ComputeBiasedOffer(UINT32 ulTotalBw,
                              UINT32 ulStreamCount,
                              float fbiasMean);

    UINT32 UpdateMasterOffer(UINT32 ulBiasedOffer);

    UINT32 DistributeBw(UINT32 ulBiasedOffer);

    UINT32 GetResistance() const { return m_ulResistanceToLower;}

    UINT32 DropToLowerBw();

    HXBOOL SetSubscriptions(UINT32 ulLeftOverForDropByN);
    void FlushSubscriptions(CHXSimpleList* pSubscriptionChanges);

    HXBOOL				m_bTimeStampDelivery;
    ASMSourceInfo*			m_pASMSourceInfo;
    IHXStreamBandwidthNegotiator*	m_pNegotiator;
    IHXStreamBandwidthBias*		m_pBias;
    IHXAtomicRuleGather*		m_pRuleGather;
    UINT32				m_ulLastBandwidth;
    float*				m_pThreshold;
    UINT32				m_ulNumThresholds;
    UINT32				m_ulThresholdPosition;
    UINT32				m_ulResistanceToLower;
    UINT32				m_ulOffer;
    UINT32				m_ulMasterRuleBookSetOffer;
    UINT32				m_ulMaxEffectiveThreshold;
    UINT32                              m_ulStreamNumber;

private:
    void RecalcResistance();

    UINT32				m_ulFixedBandwidth;
};

class HXSM : public IHXBandwidthManager
{
public:
    HXSM();
    ~HXSM();

    STDMETHOD (QueryInterface ) (THIS_ REFIID ID, void** ppInterfaceObj);
    STDMETHOD_(UINT32, AddRef ) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);

    /*
     *	IHXBandwidthManager methods
     */
    STDMETHOD(RegisterSource)	(THIS_
				HXSource* pSource,
				IUnknown* pUnknown);

    STDMETHOD(RegisterSourcesDone)	(THIS);

    STDMETHOD_(HXBOOL, NotEnoughBandwidth)(THIS);

    STDMETHOD(UnRegisterSource)	(THIS_
				HXSource* pSource);

    /* If the source has enough data, it may tell the bandwidth
     * manager to cut down on accelerated buffering.
     */
    STDMETHOD(ChangeAccelerationStatus)	(THIS_
				HXSource* pSource,
				HXBOOL	   bMayBeAccelerated,
				HXBOOL	   bUseAccelerationFactor,
				UINT32	   ulAccelerationFactor);

    void UpShiftInfo(UINT32 ulTimeStamp, UINT32 ulSize);

    /* Called by HXPlayer at end of each presentation */
    STDMETHOD(PresentationDone)	(THIS);

    STDMETHOD(ChangeBW) (THIS_ 
		UINT32 newBW, HXSource* pSource);

    STDMETHOD(GetUpshiftBW) (THIS_ REF(UINT32) uUpshiftBW);

    typedef enum { HX_NONE, CONGESTION, INIT, INIT_REDIST, REDIST, REDO_ACCEL, CHILL_BUFFERING } RState;
    RState		m_State;
private:
    INT32		m_lRefCount;
    CHXSimpleList*	m_pASMSourceInfo;
    CHXSimpleList*	m_pASMStreamInfo;
    UINT32		m_ulOriginalHighestBandwidthAvail;
    UINT32		m_ulHighestBandwidthAvail;
    UINT32		m_ulPeakUsedBandwidth;
    UINT32		m_ulUpShiftBandwidthAvail;
    UINT32		m_ulNumSources;
    UINT32		m_ulMaxAccelBitRate;
    UINT32		m_ulResistanceBitRate;
    UINT32		m_ulOriginalResistanceBitRate;
    INT32		m_ulNumReportsSinceUpShift;
    UINT32		m_ulLastStableBandwidth;
    UINT32		m_ulUpShiftTestPointScaleFactor;
    UINT32		m_ulOfferToRecalc;
    UINT32		m_ulNextPacketWindow;
    INT32		m_lPacketCounter;
    UINT32		m_ulUpShiftPastResistanceCount;
    INT16		m_lLoss : 16;
    HX_BITFIELD		m_bInitialHighBwAvail : 1;
    HX_BITFIELD		m_bPipeFull : 1;
    HX_BITFIELD		m_bDidOfferUpShiftToRecalc : 1;
    HX_BITFIELD         m_bEnableSDB : 1;
    HXMovingMedian      m_upshiftBW;

    HXFLOAT m_fAccelerationFactor;

    HXNetInterfaceAdviseSinkHelper m_netIFSinkHlpr;

#ifndef GOLD
    IHXErrorMessages*	m_pEM;
#endif
    
    void InitBandwidthVars(IUnknown* pUnknown);
    ASMSourceInfo* FindASMSourceInfo(HXSource* pSource);
    void UpdateHighBwAvailOnPipeFull(INT32 lBwUsage);

    void Recalc();
    void RecalcAccel();
 
    void DistributeAndSetBandwidth(const UINT32 ulAggregateUsed,
                                   const UINT32 ulTotalBwAvailable);
    void EnterCongestionState(float fRecalcOffer,
                              const UINT32 ulAggregateUsed);
    void HandleCongestionState(float fConservativeOffer,
                               float fRequiredOffer,
                               const UINT32 ulAggregateUsed);

    friend class ASMSourceInfo;

    HX_RESULT NetInterfacesUpdated();
    static HX_RESULT static_NetInterfacesUpdated(void* pObj);
};

#endif  /* ifndef _HXSM_H_ */
