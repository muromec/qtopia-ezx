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

#ifndef _STREAMGROUPMGR_H_
#define _STREAMGROUPMGR_H_

#include "ratedescmgr.h"
#include "hxstrutl.h"
#include "ispifs.h"
#include "unkimp.h"

_INTERFACE IHXCommonClassFactory;
_INTERFACE IHXValues;
_INTERFACE IHXRateDescVerifier;
_INTERFACE IHXASMSource;

class ASMRuleBook;


/////////////////////////////////////////////////////////////////////////
// Class:
//  CAsmRuleBookParser
// Purpose:
//  Helper class for parsing ASM rulebook
struct CAsmRuleBookParser
{
    static HX_RESULT GetThresholdInfo(ASMRuleBook* pRules, IHXCommonClassFactory *pCCF, REF(UINT32) ulNumThreshold, REF(float*) pThresholds);
    static HX_RESULT GetRuleSubscriptions(ASMRuleBook* pRuleBook, IHXCommonClassFactory *pCCF, UINT32 ulBandwidth, REF(BOOL*) pSubs);
};

/////////////////////////////////////////////////////////////////////////
// Class:
//  CSwitchGroupContainer
// Purpose:
//  Container for physical streams.  Implements custom behavior for adding/comparing 
//  physical streams.
class CSwitchGroupContainer: 
    public CUnknownIMP
    ,public CRateDescriptionMgr
    ,public IHXRateDescEnumerator
{
    DECLARE_UNKNOWN(CSwitchGroupContainer)

public:

    CSwitchGroupContainer();
    virtual ~CSwitchGroupContainer();

    // IHXRateDescEnumerator methods
    STDMETHOD_(UINT32,GetNumRateDescriptions) (THIS) {return CRateDescriptionMgr::GetNumRateDescriptions();}
    STDMETHOD(GetRateDescription)(THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc) {return CRateDescriptionMgr::GetRateDescription(ulIndex, pRateDesc);}

    STDMETHOD_(UINT32,GetNumSelectableRateDescriptions) (THIS) {return CRateDescriptionMgr::GetNumSelectableRateDescriptions();}
    STDMETHOD(GetSelectableRateDescription)(THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc) {return CRateDescriptionMgr::GetSelectableRateDescription(ulIndex, pRateDesc);}
    STDMETHOD_(UINT32,GetNumSwitchableRateDescriptions) (THIS) {return CRateDescriptionMgr::GetNumSwitchableRateDescriptions();}
    STDMETHOD(GetSwitchableRateDescription)(THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc) {return CRateDescriptionMgr::GetSwitchableRateDescription(ulIndex, pRateDesc);}

    STDMETHOD(FindRateDescByRule)(THIS_ UINT32 ulRuleNumber, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc) {return CRateDescriptionMgr::FindRateDescByRule(ulRuleNumber, bRequireSelectable, bRequireSwitchable, pRateDesc);}
    STDMETHOD(FindRateDescByExactAvgRate)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc) {return CRateDescriptionMgr::FindRateDescByExactAvgRate(ulAvgRate, bRequireSelectable, bRequireSwitchable, pRateDesc);}
    STDMETHOD(FindRateDescByClosestAvgRate)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc) {return CRateDescriptionMgr::FindRateDescByClosestAvgRate(ulAvgRate, bRequireSelectable, bRequireSwitchable, pRateDesc);}
    STDMETHOD(FindRateDescByMidpoint)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc) {return CRateDescriptionMgr::FindRateDescByMidpoint(ulAvgRate, bRequireSelectable, bRequireSwitchable, pRateDesc);}

    // CSwitchGroupContainer methods
    inline UINT32 GetNumPhysicalStreams() {return CRateDescriptionMgr::GetNumRateDescriptions();}
    inline UINT32 GetMaxPhysicalStreams() {return GetMaxRateDescriptions();}
    inline UINT32 GetMaxRules() {return GetMaxRateDescriptions();}

    inline HX_RESULT AddPhysicalStream(CPhysicalStream* pPhysicalStream) 
        { return AddRateDescription(pPhysicalStream); }
    inline CPhysicalStream* GetPhysicalStream(UINT32 ulIndex)
        { return (CPhysicalStream*)CRateDescriptionMgr::GetRateDescription(ulIndex); }

    HX_RESULT HandlePacket(UINT32 unRuleNum);
    BOOL IsPending();

    HX_RESULT SetASMSource(IHXASMSource* pASMSource); 

    inline void SetSwitchGroupID(UINT32 ulSwitchGroupID) 
                { m_ulSwitchGroupID = ulSwitchGroupID; }
    inline UINT32 GetSwitchGroupID() { return m_ulSwitchGroupID; }

    HX_RESULT Subscribe(UINT32 ulStreamNum, UINT32 ulRuleNum);
    HX_RESULT Unsubscribe(UINT32 ulStreamNum, UINT32 ulRuleNum);
    HX_RESULT DoneProcessingRulebook();
    HX_RESULT UpdateCurrentRateDesc();

protected:

    // CSwitchGroupContainer methods    
    inline CPhysicalStream* GetCurrentPhysicalStream()
        { return (CPhysicalStream*)GetCurrentRateDescription(); }

    virtual BOOL isDuplicate(CRateDescription* pRateDescription, REF(UINT32)ulIndex);
    virtual BOOL shouldSwap(UINT32 ulIndex, CRateDescription* pRateDescription);    

    virtual HX_RESULT GrowRateDescContainer(UINT32 ulMaxRateDescriptions);
    virtual HX_RESULT ReplaceRateDescription(UINT32 ulIndex, CRateDescription* pRateDescription);
    virtual HX_RESULT InsertRateDescription(UINT32 ulIndex, CRateDescription* pRateDescription);
    virtual HX_RESULT Cleanup();

private:    

    // CSwitchGroupContainer methods
    void DumpActivation();

    // Need to keep track when exactly subscription takes place -- otherwise rate mgr gets thrown off
    BOOL m_bPending;
    BOOL* m_pbPendingRule;
    BOOL* m_pbSubscribedRule;
    BOOL m_bDoneProcessingRulebook;

    IHXASMSource* m_pASMSource;

    UINT32 m_ulSwitchGroupID;
};

/////////////////////////////////////////////////////////////////////////
// Class:
//  CStreamGroupContainer
// Purpose:
//  Container for physical streams.  Implements custom behavior for adding/comparing 
//  physical streams, and tracks ShiftPending behavior.
class CStreamGroupContainer: 
    public CRateDescriptionMgr
{
public:
    // CStreamGroupContainer methods
    inline UINT32 GetNumPhysicalStreams() {return CRateDescriptionMgr::GetNumRateDescriptions();}
    inline UINT32 GetMaxPhysicalStreams() {return GetMaxRateDescriptions();}

    inline HX_RESULT AddPhysicalStream(CPhysicalStream* pPhysicalStream) 
        { return AddRateDescription(pPhysicalStream); }
    inline CPhysicalStream* GetPhysicalStream(UINT32 ulIndex)
        { return (CPhysicalStream*)CRateDescriptionMgr::GetRateDescription(ulIndex); }
    inline CPhysicalStream* GetCurrentPhysicalStream()
        { return (CPhysicalStream*)GetCurrentRateDescription(); }

protected:

    BOOL isDuplicate(CRateDescription* pRateDescription, REF(UINT32)ulIndex);
    BOOL shouldSwap(UINT32 ulIndex, CRateDescription* pRateDescription);    
};


/////////////////////////////////////////////////////////////////////////
// Class:
//  CStreamGroupManager
// Purpose:
//  Container for physical streams.  Processes stream header/ASM stream rulebook
//  and populates the container with physical streams.  Subscribes/unsubscribes
//  to the appropriate rules.  Handles stream-level upshift/downshift requests
class CStreamGroupManager: 
    public CUnknownIMP
    ,public CStreamGroupContainer
    ,public IHXRateDescEnumerator
{
    DECLARE_UNKNOWN(CStreamGroupManager)

public:
    CStreamGroupManager(IHXCommonClassFactory* pCCF=NULL);
    virtual ~CStreamGroupManager();

    // IHXRateDescEnumerator methods
    STDMETHOD_(UINT32,GetNumRateDescriptions) (THIS) 
        { return CRateDescriptionMgr::GetNumRateDescriptions(); }
    STDMETHOD(GetRateDescription)(THIS_ UINT32 ulIndex, 
                                  REF(IHXRateDescription*)pRateDesc) 
        { return CRateDescriptionMgr::GetRateDescription(ulIndex, pRateDesc); }
    STDMETHOD_(UINT32,GetNumSelectableRateDescriptions) (THIS) 
        { return CRateDescriptionMgr::GetNumSelectableRateDescriptions(); }
    STDMETHOD(GetSelectableRateDescription)(THIS_ UINT32 ulIndex, 
                                            REF(IHXRateDescription*)pRateDesc)
        { return CRateDescriptionMgr::GetSelectableRateDescription(ulIndex, pRateDesc); }
    STDMETHOD_(UINT32,GetNumSwitchableRateDescriptions) (THIS) 
        { return CRateDescriptionMgr::GetNumSwitchableRateDescriptions(); }
    STDMETHOD(GetSwitchableRateDescription)(THIS_ UINT32 ulIndex, 
                                            REF(IHXRateDescription*)pRateDesc) 
        { return CRateDescriptionMgr::GetSwitchableRateDescription(ulIndex, pRateDesc); }

    STDMETHOD(FindRateDescByRule)(THIS_ UINT32 ulRuleNumber, 
                                  BOOL bRequireSelectable, 
                                  BOOL bRequireSwitchable, 
                                  REF(IHXRateDescription*)pRateDesc) 
    { 
        return CRateDescriptionMgr::FindRateDescByRule(ulRuleNumber, 
                bRequireSelectable, bRequireSwitchable, pRateDesc); 
    }
    STDMETHOD(FindRateDescByExactAvgRate)(THIS_ UINT32 ulAvgRate, 
                                          BOOL bRequireSelectable, 
                                          BOOL bRequireSwitchable, 
                                          REF(IHXRateDescription*)pRateDesc) 
    { 
        return CRateDescriptionMgr::FindRateDescByExactAvgRate(ulAvgRate, 
                bRequireSelectable, bRequireSwitchable, pRateDesc);
    }
    STDMETHOD(FindRateDescByClosestAvgRate)(THIS_ UINT32 ulAvgRate, 
                                            BOOL bRequireSelectable, 
                                            BOOL bRequireSwitchable, 
                                            REF(IHXRateDescription*)pRateDesc) 
    {
        return CRateDescriptionMgr::FindRateDescByClosestAvgRate(ulAvgRate, 
            bRequireSelectable, bRequireSwitchable, pRateDesc);
    }
    STDMETHOD(FindRateDescByMidpoint)(THIS_ UINT32 ulAvgRate, 
                                      BOOL bRequireSelectable, 
                                      BOOL bRequireSwitchable, 
                                      REF(IHXRateDescription*)pRateDesc)
    { 
        return CRateDescriptionMgr::FindRateDescByMidpoint(ulAvgRate, 
                bRequireSelectable, bRequireSwitchable, pRateDesc);
    }

    // CStreamGroupManager methods
    HX_RESULT Init(UINT32 ulStreamGroupNum, 
                   UINT32 ulMaxNumLogicalStreams, 
                   BOOL bCheckAverageBandwidth);
    HX_RESULT SetRateDesc(IHXRateDescription* pRateDesc, 
                          IHXStreamRateDescResponse* pResp,
                          UINT32 ulLogicalStream = kulInvalidLogicalStreamNum);
    HX_RESULT Upshift(UINT32 ulRate, IHXStreamRateDescResponse* pResp);
    HX_RESULT Downshift(UINT32 ulRate, IHXStreamRateDescResponse* pResp);
    HX_RESULT SetASMSource(IHXASMSource* pASMSource); 
    HX_RESULT SetStreamHeader(UINT32 ulLogicalStreamNum, 
                              IHXValues* pStreamHeader); 
    HX_RESULT CommitInitialRateDesc(BOOL bDisableAudioRateSwitching = FALSE);
    HX_RESULT GetLogicalStream(UINT32 ulLogicalStreamNum, 
                               REF(IHXRateDescEnumerator*)pLogicalStreamEnum);
    HX_RESULT GetSelectedLogicalStreamNum(REF(UINT32)ulSelectedLogicalStreamNum);
    HX_RESULT SelectLogicalStream(UINT32 ulLogicalStream);

    inline BOOL IsInitalRateDescCommitted() 
    { return m_bInitialRateDescCommitted; }

    HX_RESULT SubscribeLogicalStreamRule(UINT32 ulLogicalStreamNum, 
                                         UINT32 ulRuleNum, 
                                         IHXStreamRateDescResponse* pResp);
    HX_RESULT UnsubscribeLogicalStreamRule(UINT32 ulLogicalStreamNum, 
                                           UINT32 ulRuleNum, 
                                           IHXStreamRateDescResponse* pResp);

    HX_RESULT HandlePacket(UINT32 unRuleNum);
    BOOL IsPending();

    void ShiftDone(HX_RESULT status);

protected:

    // CStreamGroupManager methods
    HX_RESULT ProcessRulebook(ASMRuleBook* pRuleBook, 
                              UINT32 ulLogicalStreamNum,
                              IHXValues* pStreamHeader, 
                              CSwitchGroupContainer* pLogicalStream);
    HX_RESULT GetPhysicalStreamRules(ASMRuleBook* pRuleBook, 
                                     UINT32 ulBandwidth, 
                                     REF(UINT32)ulNumRules, 
                                     REF(UINT32*)pRules);
    HX_RESULT HandleDefaults(CPhysicalStream* pPhysicalStream, 
                             IHXValues* pStreamHeader,
                             BOOL bDefaultStream);
    HX_RESULT HandleRVThinningStream(CPhysicalStream* pPhysicalStream, 
                                     ASMRuleBook* pRuleBook, 
                                     float* pThresholds, 
                                     UINT32 ulNumThreshold, 
                                     UINT32 ulCurThresholdIndex);
    HX_RESULT InitPhysicalStream(CPhysicalStream* pPhysicalStream, 
                                    UINT32 ulLogicalStreamNum, 
                                    IHXValues* pStreamHeader);
    HX_RESULT UpdatePhysicalStream(CPhysicalStream* pPhysicalStream, 
                                    UINT32 ulLogicalStreamNum, 
                                    IHXValues* pStreamHeader);
    BOOL IsDefaultStream(CPhysicalStream* pPhysicalStream, 
                            IHXValues* pStreamHeader);

    HX_RESULT UpdateSwitchGroups(CPhysicalStream* pSelectedPhysicalStream);
    HX_RESULT DisableRateSwitching(CPhysicalStream* pSelectedPhysicalStream);

    HX_RESULT Switch(CPhysicalStream* pNew, UINT32 ulNewLogical);
    HX_RESULT SubscribePhysicalStream(CPhysicalStream* pPhysicalStream, 
                                        UINT32 ulLogicalStream);
    HX_RESULT UnsubscribePhysicalStream(CPhysicalStream* pPhysicalStream, 
                                        UINT32 ulLogicalStream);
    HX_RESULT UpdateCurrentRateDesc();

    CSwitchGroupContainer* GetSwitchGroup(UINT32 ulSwitchGroupID);

    BOOL m_bCheckAverageBandwidth;

    IHXCommonClassFactory* m_pCCF;

    IHXASMSource* m_pASMSource;

    IHXBuffer* m_pMimeType;

    UINT32 m_ulStreamGroupNum;
    BOOL m_bInitialRateDescCommitted;  // Can't Upshift/Downshift until initial rate has been committed

    UINT32 m_ulNumLogicalStreams;
    UINT32 m_ulMaxNumLogicalStreams;
    UINT32 m_ulNumSwitchGroups;

    CSwitchGroupContainer** m_ppSwitchGroup;
    CSwitchGroupContainer** m_ppLogicalStream;    

    IHXStreamRateDescResponse* m_pRateDescResp;

    UINT32 m_ulSelectedLogicalStream; // The logical stream that contains the initial rate desc
};

#endif /* _STREAMGROUPMGR_H_ */
