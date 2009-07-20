/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxconnbwinfo.cpp,v 1.18 2006/08/01 22:02:32 gwright Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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
#include "hxconnbwinfo.h"
#include "hxassert.h"
#include "pckunpck.h"
#include "hxtlogutil.h"
#include "hxprefutil.h"
#include "hxnetifsinkhlpr.h"
#include "safestring.h"

// There is nothing mystical about these values. They just provide
// a decent amount of smoothing
const UINT32 DynamicABDMovMedSize   = 5;
const UINT32 ConnectionBWMovAvgSize = 3;

class HXConnBWInfoABDSink : public IHXAutoBWDetectionAdviseSink
{
public:
    HXConnBWInfoABDSink(HXConnectionBWInfo* pOwner);
    ~HXConnBWInfoABDSink();

    HX_RESULT Init(IHXAutoBWDetection* pABD);
    HX_RESULT Close();

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface) (THIS_
                               REFIID riid,
                               void** ppvObj);

    STDMETHOD_(ULONG32,AddRef) (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *  IHXAutoBWDetectionAdviseSink methods
     */
    STDMETHOD(AutoBWDetectionDone) (THIS_
                                    HX_RESULT  status,
                                    UINT32     ulBW);
private:
    ULONG32 m_lRefCount;
    HXConnectionBWInfo* m_pOwner;
    IHXAutoBWDetection* m_pABD;
};

HXConnBWInfoABDSink::HXConnBWInfoABDSink(HXConnectionBWInfo* pOwner) :
    m_lRefCount(0),
    m_pOwner(pOwner),
    m_pABD(NULL)
{
    HXLOGL3(HXLOG_CBWI, "%p CBIAS::CBIAS", this);
}

HXConnBWInfoABDSink::~HXConnBWInfoABDSink()
{
    HXLOGL3(HXLOG_CBWI, "%p CBIAS::~CBIAS", this);

    // Somehow this object was destroyed before
    // calling Close(). The reference counts 
    // must be messed up somewhere
    HX_ASSERT(!m_pABD);

    HX_RELEASE(m_pABD);

    m_pOwner = NULL;
}

HX_RESULT HXConnBWInfoABDSink::Init(IHXAutoBWDetection* pABD)
{
    HXLOGL3(HXLOG_CBWI, "%p CBIAS::Init : %p", this, pABD);

    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pABD)
    {
        if (m_pABD)
        {
            Close();
        }

        res = pABD->AddAutoBWDetectionSink(this);

        if (HXR_OK == res)
        {
            m_pABD = pABD;
            m_pABD->AddRef();
        }
    }

    return res;
}

HX_RESULT HXConnBWInfoABDSink::Close()
{
    if (m_pABD)
    {
        HXLOGL3(HXLOG_CBWI, "%p CBIAS::Close", this);
        m_pABD->RemoveAutoBWDetectionSink(this);

        HX_RELEASE(m_pABD);
    }
    
    m_pOwner = NULL;

    return HXR_OK;  
}

/*
 *  IUnknown methods
 */
STDMETHODIMP
HXConnBWInfoABDSink::QueryInterface(THIS_ REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXAutoBWDetectionAdviseSink), (IHXAutoBWDetectionAdviseSink*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXAutoBWDetectionAdviseSink*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXConnBWInfoABDSink::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
    HXConnBWInfoABDSink::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *      IHXAutoBWDetectionAdviseSink methods
 */
STDMETHODIMP
HXConnBWInfoABDSink::AutoBWDetectionDone(THIS_
                                         HX_RESULT  status,
                                         UINT32     ulBW)
{
    HXLOGL3(HXLOG_CBWI, "%p CBIAS::ABDDone : %08x %u", 
            this, status, ulBW);

    if (m_pABD && m_pOwner)
    {
        m_pOwner->AutoBWDetectDone(m_pABD, status, ulBW  * 1000);
    }

    return HXR_OK;
}

BEGIN_INTERFACE_LIST_NOCREATE(HXConnectionBWInfo)
    INTERFACE_LIST_ENTRY(IID_IHXConnectionBWInfo, IHXConnectionBWInfo)
    INTERFACE_LIST_ENTRY(IID_IHXStatistics, IHXStatistics)
    /* NOTE: IID_IHXAutoBWCalibrationAdviseSink is intentionally left out
     *       because we don't want it exposed to other objects
     */
END_INTERFACE_LIST

HXConnectionBWInfo::HXConnectionBWInfo() :
    m_pContext(NULL),
    m_pPrefTranMgr(NULL),
    m_pABDCal(NULL),
    m_pBWManager(NULL),
    m_uConnectionBW(0),
    m_bIsDetectedBW(FALSE),
    m_uCachedBwPref(0),
    m_bNeedRecalc(FALSE),
    m_pEstimatedBWStats(NULL),
    m_abdCalState(acsInit),
    m_uABDCalibrationBW(0),
    m_pMutex(NULL),
    m_sinkDispatchItr(0),
    m_bGotDynamicABDSample(FALSE)
{
    HXLOGL3(HXLOG_CBWI, "%p CBI::CBI", this);
}

HXConnectionBWInfo::~HXConnectionBWInfo()
{
    HXLOGL3(HXLOG_CBWI, "%p CBI::~CBI", this);

    clearSinkMap();
    clearTransportMap();

    HX_DELETE(m_pEstimatedBWStats);

    if (m_pABDCal)
    {
        m_pABDCal->RemoveAutoBWCalibrationSink(this);
    }
    HX_RELEASE(m_pABDCal);

    HX_RELEASE(m_pBWManager);
    HX_RELEASE(m_pPrefTranMgr);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pMutex);
}

HX_RESULT HXConnectionBWInfo::Create(IUnknown* pContext, 
                                     IUnknown* pOuter, 
                                     IUnknown*& pObj)
{
    HX_RESULT res  = HXR_OUTOFMEMORY;
    IUnknown* pTmp = NULL;
    
    pObj = NULL;

    HXConnectionBWInfo* pNew = new HXConnectionBWInfo;
    
    if (pNew)
    {
        InterlockedIncrement(&(pNew->m_lCount));
        res = pNew->FinalConstruct();
        InterlockedDecrement(&(pNew->m_lCount));

        if (SUCCEEDED(res))
        {
            //XXXgfw I use pTmp here instead of pObj because we do not
            //want to set pObj until it is completely constructed and
            //initialized. If you set it before the call to Init, the
            //core can try and QI off of it, which will result in a
            //nice crash.
            res = pNew->SetupAggregation( pOuter, &pTmp);

            if (SUCCEEDED(res))
            {
                res = pNew->init(pContext);
                if( SUCCEEDED(res) )
                {
                    pObj = pTmp;
                }
            }
        }
        
        if (FAILED(res))
        {
            delete pNew;
            pNew = NULL;
        }
    }

    return res; 
}

/*
 *      IHXConnectionBWInfo methods
 */
STDMETHODIMP
HXConnectionBWInfo::AddABDInfo(THIS_ IHXAutoBWDetection* pABD,
                               IHXPreferredTransport* pPrefTransport)
{
    HXLOGL3(HXLOG_CBWI, "%p CBI::AddABDInfo : %p %p", 
            this, pABD, pPrefTransport);

    HX_RESULT res = HXR_INVALID_PARAMETER;

    m_pMutex->Lock();    

    if (pABD)
    {
        RemoveABDInfo(pABD);

        HXBOOL bABDEnabled = TRUE;
        ReadPrefBOOL(m_pContext, "AutoBWDetection", bABDEnabled);
        
        res = pABD->InitAutoBWDetection(bABDEnabled);

        if (bABDEnabled)
        {
            if (m_abdToPrefTransportMap.SetAt(pABD, pPrefTransport))
            {
                // Hold on to references to these objects now that
                // their pointers are in the map
                pABD->AddRef();

                if (pPrefTransport)
                {
                    pPrefTransport->AddRef();
                }
                
                res = createABDSink(pABD);
            }
            else
            {
                res = HXR_OUTOFMEMORY;
            }

            if (HXR_OK != res)
            {
                RemoveABDInfo(pABD);
            }
        }
    }

    m_pMutex->Unlock();

    return res;
}

STDMETHODIMP
HXConnectionBWInfo::RemoveABDInfo(THIS_ IHXAutoBWDetection* pABD)
{
    HXLOGL3(HXLOG_CBWI, "%p CBI::RemoveABDInfo : %p", this, pABD);

    HX_RESULT res HXR_INVALID_PARAMETER;

    m_pMutex->Lock();

    if (pABD)
    {
        removeABDSink(pABD);
        
        void* pTmp = NULL;
        
        if (m_abdToPrefTransportMap.Lookup((void*)pABD, pTmp))
        {
            IHXPreferredTransport* pCurrent = 
                (IHXPreferredTransport*)pTmp;
            
            // Remove key from the map
            m_abdToPrefTransportMap.RemoveKey(pABD);
            
            // Release map references
            HX_RELEASE(pABD);
            HX_RELEASE(pCurrent);

            res = HXR_OK;
        }
        else
        {
            // Couldn't find the ABD info in the map
            res = HXR_ELEMENT_NOT_FOUND;
        }
    }

    m_pMutex->Unlock();

    return res;
}

STDMETHODIMP
HXConnectionBWInfo::GetConnectionBW(THIS_ REF(UINT32) uBw,
                                    HXBOOL bDetectedBWOnly)
{
    HX_RESULT res = HXR_FAILED;

    m_pMutex->Lock();

    if (needToRecalc())
    {
        recalc();
    }

    if (m_uConnectionBW)
    {
        uBw = m_uConnectionBW;

        if (m_bIsDetectedBW || !bDetectedBWOnly)
        {
            res = HXR_OK;
        }
        else
        {
            // This return value signals that the
            // value being returned is not a detected
            // value.
            res = HXR_INCOMPLETE;
        }
        UpdateStatistics();
    }
    else if (m_abdToSinkMap.GetCount() || (acsPending == m_abdCalState))
    {
        // HXR_WOULD_BLOCK is returned to indicate either dynamic ABD is running or
        // ABD calibration is running
        res = HXR_WOULD_BLOCK;
    }
    else
    {
        // something is wrong here
        HX_ASSERT(FALSE);
    }

    HXLOGL3(HXLOG_CBWI, "%p CBI::GetConnectionBW : res %08x bw %u", this, res, uBw);

    m_pMutex->Unlock();

    return res;
}

STDMETHODIMP
HXConnectionBWInfo::AddSink(THIS_ IHXConnectionBWAdviseSink* pSink)
{
    HXLOGL3(HXLOG_CBWI, "%p CBI::AddSink : %p", this, pSink);

    HX_RESULT res = HXR_INVALID_PARAMETER;

    m_pMutex->Lock();

    if (pSink)
    {
        if (m_sinkList.Find(pSink))
        {
            // We already have this sink in the list
            res = HXR_OK;
        }
        else
        {
            if (m_sinkList.AddTail(pSink))
            {
                pSink->AddRef();
                res = HXR_OK;
            }
            else
            {
                res = HXR_OUTOFMEMORY;
            }
        }
    }

    m_pMutex->Unlock();

    return res;
}

STDMETHODIMP
HXConnectionBWInfo::RemoveSink(THIS_ IHXConnectionBWAdviseSink* pSink)
{
    HXLOGL3(HXLOG_CBWI, "%p CBI::RemoveSink : %p", this, pSink);

    HX_RESULT res = HXR_INVALID_PARAMETER;

    m_pMutex->Lock();

    if (pSink)
    {
        LISTPOSITION pos = m_sinkList.Find(pSink);
        
        if (pos)
        {
            // Check to see if we are deleting
            // the object at the current iterator
            // position
            if (pos == m_sinkDispatchItr)
            {
                // Update m_sinkDispatchItr so it will 
                // point to the correct location
                (void)m_sinkList.GetAt(m_sinkDispatchItr);
            }

            m_sinkList.RemoveAt(pos);
            res = HXR_OK;
        }
    }

    m_pMutex->Unlock();

    return res;
}

STDMETHODIMP
HXConnectionBWInfo::AutoBWCalibrationStarted(THIS_ const char* pszServer)
{
    HXLOGL3(HXLOG_CBWI, "%p CBI::AutoBWCalibrationStarted : %s", 
            this, pszServer);
    return HXR_OK;
}

STDMETHODIMP
HXConnectionBWInfo::AutoBWCalibrationDone(THIS_
                                          HX_RESULT  status,
                                          UINT32     uBW)
{
    HXLOGL3(HXLOG_CBWI, "%p CBI::AutoBWCalibrationDone : %08x %u", 
            this, status, uBW);

    m_pMutex->Lock();

    if (HXR_OK == status)
    {
        m_uABDCalibrationBW = uBW * 1000;

        if (m_pPrefTranMgr)
        {
            // Store the calibration value in the preferred transport
            // manager
            m_pPrefTranMgr->SetAutoBWDetectionValue(m_uABDCalibrationBW);
        }

        m_abdCalState = acsInit;
    }
    else
    {
        m_abdCalState = acsError;
    }

    recalc();

    m_pMutex->Unlock();

    return HXR_OK;
}

STDMETHODIMP
HXConnectionBWInfo::InitializeStatistics(UINT32  /*IN*/ ulRegistryID)
{
    HX_RESULT       rc = HXR_INVALID_PARAMETER;
    char            szRegKeyName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
    IHXBuffer*      pszParentName = NULL;    
    IHXRegistry*    pRegistry = NULL;

    HX_DELETE(m_pEstimatedBWStats);

    if (m_pContext &&
        HXR_OK == m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry))
    {
        if (HXR_OK == pRegistry->GetPropName(ulRegistryID, pszParentName))
        {
            SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.EstimatedBandwidth", pszParentName->GetBuffer());
            
            m_pEstimatedBWStats = new CStatisticEntry(pRegistry, szRegKeyName, REG_TYPE_NUMBER);
            if (!m_pEstimatedBWStats)
            {
                rc = HXR_OUTOFMEMORY;
                goto cleanup;
            }
            rc = HXR_OK;
        }
    }

  cleanup:

    HX_RELEASE(pszParentName);
    HX_RELEASE(pRegistry);

    return rc;
}

STDMETHODIMP HXConnectionBWInfo::UpdateStatistics()
{
    if (m_pEstimatedBWStats)
    {
        return m_pEstimatedBWStats->SetInt(m_uConnectionBW); 
    }

    return HXR_FAILED;
}

void 
HXConnectionBWInfo::AutoBWDetectDone(IHXAutoBWDetection* pABD, 
                                     HX_RESULT  status,
                                     UINT32     uBW)
{
    HXLOGL3(HXLOG_CBWI, "%p CBI::AutoBWDetectDone : %p %08x %u", 
            this, pABD, status, uBW);

    m_pMutex->Lock();

    switch(status) {
       case HXR_OK:
           handleNewABDData(pABD, uBW);
           break;
       case HXR_NOT_SUPPORTED:
           handleABDNotSupported(pABD);
           break;
       case HXR_TIMEOUT:
           /* no-op. Other code will try switching transport */
           break;
       default:
           handleABDError(pABD, status);
           break;
    };

    m_pMutex->Unlock();
}

HX_RESULT HXConnectionBWInfo::static_NetInterfacesUpdated(void* pObj)
{
    HX_RESULT res = HXR_OK;

    if (pObj)
    {
        HXConnectionBWInfo* pConnBWInfo = (HXConnectionBWInfo*)pObj;
        res = pConnBWInfo->NetInterfacesUpdated();
    }

    return res;
}

HX_RESULT
HXConnectionBWInfo::NetInterfacesUpdated(THIS)
{
    HXLOGL3(HXLOG_CBWI, "%p CBI::NetInterfacesUpdated", this);

    m_pMutex->Lock();

    // The network interface has changed. 
    // Clear all cached state since it is likely
    // no longer correct.

    // Clear the current connection BW information
    m_uConnectionBW = 0;
    m_bIsDetectedBW = FALSE;
    m_bNeedRecalc = TRUE;

    // Clear ConnectionBandwidth so that it won't get used
    // as a default
    WritePrefUINT32(m_pContext, "ConnectionBandwidth", 0);

    // Reset calibration state so that we
    // can initiate a ABD calibration if we
    // need to
    m_abdCalState = acsInit;
    m_uABDCalibrationBW = 0;

    // Reset dynamic ABD member variables
    m_dynamicABDMed.Reset();
    m_bGotDynamicABDSample = FALSE;

    // Reset the overall connection BW avg.
    m_connectionBWAvg.Reset();

    m_pMutex->Unlock();

    return HXR_OK;
}

HX_RESULT HXConnectionBWInfo::init(IUnknown* pContext)
{
    HX_RESULT res = HXR_OK;

    if (pContext)
    {
        m_pContext = pContext;
        m_pContext->AddRef();

        pContext->QueryInterface(IID_IHXPreferredTransportManager,
                                 (void**)&m_pPrefTranMgr);

        pContext->QueryInterface(IID_IHXAutoBWCalibration,
                                 (void**)&m_pABDCal);

        pContext->QueryInterface(IID_IHXBandwidthManager,
                                 (void**)&m_pBWManager);

        m_netIFSinkHlpr.Init(pContext, this,
                             &static_NetInterfacesUpdated);

	CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, pContext);  

        if (!m_dynamicABDMed.Init(DynamicABDMovMedSize) ||
            !m_connectionBWAvg.Init(ConnectionBWMovAvgSize))
        {
            res = HXR_FAILED;
        }
    }
    else
    {
        res = HXR_INVALID_PARAMETER;
    }

    return res;
}

HX_RESULT 
HXConnectionBWInfo::createABDSink(IHXAutoBWDetection* pABD)
{
    HX_RESULT res = HXR_OUTOFMEMORY;

    HXConnBWInfoABDSink* pSink = new HXConnBWInfoABDSink(this);
    if (pSink)
    {
        pSink->AddRef();
        
        res = pSink->Init(pABD);
        
        if (HXR_OK == res)
        {
            if (m_abdToSinkMap.SetAt(pABD, pSink))
            {
                // Hold onto a reference for the map
                pSink->AddRef(); 
            }
            else
            {
                pSink->Close();
                res = HXR_OUTOFMEMORY;
            }
        }
            
        HX_RELEASE(pSink);
    }

    return res;
}

void HXConnectionBWInfo::removeABDSink(IHXAutoBWDetection* pABD)
{
    void* pTmp = NULL;

    if (m_abdToSinkMap.Lookup((void*)pABD, pTmp))
    {
        HXConnBWInfoABDSink* pSink = 
            (HXConnBWInfoABDSink*) pTmp;
        
        m_abdToSinkMap.Remove(pABD);

        pSink->Close();
        HX_RELEASE(pSink);
    }
}

void HXConnectionBWInfo::clearTransportMap()
{
    CHXMapPtrToPtr::Iterator itr = 
        m_abdToPrefTransportMap.Begin();
    
    for (; itr != m_abdToPrefTransportMap.End(); ++itr)
    {
        IHXAutoBWDetection* pABD = 
            (IHXAutoBWDetection*)itr.get_key();

        IHXPreferredTransport* pPrefTrans = 
            (IHXPreferredTransport*)*itr;

        HX_RELEASE(pABD);
        HX_RELEASE(pPrefTrans);
    }
    m_abdToPrefTransportMap.RemoveAll();
}

void HXConnectionBWInfo::clearSinkMap()
{
    CHXMapPtrToPtr::Iterator itr = 
        m_abdToSinkMap.Begin();
    
    for (; itr != m_abdToSinkMap.End(); ++itr)
    {
        HXConnBWInfoABDSink* pSink =
            (HXConnBWInfoABDSink*)*itr;

        pSink->Close();

        HX_RELEASE(pSink);
    }
    m_abdToSinkMap.RemoveAll();
}

HXBOOL HXConnectionBWInfo::needToRecalc()
{
    UINT32 uCachedBwPref = 0;
    if (!m_bNeedRecalc &&
        (HXR_OK == ReadPrefUINT32(m_pContext, "Bandwidth", 
                                  uCachedBwPref)) &&
        (uCachedBwPref != m_uCachedBwPref))
    {
        // Clear the calibration value if we detect that
        // the bandwidth preference has changed. The
        // idea is that if the user changed the preference
        // then perhaps that means that the connection has
        // changed as well.
        m_uABDCalibrationBW = 0;

        m_bNeedRecalc = TRUE;
    }

    return m_bNeedRecalc;
}

void HXConnectionBWInfo::recalc()
{
    UINT32 uConnectionBW     = 0;
    HXBOOL bIsDetectedBW     = FALSE;
    HXBOOL bABDSystemEnabled = TRUE;

    HXLOGL3(HXLOG_CBWI, "%p CBI::recalc", this);

    // Update our cached Bandwidth pref value
    ReadPrefUINT32(m_pContext, "Bandwidth", m_uCachedBwPref);

    //Grab the ADBSystemEnabled pref. This controls the entire ABD
    //system. If this pref is turned off (its on by default) then we
    //don't do anything except read the bandwidth prefs out of the
    //registry, if they are there. We will not do Dynamic ABD, back to
    //back packets or any other runtime determination of the
    //bandwidth.
    ReadPrefBOOL( m_pContext, "ABDSystemEnabled", bABDSystemEnabled);
        

    // Check to see if there are any outstanding ABD callbacks
    if( m_abdToSinkMap.GetCount()==0 && bABDSystemEnabled )
    {
        if (m_bGotDynamicABDSample)
        {
            // Initially use the ABD values collected
            // from ABD requests
            uConnectionBW = m_dynamicABDMed.Median();
            bIsDetectedBW = (uConnectionBW > 0) ? TRUE : FALSE;
            
            // Clear the flag since we have now used these
            // new samples in a connection BW computation
            m_bGotDynamicABDSample = FALSE;

            HXLOGL3(HXLOG_CBWI, 
                    "%p CBI::recalc() : Dynamic ABD Moving Average %u", 
                    this, uConnectionBW);
        }

        if (!uConnectionBW && m_pBWManager )
        {
            // Try using information collected from upshift info
            UINT32 uUpshiftBW;
            HX_RESULT tmpRes = m_pBWManager->GetUpshiftBW(uUpshiftBW);

            if (HXR_OK == tmpRes)
            {
                uConnectionBW = uUpshiftBW;
                bIsDetectedBW = (uConnectionBW > 0) ? TRUE : FALSE;

                HXLOGL3(HXLOG_CBWI, 
                        "%p CBI::recalc() : Upshift BW %u", 
                        this, uConnectionBW);
            }
            else if (HXR_NOTIMPL == tmpRes)
            {
                // The function is not implemented so don't
                // bother trying again.
                HX_RELEASE(m_pBWManager);
            }
        }

        if (!uConnectionBW && m_abdToPrefTransportMap.GetCount())
        {
            // Try using the info in the preferred transport
            // objects
            uConnectionBW = prefTransportAvg();
            bIsDetectedBW = (uConnectionBW > 0) ? TRUE : FALSE;

            HXLOGL3(HXLOG_CBWI, 
                    "%p CBI::recalc() : Preferred Transport Average %u", 
                    this, uConnectionBW);
        }
        
        if (!uConnectionBW && m_pPrefTranMgr)
        {
            // Try getting the value from the preferred transport
            // manager
            m_pPrefTranMgr->GetAutoBWDetectionValue(uConnectionBW);
            bIsDetectedBW = (uConnectionBW > 0) ? TRUE : FALSE;

            HXLOGL3(HXLOG_CBWI, 
                    "%p CBI::recalc() : Preferred Transport Manager BW %u", 
                    this, uConnectionBW);
        }

        if (!uConnectionBW && m_pABDCal)
        {
            // Try ABD calibration
            if (m_uABDCalibrationBW)
            {
                // We have a calibration value so use that
                uConnectionBW = m_uABDCalibrationBW;
                bIsDetectedBW = (uConnectionBW > 0) ? TRUE : FALSE;

                HXLOGL3(HXLOG_CBWI, 
                        "%p CBI::recalc() : ABD Calibration BW %u", 
                        this, uConnectionBW);
            }
            else
            {
                HXBOOL bABDEnabled = TRUE;
                ReadPrefBOOL(m_pContext, "AutoBWDetection", bABDEnabled);
                
                if (bABDEnabled)
                {
                    switch(m_abdCalState) {
                       case acsInit: {
                           // We have not requested an ABD calibration
                           // yet. Let's do so now.
                    
                           m_abdCalState = acsPending;
                    
                           m_pABDCal->AddAutoBWCalibrationSink(this);
                    
                           HX_RESULT res_ABD = 
                               m_pABDCal->StartAutoBWCalibration();
                    
                           if ((HXR_OK == res_ABD) ||
                               (HXR_WOULD_BLOCK == res_ABD))
                           {
                               // Clear m_bNeedRecalc so we don't
                               // call recalc() unnecessarily during
                               // the calibration request
                               m_bNeedRecalc = FALSE;
                           }
                           else
                           {
                               m_abdCalState = acsError;
                           }

                       }break;
                       case acsPending:
                           // An ABD Calibration request is
                           // currently pending. Do nothing
                           // for now
                           break;
                       case acsError:
                           // An ABD calibration has already
                           // been done, but it resulted in an
                           // error. Do nothing and rely on
                           // default behavior
                           break;
                    };
                }
            }
        }

        if ((acsPending != m_abdCalState) && !uConnectionBW)
        {
            // First try reading the ConnectionBandwidth preference. 
            UINT32 uConnectionBWPref = 0;
            if ((HXR_OK == ReadPrefUINT32(m_pContext, 
                                          "ConnectionBandwidth", 
                                          uConnectionBWPref)) &&
                (uConnectionBWPref > 0)) 
            {
                uConnectionBW = uConnectionBWPref;

                HXLOGL3(HXLOG_CBWI, 
                        "%p CBI::recalc() : ConnectionBandwidth Pref %u", 
                        this, uConnectionBW);
            }
        }
    }

    if( !uConnectionBW && (acsPending!=m_abdCalState || !bABDSystemEnabled) )
    {
        if( m_uCachedBwPref )
        {
            // Use the preference value if we have one.
            uConnectionBW = m_uCachedBwPref;
            HXLOGL3(HXLOG_CBWI, "%p CBI::recalc() : Bandwidth Pref %u",
                    this, uConnectionBW);
        }
        else
        {
            // Wild guess
            uConnectionBW = 40000;
            HXLOGL3(HXLOG_CBWI, "%p CBI::recalc() : Wild Guess %u", 
                    this, uConnectionBW);
        }
    }

    if (uConnectionBW)
    {
        // Add this value to the connection BW moving
        // average
        m_connectionBWAvg.AddSample(uConnectionBW);

        // Get the connection BW value.
        m_uConnectionBW = m_connectionBWAvg.Average();
        m_bIsDetectedBW = bIsDetectedBW;

        // Store the preference in the registry so that
        // it can be recovered the next time the player
        // starts
        WritePrefUINT32(m_pContext, "ConnectionBandwidth", 
                        m_uConnectionBW);
            
        m_bNeedRecalc = FALSE;
        dispatchSinkCalls(m_uConnectionBW);
    }

}

void HXConnectionBWInfo::handleNewABDData(IHXAutoBWDetection* pABD, 
                                          UINT32     uBW)
{
    // need to figure out why ABD reports 0 BW
    HX_ASSERT(0 != uBW);
    // exclude 0 BW reported
    if (uBW)
    {
        IHXPreferredTransport* pTransport = getTransport(pABD);

        if (pTransport)
        {
            pTransport->SetAutoBWDetectionValue(uBW);
        }
        HX_RELEASE(pTransport);

        m_dynamicABDMed.AddSample(uBW);
        m_bGotDynamicABDSample = TRUE;
    }

    removeABDSink(pABD);
    recalc();
}

void HXConnectionBWInfo::handleABDNotSupported(IHXAutoBWDetection* pABD)
{
    // Remove the sink for this ABD object since ABD is not supported
    removeABDSink(pABD);
    recalc();
}

void HXConnectionBWInfo::handleABDError(IHXAutoBWDetection* pABD,
                                        HX_RESULT status)
{
    // Remove the sink for this ABD object since an error occurred
    removeABDSink(pABD);
    recalc();
}

void HXConnectionBWInfo::dispatchSinkCalls(UINT32 uBW)
{
    LISTPOSITION pos = m_sinkList.GetHeadPosition();

    while(pos)
    {
        IHXConnectionBWAdviseSink* pSink = 
            (IHXConnectionBWAdviseSink*)m_sinkList.GetNext(pos);
        
        // If this fires it means that 2 pieces of code
        // are trying to dispatch at the same time. That
        // shouldn't ever happen.
        HX_ASSERT(m_sinkDispatchItr == 0);

        m_sinkDispatchItr = pos;
        
        pSink->AddRef();
        pSink->NewConnectionBW(uBW);
        HX_RELEASE(pSink);
            
        // NOTE: m_abdDispatchItr can change inside the call above if
        //       pSink was removed from the list
        pos = m_sinkDispatchItr;
        m_sinkDispatchItr = 0;
    }
}

UINT32 HXConnectionBWInfo::prefTransportAvg()
{
    UINT32 uRet = 0;

    if (m_abdToPrefTransportMap.GetCount())
    {
        HXBOOL bAddedSample = FALSE;

        CHXMapPtrToPtr::Iterator itr = 
            m_abdToPrefTransportMap.Begin();
        
        // Collect the ABD values from the preferred transport
        // objects
        for (; itr != m_abdToPrefTransportMap.End(); ++itr)
        {
            IHXPreferredTransport* pPrefTran = 
                (IHXPreferredTransport*)*itr;
            UINT32 uTmp;
            
            if (pPrefTran &&
                (HXR_OK == pPrefTran->GetAutoBWDetectionValue(uTmp)))
            {
                m_dynamicABDMed.AddSample(uTmp);
                bAddedSample = TRUE;
            }
        }
        
        if (bAddedSample)
        {
            uRet = m_dynamicABDMed.Median();
        }
    }

    return uRet;
}
