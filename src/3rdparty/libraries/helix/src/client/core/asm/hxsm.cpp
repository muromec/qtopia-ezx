/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsm.cpp,v 1.44 2007/07/06 21:58:15 jfinnecy Exp $
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

/*
 *  
 *  ASM Manager for all streams in all Players.
 *
 *  This class will manage the bandwidth requirements
 *  of multiple streams.
 */

#include "hlxclib/stdio.h"      /* printf */
#include "hlxclib/stdlib.h"     /* atoi on Mac */
#include "hxtypes.h"    /* Basic Types */
#include "hxcom.h"      /* IUnknown */
#include "hxslist.h"
#include "hxerror.h"
#include "hxsmbw.h"
#include "hxsm.h"      /* HXSM */
#include "hxsmutil.h"
#include "hxpref.h"
#include "ihxpckts.h"
#include "hxcore.h"
#include "asmrulep.h"
#include "hxbuffer.h"
#include "chxpckts.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "errdbg.h"
#include "rtspif.h"
#include "hxurl.h"
#include "hxtick.h"
#include "hxstrutl.h"
#include "hxbufctl.h"
#include "hxtlogutil.h"

#include "hxheap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

HXSM::HXSM()
    : m_State(HX_NONE)
    , m_lRefCount(0)
    , m_ulOriginalHighestBandwidthAvail(0)
    , m_ulHighestBandwidthAvail(0)
    , m_ulPeakUsedBandwidth(0)
    , m_ulUpShiftBandwidthAvail(0)
    , m_ulNumSources(0)
    , m_ulMaxAccelBitRate(0xffffffff)
    , m_ulNumReportsSinceUpShift(2)
    , m_ulLastStableBandwidth(0)
    , m_ulUpShiftTestPointScaleFactor(4000)
    , m_ulOfferToRecalc(0)
    , m_ulNextPacketWindow(0)
    , m_lPacketCounter(0)
    , m_ulUpShiftPastResistanceCount(0)
    , m_lLoss(0)
    , m_bInitialHighBwAvail(TRUE)
    , m_bPipeFull(FALSE)
    , m_bDidOfferUpShiftToRecalc(FALSE)
    , m_fAccelerationFactor(4.0)
    , m_bEnableSDB(TRUE)
#ifndef GOLD
    , m_pEM(0)
#endif
{
    m_pASMSourceInfo = new CHXSimpleList;
    m_pASMStreamInfo = new CHXSimpleList;

    // There is nothing mystical about the number picked here.
    // It just provides a decent amount of smoothing to the raw
    // input data without sacrificing reponsiveness to changes in
    // the connection BW.
    m_upshiftBW.Init(12);
}

HXSM::~HXSM()
{
#ifndef GOLD
    HX_RELEASE(m_pEM);
#endif
    CHXSimpleList::Iterator     i;
    ASMSourceInfo*                 pASMSourceInfo;
    ASMStreamInfo*                 pASMStreamInfo;

    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
	pASMSourceInfo = (ASMSourceInfo*)(*i);
	pASMSourceInfo->Release();
    }

    for (i = m_pASMStreamInfo->Begin(); i != m_pASMStreamInfo->End(); ++i)
    {
	pASMStreamInfo = (ASMStreamInfo*)(*i);
	delete pASMStreamInfo;
    }

    delete m_pASMSourceInfo;
    delete m_pASMStreamInfo;
}

STDMETHODIMP_(UINT32)
HXSM::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
HXSM::Release(void)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
HXSM::QueryInterface
(
    REFIID interfaceID,
    void** ppInterfaceObj
)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)this },
            { GET_IIDHANDLE(IID_IHXBandwidthManager), (IHXBandwidthManager*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), interfaceID, ppInterfaceObj);
}

STDMETHODIMP
HXSM::RegisterSource(HXSource* pSource, IUnknown* pUnknown)
{
    IHXSourceBandwidthInfo* pSBI;
    ASMSourceInfo* pASMSourceInfo = NULL;

#ifndef GOLD
    HX_RELEASE(m_pEM);
    pUnknown->QueryInterface(IID_IHXErrorMessages, (void **)&m_pEM);
#endif

    HXLOGL2(HXLOG_BAND, "Register Source %p %s", 
                                 pSource, pSource->GetURL());

    if (HXR_OK == pSource->QueryInterface(IID_IHXSourceBandwidthInfo, 
	    (void **)&pSBI))
    {
	pASMSourceInfo = new ASMSourceInfo(pSource, this, pUnknown);
	pASMSourceInfo->AddRef();

	m_ulNumSources++;
	m_pASMSourceInfo->AddTail((void *)pASMSourceInfo);
	pSBI->InitBw(pASMSourceInfo);

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
    if (m_ulHighestBandwidthAvail == 0)
    {
        
        InitBandwidthVars(pUnknown);
    }
    
    if (m_netIFSinkHlpr.NeedsInit())
    {
        m_netIFSinkHlpr.Init(pUnknown, this,
                             &static_NetInterfacesUpdated);
    }

    UINT16 unStreamCount = pSource->GetStreamCount();     
    HXBOOL bEnableSDB = (unStreamCount == 0);

    // Create ASMStreamInfo objects for all the
    // streams in this source. Add them to the
    // ASMSourceInfo object and the m_pASMStreamInfo
    // list.
    for (UINT16 i = 0; i < unStreamCount; i++)
    {
        UINT32 ulLowestBandwidthBeforeTimeStamp;
        IUnknown* pStream = 0;

        HX_VERIFY(HXR_OK == pSource->GetStream(i, pStream));
 
        // Note: ulLowestBandwidthBeforeTimeStamp gets
        //       updated by the constructor
	ASMStreamInfo* pInfo = 
            new ASMStreamInfo(pASMSourceInfo, pStream,
                              ulLowestBandwidthBeforeTimeStamp);

        if (pInfo && pInfo->GetFixedBw() != 1)
        {
            bEnableSDB = TRUE;
        }

        pASMSourceInfo->AddStream(i, pInfo, 
                                  ulLowestBandwidthBeforeTimeStamp);
	m_pASMStreamInfo->AddTail((void *)pInfo);

        HX_RELEASE(pStream);
    }

    // Rule for determining if SetDeliveryBandwidth is used.
    //
    // HXASMStream::bFixed   bEnabledSDB  m_bEnableSDB
    // ------------------------------------------
    //     FALSE             FALSE       TRUE  // no fbw means bw > 1
    //     FALSE             TRUE        TRUE  // at least one rule is fbw==1
    //     TRUE              FALSE       FALSE // all bw are fbw==1
    //     TRUE              TRUE        TRUE  // no streamcount, SDB true
    m_bEnableSDB = bEnableSDB;

    return HXR_OK;
}

STDMETHODIMP
HXSM::RegisterSourcesDone()
{
    if (m_pASMSourceInfo->GetCount() > 0 &&
	m_pASMStreamInfo->GetCount() > 0)
    {
        CHXSimpleList::Iterator     i;
        ASMSourceInfo*		    pASMSourceInfo;

        HXLOGL2(HXLOG_TRAN, 
                  "RegisterSourcesDone AccelFactor:%f", 
                   m_fAccelerationFactor);
        
        // Notify all of the ASMSourceInfo objects that we are done
        // registering sources
        for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
        {
	    pASMSourceInfo = (ASMSourceInfo*)(*i);

            if (pASMSourceInfo)
            {
                pASMSourceInfo->RegisterSourcesDone();
            }
        }

	m_State = INIT;
	RecalcAccel();
    }

    return HXR_OK;    
}


HXBOOL
HXSM::NotEnoughBandwidth()
{
    CHXSimpleList::Iterator i;
    ASMSourceInfo*          pASMSourceInfo;
    UINT32		    ulTotal	    = 0;
    HXBOOL		    bIsLive	    = FALSE;

    // Get the total bandwidth subscribed to across all sources
    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
	pASMSourceInfo = (ASMSourceInfo*)(*i);

	ulTotal += pASMSourceInfo->GetSubscribedBw();

	if (pASMSourceInfo->IsLive())
	{
	    bIsLive = TRUE;
	}
    }

    // Only force a bandwidth limit if we are playing a live clip
    // and the bandwidth that we are subscribed to is greater
    // than the connection bandwidth.
    return (bIsLive && ulTotal > m_ulHighestBandwidthAvail);
}

STDMETHODIMP
HXSM::UnRegisterSource(HXSource* pSource)
{
    LISTPOSITION		lPos;
    ASMSourceInfo*                 pASMSourceInfo = 0;
    ASMStreamInfo*                 pASMStreamInfo;
    HXBOOL			bFound = FALSE;

    lPos = m_pASMSourceInfo->GetHeadPosition();

    HXLOGL2(HXLOG_BAND, 
              "UnRegister Source %p %s", pSource, pSource->GetURL());

    // Search for the ASMSourceInfo object associated with pSource.
    // Remove it form the M_pASMSourceInfo list and call Done() on it.
    while (lPos)
    {
	pASMSourceInfo = (ASMSourceInfo *)m_pASMSourceInfo->GetAt(lPos);

	if (pASMSourceInfo->HasSource(pSource))
	{
	    m_pASMSourceInfo->RemoveAt(lPos);
	    pASMSourceInfo->Done();
	    bFound  = TRUE;
	    break;
	}
	m_pASMSourceInfo->GetNext(lPos);
    }
    
    if (!bFound)
    {
        // We didn't find a ASMSourceInfo object for pSource
	return HXR_OK;
    }

    lPos = m_pASMStreamInfo->GetHeadPosition();

    // Search for all the ASMStreamInfo objects associated with
    // pASMSourceInfo. Remove them from the list and destroy them.
    while (lPos)
    {
	pASMStreamInfo = (ASMStreamInfo*) m_pASMStreamInfo->GetAt(lPos);
	if (pASMStreamInfo->HasSourceInfo(pASMSourceInfo))
	{
	    /* RemoveAt returns the next position in the list.
	     * DO NOT use GetNext if you remove a node.
	     */
	    lPos = m_pASMStreamInfo->RemoveAt(lPos);

            // Destroy ASMStreamInfo object.
            // NOTE : pASMSourceInfo has a pointer to these
            //        objects as well so it is important that
            //        nothing tries to access those pointers
            //        after this point.
	    delete pASMStreamInfo;
	}
	else
	{
	    m_pASMStreamInfo->GetNext(lPos);
	}
    }
    
    HX_RELEASE(pASMSourceInfo);
    
    m_ulNumSources--;

    if (m_ulNumSources > 0)
    {
	m_State = REDIST;
	RecalcAccel();
    }

    return HXR_OK;
}


/* Called by HXPlayer at end of each presentation */
STDMETHODIMP
HXSM::PresentationDone(void)
{
    if (m_ulNumSources == 0)
    {
	m_ulHighestBandwidthAvail = 0;
	m_ulPeakUsedBandwidth = 0;
	m_bInitialHighBwAvail = TRUE;
	m_bPipeFull = FALSE;
	m_ulUpShiftBandwidthAvail = 0;
	m_ulNumReportsSinceUpShift = 2;
	m_ulOfferToRecalc = 0;
	m_State = HX_NONE;
	m_bDidOfferUpShiftToRecalc = FALSE;
	m_lLoss = 0;
	m_ulNextPacketWindow = 0;
	m_lPacketCounter = 0;
	m_ulUpShiftPastResistanceCount = 0;
	m_ulUpShiftTestPointScaleFactor = 4000;
    }

    return HXR_OK;
}

/* If the source has enough data, it may tell the bandwidth
 * manager to cut down on accelerated buffering.
 */
STDMETHODIMP
HXSM::ChangeAccelerationStatus(HXSource* pSource,
			        HXBOOL	   bMayBeAccelerated,
				HXBOOL	   bUseAccelerationFactor,
				UINT32	   ulAccelerationFactor)
{
    ASMSourceInfo* pASMSourceInfo = FindASMSourceInfo(pSource);

    if (pASMSourceInfo)
    {
	pASMSourceInfo->ChangeAccelerationStatus(bMayBeAccelerated,
	    bUseAccelerationFactor, ulAccelerationFactor);
    }
    else
    {
	/* Hmmm... ASM cannot help us here */
	/* Must be PNA. This may happen in case of TCP where the server sends 
	 * data 300% faster than the content bandwidth.
	 * Do the old style flow control by Pausing/Resuming the server
	 */
	if (!bMayBeAccelerated)
	{
	    pSource->DoPause();
	}
	else
	{
	    pSource->DoResume();
	}
    }

    return HXR_OK;
}

//ChangeBW() allows setting the maximum bandwidth limit on a source. Called by 
//RTSPProtocol::HandleSetParameterRequest()

STDMETHODIMP
HXSM::ChangeBW(UINT32 newBW, HXSource* pSource)
{
    ASMSourceInfo* pASMSourceInfo = FindASMSourceInfo(pSource);

    HXLOGL2(HXLOG_BAND, 
              "(%p)Request to change BW to %ld", pSource, newBW);
    
    if (pASMSourceInfo)
    {
        pASMSourceInfo->ChangeBW(newBW);

        m_State = REDIST;
        RecalcAccel();
    }

    HX_ASSERT(pASMSourceInfo);

    return HXR_OK; 
}

STDMETHODIMP
HXSM::GetUpshiftBW(THIS_ REF(UINT32) uUpshiftBW)
{
    HX_RESULT res = HXR_FAILED;

    if (m_upshiftBW.Full())
    {
        uUpshiftBW = m_upshiftBW.Median();
        res = HXR_OK;
    }

    return res;
}

void
HXSM::Recalc()
{
    CHXSimpleList::Iterator     i, j;
    ASMSourceInfo*                 pASMSourceInfo;
    ASMStreamInfo*                 pASMStreamInfo;
    INT32			lAggregateBandwidthUsage = 0;
    INT32			lAggregateBandwidthSent = 0;
    UINT32			ulSourceCount;
    UINT32			ulStreamCount;
    float			fBiasMean = (float) 0.;

    ulSourceCount = m_pASMSourceInfo->GetCount();
    ulStreamCount = m_pASMStreamInfo->GetCount();

    // Notify all the ASMSourceInfo objects that we are
    // starting a new Recalc()
    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
	pASMSourceInfo = (ASMSourceInfo*)(*i);

        if (pASMSourceInfo)
        {
            pASMSourceInfo->RecalcInit();
        }
    }

    // m_ulOfferToRecalc contains how much bandwidth is
    // currently available for the streams
    lAggregateBandwidthUsage = m_ulOfferToRecalc;

    INT32 lCorrectAggregateBandwidthUsage = lAggregateBandwidthUsage;

    /*
     * For each stream that is at a fixed bitrate, remove that bitrate
     * from our available bandwidth.
     */
    for (j = m_pASMStreamInfo->Begin(); j != m_pASMStreamInfo->End(); ++j)
    {
	pASMStreamInfo = (ASMStreamInfo*)(*j);

        if (pASMStreamInfo)
        {
            INT32 lBias = 0;

            if (HXR_OK == pASMStreamInfo->GetBiasFactor(lBias))
            {
                fBiasMean += lBias;
            }

            if (pASMStreamInfo->GetFixedBw())
            {
                lAggregateBandwidthUsage -= pASMStreamInfo->GetFixedBw();
                ulStreamCount--;
            }

            /* Init this for later */
            pASMStreamInfo->SetMasterRuleBookOffer(0);
        }
    }

    /* fBiasMean is not needed if everything has a fixed bandwidth */
    if (ulStreamCount != 0)
    {
	fBiasMean /= (float)ulStreamCount;
    }

    /*
     * Calculate the offer for each source that has a master rulebook
     * defining it's bandwidth division.
     */
    INT32 lNewAggregateBandwidthUsage = lAggregateBandwidthUsage;

    for (j = m_pASMStreamInfo->Begin(); j != m_pASMStreamInfo->End(); ++j)
    {
	pASMStreamInfo = (ASMStreamInfo*)(*j);

        // Offer bandwidth only to streams that aren't fixed since
        // we've already accounted for the fixed bandwidth streams
        // in the previous loop.  Fixes PR 134076:
        if (pASMStreamInfo  &&  !pASMStreamInfo->GetFixedBw())
        {
            // Compute the stream's fair share of the bandwidth
            UINT32 ulBiasedOffer = 
                pASMStreamInfo->ComputeBiasedOffer(lAggregateBandwidthUsage,
                                                   ulStreamCount,
                                                   fBiasMean);
            UINT32 ulNewOffer = 
                pASMStreamInfo->UpdateMasterOffer(ulBiasedOffer);

            // Subtract the offer for this stream from
            // the bandwidth available
            lNewAggregateBandwidthUsage -= ulNewOffer;
        }
    }
    
    lAggregateBandwidthUsage = lNewAggregateBandwidthUsage;

    /*
     * For each source that has a master rule book, evaluate it to find
     * out how much to distribute to each stream.
     */
    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
	pASMSourceInfo = (ASMSourceInfo*)(*i);

        if (pASMSourceInfo)
        {
            pASMSourceInfo->SetMasterRuleBookOffer();
        }
    }

    /*
     *  Now go through each of the streams that are not at a
     *  fixed bitrate and try to distribute the rest of the bandwidth.
     */
    UINT32 ulTakenBandwidth = 0;

    for (j = m_pASMStreamInfo->Begin(); j != m_pASMStreamInfo->End(); ++j)
    {
	pASMStreamInfo = (ASMStreamInfo*)(*j);

        if (pASMStreamInfo)
        {
            // Compute the streams fair share of the bandwidth
            UINT32 ulBiasedOffer = 
                pASMStreamInfo->ComputeBiasedOffer(lAggregateBandwidthUsage,
                                                   ulStreamCount,
                                                   fBiasMean);

            // Distribute the bandwidth
            ulTakenBandwidth += pASMStreamInfo->DistributeBw(ulBiasedOffer);
        }
    }

    lAggregateBandwidthUsage = lCorrectAggregateBandwidthUsage;

tryagain:

    if (lAggregateBandwidthUsage < (INT32)ulTakenBandwidth)
    {
        // The distributed bandwidth is >= the bandwidth
        // available

	/* Resistance is Futile.  You will be Real(tm)lyAssimilated */

        // Find the stream with the lowest resistance
	UINT32 ulLowestResistance = 0xffffffff;
	ASMStreamInfo* pLowestResistanceStream  = 0;
	for (j = m_pASMStreamInfo->Begin(); j != m_pASMStreamInfo->End(); ++j)
	{
	    pASMStreamInfo = (ASMStreamInfo*)(*j);

	    if (pASMStreamInfo &&
                (pASMStreamInfo->GetResistance() < ulLowestResistance))
	    {
		ulLowestResistance = pASMStreamInfo->GetResistance();
		pLowestResistanceStream = pASMStreamInfo;
	    }
	}

	if (ulLowestResistance == 0xffffffff)
	{
            // We can't do anything none of the
            // streams can be lowered
	}
	else
	{
            // Lower the bitrate of the stream with the lowest resistance
            ulTakenBandwidth -= pLowestResistanceStream->DropToLowerBw();

	    goto tryagain;
	}
    }

    UINT32 ulLeftOverForDropByN = lAggregateBandwidthUsage - ulTakenBandwidth;
    HXBOOL bForce = FALSE;

    for (j = m_pASMStreamInfo->Begin(); j != m_pASMStreamInfo->End(); ++j)
    {
	pASMStreamInfo = (ASMStreamInfo*)(*j);

        if (pASMStreamInfo &&
            pASMStreamInfo->SetSubscriptions(ulLeftOverForDropByN))
        {
            bForce = TRUE;
        }
    }

    // Flush the rule subscriptions for all sources. 
    // This is when the changes get sent to the server.
    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
	pASMSourceInfo = (ASMSourceInfo*)(*i);

        if (pASMSourceInfo)
        {
            pASMSourceInfo->FlushSubscriptions();
        }
    }

    if ((m_State == REDO_ACCEL) ||
        (m_State == CONGESTION))
    {
	RecalcAccel();
    }
    else if (m_State == INIT)
    {
	m_State = INIT_REDIST;
	RecalcAccel();
    }
    else if (bForce)
    {
	m_State = REDO_ACCEL;
	RecalcAccel();
    }
}

void
HXSM::RecalcAccel()
{
    CHXSimpleList::Iterator     i, j;
    ASMSourceInfo*                 pASMSourceInfo;
    ASMStreamInfo*                 pASMStreamInfo;
    UINT32 ulAggregateUsed = 0;
    UINT32 ulTotalMaxSubscribedBw = 0;

    if (m_State == INIT)
    {
        // This does the inital bandwidth distribution
        // based on the connection bandwidth.

        // Setup offer for Recalc().
	m_ulOfferToRecalc = m_ulHighestBandwidthAvail;

	if (m_ulOfferToRecalc > m_ulMaxAccelBitRate)
	{
	    m_ulOfferToRecalc = m_ulMaxAccelBitRate;
	}

	HXLOGL3(HXLOG_TRAN, 
			"INIT Offer to Recalc() %d", m_ulOfferToRecalc);

        // Recalculate bandwidth distribution
	Recalc();
	return;
    }

    // Clear subscribed bandwidth values
    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
	pASMSourceInfo = (ASMSourceInfo*)(*i);

        if (pASMSourceInfo)
        {
            pASMSourceInfo->ClearSubscribedBw();
        }
    }

    // Compute the maximum bandwidth you can subscribe to as well
    // as how much bandwidth we are currently subscribed to.
    for (j = m_pASMStreamInfo->Begin(); j != m_pASMStreamInfo->End(); ++j)
    {
	pASMStreamInfo = (ASMStreamInfo*)(*j);

        UINT32 ulCurBw = pASMStreamInfo->GetCurrentSubBw();
        UINT32 ulMaxBw = pASMStreamInfo->GetMaxSubscribeBw();

        pASMStreamInfo->UpdateSourceSubBw(ulCurBw, ulMaxBw);

        ulTotalMaxSubscribedBw += ulMaxBw;
        ulAggregateUsed += ulCurBw;
    }

    UINT32 ulNumBehindSources		= 0;
    UINT32 ulNumSlightlyBehindSources	= 0;
    INT32  lAggregateBandwidthUsage	= 0;
    INT32  ulMaxNeededBW                = 0;
    HXBOOL   bAllZeroBw			= TRUE;
    HXBOOL   bFastStart                   = FALSE;
    
    // Collect info about the sources
    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
	pASMSourceInfo = (ASMSourceInfo*)(*i);

        if (pASMSourceInfo)
        {
            pASMSourceInfo->GetSrcStats(bFastStart,
                                        ulNumBehindSources,
                                        ulNumSlightlyBehindSources,
                                        lAggregateBandwidthUsage,
                                        bAllZeroBw,
                                        ulMaxNeededBW);
        }
    }

    // so if we are in fast start, let's check to see if we don't want to be in
    // fast start!
    double maxPossibleAccelRatio = 0.;
    if (ulMaxNeededBW > 0)
    {
        maxPossibleAccelRatio = (double) m_ulOriginalHighestBandwidthAvail / (double) ulMaxNeededBW;
    }

    // Set the maximum acceleration ratio for all the sources
    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
	pASMSourceInfo = (ASMSourceInfo*)(*i);

        if (pASMSourceInfo)
        {
            pASMSourceInfo->SetMaxAccelRatio(maxPossibleAccelRatio);
            
            // turn this stuff of if we are on modem. *sigh* It probably sort of works on 
            // modems too. This value makes it work on ISDN, that's ok by me, but if people
            // don't like it, I can change it.         
            if (maxPossibleAccelRatio <= 1.0 || 
                (m_ulOriginalHighestBandwidthAvail < 110000)) // it now works on DSL or higher
            {
                bFastStart = FALSE;
                if (pASMSourceInfo->IsFastStart())
                {
                    HXLOGL2(HXLOG_TRAN, "(%p)ASM %d - Leaving TurboPlay", pASMSourceInfo, __LINE__);
                    pASMSourceInfo->LeaveFastStart(TP_OFF_BY_NOTENOUGHBW);
                }
            }
        }
    }

    if (m_bPipeFull)
    {
        // Update m_ulHighestBandwidthAvail &
        // m_ulPeakUsedBandwidth with the current
        // bandwidth usage.
        UpdateHighBwAvailOnPipeFull(lAggregateBandwidthUsage);
    }

    if (m_State == REDIST)
    {
        // Redistribute the bandwidth.
        m_State = REDO_ACCEL;
        m_ulOfferToRecalc = m_ulHighestBandwidthAvail;
        Recalc();
	return;
    }

    if ((m_State == REDO_ACCEL) || (m_State == INIT_REDIST))
    {
        UINT32 ulTotalBwAvailable = lAggregateBandwidthUsage;
        
        if ((m_bInitialHighBwAvail) || (m_State == REDO_ACCEL))
        {
            if (bFastStart && (m_State == INIT_REDIST))
            {
                ulTotalBwAvailable = m_ulMaxAccelBitRate;
            }
            else if ((INT32)m_ulPeakUsedBandwidth <= lAggregateBandwidthUsage)
            {
                ulTotalBwAvailable = m_ulPeakUsedBandwidth;
            }
        }

        HX_ASSERT(ulTotalBwAvailable > 0); // Rahul's Crazy, This won't ever happen!

        DistributeAndSetBandwidth(ulAggregateUsed,
                                  ulTotalBwAvailable);

        m_State = HX_NONE;
	return;
    }

    if (m_State == CHILL_BUFFERING)
    {
	m_ulOfferToRecalc = lAggregateBandwidthUsage;

	HXLOGL3(HXLOG_TRAN, 
			"CHILL to Recalc() %d", m_ulOfferToRecalc);

	m_State = HX_NONE;

	Recalc();
    }

    if ((ulNumBehindSources) ||
	((lAggregateBandwidthUsage > ((INT32)m_ulMaxAccelBitRate + 100)) &&
	    (bAllZeroBw)))
    {
        // At least one source is behind or the amount of bandwidth
        // we are subscribed to is > the max acceleration bitrate and
        // we have not received any data.

	if ((lAggregateBandwidthUsage > (INT32)m_ulMaxAccelBitRate) &&
	    (bAllZeroBw))
	{
	    lAggregateBandwidthUsage = m_ulMaxAccelBitRate;
	}

	// XXXRA change m_ulNumReportsSinceUpShift to m_lNumReportsSinceUpShift
	m_ulNumReportsSinceUpShift = -2;
	UINT32 ulLow  = (UINT32)(m_ulLastStableBandwidth * 0.90);
	UINT32 ulHigh = (UINT32)(m_ulLastStableBandwidth * 1.10);
	/*
	 * XXXSMP Maybe we don't want to use stable point when the aggregate
	 * detected is more then the stable point?
	 */
	if ((lAggregateBandwidthUsage > (INT32)ulLow) && 
	    (lAggregateBandwidthUsage < (INT32)ulHigh))
	{
	    /*
	     * If we are close to the last stable bandwidth, then let's
	     * try that one again.
	     */
	    lAggregateBandwidthUsage = m_ulLastStableBandwidth;

	    HXLOGL3(HXLOG_TRAN, 
			"Used Stable Point %d", m_ulLastStableBandwidth);


	    m_ulLastStableBandwidth = 0;

	    m_ulUpShiftTestPointScaleFactor = MAX(1500, 
		(UINT32)(m_ulUpShiftTestPointScaleFactor * 0.85));
	}
	if (m_State != CONGESTION)
	{
            float fRecalcOffer = lAggregateBandwidthUsage * 0.97f;
            EnterCongestionState(fRecalcOffer, ulAggregateUsed);
	    return;
	} //m_State != CONGESTION
	else
	{
            float fConservativeOffer = 
                (float)lAggregateBandwidthUsage * 0.70f;
            float fRequiredOffer = 
                (float)lAggregateBandwidthUsage * 0.97f;

            HandleCongestionState(fConservativeOffer,
                                  fRequiredOffer,
                                  ulAggregateUsed);
	}
    }
    else if (!ulNumSlightlyBehindSources)
    {
	INT32 lAccelTestPoint;
	double dRFactor = 1.05;

	if ((INT32)m_ulUpShiftBandwidthAvail > lAggregateBandwidthUsage)
	{
	    lAccelTestPoint = (INT32) 
		((m_ulUpShiftBandwidthAvail - lAggregateBandwidthUsage)
		* ((float)m_ulUpShiftTestPointScaleFactor / 10000.0)
		    + lAggregateBandwidthUsage);
	}
	else
	{
	    lAccelTestPoint = lAggregateBandwidthUsage;
	}

	UINT32 ulAccelTestPoint =
	    (lAccelTestPoint > 0) ? (UINT32)lAccelTestPoint : 0;
	HXBOOL bResistanceLimited = FALSE;
	HXBOOL bWentHigherThanResistanceRate = FALSE;

	if (ulAccelTestPoint > m_ulMaxAccelBitRate)
	{
	    ulAccelTestPoint = m_ulMaxAccelBitRate;
	}
	
	if (ulAccelTestPoint > m_ulResistanceBitRate)
	{
	    UINT32 ulActualResistanceBitRate = m_ulResistanceBitRate;

	    if (ulActualResistanceBitRate < ulAggregateUsed)
	    {
		ulActualResistanceBitRate = (UINT32)(ulAggregateUsed * 1.05);
	    }

	    UINT32 ulOldAccelTestPoint = ulAccelTestPoint;
	    if (ulAccelTestPoint > ulActualResistanceBitRate)
	    {
		UINT32 bHowManyReports =
		    (ulActualResistanceBitRate <
		      ulTotalMaxSubscribedBw) ? 5 : 10;

		if (m_ulUpShiftPastResistanceCount > bHowManyReports)
		{
		    bWentHigherThanResistanceRate = TRUE;

		    if ((m_ulOriginalResistanceBitRate >
		        ulActualResistanceBitRate) &&
			(ulActualResistanceBitRate < ulTotalMaxSubscribedBw))
		    {
			dRFactor = (ulActualResistanceBitRate +
			    ((m_ulOriginalResistanceBitRate -
			    ulActualResistanceBitRate) * 0.10)) /
			    (double)ulActualResistanceBitRate;
			if (dRFactor < 1.01)
			{
			    dRFactor = 1.01;
			}

		        HXLOGL3(HXLOG_TRAN, 
			    "Resistance Accel Factor %0.2f", dRFactor);

		    }
		    else
		    {
			dRFactor = 1.01;
		    }
 		    ulAccelTestPoint = (UINT32)(ulActualResistanceBitRate *
		        dRFactor);
		}
		else
		{
		    ulAccelTestPoint = ulActualResistanceBitRate;
		}
		if (ulOldAccelTestPoint < ulAccelTestPoint)
		{
		    ulAccelTestPoint = ulOldAccelTestPoint;
		}
		else
		{
		    bResistanceLimited = TRUE;
		}
	    }
	}

	if ((m_ulNumReportsSinceUpShift >= 2) &&
		((INT32)m_ulHighestBandwidthAvail < lAggregateBandwidthUsage))
	{
	    m_ulHighestBandwidthAvail = lAggregateBandwidthUsage;
	    m_ulPeakUsedBandwidth = lAggregateBandwidthUsage;
	    m_bInitialHighBwAvail = FALSE;
	}

	HXLOGL3(HXLOG_TRAN,
	    "UP Bw Report: Num=%d, Avail=%d, CurrentBw=%d, TestPoint=%d", 
	    m_ulNumReportsSinceUpShift, m_ulUpShiftBandwidthAvail,
	    lAggregateBandwidthUsage, ulAccelTestPoint);

	HXBOOL bDidChange = FALSE;
	HXBOOL bBrokeMax = FALSE;
	if ((m_ulNumReportsSinceUpShift >= 2) &&
	    (INT32) (ulAccelTestPoint) > lAggregateBandwidthUsage)
	{
	    m_ulLastStableBandwidth = lAggregateBandwidthUsage;
	    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
	    {
		pASMSourceInfo = (ASMSourceInfo*)(*i);

		IHXThinnableSource* pThin = 0;
		UINT32 ulSourceBandwidth = pASMSourceInfo->GetSubscribedBw();

		UINT32 ulNewValue = 
			(UINT32)(
			(float)ulSourceBandwidth /
			(float)ulAggregateUsed *
			(float)ulAccelTestPoint);

                ulNewValue = 
                    pASMSourceInfo->ApplyNonFastStartLimits(ulNewValue);
                
                ulNewValue =
                    pASMSourceInfo->Apply3xUpshiftLimit(ulNewValue);

		if (pASMSourceInfo->m_ulLastSetDelivery >
			pASMSourceInfo->m_ulMaxSubscribedBw)
		{
		    bBrokeMax = TRUE;
		}

		if (pASMSourceInfo->m_bMayBeAccelerated &&
		   (HXR_OK == pASMSourceInfo->m_pSource->
		    QueryInterface(IID_IHXThinnableSource, (void **)&pThin)))
		{
		    double Factor;

		    if (bResistanceLimited)
		    {
			Factor = 1.005;
		    }
		    else if (pASMSourceInfo->IsLive())
		    {
			Factor = 1.01;
		    }
		    else
		    {
			Factor = 1.05;
		    }

		    if ((ulNewValue > (pASMSourceInfo->m_ulLastSetDelivery * Factor)) && 
			pASMSourceInfo->m_bSourceAccelAllowed)
		    {
			HXLOGL2(HXLOG_TRAN, 
			"(%p)Accelerating: NewTransmissionRate=%d", pASMSourceInfo->m_pSource, ulNewValue);

			m_ulUpShiftPastResistanceCount = 0;

			if (ulNewValue > ulSourceBandwidth)
			{
			    pASMSourceInfo->m_pSource->LeaveBufferedPlay();
			}

			bDidChange = TRUE;

			/*
			 * WHOA!  If this is a timestamp delivered source
			 * i.e. 5.0 thinning, then the imperical Getbandwidth()
			 * will never reach the rate that we set, so we help
			 * it along a bit (just a bit of a nudge, eh?).
			 */
			if (pASMSourceInfo->m_bTimeStampDelivery)
			{
			    pASMSourceInfo->m_ulIncomingBandwidth = ulNewValue;
			}
			m_ulNumReportsSinceUpShift = 0;
			pASMSourceInfo->m_bInvalidUpReport = TRUE;
			m_bDidOfferUpShiftToRecalc = FALSE;
			pThin->SetDeliveryBandwidth(
			    (pASMSourceInfo->m_ulLastSetDelivery = ulNewValue), 0);
		    }
		}
		HX_RELEASE(pThin);
	    }
	}
	if ((bDidChange == TRUE) && (bWentHigherThanResistanceRate))
	{
	    m_ulResistanceBitRate = (UINT32)(m_ulResistanceBitRate * dRFactor);
	    HXLOGL2(HXLOG_TRAN, 
	        "Went over ResistanceBitRate %d", m_ulResistanceBitRate);
	}
	if (((bDidChange == FALSE) &&
	    (m_ulNumReportsSinceUpShift > NUM_REPORTS_NEEDED_TO_UPSHIFT) ||
	    (bBrokeMax == TRUE)) && (m_bDidOfferUpShiftToRecalc == FALSE))
	{
	    m_State = HX_NONE;
	    m_bDidOfferUpShiftToRecalc = TRUE;
	    m_ulOfferToRecalc = lAggregateBandwidthUsage;

	    HXLOGL2(HXLOG_TRAN, 
			"Upshift Offer to Recalc() %d", m_ulOfferToRecalc);


	    Recalc();
	    return;
	}
    }
}

void HXSM::DistributeAndSetBandwidth(const UINT32 ulAggregateUsed,
                                     const UINT32 ulTotalBwAvailable)
{
    CHXSimpleList::Iterator i;
    ASMSourceInfo* pASMSourceInfo;

    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
        pASMSourceInfo = (ASMSourceInfo*)(*i);
        
        if (pASMSourceInfo)
        {
            pASMSourceInfo->DistributeAndSetBandwidth(ulAggregateUsed,
                                                      ulTotalBwAvailable,
                                                      m_fAccelerationFactor,
                                                      m_ulOriginalHighestBandwidthAvail,
                                                      m_ulResistanceBitRate,
                                                      m_bEnableSDB);
        }
    }
}

void HXSM::EnterCongestionState(float fRecalcOffer,
                                const UINT32 ulAggregateUsed)
{
    CHXSimpleList::Iterator     i, j;
    ASMSourceInfo* pASMSourceInfo;

    // Look for sources that could get downshifted because of
    // the new recalc offer.
    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
        pASMSourceInfo = (ASMSourceInfo*)(*i);

        UINT32 ulSourceBandwidth = pASMSourceInfo->GetSubscribedBw();
        UINT32 ulNewValue = 
            (UINT32)(
                (float)ulSourceBandwidth /
                (float)ulAggregateUsed *
                (float)fRecalcOffer);

        if (ulNewValue < ulSourceBandwidth)
        {
            // We have found a source that may get downshifted.
            // Attempt ASM Switching to reduce bandwidth usage.
            m_State = CONGESTION;
            m_ulOfferToRecalc = (UINT32)fRecalcOffer;


            HXLOGL2(HXLOG_TRAN, "CONGESTION to Recalc() %d", m_ulOfferToRecalc);


            // once again, if we are in start start mode we need to 
            // turn it off. It may be off already, but I just want to 
            // make sure.

            if (pASMSourceInfo->IsFastStart())
            {
                HXLOGL2(HXLOG_TRAN, "(%p)ASM %d - Leaving TurboPlay", pASMSourceInfo, __LINE__);
                pASMSourceInfo->LeaveFastStart(TP_OFF_BY_NETCONGESTION);
            }

            Recalc();
            return;
        }
    }
    m_State = CONGESTION;
    RecalcAccel();
}

void HXSM::HandleCongestionState(float fConservativeOffer,
                                 float fRequiredOffer,
                                 const UINT32 ulAggregateUsed)
{
    CHXSimpleList::Iterator     i, j;
    ASMSourceInfo* pASMSourceInfo;

    HXBOOL bLossBehind = FALSE;
    m_State = HX_NONE;
    UINT32 ulTotalBandwidth = 0;
    for (i = m_pASMSourceInfo->Begin(); i != m_pASMSourceInfo->End(); ++i)
    {
        pASMSourceInfo = (ASMSourceInfo*)(*i);

        if (pASMSourceInfo)
        {
            UINT32 ulSourceBandwidth = pASMSourceInfo->GetSubscribedBw();

            UINT32 ulNewValue = 
                (UINT32)(
                    (float)ulSourceBandwidth /
                    (float)ulAggregateUsed *
                    fConservativeOffer);
            
            if (ulNewValue < ulSourceBandwidth)
            {
                /* Can't be conservative, so go use what we need */
                ulNewValue = 
                    (UINT32)(
                        (float)ulSourceBandwidth /
                        (float)ulAggregateUsed *
                        fRequiredOffer);
                
                // once again, if we are in start start mode we need to 
                // turn it off. It may be off already, but I just want to 
                // make sure.
                
                if (pASMSourceInfo->IsFastStart())
                {
                    HXLOGL2(HXLOG_TRAN, 
                              "(%p)ASM %d - Leaving TurboPlay", 
                               this, __LINE__);
                    
                    pASMSourceInfo->LeaveFastStart(TP_OFF_BY_NETCONGESTION);
                }
            }

            ulTotalBandwidth += 
                pASMSourceInfo->ChangeBwBecauseOfCongestion(ulNewValue,
                                                            bLossBehind);
        }
    }
    if (bLossBehind)
    {
        m_ulNumReportsSinceUpShift = -10;
        m_ulResistanceBitRate = MAX(15000, ulTotalBandwidth);
        HXLOGL2(HXLOG_TRAN, 
                  "Resistance Move %d", m_ulResistanceBitRate);
    }

}

typedef struct {
    UINT32 m_ulBandwidth;
    UINT32 m_ulHighBw;
    UINT32 m_ulPeakBw;
} HXSMBwInfo;

static HXSMBwInfo z_bwTbl[] = {
    {14400, 11000, 12240},
    {19200, 14400, 16320},
    {28800, 21600, 24480},
    {33600, 25000, 28560},
    {34400, 34400, 34400},
    {57600, 50000, 51840},
    {65536, 56360, 58980},
    {115200, 100000, 104000}
};
    
void HXSM::InitBandwidthVars(IUnknown* pUnknown)
{
    IHXPreferences* pPreferences = NULL;

    pUnknown->QueryInterface(IID_IHXPreferences, (void **)&pPreferences);
    

    UINT32 ulTemp = 0;
    
    IHXConnectionBWInfo* pConnBWInfo = NULL;
    if (HXR_OK == pUnknown->QueryInterface(IID_IHXConnectionBWInfo,
                                           (void**)&pConnBWInfo))
    {
        pConnBWInfo->GetConnectionBW(ulTemp, FALSE);
        HX_ASSERT(ulTemp);
    }
    else
    {
        /* Get initial bandwidth guess from Prefs */
        ReadPrefUINT32(pPreferences, "Bandwidth", ulTemp);
    }
    HX_RELEASE(pConnBWInfo);

    if (ulTemp != 0)
    {
        m_bInitialHighBwAvail = TRUE;
        
        /* Translate the bandwidth from prefs into a starting point */

        HXBOOL bFoundBw = FALSE;
        
        int bwCount = sizeof(z_bwTbl) / sizeof(HXSMBwInfo);
        for (int i = 0; !bFoundBw && (i < bwCount); i++)
        {
            if (ulTemp == z_bwTbl[i].m_ulBandwidth)
            {
                m_ulHighestBandwidthAvail = z_bwTbl[i].m_ulHighBw;
                m_ulPeakUsedBandwidth = z_bwTbl[i].m_ulPeakBw;
                bFoundBw = TRUE;
            }
        }
        if (!bFoundBw)
        {
            if (ulTemp >  150000)   
            {
                m_ulHighestBandwidthAvail = (UINT32)(ulTemp * 0.90);
                m_ulPeakUsedBandwidth = (UINT32)(ulTemp * 0.91);
            }
            else
            {
                m_ulHighestBandwidthAvail = (UINT32)(ulTemp * 0.85);
                m_ulPeakUsedBandwidth = (UINT32)(ulTemp * 0.90);
            }
        }
        
        /*
         * Figure out the resistance bitrate for upshifting.
         * Modems get 65k.
         * DSL / Low BW LANs get their pref value.
         * High bandwidth devices cap at 600k
         *    (unless the presentation is more)
         */
        if (ulTemp < 65000)
        {
            m_ulResistanceBitRate = 65000;
        }
        else if (ulTemp < 600000)
        {
            m_ulResistanceBitRate = m_ulPeakUsedBandwidth;
        }
        else
        {
            m_ulResistanceBitRate = 600000;
        }
        
        m_ulOriginalResistanceBitRate = m_ulResistanceBitRate;
    }
    else
    {
        /* Wild Guess */
        m_ulHighestBandwidthAvail = 40000;
        m_ulPeakUsedBandwidth = 40000;
    }
    
    ReadPrefFLOAT(pPreferences, "AccelerationFactor", m_fAccelerationFactor);

    /* Get MaxBandwidth from Prefs */
    ReadPrefUINT32(pPreferences, "MaxBandwidth", m_ulMaxAccelBitRate); 
    
    HX_RELEASE(pPreferences);
    m_ulOriginalHighestBandwidthAvail = m_ulHighestBandwidthAvail;
}

ASMSourceInfo* HXSM::FindASMSourceInfo(HXSource* pSource)
{
    ASMSourceInfo* pRet = NULL;

    if (m_pASMSourceInfo)
    {
        LISTPOSITION lPos = m_pASMSourceInfo->GetHeadPosition();

        while (lPos)
        {
            ASMSourceInfo* pASMSourceInfo = 
                (ASMSourceInfo *)m_pASMSourceInfo->GetAt(lPos);

            if (pASMSourceInfo && pASMSourceInfo->HasSource(pSource))
            {
                pRet = pASMSourceInfo;
                break;
            }
            m_pASMSourceInfo->GetNext(lPos);
        }
    }

    return pRet;
}

void HXSM::UpdateHighBwAvailOnPipeFull(INT32 lBwUsage)
{
    /*
     * Adjust the highest available bandwidth because we have found
     * the maximum bandwidth that the pipe can handle.  We do this
     * so that sources that get added in the future will have
     * some information about the max. bandwidth that exists.  This
     * value is aggresive because a source will want to consume everything
     * it can and fall back if it goes over the top.
     */
    HX_ASSERT(lBwUsage >= 100);
    if (lBwUsage < 100)
    {
        /*
         * Please have at *least* 100bps before attempting to
         * run the RealPlayer.
         */
        lBwUsage = 100;
    }
    m_ulHighestBandwidthAvail = (UINT32)(lBwUsage);
    m_ulPeakUsedBandwidth = (UINT32)(lBwUsage);
    m_bInitialHighBwAvail = FALSE;
    m_bPipeFull = FALSE;
}

HX_RESULT HXSM::NetInterfacesUpdated()
{
    // Clear the upshift BW state since 
    // the interfaces changed and it is
    // now likely invalid
    m_upshiftBW.Reset();

    return HXR_OK;
}

HX_RESULT HXSM::static_NetInterfacesUpdated(void* pObj)
{
    HX_RESULT res = HXR_OK;

    if (pObj)
    {
        HXSM* pHXSM = (HXSM*)pObj;

        res = pHXSM->NetInterfacesUpdated();
    }

    return res;
}

ASMSourceInfo::ASMSourceInfo(HXSource* pSource, HXSM* pHXASM,
                             IUnknown* pUnknown)
    : m_ulLastReportTime(0)
    , m_ulIncomingBandwidth(0)
    , m_ulRateBeforeDeAccel(0)
    , m_lTimeDiffBase(0)
    , m_ulBytesBehind(0)
    , m_lLastBehindTime(0)
    , m_ulLastSetDelivery(0xffffffff)
    , m_ulSubscribedBw(0)
    , m_ulMaxSubscribedBw(0)
    , m_bBehind(FALSE)
    , m_bLossBehind(FALSE)
    , m_bSlightlyBehind(FALSE)
    , m_bTimeStampDelivery(FALSE)
    , m_bPendingChill(FALSE)
    , m_bInvalidUpReport(FALSE)
    , m_bPerfectPlay(FALSE)
    , m_bIsDone(FALSE)
    , m_bMayBeAccelerated(TRUE)
    , m_bTryToUpShift(FALSE)
    , m_bAdjustBandwidth(FALSE)
    , m_ulLowestBandwidthBeforeTimeStamp(0)
    , m_bSourceAccelAllowed(TRUE)
    , m_bSourceDecelAllowed (TRUE)
    , m_bSlidingBwWindowReady(FALSE)
    , m_pMasterRuleBook(0)
    , m_pStreams(0)
    , m_TransportType(TNG_UDP)
    , m_pSource(pSource)
    , m_pSBI(0)
    , THRESHOLD(1000)
    , m_lRefCount(0)
    , m_ulBwDetectionDataCount(0)
    , m_ulBwDetectionDataLen(0)
    , m_ulSlidingWindowLocation(0)
    , m_pBwDetectionData(NULL)
    , m_pHXASM(pHXASM)
    , m_pSubscriptionVariables(NULL)
    , m_pContext(NULL)
{
    m_lOuterThreshold = THRESHOLD;
    IHXValues* pHeader = 0;

    pSource->AddRef();

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

    pSource->QueryInterface(IID_IHXSourceBandwidthInfo,
                            (void**)&m_pSBI);

    /* Local source is always considered in perfect play mode UNLESS we are
	 * in simulated network playback mode (used Preview mode in the Encoder)
	 */
    m_bPerfectPlay = pSource->IsPerfectPlay() && !pSource->IsSimulatedNetworkPlayback();

    SetupLoadTestVars(pUnknown);

    m_pStreams = new ASMStreamInfo*[pSource->GetStreamCount()];
    m_ulLowestBandwidthBeforeTimeStamp = 0;

    m_pContext = pUnknown;
    HX_ADDREF(m_pContext);
}

ASMSourceInfo::~ASMSourceInfo()
{
    HX_VECTOR_DELETE(m_pBwDetectionData);
    HX_VECTOR_DELETE(m_pStreams);
    HX_DELETE(m_pMasterRuleBook);
    HX_RELEASE(m_pSubscriptionVariables);
    HX_RELEASE(m_pContext);
}


HXBOOL
ASMSourceInfo::AllocBWDetectionData(UINT32 ulReqSize)
{
    HXBOOL bOk = TRUE;

    // Our current array is too small
    if (ulReqSize > m_ulBwDetectionDataLen)
    {
	BwDetectionData* pTemp = new BwDetectionData[ulReqSize];
	if (!pTemp)
	{
	    bOk = FALSE;
	}
	else
	{
	    if (m_pBwDetectionData)
	    {
		memcpy(pTemp, m_pBwDetectionData, m_ulBwDetectionDataLen * sizeof(BwDetectionData)); /* Flawfinder: ignore */
		HX_VECTOR_DELETE(m_pBwDetectionData);
	    }
	    m_pBwDetectionData = pTemp;
	    m_ulBwDetectionDataLen = ulReqSize;
	}
    }

    return bOk;
}


void
ASMSourceInfo::Done()
{
    HX_RELEASE(m_pSBI);
    HX_RELEASE(m_pSource);
    m_bIsDone	= TRUE;
}

STDMETHODIMP_(UINT32)
ASMSourceInfo::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
ASMSourceInfo::Release(void)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
ASMSourceInfo::QueryInterface
(
    REFIID interfaceID,
    void** ppInterfaceObj
)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)this },
            { GET_IIDHANDLE(IID_IHXBandwidthManagerInput), (IHXBandwidthManagerInput*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), interfaceID, ppInterfaceObj);
}


STDMETHODIMP
ASMSourceInfo::ReportDataPacket
(
    UINT32 ulTimeStamp,
    UINT32 ulArrivedTimeStamp,
    UINT32 ulSize
)
{
    if (m_bIsDone)
    {
	return HXR_OK;
    }

    UINT32 ulCurrentTime = HX_GET_TICKCOUNT();
    if (!m_pHXASM->m_ulNextPacketWindow)
    {
	m_pHXASM->m_ulNextPacketWindow = ulCurrentTime;
    }

    if (ulSize == REPORT_DATA_PACKET_LOST)
    {
        ULONG32 ulNumLost = ulArrivedTimeStamp-ulTimeStamp+1;
	//XXXgfw Don't allow Decel if this is a load test.....
	if (m_bSourceDecelAllowed)
	{
            m_pHXASM->m_lLoss += ulNumLost;    
	}
        m_pHXASM->m_lPacketCounter += ulNumLost;
	return HXR_OK;
    }

    m_pHXASM->m_lPacketCounter++;

    // Make sure we have a buffer for bandwidth detection!
    if (!m_pBwDetectionData)
    {
	//XXXNH: SMP pulled these numbers out of his hat, so let's
	// stick with them for now.
	UINT32 uEstSize = m_TransportType == TNG_TCP ? 300 : 20;
	
	if (!AllocBWDetectionData(uEstSize))
	    return HXR_OUTOFMEMORY;
    }

    if (CALCULATE_ELAPSED_TICKS(m_pHXASM->m_ulNextPacketWindow,
    	ulCurrentTime) > 3000)
    {
	m_pHXASM->m_ulNextPacketWindow = ulCurrentTime;

	UINT32 ulLossPercentage = (UINT32)(
	    100 * (float)m_pHXASM->m_lLoss / (float)m_pHXASM->m_lPacketCounter);

	HXLOGL3(HXLOG_TRAN, "(%p)Loss %d", m_pSource, ulLossPercentage);

	/* Don't do anything unless we have 4% Loss */
	HXBOOL bDeAccel = (ulLossPercentage >= 4);

	/* Cut down bandwidth usage ONLY if it won't cause a downshift */
	bDeAccel = bDeAccel &&
		(m_ulLastSetDelivery * 0.95 > m_ulSubscribedBw);

	/* Don't throttle back if we're in chill acc buffering */
	bDeAccel = bDeAccel && (m_ulRateBeforeDeAccel == 0);

	/* If Loss is over 20%, always throttle back */
	bDeAccel = bDeAccel || (ulLossPercentage >= 20);

	if (bDeAccel)
	{
	    if (ulLossPercentage >= 20)
	    {
		/* 
		 * make sure to never go into timestamp delivery/keyframe mode
		 * due to loss
		 */
	        m_ulIncomingBandwidth = (UINT32) MAX(m_ulLastSetDelivery * 0.85, 
				m_ulLowestBandwidthBeforeTimeStamp * 1.05);
	    }
	    else
	    {
		if (ulLossPercentage > 15)
		{
		    ulLossPercentage = 15;
		}
	        
		// do not change subscription due to "less than 20% loss"
		m_ulIncomingBandwidth = (UINT32) MAX(m_ulLastSetDelivery *
		    (1 - (ulLossPercentage / (float)100)),
		    m_ulSubscribedBw * 1.05);
	    }

            if (m_pSource->m_bFastStart)
            {
                HXLOGL2(HXLOG_TRAN, "(%p)ASM %d - Leaving TurboPlay", m_pSource, __LINE__);
                m_pSource->LeaveFastStart(TP_OFF_BY_NETCONGESTION);
                if (m_ulIncomingBandwidth > m_ulMaxSubscribedBw * 1.05)
                {
                    m_ulIncomingBandwidth = m_ulMaxSubscribedBw * 1.05;
                }
            }

	    m_bLossBehind = TRUE;
	    HXLOGL2(HXLOG_TRAN, "(%p)Lower Loss %d", m_pSource, m_ulIncomingBandwidth);
	    /* Signal Recalc to adjust the highest available bandwidth */
	    m_pHXASM->m_bPipeFull = TRUE;
	    m_bBehind = TRUE;

	    m_pHXASM->RecalcAccel();
	    m_bBehind = FALSE;
	    m_bLossBehind = FALSE;
	}
	else if (ulLossPercentage <= 1)
	{
	    m_pHXASM->m_ulUpShiftPastResistanceCount++;
	}

	if (ulLossPercentage > 2)
	{
	    m_pHXASM->m_ulUpShiftPastResistanceCount = 0;
	}

	m_pHXASM->m_lLoss = 0;
	m_pHXASM->m_lPacketCounter = 0;
    }

    if (m_bSlidingBwWindowReady)
    {
	//INT32 i;
	UINT32 ulPrevLocation = m_ulSlidingWindowLocation;

	/*
	 *  Set the data for the current packet
	 */
	m_pBwDetectionData[m_ulSlidingWindowLocation].
	    m_ulSize	    = ulSize;
	m_pBwDetectionData[m_ulSlidingWindowLocation].
	    m_ulTimeStamp	    = ulTimeStamp;
	m_pBwDetectionData[m_ulSlidingWindowLocation].
	    m_ulATime	    = ulArrivedTimeStamp;

	m_ulSlidingWindowLocation++;

	/*
	 * wrap around if at the end
	 */
	if (m_ulSlidingWindowLocation == m_ulBwDetectionDataCount)
	{
	    m_ulSlidingWindowLocation = 0;
	}

	/*
	 * count up all data for the whole window
	 */
	//UINT32 ulWindowSize = 0;
	//for (i = 0; i < m_ulBwDetectionDataCount; i++)
	//{
	//    ulWindowSize += m_pBwDetectionData[i].m_ulSize;
	//}

	/*
	 *  m_ulSlidingWindowLocation now points to the very first
	 *  info in the window.  Take the diff in time from the data
	 *  that we just entered (ulPrevLocation) and the time of the
	 *  first data.
	 */
	//UINT32 ulWindowRealTime = m_pBwDetectionData[ulPrevLocation].m_ulATime
	//    - m_pBwDetectionData[m_ulSlidingWindowLocation].m_ulATime;

	/*
	 *  Calculate bandwidth for the window in bytes * 8 / millis.
	 *  I have no idea why it is bytes * 8. //XXXPM?
	 *  If we are in constant bitrate mode, then this calculated
	 *  bitrate will be compared to the constant bitrate.  If we
	 *  are not in constant bitrate mode, then this calculated
	 *  bitrate will be compared to the bitrate coming from the
	 *  bandwidth reports.
	 */
	//m_ulLastBandwidthReport = (UINT32) (ulWindowSize * 8 /
	//    ((ulWindowRealTime) / 1000.0));

	// This is the short-term bandwidth count (sans buffering fudge)
    }
    else
    {
	m_pBwDetectionData[m_ulBwDetectionDataCount].
	    m_ulSize	    = ulSize;
	m_pBwDetectionData[m_ulBwDetectionDataCount].
	    m_ulTimeStamp	    = ulTimeStamp;
	m_pBwDetectionData[m_ulBwDetectionDataCount].
	    m_ulATime	    = ulArrivedTimeStamp;

	if(m_TransportType == TNG_TCP)
	{
	    if(((m_pBwDetectionData[m_ulBwDetectionDataCount].m_ulTimeStamp -
		m_pBwDetectionData[0].m_ulTimeStamp > 30000) &&
		m_ulBwDetectionDataCount > 300))
	    {
		m_bSlidingBwWindowReady = TRUE;
	    }
	}
	else if(m_TransportType == TNG_UDP)
	{
	    if (((((m_pBwDetectionData[m_ulBwDetectionDataCount].m_ulTimeStamp -
	        m_pBwDetectionData[0].m_ulTimeStamp) > 3000) &&
	        (m_ulBwDetectionDataCount > 20)) ||
	       (m_pBwDetectionData[m_ulBwDetectionDataCount].m_ulATime -
	        m_pBwDetectionData[0].m_ulATime > 3000)))
	    // XXXSMP Re-examine this formula later.
	    {
	        m_bSlidingBwWindowReady = TRUE;
	    }
	}
	if(!m_bSlidingBwWindowReady)	
	{
	    m_ulBwDetectionDataCount++;

	    // do we not have a big enough buffer for calculating bandwidth?
	    if (m_ulBwDetectionDataCount >= m_ulBwDetectionDataLen)
	    {
		// if we already have at least 1024 data points or we are 
		// out of memory then we'll just have to make do
		UINT32 ulNewSize = m_ulBwDetectionDataLen * 2;
		UINT32 ulMaxSize = HX_MIN(BW_DETECTION_DATA_POINTS, ulNewSize);
		if (m_ulBwDetectionDataLen >= BW_DETECTION_DATA_POINTS ||
		    !AllocBWDetectionData(ulMaxSize))
		{
		    m_ulBwDetectionDataCount--;
		    m_bSlidingBwWindowReady = TRUE;
		}
	    }
	}
    }

    return HXR_OK;
}

UINT32
ASMSourceInfo::GetBandwidthSince(UINT32 ulTime,
				     UINT32 ulNow)
{
    if (m_bIsDone)
    {
	return HXR_OK;
    }

    /*
     *  If we have not seen enough yet to get an idea.
     */
    if(!m_bSlidingBwWindowReady)
    {
	//XXXPM Hack this in to make TCP do small window calcs
	if(m_pBwDetectionData && m_TransportType == TNG_TCP &&
	(m_pBwDetectionData[m_ulBwDetectionDataCount].m_ulTimeStamp -
	m_pBwDetectionData[0].m_ulTimeStamp > 1000))
	{
	    /* XXXPM, Disgusting */
	    goto tcphackedin;
	}
	return 0;
    }
tcphackedin:

    UINT32 i;
    UINT32 sane = m_ulSlidingWindowLocation;
    UINT32 ulCount = 0;

    if(m_ulSlidingWindowLocation == 0)
    {
	i = m_ulBwDetectionDataCount - 1;
    }
    else
    {
	i = m_ulSlidingWindowLocation - 1;
    }

    UINT32 ulLastTime = ulNow;
    /*
     * add up the data size for all packets since ulTime
     */
    while(m_pBwDetectionData[i].m_ulATime > ulTime)
    {	
	ulCount += m_pBwDetectionData[i].m_ulSize;
	/*
	 * remember the time of this packet so that we can calculate duration
	 */
	ulLastTime = m_pBwDetectionData[i].m_ulATime; 
	if(i == 0)
	{
	    i = m_ulBwDetectionDataCount - 1;
	}
	else
	{
	    i--;
	}
	if(i == sane)
	{
	    //XXXPM we have travelled all the way around the window
	    //and still don't have enough.
	    break;
	}
    }
    if (ulNow == ulLastTime)
    {
	return 0;
    }
    return (UINT32)((ulCount * 8) / ((ulNow - ulLastTime) / 1000.0));
}


STDMETHODIMP
ASMSourceInfo::ReportUpshiftInfo(UINT32 ulTimeStamp,
				     UINT32 ulSize)
{
    // In low heap mode, skip this because it causes the server to alter the
    // transmission rate. NOTE: technically this is the similar change in
    // ReportLatency should disable surestream stream switching, but in
    // testing this is not born out.
#if defined(HELIX_CONFIG_LOW_HEAP_STREAMING)
    return HXR_OK;
#endif
    if (m_bIsDone)
    {
	return HXR_OK;
    }

    if (m_TransportType == TNG_TCP)
	return HXR_OK;

    m_pHXASM->UpShiftInfo(ulTimeStamp, ulSize);

    return HXR_OK;
}

void
HXSM::UpShiftInfo(UINT32 ulTimeInMicroSec, UINT32 ulSize)
{
    if (ulTimeInMicroSec)
    {
        INT64 llBW = ((INT64)ulSize * 8000000) / ulTimeInMicroSec;
        UINT32 uBW = INT64_TO_UINT32(llBW);
        m_upshiftBW.AddSample(uBW);

        HXLOGL3(HXLOG_TRAN,
                "HXSM::UpShiftInfo : gap %u size %u bw %u", 
                ulTimeInMicroSec, ulSize, uBW);

        if (m_upshiftBW.Full())
        {
            m_ulUpShiftBandwidthAvail = m_upshiftBW.Median();
        }
    }

    RecalcAccel();
}


HX_RESULT
ASMSourceInfo::ReportLatency(UINT32 ulServerTime,
			     UINT32 ulClientTime)
{
    // In low heap mode, skip this because it causes the server to alter the
    // transmission rate. NOTE: technically this is the similar change in
    // ReportUpShiftInfo should disable surestream stream switching, but in
    // testing this is not born out.
#if defined(HELIX_CONFIG_LOW_HEAP_STREAMING)
    return HXR_OK;
#endif
    if (m_bIsDone)
    {
	return HXR_OK;
    }

    INT32 lBackedUp;

    lBackedUp = CalcBackup(ulServerTime, ulClientTime);
    INT32 lDetectedBandwidth;

    if(m_TransportType == TNG_UDP)
    {
	UINT32 ulTemp = MAX(800, 4000 - m_lOuterThreshold);
	UINT32 ulStartWindow = ulClientTime - ulTemp;

	lDetectedBandwidth = GetBandwidthSince(ulStartWindow, ulClientTime);
    }
    else
    {
	lDetectedBandwidth = GetBandwidthSince(ulClientTime -
				TCP_BANDWIDTH_WINDOW, ulClientTime);
    }

    /*
     *  The THRESHOLD window is from THRESHOLD to m_lOuterThreshold.
     *  Once we go over threshold, we recalc the bandwidth and will do
     *  something to slow things down in m_pHXASM->Recalc.  We don't
     *  want to recalc the bandwidth again until we catch up to THRESHOLD
     *  or get slower again.  This is so that when we send one LimitBandwidth
     *  message we don't send another just when we are catching up (because
     *  sometimes during this time we can receive less and less data.)
     */

    //XXXgfw Don't allow Decel if this is a load test.....
    if ((lBackedUp >= (INT32)THRESHOLD) && (m_bSourceDecelAllowed))
    {
	/*
	 *  If we are backed up over our threshold window, then resize the
	 *  window and recalc our bandwidth.
	 */
	if ((lBackedUp > m_lOuterThreshold) &&
	    ((lDetectedBandwidth < (INT32)m_ulIncomingBandwidth) || 
		(!m_ulIncomingBandwidth)))
	{
	    m_ulIncomingBandwidth = lDetectedBandwidth;
	    /* Signal Recalc to adjust the highest available bandwidth */
	    m_pHXASM->m_bPipeFull = TRUE;
	    m_lOuterThreshold = lBackedUp + 500;
	}
	/*
	 * This can either grow or shrink the threshold window.
	 */
	m_bBehind = TRUE;
	m_bSlightlyBehind = TRUE;
    }
    else 
    {
	m_lOuterThreshold = THRESHOLD;
	m_bBehind = FALSE;

	UINT ulNew = 0;

	//XXXgfw Don't allow Decel if this is a load test.....
	if ((lBackedUp > 400) && (m_bSourceDecelAllowed))
	    m_bSlightlyBehind = TRUE;
	else
	{
	    m_bSlightlyBehind = FALSE;
	    if (m_TransportType == TNG_TCP)
	    {
		HXLOGL2(HXLOG_TRAN,
		    "(%p)TCP Shift up = %d", m_pSource,
		    m_pHXASM->m_ulUpShiftBandwidthAvail);
		m_pHXASM->m_ulUpShiftBandwidthAvail = MAX(
		    m_pHXASM->m_ulUpShiftBandwidthAvail,
		    (UINT32)(lDetectedBandwidth * 1.5));
	    }
	}

	if (m_bInvalidUpReport)
	{
	    m_bInvalidUpReport = FALSE;
	    m_ulLastReportTime = ulClientTime;
	    return HXR_OK;
	}

	UINT32 ulTemp = MAX(m_ulRateBeforeDeAccel, m_ulLastSetDelivery);
	if ((lDetectedBandwidth > (INT32)ulTemp) ||
	    (!m_ulIncomingBandwidth))
	    ulNew = ulTemp + 1;
	else
	    ulNew = lDetectedBandwidth;

	if (ulNew > m_ulIncomingBandwidth)
	{
	    m_ulIncomingBandwidth = ulNew;
	}
    }
    m_ulLastReportTime = ulClientTime;
    m_pHXASM->m_ulNumReportsSinceUpShift++;

    if (m_bPendingChill)
    {
	m_bPendingChill = FALSE;
	m_pHXASM->m_State = HXSM::CHILL_BUFFERING;
	m_pHXASM->RecalcAccel();

	IHXThinnableSource* pThin = NULL;

	if ((HXR_OK == m_pSource->
	    QueryInterface(IID_IHXThinnableSource, (void **)&pThin)))
	{
	    HX_ASSERT(m_ulSubscribedBw > 0);

	    m_ulRateBeforeDeAccel = m_ulLastSetDelivery;

	    UINT32 ulSet;

	    if (m_ulIncomingBandwidth > m_ulMaxSubscribedBw)
	    {
		ulSet = m_ulMaxSubscribedBw;
	    }
	    else
	    {
		ulSet = (UINT32)(m_ulSubscribedBw * 0.75);
	    }

	    HXLOGL2(HXLOG_TRAN, 
		"(%p)Acceleration Buffer Full: NewTransmissionRate=%d %p", m_pSource,
		    ulSet, this);

	    pThin->SetDeliveryBandwidth((m_ulLastSetDelivery = ulSet), 0);
	}
	HX_RELEASE(pThin);
    }
    else
	m_pHXASM->RecalcAccel();

    return HXR_OK;
}

INT32
ASMSourceInfo::CalcBackup(UINT32 ulServerTime, UINT32 ulClientTime)
{

    INT32 lNewDiff = ulClientTime - ulServerTime;

    /* 
     *  If this is the first time we will have no idea of the base
     *  time diff.
     */
    if(!m_lTimeDiffBase)
    {		
	m_lTimeDiffBase = lNewDiff;
	return 0;
    }

    /*
     * If our difference got shorter than before then this is the new base
     * and our backup is 0
     */
    if(lNewDiff < m_lTimeDiffBase)
    {
	m_lTimeDiffBase = lNewDiff;
	return 0;
    }

    if (m_lLastBehindTime)
    {
	if (((INT32)m_ulBytesBehind +
	    ((lNewDiff - m_lTimeDiffBase) - m_lLastBehindTime) * 
	    (INT32)m_ulLastSetDelivery / 8000) > 0)
	{
	    m_ulBytesBehind +=
	    ((lNewDiff - m_lTimeDiffBase) - m_lLastBehindTime) * 
	    (INT32)m_ulLastSetDelivery / 8000;
	}
	else
	{
	    m_ulBytesBehind = 0;
	}
    }
    m_lLastBehindTime = lNewDiff - m_lTimeDiffBase;

    HXLOGL3(HXLOG_TRAN, 
	    "(%p)Terminal Buffer Report: Behind by %dms (%d bytes)", m_pSource,
	    lNewDiff - m_lTimeDiffBase, m_ulBytesBehind);

    return lNewDiff - m_lTimeDiffBase;
}

STDMETHODIMP
ASMSourceInfo::SetCongestionFactor(UINT32 ulFactor)
{
    if (m_bIsDone)
    {
	return HXR_OK;
    }

    return HXR_OK;
}


void    
ASMSourceInfo::ChangeAccelerationStatus(HXBOOL	   bMayBeAccelerated,
					HXBOOL	   bUseAccelerationFactor,
					UINT32	   ulAccelerationFactor)
{
    if (m_bMayBeAccelerated == FALSE && bMayBeAccelerated == FALSE)
    {
	HXLOGL2(HXLOG_TRAN, 
		"(%p)Acceleration Buffer Way Full: Factor=%d", m_pSource, ulAccelerationFactor);
	HX_ASSERT(bUseAccelerationFactor);

	UINT32 ulNewRate = m_ulSubscribedBw * ulAccelerationFactor / 100;

	IHXThinnableSource* pThin = NULL;

	if (HXR_OK == m_pSource->
	    QueryInterface(IID_IHXThinnableSource, (void **)&pThin))
	{
	    pThin->SetDeliveryBandwidth((m_ulLastSetDelivery = ulNewRate), 0);
	}
	HX_RELEASE(pThin);

	return;
    }

    m_bMayBeAccelerated = bMayBeAccelerated;

    if (!m_bMayBeAccelerated && (m_ulLastSetDelivery > m_ulSubscribedBw))
    {
	m_bPendingChill = TRUE;
    }
    else if (m_bMayBeAccelerated && m_ulRateBeforeDeAccel)
    {
	IHXThinnableSource* pThin = NULL;

	if (HXR_OK == m_pSource->
	    QueryInterface(IID_IHXThinnableSource, (void **)&pThin))
	{
	    /* Only attempt conservative restart on LBR sources */
	    if (m_ulRateBeforeDeAccel < 150000)
	    {
		/* Can we be conservative when switching back up? */
		if ((m_ulRateBeforeDeAccel * 0.50) > m_ulSubscribedBw)
		{
		    /* Yes, be really conservative */
		    m_ulRateBeforeDeAccel = (UINT32)
		        (m_ulRateBeforeDeAccel * 0.70);
		}
		else if ((m_ulRateBeforeDeAccel * 0.70) > m_ulSubscribedBw)
		{
		    /* Be somewhat conservative */
		    m_ulRateBeforeDeAccel = (UINT32)
		        (m_ulRateBeforeDeAccel * 0.85);
		}
	    }

	    HXLOGL2(HXLOG_TRAN, 
		"(%p)Acceleration Buffer at 50 percent: NewTransmissionRate=%d", m_pSource, m_ulRateBeforeDeAccel);

	    pThin->SetDeliveryBandwidth(
		(m_ulLastSetDelivery = (UINT32) (m_ulRateBeforeDeAccel)), 0);
	    m_ulRateBeforeDeAccel = 0;
	}
	HX_RELEASE(pThin);
    }
}

HX_RESULT ASMSourceInfo::AddStream(UINT16 i,
                                   ASMStreamInfo* pInfo,
                                   UINT32 ulLowestBwBeforeTS)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pInfo && m_pStreams)
    {
        m_pStreams[i] = pInfo;
        m_ulLowestBandwidthBeforeTimeStamp += ulLowestBwBeforeTS;
    }
    
    return res;
}

STDMETHODIMP
ASMSourceInfo::SetTransportType(TRANSPORT_TYPE type)
{
    if (m_bIsDone)
    {
	return HXR_OK;
    }

    m_TransportType = type;

    if (m_TransportType == TNG_TCP)
    {
	THRESHOLD = 5000;
	m_lOuterThreshold = 5000;
    }

    return HXR_OK;
}

void ASMSourceInfo::SetupLoadTestVars(IUnknown* pUnknown)
{
#ifdef HELIX_FEATURE_LOADTEST

    //	       LOAD TEST ONLY OPTIONS
    // Read in the preferences for enabling/disabling Accel and 
    // Decel for load tests.
    //

    IHXPreferences* pPreferences = NULL;
    if (HXR_OK == pUnknown->QueryInterface(IID_IHXPreferences, 
                                           (void**)&pPreferences))
    {
        HXBOOL bLoadTest = FALSE;
        HXBOOL bDoAccel = TRUE;
        HXBOOL bDoDecel = TRUE;
        HXBOOL bDisableBothAccelDecel = FALSE;
        
        ReadPrefBOOL(pPreferences, "LoadTest", bLoadTest);
        ReadPrefBOOL(pPreferences, "DoAccel", bDoAccel);
        ReadPrefBOOL(pPreferences, "DoDecel", bDoDecel);
        ReadPrefBOOL(pPreferences, "DisableBothAccelDecel", bDisableBothAccelDecel);
    
        // DisableBothAccelDecel override all other preferences.
        // If it is true, set both DoAccel and DoDecel to false.
        // If DisableBothAccelDecel is FALSE do nothing at all.
        if (TRUE == bDisableBothAccelDecel)
        {
            bDoAccel = FALSE;
            bDoDecel = FALSE;
        }
    
        /* This is ONLY used for load testing */
        if (bLoadTest)
        {
            const char* pSourceURL	       = NULL;
            CHXURL*	    pURL	       = NULL;
            UINT32	    ulTemp	       = 0;
            IHXValues*  pOptions	       = NULL;
            
            pSourceURL = m_pSource->GetURL();
            HX_ASSERT(pSourceURL);
            pURL = new CHXURL(pSourceURL, m_pContext);
            HX_ASSERT(pURL);
            pOptions = pURL->GetOptions();
            
            // Check for the DoAccel and DoDecel URL options. If they are
            // there set the sources properties to reflect them.  Also,
            // each option is overriden, in the 'off' state by the global
            // preferences DoAccel and DoDecel. Remember, both the
            // preferences values can be made false by setting the global
            // preference DisableBothAccelDecel to TRUE. Truth Table:
            //
            // Resulting ASM functionality = Option ^ Preference
            //
            // or,
            //
            // bSourceAccelAllowed = bSourceAccelAllowed^bDoAccel
            // bSourceDecelAllowed = bSourceDecelAllowed^bDoDecel
            //
            //                                           Resulting
            //   Preferences     URL Option String   ASM Functionality
            // DoAccel  DoDecel   DoAccel  DoDecel    Accel   Decel
            //   0        0         0        0          0       0
            //   0        0         0        1          0       0
            //   0        0         1        0          0       0
            //   0        0         1        1          0       0
            //   ...      ...       ...      ...        ...     ...
            //   0        1         0        0          0       0
            //   1        1         1        1          1       1
            // You get the idea. :-)
            
            //First, get any option strings. They default to FALSE in
            //the constructors.
            if( NULL != pOptions )
            {
                // Only override if global preference is true
                if( bDoAccel &&
                    pOptions->GetPropertyULONG32("DoAccel", ulTemp) == HXR_OK)
                {
                    m_bSourceAccelAllowed = (ulTemp == 1);
                }
                if( bDoDecel &&
                    pOptions->GetPropertyULONG32("DoDecel", ulTemp) == HXR_OK)
                {
                    m_bSourceDecelAllowed = (ulTemp == 1);
                }
            }//NULL != pOptions


            HX_RELEASE(pOptions);
            HX_DELETE(pURL);
            

        }//bLoadTest
    }

    //Report status of load test vars.
//      HXLOGL4( HXLOG_ASMX, "LoadTest %d", bLoadTest);
//      HXLOGL4( HXLOG_ASMX, "    DoAccel %u",  (UINT16)bDoAccel);
//      HXLOGL4( HXLOG_ASMX, "    DoDecel %u", (UINT16)bDoDecel);
//      HXLOGL4( HXLOG_ASMX, "    DisableBothAccelDecel %d",
//  				bDisableBothAccelDecel);
//      HXLOGL4( HXLOG_ASMX, "    m_bSourceAccelAllowed %d",
//  		    m_bSourceAccelAllowed);
//      HXLOGL4( HXLOG_ASMX, "    m_bSourceDecelAllowed %d",
//  		m_bSourceDecelAllowed);
    HX_RELEASE(pPreferences);

#endif /* HELIX_FEATURE_LOADTEST */
}

HX_RESULT ASMSourceInfo::GetStreamBW(IHXValues* pProps, 
                                     UINT32 uStream,
                                     REF(UINT32) ulBw)
{
    HX_RESULT res = HXR_FAILED;
    
    if (pProps)
    {
        IHXBuffer* pBw = NULL;

        // Construct Stream%ldBandwidth
        // Don't assume that streamid == index in pASMSourceInfo->m_pStreams[j]
        CHXString temp("Stream");
        temp.AppendULONG(m_pStreams[uStream]->m_ulStreamNumber);
        temp += "Bandwidth";

        /*
         * if this tripps either there's a bug in here or
         * the content is messed up
         *
         */ 
        if (HXR_OK == pProps->GetPropertyCString(temp, pBw))
        {
            ulBw = (UINT32)(atoi((char*)pBw->GetBuffer()));

            res = HXR_OK;
        }

        HX_RELEASE(pBw);
    }

    return res;
}
void ASMSourceInfo::RegisterSourcesDone()
{
    if (m_pSource && m_pSource->m_bFastStart)
    {
        m_ulLastSetDelivery = 0;
    }
}

void ASMSourceInfo::ClearSubscribedBw()
{
    m_ulSubscribedBw = 0;
    m_ulMaxSubscribedBw = 0;
}

void ASMSourceInfo::AddStreamSubBw(UINT32 ulCurBw, 
                                   UINT32 ulMaxBw)
{
    m_ulSubscribedBw += ulCurBw;

    HX_ASSERT(m_ulSubscribedBw < 0x7fffffff);

    m_ulMaxSubscribedBw += ulMaxBw;
}

void ASMSourceInfo::ChangeBW(UINT32 newBW)
{
    HXBOOL bDownShift = FALSE;

    if (m_pMasterRuleBook)
    {
        UINT16 usRuleCount = m_pMasterRuleBook->GetNumRules();
        HXBOOL* pCurrentSubInfo = new HXBOOL[usRuleCount];
            
        HXSMUpdateSubscriptionVars(m_pContext, m_pSubscriptionVariables, 
                                   newBW, FALSE, 0);

        // Figure out what rules we need to subscribe to
        HX_RESULT lResult =
            m_pMasterRuleBook->GetSubscription(pCurrentSubInfo, 
                                               m_pSubscriptionVariables);
        HX_ASSERT(lResult == HXR_OK);
        for (UINT16 idxRule = 0; idxRule < usRuleCount; idxRule++)
        {	
            if (pCurrentSubInfo[idxRule])
            {
                // We should subscribe to this rule
                IHXValues* pProps = 0;
                UINT32 ulNumStreamsForThisSource = GetStreamCount();

                // Get the properties for this rule
                m_pMasterRuleBook->GetProperties(idxRule, pProps);
                for (UINT32 j = 0; j < ulNumStreamsForThisSource; j++)
                {		
                    UINT32 newMaxBW;

                    // Get the bandwidth allocated for this stream
                    if (HXR_OK == GetStreamBW(pProps, j, newMaxBW))
                    {
                        // Tell the stream to change to the new bandwidth
                        HXBOOL bTryToUpShift = m_bTryToUpShift;
                        HXBOOL bFoundMax = 
                            m_pStreams[j]->ChangeBW(newMaxBW,
                                                    FALSE,
                                                    bDownShift,
                                                    bTryToUpShift);

                        m_bTryToUpShift = bTryToUpShift;

                        HX_ASSERT(bFoundMax);
                        if (bDownShift || m_bTryToUpShift)
                            m_bAdjustBandwidth = TRUE;
                    } 
                } 
                HX_RELEASE(pProps);
            } 
        } 

        delete [] pCurrentSubInfo;
    }
    else 	    
    {
        //Assume a single stream source, or a fixedbw source .. 
        //for live, we have more than 1 streams ??
        HXBOOL bTryToUpShift = m_bTryToUpShift;
        HXBOOL bFoundMax = m_pStreams[0]->ChangeBW(newBW,
                                                 TRUE,
                                                 bDownShift,
                                                 bTryToUpShift);
        m_bTryToUpShift = bTryToUpShift;
        if (bDownShift || m_bTryToUpShift)
            m_bAdjustBandwidth = TRUE;
    }
}

void ASMSourceInfo::SetMasterRuleBookOffer()
{
    if (m_pMasterRuleBook)
    {
        UINT32 ulNumStreamsForThisSource = GetStreamCount();

        UINT16 usRuleCount = m_pMasterRuleBook->GetNumRules();
        HXBOOL* pCurrentSubInfo = new HXBOOL[usRuleCount];

        HXSMUpdateSubscriptionVars(m_pContext, 
				   m_pSubscriptionVariables, 
                                   m_ulMasterOffer,
                                   FALSE, 0);

        HX_RESULT lResult =
            m_pMasterRuleBook->GetSubscription(pCurrentSubInfo, 
                                               m_pSubscriptionVariables);

        HX_ASSERT(lResult == HXR_OK);

        for (UINT16 idxRule = 0; idxRule < usRuleCount; idxRule++)
        {
            if (pCurrentSubInfo[idxRule])
            {
                IHXValues* pProps = 0;
                // Set Distribution
                m_pMasterRuleBook->GetProperties(idxRule, pProps);
                
                for (UINT32 j = 0; j < ulNumStreamsForThisSource; j++)
                {
                    UINT32 ulBw = 0;

                    GetStreamBW(pProps, j, ulBw);

                    m_pStreams[j]->SetMasterRuleBookOffer(ulBw);
                }
                
                HX_RELEASE(pProps);
                break;
            }
        }
        
        delete [] pCurrentSubInfo;
    }
}

void ASMSourceInfo::RecalcInit()
{
    m_ulMasterOffer = 0;
    m_bTimeStampDelivery = FALSE;
}

void ASMSourceInfo::AddToMasterOffer(UINT32 ulOffer)
{
    m_ulMasterOffer += ulOffer;
}

void ASMSourceInfo::SetRuleGatherList(IHXAtomicRuleGather* pRuleGather)
{
    if (pRuleGather)
    {
        pRuleGather->RuleGather(&m_SubscriptionChanges);
    }
}

void ASMSourceInfo::FlushSubscriptions()
{
    if (!m_SubscriptionChanges.IsEmpty())
    {
        if (m_pStreams && m_pStreams[0])
        {
            m_pStreams[0]->FlushSubscriptions(
                &m_SubscriptionChanges);
        }
        
        CHXSimpleList::Iterator j;
        for (j = m_SubscriptionChanges.Begin();
             j != m_SubscriptionChanges.End(); ++j)
        {
            RTSPSubscription* pSub = (RTSPSubscription*)(*j);
            delete pSub;
        }
        m_SubscriptionChanges.RemoveAll();
    }
}

void 
ASMSourceInfo::GetSrcStats(REF(HXBOOL) bFastStart,
                           REF(UINT32) ulNumBehind,
                           REF(UINT32) ulNumSlightlyBehind,
                           REF(INT32) lBwUsage,
                           REF(HXBOOL) bAllZeroBw,
                           REF(INT32) lMaxNeededBW)
{
    if (m_ulLastSetDelivery == 0xffffffff)
    {
        m_ulLastSetDelivery = m_ulSubscribedBw;
    }

    // if ANY of the sources are fast start then we are in 
    // fast start mode. We may wish to re-visit this decision!
    if (IsFastStart())
    {
        bFastStart = TRUE;
    }
    
    if (m_bBehind)
    {
        ulNumBehind++;
    }

    if (m_bSlightlyBehind)
    {
        ulNumSlightlyBehind++;
    }

    UINT32 ulBw = GetBandwidth();
    lBwUsage += ulBw;
    if (!ulBw)
        lBwUsage += m_ulSubscribedBw;
    else
        bAllZeroBw = FALSE;

    lMaxNeededBW += m_ulMaxSubscribedBw;
}

void 
ASMSourceInfo::SetMaxAccelRatio(double maxAccelRatio)
{
    if (m_pSource)
    {
        m_pSource->SetMaxPossibleAccelRatio(maxAccelRatio);
    }
}

void ASMSourceInfo::LeaveFastStart(TurboPlayOffReason leftReason)
{
    if (m_pSource)
    {
        m_pSource->LeaveFastStart(leftReason);
    }
}

void 
ASMSourceInfo::DistributeAndSetBandwidth(const UINT32 ulAggregateUsed,
                                         const UINT32 ulTotalBwAvailable,
                                         const float fAccelerationFactor,
                                         const UINT32 ulOriginalHighestBandwidthAvail,
                                         const UINT32 ulResistanceBitRate,
                                         const HXBOOL bEnableSDB)
{
    IHXThinnableSource* pThin = 0;
    UINT32 ulSourceBandwidth = GetSubscribedBw();

    UINT32 ulNewValue = (UINT32)(
        ((float)ulSourceBandwidth /
         (float)ulAggregateUsed) *
        (float)(ulTotalBwAvailable));

    if ((HXR_OK == m_pSource->
         QueryInterface(IID_IHXThinnableSource, (void **)&pThin)))
    {
        if ((ulNewValue > (m_ulLastSetDelivery * 1.02)) ||
            (ulNewValue < (m_ulLastSetDelivery * 0.98)) ||
            (m_bAdjustBandwidth && IsLive()))
        {
            HXBOOL bFastStart = IsFastStart();

            // Apply various bandwidth caps.
            if (bFastStart)
            {
                UINT32 ulAccelCap = (UINT32) 
                    (m_ulMaxSubscribedBw * fAccelerationFactor);
                
                ulNewValue = MIN(ulNewValue, ulAccelCap);
                
                // if the server says to cap the value, we cap the value!
                if (m_pSource)
                {
                    UINT32 ulSourceCap = m_pSource->m_ulMaxBandwidth * 1000;
                    ulNewValue = MIN(ulNewValue, ulSourceCap);
                }

                // if the value is greater is than the pipe set it to a 
                // little less than the pipe!
                ulNewValue = MIN(ulNewValue, 
                                 ulOriginalHighestBandwidthAvail);
            }
            else
            {
                ulNewValue = ApplyNonFastStartLimits(ulNewValue);
            }
            m_bAdjustBandwidth = FALSE;

            ulNewValue = Apply3xUpshiftLimit(ulNewValue);

            if ((ulNewValue < ulSourceBandwidth) && (ulSourceBandwidth > 10))
            {
                m_pSource->EnterBufferedPlay();
            } 
            if (ulNewValue >= ulSourceBandwidth)
            {
                m_pSource->LeaveBufferedPlay();
            }

            if ((ulNewValue >= m_ulLastSetDelivery) &&
                (!m_bMayBeAccelerated) && 
                (!m_bTryToUpShift))
            {
                goto dont_actually_set_the_rate;
            }

            if (ulNewValue > ulResistanceBitRate && !bFastStart)
            {
                UINT32 ulActualResistanceBitRate = ulResistanceBitRate;

                if (ulActualResistanceBitRate < ulAggregateUsed)
                {
                    ulActualResistanceBitRate = (UINT32)
                        (ulAggregateUsed * 1.05);
                }

                if (ulNewValue > ulActualResistanceBitRate)
                {
                    ulNewValue = ulActualResistanceBitRate;
                }
            }
 
            if(m_bTryToUpShift)
            {
                m_bTryToUpShift=FALSE;

                //XXXRA why not check for ulNewValue < ((UINT32)(ulSourceBandwidth * 1.15)
                // before assignment.  
                ulNewValue = (UINT32)(ulSourceBandwidth * 1.15);
            }
		    
            m_ulLastSetDelivery = ulNewValue;

            UINT32 ulActualRate = AdjustForTCP(ulNewValue);

            HXLOGL2(HXLOG_TRAN, "(%p)Redist: Tranmission Rate to %d", m_pSource, ulActualRate);


            if (bEnableSDB)
            {
                pThin->SetDeliveryBandwidth(ulActualRate, 0);
            }

            if (bFastStart)
            {
                m_pSource->m_turboPlayStats.ulAcceleratedBW = ulActualRate;
            }
        }
    }
 dont_actually_set_the_rate:
    HX_RELEASE(pThin);
}

UINT32
ASMSourceInfo::ChangeBwBecauseOfCongestion(UINT32 ulNewValue,
                                           REF(HXBOOL) bLossBehind)
{
    UINT32 ulRet = 0;

    IHXThinnableSource* pThin = 0;
    UINT32 ulSourceBandwidth = GetSubscribedBw();

    if (m_bLossBehind &&
        (ulNewValue < ulSourceBandwidth))
    {
        HXLOGL2(HXLOG_TRAN, 
                  "(%p)No Loss Reduce: Will Force BP", 
                   m_pSource);

        ulNewValue = ulSourceBandwidth;
    }

        
    if (m_pSource && (HXR_OK == m_pSource->
         QueryInterface(IID_IHXThinnableSource, (void **)&pThin)))
    {
        if (ulNewValue < m_ulLastSetDelivery)
        {
            HXLOGL2(HXLOG_TRAN,"(%p)Congestion: Slow Tranmission Rate to %d %p", m_pSource,
                       ulNewValue, this);

            if (m_bLossBehind)
            {
                m_ulIncomingBandwidth = ulNewValue;
                bLossBehind = TRUE;
            }

            if (m_ulRateBeforeDeAccel)
            {
                m_ulRateBeforeDeAccel = ulNewValue;
                /*
                 * Reset the core's acceleration status. 
                 */

                IHXWatermarkBufferControl* pWMBufCtl = NULL;
			    
                m_pSource->
                    QueryInterface(IID_IHXWatermarkBufferControl,
                                   (void**)&pWMBufCtl);

                if (pWMBufCtl)
                {
                    pWMBufCtl->ClearChillState();
                    pWMBufCtl->Release();
                    pWMBufCtl = NULL;
                }
                m_bMayBeAccelerated = TRUE;
                m_bPendingChill = FALSE;
            }

            if ((ulNewValue < ulSourceBandwidth) && 
                (ulSourceBandwidth > 10))
            {
                m_pSource->EnterBufferedPlay();
            }

            m_ulLastSetDelivery = ulNewValue;
            ulRet += ulNewValue;

            UINT32 ulActualRate = 
                AdjustForTCP(ulNewValue);

            // In low heap mode, do not change the delivery bw.
            // NOTE: There is concern that this is not 
            // satisfactory as a truly long term solution since
            // bw rate control is regarded by some as critical
            // for limited resource platforms.

#if !defined(HELIX_CONFIG_LOW_HEAP_STREAMING)
            pThin->SetDeliveryBandwidth(ulActualRate, 0);
#endif
        }
        else
        {
            ulRet += m_ulLastSetDelivery;
        }
    }
    HX_RELEASE(pThin);

    return ulRet;
}
UINT32 
ASMSourceInfo::AdjustForTCP(UINT32 ulActualRate) const
{
    UINT32 ulRet = ulActualRate;

    /*
     * Always keep TCP traffic faster then needed
     * (but keep it quiet so the rest of the algorithm doesn't
     * find out :-)
     */
    if (m_TransportType == TNG_TCP)
    {
        ulRet = MAX(ulActualRate,
                    (UINT32)(GetSubscribedBw() * 1.10));
    }

    return ulRet;
}

UINT32 
ASMSourceInfo::ApplyNonFastStartLimits(const UINT32 ulNewValue) const
{
    UINT32 ulRet = ulNewValue;

    UINT32 ulAccelCap = m_ulMaxSubscribedBw * 4;

    ulRet = MIN(ulRet, ulAccelCap);

    /*
     * Live streams get capped at 107% of Max to prevent 
     * unneeded bandwidht modulation.
     */
    if (IsLive())
    {
        UINT32 ulLiveCap = (UINT32)
            (m_ulMaxSubscribedBw * 1.07);
        
        ulRet = MIN(ulRet, ulLiveCap);
    }

    return ulRet;
}

UINT32 
ASMSourceInfo::Apply3xUpshiftLimit(const UINT32 ulNewValue) const
{
    UINT32 ulRet = ulNewValue;

    if ((ulRet > (m_ulMaxSubscribedBw)) &&
        (ulRet > (GetSubscribedBw() * 3)) &&
        (m_ulLastSetDelivery > m_ulMaxSubscribedBw))
    {
        /*
         * If we are already accelerating 3x subscribed bandwidth
         * and we are about to upshift beyond the max possible
         * bandwidth, then let's stop and take a breather just
         * above the max subscription.  This prevents us from
         * buffering huge amounts of the low bw stream.
         */
        UINT32 ulTemp;
        ulTemp = (UINT32) (m_ulMaxSubscribedBw * 1.10);
        ulRet = MIN(ulTemp, ulRet);
    }

    return ulRet;
}

ASMStreamInfo::ASMStreamInfo(ASMSourceInfo* pASMSourceInfo,
                             IUnknown* pStream,
                             REF(UINT32) ulLowestBwBeforeTS) :
    m_bTimeStampDelivery(FALSE),
    m_pASMSourceInfo(pASMSourceInfo),
    m_pNegotiator(0),
    m_pBias(0),
    m_pRuleGather(0),
    m_ulFixedBandwidth(0),
    m_ulLastBandwidth(0),
    m_pThreshold(NULL),
    m_ulNumThresholds(0),
    m_ulThresholdPosition(0),
    m_ulResistanceToLower(0),
    m_ulOffer(0),
    m_ulMasterRuleBookSetOffer(0),
    m_ulMaxEffectiveThreshold(-1),
    m_ulStreamNumber(0)
{
    IHXStream* pHXStream = NULL;

    HX_VERIFY(HXR_OK == pStream->QueryInterface
              (IID_IHXStreamBandwidthNegotiator,
               (void **)&m_pNegotiator));
    HX_VERIFY(HXR_OK == pStream->QueryInterface
              (IID_IHXStreamBandwidthBias,
               (void **)&m_pBias));
    HX_VERIFY(HXR_OK == pStream->QueryInterface
              (IID_IHXAtomicRuleGather,
               (void **)&m_pRuleGather));

    pStream->QueryInterface(IID_IHXStream, (void**)&pHXStream);

    if (pHXStream)
    {
        m_ulStreamNumber = pHXStream->GetStreamNumber();
        HX_RELEASE(pHXStream);
    }

    m_pNegotiator->GetFixedBandwidth(m_ulFixedBandwidth);

    ulLowestBwBeforeTS = 0;
    if (m_ulFixedBandwidth != 0)
    {
        ulLowestBwBeforeTS = m_ulFixedBandwidth;
    }
    else
    {
        // XXXNH: 6/7/99
        // We make this threshold array once and create it to be as large
        // as we will ever possibly need.  Each subsequent call to
        // GetThresholdInfo() should never need more than GetNumThresholds
        // returns.  UNLESS the ASMStreamInfo's m_pNegotiator were to 
        // change, but I don't think that's a feature we support.
        
        UINT32 ulNumThresholds = m_pNegotiator->GetNumThresholds();
        m_pThreshold = new float[ulNumThresholds];
        m_pNegotiator->GetThresholdInfo((float*)m_pThreshold, 
                                        m_ulNumThresholds);
        m_ulMaxEffectiveThreshold = m_ulNumThresholds - 1;
        ulNumThresholds = m_ulNumThresholds;

        ulLowestBwBeforeTS = (UINT32) m_pThreshold[ulNumThresholds-1];
        for (UINT32 i = ulNumThresholds-1; i > 0 ; i--)
        {
            UINT32 ulCurBand = (UINT32)m_pThreshold[i];
            // used for timestamp rules
            if (ulCurBand == 0 || ulCurBand == 1)
            {
                break;
            }
            else 
            {
                ulLowestBwBeforeTS = ulCurBand;
            }
        }
    }

    HX_ASSERT(ulLowestBwBeforeTS != 0);
}

ASMStreamInfo::~ASMStreamInfo()
{
    if (m_pNegotiator)
    {
        m_pNegotiator->UnRegister();
    }

    HX_VECTOR_DELETE(m_pThreshold);
    HX_RELEASE(m_pNegotiator);
    HX_RELEASE(m_pBias);
    HX_RELEASE(m_pRuleGather);
}

UINT32 ASMStreamInfo::GetCurrentSubBw() const
{
    UINT32 uRet = m_ulFixedBandwidth;
        
    if (!m_ulFixedBandwidth && m_pThreshold)
    {
        uRet = (UINT32)
            m_pThreshold[m_ulThresholdPosition];
    }

    return uRet;
}

UINT32 ASMStreamInfo::GetMaxSubscribeBw() const
{
    UINT32 uRet = m_ulFixedBandwidth;

    if (!m_ulFixedBandwidth && m_pThreshold)
    {
        uRet =  (UINT32)
            m_pThreshold[m_ulMaxEffectiveThreshold];
    }

    return uRet;
}

void ASMStreamInfo::UpdateSourceSubBw(UINT32 ulCurBw, 
                                      UINT32 ulMaxBw)
{
    if (m_pASMSourceInfo)
    {
        m_pASMSourceInfo->AddStreamSubBw(ulCurBw, ulMaxBw);
    }
}

HXBOOL ASMStreamInfo::ChangeBW(UINT32 newMaxBW, 
                             HXBOOL bUpdateMaxEffeciveThresh,
                             REF(HXBOOL) bDownShift, 
                             REF(HXBOOL) bTryToUpshift)
{
    HXBOOL bFoundMax = FALSE;

    for(UINT32 cnt =0; cnt < m_ulNumThresholds; cnt++)
    {
        if (newMaxBW == m_pThreshold[cnt])
        {
            bFoundMax = TRUE;
            m_ulMaxEffectiveThreshold = cnt;

            if (m_ulThresholdPosition > cnt) 
            {
                // We are downshifting!!
                bDownShift = TRUE;
                m_ulThresholdPosition = cnt;
            }
            else if (m_ulThresholdPosition < cnt)  
            {
                // We will try to upshift
                bTryToUpshift = TRUE;
            }

            if (bUpdateMaxEffeciveThresh)
            {
                m_ulMaxEffectiveThreshold = cnt;
            }

            break;
        }
    }

    return bFoundMax;
}

void ASMStreamInfo::SetMasterRuleBookOffer(UINT32 ulOffer)
{
    m_ulMasterRuleBookSetOffer = ulOffer;
}

HX_RESULT ASMStreamInfo::GetBiasFactor(REF(INT32)lBias)
{
    HX_RESULT res = HXR_FAILED;

    if (m_pBias)
    {
        res = m_pBias->GetBiasFactor(lBias);
    }

    return res;
}

UINT32 ASMStreamInfo::ComputeBiasedOffer(UINT32 uTotalBw,
                                         UINT32 uStreamCount,
                                         float fBiasMean)
{
    // Offer each stream it's fair share of the 
    // bandwidth
    UINT32 uRet = 0;

    if (uStreamCount)
    {
        uRet = uTotalBw / uStreamCount;
    }

    // Compute bias deviation factor. It is 2% of the streams
    // fair share of the bandwidth.
    // I'm not sure how this magic factor was derived. 
    float biasFactor = (((float)uTotalBw / 100.0f) *
                        (2.0f / (float)uStreamCount));

    INT32 lBias;
    if (HXR_OK == GetBiasFactor(lBias))
    {
        float biasDev = ((float)lBias - fBiasMean);

        uRet += (UINT32)(biasDev * biasFactor);
    }

    return uRet;
}

UINT32 ASMStreamInfo::UpdateMasterOffer(UINT32 ulBiasedOffer)
{
    UINT32 uRet = 0;

    if (m_pASMSourceInfo &&
        m_pASMSourceInfo->HasMasterRuleBook())
    {
        uRet = ulBiasedOffer;
        
        m_pASMSourceInfo->AddToMasterOffer(uRet);
    }

    return uRet;
}

UINT32 ASMStreamInfo::DistributeBw(UINT32 ulBiasedOffer)
{
    UINT32 uRet = 0;

    if (m_ulFixedBandwidth != 0)
    {
        // This is a fixed bitrate stream.
        uRet = m_ulFixedBandwidth;
        m_ulResistanceToLower = 0xffffffff;
    }
    else if (m_pASMSourceInfo && 
             m_pASMSourceInfo->IsPerfectPlay())
    {
        /*
         * If we are in perfect play mode, just select the 
         * highest bandwidth rule and don't negotiate any 
         * further.
         */
        
        UINT32 i = m_ulMaxEffectiveThreshold;

        if (m_pThreshold)
        {
            // Set the offer to the highest threshold value
            m_ulOffer = (UINT32)m_pThreshold[i];
        }

        m_ulResistanceToLower = 0xffffffff;
        m_ulThresholdPosition = i;

        uRet = m_ulOffer;
    }
    else
    {
        UINT32 ulOffer = 0;
        
        if (m_ulMasterRuleBookSetOffer)
        {
            // We have a master rulebook offer so 
            // use that.
            ulOffer = m_ulMasterRuleBookSetOffer - 1;
        }
        else
        {
            // We don't have a master rulebook offer
            // so we should just use the default
            ulOffer = ulBiasedOffer;
        }
        
        HX_ASSERT(m_ulMaxEffectiveThreshold >= 0);
        if (m_ulMaxEffectiveThreshold == 0)
        {
            // The max threshold is 0 so
            // we don't have to search for
            // the appropriate bitrate
            HX_ASSERT(m_pThreshold[0] == 0);
            
            uRet = (UINT32)m_pThreshold[0];
            m_ulResistanceToLower = 0xffffffff;
        }
        else
        {
            // Find the lowest bitrate threshold that is >=
            // to the offer and is <= to the max effective
            // bandwidth threshold
            for (UINT32 i = 1; i <= m_ulMaxEffectiveThreshold; i++)
            {
                if ((ulOffer <= m_pThreshold[i]) ||
                    (i == (m_ulMaxEffectiveThreshold)))
                {
                    uRet = (UINT32)m_pThreshold[i];

                    m_ulOffer = ulOffer;
                    m_ulThresholdPosition = i;

                    RecalcResistance();
                        
                    break;
                }
            }
        }
    }

    return uRet;
}

void ASMStreamInfo::RecalcResistance()
{
    if (m_ulThresholdPosition == 1)
    {
        // We are at the lowest bitrate so make
        // resistance to lowering the bitrate
        // very high
        m_ulResistanceToLower = 0xffffffff;
    }
    else
    {
        // Set the resistance to lowering proportional
        // to how far the offer is from the next lower
        // bandwidth threshold.
        UINT32 i = m_ulThresholdPosition;
        m_ulResistanceToLower = 
            (m_ulOffer -(UINT32)m_pThreshold[i - 1]) * m_ulOffer;
    }
}

UINT32 ASMStreamInfo::DropToLowerBw()
{
    UINT32 uRet = 0;

    if (m_pThreshold)
    {
        uRet = (UINT32)m_pThreshold[m_ulThresholdPosition];
    
        m_ulThresholdPosition--;
    
        uRet -= (UINT32)m_pThreshold[m_ulThresholdPosition];
        
        RecalcResistance();
    }
    
    return uRet;
}

HXBOOL ASMStreamInfo::SetSubscriptions(UINT32 ulLeftOverForDropByN)
{
    HXBOOL bRet = FALSE;

    // Get the selected bandwidth. Initially assume
    // this is a fixed bitrate stream
    UINT32 ulBw = m_ulFixedBandwidth;
    
    if (!m_ulFixedBandwidth)
    {
        // This not a fixed bitrate stream so we
        // should use the selected threshold
        ulBw = (UINT32)m_pThreshold[m_ulThresholdPosition];
    }

    UINT32 ulBwOffered = ulBw;

    if (ulBw == 1)
    {
        // Hack Alert for DropByN. XXXSMP
        ulBwOffered = ulBw = ulLeftOverForDropByN;
    }

    if ((ulBw != m_ulLastBandwidth) &&
        (!m_ulFixedBandwidth))
    {
        // If the bandwidth changed and this
        // isn't a fixed bitrate stream, then
        // signal that RecalcAccel() should be called
        bRet = TRUE;
    }
    
    if (m_pASMSourceInfo)
    {
        // Provides m_pRuleGather with m_pASMSourceInfo's
        // rule gathering list
        m_pASMSourceInfo->SetRuleGatherList(m_pRuleGather);
    }

    // Update m_ulLastBandwidth with the new bandwidth
    m_ulLastBandwidth = ulBw;

    HX_ASSERT(ulBw == ulBwOffered);

    //update the HXASMStream with our new bandwidth
    if (m_pNegotiator)
    {
        m_pNegotiator->SetBandwidthUsage(ulBw,
                                         m_bTimeStampDelivery);
    }

    //update the source's knowledge of tsd
    if(m_bTimeStampDelivery && m_pASMSourceInfo)
    {
	m_pASMSourceInfo->SetTSDelivery(TRUE);
    }

    //if the stream is behind, tell the server to chill
    if(m_pASMSourceInfo && m_pASMSourceInfo->IsBehind())
    {
	m_pNegotiator->HandleSlowSource(ulBwOffered);
    }

    // Clear the rule gather list pointer that was set
    // by the m_pASMSourceInfo->SetRuleGatherList()
    // call
    m_pRuleGather->RuleGather(0);

    return bRet;
}

void ASMStreamInfo::FlushSubscriptions(CHXSimpleList* pSubscriptionChanges)
{
    if (m_pRuleGather)
    {
        m_pRuleGather->RuleFlush(pSubscriptionChanges);
    }
}
