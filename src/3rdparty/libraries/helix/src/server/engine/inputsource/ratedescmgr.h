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

#ifndef _RATEDESCMGR_H_
#define _RATEDESCMGR_H_

#include "ispifs.h"

_INTERFACE IHXValues;
_INTERFACE IHXRateDescVerifier;

static const UINT32 kulInvalidSwitchGroupID = 0;
static const UINT32 kulInvalidRateDescIndex = 0xffffffff;
static const UINT32 kulInvalidLogicalStreamNum = 0xffffffff;
static const UINT32 kulInvalidStreamGroupNum = 0xffffffff;

/////////////////////////////////////////////////////////////////////////
// Class:
//  CRateDescription
// Purpose:
//  Contains properties of a physical stream or bandwidth grouping
class CRateDescription:
    public IHXRateDescription
{
public:
    CRateDescription(UINT32 ulBandwidth);

    // IUnknown methods
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);    

    // IHXRateDescription methods
    STDMETHOD(GetAvgRate)   (THIS_ REF(UINT32)ulRate);
    STDMETHOD(GetMaxRate)   (THIS_ REF(UINT32)ulRate);
    STDMETHOD(GetPredata)   (THIS_ REF(UINT32)ulRate);
    STDMETHOD(GetPreroll)   (THIS_ REF(UINT32)ulRate);    
    STDMETHOD(GetValue)     (THIS_ UINT32 id, REF(UINT32)ulValue);

    STDMETHOD_(UINT32, GetNumBandwidthAllocations)() { return m_ulNumBandwidthAllocations;}
    STDMETHOD_(UINT32*, GetBandwidthAllocationArray)() { return m_aulBandwidthAllocation;}

    STDMETHOD_(UINT32,GetNumRules) (THIS) {return m_ulNumRules;}; 
    STDMETHOD_(UINT32*,GetRuleArray) (THIS) {return m_aulRule;};

    STDMETHOD_(BOOL,IsSelectable) (THIS);
    STDMETHOD_(BOOL,IsSwitchable) (THIS);

    STDMETHOD(ExcludeFromSelection) (THIS_ BOOL bExcludeFromSelection, EHXSelectableReason eSelReason);
    STDMETHOD(ExcludeFromSwitching) (THIS_ BOOL bExcludeFromSwitching, EHXSwitchableReason eSwiReason);

    // CRateDescription methods
    inline UINT32 GetSwitchGroupID() { return m_ulSwitchGroupID; }

    virtual void Dump();
    
protected:
    virtual ~CRateDescription();

private:    
    UINT32  m_ulRefCount;
public:

    UINT32  m_ulAvgRate;
    UINT32  m_ulMaxRate;
    UINT32  m_ulPredata;
    UINT32  m_ulPreroll;

    UINT32 m_ulSwitchGroupID;

    UINT32  m_ulNumRules; 
    UINT32* m_aulRule;

    UINT32 m_ulNumBandwidthAllocations;
    UINT32* m_aulBandwidthAllocation;

    BOOL m_abExcludeFromSwitching[HX_SWI_NUM_REASONS];
    BOOL m_abExcludeFromSelection[HX_SEL_NUM_REASONS];

    BOOL m_bIsThinningStream; // a.k.a. the keyframe-only stream
};


//XXXDC - Must rename this class for statically-linked builds so it
// doesn't conflict with a class of the same name in the rmff plugin:
#ifdef _STATICALLY_LINKED
#define CPhysicalStream CPhysicalStream_ratedescmgr_h
#endif

/////////////////////////////////////////////////////////////////////////
// Class:
//  CPhysicalStream
// Purpose:
//  Represents a physical stream.  Has all rate description properties 
//  except for bandwidth allocation info
class CPhysicalStream: 
    public CRateDescription
{
public:
    CPhysicalStream(UINT32 ulBandwidth);

    BOOL IsEntriesSame(CPhysicalStream* pPhysicalStream);

    inline UINT32  GetBandwidth() { return m_ulAvgRate; }
    inline UINT32  GetDefaultLogicalStream() { return m_ulDefaultStream; }
    inline void SetDefaultLogicalStream(UINT32 ulStream) 
        { m_ulDefaultStream = ulStream; }

    // Note: These methods declared private as cheap way to prevent 
    // them from being called by the container classes 
private:
    STDMETHOD_(UINT32, GetNumBandwidthAllocations)() { return m_ulNumBandwidthAllocations;}
    STDMETHOD_(UINT32*, GetBandwidthAllocationArray)() { return m_aulBandwidthAllocation;}

    UINT32 m_ulDefaultStream;
};

/////////////////////////////////////////////////////////////////////////
// Class:
//  CBandwidthGrouping
// Purpose:
//  Represents a bandwidth grouping (how to allocate bandwidth to different 
//  stream groups).
class CBandwidthGrouping:
    public CRateDescription
{
public:
    CBandwidthGrouping(UINT32 ulBandwidth);

    BOOL IsEntriesSame(CBandwidthGrouping* pBandwidthGrouping);

    inline UINT32  GetBandwidth() { return m_ulAvgRate; }

    // Note: These methods declared private as cheap way to prevent 
    // them from being called by the container classes 
private:
    STDMETHOD_(UINT32,GetNumRules) (THIS) {return m_ulNumRules;}; 
    STDMETHOD_(UINT32*,GetRuleArray) (THIS) {return m_aulRule;};

    inline UINT32 GetSwitchGroupID() { return m_ulSwitchGroupID; }
};

/////////////////////////////////////////////////////////////////////////
// Class:
//  CRateDescriptionMgr
// Purpose:
//  Container class for rate descriptions.  Has utility methods for 
//  enumerating/searching for rate descriptions.
//  Rate descriptions are sorted in asccending order by avg bitrate
class CRateDescriptionMgr
{
public:

    CRateDescriptionMgr();
    virtual ~CRateDescriptionMgr();

    // IHXRateDescEnumerator methods
    STDMETHOD_(UINT32,GetNumRateDescriptions) (THIS) {return m_ulNumRateDescriptions;}
    STDMETHOD(GetRateDescription) (THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc);
    STDMETHOD_(UINT32,GetNumSelectableRateDescriptions) (THIS);
    STDMETHOD(GetSelectableRateDescription) (THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc);
    STDMETHOD_(UINT32,GetNumSwitchableRateDescriptions) (THIS);
    STDMETHOD(GetSwitchableRateDescription) (THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc);

    STDMETHOD(GetCurrentRateDesc) (THIS_ REF(IHXRateDescription*)pRateDesc);
    STDMETHOD(FindRateDescByRule)(THIS_ UINT32 ulRuleNumber, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc);
    STDMETHOD(FindRateDescByExactAvgRate)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc);
    STDMETHOD(FindRateDescByClosestAvgRate)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc);
    STDMETHOD(FindRateDescByMidpoint)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc);

    // CRateDescriptionMgr methods
    virtual HX_RESULT Init(UINT32 ulMaxRateDescriptions);
    virtual HX_RESULT Cleanup();

    HX_RESULT AddRateDescription(CRateDescription* pRateDescription);

    CRateDescription* GetRateDescription(UINT32 ulIndex);
    CRateDescription* GetCurrentRateDescription();
    CRateDescription* FindRateDescription(UINT32 ulBandwidth);
    HX_RESULT GetRateDescIndex(IHXRateDescription* pRateDescription, REF(UINT32)ulIndex);
    HX_RESULT SetCurrentRateDesc(UINT32 ulIndex) {m_ulCurRateDescriptionIndex = ulIndex; return HXR_OK;}
    HX_RESULT GetNextSwitchableRateDesc(REF(IHXRateDescription*)pRateDesc);
    HX_RESULT GetPrevSwitchableRateDesc(REF(IHXRateDescription*)pRateDesc);


    virtual BOOL IsInitialized() {return m_ppRateDescription != NULL;}

    virtual void Dump();

protected:

    // CRateDescriptionMgr methods
    inline UINT32 GetMaxRateDescriptions() {return m_ulMaxRateDescriptions;}
    virtual HX_RESULT ReplaceRateDescription(UINT32 ulIndex, CRateDescription* pRateDescription);
    virtual HX_RESULT InsertRateDescription(UINT32 ulIndex, CRateDescription* pRateDescription);
    
    virtual HX_RESULT GrowRateDescContainer(UINT32 ulMaxRateDescriptions);

    virtual BOOL shouldSwap(UINT32 ulIndex, CRateDescription* pRateDescription) = 0;
    virtual BOOL isDuplicate(CRateDescription* pRateDescription, REF(UINT32)ulIndex) = 0;

    UINT32 m_ulCurRateDescriptionIndex;

private:

    UINT32 m_ulNumRateDescriptions;
    UINT32 m_ulMaxRateDescriptions;
    IHXRateDescription** m_ppRateDescription;
};

#endif /* _RATEDESCMGR_H_ */
