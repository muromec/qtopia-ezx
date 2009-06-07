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
#include "streamgroupmgr.h"

#include "hxccf.h"
#include "source.h"
#include "hxasm.h"
#include "hxmime.h"

#include "proc.h"
#include "hxassert.h"
#include "asmrulep.h"

static const UINT32 DEFAULT_PREROLL = 1000;

// Implements basic IUnknown functionality
BEGIN_INTERFACE_LIST(CSwitchGroupContainer)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXRateDescEnumerator)
END_INTERFACE_LIST


// Implements basic IUnknown functionality
BEGIN_INTERFACE_LIST(CStreamGroupManager)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXRateDescEnumerator)
END_INTERFACE_LIST




/////////////////////////////////////////////////////////////////////////
// Method:
//  CSwitchGroupContainer::CSwitchGroupContainer
// Purpose:
//  Constructor
CSwitchGroupContainer::CSwitchGroupContainer():
    m_bPending(FALSE)
    ,m_pbPendingRule(NULL)
    ,m_pbSubscribedRule(NULL)
    ,m_bDoneProcessingRulebook(FALSE)
    ,m_pASMSource(NULL)
{
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CSwitchGroupContainer::~CSwitchGroupContainer
// Purpose:
//  Destructor
CSwitchGroupContainer::~CSwitchGroupContainer()
{
    HX_RELEASE(m_pASMSource);

    Cleanup();
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CSwitchGroupContainer::isDuplicate
// Purpose:
//  Determines if pRateDescription is a duplicate of an existing physical stream
BOOL
CSwitchGroupContainer::isDuplicate(CRateDescription* pRateDescription, REF(UINT32)ulIndex)
{
    CPhysicalStream* pPhysicalStream = (CPhysicalStream*)pRateDescription;

    BOOL bIsDuplicate = FALSE;
    for (UINT32 i=0; i<GetNumPhysicalStreams() && !bIsDuplicate; i++)
    {
	CPhysicalStream* pCurPhysicalStream = GetPhysicalStream(i);

	if (pCurPhysicalStream->GetBandwidth() == pPhysicalStream->GetBandwidth())
	{
	    ulIndex = i;
	    bIsDuplicate = TRUE;
	}
	else if (pCurPhysicalStream->IsEntriesSame(pPhysicalStream))
	{
	    ulIndex = i;
	    bIsDuplicate = TRUE;
	}

	HX_RELEASE(pCurPhysicalStream);
    }

    return bIsDuplicate;        
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CSwitchGroupContainer::shouldSwap
// Purpose:
//  Replaces entry at ulIndex with pPhysicalStream, if pPhysicalStream is more descriptive
BOOL
CSwitchGroupContainer::shouldSwap(UINT32 ulIndex, CRateDescription* pRateDescription)
{
    BOOL bShouldSwap = FALSE;

    CPhysicalStream* pPhysicalStream = (CPhysicalStream*)pRateDescription;
    CPhysicalStream* pCurPhysicalStream = GetPhysicalStream(ulIndex);

    // Use bandwidth and number of rules as the tiebreaker
    if (pCurPhysicalStream->GetBandwidth() == 0 && pPhysicalStream->GetBandwidth() > 0)
    {
	// pick the one with non-zero bandwidth
	bShouldSwap = TRUE;
    }
    else if (pCurPhysicalStream->GetNumRules() < pPhysicalStream->GetNumRules())
    {
	// pick the one with more allocations :)
	bShouldSwap = TRUE;
    }    

    HX_RELEASE(pCurPhysicalStream);
    return bShouldSwap;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CSwitchGroupContainer::GrowRateDescContainer
// Purpose:
//  Creates CPhysicalStream/pending rule containers
HX_RESULT
CSwitchGroupContainer::GrowRateDescContainer(UINT32 ulMaxRateDescriptions)
{
    HX_RESULT res = HXR_OK;

    // Validate state -- can't add rate descriptions after rulebook processing is complete
    if (m_bDoneProcessingRulebook)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Resize pending rules array, if necessary
    if (ulMaxRateDescriptions > GetMaxRateDescriptions())
    {
	// If the container is being resized (beyond the initial allocation), make the
	// new container a bit larger than necessary to avoid continual resizing
	if (GetNumPhysicalStreams() > 0)
	    ulMaxRateDescriptions += 5;

	// Update rule pending array
	if (SUCCEEDED(res))
	{
	    HX_VECTOR_DELETE(m_pbPendingRule);
	    m_pbPendingRule = new BOOL[ulMaxRateDescriptions];
	    
	    if (!m_pbPendingRule)
		res = HXR_OUTOFMEMORY;
	    else
		memset(m_pbPendingRule, 0, sizeof(BOOL)*ulMaxRateDescriptions);
	}

	// Update subscribed rule array
	if (SUCCEEDED(res))
	{
	    HX_VECTOR_DELETE(m_pbSubscribedRule);
	    m_pbSubscribedRule = new BOOL[ulMaxRateDescriptions];

	    if (!m_pbSubscribedRule)
		res = HXR_OUTOFMEMORY;
	    else
		memset(m_pbSubscribedRule, 0, sizeof(BOOL)*ulMaxRateDescriptions);
	}
    }

    if (SUCCEEDED(res))
	res = CRateDescriptionMgr::GrowRateDescContainer(ulMaxRateDescriptions);

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CSwitchGroupContainer::Cleanup
// Purpose:
//  Cleans up pending rules array
HX_RESULT
CSwitchGroupContainer::Cleanup()
{
    HX_RESULT res = HXR_OK;

    HX_VECTOR_DELETE(m_pbPendingRule);
    HX_VECTOR_DELETE(m_pbSubscribedRule);

    return CRateDescriptionMgr::Cleanup();
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CSwitchGroupContainer::ReplaceRateDescription
// Purpose:
//  Sets pRateDescription into the CRateDescription container, potentially
//  replacing an existing entry
HX_RESULT
CSwitchGroupContainer::ReplaceRateDescription(UINT32 ulIndex, CRateDescription* pRateDescription)
{    
    HX_RESULT res = HXR_OK;

    // Validate state -- can't add rate descriptions after rulebook processing is complete
    if (m_bDoneProcessingRulebook)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    res = CRateDescriptionMgr::ReplaceRateDescription(ulIndex, pRateDescription);

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CSwitchGroupContainer::InsertRateDescription
// Purpose:
//  Inserts pRateDescription into the CRateDescription container, at ulIndex,
//  moving the existing entries (ulIndex, m_ulNumRateDescriptions-1) inclusively up one
// Notes:
//  Must ensure that m_ppRateDescription remains a contiguous array
HX_RESULT
CSwitchGroupContainer::InsertRateDescription(UINT32 ulInsertIndex, CRateDescription* pRateDescription)
{    
    HX_RESULT res = HXR_OK;

    // Validate state -- can't add rate descriptions after rulebook processing is complete
    if (m_bDoneProcessingRulebook)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    return CRateDescriptionMgr::InsertRateDescription(ulInsertIndex, pRateDescription);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CSwitchGroupContainer::SetASMSource
// Purpose:
//  Used to set the ASM source
HX_RESULT
CSwitchGroupContainer::SetASMSource(IHXASMSource* pASMSource)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!pASMSource)
    {
	HX_ASSERT(FALSE);
	return HXR_POINTER;
    }

    HX_RELEASE(m_pASMSource);

    m_pASMSource = pASMSource;
    m_pASMSource->AddRef();

    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CSwitchGroupContainer::HandlePacket
// Purpose:
//  Clears packet pending flag for a given rule number
HX_RESULT 
CSwitchGroupContainer::HandlePacket(UINT32 unRuleNum)
{
    HX_ASSERT(unRuleNum < GetMaxRules());
    
    if (m_bPending && m_pbPendingRule[unRuleNum])
    {
	m_bPending = FALSE;
	memset(m_pbPendingRule, FALSE, sizeof(BOOL)*GetMaxRules());	
    }
    return HXR_OK;
}

BOOL
CSwitchGroupContainer::IsPending()
{
    return m_bPending;
}

void
CSwitchGroupContainer::DumpActivation()
{
    printf("rules: ");
    if (m_pbPendingRule)
    {
    	for (UINT32 i = 0; i < GetMaxRules(); i++)
    	{
	    printf("%6u ", m_pbPendingRule[i]);
	}
	printf("\n");
    }
    else
    {
	printf(" unknown\n");
    }
    fflush(0);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CSwitchGroupContainer::Subscribe
// Purpose:
//  Subscribes to a logical stream's rule.  Updates subscribed
//  rule array.  Updates current rate desc.
HX_RESULT
CSwitchGroupContainer::Subscribe(UINT32 ulStreamNum, UINT32 ulRuleNum)
{
    HX_RESULT res = HXR_OK;

    // Validate state, params
    if (!m_pASMSource || ulRuleNum >= GetMaxRules() || ulStreamNum==kulInvalidLogicalStreamNum)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Sanity check
    HX_ASSERT(m_pbSubscribedRule[ulRuleNum] == FALSE);

    // Subscribe to the rule 
    if (SUCCEEDED(res))    
        res = m_pASMSource->Subscribe((UINT16)ulStreamNum, (UINT16)ulRuleNum);

    // Update subscribed rule array
    if (SUCCEEDED(res))
	m_pbSubscribedRule[ulRuleNum] = TRUE;

    // Update the current rate desc -- OK if it fails (expected behavior until all rules
    // contained by a rate description have been subscribed to)
    if (SUCCEEDED(res))
    {
	HX_RESULT resUpdate = UpdateCurrentRateDesc();
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CSwitchGroupContainer::Unsubscribe
// Purpose:
//  Unsubscribes from a logical stream's rule
HX_RESULT
CSwitchGroupContainer::Unsubscribe(UINT32 ulStreamNum, UINT32 ulRuleNum)
{
    HX_RESULT res = HXR_OK;

    // Validate state, params
    if (!m_pASMSource || ulRuleNum >= GetMaxRules() || ulStreamNum==kulInvalidLogicalStreamNum)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Sanity check
    HX_ASSERT(m_pbSubscribedRule[ulRuleNum] == TRUE);

    // Unubscribe from the rule 
    if (SUCCEEDED(res))
	res = m_pASMSource->Unsubscribe((UINT16)ulStreamNum, (UINT16)ulRuleNum);

    // Update subscribed rule array
    if (SUCCEEDED(res))
	m_pbSubscribedRule[ulRuleNum] = FALSE;

    // Update the current rate desc -- OK if it fails (expected behavior until all rules
    // contained by a rate description have been subscribed to)
    if (SUCCEEDED(res))
    {
	HX_RESULT resUpdate = UpdateCurrentRateDesc();
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CSwitchGroupContainer::DoneProcessingRulebook
// Purpose:
//  Signals that rulebook processing is complete
HX_RESULT
CSwitchGroupContainer::DoneProcessingRulebook()
{
    HX_RESULT res = HXR_OK;

    HX_ASSERT(!m_bDoneProcessingRulebook);
    m_bDoneProcessingRulebook = TRUE;
    
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CSwitchGroupContainer::UpdateCurrentRateDesc
// Purpose:
//  Updates the current rate description.  Also updates the shift pending status
HX_RESULT
CSwitchGroupContainer::UpdateCurrentRateDesc()
{
    HX_RESULT res = HXR_OK;

    // Search for a rate description that matches the current subscribed rule array
    CPhysicalStream* pNewPhysStream = NULL;
    for (UINT32 i=0; i<GetNumPhysicalStreams(); i++)
    {
	CPhysicalStream* pPhysicalStream = GetPhysicalStream(i);

	UINT32 ulNumRulesMatched = 0;
	UINT32* aulStreamRuleArray = pPhysicalStream->GetRuleArray();
	for (UINT32 j=0; j<pPhysicalStream->GetNumRules(); j++)
	{
	    if (m_pbSubscribedRule[aulStreamRuleArray[j]])
		ulNumRulesMatched++;
	    else
		break;
	}

	// If the subscribed rule array contains all of the rules associated with
	// the rate description, and the rate description and subscribed rule array have
	// the same number of rules, a matching rate description has been found
	if (ulNumRulesMatched == pPhysicalStream->GetNumRules())
	{
	    UINT32 ulNumRulesSubscribed = 0;
	    for (UINT32 k=0; k<GetMaxRules(); k++)
	    {
		if (m_pbSubscribedRule[k])
		    ulNumRulesSubscribed++;
	    }

	    if (ulNumRulesMatched == ulNumRulesSubscribed)
	    {
		pNewPhysStream = pPhysicalStream;
		pNewPhysStream->AddRef();

		HX_RELEASE(pPhysicalStream);
		break;
	    }
	}

	HX_RELEASE(pPhysicalStream);
    }

    // Return HXR_IGNORE if the subscribed rule array doesn't match any physical stream
    if (pNewPhysStream == NULL)
	res = HXR_IGNORE;

    // Set shift pending flag -- only want this if rate description has changed
    if (SUCCEEDED(res))
    {
	// OK if current stream is NULL
	CPhysicalStream* pCurrentPhysStream = GetCurrentPhysicalStream();

	if (pCurrentPhysStream != pNewPhysStream)
	{
	    m_bPending = TRUE;
	    memcpy(m_pbPendingRule, m_pbSubscribedRule, GetMaxRules()*sizeof(BOOL));	    
	}

	HX_RELEASE(pCurrentPhysStream);
    }

    // Update current rate desc
    UINT32 ulIndex = kulInvalidRateDescIndex;
    if (SUCCEEDED(res))
	res = GetRateDescIndex(pNewPhysStream, ulIndex);

    // Set the current rate description (OK to set to invalid index, if that's actually the case)
    SetCurrentRateDesc(ulIndex);

    HX_RELEASE(pNewPhysStream);
    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupContainer::isDuplicate
// Purpose:
//  Determines if pRateDescription is a duplicate of an existing physical stream
BOOL
CStreamGroupContainer::isDuplicate(CRateDescription* pRateDescription, REF(UINT32)ulIndex)
{
    CPhysicalStream* pPhysicalStream = (CPhysicalStream*)pRateDescription;

    BOOL bIsDuplicate = FALSE;
    for (UINT32 i=0; i<GetNumPhysicalStreams() && !bIsDuplicate; i++)
    {
	CPhysicalStream* pCurPhysicalStream = GetPhysicalStream(i);

	// Note: Requires that all physical streams in the same switch group
	// have a different bandwidth
	if (pCurPhysicalStream->GetSwitchGroupID() == pPhysicalStream->GetSwitchGroupID())
	{
	    if (pCurPhysicalStream->GetBandwidth() == pPhysicalStream->GetBandwidth())
	    {
		ulIndex = i;
		bIsDuplicate = TRUE;
	    }
	    else if (pCurPhysicalStream->IsEntriesSame(pPhysicalStream))
	    {
		ulIndex = i;
		bIsDuplicate = TRUE;
	    }
	}

	HX_RELEASE(pCurPhysicalStream);
    }

    return bIsDuplicate;        
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupContainer::shouldSwap
// Purpose:
//  Replaces entry at ulIndex with pPhysicalStream, if pPhysicalStream is more descriptive
BOOL
CStreamGroupContainer::shouldSwap(UINT32 ulIndex, CRateDescription* pRateDescription)
{
    BOOL bShouldSwap = FALSE;

    CPhysicalStream* pPhysicalStream = (CPhysicalStream*)pRateDescription;
    CPhysicalStream* pCurPhysicalStream = GetPhysicalStream(ulIndex);

    if (pCurPhysicalStream->GetBandwidth() == 0 && pPhysicalStream->GetBandwidth() > 0)
    {
	// pick the one with non-zero bandwidth
	bShouldSwap = TRUE;
    }
    else if (pCurPhysicalStream->GetNumRules() < pPhysicalStream->GetNumRules())
    {
	// pick the one with more allocations :)
	bShouldSwap = TRUE;
    }    

    HX_RELEASE(pCurPhysicalStream);
    return bShouldSwap;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::CStreamGroupManager
// Purpose:
//  Constructor
CStreamGroupManager::CStreamGroupManager(IHXCommonClassFactory* pCCF):
    m_bCheckAverageBandwidth(FALSE)
    ,m_pCCF(pCCF)
    ,m_pASMSource(NULL)
    ,m_pMimeType(NULL)
    ,m_ulStreamGroupNum(kulInvalidStreamGroupNum)
    ,m_bInitialRateDescCommitted(FALSE)
    ,m_ulNumLogicalStreams(0)
    ,m_ulMaxNumLogicalStreams(0)
    ,m_ulNumSwitchGroups(0)
    ,m_ppLogicalStream(NULL)
    ,m_ppSwitchGroup(NULL)
    ,m_ulSelectedLogicalStream(kulInvalidLogicalStreamNum)
    ,m_pRateDescResp(NULL)

{
    HX_ASSERT(m_pCCF);
    if (m_pCCF)
    {
	m_pCCF->AddRef();
    }
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::~CStreamGroupManager
// Purpose:
//  Destructor
CStreamGroupManager::~CStreamGroupManager()
{
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pASMSource);
    HX_RELEASE(m_pMimeType);
    HX_RELEASE(m_pRateDescResp);

    if (m_ppSwitchGroup)
    {
	for (UINT32 i=0; i<m_ulNumSwitchGroups; i++)
	{
	    HX_RELEASE(m_ppSwitchGroup[i]);
	}

	HX_VECTOR_DELETE(m_ppSwitchGroup);
    }

    // All elements' references released above from m_ppSwitchGroup
    HX_VECTOR_DELETE(m_ppLogicalStream);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::Init
// Purpose:
//  Creates CPhysicalStream/pending rule containers
HX_RESULT
CStreamGroupManager::Init(UINT32 ulStreamGroupNum, UINT32 ulMaxNumLogicalStreams, BOOL bCheckAverageBandwidth)
{
    HX_RESULT res = HXR_OK;

    // Validate state
    if (m_ulStreamGroupNum != kulInvalidStreamGroupNum || !m_pCCF || 
        m_ulMaxNumLogicalStreams > 0 || ulMaxNumLogicalStreams == 0)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    m_ulStreamGroupNum = ulStreamGroupNum;
    m_ulMaxNumLogicalStreams = ulMaxNumLogicalStreams;
    m_bCheckAverageBandwidth = bCheckAverageBandwidth;

    if (SUCCEEDED(res))
    {
        m_ppLogicalStream = new CSwitchGroupContainer*[m_ulMaxNumLogicalStreams];
        m_ppSwitchGroup = new CSwitchGroupContainer*[m_ulMaxNumLogicalStreams];
        if (m_ppLogicalStream && m_ppSwitchGroup)
        {
            memset(m_ppLogicalStream, 0, sizeof(CSwitchGroupContainer*)*m_ulMaxNumLogicalStreams);
            memset(m_ppSwitchGroup, 0, sizeof(CSwitchGroupContainer*)*m_ulMaxNumLogicalStreams);
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::GetLogicalStream
// Purpose:
//  Get a logical stream enuerator
HX_RESULT
CStreamGroupManager::GetLogicalStream(UINT32 ulLogicalStreamNum, REF(IHXRateDescEnumerator*)pLogicalStreamEnum)
{
    HX_RESULT res = HXR_OK;

    // Validate state, params
    if (!m_ppLogicalStream || ulLogicalStreamNum >= m_ulMaxNumLogicalStreams || !m_ppLogicalStream[ulLogicalStreamNum])
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    pLogicalStreamEnum = m_ppLogicalStream[ulLogicalStreamNum];
    pLogicalStreamEnum->AddRef();

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::GetSelectedLogicalStreamNum
// Purpose:
//  Returns the selected logical stream number
HX_RESULT
CStreamGroupManager::GetSelectedLogicalStreamNum(REF(UINT32)ulSelectedLogicalStreamNum)
{
    HX_RESULT res = HXR_OK;

    // Validate state, params
    if (!m_bInitialRateDescCommitted || m_ulSelectedLogicalStream == kulInvalidLogicalStreamNum)
    {
	return HXR_FAIL;
    }

    ulSelectedLogicalStreamNum = m_ulSelectedLogicalStream;
    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::SetStreamHeader
// Purpose:
//  Creates stream group mgrs
HX_RESULT
CStreamGroupManager::SetStreamHeader(UINT32 ulLogicalStreamNum, 
                                     IHXValues* pStreamHeader)
{
    HX_RESULT res = HXR_OK;

    // Validate state, params
    if (m_ulStreamGroupNum == kulInvalidStreamGroupNum || !pStreamHeader 
        || !m_ppLogicalStream 
        || m_ulNumLogicalStreams >= m_ulMaxNumLogicalStreams)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    // Get stream number group number
    UINT32 ulStreamGroupNum = 0;
    res = pStreamHeader->GetPropertyULONG32("StreamGroupNumber", 
            ulStreamGroupNum);

    if (SUCCEEDED(res) && ulStreamGroupNum == m_ulStreamGroupNum)
    {
        UINT32 ulSwitchGroupID = kulInvalidSwitchGroupID;
        CSwitchGroupContainer* pSwitchGroup = NULL;

        // If SwitchGroupID is not set or set to 0 (kulInvalidSwitchGroupID)
        // then it's not switchable - so it's the only stream in the switch 
        // group
        pStreamHeader->GetPropertyULONG32("SwitchGroupID", ulSwitchGroupID);
        if (ulSwitchGroupID != kulInvalidSwitchGroupID)
        {
            pSwitchGroup = GetSwitchGroup(ulSwitchGroupID);
        }

        // If we've already parsed this switch group
        if (pSwitchGroup)
        {
            UINT32 ulNumStreams = pSwitchGroup->GetNumPhysicalStreams();
            CPhysicalStream* pPhysicalStream = NULL;
            for (UINT32 i = 0; i < ulNumStreams; i++)
            {
                pPhysicalStream = pSwitchGroup->GetPhysicalStream(i);
                if (pPhysicalStream)
                {
                    res = UpdatePhysicalStream(pPhysicalStream, 
                            ulLogicalStreamNum, pStreamHeader);
                    pPhysicalStream->Release();
                }
            }
        }

        // else this is a new switch group, parse the rule book
        else
        {
            // Get the logical stream rulebook -- must have one
            IHXBuffer* pRuleBook = NULL;
            ASMRuleBook* pRuleBookParser = NULL;
            UINT32 ulNumRules = 0;

            res = pStreamHeader->GetPropertyCString("ASMRuleBook", pRuleBook);

            // Process the rulebook -- extract physical stream info
            if (SUCCEEDED(res))
            {
                pRuleBookParser = 
                    new ASMRuleBook((const char*)pRuleBook->GetBuffer());

                if (!pRuleBookParser)
                {
                    res = HXR_OUTOFMEMORY;
                }
            }

            if (SUCCEEDED(res))
            {
                // Create/init switch group container
                pSwitchGroup = new CSwitchGroupContainer();

                if (pSwitchGroup)
                {
                    pSwitchGroup->AddRef();
                    res = pSwitchGroup->Init(pRuleBookParser->GetNumRules());
                }
                else
                {
                    res = HXR_OUTOFMEMORY;
                }
            }

            // Init stream group container -- may handle multiple stream headers
            if (SUCCEEDED(res))
            {
                ulNumRules = pRuleBookParser->GetNumRules();
                res = CStreamGroupContainer::GrowRateDescContainer(ulNumRules);             
            }

            // Process the rulebook
            if (SUCCEEDED(res))
            {
                pSwitchGroup->SetSwitchGroupID(ulSwitchGroupID);

                res = ProcessRulebook(pRuleBookParser, ulLogicalStreamNum, 
                                        pStreamHeader, pSwitchGroup);

                // OK if no new streams were added to the stream group
                if (res == HXR_IGNORE)
                    res = HXR_OK;
            }

            if (SUCCEEDED(res))
            {
                if (m_pASMSource)
                {
                    pSwitchGroup->SetASMSource(m_pASMSource);
                }
                pSwitchGroup->DoneProcessingRulebook();

                // Store this switch group
                m_ppSwitchGroup[m_ulNumSwitchGroups] = pSwitchGroup;
                m_ulNumSwitchGroups++;
            }
            else
            {
                HX_RELEASE(pSwitchGroup);
            }

            HX_RELEASE(pRuleBook);
            HX_DELETE(pRuleBookParser);
        }

        // Store the switch group in the logical stream array
        // Allows for efficient access by logical stream number
        if (SUCCEEDED(res))
        {
            m_ppLogicalStream[ulLogicalStreamNum] = pSwitchGroup;
            m_ulNumLogicalStreams++;
        }
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::ProcessRulebook
// Purpose:
//  Processes a logical stream's rulebook
// Returns:
//  HXR_OK if there was at least one new valid physical stream.  
//  HXR_IGNORE if there were no new valid physical streams.
//  Otherwise, HXR_FAIL.
HX_RESULT
CStreamGroupManager::ProcessRulebook(ASMRuleBook* pRuleBook, 
                                     UINT32 ulLogicalStreamNum,
                                     IHXValues* pStreamHeader, 
                                     CSwitchGroupContainer* pSwitchGroup)
{
    HX_RESULT res = HXR_OK;

    // Determine if RealVideo
    BOOL bIsRealVideo = FALSE;
    if (SUCCEEDED(res))
    {
	HX_RESULT resMimeType = pStreamHeader->GetPropertyCString("MimeType", m_pMimeType);

	if (SUCCEEDED(resMimeType) && 
	    (strcasecmp((const char*) m_pMimeType->GetBuffer(), REALVIDEO_MIME_TYPE) == 0 || 
	    strcasecmp((const char*) m_pMimeType->GetBuffer(), REALVIDEO_MULTIRATE_MIME_TYPE) == 0))
	{
	    bIsRealVideo = TRUE;
	}
    }

    // Determine number of bandwidth thresholds
    UINT32 ulNumThreshold = 0;
    float* pThresholds = NULL;
    UINT32 ulSwitchGroupID = 0;
    UINT32 ulStreamNum = 0;

    if (SUCCEEDED(res))
    {
	res = CAsmRuleBookParser::GetThresholdInfo(pRuleBook, m_pCCF, ulNumThreshold, pThresholds);
    //    printf("Strm: %u Thresholds %u:", ulStreamNo, ulNumThreshold);fflush(0);
    }

    // Create a physical stream for each threshold bandwidth
    for (UINT32 i = 0; i < ulNumThreshold && SUCCEEDED(res); i++)
    {
	// Handle case of zero bandwidth
	if ((UINT32)pThresholds[i] == 0)
	{
	    // If there is more than one threshold, just skip the current threshold (will be duplicate 
	    // of some other stream)
	    if (ulNumThreshold > 1)
	    {
		continue;
	    }

	    // Otherwise, if there is only a single threshold, try to get the avg bitrate
	    // from the logical stream header
	    else
	    {
                // If no AvgBitRate available with only one threshold, 
                // that's okay just stick with 0.
                // Only happens for live RTP streams with no bandwidth
                // info. Needs further consideration for MDP case.
		UINT32 ulAvgRate = 0;
		pStreamHeader->GetPropertyULONG32("AvgBitRate", ulAvgRate);

                pThresholds[i] = (float)ulAvgRate;
	    }
	}

	// Create physical stream for the given bandwidth (if one doesn't already exist)
	CPhysicalStream* pPhysicalStream = NULL;
	if (SUCCEEDED(res))
	{
	    pPhysicalStream = new CPhysicalStream((UINT32)pThresholds[i]);	    
	    if (pPhysicalStream)
		pPhysicalStream->AddRef();
	    else
		res = HXR_OUTOFMEMORY;

            pPhysicalStream->m_ulSwitchGroupID = pSwitchGroup->GetSwitchGroupID();

	    // Determine the ASM rules associated with the current bandwidth threshold
	    if (SUCCEEDED(res))
	    {
		res = GetPhysicalStreamRules(pRuleBook, pPhysicalStream->m_ulAvgRate, pPhysicalStream->m_ulNumRules,
		        pPhysicalStream->m_aulRule);
	    }

	    if (SUCCEEDED(res) && bIsRealVideo)
            {
		res = HandleRVThinningStream(pPhysicalStream, pRuleBook, pThresholds, 
                        ulNumThreshold, i);
            }
	    // Sets default maxrate/preroll/predata values, other stream info
	    if (SUCCEEDED(res))
            {
		res = InitPhysicalStream(pPhysicalStream, ulLogicalStreamNum, pStreamHeader);
            }
	}

	// Add the stream to the LogicalStream container
	if (SUCCEEDED(res))
	{
            res = pSwitchGroup->AddPhysicalStream(pPhysicalStream);

	    // Ok if not added
	    if (res == HXR_IGNORE)
		res = HXR_OK;
	}

	// Add the physical stream to the StreamGroup container
	if (SUCCEEDED(res))
	{
	    res = AddPhysicalStream(pPhysicalStream);

	    // Ok if not added
	    if (res == HXR_IGNORE)
		res = HXR_OK;
	}

	HX_RELEASE(pPhysicalStream);
    }	

    //    printf("\n");
    HX_VECTOR_DELETE(pThresholds);

    if (SUCCEEDED(res) && GetNumPhysicalStreams() == 0)
	res = HXR_IGNORE;

    HX_ASSERT(SUCCEEDED(res) || res == HXR_IGNORE);
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::HandleRVThinningStream
// Purpose:
//  Processes a logical stream's rulebook
HX_RESULT
CStreamGroupManager::HandleRVThinningStream(CPhysicalStream* pPhysicalStream, ASMRuleBook* pRuleBook, float* pThresholds, UINT32 ulNumThreshold, UINT32 ulCurThresholdIndex)
{
    HX_RESULT res = HXR_OK;

    // If there is exactly one rule associated with the physical stream, and the rule is
    // has a TimeStampDelivery directive, it is the keyframe-only stream
    // Note: See CLogicalStreamManager::TagThinningStreams (multimgr.cpp) for additional info
    BOOL bIsThinningStream = FALSE;
    if (pPhysicalStream->GetNumRules() == 1)
    {
	IHXValues* pProps = NULL;
	HX_RESULT resProps = pRuleBook->GetProperties((UINT16)pPhysicalStream->GetRuleArray()[0], pProps);

	if (SUCCEEDED(resProps))
	{
	    IHXBuffer* pBuffer = NULL;
	    resProps = pProps->GetPropertyCString("TimeStampDelivery", pBuffer);

	    if (SUCCEEDED(resProps))
		bIsThinningStream = TRUE;

	    HX_RELEASE(pBuffer);
	}
	HX_RELEASE(pProps);
    }

    // If the physical stream is a thinning stream, tag it, and update bitrate
    if (bIsThinningStream)
    {
	pPhysicalStream->m_bIsThinningStream = TRUE;

	// Note: A lot of thinning streams don't have avg bandwidth directives, so just make a 
	// rough guesstimate -- take 40% of the bitrate of the full stream
	if (pPhysicalStream->m_ulAvgRate == 1 
	    && ulNumThreshold > 2 && ulCurThresholdIndex < ulNumThreshold - 1)
	{
	    pPhysicalStream->m_ulAvgRate = (UINT32)(pThresholds[ulCurThresholdIndex+1] * .4);
	}		   
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::GetPhysicalStreamRules
// Purpose:
//  Populates pRules array with the rule numbers associated with ulBandwidth
HX_RESULT 
CStreamGroupManager::GetPhysicalStreamRules(ASMRuleBook* pRuleBook, UINT32 ulBandwidth, REF(UINT32)unCount, REF(UINT32*)pRules)
{
    // Validate state
    if (!pRuleBook)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    HX_RESULT theErr = HXR_FAIL;
    BOOL*      pSubs = NULL;

    // Determine which rules will be subscribed to for the specified bandwidth
    UINT32 ulNumRules = 0;
    if (ulNumRules = pRuleBook->GetNumRules())
    {
	theErr = CAsmRuleBookParser::GetRuleSubscriptions(pRuleBook, m_pCCF, ulBandwidth, pSubs);
    }

    //    printf("stream: %u\n", ulStreamNo);
    IHXValues* pVal = NULL;
    IHXBuffer* pBuf = NULL;

    if (HXR_OK == theErr)
    {
	// unCount shall never be > ulNumRules
	unCount = 0;
	pRules = new UINT32[ulNumRules];
	if (pRules)
	{
	    memset(pRules, 0, sizeof(UINT32)*ulNumRules);
	}
	else
	{
	    theErr = HXR_OUTOFMEMORY;
	}
    }

    // Update the rules array with the rule numbers that are associated 
    // with the given bandwidth.
    for (UINT32 i = 0; HXR_OK == theErr && i < ulNumRules; i++)
    {
	if (pSubs[i])
	{
	    theErr = pRuleBook->GetProperties((UINT16)i, pVal);
	    if (HXR_OK == theErr)
	    {
#if 0
		// XXXLY -- Remove this if not needed from rmevents code (don't know yet)
		if (m_bCheckAverageBandwidth)
		{		    
		    theErr = pVal->GetPropertyCString("AverageBandwidth", pBuf);
		    if (HXR_OK == theErr)
		    {
			pRules[unCount++] = i;

			//		    printf("\trule: %u AverageBandwidth: %u\n", i, 
			//			atoi((char*)pBuf->GetBuffer()));fflush(0);
			HX_RELEASE(pBuf);			
		    }
		    else
		    {
			// fine keep going
			theErr = HXR_OK;
		    }
		}
		else
#endif
		{
		    pRules[unCount++] = i;		    
		}
	    }            
	    HX_RELEASE(pVal);
	}
    }
    HX_VECTOR_DELETE(pSubs);

    if (HXR_OK == theErr)
    {
	if (unCount)
	{
	    return HXR_OK;
	}
	else
	{
	    theErr = HXR_IGNORE;
	}
    }

    // failed...cleana up   
    HX_VECTOR_DELETE(pRules);	
    return theErr;    
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupContainer::HandleDefaults
// Purpose:
//  Populates pPhysicalStream with default properties from the stream header
HX_RESULT
CStreamGroupManager::HandleDefaults(CPhysicalStream* pPhysicalStream, 
                                    IHXValues* pStreamHeader,
                                    BOOL bDefaultStream)
{
    // Set preroll default, if necessary
    UINT32 ulTemp = 0;
    if (SUCCEEDED(pStreamHeader->GetPropertyULONG32("ServerPreroll", 
        ulTemp)))
    {
        pPhysicalStream->m_ulPreroll = ulTemp;
    }
    else
    {
        // ServerPreroll should have been added already!!
        HX_ASSERT(FALSE);
        pPhysicalStream->m_ulPreroll = DEFAULT_PREROLL;
    }

    // Set max rate default, if necessary
    if (bDefaultStream &&
        SUCCEEDED(pStreamHeader->GetPropertyULONG32("MaxBitRate", ulTemp)))
    {
        // Only use the stream header's max rate for the master stream -- all other
        // streams are assumed to be CBR (per-stream maxrate info is not preserved in
        // the logical stream header)
        pPhysicalStream->m_ulMaxRate = ulTemp;
    }
    else
    {
        // use avg rate
        pPhysicalStream->m_ulMaxRate = pPhysicalStream->m_ulAvgRate;
    }
    
    // Set predata default, if necessary
    if (!pPhysicalStream->m_ulPredata &&
        SUCCEEDED(pStreamHeader->GetPropertyULONG32("Predata", ulTemp)))
    {
        pPhysicalStream->m_ulPredata = ulTemp;
    }
    else
    {
        // in bytes
        pPhysicalStream->m_ulPredata = pPhysicalStream->m_ulPreroll * 
            pPhysicalStream->m_ulMaxRate / 1000 / 8;
    }

    return HXR_OK;
}

HX_RESULT
CStreamGroupManager::InitPhysicalStream(CPhysicalStream* pPhysicalStream, 
                                        UINT32 ulLogicalStreamNum,
                                        IHXValues* pStreamHeader)
{
    pPhysicalStream->SetDefaultLogicalStream(ulLogicalStreamNum);

    return HandleDefaults(pPhysicalStream, pStreamHeader, 
            IsDefaultStream(pPhysicalStream, pStreamHeader));
}

HX_RESULT
CStreamGroupManager::UpdatePhysicalStream(CPhysicalStream* pPhysicalStream, 
                                          UINT32 ulLogicalStreamNum, 
                                          IHXValues* pStreamHeader)
{
    UINT32 ulLogicalStreamAvgRate = 0;

    // If this is the default physical stream for this logical stream, set
    // this logical stream as the default and update its default values
    if (IsDefaultStream(pPhysicalStream, pStreamHeader))
    {
        pPhysicalStream->SetDefaultLogicalStream(ulLogicalStreamNum);

        return HandleDefaults(pPhysicalStream, pStreamHeader, TRUE);
    }

    return HXR_OK;
}

BOOL 
CStreamGroupManager::IsDefaultStream(CPhysicalStream* pPhysicalStream, 
                                     IHXValues* pStreamHeader)
{
    UINT32 ulRuleNum = 0;
    UINT32 ulLogicalStreamAvgRate = 0;

    // If we have a default rule number, check if it indicates
    // this physical stream
    if (SUCCEEDED(pStreamHeader->GetPropertyULONG32("BaseRule", ulRuleNum)))
    {
        for (UINT32 j = 0; j < pPhysicalStream->m_ulNumRules; j++)
        {
            if (pPhysicalStream->m_aulRule[j] == ulRuleNum)
            {
                return TRUE;
            }
        }

        return FALSE;
    }

    // Otherwise check for a matching average bitrate
    if (SUCCEEDED(pStreamHeader->GetPropertyULONG32("AvgBitRate", 
            ulLogicalStreamAvgRate)) &&
            pPhysicalStream->m_ulAvgRate == ulLogicalStreamAvgRate)
    {
        return TRUE;
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CAsmRuleBookParser::GetThresholdInfo
// Purpose:
//  Given an ASM rulebook, determines the number of bandwidth threshholds
HX_RESULT 
CAsmRuleBookParser::GetThresholdInfo(ASMRuleBook* pRules, IHXCommonClassFactory* pCCF,
				 REF(UINT32) ulNumThreshold, REF(float*) pThresholds)
{
    // Validate params
    if (!pRules || !pCCF)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    pThresholds = NULL;

    IHXValues* pValues = NULL;
    IHXBuffer* pBuffer = NULL;
    HX_RESULT res = HXR_FAIL;
    
    res = pCCF->CreateInstance(CLSID_IHXValues, (void**)&pValues);
    if (HXR_OK == res)
    {
	res = pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
    }	

    if (HXR_OK == res)
    {
    	// we have to send "Bandwidth" to be 0 in pValues
    	UINT8 pBandwidth[128];
    	sprintf ((char *)pBandwidth, "%ld", 0);
    	
    	res = pBuffer->Set(pBandwidth, strlen((char *)pBandwidth) + 1);
    }
    
    if (HXR_OK == res)
    {
    	res = pValues->SetPropertyCString("Bandwidth",  pBuffer);
    }    	

    if (HXR_OK == res)
    {
	res = HXR_FAIL;
	
	// can't be more than number of rules.	
	ulNumThreshold = pRules->GetNumRules();
	if (ulNumThreshold)
	{
	    pThresholds = new float[ulNumThreshold];	    
	    if (pThresholds)
	    {
		res = HXR_OK;
		memset(pThresholds, 0, sizeof(float)*ulNumThreshold);
	    }
	}
    }
    
    if (HXR_OK == res)
    {
    	res = pRules->GetPreEvaluate(pThresholds, ulNumThreshold,
	    	pValues, "Bandwidth");
    }

    HX_RELEASE(pValues);
    HX_RELEASE(pBuffer);

    if (HXR_OK == res)
    {
	return HXR_OK;
    }
    else
    {
	HX_VECTOR_DELETE(pThresholds);
	return res;
    }
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CAsmRuleBookParser::GetRuleSubscriptions
// Purpose:
//  For a given bandwidth, determines which rules will be subscribed to
HX_RESULT
CAsmRuleBookParser::GetRuleSubscriptions(ASMRuleBook* pRuleBook, IHXCommonClassFactory *pCCF, UINT32 ulBandwidth, REF(BOOL*) pSubs)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!pRuleBook || !pCCF)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }
    
    if (HXR_OK == res)
    {
	if (pRuleBook->GetNumRules() > 0)
	{
	    pSubs = new BOOL[pRuleBook->GetNumRules()];
	}

	if (!pSubs)
	{
	    res = HXR_UNEXPECTED;
	}
    }

    IHXValues* pVars = NULL;
    IHXBuffer* pBuf = NULL;

    // Create bandwidth query string
    if (HXR_OK == res)
    {
	res = pCCF->CreateInstance(CLSID_IHXValues, (void**) &pVars);
    }
    if (HXR_OK == res)
    {
	res = pCCF->CreateInstance(CLSID_IHXBuffer, (void**) &pBuf);
    }
	
    if (HXR_OK == res)
    {
	char pc[32];	
	sprintf(pc, "%lu", ulBandwidth);
	res = pBuf->Set((const BYTE*)pc, strlen(pc)+1);	
    }        

    if (HXR_OK == res)
    {
	res = pVars->SetPropertyCString("Bandwidth", pBuf);
    }

    // Determines which rules will be subscribed to
    if (HXR_OK == res)
    {
	res = pRuleBook->GetSubscription(pSubs, pVars);
    }

    HX_RELEASE(pVars);
    HX_RELEASE(pBuf);
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::Switch
// Purpose:
//  Unsubscribes from the old physical stream and subscribes to the new physical stream
// Notes:
//  OK is pOld is NULL (skip unsubscribe)
HX_RESULT
CStreamGroupManager::Switch(CPhysicalStream* pNew, UINT32 ulNewLogical)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!pNew)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Get the current physical stream -- OK if NULL
    CPhysicalStream* pOld = GetCurrentPhysicalStream();
    UINT32 ulOldLogical = m_ulSelectedLogicalStream;

    // No need to do anything if old and new streams are the same
    if (pOld == pNew && ulNewLogical == ulOldLogical)
    {
	res = HXR_IGNORE;
    }

    // If there is a current stream, unsubscribe from it
    if (SUCCEEDED(res) && pOld)
    {
	res = UnsubscribePhysicalStream(pOld, ulOldLogical);
    }

    if (SUCCEEDED(res))
    {
	res = SubscribePhysicalStream(pNew, ulNewLogical);

	// If subscription failed for some reason, unsubscribe from the new stream
	// and re-subscribe to the old stream
        if (FAILED(res))
	{
	    HX_ASSERT(FALSE);
	    HX_RESULT resSub = UnsubscribePhysicalStream(pNew, ulNewLogical);

	    if (pOld)
		resSub = SubscribePhysicalStream(pOld, ulOldLogical);
	}
    }

    HX_RELEASE(pOld);

    HX_ASSERT(SUCCEEDED(res) || res==HXR_IGNORE);
    return res;
}



/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::SubscribePhysicalStream
// Purpose:
//  Given a physical stream, subscribes to the corresponding set of ASM rules.
HX_RESULT
CStreamGroupManager::SubscribePhysicalStream(CPhysicalStream* pPhysicalStream,
                                             UINT32 ulLogicalStream)
{
    HX_RESULT res = HXR_OK;

    // Validate state, params
    if (!pPhysicalStream)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    // If we've committed a logical stream, stick with it
    if (m_bInitialRateDescCommitted)
    {
        HX_ASSERT(ulLogicalStream == kulInvalidLogicalStreamNum ||
            ulLogicalStream == m_ulSelectedLogicalStream);
        ulLogicalStream = m_ulSelectedLogicalStream;
    }

    // If no stream specified, choose the default
    if (ulLogicalStream == kulInvalidLogicalStreamNum)
    {
        ulLogicalStream = pPhysicalStream->GetDefaultLogicalStream();
    }

    // Subscribe to each rule associated with the physical stream
    for (UINT32 i = 0; i < pPhysicalStream->GetNumRules() && SUCCEEDED(res); i++)
    {
        UINT32 ulRuleNum = pPhysicalStream->GetRuleArray()[i];
        //if (m_bDumpSub)
            //printf("\tSubscribing strm: %u rule: %u\n", ulStreamNo, unRuleNum);fflush(0);

        res = SubscribeLogicalStreamRule(ulLogicalStream, ulRuleNum, NULL);
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::UnsubscribePhysicalStream
// Purpose:
//  Given a physical stream, unsubscribes to the corresponding set of ASM rules.
HX_RESULT
CStreamGroupManager::UnsubscribePhysicalStream(CPhysicalStream* pPhysicalStream,
                                               UINT32 ulLogicalStream)
{
    HX_RESULT res = HXR_OK;

    // Validate state, params
    if (!m_pASMSource || !pPhysicalStream)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    if (ulLogicalStream == kulInvalidLogicalStreamNum)
    {
        ulLogicalStream = m_ulSelectedLogicalStream;
    }

    // Unsubscribe to each rule associated with the physical stream
    for (UINT16 i = 0; i < pPhysicalStream->GetNumRules(); i++)
    {
	UINT32 ulRuleNum = (UINT16)pPhysicalStream->GetRuleArray()[i];

	//if (m_bDumpSub)
	//	printf("\tUnsubscribing strm: %u rule: %u\n", ulStreamNo, pPhysicalStream->GetRuleArray()[i]);fflush(0);

	res = UnsubscribeLogicalStreamRule(ulLogicalStream, ulRuleNum, NULL);
    }	

    // Ignore unsubscribe failures
    HX_ASSERT(SUCCEEDED(res));
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::SetASMSource
// Purpose:
//  Used to set the ASM source
HX_RESULT
CStreamGroupManager::SetASMSource(IHXASMSource* pASMSource)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!pASMSource)
    {
	HX_ASSERT(FALSE);
	return HXR_POINTER;
    }

    HX_RELEASE(m_pASMSource);

    m_pASMSource = pASMSource;
    m_pASMSource->AddRef();

    for (UINT32 i=0; i<m_ulMaxNumLogicalStreams; i++)
    {
	if (m_ppLogicalStream[i])
	    m_ppLogicalStream[i]->SetASMSource(pASMSource);
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::SetRateDesc
// Purpose:
//  Subscribes to the appropriate physical streams for the given rate
// Returns:
//  HXR_OK if a new rate was selected.  
//  HXR_IGNORE if the new rate is the same as the current rate
//  Otherwise, a FAIL code.
HX_RESULT
CStreamGroupManager::SetRateDesc(IHXRateDescription* pRateDesc, 
                                 IHXStreamRateDescResponse* pResp,
                                 UINT32 ulLogicalStream)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!pRateDesc)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Get the physical stream associated with the rate desc
    CPhysicalStream* pNewInfo = NULL;
    if (SUCCEEDED(res))
    {
	UINT32 ulIndex = 0;
	res = GetRateDescIndex(pRateDesc, ulIndex);

	if (SUCCEEDED(res))
	{
	    pNewInfo = GetPhysicalStream(ulIndex);

	    if (!pNewInfo)
		res = HXR_FAIL;
	}
    }

    // Switch streams
    if (SUCCEEDED(res))
    {
        res = Switch(pNewInfo, ulLogicalStream);
    }

    if (SUCCEEDED(res))
    {
        HX_ASSERT(m_pRateDescResp == NULL);
        HX_RELEASE(m_pRateDescResp);

        if (pResp)
        {
            m_pRateDescResp = pResp;
            m_pRateDescResp->AddRef();
        }
    }

    HX_RELEASE(pNewInfo);

    HX_ASSERT(SUCCEEDED(res) || res==HXR_IGNORE);
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::CommitInitialRateDesc
// Purpose:
//  Commits stream selections
// Returns:
//  HXR_OK if initial rate desc was successfully committed.  
//  HXR_IGNORE if no rate description had been selected (stream group will not be active).  
//  Otherwise, a FAIL code.
HX_RESULT
CStreamGroupManager::CommitInitialRateDesc(BOOL bEnableAudioRateSwitching)
{
    HX_RESULT res = HXR_OK;

    // Quick sanity check -- not really a problem if called more than once (the call 
    // won't do anything after the first time)
    HX_ASSERT(!m_bInitialRateDescCommitted);

    // No stream has been selected -- streamgroup will be inactive
    if (m_ulCurRateDescriptionIndex == kulInvalidRateDescIndex)
    {
	res = HXR_IGNORE;
    }

    if (SUCCEEDED(res) && !m_bInitialRateDescCommitted)
    {
	CPhysicalStream* pPhysicalStream = GetPhysicalStream(m_ulCurRateDescriptionIndex);
	if (!pPhysicalStream)
	    res = HXR_FAIL;
	
        // Mark streams outside of the current switch group as not switchable
	if (SUCCEEDED(res))
	    res = UpdateSwitchGroups(pPhysicalStream);

        // For some handsets, mark all audio streams except the current one
        // as not switchable.
        if (SUCCEEDED(res) && !bEnableAudioRateSwitching)
        {
            if (!strncasecmp((const char*)m_pMimeType->GetBuffer(), "audio/", 6))
            {
                res = DisableRateSwitching(pPhysicalStream);
            }
        }

        HX_RELEASE(pPhysicalStream);

	if (SUCCEEDED(res))
	    m_bInitialRateDescCommitted = TRUE;
    }

    HX_ASSERT(SUCCEEDED(res) || res == HXR_IGNORE);
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::UpdateSwitchGroups
// Purpose:
//  Marks all physical streams in different switch groups as not switchable.
HX_RESULT
CStreamGroupManager::UpdateSwitchGroups(CPhysicalStream* pSelectedPhysicalStream)
{
    HX_RESULT res = HXR_OK;

    UINT32 ulSelectedSwitchGroup = pSelectedPhysicalStream->GetSwitchGroupID();

    for (UINT32 i=0; i<GetNumPhysicalStreams() && SUCCEEDED(res); i++)
    {
	CPhysicalStream* pCurPhysicalStream = GetPhysicalStream(i);

	// If the physical stream is in a different switch group, mark it 
	// as not available for switching
	if (pCurPhysicalStream->GetSwitchGroupID() != ulSelectedSwitchGroup)
	{
	    res = pCurPhysicalStream->ExcludeFromSwitching(TRUE, HX_SWI_INACTIVE_SWITCH_GROUP);
	}

	HX_RELEASE(pCurPhysicalStream);
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::DisableRateSwitching
// Purpose:
//  Marks all other streams in this group as not switchable. 
//  For clients which do not correctly handle server-side audio 
//  switching.
HX_RESULT
CStreamGroupManager::DisableRateSwitching(CPhysicalStream* pSelectedPhysicalStream)
{
    HX_RESULT res = HXR_OK;

    for (UINT32 i = 0; i < GetNumPhysicalStreams() && SUCCEEDED(res); i++)
    {
	CPhysicalStream* pCurPhysicalStream = GetPhysicalStream(i);

        // Mark all streams other than selected as unavailable.
	if (pCurPhysicalStream != pSelectedPhysicalStream)
	{
	    res = pCurPhysicalStream->ExcludeFromSwitching(TRUE, HX_SWI_INADEQUATE_CLIENT_CAPABILITIES);
	}

	HX_RELEASE(pCurPhysicalStream);
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::Upshift
// Purpose:
//  Upshifts to the appropriate physical streams for the given rate
// Notes:
//  An ulRate of 0 corresponds to taking the next highest rate
// Returns:
//  HXR_OK if a new rate was selected.  
//  HXR_IGNORE if the new rate is the same as the current rate, or no appropriate rate
//  could be found
//  Otherwise, a FAIL code.
HX_RESULT
CStreamGroupManager::Upshift(UINT32 ulRate, IHXStreamRateDescResponse* pResp)
{
    HX_RESULT res = HXR_OK;

    // Validate state -- can't upshift until after stream selection has been finalized
    if (!m_bInitialRateDescCommitted)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    CPhysicalStream* pCurPhysicalStream = GetCurrentPhysicalStream();
    UINT32 ulCurRate = 0;

    if (!pCurPhysicalStream)
    {
        return HXR_FAIL;
    }

    ulCurRate = pCurPhysicalStream->GetBandwidth();

    if (ulRate > 0 && ulRate <= ulCurRate)
    {
	res = HXR_IGNORE;
    }

    HX_RELEASE(pCurPhysicalStream);
 
    // Find the physical stream to shift to
    IHXRateDescription* pPhysicalStream = NULL;

    if (SUCCEEDED(res))
    {
	// If bitrate was specified, search for ulRate
	if (ulRate > 0)
	{	
	    res = FindRateDescByClosestAvgRate(ulRate, FALSE, TRUE, pPhysicalStream);

	    // If an appropriate bitrate couldn't be found, find the next closest
	    if (FAILED(res))
	    {
	    	res = FindRateDescByMidpoint(ulRate, FALSE, TRUE, pPhysicalStream);		    
	    }	    
	}

	// Either appropriate bitrate couldn't be found 
        // or requested rate is 0 (force switch to next-highest).
	if (FAILED(res) || ulRate == 0)
	{
	    res = GetNextSwitchableRateDesc(pPhysicalStream);	
	}

	if (FAILED(res))
        {
	    res = HXR_IGNORE;
        }
    }

    if (SUCCEEDED(res))
    {
        HX_ASSERT(pPhysicalStream);

        UINT32 ulNewRate = 0;
        pPhysicalStream->GetAvgRate(ulNewRate);

        // Same rate. Don't bother shifting!
        if (ulNewRate == ulCurRate)
        {
            res = HXR_IGNORE;
        }
    }

    if (SUCCEEDED(res))
    {
	res = SetRateDesc(pPhysicalStream, pResp);
    }

    if (FAILED(res) && pResp)
    {
        pResp->ShiftDone(res, (UINT16)m_ulSelectedLogicalStream, NULL);
    }

    HX_RELEASE(pPhysicalStream);

    HX_ASSERT(SUCCEEDED(res) || res==HXR_IGNORE || res == HXR_NOTENOUGH_BANDWIDTH);
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::Downshift
// Purpose:
//  Downshifts to the appropriate physical streams for the given rate
// Notes:
//  An ulRate of 0 corresponds to taking the next lowest rate
// Returns:
//  HXR_OK if a new rate was selected.  
//  HXR_IGNORE if the new rate is the same as the current rate, or no appropriate rate
//  could be found
//  Otherwise, a FAIL code.
HX_RESULT
CStreamGroupManager::Downshift(UINT32 ulRate, IHXStreamRateDescResponse* pResp)
{
    HX_RESULT res = HXR_OK;

    // Validate state -- can't downshift until after stream selection has been finalized
    if (!m_bInitialRateDescCommitted)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    CPhysicalStream* pCurPhysicalStream = GetCurrentPhysicalStream();
    UINT32 ulCurRate = 0;

    if (!pCurPhysicalStream)
    {
        return HXR_FAIL;
    }

    ulCurRate = pCurPhysicalStream->GetBandwidth();

    // Downshift a single step if the new rate is greater than current rate
    if (ulRate >= ulCurRate)
    {
	ulRate = 0;
    }

    HX_RELEASE(pCurPhysicalStream);

    // Find the physical stream to shift to
    IHXRateDescription* pPhysicalStream = NULL;
    if (SUCCEEDED(res))
    {
	// If bitrate was specified, search for ulRate
	if (ulRate > 0)
	{	
	    res = FindRateDescByClosestAvgRate(ulRate, FALSE, TRUE, pPhysicalStream);

	    // If an appropriate bitrate couldn't be found, find the next closest
	    if (FAILED(res))
	    {
	    	res = FindRateDescByMidpoint(ulRate, FALSE, TRUE, pPhysicalStream);		    
	    }	    
	}

	// Either appropriate bitrate couldn't be found 
        // or requested rate is 0 (force switch to next-lowest).
	if (FAILED(res) || ulRate == 0)
	{
	    res = GetPrevSwitchableRateDesc(pPhysicalStream);	
	}

	if (FAILED(res))
        {
	    res = HXR_IGNORE;
        }
    }

    if (SUCCEEDED(res))
    {
        HX_ASSERT(pPhysicalStream);

        UINT32 ulNewRate = 0;
        pPhysicalStream->GetAvgRate(ulNewRate);

        // Same rate. Don't bother shifting!
        if (ulNewRate == ulCurRate)
        {
            res = HXR_IGNORE;
        }
    }

    if (SUCCEEDED(res))
    {
	res = SetRateDesc(pPhysicalStream, pResp);
    }

    if (FAILED(res) && pResp)
    {
        pResp->ShiftDone(res, (UINT16)m_ulSelectedLogicalStream, NULL);
    }

    HX_RELEASE(pPhysicalStream);
	   
    HX_ASSERT(SUCCEEDED(res) || res==HXR_IGNORE || res == HXR_NOTENOUGH_BANDWIDTH);
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::SubscribeLogicalStreamRule
// Purpose:
//  Subscribes to a logical stream's rule.  Also updates the current rate description.
HX_RESULT
CStreamGroupManager::SubscribeLogicalStreamRule(UINT32 ulLogicalStreamNum, 
                                                UINT32 ulRuleNum, 
                                                IHXStreamRateDescResponse* pResp)
{
    HX_RESULT res = HXR_OK;

    // Validate state
    if (!m_ppLogicalStream || 
        ulLogicalStreamNum >= m_ulMaxNumLogicalStreams ||
        !m_ppLogicalStream[ulLogicalStreamNum])
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    CPhysicalStream* pPhysicalStream = GetCurrentPhysicalStream();
    
    // We have already selected a physical stream. Unsubscribe to it first, if it is not same.
    // We need to do this only during initial subscription/selection. During the playback, ::Switch() 
    // takes care of unsubscribing to the current physical stream before subscribing to a new one
    if (!m_bInitialRateDescCommitted && pPhysicalStream)
    {
        for (UINT16 i = 0; i < pPhysicalStream->GetNumRules(); i++)
        {
            UINT32 ulRule = (UINT16)pPhysicalStream->GetRuleArray()[i];
            if (ulRule == ulRuleNum && ulLogicalStreamNum == m_ulSelectedLogicalStream)
            {
                //Already subscribed
                return HXR_OK;
            }
        }

        if (ulLogicalStreamNum == m_ulSelectedLogicalStream)
        {
            UnsubscribePhysicalStream(pPhysicalStream, m_ulSelectedLogicalStream);
        }
    }
    HX_RELEASE(pPhysicalStream);

    m_ulSelectedLogicalStream = ulLogicalStreamNum;

    // Subscribe to the rule
    res = m_ppLogicalStream[ulLogicalStreamNum]->
            Subscribe(ulLogicalStreamNum, ulRuleNum);

    // Update the current rate description -- OK if it fails
    if (SUCCEEDED(res))
    {
	HX_RESULT resUpdate = UpdateCurrentRateDesc();
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::UnsubscribeLogicalStreamRule
// Purpose:
//  Unsubscribes from a logical stream's rule.  Also updates the current rate description.
HX_RESULT
CStreamGroupManager::UnsubscribeLogicalStreamRule(UINT32 ulLogicalStreamNum, 
                                                  UINT32 ulRuleNum, 
                                                  IHXStreamRateDescResponse* pResp)
{
    HX_RESULT res = HXR_OK;

    // Validate state
    if (!m_ppLogicalStream || 
        ulLogicalStreamNum >= m_ulMaxNumLogicalStreams ||
        !m_ppLogicalStream[ulLogicalStreamNum])
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Sanity check 
    HX_ASSERT(m_ulSelectedLogicalStream == kulInvalidLogicalStreamNum || 
	m_ulSelectedLogicalStream == ulLogicalStreamNum);

    // Unsubscribe from the rule
    res = m_ppLogicalStream[ulLogicalStreamNum]->
            Unsubscribe(ulLogicalStreamNum, ulRuleNum);

    // Update the current rate description -- OK if it fails
    if (SUCCEEDED(res))
    {
	HX_RESULT resUpdate = UpdateCurrentRateDesc();
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::UpdateCurrentRateDesc
// Purpose:
//  Updates the current rate description
HX_RESULT
CStreamGroupManager::UpdateCurrentRateDesc()
{
    HX_RESULT res = HXR_OK;

    // Validate state
    if (m_ulSelectedLogicalStream == kulInvalidLogicalStreamNum)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    IHXRateDescription* pRateDesc = NULL;
    res = m_ppLogicalStream[m_ulSelectedLogicalStream]->GetCurrentRateDesc(pRateDesc);

    UINT32 ulIndex = kulInvalidRateDescIndex;
    if (SUCCEEDED(res))
	res = GetRateDescIndex(pRateDesc, ulIndex);

    // Set the current rate description (OK to set to invalid index, if that's actually the case)
    SetCurrentRateDesc(ulIndex);
    
    HX_RELEASE(pRateDesc);
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CStreamGroupManager::HandlePacket
// Purpose:
//  Clears packet pending flag for a given rule number
HX_RESULT 
CStreamGroupManager::HandlePacket(UINT32 ulRuleNum)
{
    // Validate state
    if (!m_ppLogicalStream || m_ulSelectedLogicalStream == kulInvalidLogicalStreamNum || 
	!m_ppLogicalStream[m_ulSelectedLogicalStream])
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    return m_ppLogicalStream[m_ulSelectedLogicalStream]->HandlePacket(ulRuleNum);
}

BOOL
CStreamGroupManager::IsPending()
{
    // Validate state
    if (!m_ppLogicalStream || m_ulSelectedLogicalStream == kulInvalidLogicalStreamNum || 
	!m_ppLogicalStream[m_ulSelectedLogicalStream])
    {
	return FALSE;
    }

    return m_ppLogicalStream[m_ulSelectedLogicalStream]->IsPending();
}

void 
CStreamGroupManager::ShiftDone(HX_RESULT status)
{
    IHXRateDescription* pRateDesc = NULL;

    if (SUCCEEDED(status))
    {
        GetCurrentRateDesc(pRateDesc);
    }

    if (m_pRateDescResp)
    {
        m_pRateDescResp->ShiftDone(status, (UINT16)m_ulSelectedLogicalStream, 
            pRateDesc);
        HX_RELEASE(m_pRateDescResp);
    }

    HX_RELEASE(pRateDesc);
}

CSwitchGroupContainer* 
CStreamGroupManager::GetSwitchGroup(UINT32 ulSwitchGroupID)
{
    if (m_ppSwitchGroup)
    {
        for (UINT32 i = 0; i < m_ulNumSwitchGroups; i++)
        {
            if (m_ppSwitchGroup[i] && 
                m_ppSwitchGroup[i]->GetSwitchGroupID() == ulSwitchGroupID)
            {
                return m_ppSwitchGroup[i];
            }
        }
    }

    return NULL;
}
