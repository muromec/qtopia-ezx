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
#include "uberstreammgr.h"

#include "hxasm.h"
#include "hxccf.h"

#include "source.h"
#include "sink.h"
#include "ispifs.h"

#include "proc.h"
#include "hxassert.h"

#include "hxqos.h"
#include "qos_cfg_names.h"

#include "asmrulep.h"
#include "ratedescmgr.h"
#include "streamgroupmgr.h"

#define DEBUG_FLAG 0x02100000
static const UINT32 DEFAULT_PREROLL = 1000;

// Implements basic IUnknown functionality
BEGIN_INTERFACE_LIST(CUberStreamManager)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXUberStreamManager)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXUberStreamManagerInit)    
    INTERFACE_LIST_ENTRY_SIMPLE(IHXRateDescEnumerator)
END_INTERFACE_LIST

/////////////////////////////////////////////////////////////////////////
// Method:
//  CreateASMRuleHandler
// Purpose:
//  Used to create IHXASMRuleHandler
HX_RESULT CreateUberStreamManager(IHXCommonClassFactory* pCCF, IHXUberStreamManagerInit** ppUberStreamMgr)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!ppUberStreamMgr || !pCCF)
    {
	HX_ASSERT(FALSE);
	return HXR_POINTER;
    }
    
    IHXUberStreamManagerInit* pUberStreamMgr  = new CUberStreamManager(pCCF);
    if (!pUberStreamMgr )
    {
	res = HXR_OUTOFMEMORY;	
    }
    
    else
    {
	*ppUberStreamMgr = pUberStreamMgr;
	(*ppUberStreamMgr)->AddRef();	
    }

    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamContainer.CUberStreamContainer
// Purpose:
//  Constructor
CUberStreamContainer::CUberStreamContainer()
{
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamContainer::~CUberStreamContainer
// Purpose:
//  Destructor
CUberStreamContainer::~CUberStreamContainer()
{
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamContainer::GetBandwidthGrouping
// Purpose:
//  Returns the CBandwidthGrouping at ulIndex
CBandwidthGrouping* 
CUberStreamContainer::GetBandwidthGrouping(UINT32 ulIndex)
{
    return (CBandwidthGrouping*)CRateDescriptionMgr::GetRateDescription(ulIndex);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamContainer::GetBandwidthGrouping
// Purpose:
//  Returns the CBandwidthGrouping at ulIndex
HX_RESULT
CUberStreamContainer::GetBandwidthGrouping(UINT32 ulIndex, REF(CBandwidthGrouping*)pBandwidthGrouping)
{
    pBandwidthGrouping = (CBandwidthGrouping*)CRateDescriptionMgr::GetRateDescription(ulIndex);

    if (pBandwidthGrouping)
	return HXR_OK;
    else
	return HXR_FAIL;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamContainer::AddBandwidthGrouping
// Purpose:
//  Inserts CBandwidthGrouping into CBandwidthGrouping container
HX_RESULT
CUberStreamContainer::AddBandwidthGrouping(CBandwidthGrouping* pBandwidthGrouping)
{
    return AddRateDescription(pBandwidthGrouping);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamContainer::isDuplicate
// Purpose:
//  Determines if pRateDescription is a duplicate of an existing CBandwidthGrouping
BOOL
CUberStreamContainer::isDuplicate(CRateDescription* pRateDescription, REF(UINT32)ulIndex)
{
    CBandwidthGrouping* pBandwidthGrouping = (CBandwidthGrouping*)pRateDescription;

    BOOL bIsDuplicate = FALSE;
    for (UINT32 i=0; i<GetNumBandwidthGroupings() && !bIsDuplicate; i++)
    {
	CBandwidthGrouping* pCurBandwidthGrouping = GetBandwidthGrouping(i);

	if (pCurBandwidthGrouping->GetBandwidth() == pBandwidthGrouping->GetBandwidth())
	{
	    ulIndex = i;
	    bIsDuplicate = TRUE;
	}
	else if (pCurBandwidthGrouping->IsEntriesSame(pBandwidthGrouping))
	{
	    ulIndex = i;
	    bIsDuplicate = TRUE;
	}

	HX_RELEASE(pCurBandwidthGrouping);
    }

    return bIsDuplicate;        
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamContainer::shouldSwap
// Purpose:
//  Replaces entry at ulIndex with pBandwidthGrouping, if pBandwidthGrouping is more descriptive
BOOL
CUberStreamContainer::shouldSwap(UINT32 ulIndex, CRateDescription* pRateDescription)
{
    BOOL bShouldSwap = FALSE;

    CBandwidthGrouping* pBandwidthGrouping = (CBandwidthGrouping*)pRateDescription;
    CBandwidthGrouping* pCurBandwidthGrouping = GetBandwidthGrouping(ulIndex);

    if (!pCurBandwidthGrouping->GetBandwidth() && pBandwidthGrouping->GetBandwidth())
    {
	// pick the one with non-zero bandwidth
	bShouldSwap = TRUE;
    }
    else if (pCurBandwidthGrouping->GetNumBandwidthAllocations() < pBandwidthGrouping->GetNumBandwidthAllocations())
    {
	// pick the one with more allocations :)
	bShouldSwap = TRUE;
    }    

    HX_RELEASE(pCurBandwidthGrouping);

    return bShouldSwap;
}

CBandwidthGrouping*	
CUberStreamContainer::GetCurrentBandwidthGrouping()
{
    return (CBandwidthGrouping*)GetCurrentRateDescription();
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::CUberStreamManager
// Purpose:
//  Constructor
CUberStreamManager::CUberStreamManager(IHXCommonClassFactory* pCCF,
                                       IHXQoSProfileConfigurator* pQoSConfig)
    : m_pCCF(pCCF)
    , m_pQoSConfig(pQoSConfig)
    , m_ulNumStreamGroups(0)
    , m_ulNumLogicalStreams(0)
    , m_ulNumLogicalStreamProcessed(0)
    , m_bShiftPending(FALSE)
    , m_pUberRuleBook(NULL)
    , m_ppStreamGroup(NULL)    
    , m_bCheckAverageBandwidth(TRUE)
    , m_bInitialRateDescCommitted(FALSE)
    , m_pASMSource(NULL)
    , m_bDumpSub(FALSE)
    , m_aulLogicalStreamToStreamGroup(NULL)
{
    HX_ASSERT(m_pCCF);
    HX_ADDREF(m_pCCF);

    HX_ASSERT(m_pQoSConfig);
    HX_ADDREF(m_pQoSConfig);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::~CUberStreamManager
// Purpose:
//  Destructor
CUberStreamManager::~CUberStreamManager()
{
    UINT32 i = 0;
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pQoSConfig);

    if (m_ppStreamGroup)
    {
	for (i = 0; i < m_ulNumStreamGroups; i++)
	{
	    HX_RELEASE(m_ppStreamGroup[i]);
	}	    
	HX_VECTOR_DELETE(m_ppStreamGroup);
    }

    HX_DELETE(m_pUberRuleBook);

    HX_RELEASE(m_pASMSource);

    HX_VECTOR_DELETE(m_aulLogicalStreamToStreamGroup);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::Init
// Purpose:
//  Creates logical stream rulebooks / AltStream objects
HX_RESULT
CUberStreamManager::Init(BOOL bCheckAverageBandwidth)
{
    HX_RESULT res = HXR_OK;

    // Validate state -- can only be called before SetFile/StreamHeader
    if (m_ulNumStreamGroups > 0 || m_ppStreamGroup)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    m_bCheckAverageBandwidth = bCheckAverageBandwidth;

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::CommitInitialAggregateRateDesc
// Purpose:
//  Commits stream selections
STDMETHODIMP
CUberStreamManager::CommitInitialAggregateRateDesc()
{
    HX_RESULT res = HXR_OK;
    BOOL bEnableAudioRateShifting = TRUE;

    // Validate state
    if (m_ulNumStreamGroups == 0 || !m_ppStreamGroup)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }
    
    if (m_bInitialRateDescCommitted)
    {
	return HXR_OK;
    }

    if (m_pQoSConfig)
    {
	INT32 lTemp = 0;
    	if (m_pQoSConfig->GetConfigInt(QOS_CFG_IS_DISABLE_AUDIO_SW, lTemp) == HXR_OK)
        {
            bEnableAudioRateShifting = (BOOL)lTemp;
        }
    }

    // Commit rate description for each stream group
    UINT32 ulActiveStreamGroups = 0;
    if (SUCCEEDED(res) && !m_bInitialRateDescCommitted)
    {
	for (UINT32 i=0; i<m_ulNumStreamGroups; i++)
	{
	    if (m_ppStreamGroup[i])
	    {
		res = m_ppStreamGroup[i]->CommitInitialRateDesc(bEnableAudioRateShifting);

		// OK if stream group returns HXR_IGNORE (means it will not be active)
		if (SUCCEEDED(res))
		{
		    ulActiveStreamGroups++;
		}
		else if (res == HXR_IGNORE)
		{
		    res = HXR_OK;
		}
	    }
	}
    }

    // There must be at least one active stream group
    if (ulActiveStreamGroups == 0)
    {
	res = HXR_FAIL;
    }

    // If there are inactive stream groups, synthesize a new uber rulebook (since the
    // current bandwidth groupings have entries for all streams groups -- including the inactive ones)
    if (SUCCEEDED(res) && GetNumActiveStreamGroups() != m_ulNumStreamGroups)
    {
	res = SynthesizeUberRulebook(TRUE);
    }

    // Update aggregate rate desc
    if (SUCCEEDED(res))
    {
	m_bInitialRateDescCommitted = TRUE;
	UpdateShiftPending();
	res = UpdateAggregateRateDesc();
    }

    // XXXJJ comment out the assert here. We might not have those configurations in the config file. In
    // those cases res will be HXR_FAIL because ulActiveStreamGroups is 0.
    //HX_ASSERT(SUCCEEDED(res));
    return res;
}
/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::SetASMSource
// Purpose:
//  Used to set the ASM source
STDMETHODIMP
CUberStreamManager::SetASMSource(IHXASMSource* pASMSource)
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

    if (m_ppStreamGroup)
    {
	for (UINT32 i=0; i<m_ulNumStreamGroups; i++)
	{
	    if (m_ppStreamGroup[i])
		res = m_ppStreamGroup[i]->SetASMSource(pASMSource);
	}
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::SetFileHeader
// Purpose:
//  Creates logical stream rulebooks / AltStream objects
STDMETHODIMP
CUberStreamManager::SetFileHeader(IHXValues* pFileHeader)
{
    HX_RESULT res = HXR_OK;

    // Validate state, params
    if (m_ulNumStreamGroups > 0 || m_ppStreamGroup)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    res = FixupFileHeader(pFileHeader);

    // Determine the number of logical streams
    if (SUCCEEDED(res))
	res = pFileHeader->GetPropertyULONG32("StreamCount", m_ulNumLogicalStreams);

    // Create logical stream to stream group map
    if (SUCCEEDED(res))
    {
	m_aulLogicalStreamToStreamGroup = new UINT32[m_ulNumLogicalStreams];

	if (m_aulLogicalStreamToStreamGroup)
	{
	    for (UINT32 i=0; i<m_ulNumLogicalStreams; i++)
		m_aulLogicalStreamToStreamGroup[i] = kulInvalidStreamGroupNum;
	}
	else
	{
	    res = HXR_OUTOFMEMORY;
	}
    }

    // Determine the number of stream groups
    if (SUCCEEDED(res))
	res = pFileHeader->GetPropertyULONG32("StreamGroupCount", m_ulNumStreamGroups);

    // Get the uber rulebook -- OK if there is none (will create a fake uber rulebook later)
    if (SUCCEEDED(res))
    {
	IHXBuffer* pRuleBook = NULL;
	pFileHeader->GetPropertyCString("ASMRuleBook", pRuleBook);

	res = ProcessUberRulebook(pRuleBook);
	HX_RELEASE(pRuleBook);
    }

    // Create stream group mgr array
    if (SUCCEEDED(res))
    {
	m_ppStreamGroup = new CStreamGroupManager*[m_ulNumStreamGroups];
	if (!m_ppStreamGroup)
	    res = HXR_OUTOFMEMORY;
	else
	    memset(m_ppStreamGroup, 0, sizeof(CStreamGroupManager*)*m_ulNumStreamGroups);
    }

    // Init stream group array
    for (UINT32 i=0; i<m_ulNumStreamGroups && SUCCEEDED(res); i++)
    {
	m_ppStreamGroup[i] = new CStreamGroupManager(m_pCCF);
	if (m_ppStreamGroup[i])
	{
	    m_ppStreamGroup[i]->AddRef();
	    res = m_ppStreamGroup[i]->Init(i, m_ulNumLogicalStreams, m_bCheckAverageBandwidth);

	    if (SUCCEEDED(res) && m_pASMSource)
		res = m_ppStreamGroup[i]->SetASMSource(m_pASMSource);
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
//  CUberStreamManager::FixupFileHeader
// Purpose:
//  Ensures that the file header contains stream group properties
HX_RESULT
CUberStreamManager::FixupFileHeader(IHXValues* pFileHeader)
{
    HX_RESULT res = HXR_OK;

    // Ensure that there is a stream group count
    if (SUCCEEDED(res))
    {
	UINT32 ulNumStreamGroups = 0;
	res = pFileHeader->GetPropertyULONG32("StreamGroupCount", ulNumStreamGroups);

	// If there was no stream group count, set it to be the number of logical streams
	if (FAILED(res))
	{
	    res = pFileHeader->GetPropertyULONG32("StreamCount", ulNumStreamGroups);

	    if (SUCCEEDED(res))
		res = pFileHeader->SetPropertyULONG32("StreamGroupCount", ulNumStreamGroups);
	}
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::SetStreamHeader
// Purpose:
//  Ensures that the stream header contains stream group properties
HX_RESULT
CUberStreamManager::FixupStreamHeader(UINT32 ulLogicalStreamNum, IHXValues* pStreamHeader)
{
    HX_RESULT res = HXR_OK;

    // Ensure that there is a stream group number
    if (SUCCEEDED(res))
    {
	UINT32 ulStreamGroupNum = 0;
	res = pStreamHeader->GetPropertyULONG32("StreamGroupNumber", ulStreamGroupNum);

	// If there was no stream group number, set it equal to the logical stream number
	if (FAILED(res))
	    res = pStreamHeader->SetPropertyULONG32("StreamGroupNumber", ulLogicalStreamNum);
    }

    // Ensure that there is a switch group number
    if (SUCCEEDED(res))
    {
	UINT32 ulSwitchGroupNum = 0;
	res = pStreamHeader->GetPropertyULONG32("SwitchGroupID", ulSwitchGroupNum);

	// If there was no switch group number, set it equal to the logical stream number
	// Note that this assumes that all (or none) of the logical streams have a switch group number
	if (FAILED(res))
	    res = pStreamHeader->SetPropertyULONG32("SwitchGroupID", ulLogicalStreamNum);
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::SetStreamHeader
// Purpose:
//  Processes a logical stream header
STDMETHODIMP
CUberStreamManager::SetStreamHeader(UINT32 ulLogicalStreamNum, IHXValues* pStreamHeader)
{
    HX_RESULT res = HXR_OK;

    // Validate state, params
    if (!m_ppStreamGroup || !pStreamHeader || ulLogicalStreamNum > m_ulNumLogicalStreams)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Sanity check logical stream number
    if (SUCCEEDED(res))
    {
	UINT32 ulTempLogicalStreamNum = 0;
	res = pStreamHeader->GetPropertyULONG32("StreamNumber", ulTempLogicalStreamNum);

	if (SUCCEEDED(res) && ulTempLogicalStreamNum != ulLogicalStreamNum)
	    res = HXR_FAIL;
    }

    if (SUCCEEDED(res))
        res = FixupStreamHeader(ulLogicalStreamNum, pStreamHeader);

    // Get stream number group number
    UINT32 ulStreamGroupNum = 0;
    if (SUCCEEDED(res))
	res = pStreamHeader->GetPropertyULONG32("StreamGroupNumber", ulStreamGroupNum);

    // Sanity check stream group number
    if (SUCCEEDED(res) &&
	(ulStreamGroupNum > m_ulNumStreamGroups || !m_ppStreamGroup[ulStreamGroupNum]))
	    res = HXR_FAIL;

    if (SUCCEEDED(res))
	m_aulLogicalStreamToStreamGroup[ulLogicalStreamNum] = ulStreamGroupNum;

    // Pass logical stream to the appropriate stream group mgr
    if (SUCCEEDED(res))
	res = m_ppStreamGroup[ulStreamGroupNum]->SetStreamHeader(ulLogicalStreamNum, pStreamHeader);

    if (SUCCEEDED(res))
	m_ulNumLogicalStreamProcessed++;

    // If all logical streams have been received, process rulebooks
    if (SUCCEEDED(res) && m_ulNumLogicalStreams == m_ulNumLogicalStreamProcessed)
    {
	// Sanity check that all logical streams have been processed
	for (UINT32 i=0; i<m_ulNumLogicalStreams; i++)
	{
	    HX_ASSERT(m_aulLogicalStreamToStreamGroup[i] != kulInvalidStreamGroupNum);
	}

	res = ProcessRulebooks();
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::ProcessUberRulebook
// Purpose:
//  Used to set uber rulebook.  Creates uber rulebook parser
// Notes:
//  OK if pRulebook is NULL (will create a fake uber rulebook)
HX_RESULT
CUberStreamManager::ProcessUberRulebook(IHXBuffer* pRulebook)
{
    HX_RESULT res = HXR_OK;
    
    // Validate state
    if (m_pUberRuleBook)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;		
    }	

    // XXXLY - Should really do init of m_cUberContainer in case where there is no uber rulebook in same place

    if (pRulebook && pRulebook->GetSize())
    {	
//	printf("ASMRuleBook: %s\n", (const char*)pRulebook->GetBuffer());fflush(0);
	m_pUberRuleBook = new ASMRuleBook((const char*)pRulebook->GetBuffer());
	if (m_pUberRuleBook)
	{
	    if (m_pUberRuleBook->GetNumRules())
	    {
		res = m_cUberContainer.Init(m_pUberRuleBook->GetNumRules());
	    }
	    else
	    {
		// well, just pretend nothing happend and see what happens.
		HX_DELETE(m_pUberRuleBook);
	    }
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
//  CUberStreamManager::ProcessRulebooks
// Purpose:
//  Processes uber/logical stream rulebooks and uses the results to
//  populate the m_pUberStreamMgr/m_ppStreamGroup arrays
HX_RESULT
CUberStreamManager::ProcessRulebooks()
{
    HX_RESULT res = HXR_FAIL;
    
    if (!m_ulNumStreamGroups)
    {
	HX_ASSERT(FALSE);
	return HXR_UNEXPECTED;
    }
    
    // Extract threshold bandwidths and bandwidth partitioning info from the uber rulebook
    if (m_pUberRuleBook)
    {	
	res = ProcessUberRule();

	// OK there were problems processing the rulebook -- will create fake one
	if (FAILED(res))
	{
	    HX_DELETE(m_pUberRuleBook);
	    res = HXR_OK;
	}
    }

    // Synthesize an uber rulebook for all streamgroups, if necessary
    if (!m_pUberRuleBook)
    {
	res = SynthesizeUberRulebook(FALSE);
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::ProcessUberRule
// Purpose:
//  Determines the bandwidth partitioning info for each threshold bandwidth 
//  in the uber rulebook and stores it in m_pUberStreamMgr.
// Returns:
//  FAIL code if there was a problem extracting any bandwidth grouping.  
//  Otherwise SUCCESS code.
HX_RESULT
CUberStreamManager::ProcessUberRule()
{
    HX_ASSERT(m_pUberRuleBook);
    HX_RESULT	    theErr = HXR_FAIL;
    UINT32	    ulNumThreshold = 0;
    float*	    pThresholds = NULL;
    
    // Determine number of bandwidth thresholds
    theErr = CAsmRuleBookParser::GetThresholdInfo(m_pUberRuleBook, m_pCCF,
			      ulNumThreshold,pThresholds);
			      
    // Get bandwidth paritioning info for each threshold bandwidth
    for (UINT32 i = 0; i < ulNumThreshold && HXR_OK == theErr; i++)
    {
	// Create bandwidth grouping for the given bitrate (if one doesn't already exist)
	theErr = ExtractBandwidthGrouping((UINT32)pThresholds[i]);
    }

    HX_VECTOR_DELETE(pThresholds);

    return theErr;
}

void
CUberStreamManager::Dump()
{
    printf("***** DUMPING *****\n");
    printf("UBER:\n");
    m_cUberContainer.Dump();

    for (UINT32 i = 0; i < m_ulNumStreamGroups; i++)
    {
	printf("STRM: %u\n", i);
	m_ppStreamGroup[i]->Dump();
    }	
    fflush(0);
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::ExtractBandwidthGrouping
// Purpose:
//  Creates a struct that contains bandwidth paritioning info for a given bitrate, 
//  and stores it in the uber container
HX_RESULT 
CUberStreamManager::ExtractBandwidthGrouping(UINT32 ulBandwidth)
{
    HX_RESULT res = HXR_OK;    
    
    // Determine bandwidth partitioning between logical streams
    UINT32 ulNumBandwidthAllocations = 0;
    UINT32* pulBandwidthAllocation = NULL;
    if (SUCCEEDED(res))
	res = UberSubscription(ulBandwidth, ulNumBandwidthAllocations, pulBandwidthAllocation);

    // Verify that each logical stream has some bandwidth allocated
    if (SUCCEEDED(res) && ulNumBandwidthAllocations != m_ulNumStreamGroups)
    {
	res = HXR_FAIL;
    }

    // Create array containing the current physical stream from each active stream group
    CPhysicalStream** apPhysicalStream = NULL;
    if (SUCCEEDED(res))
    {
	apPhysicalStream = new CPhysicalStream*[m_ulNumStreamGroups];
    
	if (!apPhysicalStream)
	    res = HXR_OUTOFMEMORY;
	else
	    memset(apPhysicalStream, 0, sizeof(CPhysicalStream*)*m_ulNumStreamGroups);

	// Note: This will fail if a bandwidth ratio has been specified
	for (UINT32 i=0; i<m_ulNumStreamGroups && SUCCEEDED(res); i++)
	{
	    res = m_ppStreamGroup[i]->FindRateDescByExactAvgRate(pulBandwidthAllocation[i], FALSE, FALSE, (IHXRateDescription*&)apPhysicalStream[i]);
	}
    }

    // Create bandwidth grouping from current stream array
    if (SUCCEEDED(res))
	res = CreateBandwidthGrouping(apPhysicalStream);

    // Cleanup array
    for (UINT32 i=0; i<m_ulNumStreamGroups && apPhysicalStream; i++)
    {
	HX_RELEASE(apPhysicalStream[i]);
    }
    HX_VECTOR_DELETE(apPhysicalStream);

    HX_VECTOR_DELETE(pulBandwidthAllocation);
    	  
    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::UberSubscription
// Purpose:
//  Determine how bandwidth will be divided between logical streams.  Returns the 
//  result in pRules.
HX_RESULT 
CUberStreamManager::UberSubscription(UINT32 ulBandwidth, REF(UINT32)unCount, REF(UINT32*)pRules)
{
    HX_ASSERT(m_pUberRuleBook);

    HX_RESULT theErr = HXR_OK;
    BOOL*      pSubs = NULL;
    UINT32 ulNumRules = 0;
    
    // Determine which rules will be subscribed to for the specified bandwidth
    if (HXR_OK == theErr && (ulNumRules = m_pUberRuleBook->GetNumRules()))
    {
	theErr = CAsmRuleBookParser::GetRuleSubscriptions(m_pUberRuleBook, m_pCCF, ulBandwidth, pSubs);
    }

    char pc[48];
    IHXValues* pVal = NULL;
    IHXBuffer* pBuf = NULL;

    if (HXR_OK == theErr)
    {
	// unCount shall never be > m_ulNumStreamGroups
	unCount = 0;
	pRules = new UINT32[m_ulNumStreamGroups];
	if (pRules)
	{
	    memset(pRules, 0, sizeof(UINT32)*m_ulNumStreamGroups);
	}
	else
	{
	    theErr = HXR_OUTOFMEMORY;
	}
    }

    // Determine the bandwidth directives associated with the subscribed rule.  This info is used
    // to divide the total bandwidth between logical streams.
    for (UINT32 i = 0; HXR_OK == theErr && i < ulNumRules; i++)
    {
        if (pSubs[i])
        {
            theErr = m_pUberRuleBook->GetProperties((UINT16)i, pVal);

            for (UINT32 j = 0; HXR_OK == theErr && j < m_ulNumStreamGroups; j++)
            {
                sprintf (pc,"Stream%uBandwidth", j);
                theErr = pVal->GetPropertyCString(pc, pBuf);
		if (HXR_OK == theErr)
		{
		    pRules[unCount++] = atoi((char*)pBuf->GetBuffer());
		    
//		    printf("%s: bandwidth: %u\n", pc,
//			atoi((char*)pBuf->GetBuffer()));fflush(0);
		    HX_RELEASE(pBuf);			
		}                
            }
            HX_RELEASE(pVal);            
            // only one rule allowed
            break;
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
//  CUberStreamManager::HandlePacket
// Purpose:
//  Updates shift pending flag
HX_RESULT 
CUberStreamManager::HandlePacket(UINT32 ulStreamGroupNum, UINT16 unRuleNum)
{
    if (!m_bShiftPending)
    {
        return HXR_OK;
    }

    HX_ASSERT(ulStreamGroupNum < m_ulNumStreamGroups);
    HX_RESULT res = m_ppStreamGroup[ulStreamGroupNum]->HandlePacket(unRuleNum);

    // Check if shift is still pending
    UpdateShiftPending();

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::UpdateShiftPending
// Purpose:
//  Determines if any logical stream has a shift pending
void
CUberStreamManager::UpdateShiftPending()
{
    if (m_bInitialRateDescCommitted)
    {
	for (UINT32 i = 0; i < m_ulNumStreamGroups; i++)
	{
	    if (m_ppStreamGroup[i]->IsPending())
	    {
		m_bShiftPending = TRUE;
		return;
	    }
	}
    }

    m_bShiftPending = FALSE;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::GetStreamGroup
// Purpose:
//  Used to get a stream group enumerator
STDMETHODIMP
CUberStreamManager::GetStreamGroup(UINT32 ulStreamGroupNum, REF(IHXRateDescEnumerator*)pMgr)
{
    // Validate state, params
    if (!m_ppStreamGroup || ulStreamGroupNum >= m_ulNumStreamGroups || !m_ppStreamGroup[ulStreamGroupNum])
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    pMgr = m_ppStreamGroup[ulStreamGroupNum];
    pMgr->AddRef();

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::GetNumActiveStreamGroups
// Purpose:
//  Used to get a stream group enumerator
STDMETHODIMP_(UINT32)
CUberStreamManager::GetNumActiveStreamGroups()
{
    // Validate state
    if (!m_ppStreamGroup)
    {
	return 0;
    }

    UINT32 ulNumActiveStreamGroups = 0;
    for (UINT32 i=0; i<m_ulNumStreamGroups; i++)
    {
	if (m_ppStreamGroup[i]->IsInitalRateDescCommitted())
	    ulNumActiveStreamGroups++;
    }

    return ulNumActiveStreamGroups;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::GetLogicalStream
// Purpose:
//  Used to get a logical stream enumerator
STDMETHODIMP
CUberStreamManager::GetLogicalStream(UINT32 ulLogicalStreamNum, REF(IHXRateDescEnumerator*)pMgr)
{
    HX_RESULT res = HXR_OK;

    UINT32 ulStreamGroupNum = 0;
    res = FindStreamGroupByLogicalStream(ulLogicalStreamNum, ulStreamGroupNum);

    if (SUCCEEDED(res))
	res = m_ppStreamGroup[ulStreamGroupNum]->GetLogicalStream(ulLogicalStreamNum, pMgr);

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::GetSelectedLogicalStreamNum
// Purpose:
//  Used to get the selected logical stream number for the given stream group
STDMETHODIMP
CUberStreamManager::GetSelectedLogicalStreamNum(UINT32 ulStreamGroupNum, REF(UINT32)ulSelectedLogicalStreamNum)
{
    // Validate params
    if (!m_ppStreamGroup || ulStreamGroupNum>=m_ulNumStreamGroups || !m_ppStreamGroup[ulStreamGroupNum])
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    return m_ppStreamGroup[ulStreamGroupNum]->GetSelectedLogicalStreamNum(ulSelectedLogicalStreamNum);
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::FindStreamGroupByLogicalStream
// Purpose:
//  Maps a logical stream number to a stream group number
STDMETHODIMP
CUberStreamManager::FindStreamGroupByLogicalStream(UINT32 ulLogicalStream, REF(UINT32)ulStreamGroup)
{
    // Validate state, params
    if (!m_aulLogicalStreamToStreamGroup || ulLogicalStream >= m_ulNumLogicalStreams ||
	m_aulLogicalStreamToStreamGroup[ulLogicalStream] == kulInvalidStreamGroupNum)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    ulStreamGroup = m_aulLogicalStreamToStreamGroup[ulLogicalStream];

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::ModifyHeaders
// Purpose:
//  Adds avg/max bitrate properties to the given logical stream header
HX_RESULT
CUberStreamManager::ModifyHeaders(IHXValues* pHdr, BOOL bUseAnnexG)
{   
    HX_ASSERT(pHdr);
    UINT32 ulVal = 0;
    IHXRateDescription* pRateDesc = NULL;
    
    // Get the current rate descriptor for the logical stream
    UINT32 ulStreamGroupNum = 0;
    HX_RESULT theErr = pHdr->GetPropertyULONG32("StreamGroupNumber", ulStreamGroupNum);  
    if (HXR_OK == theErr)
    {
	HX_ASSERT(ulStreamGroupNum < m_ulNumStreamGroups);
	theErr = m_ppStreamGroup[ulStreamGroupNum]->GetCurrentRateDesc(pRateDesc);
    }

    // Set avg/max bitrate properties
    if (HXR_OK == theErr)
    {
        if (m_ulNumStreamGroups == m_ulNumLogicalStreams && 
            SUCCEEDED(pRateDesc->GetAvgRate(ulVal)))
        {
    	    pHdr->SetPropertyULONG32("AvgBitRate", ulVal);
        }    
        if (bUseAnnexG)
        {
            pHdr->SetPropertyULONG32("UseAnnexG", 1);

            // Only set the annex G values if they have not already been
            // set by the source

            if (FAILED(pHdr->GetPropertyULONG32("X-DecByteRate", ulVal)) &&
                SUCCEEDED(pRateDesc->GetMaxRate(ulVal)) && ulVal >= 8)
            {
                // Convert MaxBitRate to bytes/sec, round up
                pHdr->SetPropertyULONG32("X-DecByteRate", (ulVal + 7) >> 3);
            }

            if (FAILED(pHdr->GetPropertyULONG32("X-InitPreDecBufPeriod", 
                ulVal)) && 
                SUCCEEDED(pRateDesc->GetPreroll(ulVal)) && ulVal)
            {
                // Convert preroll value to 90kHz
                pHdr->SetPropertyULONG32("X-InitPreDecBufPeriod", ulVal * 90);
            }
        }

        pRateDesc->Release();
    }

    // ok if this fails for now
    return HXR_OK;        
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::SetRateDesc
// Purpose:
//  Subscribes to the appropriate physical streams for the given rate
// Returns:
//  HXR_OK if a new rate was selected.  
//  HXR_IGNORE if the new rate is the same as the current rate
//  Otherwise, a FAIL code.
STDMETHODIMP
CUberStreamManager::SetAggregateRateDesc(IHXRateDescription* pRateDescription, IHXRateDescResponse* pResp)
{
    HX_RESULT res = HXR_OK;

    // Validate state
    if (m_ulNumStreamGroups == 0 || !m_ppStreamGroup)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Note: Ensure that rate description came from the uber container -- this could eventually 
    // be updated to handle unknown/externally generate rate descriptions.
    CBandwidthGrouping* pBandwidthGrouping = NULL;
    if (SUCCEEDED(res))
    {
	UINT32 ulIndex = 0;
	res = m_cUberContainer.GetRateDescIndex(pRateDescription, ulIndex);

	if (SUCCEEDED(res))
	{
	    pBandwidthGrouping = m_cUberContainer.GetBandwidthGrouping(ulIndex);
	    if (!pBandwidthGrouping)
		res = HXR_FAIL;
	}
    }

    // Ignore shift if it wouldn't change the current bandwidth grouping
    if (SUCCEEDED(res))
    {        
	CBandwidthGrouping* pCurBandwidthGrouping = m_cUberContainer.GetCurrentBandwidthGrouping();
	if (pBandwidthGrouping == pCurBandwidthGrouping)
	    res = HXR_IGNORE;

	HX_RELEASE(pCurBandwidthGrouping);
    }

    // Use the bandwidth partitioning info to allocate bandwidth
    // between logical streams and perform the rate shift for each stream
    if (SUCCEEDED(res))
    {
	for (UINT32 i = 0; i < pBandwidthGrouping->GetNumBandwidthAllocations() && SUCCEEDED(res); i++)
	{   
	    // Find exact matching rate
	    IHXRateDescription* pRateDesc = NULL;
	    res = m_ppStreamGroup[i]->FindRateDescByExactAvgRate(pBandwidthGrouping->GetBandwidthAllocationArray()[i], !m_bInitialRateDescCommitted, m_bInitialRateDescCommitted, pRateDesc);

	    if (SUCCEEDED(res))
		res = m_ppStreamGroup[i]->SetRateDesc(pRateDesc, NULL);

	    HX_RELEASE(pRateDesc);
	    if (HXR_IGNORE == res)
	    {   
		// ok if one of streams didn't shift
		res = HXR_OK;
	    }
	    else
	    {
		UpdateShiftPending();
	    }
	}
    }

    // Update the current rate description
    if (SUCCEEDED(res))
    {
	UINT32 ulIndex = 0;
	res = m_cUberContainer.GetRateDescIndex(pBandwidthGrouping, ulIndex);

	if (SUCCEEDED(res))
	    res = m_cUberContainer.SetCurrentRateDesc(ulIndex);
    }

    // Note: Implement shift response as needed.  The asm stream filter currently handles this.
    if (SUCCEEDED(res) && pResp)
    {
	HX_ASSERT(FALSE);
    }

    HX_RELEASE(pBandwidthGrouping);

    HX_ASSERT(SUCCEEDED(res) || res==HXR_IGNORE);

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::UpshiftAggregate
// Purpose:
//  Allocates bandwidth to each logical stream, and upshifts to the
//  appropriate physical streams
// Notes:
//  An ulRate of 0 corresponds to taking the next highest rate
// Returns:
//  HXR_OK if a new rate was selected.  
//  HXR_IGNORE if the new rate is the same as the current rate
//  HXR_NOTENOUGH_BANDWIDTH if there were no available streams for the given bandwidth
//  Otherwise, a FAIL code.
STDMETHODIMP
CUberStreamManager::UpshiftAggregate(UINT32 ulRate, IHXRateDescResponse* pResp)
{
    HX_RESULT res = HXR_OK;

    // Validate state -- can't change rate until stream selection has been finalized
    if (!m_bInitialRateDescCommitted)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Ignore upshift if new rate is lower than current rate
    if ((ulRate != 0) && SUCCEEDED(res))
    {
	CBandwidthGrouping* pCurBandwidthGrouping = m_cUberContainer.GetCurrentBandwidthGrouping();
	if (!pCurBandwidthGrouping)
	    res = HXR_FAIL;

	if (SUCCEEDED(res))
	{
	    UINT32 ulCurRate = pCurBandwidthGrouping->GetBandwidth();

	    if (ulRate < ulCurRate)
		res = HXR_IGNORE;
	}

	HX_RELEASE(pCurBandwidthGrouping);
    }

    // Find the bandwidth grouping to shift to
    IHXRateDescription* pBandwidthGrouping = NULL;
    if (SUCCEEDED(res))
    {
	// If bitrate was specified, search for ulRate
	res = HXR_FAIL;
	if (ulRate > 0)
	{	
	    res = m_cUberContainer.FindRateDescByClosestAvgRate(ulRate, FALSE, TRUE, pBandwidthGrouping);

	    // If an appropriate bitrate couldn't be found, find the next closest
	    if (FAILED(res))
	    {
	    	res = m_cUberContainer.FindRateDescByMidpoint(ulRate, FALSE, TRUE, pBandwidthGrouping);		    
	    }	    
	}

	// If an appropriate bitrate couldn't be found, just get the next highest bitrate
	if (FAILED(res))
	{
	    res = m_cUberContainer.GetNextSwitchableRateDesc(pBandwidthGrouping);	
	}

	// Couldn't find an appropriate rate desc
	if (FAILED(res))
	{
	    res = HXR_IGNORE;
	}
    }

    // Return HXR_IGNORE if the new bandwidth grouping is the same as the current one
    if (SUCCEEDED(res))
    {
	IHXRateDescription* pCurBandwidthGrouping = NULL;
	res = m_cUberContainer.GetCurrentRateDesc(pCurBandwidthGrouping);

	if (SUCCEEDED(res) && pCurBandwidthGrouping == pBandwidthGrouping)
	    res = HXR_IGNORE;

	HX_RELEASE(pCurBandwidthGrouping);
    }

    UINT32 ulShiftsIgnored = 0;

    // Perform the actual upshift for each logical stream
    if (SUCCEEDED(res))
    {
	for (UINT32 i = 0; i < pBandwidthGrouping->GetNumBandwidthAllocations() && SUCCEEDED(res); i++)
	{   
	    res = m_ppStreamGroup[i]->Upshift(pBandwidthGrouping->GetBandwidthAllocationArray()[i], NULL);

	    if (HXR_IGNORE == res)
	    {   
		// ok if one of streams didn't shift
		res = HXR_OK;
                ulShiftsIgnored++;
	    }
	    else
	    {
		UpdateShiftPending();
	    }
	}
    }

    // Update the current rate description
    if (SUCCEEDED(res))
    {
        if (ulShiftsIgnored > 0)
        {
            UpdateAggregateRateDesc();
        }
        else 
        {
	    UINT32 ulIndex = 0;
	    res = m_cUberContainer.GetRateDescIndex(pBandwidthGrouping, ulIndex);

	    if (SUCCEEDED(res))
            {
	        res = m_cUberContainer.SetCurrentRateDesc(ulIndex);
            }
        }
    }

    // XXXLY - need to respond to upshift here
    if (SUCCEEDED(res) && pResp)
    {
	HX_ASSERT(FALSE);
    }

    HX_RELEASE(pBandwidthGrouping);

    HX_ASSERT(SUCCEEDED(res) || res==HXR_IGNORE || res == HXR_NOTENOUGH_BANDWIDTH);
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::DownshiftAggregate
// Purpose:
//  Allocates bandwidth to each logical stream, and downshifts to the
//  appropriate physical streams
// Notes:
//  An ulRate of 0 corresponds to taking the next lowest rate
// Returns:
//  HXR_OK if a new rate was selected.  
//  HXR_IGNORE if the new rate is the same as the current rate
//  HXR_NOTENOUGH_BANDWIDTH if there were no available streams for the given bandwidth
//  Otherwise, a FAIL code.
STDMETHODIMP
CUberStreamManager::DownshiftAggregate(UINT32 ulRate, IHXRateDescResponse* pResp)
{
    HX_RESULT res = HXR_OK;

    // Validate state -- can't change rate until stream selection has been finalized
    if (!m_bInitialRateDescCommitted)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Downshift a single step if the new rate is greater than current rate
    if ((ulRate != 0) && SUCCEEDED(res))
    {
	CBandwidthGrouping* pCurBandwidthGrouping = m_cUberContainer.GetCurrentBandwidthGrouping();
	if (!pCurBandwidthGrouping)
	    res = HXR_FAIL;

	if (SUCCEEDED(res))
	{
	    UINT32 ulCurRate = pCurBandwidthGrouping->GetBandwidth();

	    if (ulRate > ulCurRate)
		ulRate = 0;
	}

	HX_RELEASE(pCurBandwidthGrouping);
    }

    // Find the bandwidth grouping to shift to
    IHXRateDescription* pBandwidthGrouping = NULL;
    if (SUCCEEDED(res))
    {
	// If bitrate was specified, search for ulRate
	res = HXR_FAIL;
	if (ulRate > 0)
	{	
	    res = m_cUberContainer.FindRateDescByClosestAvgRate(ulRate, FALSE, TRUE, pBandwidthGrouping);

	    // If an appropriate bitrate couldn't be found, find the next closest
	    if (FAILED(res))
	    {
	    	res = m_cUberContainer.FindRateDescByMidpoint(ulRate, FALSE, TRUE, pBandwidthGrouping);		    
	    }	    
	}

	// If an appropriate bitrate couldn't be found, just get the next lowest bitrate
	if (FAILED(res))
	{
	    res = m_cUberContainer.GetPrevSwitchableRateDesc(pBandwidthGrouping);	
	}

	// Couldn't find an appropriate rate desc
	if (FAILED(res))
	{
	    res = HXR_IGNORE;
	}
    }

    // Return HXR_IGNORE if the new bandwidth grouping is the same as the current one
    if (SUCCEEDED(res))
    {
	IHXRateDescription* pCurBandwidthGrouping = NULL;
	res = m_cUberContainer.GetCurrentRateDesc(pCurBandwidthGrouping);

	if (SUCCEEDED(res) && pCurBandwidthGrouping == pBandwidthGrouping)
	    res = HXR_IGNORE;

	HX_RELEASE(pCurBandwidthGrouping);
    }

    UINT32 ulShiftsIgnored = 0;

    // Perform the actual downshift for each logical stream
    if (SUCCEEDED(res))
    {
	for (UINT32 i = 0; i < pBandwidthGrouping->GetNumBandwidthAllocations() && SUCCEEDED(res); i++)
	{   
	    res = m_ppStreamGroup[i]->Downshift(pBandwidthGrouping->GetBandwidthAllocationArray()[i], NULL);

	    if (HXR_IGNORE == res)
	    {   
		// ok if one of streams didn't shift
		res = HXR_OK;
                ulShiftsIgnored++;
	    }
	    else
	    {
		UpdateShiftPending();
	    }
	}
    }

    // Update the current rate description
    if (SUCCEEDED(res)) 
    {
        if (ulShiftsIgnored > 0)
        {
            UpdateAggregateRateDesc();
        }
        else 
        {
	    UINT32 ulIndex = 0;
	    res = m_cUberContainer.GetRateDescIndex(pBandwidthGrouping, ulIndex);

	    if (SUCCEEDED(res))
            {
	        res = m_cUberContainer.SetCurrentRateDesc(ulIndex);
            }
        }
    }

    // Note: Implement shift response as needed.  The asm stream filter currently handles this.
    if (SUCCEEDED(res) && pResp)
    {
	HX_ASSERT(FALSE);
    }

    HX_RELEASE(pBandwidthGrouping);

    HX_ASSERT(SUCCEEDED(res) || res==HXR_IGNORE || res == HXR_NOTENOUGH_BANDWIDTH);
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::SetStreamGroupRateDesc
// Purpose:
//  Sets stream group's rate desc
STDMETHODIMP
CUberStreamManager::SetStreamGroupRateDesc(UINT32 ulStreamGroupNum, 
                                           UINT32 ulLogicalStreamNum, 
                                           IHXRateDescription* pRateDesc, 
                                           IHXStreamRateDescResponse* pResp)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!m_ppStreamGroup || ulStreamGroupNum>=m_ulNumStreamGroups || !m_ppStreamGroup[ulStreamGroupNum])
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    res = m_ppStreamGroup[ulStreamGroupNum]->SetRateDesc(pRateDesc, pResp, 
            ulLogicalStreamNum);

    // Update aggregate rate desc -- OK if it fails.  Also track shift pending status
    if (SUCCEEDED(res))
    {
	UpdateShiftPending();
	UpdateAggregateRateDesc();
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::GetCurrentStreamGroupRateDesc
// Purpose:
//  Sets stream group's rate desc
STDMETHODIMP
CUberStreamManager::GetCurrentStreamGroupRateDesc(UINT32 ulStreamGroupNum, REF(IHXRateDescription*)pRateDesc)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!m_ppStreamGroup || ulStreamGroupNum>=m_ulNumStreamGroups || !m_ppStreamGroup[ulStreamGroupNum])
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    res = m_ppStreamGroup[ulStreamGroupNum]->GetCurrentRateDesc(pRateDesc);

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::UpshiftStreamGroup
// Purpose:
//  Upshifts stream group
STDMETHODIMP
CUberStreamManager::UpshiftStreamGroup(UINT32 ulStreamGroupNum, 
                                       UINT32 ulRate, 
                                       IHXStreamRateDescResponse* pResp)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!m_ppStreamGroup || ulStreamGroupNum>=m_ulNumStreamGroups || !m_ppStreamGroup[ulStreamGroupNum])
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    res =  m_ppStreamGroup[ulStreamGroupNum]->Upshift(ulRate, pResp);

    // Update aggregate rate desc -- OK if it fails.  Also track shift pending status
    if (SUCCEEDED(res))
    {
	UpdateShiftPending();
	UpdateAggregateRateDesc();
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::DownshiftStreamGroup
// Purpose:
//  Downshifts stream group
STDMETHODIMP
CUberStreamManager::DownshiftStreamGroup(UINT32 ulStreamGroupNum, 
                                         UINT32 ulRate, 
                                         IHXStreamRateDescResponse* pResp)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!m_ppStreamGroup || ulStreamGroupNum>=m_ulNumStreamGroups || !m_ppStreamGroup[ulStreamGroupNum])
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    res = m_ppStreamGroup[ulStreamGroupNum]->Downshift(ulRate, pResp);

    // Update aggregate rate desc -- OK if it fails.  Also track shift pending status
    if (SUCCEEDED(res))
    {
	UpdateShiftPending();
	UpdateAggregateRateDesc();
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::SubscribeLogicalStreamRule
// Purpose:
//  Subscribes to a logical stream's rule
STDMETHODIMP
CUberStreamManager::SubscribeLogicalStreamRule(UINT32 ulLogicalStreamNum, 
                                               UINT32 ulRuleNum, 
                                               IHXStreamRateDescResponse* pResp)
{
    HX_RESULT res = HXR_OK;

    UINT32 ulStreamGroupNum = 0;
    res = FindStreamGroupByLogicalStream(ulLogicalStreamNum, ulStreamGroupNum);

    if (SUCCEEDED(res))
	res = m_ppStreamGroup[ulStreamGroupNum]->SubscribeLogicalStreamRule(ulLogicalStreamNum, ulRuleNum, pResp);

    // Update aggregate rate desc -- OK if it fails.  Also track shift pending status
    if (SUCCEEDED(res))
    {
	UpdateShiftPending();
	UpdateAggregateRateDesc();
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::UnsubscribeLogicalStreamRule
// Purpose:
//  Unsubscribes from a logical stream's rule
STDMETHODIMP
CUberStreamManager::UnsubscribeLogicalStreamRule(UINT32 ulLogicalStreamNum, 
                                                 UINT32 ulRuleNum, 
                                                 IHXStreamRateDescResponse* pResp)
{
    HX_RESULT res = HXR_OK;

    UINT32 ulStreamGroupNum = 0;
    res = FindStreamGroupByLogicalStream(ulLogicalStreamNum, ulStreamGroupNum);

    if (SUCCEEDED(res))
	res = m_ppStreamGroup[ulStreamGroupNum]->UnsubscribeLogicalStreamRule(ulLogicalStreamNum, ulRuleNum, pResp);

    // Update aggregate rate desc -- OK if it fails.  Also track shift pending status
    if (SUCCEEDED(res))
    {
	UpdateShiftPending();
	UpdateAggregateRateDesc();
    }

    return res;
}




/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::CommitInitialStreamGroupRateDesc
// Purpose:
//  Downshifts stream group
STDMETHODIMP
CUberStreamManager::CommitInitialStreamGroupRateDesc(UINT32 ulStreamGroupNum)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!m_ppStreamGroup || ulStreamGroupNum>=m_ulNumStreamGroups || !m_ppStreamGroup[ulStreamGroupNum])
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Quick sanity check -- not really a problem if called more than once
    HX_ASSERT(!m_bInitialRateDescCommitted);

    BOOL bEnableAudioRateShifting = TRUE;

    if (m_pQoSConfig)
    {
	INT32 lTemp = 0;
    	if (m_pQoSConfig->GetConfigInt(QOS_CFG_MDP, lTemp) == HXR_OK)
        {
            bEnableAudioRateShifting = (BOOL)lTemp;
        }
    }

    res = m_ppStreamGroup[ulStreamGroupNum]->CommitInitialRateDesc(bEnableAudioRateShifting);

    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::IsInitalStreamGroupRateDescCommitted
// Purpose:
//  Downshifts stream group
STDMETHODIMP_(BOOL)
CUberStreamManager::IsInitalStreamGroupRateDescCommitted(UINT32 ulStreamGroupNum)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!m_ppStreamGroup || ulStreamGroupNum>=m_ulNumStreamGroups || !m_ppStreamGroup[ulStreamGroupNum])
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    return m_ppStreamGroup[ulStreamGroupNum]->IsInitalRateDescCommitted();
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::CreateBandwidthGrouping
// Purpose:
//  Creates bandwidth grouping from a set of physical streams
HX_RESULT
CUberStreamManager::CreateBandwidthGrouping(CPhysicalStream* apPhysicalStream[])
{
    HX_RESULT res = HXR_OK;

    // Create bandwidth grouping
    CBandwidthGrouping* pBandwidthGrouping = new CBandwidthGrouping(0);
    if (pBandwidthGrouping)
	pBandwidthGrouping->AddRef();
    else
	res = HXR_OUTOFMEMORY;

    // Init bandwidth grouping with properties from physical stream array
    if (SUCCEEDED(res))
    {
	pBandwidthGrouping->m_ulNumBandwidthAllocations = m_ulNumStreamGroups;
	pBandwidthGrouping->m_aulBandwidthAllocation = new UINT32[pBandwidthGrouping->m_ulNumBandwidthAllocations];

	for (UINT32 i=0; i<m_ulNumStreamGroups; i++)
	{
	    // Note: Physical streams from inactive stream groups will be NULL
	    if (apPhysicalStream[i])
	    {
		pBandwidthGrouping->m_aulBandwidthAllocation[i] = apPhysicalStream[i]->GetBandwidth();
		pBandwidthGrouping->m_ulAvgRate += apPhysicalStream[i]->GetBandwidth();

		pBandwidthGrouping->m_ulMaxRate += apPhysicalStream[i]->m_ulMaxRate;

		pBandwidthGrouping->m_ulPredata += apPhysicalStream[i]->m_ulPredata;

		if (pBandwidthGrouping->m_ulPreroll < apPhysicalStream[i]->m_ulPreroll)
		    pBandwidthGrouping->m_ulPreroll = apPhysicalStream[i]->m_ulPreroll;
	    }

	    // Handle inactive streamgroups
	    else
	    {
		pBandwidthGrouping->m_aulBandwidthAllocation[i] = 0;
	    }
	}
    }

    // If everything went OK, add the bandwidth grouping
    if (SUCCEEDED(res))
	res = m_cUberContainer.AddBandwidthGrouping(pBandwidthGrouping);

    HX_RELEASE(pBandwidthGrouping);
    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::CreateBandwidthGroupingFromCurrentStreams
// Purpose:
//  Creates bandwidth grouping based on current stream group settings
HX_RESULT
CUberStreamManager::CreateBandwidthGroupingFromCurrentStreams()
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!m_ppStreamGroup || !m_bInitialRateDescCommitted)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Create array containing the current physical stream from each active stream group
    CPhysicalStream** apPhysicalStream = NULL;
    if (SUCCEEDED(res))
    {
	apPhysicalStream = new CPhysicalStream*[m_ulNumStreamGroups];
    
	if (!apPhysicalStream)
	    res = HXR_OUTOFMEMORY;
	else
	    memset(apPhysicalStream, 0, sizeof(CPhysicalStream*)*m_ulNumStreamGroups);

	for (UINT32 i=0; i<m_ulNumStreamGroups && SUCCEEDED(res); i++)
	{
	    if (m_ppStreamGroup[i]->IsInitalRateDescCommitted())
	    {
		apPhysicalStream[i] = m_ppStreamGroup[i]->GetCurrentPhysicalStream();    	    
	    }
	}
    }

    // Create bandwidth grouping from current stream array
    if (SUCCEEDED(res))
	res = CreateBandwidthGrouping(apPhysicalStream);

    // Cleanup array
    for (UINT32 i=0; i<m_ulNumStreamGroups && apPhysicalStream; i++)
    {
	HX_RELEASE(apPhysicalStream[i]);
    }
    HX_VECTOR_DELETE(apPhysicalStream);

    HX_ASSERT(SUCCEEDED(res));
    return res;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::UpdateAggregateRateDesc
// Purpose:
//  Sets aggregate rate desc based on stream groups' rate desc
HX_RESULT
CUberStreamManager::UpdateAggregateRateDesc()
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!m_ppStreamGroup || m_ulNumStreamGroups == 0)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Prior to stream selection, rates must have been set for all stream groups (since the bandwidth
    // grouping will have an entry for each stream group).
    if (!m_bInitialRateDescCommitted && GetNumActiveStreamGroups() != m_ulNumStreamGroups)
    {
	res = HXR_FAIL;
    }

    // Otherwise, add up the total bandwidth to determine the current bandwidth grouping
    else
    {
	// Determine the total bitrate
	UINT32 ulTotalBitrate = 0;
	for (UINT32 i=0; i<m_ulNumStreamGroups && SUCCEEDED(res); i++)
	{
	    // Skip inactive stream groups
	    if (m_ppStreamGroup[i]->IsInitalRateDescCommitted())
	    {
		// Get the current rate desc
		IHXRateDescription* pRateDesc = NULL;
		res = m_ppStreamGroup[i]->GetCurrentRateDesc(pRateDesc);		

		UINT32 ulAvgRate = 0;
		if (SUCCEEDED(res))
		    res = pRateDesc->GetAvgRate(ulAvgRate);

		if (SUCCEEDED(res))
		    ulTotalBitrate += ulAvgRate;

		HX_RELEASE(pRateDesc);
	    }
	}

	// Use aggregate bitrate to determine the current bandwidth grouping
	if (SUCCEEDED(res))
	{
	    // Find exact rate match
	    IHXRateDescription* pRateDesc = NULL;
	    res = FindRateDescByExactAvgRate(ulTotalBitrate, !m_bInitialRateDescCommitted, m_bInitialRateDescCommitted, pRateDesc);

            if (FAILED(res))
            {
                if (m_bInitialRateDescCommitted)
                {
                    HX_ASSERT(!pRateDesc);
                    res = CreateBandwidthGroupingFromCurrentStreams();

                    if (SUCCEEDED(res))
                    {
                        //XXXDPL - Might be better to tweak outparams of CreateBandwidthGrouping() to 
                        // avoid calling this twice...
                        res = FindRateDescByExactAvgRate(ulTotalBitrate, 
                                                        !m_bInitialRateDescCommitted, 
                                                        m_bInitialRateDescCommitted, 
                                                        pRateDesc);

                        // We just created it the grouping!
                        HX_ASSERT(SUCCEEDED(res) && pRateDesc);
                    }
                }

	        // Otherwise, if a suitable bandwidth grouping could not be found, set it to an invalid index
	        // This is expected behavior until all streamgroups have been initialized                
                else
                {                    
	            m_cUberContainer.SetCurrentRateDesc(kulInvalidRateDescIndex);
                }
            }

	    UINT32 ulIndex = 0;

	    if (SUCCEEDED(res))
            {
		res = m_cUberContainer.GetRateDescIndex(pRateDesc, ulIndex);
            }

	    if (SUCCEEDED(res))
            {
		res = m_cUberContainer.SetCurrentRateDesc(ulIndex);
            }

	    HX_RELEASE(pRateDesc);
	}
    }

    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::GetAvgRate
// Purpose:
//  Helper function to get avg bitrate from a streamgroup's rate description
UINT32
CUberStreamManager::GetAvgRate(UINT32 ulStreamGroupNum, UINT32 ulRateDescNum)
{
    IHXRateDescription* pRateDesc = NULL;
    HX_RESULT res = m_ppStreamGroup[ulStreamGroupNum]->GetRateDescription(ulRateDescNum, pRateDesc);

    UINT32 ulAvgRate = 0;
    if (SUCCEEDED(res))
	res = pRateDesc->GetAvgRate(ulAvgRate);

    HX_RELEASE(pRateDesc);

    HX_ASSERT(SUCCEEDED(res));
    return ulAvgRate;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::GetLowestNonThinningAvgRate
// Purpose:
//  Get the lowest non-thinning avg rate stream for the given streamgroup
UINT32
CUberStreamManager::GetLowestNonThinningAvgRate(UINT32 ulStreamGroupNum)
{
    HX_RESULT res = HXR_OK;    
    
    //  Search for the lowest non-thinning avg rate stream 
    UINT32 ulAvgRate = 0;
    for (UINT32 i=0; i<m_ppStreamGroup[ulStreamGroupNum]->GetNumPhysicalStreams() && ulAvgRate == 0; i++)
    {
	CPhysicalStream* pPhysicalStream = m_ppStreamGroup[ulStreamGroupNum]->GetPhysicalStream(i);

	if (pPhysicalStream && !pPhysicalStream->m_bIsThinningStream)
	    ulAvgRate = pPhysicalStream->m_ulAvgRate;

	HX_RELEASE(pPhysicalStream);
    }

    return ulAvgRate;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CUberStreamManager::SynthesizeUberRulebook
// Purpose:
//  Creates stream pairings based on the ratio of the highest and lowest
//  rate descriptions within each streamgroup.
HX_RESULT
CUberStreamManager::SynthesizeUberRulebook(BOOL bSkipInactiveStreamGroups)
{
    HX_RESULT res = HXR_OK;

    // Validate state
    if (m_ulNumStreamGroups == 0 || !m_ppStreamGroup)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Find the "primary" streamgroup -- the one with the most physical streams
    UINT32 ulPrimaryStreamGroupNum = 0;
    if (SUCCEEDED(res))
    {	
	UINT32 ulMaxNumPhysicalStreams = 0;
	for (UINT32 i=0; i<m_ulNumStreamGroups; i++)
	{
	    if (m_ppStreamGroup[i]->GetNumRateDescriptions() > ulMaxNumPhysicalStreams)
	    {
		ulPrimaryStreamGroupNum = i;
		ulMaxNumPhysicalStreams = m_ppStreamGroup[i]->GetNumRateDescriptions();
	    }
	}
    }

    // Remove/re-init bandwidth grouping container
    if (SUCCEEDED(res))
	res = m_cUberContainer.Cleanup();

    const UINT32 ulNumPrimaryRateDesc = m_ppStreamGroup[ulPrimaryStreamGroupNum]->GetNumRateDescriptions();
    if (SUCCEEDED(res))
	res = m_cUberContainer.Init(ulNumPrimaryRateDesc);

    // Calculate the bandwidth ratios of the lowest bitrate streams
    UINT32* pulLowerBandwidthRatio = NULL;
    UINT32 ulPrimaryStreamLowestAvgRate = 0;
    if (SUCCEEDED(res))
    {
	pulLowerBandwidthRatio = new UINT32[m_ulNumStreamGroups];
	if (!pulLowerBandwidthRatio)
	    res = HXR_OUTOFMEMORY;

	if (SUCCEEDED(res))
	{
	    ulPrimaryStreamLowestAvgRate = GetLowestNonThinningAvgRate(ulPrimaryStreamGroupNum);
	    
	    if (ulPrimaryStreamLowestAvgRate == 0)
	    {
		HX_ASSERT(FALSE);
		res = HXR_FAIL;
	    }

	    for (UINT32 i=0; i<m_ulNumStreamGroups && SUCCEEDED(res); i++)
	    {
		// Skip inactive streamgroups and thinning streams
		if ((!bSkipInactiveStreamGroups || m_ppStreamGroup[i]->IsInitalRateDescCommitted()) && m_ppStreamGroup[i]->GetNumRateDescriptions() > 0)
		{
		    pulLowerBandwidthRatio[i] = GetLowestNonThinningAvgRate(i) * 1000 / ulPrimaryStreamLowestAvgRate;

		    // Ensure that each stream has at least a tiny bit of bandwidth allocated
		    if (pulLowerBandwidthRatio[i] == 0)
			pulLowerBandwidthRatio[i] = 1;
		}
		else
		{
		    pulLowerBandwidthRatio[i] = 0;
		}
	    }
	}
    }


    // Calculate the bandwidth ratios of the higher bitrate streams
    UINT32* pulHigherBandwidthRatio = NULL;    
    UINT32 ulPrimaryStreamHighestAvgRate = 0;
    if (SUCCEEDED(res))
    {
	pulHigherBandwidthRatio = new UINT32[m_ulNumStreamGroups];
	if (!pulHigherBandwidthRatio)
	    res = HXR_OUTOFMEMORY;

	if (SUCCEEDED(res))
	{	    
	    ulPrimaryStreamHighestAvgRate = GetAvgRate(ulPrimaryStreamGroupNum, m_ppStreamGroup[ulPrimaryStreamGroupNum]->GetNumRateDescriptions()-1);

	    if (ulPrimaryStreamHighestAvgRate == 0)
	    {
		HX_ASSERT(FALSE);
		res = HXR_FAIL;
	    }

	    for (UINT32 i=0; i<m_ulNumStreamGroups && SUCCEEDED(res); i++)
	    {
		// Skip inactive streamgroups, if necessary
		if ((!bSkipInactiveStreamGroups || m_ppStreamGroup[i]->IsInitalRateDescCommitted()) && m_ppStreamGroup[i]->GetNumRateDescriptions() > 0)
		{
		    pulHigherBandwidthRatio[i] = GetAvgRate(i, m_ppStreamGroup[i]->GetNumRateDescriptions()-1) * 1000 / ulPrimaryStreamHighestAvgRate;

		    // Ensure that each stream has at least a tiny bit of bandwidth allocated
		    if (pulHigherBandwidthRatio[i] == 0)
			pulHigherBandwidthRatio[i] = 1;
		}
		else
		{
		    pulHigherBandwidthRatio[i] = 0;
		}
	    }
	}
    }

    CPhysicalStream** apPhysicalStream = NULL;
    if (SUCCEEDED(res))
    {
	apPhysicalStream = new CPhysicalStream*[m_ulNumStreamGroups];
	if (!apPhysicalStream)
	    res = HXR_OUTOFMEMORY;
	else
	    memset(apPhysicalStream, 0, sizeof(CPhysicalStream*)*m_ulNumStreamGroups);
    }

    // Track the last avg rate seleceted
    UINT32* pulLastAvgRate = NULL;
    if (SUCCEEDED(res))
    {
    	pulLastAvgRate = new UINT32[m_ulNumStreamGroups];
	if (!pulLastAvgRate)
	    res = HXR_OUTOFMEMORY;
	else
	    memset(pulLastAvgRate, 0, sizeof(UINT32)*m_ulNumStreamGroups);
    }

    // For each rate description in the primary stream group, find appropriate stream
    // pairings, and create a bandwidth grouping struct
    if (SUCCEEDED(res))
    {
	// Determine boundary point for low/high bandwidth ratio -- prefer to use high
	// bandwidth ratio for the majority of streams
	UINT32 ulLowHighBoundary = (UINT32)(ulPrimaryStreamLowestAvgRate + ulPrimaryStreamHighestAvgRate) * .3;	    
	if (ulLowHighBoundary < ulPrimaryStreamLowestAvgRate)
	    ulLowHighBoundary = ulPrimaryStreamLowestAvgRate;

	for (UINT32 i=0; i<ulNumPrimaryRateDesc && SUCCEEDED(res); i++)
	{
	    UINT32 ulPrimaryAvgRate = GetAvgRate(ulPrimaryStreamGroupNum, i);

	    // Select bandwidth ratio
	    UINT32* pulBandwidthRatio = pulLowerBandwidthRatio;
	    if (ulPrimaryAvgRate > ulLowHighBoundary)
		pulBandwidthRatio = pulHigherBandwidthRatio;

	    // Use the bandwidth ratio array to find appropriate streaming pairings for 
	    // the current primary rate description
	    for (UINT32 j=0; j<m_ulNumStreamGroups && SUCCEEDED(res); j++)
	    {
		// Skip streamgroups that don't have any bandwidth allocated
		if (pulBandwidthRatio[j] > 0)
		{
		    UINT32 ulIdealRate = ulPrimaryAvgRate * pulBandwidthRatio[j] / 1000;

		    // Ensure that a streamgroup's bitrate never decreases across bandwidth groupings (only an 
		    // issue for rate descriptions near the low/high bandwidth ratio boundarty)
		    if (ulIdealRate < pulLastAvgRate[j])
			ulIdealRate = pulLastAvgRate[j];

		    res = m_ppStreamGroup[j]->FindRateDescByMidpoint(ulIdealRate, TRUE, TRUE, ((IHXRateDescription*&)apPhysicalStream[j]));

		    if (SUCCEEDED(res))
			res = apPhysicalStream[j]->GetAvgRate(pulLastAvgRate[j]);
		}

		else
		{
		    apPhysicalStream[j] = NULL;
		}
	    }

	    // Create the bandwidth grouping
	    if (SUCCEEDED(res))
		res = CreateBandwidthGrouping(apPhysicalStream);

	    for (UINT32 k=0; k<m_ulNumStreamGroups; k++)
	    {
		HX_RELEASE(apPhysicalStream[k]);
	    }
	}
    }


    HX_VECTOR_DELETE(pulLastAvgRate);
    HX_VECTOR_DELETE(apPhysicalStream);

    HX_VECTOR_DELETE(pulLowerBandwidthRatio);
    HX_VECTOR_DELETE(pulHigherBandwidthRatio);

    HX_ASSERT(SUCCEEDED(res));
    return res;
}


