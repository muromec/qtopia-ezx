/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsm2.cpp,v 1.21 2007/07/06 21:58:15 jfinnecy Exp $
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

#include "hxsm2.h"
#include "hxsmutil.h"
#include "ihxpckts.h"
#include "hxprefs.h"
#include "hxpref.h"
#include "hxbsrc.h" /* HXSource */
#include "asmrulep.h" /* ASMRuleBook */
#include "hxbuffer.h" /* CHXBuffer */
#include "chxpckts.h" /* CHXHeader */
#include "safestring.h" /* SafeSprintf */
#include "rtspif.h"     /* RTSPSubscription */
#include "hxengin.h" /* IHXConnectionBWInfo */

class ASMStreamInfo2;

class ASMSourceInfo2
{
public:
    ASMSourceInfo2(IUnknown* pUnknown, HXSource* pSource);
    ~ASMSourceInfo2();

    UINT32 StreamCount() const;
    ASMStreamInfo2* GetStream(UINT32 i);

    UINT32& SubscribeBw();
    UINT32& LastSetDelivery();

    UINT32& MasterOffer();

    HXBOOL IsMySource(HXSource* pSource);
    HXBOOL IsLive();
    HXBOOL IsPerfectPlay() const;

    HXBOOL HasMasterRuleBook() const;
    void DistributeBw();

    CHXSimpleList* SubsChanges();

    void SubscribeToChanges();
    void SetDeliveryBw(UINT32 ulMaxBw);

    void Done();

private:
    HX_RESULT GetSubInfo(UINT32 ulBandwidth,
			 HXBOOL* pCurrentSubInfo);

    UINT32 GetStreamBw(IHXValues* pProps, 
		       UINT32 ulStreamIndex) const;

    HXSource* m_pSource;

    UINT32 m_ulStreamCount;
    ASMStreamInfo2** m_pStreams;

    UINT32 m_ulSubscribedBw;
    UINT32 m_ulLastSetDelivery;

    UINT32 m_ulMasterOffer;

    HXBOOL m_bPerfectPlay;
    ASMRuleBook* m_pMasterRuleBook;
    IHXValues* m_pSubsVars;
    CHXSimpleList   m_subsChanges;
};

class ASMStreamInfo2
{
public:
    ASMStreamInfo2(IUnknown* pStream);
    ~ASMStreamInfo2();

    HX_RESULT GetBiasFactor(REF(INT32) lBiasFactor);

    HXBOOL IsFixedBw() const;
    UINT32 CurrentBw() const;

    UINT32 StreamID() const;
    
    void SetStreamOffer(UINT32 ulOffer);

    void SelectBw(HXBOOL bPerfectPlay,
		  UINT32 ulAltOffer);
    
    UINT32 ResistanceToLower() const;

    void SelectNextLowerBw();

    UINT32 GetLastBw() const;
    void SubscribeToNewBw(UINT32 ulBw, CHXSimpleList* pSubChanges);
    void SubscribeToChanges(CHXSimpleList* pSubChanges);

private:
    void ComputeResistence(HXBOOL bPerfectPlay);

    HXBOOL   m_bFixedBwValid;
    UINT32 m_ulFixedBandwidth;
    UINT32 m_ulNumThresholds;
    float* m_pThresholds;
    UINT32 m_ulCurThresh;
    UINT32 m_ulMaxEffectiveThreshold;
    IHXStreamBandwidthBias* m_pBias;
    IHXStreamBandwidthNegotiator* m_pNegotiator;
    IHXAtomicRuleGather* m_pRuleGather;

    UINT32 m_ulStreamID;

    UINT32 m_ulStreamOffer; // From master rulebook
    UINT32 m_ulOffer;       // From SelectBw()
    UINT32 m_ulResistanceToLower;

    UINT32 m_ulLastBw;
};

class ASMStreamInfoItr
{
public:
    ASMStreamInfoItr(CHXSimpleList* pSourceList);
    
    ASMSourceInfo2* Source();
    ASMStreamInfo2* Stream();

    HXBOOL More() const;
    void Next();
    void Reset();

private:
    void FindNextSource();

    CHXSimpleList* m_pSourceList;
    CHXSimpleList::Iterator m_itr;
    UINT32         m_ulStream;
};

ASMStreamInfoItr::ASMStreamInfoItr(CHXSimpleList* pSourceList) :
    m_pSourceList(pSourceList),
    m_ulStream(0)
{
    Reset();
}

ASMSourceInfo2* ASMStreamInfoItr::Source()
{
    ASMSourceInfo2* pRet = 0;
    
    if (More())
    {
	pRet = (ASMSourceInfo2*)(*m_itr);
    }

    return pRet;
}

ASMStreamInfo2* ASMStreamInfoItr::Stream()
{
    ASMStreamInfo2* pRet = 0;
    
    ASMSourceInfo2* pSrc = Source();

    if (pSrc)
    {
	pRet = pSrc->GetStream(m_ulStream);
    }

    return pRet;
}

HXBOOL ASMStreamInfoItr::More() const
{
    return  (m_pSourceList && 
	     (m_itr != m_pSourceList->End()));
}

void ASMStreamInfoItr::Next()
{
    if (More())
    {
	m_ulStream++;

	if (m_ulStream == Source()->StreamCount())
	{
	    // We are done with this source.
	    // Move onto the next one
	    m_ulStream = 0;

	    ++m_itr;
	    FindNextSource();
	}
    }
}

void ASMStreamInfoItr::Reset()
{
    if (m_pSourceList)
    {
	m_ulStream = 0;
	
	m_itr = m_pSourceList->Begin(); 
	FindNextSource();
    }
}

void ASMStreamInfoItr::FindNextSource()
{
    HXBOOL bDone = FALSE;
    while(!bDone && m_itr != m_pSourceList->End())
    {
	ASMSourceInfo2* pSrc = (ASMSourceInfo2*)(*m_itr);
	
	if (pSrc && pSrc->StreamCount() > 0)
	{
	    bDone = TRUE;
	}
	else
	{
	    ++m_itr;
	}
    }
}

ASMSourceInfo2::ASMSourceInfo2(IUnknown* pUnknown,  HXSource* pSource) :
    m_pSource(pSource),
    m_ulStreamCount(0),
    m_pStreams(0),
    m_ulSubscribedBw(0),
    m_ulLastSetDelivery(0),
    m_ulMasterOffer(0),
    m_bPerfectPlay(FALSE),
    m_pMasterRuleBook(0),
    m_pSubsVars(0)
{
    if (m_pSource)
    {
	m_pSource->AddRef();
	m_bPerfectPlay = m_pSource->IsPerfectPlay();


        m_ulStreamCount = pSource->GetStreamCount();     
	m_pStreams = new ASMStreamInfo2*[m_ulStreamCount];

	if (m_pStreams)
	{
	    for (UINT16 i = 0; i < m_ulStreamCount; i++)
	    {
		IUnknown* pStream = 0;

		HX_VERIFY(HXR_OK == pSource->GetStream(i, pStream));

		m_pStreams[i] = new ASMStreamInfo2(pStream);
	    
		HX_RELEASE(pStream);
	    }
	}

	// Get Master Rulebook
	if (pSource->m_pFileHeader)
	{
	    IHXBuffer* pMasterRuleBook = NULL;
	    pSource->m_pFileHeader->
		GetPropertyCString("ASMRuleBook", pMasterRuleBook);
	    
	    if (pMasterRuleBook)
	    {
		m_pMasterRuleBook = new ASMRuleBook
		    (pUnknown, (const char *)pMasterRuleBook->GetBuffer());
	    }
	    
	    HX_RELEASE(pMasterRuleBook);
	}
    }
}

ASMSourceInfo2::~ASMSourceInfo2()
{
    if (m_pStreams)
    {
	for (UINT32 i = 0; i < m_ulStreamCount; i++)
	{
	    delete m_pStreams[i];
	    m_pStreams[i] = 0;
	}

	delete [] m_pStreams;
    }

    HX_RELEASE(m_pSource);
    HX_RELEASE(m_pSubsVars);

    delete m_pMasterRuleBook;
    m_pMasterRuleBook = 0;
}

inline
UINT32 ASMSourceInfo2::StreamCount() const
{
    return m_ulStreamCount;
}

ASMStreamInfo2* ASMSourceInfo2::GetStream(UINT32 i)
{
    ASMStreamInfo2* pRet = 0;

    if (m_pStreams && (i < m_ulStreamCount))
    {
	pRet = m_pStreams[i];
    }

    return pRet;
}

inline
UINT32& ASMSourceInfo2::SubscribeBw()
{
    return m_ulSubscribedBw;
}

inline
UINT32& ASMSourceInfo2::LastSetDelivery()
{
    return m_ulLastSetDelivery;
}

inline
UINT32& ASMSourceInfo2::MasterOffer()
{
    return m_ulMasterOffer;
}

inline
HXBOOL ASMSourceInfo2::IsMySource(HXSource* pSource)
{
    return (m_pSource == pSource);
}

HXBOOL ASMSourceInfo2::IsLive()
{
    HXBOOL bRet = FALSE;

    if (m_pSource)
    {
	bRet = m_pSource->IsLive();
    }

    return bRet;
}

inline
HXBOOL ASMSourceInfo2::IsPerfectPlay() const
{
    return m_bPerfectPlay;
}

inline
HXBOOL ASMSourceInfo2::HasMasterRuleBook() const
{
    return (m_pMasterRuleBook != NULL);
}

void ASMSourceInfo2::DistributeBw()
{
    if (m_pMasterRuleBook)
    {
	UINT32 ulRuleCount = m_pMasterRuleBook->GetNumRules();
	HXBOOL* pCurrentSubInfo = new HXBOOL[ulRuleCount];

	GetSubInfo(m_ulMasterOffer, pCurrentSubInfo);

	for (UINT16 idxRule = 0; idxRule < ulRuleCount; idxRule++)
	{
	    if (pCurrentSubInfo[idxRule])
	    {
		IHXValues* pProps = 0;
		// Set Distribution
		m_pMasterRuleBook->GetProperties(idxRule, pProps);

		for (UINT32 j = 0; j < m_ulStreamCount; j++)
		{
		    UINT32 ulStreamBw = GetStreamBw(pProps, j);
			
		    m_pStreams[j]->SetStreamOffer(ulStreamBw);
		}

		HX_RELEASE(pProps);
		break;
	    }
	}

	delete [] pCurrentSubInfo;
    }
}

inline
CHXSimpleList* ASMSourceInfo2::SubsChanges()
{
    return &m_subsChanges;
}

void ASMSourceInfo2::SubscribeToChanges()
{
    if (!m_subsChanges.IsEmpty() && m_pStreams && m_pStreams[0])
    {
	m_pStreams[0]->SubscribeToChanges(&m_subsChanges);

	while(!m_subsChanges.IsEmpty())
	{
	    RTSPSubscription* pSub = (RTSPSubscription*)m_subsChanges.RemoveHead();
	    delete pSub;
	}
    }
}

void ASMSourceInfo2::SetDeliveryBw(UINT32 ulMaxBw)
{
    if (m_pStreams)
    {
	UINT32 ulTotalBw = 0;
	
	// Calculate the total BW used by the streams
	for (UINT32 i = 0; i < m_ulStreamCount; i++)
	{
	    ulTotalBw += m_pStreams[i]->CurrentBw();
	}

	m_ulSubscribedBw = ulTotalBw;

	if (ulTotalBw)
	{
	    IHXThinnableSource* pThin = 0;
	    
	    if (HXR_OK == m_pSource->QueryInterface(IID_IHXThinnableSource, 
						    (void **)&pThin))
	    {
		/* Clamp bandwidth request */
		if (ulTotalBw > ulMaxBw)
		{
		    ulTotalBw = ulMaxBw;
		}
		
		pThin->SetDeliveryBandwidth(ulTotalBw, 0);
		
		HX_RELEASE(pThin);
	    }
	}
    }
}

inline
void ASMSourceInfo2::Done()
{
    HX_RELEASE(m_pSource);
}

HX_RESULT ASMSourceInfo2::GetSubInfo(UINT32 ulBandwidth,
				     HXBOOL* pCurrentSubInfo)
{
    HX_RESULT res = HXR_FAILED;

    if (m_pMasterRuleBook)
    {
        IUnknown *pUnknown;
        res = m_pSource->QueryInterface(IID_IUnknown, (void**) &pUnknown);

        if (HXR_OK == res)
        {
            res = HXSMUpdateSubscriptionVars(pUnknown, m_pSubsVars, ulBandwidth, FALSE, 0);
        }

	if (HXR_OK == res)
	{
	    res = m_pMasterRuleBook->GetSubscription(pCurrentSubInfo, 
						     m_pSubsVars);
	}
        pUnknown->Release();
    }

    return res;
}

UINT32 ASMSourceInfo2::GetStreamBw(IHXValues* pProps, 
				   UINT32 ulStreamIndex) const
{
    UINT32 ulRet = 0;

    UINT8       pTemp[128];
    IHXBuffer* pBw = NULL;
    HX_RESULT   hxResult;
    
    // Don't assume that streamid == index in m_pStreams[j]
    sprintf((char *)pTemp, 
	    "Stream%ldBandwidth", 
	    m_pStreams[ulStreamIndex]->StreamID()); /* Flawfinder: ignore */
		    
    /*
     * if this tripps either there's a bug in here or
     * the content is messed up
     *
     */
    //If the Stream?Bandwidth property isn't found for
    //all streams don't worry about it. Not all streams
    //may have rule books.
    hxResult = pProps->GetPropertyCString((char *)pTemp, pBw);
    if(HXR_OK == hxResult && pBw)
    {
	ulRet = atoi((char*)pBw->GetBuffer());
	
	HX_RELEASE(pBw);
    }

    return ulRet;
}

ASMStreamInfo2::ASMStreamInfo2(IUnknown* pStream) :
    m_bFixedBwValid(FALSE),
    m_ulFixedBandwidth(0),
    m_ulNumThresholds(0),
    m_pThresholds(0),
    m_ulCurThresh(0),
    m_ulMaxEffectiveThreshold(0),
    m_pBias(0),
    m_pNegotiator(0),
    m_pRuleGather(0),
    m_ulStreamID(0),
    m_ulStreamOffer(0),
    m_ulOffer(0),
    m_ulResistanceToLower(0xffffffff)
{
    if (pStream)
    {
	if (HXR_OK == pStream->QueryInterface
	    (IID_IHXStreamBandwidthNegotiator,
	     (void **)&m_pNegotiator))
	{
	    m_pNegotiator->GetFixedBandwidth(m_ulFixedBandwidth);

	    if (m_ulFixedBandwidth)
	    {
		/* We need to check to see if the fixed bandwidth value
		 * matches the ASMProps Bandwidth value. If it doesn't then
		 * that means that we are dealing with a clip that doesn't
		 * advertise any bandwidth information. This is a corner
		 * case discover by streaming some unhinted .3gp files.
		 */
		IHXASMProps* pASMProps = NULL;
		UINT32 ulASMBw = 0;

		if ((HXR_OK == pStream->QueryInterface(IID_IHXASMProps,
						       (void**)&pASMProps))&&
		    (HXR_OK == pASMProps->GetBandwidth(ulASMBw)) &&
		    (m_ulFixedBandwidth == ulASMBw))
		{
		    m_bFixedBwValid = TRUE;
		}

		HX_RELEASE(pASMProps);
	    }
	    else
	    {
		m_ulNumThresholds = m_pNegotiator->GetNumThresholds();
		m_pThresholds = new float[m_ulNumThresholds];

		if (m_pThresholds)
		{
		    m_pNegotiator->GetThresholdInfo(m_pThresholds, 
						m_ulNumThresholds);

		    m_ulMaxEffectiveThreshold = m_ulNumThresholds - 1;
		}
	    }
	}
	
	pStream->QueryInterface(IID_IHXStreamBandwidthBias,
				(void **)&m_pBias);
	
	pStream->QueryInterface(IID_IHXAtomicRuleGather,
				(void **)&m_pRuleGather);

	// Get StreamID
	IHXStream* pHXStream = 0;

	pStream->QueryInterface(IID_IHXStream, (void**)&pHXStream);
	if (pHXStream)
        {
            m_ulStreamID = pHXStream->GetStreamNumber();
            pHXStream->Release();
        }
    }
}

ASMStreamInfo2::~ASMStreamInfo2()
{
    delete [] m_pThresholds;
    m_pThresholds = 0;

    HX_RELEASE(m_pBias);
    HX_RELEASE(m_pNegotiator);
    HX_RELEASE(m_pRuleGather);
}

HX_RESULT ASMStreamInfo2::GetBiasFactor(REF(INT32) lBiasFactor)
{
    HX_RESULT res = HXR_FAILED;
    
    if (m_pBias)
    {
	res = m_pBias->GetBiasFactor(lBiasFactor);
    }

    return res;
}

inline
HXBOOL ASMStreamInfo2::IsFixedBw() const
{
    return (m_ulFixedBandwidth != 0);
}

UINT32 ASMStreamInfo2::CurrentBw() const
{
    UINT32 ulRet = (m_bFixedBwValid) ? m_ulFixedBandwidth : 0;

    if (!ulRet && m_pThresholds)
    {
	ulRet = (UINT32)m_pThresholds[m_ulCurThresh];
    }

    return ulRet;
}

inline
UINT32 ASMStreamInfo2::StreamID() const
{
    return m_ulStreamID;
}

inline
void ASMStreamInfo2::SetStreamOffer(UINT32 ulOffer)
{
    m_ulStreamOffer = ulOffer;
}

void ASMStreamInfo2::SelectBw(HXBOOL bPerfectPlay,
			      UINT32 ulAltOffer)
{
    if (m_pThresholds)
    {
	if (bPerfectPlay)
	{
	    /*
	     * If we are in perfect play mode, just select the highest bandwidth rule
	     * and don't negotiate any further.
	     */
	    m_ulCurThresh = m_ulMaxEffectiveThreshold;
	    m_ulOffer = CurrentBw();
	}
	else
	{ 
	    UINT32 ulOffer = ulAltOffer;

	    if (m_ulStreamOffer)
	    {
		ulOffer = m_ulStreamOffer - 1;
	    }
	    
	    if (m_ulMaxEffectiveThreshold != 0)
	    {
		for (UINT32 i = 1; i <= m_ulMaxEffectiveThreshold; i++)
		{
		    if ((ulOffer <= m_pThresholds[i]) ||
			(i == m_ulMaxEffectiveThreshold))
		    {
			m_ulOffer = ulOffer;
			m_ulCurThresh = i;
			break;
		    }
		}
	    }
	}

	ComputeResistence(bPerfectPlay);
    }
}

inline
UINT32 ASMStreamInfo2::ResistanceToLower() const
{
    return m_ulResistanceToLower;
}

void ASMStreamInfo2::SelectNextLowerBw()
{
    if (m_ulCurThresh > 0)
    {
	m_ulCurThresh--;

	ComputeResistence(FALSE);
    }
}

inline
UINT32 ASMStreamInfo2::GetLastBw() const
{
    return m_ulLastBw;
}

void ASMStreamInfo2::SubscribeToNewBw(UINT32 ulBw, CHXSimpleList* pSubChanges)
{
    HXBOOL bTimeStampDelivery = FALSE;
    
    if (m_pRuleGather && m_pNegotiator)
    {
	m_pRuleGather->RuleGather(pSubChanges);
	
	//update the HXASMStream with our new bandwidth
	m_pNegotiator->SetBandwidthUsage(ulBw, bTimeStampDelivery);
	
	m_pRuleGather->RuleGather(0);
    }

    m_ulLastBw = ulBw;
}

void ASMStreamInfo2::SubscribeToChanges(CHXSimpleList* pSubChanges)
{
    if (m_pRuleGather)
    {
	m_pRuleGather->RuleFlush(pSubChanges);
    }
}

void ASMStreamInfo2::ComputeResistence(HXBOOL bPerfectPlay)
{
    /* NOTE:
     * m_ulCurThresh and m_ulOffer must
     * be valid when this function is called
     */

    if (IsFixedBw() || 
	bPerfectPlay ||
	(m_ulMaxEffectiveThreshold == 0) ||
	(m_ulCurThresh == 1))
    {
	m_ulResistanceToLower = 0xffffffff;
    }
    else
    {
	m_ulResistanceToLower = 
	    (m_ulOffer - (UINT32)m_pThresholds[m_ulCurThresh - 1]) * m_ulOffer;
    }
}

HXSM2::HXSM2() :
    m_lRefCount(0),
    m_ulNumSources(0),
    m_pASMSourceInfo(0),
    m_ulNumStreams(0),
    m_ulOfferToRecalc(0),
    m_ulSelectionBitRate(0),
    m_ulMaxAccelBitRate(0),
    m_ulSustainableBitRate(0),
    m_pSubscriptionVariables(0),
    m_bCheckOnDemandBw(FALSE),
    m_pContext(NULL)
{
    m_pASMSourceInfo = new CHXSimpleList();
}

HXSM2::~HXSM2()
{
    if (m_pASMSourceInfo)
    {
	while(!m_pASMSourceInfo->IsEmpty())
	{
	    ASMSourceInfo2* pSrc = 
		(ASMSourceInfo2*)m_pASMSourceInfo->RemoveHead();

	    delete pSrc;
	}

	delete m_pASMSourceInfo;
    }

    HX_RELEASE(m_pContext);
}

STDMETHODIMP HXSM2::QueryInterface(THIS_ REFIID ID, void** ppInterfaceObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), this },
		{ GET_IIDHANDLE(IID_IHXBandwidthManager), (IHXBandwidthManager*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), ID, ppInterfaceObj);
}

STDMETHODIMP_(UINT32) HXSM2::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32) HXSM2::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *	IHXBandwidthManager methods
 */
STDMETHODIMP HXSM2::RegisterSource(THIS_
				   HXSource* pSource,
				   IUnknown* pUnknown)
{
    HX_RESULT res = HXR_OK;

    IHXSourceBandwidthInfo* pSBI;
    IHXPreferences* pPreferences = NULL;
    ASMSourceInfo2* pASMSourceInfo = NULL;

    if (HXR_OK == pSource->QueryInterface(IID_IHXSourceBandwidthInfo, 
            (void **)&pSBI))
    {
        pASMSourceInfo = new ASMSourceInfo2(pUnknown, pSource);

        m_ulNumSources++;
        m_ulNumStreams += pASMSourceInfo->StreamCount();

        m_pASMSourceInfo->AddTail((void *)pASMSourceInfo);

        HX_RELEASE(pSBI);
    }
    else
    {
        return HXR_OK;
    }

    /*
     * This variable tells us the highest amount of bandwidth that we
     * believe is useable.  If we stream thin, then the highest value
     * that the pipe can support would be the aggregate bandwidth of
     * the sources.  Barring any thinning, this would be the bandwidth
     * from prefs (unless we can determine it through fast buffering or
     * another transport feature).  This value is used when starting sources
     * as a initial guess for bandwidth (to set subscriptions).
     */
    if (m_ulSelectionBitRate == 0)
    {
        pUnknown->QueryInterface(IID_IHXPreferences, (void **)&pPreferences);

        UINT32 ulTemp = 0;
        UINT32 ulConnectionBw = 10485760; /* wild guess */

        IHXConnectionBWInfo* pConnBWInfo = NULL;
        if (HXR_OK == pUnknown->QueryInterface(IID_IHXConnectionBWInfo,
                                               (void**)&pConnBWInfo))
        {
            pConnBWInfo->GetConnectionBW(ulConnectionBw, FALSE);
            HX_ASSERT(ulConnectionBw);
        }
        else if ((HXR_OK == ReadPrefUINT32(pPreferences, 
                                           "Bandwidth", ulTemp))&&
                 (ulTemp > 0))
        {
            /* Get initial bandwidth guess from Prefs */
            ulConnectionBw = ulTemp;
        }
        HX_RELEASE(pConnBWInfo);

        /*
         * m_ulSustainableBitRate refers to the max bitrate that we believe is
         * sustainable over the connection. The client should never ask
         * the server to send bits faster than this rate because it could
         * cause congestion collapse.
         *
         * m_ulSelectionBitRate refers to the highest bitrate stream that we'll
         * ever try to play. Note that people can choose to pick values
         * where m_ulSelectionBitRate > m_ulSustainableBitRate. This may
	 * cause rebuffers because the delivery will be capped at 
	 * m_ulSustainableBitRate. Why would someone do that? They may accept
	 * rebuffers if it means they get a higher quality presentation.
         */
        
        /* See if we have a sustainable bandwidth factor */
        if (HXR_OK == ReadPrefUINT32(pPreferences, "SustainableBwFactor", 
                                    ulTemp))
        {
            /* clamp value to 0 <= ulTemp <= 100 range */
            if (ulTemp > 100)
            {
                ulTemp = 100;
            }
            
            /* Apply factor */
            m_ulSustainableBitRate = 
                (((ulConnectionBw / 100) * ulTemp) + 
                 (((ulConnectionBw % 100) * ulTemp) / 100));
        }
        else
        {
            m_ulSustainableBitRate = ulConnectionBw;
        }
        

        /* See if we have a selection bandwidth factor */
        if (HXR_OK == ReadPrefUINT32(pPreferences, "SelectionBwFactor", 
                                    ulTemp))
        {
            /* clamp value to 0 <= ulTemp <= 200 range */
            if (ulTemp > 200)
            {
                ulTemp = 200;
            }
            
            /* Apply factor */
            m_ulSelectionBitRate  = 
                (((m_ulSustainableBitRate / 100) * ulTemp) + 
                 (((m_ulSustainableBitRate % 100) * ulTemp) / 100));
        }
        else
        {
            m_ulSelectionBitRate = m_ulSustainableBitRate; 
        }

        /* Get MaxBandwidth from Prefs */
        if (HXR_OK != ReadPrefUINT32(pPreferences, "MaxBandwidth", 
                                    m_ulMaxAccelBitRate))
        {
            /* 
             * Failed to get the MaxBandwidth preference. 
             * Use m_ulSelectionBitRate value instead.
             */
            m_ulMaxAccelBitRate = m_ulSelectionBitRate;
        }

        /* 
         * Get preference that controls whether we report
         * "not enough bandwidth" conditions for on-demand clips
         */
        ReadPrefBOOL(pPreferences, "CheckOnDemandBw", m_bCheckOnDemandBw);

        HX_RELEASE(pPreferences);
    }

    m_pContext = pUnknown;
    HX_ADDREF(m_pContext);

    return res;
}

STDMETHODIMP HXSM2::RegisterSourcesDone(THIS)
{
    CHXSimpleList::Iterator     i;
    ASMSourceInfo2*    pASMSourceInfo;

    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
	pASMSourceInfo = (ASMSourceInfo2*)(*i);
	if (pASMSourceInfo)
	{
	    pASMSourceInfo->LastSetDelivery() = 0;
	}
    }
    
    m_ulOfferToRecalc = m_ulSelectionBitRate;

    if (m_ulOfferToRecalc > m_ulMaxAccelBitRate)
    {
	m_ulOfferToRecalc = m_ulMaxAccelBitRate;
    }

    Recalc();

    return HXR_OK;
}
    
STDMETHODIMP_(HXBOOL) HXSM2::NotEnoughBandwidth(THIS)
{
    CHXSimpleList::Iterator i;
    ASMSourceInfo2*          pASMSourceInfo;
    UINT32		    ulTotal	= 0;
    HXBOOL		    bCheckTotal = m_bCheckOnDemandBw;

    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
	pASMSourceInfo = (ASMSourceInfo2*)(*i);

	ulTotal += pASMSourceInfo->SubscribeBw();

	if (pASMSourceInfo->IsLive())
	{
	    bCheckTotal = TRUE;
	}
    }

    if (bCheckTotal && ulTotal > m_ulSelectionBitRate)
    {
	return TRUE;
    }
    else
    {
	return FALSE;
    }
}

STDMETHODIMP HXSM2::UnRegisterSource(THIS_
				     HXSource* pSource)
{
    LISTPOSITION		lPos;
    ASMSourceInfo2*             pASMSourceInfo = 0;
    HXBOOL			bFound = FALSE;

    lPos = m_pASMSourceInfo->GetHeadPosition();

    while (lPos)
    {
	pASMSourceInfo = (ASMSourceInfo2*)m_pASMSourceInfo->GetAt(lPos);
	if (pASMSourceInfo->IsMySource(pSource))
	{
	    m_pASMSourceInfo->RemoveAt(lPos);
	    pASMSourceInfo->Done();
	    bFound  = TRUE;
	    break;
	}
	m_pASMSourceInfo->GetNext(lPos);
    }
    
    if (bFound)
    {
	m_ulNumStreams -= pASMSourceInfo->StreamCount();

	delete pASMSourceInfo;

	m_ulNumSources--;
	
	if (m_ulNumSources > 0)
	{
	    Recalc();
	}
    }

    return HXR_OK;
}

STDMETHODIMP HXSM2::ChangeAccelerationStatus(THIS_
				HXSource* pSource,
				HXBOOL	   bMayBeAccelerated,
				HXBOOL	   bUseAccelerationFactor,
				UINT32	   ulAccelerationFactor)
{

    return HXR_OK;
}

    /* Called by HXPlayer at end of each presentation */
STDMETHODIMP HXSM2::PresentationDone(THIS)
{
    return HXR_OK;
}
    
STDMETHODIMP HXSM2::ChangeBW(THIS_ 
			     UINT32 newBW, HXSource* pSource)
{
    return HXR_OK;
}

STDMETHODIMP HXSM2::GetUpshiftBW(THIS_ REF(UINT32) uUpshiftBW)
{
    return HXR_NOTIMPL;
}

void
HXSM2::Recalc()
{
    CHXSimpleList::Iterator     i;
    ASMStreamInfoItr            streamItr(m_pASMSourceInfo);
    ASMSourceInfo2*             pASMSourceInfo;
    ASMStreamInfo2*             pASMStreamInfo;
    INT32			lAggregateBandwidthUsage = 0;
    UINT32			ulSourceCount;
    UINT32			ulStreamCount;
    float			fBiasMean = (float) 0.;

    ulSourceCount = m_pASMSourceInfo->GetCount();
    ulStreamCount = m_ulNumStreams;

    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
	pASMSourceInfo = (ASMSourceInfo2*)(*i);

        /* Init these for later */
	pASMSourceInfo->MasterOffer() = 0;
    }

    lAggregateBandwidthUsage = m_ulOfferToRecalc;

    INT32 lCorrectAggregateBandwidthUsage = lAggregateBandwidthUsage;

    /*
     * For each stream that is at a fixed bitrate, remove that bitrate
     * from our available bandwidth.
     */
    for (streamItr.Reset(); streamItr.More(); streamItr.Next())
    {
        INT32 lBias;

	pASMStreamInfo = streamItr.Stream();

	HX_VERIFY(HXR_OK == pASMStreamInfo->GetBiasFactor(lBias));
	fBiasMean += lBias;

	if (pASMStreamInfo->IsFixedBw())
	{
	    lAggregateBandwidthUsage -= pASMStreamInfo->CurrentBw();
	    ulStreamCount--;
	}

	/* Init this for later */
	pASMStreamInfo->SetStreamOffer(0);
    }

    if (ulStreamCount != 0)
    {
        /* At least 1 stream is not fixed bandwidth */

        fBiasMean /= ulStreamCount;
        
        /*
         * Calculate the offer for each source that has a master rulebook
         * defining it's bandwidth division.
         */
        INT32 lNewAggregateBandwidthUsage = lAggregateBandwidthUsage;

        for (streamItr.Reset(); streamItr.More(); streamItr.Next())
        {
            INT32 lBias;
            
            pASMStreamInfo = streamItr.Stream();
            pASMSourceInfo = streamItr.Source();
            
            HX_VERIFY(HXR_OK == pASMStreamInfo->GetBiasFactor(lBias));
            
            if (pASMSourceInfo->HasMasterRuleBook())
            {
                UINT32 ulOffer = 
                    (UINT32)(lAggregateBandwidthUsage / ulStreamCount);
                
                ulOffer += 
                    (UINT32)(((float)lBias - fBiasMean) *
                             ((float)lAggregateBandwidthUsage / 100.0) *
                             (2.0 / ulStreamCount));
                
                pASMSourceInfo->MasterOffer() += ulOffer;
                lNewAggregateBandwidthUsage -= ulOffer;
            }
        }

        lAggregateBandwidthUsage = lNewAggregateBandwidthUsage;
    }

    /*
     * For each source that has a master rule book, evaluate it to find
     * out how much to distribute to each stream.
     */
    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
	pASMSourceInfo = (ASMSourceInfo2*)(*i);
	if (pASMSourceInfo->HasMasterRuleBook())
	{
	    pASMSourceInfo->DistributeBw();
	}
    }

    /*
     *  Now go through each of the streams that are not at a
     *  fixed bitrate and try to distribute the rest of the bandwidth.
     */
    UINT32 ulTakenBandwidth = 0;

    for (streamItr.Reset(); streamItr.More(); streamItr.Next())
    {
	pASMStreamInfo = streamItr.Stream();

	if (!pASMStreamInfo->IsFixedBw())
	{
	    INT32 lBias;
	    
	    HX_VERIFY(HXR_OK == pASMStreamInfo->GetBiasFactor(lBias));
	    
	    UINT32 ulAltOffer =
		(UINT32)(lAggregateBandwidthUsage / (float)ulStreamCount);

	    ulAltOffer += (UINT32)(((float)lBias - fBiasMean) *
				   ((float)lAggregateBandwidthUsage / 100.0) *
				   (2.0 / (float)ulStreamCount));

	    pASMStreamInfo->SelectBw(streamItr.Source()->IsPerfectPlay(),
				     ulAltOffer);
	}

	ulTakenBandwidth += pASMStreamInfo->CurrentBw();
    }

    lAggregateBandwidthUsage = lCorrectAggregateBandwidthUsage;

    HXBOOL bDone = FALSE;
    while (!bDone && (lAggregateBandwidthUsage < (INT32)ulTakenBandwidth))
    {
	/* Resistance is Futile.  You will be Real(tm)lyAssimilated */
	UINT32 ulLowestResistance = 0xffffffff;
	ASMStreamInfo2* pLowestResistanceStream  = 0;

	// Find the stream with the lowest resistence
	for (streamItr.Reset(); streamItr.More(); streamItr.Next())
	{
	    pASMStreamInfo = streamItr.Stream();

	    if (pASMStreamInfo->ResistanceToLower() < ulLowestResistance)
	    {
		ulLowestResistance = pASMStreamInfo->ResistanceToLower();
		pLowestResistanceStream = pASMStreamInfo;
	    }
	}

	if (ulLowestResistance == 0xffffffff)
	{
	    bDone = TRUE;
	}
	else
	{
	    ulTakenBandwidth -= (UINT32)
		pLowestResistanceStream->CurrentBw();

	    pLowestResistanceStream->SelectNextLowerBw();

	    ulTakenBandwidth += (UINT32)
		pLowestResistanceStream->CurrentBw();
	}
    }

    UINT32 ulLeftOverForDropByN = lAggregateBandwidthUsage - ulTakenBandwidth;
    HXBOOL bForce = FALSE;

    for (streamItr.Reset(); streamItr.More(); streamItr.Next())
    {
	pASMStreamInfo = streamItr.Stream();

	UINT32 ulBw = pASMStreamInfo->CurrentBw();

	if (ulBw == 1)
	{
	    // Hack Alert for DropByN. XXXSMP
	    ulBw = ulLeftOverForDropByN;
	}

	if ((ulBw != pASMStreamInfo->GetLastBw()) &&
	    (pASMStreamInfo->IsFixedBw()))
	{
	    bForce = TRUE;
	}
	
	CHXSimpleList* pSubsChanges = 
	    streamItr.Source()->SubsChanges();

	pASMStreamInfo->SubscribeToNewBw(ulBw, pSubsChanges);
    }

    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
	pASMSourceInfo = (ASMSourceInfo2*)(*i);

	pASMSourceInfo->SubscribeToChanges();	
	pASMSourceInfo->SetDeliveryBw(m_ulSustainableBitRate);
    }
}

