/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsmstr.cpp,v 1.25 2008/05/06 20:04:29 ping Exp $
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

#include "hlxclib/stdio.h"      /* printf */
#include "hlxclib/stdlib.h"     /* atoi on Mac */
#include "hxtypes.h"    /* Basic Types */
#include "hxcom.h"      /* IUnknown */ 
#include "hxcore.h"    /* IHXStream */
#include "hxerror.h"
#include "hxasm.h"
#include "hxsmbw.h"
#include "hxmap.h"
#include "hxcomm.h"
#include "hxengin.h"
#include "hxsmstr.h"	/* HXASMStream */
#include "hxsmutil.h"
#include "ihxpckts.h"
#include "hxprefs.h"
#include "hxstrm.h"
#include "hxcore.h"
#include "asmrulep.h"
#include "hxmon.h"
#include "chxpckts.h"
#include "hxassert.h"
#include "hxslist.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "hxstrutl.h"
#include "errdbg.h"
#include "rtspif.h"
#include "rlstate.h"
#include "hxmime.h"
#include "hxtlogutil.h"

#include "hxheap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

HXASMStream::HXASMStream(HXStream* pStream, HXSource* pSource)
    : m_bTimeStampDeliveryMode(FALSE)
    , m_bInitialSubscribe(TRUE)
    , m_bHasExpression(FALSE)
    , m_bEndOneRuleEndAll(FALSE)
    , m_ulLastLimitBandwidth(0xffffffff)
    , m_lRefCount(0)
    , m_nNumRules(0)
    , m_pAtomicRuleChange(0)
    , m_pRuleBook(0)
    , m_pLossCB(0)
    , m_ulLossCBHandle(0)
    , m_ulBandwidthAllocation(0)
    , m_ulFixedBandwidth(0)
    , m_pSubInfo(0)
    , m_ulRuleBw(0)
    , m_ulRulePreData(0)
    , m_ulCurrentPreData(0)
    , m_ulCurrentBandwidth(0)
    , m_ulLastBandwidth(0)
    , m_bRuleTimeStampDelivery(0)
    , m_ulSubscribedBw(0)
    , m_lBias(0)
    , m_bStartRecalc(FALSE)
    , m_pSubList(0)
    , m_pSubscriptionVariables(0)
    , m_pRuleSubscribeStatus(NULL)
    , m_pASMRuleState(NULL)
    , m_pRuleEnableState(NULL)
    , m_bSubsLocked(FALSE)
#ifndef GOLD
    , m_pEM(0)
#endif
{
    m_pStreamSinkMap = new CHXMapPtrToPtr;
    
    UINT32  ulStreamID;
    IHXPlayer* pPlayer;

    pStream->GetID(ulStreamID);
    m_pSource = pSource;
    m_pSource->AddRef();
    m_pHeader = pStream->GetHeader();
    m_uStreamNumber = pStream->GetStreamNumber();

    /* Hold onto some useful interfaces */

    m_pSource->GetPlayer(pPlayer);
    pPlayer->QueryInterface(IID_IHXRegistry,  (void **)&m_pRegistry);
    pPlayer->QueryInterface(IID_IHXScheduler,   (void **)&m_pScheduler);
    pPlayer->QueryInterface(IID_IHXCommonClassFactory,   (void **)&m_pCCF);
    pPlayer->Release();

    /*
     * WARNING!  You must update all interfaces queried off the source
     * in ResetASMSource().
     */
    m_pSource->QueryInterface(IID_IHXASMSource, (void **)&m_pASMSource);
    m_pSource->QueryInterface(IID_IHXAtomicRuleChange,
	(void **)&m_pAtomicRuleChange);

#ifndef GOLD
    pPlayer->QueryInterface(IID_IHXErrorMessages, (void **)&m_pEM);
#endif

    UINT32 ulEndOneruleEndAll = 0;
    if (m_pHeader->GetPropertyULONG32("EndOneRuleEndAll", ulEndOneruleEndAll) == HXR_OK)
    {
	m_bEndOneRuleEndAll = (ulEndOneruleEndAll == 1);
    }
    /* temporary hack to properly shut down RA and RV streams with end time */
    else
    {	
	IHXBuffer* pMimeType = NULL;
	m_pHeader->GetPropertyCString("MimeType", pMimeType);

	if (pMimeType && 
	    ((::strcasecmp((const char*) pMimeType->GetBuffer(), REALAUDIO_MIME_TYPE) == 0) ||
	     (::strcasecmp((const char*) pMimeType->GetBuffer(), REALAUDIO_MULTIRATE_MIME_TYPE) == 0) ||
	     (::strcasecmp((const char*) pMimeType->GetBuffer(), REALVIDEO_MIME_TYPE) == 0) ||
	     (::strcasecmp((const char*) pMimeType->GetBuffer(), REALVIDEO_MULTIRATE_MIME_TYPE) == 0)))
	{
	    m_bEndOneRuleEndAll = TRUE;
	}

	HX_RELEASE(pMimeType);
    }

    /* Extract RuleBook from the stream header */

    IHXBuffer* pRuleBook = NULL;
    m_pHeader->GetPropertyCString("ASMRuleBook", pRuleBook);
    if (pRuleBook)
    {
        m_pRuleBook = new ASMRuleBook (m_pCCF, (const char *)pRuleBook->GetBuffer());

	m_nNumRules = m_pRuleBook->GetNumRules();
        if (m_nNumRules > 0)
        {
	    m_ulRuleBw = new UINT32[m_nNumRules];
	    m_ulRulePreData = new UINT32[m_nNumRules];
	    m_bRuleTimeStampDelivery = new HXBOOL[m_nNumRules];
	    m_pSubInfo = new HXBOOL[m_nNumRules];
	    m_pRuleSubscribeStatus = new HXBOOL[m_nNumRules];
            m_pRuleEnableState = new RuleEnableState[m_nNumRules];
        }

        for (UINT16 i = 0; i < m_nNumRules; i++)
        {
	    IHXValues*	pValues;
	    IHXBuffer* pBuffer;

	    m_pRuleBook->GetProperties(i, pValues);

	    m_ulRuleBw[i] = 0;
	    m_ulRulePreData[i] = 0;
	    m_bRuleTimeStampDelivery[i] = FALSE;
            m_pRuleEnableState[i] = resEnabled;

	    if (HXR_OK == pValues->GetPropertyCString("PreData",
		pBuffer))
	    {
		m_ulRulePreData[i] = atoi((char*)pBuffer->GetBuffer());
		pBuffer->Release();
	    }

	    if (HXR_OK == pValues->GetPropertyCString("AverageBandwidth",
		pBuffer))
	    {
		m_ulRuleBw[i] = atoi((char*)pBuffer->GetBuffer());
		pBuffer->Release();
	    }
	    else if (HXR_OK == pValues->GetPropertyCString("TimeStampDelivery",
		pBuffer))
	    {
		if ((pBuffer->GetBuffer()[0] == 'T') ||
		    (pBuffer->GetBuffer()[0] == 't'))
		{
		    // Handle TimeStamp Delivery (i.e. assume significant bandwidth)
		    m_ulRuleBw[i] = 0;
		    m_bRuleTimeStampDelivery[i] = TRUE;
		}
		pBuffer->Release();
	    }
	    else
	    {
		/* XXXSMP Temporary hack for invalid rulebooks */
		if (i == 0)
		{
		    IHXValues* pHeader = 0;

		    HX_VERIFY(pHeader = pStream->GetHeader());
		    pHeader->GetPropertyULONG32("AvgBitRate",m_ulRuleBw[i]);
		    pHeader->Release();
		}
		else
		{
		    m_ulRuleBw[i] = 0;
		}
	    }
	    pValues->Release();

            m_pSubInfo[i] = 0;
	    m_pRuleSubscribeStatus[i] = FALSE;
        }

	HXBOOL bFixed = TRUE;

	m_bHasExpression = m_pRuleBook->HasExpression();
	if (m_bHasExpression == FALSE)
	{
	    UINT16 i;
	    for (i = 0; i < m_nNumRules; i++)
	    {
		if (m_bRuleTimeStampDelivery[i] == FALSE)
		{
		    bFixed = FALSE;
		    break;
		}
	    }

	    if (bFixed)
	    {
		m_ulFixedBandwidth = 1;
	    }
	    else
	    {
		m_ulCurrentBandwidth = 0;
		for (i = 0; i < m_nNumRules; i++)
		{
		    m_ulCurrentBandwidth += m_ulRuleBw[i];
		}

		m_ulFixedBandwidth = m_ulCurrentBandwidth;
	    }
	}
    }
    else
    {
	IHXValues* pHeader = 0;

	HX_VERIFY(pHeader = pStream->GetHeader());
	HX_VERIFY(HXR_OK == pHeader->GetPropertyULONG32("AvgBitRate",
	    m_ulFixedBandwidth));
	
	m_ulCurrentBandwidth = m_ulFixedBandwidth; 

	pHeader->Release();
    }

    /* Get Registry ID's for interesting properties */
    IHXBuffer* pPropName = 0;

    // init. 
    memset(m_szRecv, 0, MAX_DISPLAY_NAME);
    memset(m_szLost, 0, MAX_DISPLAY_NAME);
    memset(m_szClipBandwidth, 0, MAX_DISPLAY_NAME);

    if( m_pRegistry )
	{
	    m_pRegistry->GetPropName(ulStreamID, pPropName);
	}
    
    if (pPropName)
    {
	SafeSprintf(m_szRecv, MAX_DISPLAY_NAME, "%s.received", pPropName->GetBuffer());
	m_ulIDRecv = m_pRegistry->GetId(m_szRecv);

	SafeSprintf(m_szLost, MAX_DISPLAY_NAME, "%s.lost", pPropName->GetBuffer());
	m_ulIDLost = m_pRegistry->GetId(m_szLost);

	SafeSprintf(m_szClipBandwidth, MAX_DISPLAY_NAME, "%s.ClipBandwidth", pPropName->GetBuffer());
	m_ulIDClipBandwidth = m_pRegistry->GetId(m_szClipBandwidth);
	pPropName->Release();
    }

    /* 
     * We consider Local source as a Network source IFF someone uses core
     * for simulated network playback (used for Preview mode in the Encoder)
     */
    if (pSource->IsLocalSource() && !pSource->IsSimulatedNetworkPlayback())
    {
	/* Some large bandwidth */
	m_ulBandwidthAllocation = 0x7FFFFFFF;
	HXBOOL b;
	SetBandwidthUsage(m_ulBandwidthAllocation, b);
    }
    else if (pSource->IsPNAProtocol())
    {
	IHXBuffer* pMimeType = NULL;
	m_pHeader->GetPropertyCString("MimeTYpe", pMimeType);

	if (pMimeType && 
	    ::strcasecmp((const char*) pMimeType->GetBuffer(), "audio/x-pn-realaudio") == 0)
	{
	    /* Some large bandwidth */
	    m_ulBandwidthAllocation = 0x7FFFFFFF;
	    HXBOOL b;
	    SetBandwidthUsage(m_ulBandwidthAllocation, b);
	}
	HX_RELEASE(pMimeType);
    }
    else if (pRuleBook)
    {
	m_pLossCB = new LossCheckCallback(this);
	m_pLossCB->AddRef();
	m_ulLossCBHandle = m_pScheduler->RelativeEnter(m_pLossCB, 1000);
    }
    
    HX_RELEASE(pRuleBook);
}

HXASMStream::~HXASMStream()
{
#ifndef GOLD
    HX_RELEASE(m_pEM);
#endif

    if (m_ulLossCBHandle)
    {
        m_pScheduler->Remove(m_ulLossCBHandle);
        m_ulLossCBHandle = 0;
    }

    HX_RELEASE(m_pLossCB);

    HX_VECTOR_DELETE(m_pSubInfo);
    HX_VECTOR_DELETE(m_ulRuleBw);
    HX_VECTOR_DELETE(m_ulRulePreData);
    HX_VECTOR_DELETE(m_bRuleTimeStampDelivery);
    HX_VECTOR_DELETE(m_pRuleSubscribeStatus);
    HX_VECTOR_DELETE(m_pRuleEnableState);
    
    HX_RELEASE(m_pASMSource);
    HX_RELEASE(m_pAtomicRuleChange);
    HX_RELEASE(m_pSource);
    HX_RELEASE(m_pHeader);
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pCCF);

    CHXMapPtrToPtr::Iterator lIterator =  m_pStreamSinkMap->Begin();
    for (; lIterator != m_pStreamSinkMap->End(); ++lIterator)
    {
	IHXASMStreamSink* pASMStreamSink = 
		(IHXASMStreamSink*) *lIterator;
	pASMStreamSink->Release();
    }
    m_pStreamSinkMap->RemoveAll();

    HX_DELETE(m_pASMRuleState);
    HX_DELETE(m_pRuleBook);
    HX_RELEASE(m_pSubscriptionVariables);
    HX_DELETE(m_pStreamSinkMap);
}

STDMETHODIMP_(UINT32)
HXASMStream::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
HXASMStream::Release(void)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
HXASMStream::QueryInterface
(
    REFIID interfaceID,
    void** ppInterfaceObj
)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXASMStream), (IHXASMStream*)this },
            { GET_IIDHANDLE(IID_IHXASMStream2), (IHXASMStream2*)this },
            { GET_IIDHANDLE(IID_IHXStreamBandwidthNegotiator), (IHXStreamBandwidthNegotiator*)this },
            { GET_IIDHANDLE(IID_IHXStreamBandwidthBias), (IHXStreamBandwidthBias*)this },
            { GET_IIDHANDLE(IID_IHXASMProps), (IHXASMProps*)this },
            { GET_IIDHANDLE(IID_IHXAtomicRuleGather), (IHXAtomicRuleGather*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXASMStream*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), interfaceID, ppInterfaceObj);
}

/*
 * IHXASMStream methods
 */

/************************************************************************
 *	Method:
 *	    IHXASMStream::AddStreamSink
 *	Purpose:
 *
 */
STDMETHODIMP
HXASMStream::AddStreamSink(IHXASMStreamSink* pASMStreamSink)
{
    HX_ASSERT(pASMStreamSink);

    if (!pASMStreamSink)
    {
	return HXR_UNEXPECTED;
    }

    void* pTmp;
    if (m_pStreamSinkMap->Lookup((void*) pASMStreamSink, pTmp))
    {
	return HXR_UNEXPECTED;
    }

    pASMStreamSink->AddRef();
    m_pStreamSinkMap->SetAt((void*) pASMStreamSink, (void*) pASMStreamSink);

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXASMStream::RemoveStreamSink
 *	Purpose:
 *
 */
STDMETHODIMP
HXASMStream::RemoveStreamSink(IHXASMStreamSink* pASMStreamSink)
{
    HX_ASSERT(pASMStreamSink && m_pASMSource);

    if (!pASMStreamSink)
    {
	return HXR_UNEXPECTED;
    }

    void* pTmp;
    if (!m_pStreamSinkMap->Lookup((void*) pASMStreamSink, pTmp))
    {
	return HXR_UNEXPECTED;
    }

    pASMStreamSink->Release();
    m_pStreamSinkMap->RemoveKey((void*) pASMStreamSink);

    return HXR_OK;
}

STDMETHODIMP
HXASMStream::Subscribe(UINT16 uRuleNumber)
{
    HX_RESULT lResult = HXR_OK;

    HXLOGL2(HXLOG_ASMX, "HXASMStream[%p]::Subscribe: Source=%p Stream=%d Rule=%d", this, m_pSource, m_uStreamNumber, uRuleNumber);

    if (m_pRuleSubscribeStatus)
    {
	m_pRuleSubscribeStatus[uRuleNumber] = TRUE;
    }
    
    if (m_pASMRuleState)
    {
	m_pASMRuleState->CompleteSubscribe(uRuleNumber);
	m_pASMRuleState->StartUnsubscribePending(uRuleNumber);
    }

    if (m_pAtomicRuleChange)
    {
	lResult = HXR_OK;
    }
    else if (m_pASMSource)
    {
	lResult = m_pASMSource->Subscribe(m_uStreamNumber, uRuleNumber); 
    }
    if ((lResult == HXR_OK) && m_pStreamSinkMap)
    {
	CHXMapPtrToPtr::Iterator lIterator =  m_pStreamSinkMap->Begin();
	for (;	(lIterator != m_pStreamSinkMap->End()) && lResult == HXR_OK; 
		++lIterator)
	{
	    IHXASMStreamSink* pASMStreamSink = 
		    (IHXASMStreamSink*) *lIterator;
	    lResult = pASMStreamSink->OnSubscribe(uRuleNumber);
	}
    }
    
    return lResult;
}

STDMETHODIMP
HXASMStream::Unsubscribe(UINT16	uRuleNumber)
{
    HX_RESULT lResult = HXR_OK;

    HXLOGL2(HXLOG_ASMX, "HXASMStream[%p]::Unsubscribe: Source=%p Stream=%d Rule=%d", this, m_pSource, m_uStreamNumber, uRuleNumber);

    if (m_pRuleSubscribeStatus)
    {
	m_pRuleSubscribeStatus[uRuleNumber] = FALSE;
    }
    
    if (m_pASMRuleState)
    {
	m_pASMRuleState->CompleteUnsubscribe(uRuleNumber);
    }

    if (m_pAtomicRuleChange)
    {
	lResult = HXR_OK;
    }
    else if (m_pASMSource)
    {
	lResult = m_pASMSource->Unsubscribe(m_uStreamNumber, uRuleNumber); 
    }
    if ((lResult == HXR_OK) && m_pStreamSinkMap)
    {
	CHXMapPtrToPtr::Iterator lIterator =  m_pStreamSinkMap->Begin();
	for (;	(lIterator != m_pStreamSinkMap->End()) && lResult == HXR_OK; 
		++lIterator)
	{
	    IHXASMStreamSink* pASMStreamSink = 
		    (IHXASMStreamSink*) *lIterator;
	    lResult = pASMStreamSink->OnUnsubscribe(uRuleNumber);
	}
    }
    
    return lResult;
}



/*
 * This is actually just a negotiation interface.  We need an interface
 * that tells us the actual allotment after oversend so that we can have
 * some more bandwidth available in case something like this happens:
 *  0 - 8k
 *  1 - 16k
 *  2 - 1k ECC for 0
 *  3 - 2k ECC for 1
 *  In this case, we need extra bandwidth already available to go and
 *  add 3 to 1
 */
STDMETHODIMP
HXASMStream::SetBandwidthUsage(REF(UINT32) ulRecvBitRate,
			       REF(HXBOOL) bTimeStampDelivery)
{
    if (m_pRuleBook == NULL)
    {
	return HXR_OK;
    }

    m_ulBandwidthAllocation = ulRecvBitRate;

    /*
     * This appears to be a var to make sure that Recalc only
     * does things when called from certain places.  I am not sure.
     */
    m_bStartRecalc = TRUE;
    Recalc();


    /* 
     *  Add up the bitrate of all of the rules we are subscribed to 
     *  and check if any of the rules put us in time stamped delivery mode.
     */

    ulRecvBitRate = 0;
    m_bTimeStampDeliveryMode = FALSE;
    for (UINT16 i = 0; i < m_nNumRules; i++)
    {
	if (m_pSubInfo[i])
	{
	    ulRecvBitRate += m_ulRuleBw[i];
	    if(m_bRuleTimeStampDelivery[i])
	    {
		m_bTimeStampDeliveryMode = TRUE;
	    }	    		
	}
    }
    bTimeStampDelivery = m_bTimeStampDeliveryMode;

    return HXR_OK;

}

STDMETHODIMP
HXASMStream::HandleSlowSource(UINT32 ulRecvBitRate)
{
#if XXXSMP
    /* Whoa!  This code is totally buggy.  We need to fix this!
     *
     * This code was NEVER executed, since day 1.  Since I fixed the TS
     * delivery flag above, we started executing it (and it doesn't work).
     *
     * Besides, not executing it is a good thing-  This mode really sucks.
     */
    if (m_bTimeStampDeliveryMode)
#else
    if (0)
#endif
    {
    /*
     *  Time stamp delivery mode.  No amount of ASM can help us now.
     */    

	/*
	 * If we are above what we previously asked the server to limit to
	 * then we have not yet started receiving the slower data yet or 
	 * the server is ignoring us.  Either way it will not help to send
	 * the message again.
	 */
	if(ulRecvBitRate >= m_ulLastLimitBandwidth)
	{
	    return HXR_OK;
	}

	IHXThinnableSource* pThin;
	if(HXR_OK == m_pASMSource->QueryInterface(IID_IHXThinnableSource,
							(void **)&pThin))
	{
	    /*
	     *  Tell the source to thin the bitrate down to 80% of what we
	     *  were getting.
	     */
	    m_ulLastLimitBandwidth = (UINT32)(0.8 * ulRecvBitRate);
	    pThin->LimitBandwidthByDropping(m_uStreamNumber, m_ulLastLimitBandwidth);
	    pThin->Release();
	}
    }

    return HXR_OK;
}

STDMETHODIMP
HXASMStream::GetThresholdInfo(float* pThreshold, REF(UINT32) ulNumThreshold)
{
    // XXXSMP Fix this.  Unify all construction of buffers & values
    // to one place.  Don't reconstruct unless you need to and store wasting
    // time, space, memory!

    if (!m_pRuleBook)
    {
	// ASM always expect num of threshold >= 1
	ulNumThreshold = 1;
	pThreshold[0] = 0.0f;
	
	return HXR_OK;
    }

    HXSMUpdateSubscriptionVars(m_pCCF, m_pSubscriptionVariables, 0, 
			       TRUE, ComputeLost());

    m_pRuleBook->GetPreEvaluate(pThreshold, ulNumThreshold,
	    m_pSubscriptionVariables, "Bandwidth");

    return HXR_OK;
}

STDMETHODIMP
HXASMStream::UnRegister(void)
{
    m_bStartRecalc = FALSE;
    /* Remove any scheduled callback */
    if (m_ulLossCBHandle && m_pLossCB && m_pScheduler)
    {
        m_pScheduler->Remove(m_ulLossCBHandle);
        m_ulLossCBHandle = 0;
    }
  
    return HXR_OK;
}


STDMETHODIMP_(ULONG32)
HXASMStream::GetNumThresholds(void)
{
    // return the maximum number of thresholds for this rulebook
    return m_pRuleBook ? m_pRuleBook->GetNumThresholds(): 0;
}


STDMETHODIMP
HXASMStream::GetFixedBandwidth(REF(UINT32) ulBitRate)
{
    if (m_ulFixedBandwidth)
    {
	ulBitRate = m_ulFixedBandwidth;
	return HXR_OK;
    }

    return HXR_FAIL;
}

STDMETHODIMP
HXASMStream::GetBiasFactor(REF(INT32) lBias)
{
    lBias = m_lBias;

    return HXR_OK;
}

STDMETHODIMP
HXASMStream::SetBiasFactor(INT32 lBias)
{
    m_lBias = lBias;

    return HXR_OK;
}

HXASMStream::LossCheckCallback::LossCheckCallback(HXASMStream* pASMStream)
{
    m_pASMStream    = pASMStream;
    m_lRefCount	    = 0;
}

STDMETHODIMP_(UINT32)
HXASMStream::LossCheckCallback::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
HXASMStream::LossCheckCallback::Release(void)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
HXASMStream::LossCheckCallback::QueryInterface
(
    REFIID interfaceID,
    void** ppInterfaceObj
)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXCallback*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), interfaceID, ppInterfaceObj);
}

STDMETHODIMP
HXASMStream::GetPreData(REF(UINT32) ulPreData)
{
    if (m_ulCurrentPreData)
    {
	ulPreData = m_ulCurrentPreData;
	return HXR_OK;
    }
    else
    {
	ulPreData = 0;
	return HXR_FAIL;
    }
}

STDMETHODIMP
HXASMStream::GetBandwidth(REF(UINT32) ulBandwidth)
{
    if (m_ulCurrentBandwidth)
    {
	ulBandwidth = m_ulCurrentBandwidth;
	return HXR_OK;
    }
    else if (m_ulLastBandwidth)
    {
	ulBandwidth = m_ulLastBandwidth;
	return HXR_OK;
    }
    else
    {
	ulBandwidth = 0;
	return HXR_FAILED;
    }
}

void
HXASMStream::RecalcCurrentProps()
{
    m_ulCurrentBandwidth    = 0;
    m_ulCurrentPreData	    = 0;
    for (UINT16 i = 0; i < m_nNumRules; i++)
    {
        if (m_pSubInfo[i])
        {
	    m_ulCurrentPreData	 += m_ulRulePreData[i];
	    m_ulCurrentBandwidth += m_ulRuleBw[i];
	}
    }

#if defined(HELIX_FEATURE_REGISTRY)
    if (m_pSource->IsActive())
    {
	INT32  lLastBandwidth = 0;
        if( m_pRegistry )
	{
            m_pRegistry->GetIntById(m_ulIDClipBandwidth, lLastBandwidth);
	}

	// Do not override bandwidth if the current bandwidth has not changed.
	if ((UINT32) lLastBandwidth != m_ulCurrentBandwidth && m_pRegistry)
	{
	    m_pRegistry->SetIntById(m_ulIDClipBandwidth, m_ulCurrentBandwidth);
	    /* A hack to tell the top level client that a stream switch occured */
            IHXPlayer* pPlayer = NULL;
            if (SUCCEEDED(m_pSource->GetPlayer(pPlayer)))
            {
                UINT32 ulRegistryID = 0;
                IHXBuffer* pPlayerRegName = NULL;
                IHXRegistryID* pRegistryID = NULL;
                if (SUCCEEDED(pPlayer->QueryInterface(IID_IHXRegistryID, (void**)&pRegistryID)))
                {
                    pRegistryID->GetID(ulRegistryID);
                    if (ulRegistryID)
                    {
                        char szRegName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
                        if (SUCCEEDED(m_pRegistry->GetPropName(ulRegistryID, pPlayerRegName)))
                        {
                            SafeSprintf(szRegName, MAX_DISPLAY_NAME, "%s.StreamSwitchOccured", pPlayerRegName->GetBuffer());
	                    m_pRegistry->SetIntByName(szRegName, 1);
                        }
                        HX_RELEASE(pPlayerRegName);
                    }
                    HX_RELEASE(pRegistryID);
                }
                HX_RELEASE(pPlayer);
            }
	}
    }
#endif /* HELIX_FEATURE_REGISTRY */

    if (m_ulCurrentBandwidth > 0)
    {
	m_ulLastBandwidth = m_ulCurrentBandwidth;
    }
    	
//DEBUG_OUT(this, (s, "New PreData %p %d New Bandwidth %d\n", this, m_ulCurrentPreData, m_ulCurrentBandwidth));
}

void
HXASMStream::Recalc()
{
    if (!m_bStartRecalc)
	return;

    HXSMUpdateSubscriptionVars(m_pCCF,
			       m_pSubscriptionVariables, 
			       m_ulBandwidthAllocation, 
			       TRUE, ComputeLost());

    HXBOOL* pCurrentSubInfo = new HXBOOL[m_nNumRules];
    
    if (m_pRuleBook)
    {
	HX_RESULT lResult = HXR_OK;
	lResult = m_pRuleBook->GetSubscription(pCurrentSubInfo, m_pSubscriptionVariables);
	HX_ASSERT(lResult == HXR_OK);
    }

    CHXSimpleList SubChange;
    CHXSimpleList* pSubList = &SubChange;

    if (m_pSubList)
    {
	pSubList = m_pSubList;
    }

    for (UINT16 i = 0; i < m_nNumRules; i++)
    {
        if (pCurrentSubInfo[i] != m_pSubInfo[i])
        {
	    RTSPSubscription* pEntry = NULL;

	    if (m_pAtomicRuleChange)
	    {
		pEntry = new RTSPSubscription;
		pEntry->m_streamNumber = m_uStreamNumber;
		pEntry->m_ruleNumber = i;
		pEntry->m_bIsSubscribe = (pCurrentSubInfo[i]) ? TRUE : FALSE;
		pSubList->AddTail(pEntry);
	    }

            if (pCurrentSubInfo[i])
	    {
                Subscribe(i);
	    }
            else
	    {
                Unsubscribe(i);
	    }

            m_pSubInfo[i] = pCurrentSubInfo[i];

        }

        if (m_bInitialSubscribe && 
            m_bSubsLocked &&
            m_pRuleEnableState && 
            m_pRuleBook &&
            !m_pSubInfo[i])
        {
            // This is the initial subscription and
            // this rule was not selected.
            // Mark it as locked and disable
            // the rule in the rulebook
            m_pRuleEnableState[i] = resLocked;
            m_pRuleBook->Disable(i);
        }
    }

    if (m_bInitialSubscribe)
    {
        m_bInitialSubscribe = FALSE;
        m_pRuleBook->ReCompute();
    }

    if (m_pAtomicRuleChange && (!m_pSubList) && (!SubChange.IsEmpty()))
    {
	m_pAtomicRuleChange->RuleChange(SubChange);
    }

    RecalcCurrentProps();

    delete [] pCurrentSubInfo;

    /* Reschedule callback if this was invoked by the scheduler callback */
    if (!m_ulLossCBHandle && m_pLossCB)
    {
	m_ulLossCBHandle = m_pScheduler->RelativeEnter(m_pLossCB, 1000);
    }
}

float HXASMStream::ComputeLost()
{
    INT32 ulRecv = 0, ulLost = 0;
    float lost = 0.0;
    
    if( m_pRegistry )
    {
        m_pRegistry->GetIntById(m_ulIDRecv, ulRecv);
        m_pRegistry->GetIntById(m_ulIDLost, ulLost);
    }

    if (ulRecv != 0)
    {
	lost = ((float)ulLost) / ((float)ulRecv) * 100;
    }

    return lost;
}

STDMETHODIMP
HXASMStream::LossCheckCallback::Func()
{
    m_pASMStream->m_ulLossCBHandle = 0;
    m_pASMStream->Recalc();
    return HXR_OK;
}

HX_RESULT	
HXASMStream::ResetASMSource(IHXASMSource* pASMSource)
{
    HX_RESULT	hr = HXR_OK;

    HX_RELEASE(m_pASMSource);

    if (pASMSource)
    {
	pASMSource->QueryInterface(IID_IHXASMSource, (void **)&m_pASMSource);
    }

    if (m_pAtomicRuleChange)
    {
	HX_RELEASE(m_pAtomicRuleChange);

	pASMSource->QueryInterface(IID_IHXAtomicRuleChange,
	    (void **)&m_pAtomicRuleChange);
    }

    // reset substream info.
    if (m_pRuleBook && m_pSubInfo)
    {
	for (UINT16 i = 0; i < m_nNumRules; i++)
	{
	    m_pSubInfo[i] = 0;
	}
    }

    // reset Registry ID
    if (strlen(m_szRecv) && m_pRegistry)
    {
	m_ulIDRecv = m_pRegistry->GetId(m_szRecv);
    }

    if (strlen(m_szLost) && m_pRegistry)
    {
	m_ulIDLost = m_pRegistry->GetId(m_szLost);
    }

    if (strlen(m_szClipBandwidth) && m_pRegistry)
    {
	m_ulIDClipBandwidth = m_pRegistry->GetId(m_szClipBandwidth);
    }

    return hr;
}

STDMETHODIMP
HXASMStream::RuleGather(CHXSimpleList* pList)
{
    /* Truely disgusting... */
    m_pSubList = pList;

    return HXR_OK;
}

STDMETHODIMP
HXASMStream::RuleFlush(CHXSimpleList* pList)
{
    m_pAtomicRuleChange->RuleChange(*pList);

    return HXR_OK;
}

void
HXASMStream::PostEndTimePacket(IHXPacket* pPacket, HXBOOL& bSentMe, HXBOOL& bEndMe)
{
    int		i = 0;
    UINT8	nRuleFlags = 0;
    UINT16	nRuleNumber = 0;
    IHXBuffer* pRuleBook = NULL;

    bSentMe = TRUE;
    bEndMe = FALSE;

    // assume the packet is NOT lost
    HX_ASSERT(pPacket->IsLost() == FALSE);

    // This is the first packet whose timestamp > endtime
    // Initialize ASMRuleState to determine whether pass it to
    // the renderer or not
    if (!m_pASMRuleState)
    {
	// Extract RuleBook from the stream header
	m_pHeader->GetPropertyCString("ASMRuleBook", pRuleBook);	    
	
	if (pRuleBook != NULL)
	{
	    m_pASMRuleState = new CASMRuleState(m_nNumRules, (char*)pRuleBook->GetBuffer(), (UINT16)pRuleBook->GetSize());
	}

	if (m_pASMRuleState)
	{
	    // initialize the ASMRuleState based on the 
	    // current subscription status of all the rules
	    for (i = 0; i < m_nNumRules; i++)
	    {
		if (m_pRuleSubscribeStatus[i])
		{
		    m_pASMRuleState->CompleteSubscribe(i);
		    m_pASMRuleState->StartUnsubscribePending(i);
		}
	    }
	}
	else
	{
	    // end the stream now is fine.
	    bSentMe = FALSE;
	    bEndMe = TRUE;
	    goto cleanup;
	}
    }

    nRuleNumber = pPacket->GetASMRuleNumber();
    nRuleFlags = pPacket->GetASMFlags();

    // if the rule is unsubscribed, then
    // we just pass it to the renderer since
    // the server will stop sending them anyway
    // i.e. during the stream swithing
    if (!m_pRuleSubscribeStatus[nRuleNumber] &&
	m_pASMRuleState->AnyPendingUnsubscribes())
    {
	goto cleanup;
    }    
    else if (nRuleFlags & HX_ASM_SWITCH_OFF)
    {
	if (m_pASMRuleState->IsUnsubscribePending(nRuleNumber) &&
	    m_pASMRuleState->CanUnsubscribeNow(nRuleNumber))
	{
	    m_pASMRuleState->CompleteUnsubscribe(nRuleNumber);
	}
    }

    bSentMe = m_pASMRuleState->IsRuleSubscribed(nRuleNumber);

    // if all the rules are unsubscribed, then 
    // we are DONE!
    if (!m_pASMRuleState->AnyPendingUnsubscribes())
    {
	bSentMe = FALSE;
	bEndMe = TRUE;
    }
    else if (bSentMe == FALSE && m_bEndOneRuleEndAll)
    {
	bEndMe = TRUE;
    }

cleanup:

    HX_RELEASE(pRuleBook);

    return;
}

void
HXASMStream::ResetASMRuleState(void)
{
    HX_DELETE(m_pASMRuleState);

    return;
}

STDMETHODIMP_(HX_RESULT)
HXASMStream::Enable( UINT16 nRule )
{
   if( m_pRuleBook  && m_pRuleEnableState)
   {
       if (m_bSubsLocked)
       {
           // Change the enable state to locked,
           // but leave the rule disabled in the
           // rulebook. When we leave the locked
           // state this rule will be reenabled
           // in the rulebook
           m_pRuleEnableState[nRule] = resLocked;
       }
       else
       {
           // Change the enable state and enable
           // the rule in the rulebook
           m_pRuleEnableState[nRule] = resEnabled;
           m_pRuleBook->Enable( nRule );
       }
   }
   return HXR_OK;
}

STDMETHODIMP_(HX_RESULT)
HXASMStream::Disable( UINT16 nRule )
{
   if( m_pRuleBook && m_pRuleEnableState)
   {
       m_pRuleEnableState[nRule] = resDisabled;
       m_pRuleBook->Disable( nRule );
   }
    return HXR_OK;
}

STDMETHODIMP_(HX_RESULT)
HXASMStream::ReCompute()
{
   HX_RESULT retVal = HXR_OK;
   if( m_pRuleBook )
   {
      m_pRuleBook->ReCompute();

      // Now we must check that there is at least 1 playable substream.
      HXBOOL* pCurrentSubInfo = new HXBOOL[m_nNumRules];
      if( pCurrentSubInfo == NULL )
      {
          return HXR_FAIL;
      }
          
      HXSMUpdateSubscriptionVars(m_pCCF, m_pSubscriptionVariables, 0, FALSE, 0);

      // GetSubscription does not actually do any subscribing. Rather it 
      // populates the boolean array pCurrentSubInfo such that subscribable
      // streams are set to TRUE;
      m_pRuleBook->GetSubscription( pCurrentSubInfo, m_pSubscriptionVariables );

      HXBOOL bAtLeastOneRuleSubscribable = FALSE;
      for( int ii=0; ii<m_nNumRules; ii++ )
      {
          if( pCurrentSubInfo[ii] == TRUE )
          {
              bAtLeastOneRuleSubscribable = TRUE;
          }
      }
      HX_VECTOR_DELETE(pCurrentSubInfo);

      if( bAtLeastOneRuleSubscribable == FALSE )
      {
          // We can't subscribe to any stream so reset and fail.
          for( int ii=0; ii<m_nNumRules; ii++ )
          {
              Enable( ii );
          }
          m_pRuleBook->ReCompute();
          retVal = HXR_FAIL;
      }
   }
   return retVal;
}

STDMETHODIMP_(HXBOOL)
HXASMStream::IsEnabled( UINT16 nRule )
{
    HXBOOL bRet = TRUE;
    if( m_pRuleBook && m_pRuleEnableState &&
        (m_pRuleEnableState[nRule] != resDisabled))
    {
        bRet = FALSE;
    }

    return bRet;
}

STDMETHODIMP 
HXASMStream::LockSubscriptions(THIS)
{
    HX_RESULT res = HXR_FAILED;

    if( m_pRuleBook  && m_pRuleEnableState)
    {
        if (m_bSubsLocked)
        {
            // The subscriptions are already locked
            res = HXR_OK;
        }
        else
        {
            if (!m_bInitialSubscribe)
            {
                // We only lock and disable rules if we have
                // done the inital subscription.
                // If we haven't done the initial subscription,
                // the locking and disabling of rules will
                // happen when the initial subscription occurs.
                
                for (UINT16 i = 0; i < m_nNumRules; i++)
                {
                    // Change the state of any rules that
                    // are currently enabled, but not subscribed
                    // to. This makes it so that the rulebook
                    // will always evaluate to the current 
                    // subscriptions
                    if (!m_pSubInfo[i] &&
                        (m_pRuleEnableState[i] == resEnabled))
                    {
                        m_pRuleEnableState[i] = resLocked;
                        m_pRuleBook->Disable(i);
                    }
                    
                }
                m_pRuleBook->ReCompute();
            }
            
            m_bSubsLocked = TRUE;
            res = HXR_OK;
        }
    }
    
    return res;
}

STDMETHODIMP 
HXASMStream::UnlockSubscriptions(THIS)
{
    HX_RESULT res = HXR_FAILED;

    if( m_pRuleBook  && m_pRuleEnableState)
    {
        if (m_bSubsLocked)
        {
            for (UINT16 i = 0; i < m_nNumRules; i++)
            {
                // Change the state of any rules that
                // are currently locked and enable them
                // in the rulebook. This makes it so that 
                // the rulebook can evaluate to any of the
                // enabled rules
                if (m_pRuleEnableState[i] == resLocked)
                {
                    m_pRuleEnableState[i] = resEnabled;
                    m_pRuleBook->Enable(i);
                }
            }
            m_pRuleBook->ReCompute();
                        
            m_bSubsLocked = FALSE;
        }
        else
        {
            // We are currently in an unlocked state
            res = HXR_OK;
        }
    }
    
    return res;
}

STDMETHODIMP_(HXBOOL)
HXASMStream::AreSubscriptionsLocked(THIS)
{
    return m_bSubsLocked;
}
