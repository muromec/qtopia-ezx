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
#ifndef _UBERSTREAMMGR_H_
#define _UBERSTREAMMGR_H_

#include "ratedescmgr.h"
#include "hxstrutl.h"
#include "ispifs.h"
#include "unkimp.h"
#include "isifs.h"
#include "streamgroupmgr.h"

_INTERFACE IHXValues;
_INTERFACE IHXRateDescVerifier;
_INTERFACE IHXCommonClassFactory;
_INTERFACE IHXASMSource;
_INTERFACE IHXSyncHeaderSource;

class ASMRuleHandler;
class ASMRuleBook;
class CStreamGroupManager;
class CUberStreamManager;
class CPhysicalStream;

/////////////////////////////////////////////////////////////////////////
// Class:
//  CUberStreamContainer
// Purpose:
//  Container for bandwidth groupings.  Implements custom behavior for adding/comparing
//  bandwidth groupings.
class CUberStreamContainer:
    public CRateDescriptionMgr
{
public:
    CUberStreamContainer();
    virtual ~CUberStreamContainer();

    // CUberStreamContainer methods
    inline UINT32 GetNumBandwidthGroupings() {return GetNumRateDescriptions();}

    CBandwidthGrouping* GetBandwidthGrouping(UINT32 ulIndex);
    HX_RESULT GetBandwidthGrouping(UINT32 ulIndex, REF(CBandwidthGrouping*)pBandwidthGrouping);
    CBandwidthGrouping* GetCurrentBandwidthGrouping();
    HX_RESULT AddBandwidthGrouping(CBandwidthGrouping* pBandwidthGrouping);

private:

    // CUberStreamContainer methods
    BOOL isDuplicate(CRateDescription* pRateDescription, REF(UINT32)ulIndex);
    BOOL shouldSwap(UINT32 ulIndex, CRateDescription* pRateDescription);
};

/////////////////////////////////////////////////////////////////////////
// Class:
//  CUberStreamManager
// Purpose:
//  Container for bandwidth groupings.  Processes file header/uber rulebook
//  and populates the container with bandwidth groupings.  Handles
//  aggregate upshift/downshift requests
class CUberStreamManager:
    public CUnknownIMP
    ,public IHXUberStreamManager
    ,public IHXUberStreamManagerInit
{
    DECLARE_UNKNOWN(CUberStreamManager)

public:
    CUberStreamManager(IHXCommonClassFactory* pCCF=NULL, IHXQoSProfileConfigurator* pQoSConfig=NULL);
    virtual ~CUberStreamManager();

    // IHXRateDescEnumerator methods (just delegate to CUberStreamContainer)
    STDMETHOD_(UINT32,GetNumRateDescriptions) (THIS) {return m_cUberContainer.GetNumRateDescriptions();}
    STDMETHOD(GetRateDescription)(THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc) {return m_cUberContainer.GetRateDescription(ulIndex, pRateDesc);}

    STDMETHOD_(UINT32,GetNumSelectableRateDescriptions) (THIS) {return m_cUberContainer.GetNumSelectableRateDescriptions();}
    STDMETHOD(GetSelectableRateDescription)(THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc) {return m_cUberContainer.GetSelectableRateDescription(ulIndex, pRateDesc);}
    STDMETHOD_(UINT32,GetNumSwitchableRateDescriptions) (THIS) {return m_cUberContainer.GetNumSwitchableRateDescriptions();}
    STDMETHOD(GetSwitchableRateDescription)(THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc) {return m_cUberContainer.GetSwitchableRateDescription(ulIndex, pRateDesc);}

    STDMETHOD(FindRateDescByRule)(THIS_ UINT32 ulRuleNumber, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc) {return m_cUberContainer.FindRateDescByRule(ulRuleNumber, bRequireSelectable, bRequireSwitchable, pRateDesc);}
    STDMETHOD(FindRateDescByExactAvgRate)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc) {return m_cUberContainer.FindRateDescByExactAvgRate(ulAvgRate, bRequireSelectable, bRequireSwitchable, pRateDesc);}
    STDMETHOD(FindRateDescByClosestAvgRate)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc) {return m_cUberContainer.FindRateDescByClosestAvgRate(ulAvgRate, bRequireSelectable, bRequireSwitchable, pRateDesc);}
    STDMETHOD(FindRateDescByMidpoint)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc) {return m_cUberContainer.FindRateDescByMidpoint(ulAvgRate, bRequireSelectable, bRequireSwitchable, pRateDesc);}

    // IHXUberStreamManager methods
    STDMETHOD_(UINT32,GetNumStreamGroups) (THIS) {return m_ulNumStreamGroups;}
    STDMETHOD(GetStreamGroup)(THIS_ UINT32 ulStreamGroupNum, REF(IHXRateDescEnumerator*)pStreamGroupMgr);
    STDMETHOD_(UINT32,GetNumActiveStreamGroups) (THIS);
    STDMETHOD(FindStreamGroupByLogicalStream)(THIS_ UINT32 ulLogicalStream, REF(UINT32)ulStreamGroup);
    STDMETHOD_(UINT32,GetNumLogicalStreams) (THIS) {return m_ulNumLogicalStreams;}
    STDMETHOD(GetLogicalStream)(THIS_ UINT32 ulLogicalStreamNum, REF(IHXRateDescEnumerator*)pLogicalStreamEnum);
    STDMETHOD(GetSelectedLogicalStreamNum)(THIS_ UINT32 ulStreamGroupNum, REF(UINT32)ulSelectedLogicalStreamNum);

    STDMETHOD(SetAggregateRateDesc)(THIS_ IHXRateDescription* pRateDesc, IHXRateDescResponse* pResp);
    STDMETHOD(GetCurrentAggregateRateDesc)(THIS_ REF(IHXRateDescription*)pRateDesc) {return m_cUberContainer.GetCurrentRateDesc(pRateDesc);}
    STDMETHOD(CommitInitialAggregateRateDesc) (THIS);
    STDMETHOD_(BOOL,IsInitalAggregateRateDescCommitted) (THIS) {return m_bInitialRateDescCommitted;}
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
    STDMETHOD(UnsubscribeLogicalStreamRule) (THIS_ UINT32 ulLogicalStreamNum,
                                            UINT32 ulRuleNum,
                                            IHXStreamRateDescResponse* pResp);
    STDMETHOD(GetLowestAvgRate)(THIS_ UINT32 ulStreamGroupNum, REF(UINT32) ulLowestAvgRate);
    STDMETHOD(SetDownshiftOnFeedbackTimeoutFlag)(THIS_ BOOL bFlag);
    STDMETHOD(DetermineSelectableStreams)(THIS_ const StreamAdaptationParams* pStreamAdaptationParams = NULL);

    // IHXUberStreamManagerInit methods
    STDMETHOD(Init) (THIS_ BOOL bCheckAverageBandwidth);
    STDMETHOD(SetFileHeader) (THIS_ IHXValues* pFileHeader);
    STDMETHOD(SetStreamHeader) (THIS_ UINT32 ulLogicalStreamNum, IHXValues* pStreamHeader);
    STDMETHOD(SetASMSource) (THIS_ IHXASMSource* pASMSource);
    void Dump(void);

    // CUberStreamManager methods
    HX_RESULT ModifyHeaders(IHXValues* pHdr, BOOL bUseAnnexG);

    /* Need to keep track of subscription state */
    HX_RESULT HandlePacket(UINT32 ulStreamGroupNum, UINT16 unRuleNum);
    inline BOOL IsShiftPending(void) { return m_bShiftPending; }
    inline BOOL IsShiftPending(UINT32 ulStreamGroup);
    inline void ShiftDone(HX_RESULT status, UINT32 ulStreamGroup);
    HX_RESULT SetStreamSelector(THIS_ CStreamSelector* pStrmSelector);

protected:


    // CStreamGroupMgr rulebook handling routines
    HX_RESULT ProcessRulebooks();
    HX_RESULT ProcessUberRule();
    HX_RESULT SynthesizeUberRulebook(BOOL bSkipInactiveStreamGroups);
    HX_RESULT ProcessUberRulebook(IHXBuffer* pRulebook);

    HX_RESULT UberSubscription(UINT32 ulBandwidth, REF(UINT32)unCount, REF(UINT32*)pRules);
    HX_RESULT ExtractBandwidthGrouping(UINT32 ulBandwidth);
    HX_RESULT CreateBandwidthGrouping(CPhysicalStream* apPhysicalStream[]);
    HX_RESULT CreateBandwidthGroupingFromCurrentStreams();

    HX_RESULT FixupFileHeader(IHXValues* pFileHeader);
    HX_RESULT FixupStreamHeader(UINT32 ulLogicalStreamNum, IHXValues* pStreamHeader);

    void UpdateShiftPending();
    HX_RESULT UpdateAggregateRateDesc();

    UINT32 GetAvgRate(UINT32 ulStreamGroupNum, UINT32 ulRateDescNum);
    UINT32 GetLowestNonThinningAvgRate(UINT32 ulStreamGroupNum);

    UINT32 m_ulNumStreamGroups;
    UINT32 m_ulNumLogicalStreams;
    UINT32 m_ulNumLogicalStreamProcessed;


    BOOL	    m_bShiftPending;

    /* should change this to not use ASMRuleBook class */
    ASMRuleBook*    m_pUberRuleBook;

    CUberStreamContainer m_cUberContainer;
    CStreamGroupManager** m_ppStreamGroup;   // Used to store stream groups

    IHXCommonClassFactory*  m_pCCF;
    IHXQoSProfileConfigurator*  m_pQoSConfig;

    BOOL	    m_bCheckAverageBandwidth;
    BOOL m_bInitialRateDescCommitted;  // Can't Upshift/Downshift until initial rate has been committed

    IHXASMSource* m_pASMSource;
    UINT32* m_aulLogicalStreamToStreamGroup;

    BOOL m_bStepwiseUpshift;
    BOOL m_bStepwiseDownshift;
    BOOL m_bDownshiftOnFeedbackTimeout;
    CStreamSelector* m_pStreamSelector;
};

inline BOOL
CUberStreamManager::IsShiftPending(UINT32 ulStreamGroup)
{
    HX_ASSERT(ulStreamGroup < m_ulNumStreamGroups);
    return m_ppStreamGroup[ulStreamGroup]->IsPending();
}

inline void
CUberStreamManager::ShiftDone(HX_RESULT status, UINT32 ulStreamGroup)
{
    HX_ASSERT(ulStreamGroup < m_ulNumStreamGroups);
    m_ppStreamGroup[ulStreamGroup]->ShiftDone(status);
}

#endif /* _UBERSTREAMMGR_H_ */
