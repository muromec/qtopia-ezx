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
#include "ratedescmgr.h"

#include "source.h"

#include "hxassert.h"

    
CPhysicalStream::CPhysicalStream(UINT32 ulBandwidth):
    CRateDescription(ulBandwidth),
    m_ulDefaultStream(kulInvalidLogicalStreamNum)
{
}
    
    
CBandwidthGrouping::CBandwidthGrouping(UINT32 ulBandwidth):
    CRateDescription(ulBandwidth)
{
}
    
/////////////////////////////////////////////////////////////////////////
// Method:
//  CPhysicalStream::IsEntriesSame
// Purpose:
//  Checks if two CPhysicalStream are associated with the same set of ASM rules
BOOL
CPhysicalStream::IsEntriesSame(CPhysicalStream* pPhysicalStream)
{
    if (pPhysicalStream && pPhysicalStream->GetNumRules() == GetNumRules())
    {
	return !memcmp(pPhysicalStream->GetRuleArray(), GetRuleArray(), sizeof(UINT32)*GetNumRules());
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CBandwidthGrouping::IsEntriesSame
// Purpose:
//  Checks if two CBandwidthGrouping are associated with the same set of ASM rules
BOOL
CBandwidthGrouping::IsEntriesSame(CBandwidthGrouping* pBandwidthGrouping)
{
    if (pBandwidthGrouping && pBandwidthGrouping->GetNumBandwidthAllocations() == GetNumBandwidthAllocations())
    {
	return !memcmp(pBandwidthGrouping->GetBandwidthAllocationArray(), GetBandwidthAllocationArray(), sizeof(UINT32)*GetNumBandwidthAllocations());
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescription::CRateDescription
// Purpose:
//  Constructor
CRateDescription::CRateDescription(UINT32 ulBandwidth)
    : m_ulRefCount(0)
    , m_ulAvgRate(ulBandwidth)
    , m_ulPredata(0)
    , m_ulMaxRate(0)
    , m_ulPreroll(0)
    , m_ulSwitchGroupID(kulInvalidSwitchGroupID)
    , m_ulNumRules(0)
    , m_aulRule(NULL)
    , m_ulNumBandwidthAllocations(0)
    , m_aulBandwidthAllocation(NULL)
    , m_bIsThinningStream(FALSE)
{
    memset(m_abExcludeFromSwitching, 0, sizeof(BOOL)*HX_SWI_NUM_REASONS);
    memset(m_abExcludeFromSelection, 0, sizeof(BOOL)*HX_SEL_NUM_REASONS);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescription::CRateDescription
// Purpose:
//  Destructor
CRateDescription::~CRateDescription()
{
    HX_VECTOR_DELETE(m_aulRule);
    HX_VECTOR_DELETE(m_aulBandwidthAllocation);
}

STDMETHODIMP_(ULONG32) 
CRateDescription::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}
STDMETHODIMP_(ULONG32) 
CRateDescription::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}
STDMETHODIMP 
CRateDescription::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXRateDescription))
    {
	AddRef();
	*ppvObj = (IHXRateDescription*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescription::GetAvgRate
// Purpose:
//  Gets average bandwidth
STDMETHODIMP
CRateDescription::GetAvgRate(REF(UINT32)ulRate)
{
    ulRate = m_ulAvgRate;
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescription::GetMaxRate
// Purpose:
//  Gets max bandwidth
STDMETHODIMP
CRateDescription::GetMaxRate(REF(UINT32)ulRate)
{
    ulRate = m_ulMaxRate;
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescription::GetAvgRate
// Purpose:
//  Gets predata amount
STDMETHODIMP
CRateDescription::GetPredata(REF(UINT32)ulBytes)
{
    ulBytes = m_ulPredata;
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescription::GetAvgRate
// Purpose:
//  Gets preroll amount
STDMETHODIMP
CRateDescription::GetPreroll(REF(UINT32)ulMs)
{
    ulMs = m_ulPreroll;
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescription::GetValue
// Purpose:
//  Gets property
STDMETHODIMP
CRateDescription::GetValue(UINT32 id, REF(UINT32)ulValue)
{
    if (HX_IS_RATEDESC_PREDATA == id)
    {
	ulValue = m_ulPredata;
	return HXR_OK;
    }
    else if (HX_IS_RATEDESC_MAXRATE == id)
    {
	ulValue = m_ulMaxRate;
	return HXR_OK;
    }
    else if (HX_IS_RATEDESC_PREROLL == id)
    {
	ulValue = m_ulPreroll;
	return HXR_OK;
    }
    return HXR_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescription::IsSelectable
// Purpose:
//  Determines if the rate description is selectable
STDMETHODIMP_(BOOL)
CRateDescription::IsSelectable()
{
    for (UINT32 i=0; i<HX_SEL_NUM_REASONS; i++)
    {
	if (m_abExcludeFromSelection[i])
	    return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescription::IsSwitchable
// Purpose:
//  Determines if the rate description is switchable
STDMETHODIMP_(BOOL)
CRateDescription::IsSwitchable()
{
    for (UINT32 i=0; i<HX_SWI_NUM_REASONS; i++)
    {
	if (m_abExcludeFromSwitching[i])
	    return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescription::ExcludeFromSelection
// Purpose:
//  Excludes the rate description from selection
STDMETHODIMP
CRateDescription::ExcludeFromSelection(BOOL bExcludeFromSelection, EHXSelectableReason eSelReason)
{
    // Validate params
    if (eSelReason >= HX_SEL_NUM_REASONS)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    m_abExcludeFromSelection[eSelReason] = bExcludeFromSelection;
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescription::ExcludeFromSwitching
// Purpose:
//  Excludes the rate description from switching
STDMETHODIMP
CRateDescription::ExcludeFromSwitching(BOOL bExcludeFromSwitching, EHXSwitchableReason eSwiReason)
{
    // Validate params
    if (eSwiReason >= HX_SWI_NUM_REASONS)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    m_abExcludeFromSwitching[eSwiReason] = bExcludeFromSwitching;
    return HXR_OK;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescription::Dump
// Purpose:
//  Dumps CRateDescription
void
CRateDescription::Dump()
{
    printf("%s:%s BW: %6u - ",
	IsSelectable()?"SL":"No", 
	IsSwitchable()?"SW":"No", 
	m_ulAvgRate);
    fflush(0);

    UINT32 i = 0;
    for (i = 0; i < m_ulNumRules; i++)
    {
	printf("%6u ", m_aulRule[i]);
    }

    for (i = 0; i < m_ulNumBandwidthAllocations; i++)
    {
	printf("%6u ", m_aulBandwidthAllocation[i]);
    }

    printf("  MR: %6u", m_ulMaxRate);
    printf("  PR: %6u", m_ulPreroll);
    printf("  PD: %6u", m_ulPredata);

    if (m_ulSwitchGroupID != kulInvalidSwitchGroupID)
    {
        printf("  SG: %2u", m_ulSwitchGroupID);
    }

    printf("\n");    
    fflush(0);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::CRateDescriptionMgr
// Purpose:
//  Constructor
CRateDescriptionMgr::CRateDescriptionMgr():
    m_ppRateDescription(NULL)
    , m_ulMaxRateDescriptions(0)
    , m_ulNumRateDescriptions(0)
    , m_ulCurRateDescriptionIndex(kulInvalidRateDescIndex)
{
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::~CRateDescriptionMgr
// Purpose:
//  Destructor
CRateDescriptionMgr::~CRateDescriptionMgr()
{
    Cleanup();
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::Cleanup
// Purpose:
//  Cleans up CRateDescription array
HX_RESULT
CRateDescriptionMgr::Cleanup()
{
    if (m_ppRateDescription)
    {
	for (UINT32 i= 0; i<m_ulNumRateDescriptions; i++)
	{	
	    HX_RELEASE(m_ppRateDescription[i]);
	}

	HX_VECTOR_DELETE(m_ppRateDescription);
    }

    m_ulNumRateDescriptions = 0;
    m_ulMaxRateDescriptions = 0;
    m_ulCurRateDescriptionIndex = kulInvalidRateDescIndex;

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::Init
// Purpose:
//  Creates CRateDescription array
HX_RESULT
CRateDescriptionMgr::Init(UINT32 ulMaxRateDescriptionCount)
{
    HX_RESULT res = HXR_OK;

    // Validate state
    if (m_ppRateDescription)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    return GrowRateDescContainer(ulMaxRateDescriptionCount);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::GrowRateDescContainer
// Purpose:
//  Creates CRateDescription array
HX_RESULT
CRateDescriptionMgr::GrowRateDescContainer(UINT32 ulMaxRateDescriptionCount)
{
    HX_RESULT res = HXR_OK;

    // Resize rate desc container, if necessary
    if (ulMaxRateDescriptionCount > m_ulMaxRateDescriptions)
    {
	// If the container is being resized (beyond the initial allocation), make the
	// new container a bit larger than necessary to avoid continual resizing
	if (m_ulMaxRateDescriptions > 0)
	    ulMaxRateDescriptionCount += 5;

	// Create rate description array -- one IHXRateDescription per physical stream/bandwidth grouping
	IHXRateDescription** ppRateDescription = new IHXRateDescription*[ulMaxRateDescriptionCount];

	if (!ppRateDescription)
	    res = HXR_OUTOFMEMORY;

	if (SUCCEEDED(res))
	{
	    memset(ppRateDescription, 0, sizeof(IHXRateDescription*)*ulMaxRateDescriptionCount);

	    // If there is an existing rate desc array, copy it over
	    if (m_ppRateDescription)
		memcpy(ppRateDescription, m_ppRateDescription, sizeof(IHXRateDescription*)*m_ulMaxRateDescriptions);
	}

	// If everything went OK, replace the rate desc array
	if (SUCCEEDED(res))
	{
	    m_ulMaxRateDescriptions = ulMaxRateDescriptionCount;
	    
	    HX_VECTOR_DELETE(m_ppRateDescription);
	    m_ppRateDescription = ppRateDescription;
	}

	else
	{
	    HX_VECTOR_DELETE(ppRateDescription);
	}
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::InsertRateDescription
// Purpose:
//  Inserts pRateDescription into the CRateDescription container, at ulIndex,
//  moving the existing entries (ulIndex, m_ulNumRateDescriptions-1) inclusively up one
// Notes:
//  Must ensure that m_ppRateDescription remains a contiguous array
HX_RESULT
CRateDescriptionMgr::InsertRateDescription(UINT32 ulInsertIndex, CRateDescription* pRateDescription)
{    
    HX_RESULT res = HXR_OK;

    // Validate state, params
    if (!m_ppRateDescription || !pRateDescription || 
	ulInsertIndex > m_ulNumRateDescriptions || ulInsertIndex >= m_ulMaxRateDescriptions)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Move all entries after the insert index up one slot
    UINT32 ulNumEntriesToMove = m_ulNumRateDescriptions - ulInsertIndex;
    for (UINT32 i=ulInsertIndex+ulNumEntriesToMove; i>ulInsertIndex; i--)
    {
	m_ppRateDescription[i] = m_ppRateDescription[i-1];
    }

    // Add new rate description
    m_ppRateDescription[ulInsertIndex] = pRateDescription;
    pRateDescription->AddRef();
    
    m_ulNumRateDescriptions++;

    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::ReplaceRateDescription
// Purpose:
//  Sets pRateDescription into the CRateDescription container, potentially
//  replacing an existing entry
// Notes:
//  Must replace an existing rate description -- m_ppRateDescription is contiguous array
HX_RESULT
CRateDescriptionMgr::ReplaceRateDescription(UINT32 ulIndex, CRateDescription* pRateDescription)
{    
    HX_RESULT res = HXR_OK;

    // Validate state, params
    if (!m_ppRateDescription || !pRateDescription || ulIndex >= m_ulNumRateDescriptions || !m_ppRateDescription[ulIndex])
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    HX_RELEASE(m_ppRateDescription[ulIndex]);

    m_ppRateDescription[ulIndex] = pRateDescription;
    pRateDescription->AddRef();

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::AddRateDescription
// Purpose:
//  Inserts pRateDescription into the CRateDescription container
HX_RESULT
CRateDescriptionMgr::AddRateDescription(CRateDescription* pRateDescription)
{    
    HX_RESULT res = HXR_OK;

    // Validate state, params
    if (!m_ppRateDescription || !pRateDescription)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // If the new rate description is not a duplicate, add it to
    // the rate desc container
    UINT32 ulDupIndex =	0;
    if (!isDuplicate(pRateDescription, ulDupIndex))
    {
	if (m_ulNumRateDescriptions == m_ulMaxRateDescriptions)
	{
	    res = GrowRateDescContainer(m_ulMaxRateDescriptions+1);
	}

	// Find insertion point (container sorted by ascending avg rate), and insert
	if (SUCCEEDED(res))
	{
	    UINT32 ulNewAvgRate = 0;
	    res = pRateDescription->GetAvgRate(ulNewAvgRate);

	    UINT32 ulInsertIndex = 0;
	    for (ulInsertIndex=0; ulInsertIndex<m_ulNumRateDescriptions && SUCCEEDED(res); ulInsertIndex++)
	    {
		UINT32 ulCurAvgRate = 0;
		res = m_ppRateDescription[ulInsertIndex]->GetAvgRate(ulCurAvgRate);

		if (SUCCEEDED(res) && ulNewAvgRate < ulCurAvgRate)
		{
		    break;
		}
	    }

	    if (SUCCEEDED(res))
		res = InsertRateDescription(ulInsertIndex, pRateDescription);
	}
    }

    // If the new rate description is a duplicate of an existing entry, replace
    // the existing entry, if necessary
    else
    {
	if (shouldSwap(ulDupIndex, pRateDescription))
	{
	    res = ReplaceRateDescription(ulDupIndex, pRateDescription);
	}
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::FindRateDescByRule
// Purpose:
//  Find the first rate description with ulRuleNumber
STDMETHODIMP
CRateDescriptionMgr::FindRateDescByRule(UINT32 ulRuleNumber, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc)
{
    HX_RESULT res = HXR_FAIL;

    // Validate state
    if (!m_ppRateDescription)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Look for a rate description containing a matching rule number
    for (UINT32 i=0; i <m_ulNumRateDescriptions; i++)
    {
	IHXRateDescription* pCurRateDesc = m_ppRateDescription[i];

	if ((!bRequireSelectable || m_ppRateDescription[i]->IsSelectable()) &&
	    (!bRequireSwitchable || m_ppRateDescription[i]->IsSwitchable()))
	{
	    UINT32* pRuleArray = pCurRateDesc->GetRuleArray();
	    for (UINT32 j=0; j<pCurRateDesc->GetNumRules(); j++)
	    {
		if (pRuleArray[j] == ulRuleNumber)
		{
		    pRateDesc = pCurRateDesc;
		    pRateDesc->AddRef();

		    res = HXR_OK;
		    break;
		}
	    }
	}
    }


    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::FindRateDescByExactAvgRate
// Purpose:
//  Find the first rate description exactly matching ulAvgRate
STDMETHODIMP
CRateDescriptionMgr::FindRateDescByExactAvgRate(UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc)
{
    HX_RESULT res = HXR_FAIL;

    // Validate state
    if (!m_ppRateDescription)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Look for a rate description containing a matching avg rate
    for (UINT32 i=0; i <m_ulNumRateDescriptions; i++)
    {
	if ((!bRequireSelectable || m_ppRateDescription[i]->IsSelectable()) &&
	    (!bRequireSwitchable || m_ppRateDescription[i]->IsSwitchable()))
	{
	    UINT32 ulCurRate = 0;
	    HX_RESULT resBandwidth = m_ppRateDescription[i]->GetAvgRate(ulCurRate);

	    if (SUCCEEDED(resBandwidth) && ulAvgRate == ulCurRate)
	    {
		pRateDesc = m_ppRateDescription[i];
		pRateDesc->AddRef();

		res = HXR_OK;
		break;
	    }
	}
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::FindRateDescByExactAvgRate
// Purpose:
//  Find the rate description closest (lower or equal) to ulAvgRate
STDMETHODIMP
CRateDescriptionMgr::FindRateDescByClosestAvgRate(UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc)
{
    HX_RESULT res = HXR_FAIL;

    // Validate state
    if (!m_ppRateDescription)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Look for the highest rate description <= ulAvgRate
    UINT32 ulHighestAvgRate = 0;
    IHXRateDescription* pHighestRateDesc = NULL;
    for (UINT32 i=0; i <m_ulNumRateDescriptions; i++)
    {
	if ((!bRequireSelectable || m_ppRateDescription[i]->IsSelectable()) &&
	    (!bRequireSwitchable || m_ppRateDescription[i]->IsSwitchable()))
	{
	    UINT32 ulCurRate = 0;
	    HX_RESULT resBandwidth = m_ppRateDescription[i]->GetAvgRate(ulCurRate);

	    if (SUCCEEDED(resBandwidth) && ulCurRate <= ulAvgRate)
	    {
		if (ulCurRate >= ulHighestAvgRate)
		{	
		    ulHighestAvgRate = ulCurRate;
		    pHighestRateDesc = m_ppRateDescription[i];
		}

		res = HXR_OK;
	    }
	}
    }

    // Return matching rate, if found
    if (SUCCEEDED(res))
    {
	pRateDesc = pHighestRateDesc;
	pRateDesc->AddRef();
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::FindRateDescByMidpoint
// Purpose:
//  Find the closest rate description using ulAvgRate as a midpoint
STDMETHODIMP
CRateDescriptionMgr::FindRateDescByMidpoint(UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc)
{
    HX_RESULT res = HXR_FAIL;

    // Validate state
    if (!m_ppRateDescription)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Look for the rate description closest (higher or lower) to ulAvgRate
    UINT32 ulMinDelta = 0xffffffff;
    IHXRateDescription* pClosestRateDesc = NULL;
    for (UINT32 i=0; i <m_ulNumRateDescriptions; i++)
    {
	if ((!bRequireSelectable || m_ppRateDescription[i]->IsSelectable()) &&
	    (!bRequireSwitchable || m_ppRateDescription[i]->IsSwitchable()))
	{
	    UINT32 ulCurRate = 0;
	    HX_RESULT resBandwidth = m_ppRateDescription[i]->GetAvgRate(ulCurRate);

	    if (SUCCEEDED(resBandwidth))
	    {
		UINT32 ulCurDelta = 0;
		if (ulCurRate > ulAvgRate)
		    ulCurDelta = ulCurRate - ulAvgRate;
		else
		    ulCurDelta = ulAvgRate - ulCurRate;

		if (ulCurDelta < ulMinDelta)
		{	
		    ulMinDelta = ulCurDelta;
		    pClosestRateDesc = m_ppRateDescription[i];
		}

		res = HXR_OK;
	    }
	}
    }

    // Return matching rate, if found
    if (SUCCEEDED(res))
    {
	pRateDesc = pClosestRateDesc;
	pRateDesc->AddRef();
    }

    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::GetRateDescription
// Purpose:
//  Gets the pRateDesc associated with ulIndex
STDMETHODIMP
CRateDescriptionMgr::GetRateDescription(UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc)
{
    HX_RESULT res = HXR_FAIL;

    // Validate state
    if (!m_ppRateDescription)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Return the nth rate description
    if (ulIndex < m_ulNumRateDescriptions && m_ppRateDescription[ulIndex])
    {
	pRateDesc = m_ppRateDescription[ulIndex];
	pRateDesc->AddRef();
	res = HXR_OK;
    }

    else
    {
	res = HXR_FAIL;
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::GetNumSwitchableRateDescriptions
// Purpose:
//  Gets the number of switchable rate descriptions
STDMETHODIMP_(UINT32)
CRateDescriptionMgr::GetNumSwitchableRateDescriptions()
{
    // Validate state
    if (!m_ppRateDescription)
    {
	HX_ASSERT(FALSE);
	return 0;
    }

    UINT32 ulActiveCount = 0;
    for (UINT32 i=0; i < m_ulNumRateDescriptions; i++)
    {
	if (m_ppRateDescription[i]->IsSwitchable())
	{
	    ulActiveCount++;
	}
    }

    return ulActiveCount;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::GetSwitchableRateDescription
// Purpose:
//  Gets the pRateDesc associated with ulIndex
STDMETHODIMP
CRateDescriptionMgr::GetSwitchableRateDescription(UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc)
{
    HX_RESULT res = HXR_FAIL;

    // Validate state
    if (!m_ppRateDescription || m_ulNumRateDescriptions == 0)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    UINT32 ulActiveCount = 0;
    for (UINT32 i=0; i < m_ulNumRateDescriptions; i++)
    {
	if (m_ppRateDescription[i]->IsSwitchable())
	{
	    if (ulActiveCount == ulIndex)
	    {
		pRateDesc = m_ppRateDescription[i];
		(m_ppRateDescription[i])->AddRef();
		res = HXR_OK;
		break;
	    }

	    ulActiveCount++;
	}
    }

    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::GetNumSelectableRateDescriptions
// Purpose:
//  Gets the pRateDesc associated with ulIndex
STDMETHODIMP_(UINT32)
CRateDescriptionMgr::GetNumSelectableRateDescriptions()
{
    // Validate state
    if (!m_ppRateDescription)
    {
	HX_ASSERT(FALSE);
	return 0;
    }

    UINT32 ulActiveCount = 0;
    for (UINT32 i=0; i < m_ulNumRateDescriptions; i++)
    {
	if (m_ppRateDescription[i]->IsSelectable())
	{
	    ulActiveCount++;
	}
    }

    return ulActiveCount;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::GetSelectableRateDescription
// Purpose:
//  Gets the pRateDesc associated with ulIndex
STDMETHODIMP
CRateDescriptionMgr::GetSelectableRateDescription(UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc)
{
    HX_RESULT res = HXR_FAIL;

    // Validate state
    if (!m_ppRateDescription || m_ulNumRateDescriptions == 0)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    UINT32 ulActiveCount = 0;
    for (UINT32 i=0; i < m_ulNumRateDescriptions; i++)
    {
	if (m_ppRateDescription[i]->IsSelectable())
	{
	    if (ulActiveCount == ulIndex)
	    {
		pRateDesc = m_ppRateDescription[i];
		(m_ppRateDescription[i])->AddRef();
		res = HXR_OK;
		break;
	    }

	    ulActiveCount++;
	}
    }

    return res;
}
/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::GetNextSwitchableRateDesc
// Purpose:
//  Returns the next switchable stream 
HX_RESULT
CRateDescriptionMgr::GetNextSwitchableRateDesc(REF(IHXRateDescription*)pRateDesc)
{
    // Validate state -- must have already set rate desc index using SetCurrentRateDesc
    if (m_ulCurRateDescriptionIndex == kulInvalidRateDescIndex)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    for (UINT32 ulIndex = m_ulCurRateDescriptionIndex + 1; ulIndex < m_ulNumRateDescriptions; ulIndex++)
    {	
	if (m_ppRateDescription[ulIndex]->IsSwitchable())
	{
	    pRateDesc = m_ppRateDescription[ulIndex];
	    pRateDesc->AddRef();
	    return HXR_OK;
	}
    }

    return HXR_FAIL;    
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::GetPrevSwitchableRateDesc
// Purpose:
//  Returns the previous switchable stream
HX_RESULT
CRateDescriptionMgr::GetPrevSwitchableRateDesc(REF(IHXRateDescription*)pRateDesc)
{
    // Validate state -- must have already set rate desc index using SetCurrentRateDesc
    if (m_ulCurRateDescriptionIndex == kulInvalidRateDescIndex)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    for (INT32 lIndex = m_ulCurRateDescriptionIndex - 1; lIndex >= 0; lIndex--)
    {	
	if (m_ppRateDescription[lIndex]->IsSwitchable())
	{
	    pRateDesc = m_ppRateDescription[lIndex];
	    pRateDesc->AddRef();
	    return HXR_OK;
	}
    }
    return HXR_FAIL;    
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::GetRateDescIndex
// Purpose:
//  Given a rate description, finds the matching index
HX_RESULT
CRateDescriptionMgr::GetRateDescIndex(IHXRateDescription* pRateDescription, REF(UINT32)ulIndex)
{
    HX_RESULT res = HXR_FAIL;

    for (UINT32 i = 0; i < m_ulNumRateDescriptions; i++)
    {
	if (m_ppRateDescription[i] == pRateDescription)
	{
	    ulIndex = i;
	    res = HXR_OK;
	    break;
	}
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::GetCurrentRateDesc
// Purpose:
//  Gets the current stream
STDMETHODIMP
CRateDescriptionMgr::GetCurrentRateDesc(REF(IHXRateDescription*)pRateDesc)
{    
    CRateDescription* pRateDescription = GetCurrentRateDescription();
    if (pRateDescription)
    {
	pRateDesc = pRateDescription;
	return HXR_OK;	
    }
    else
    {
	return HXR_FAIL;
    }    
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CRateDescriptionMgr::GetRateDescription
// Purpose:
//  Returns the CRateDescription at ulIndex
CRateDescription* 
CRateDescriptionMgr::GetRateDescription(UINT32 ulIndex)
{
    // Validate state
    if (!m_ppRateDescription || ulIndex>= m_ulNumRateDescriptions || !m_ppRateDescription[ulIndex])
    {
	HX_ASSERT(FALSE);
	return NULL;
    }

    // XXXLY - get rid of this
    m_ppRateDescription[ulIndex]->AddRef();
    return (CRateDescription*)m_ppRateDescription[ulIndex];
}

void
CRateDescriptionMgr::Dump()
{
    if (m_ppRateDescription)
    {
    	for (UINT32 i = 0; i < m_ulNumRateDescriptions; i++)
    	{
    	    printf("   Step %2u: ", i);
	    // XXXLY - Get rid of this
	    ((CRateDescription*)m_ppRateDescription[i])->Dump();
    	}    
    }    
}

CRateDescription*	
CRateDescriptionMgr::GetCurrentRateDescription()
{
    // Validate state -- must have already set rate desc index using SetCurrentRateDesc
    if (m_ulCurRateDescriptionIndex == kulInvalidRateDescIndex)
    {
	return NULL;
    }

    return GetRateDescription(m_ulCurRateDescriptionIndex);
}

