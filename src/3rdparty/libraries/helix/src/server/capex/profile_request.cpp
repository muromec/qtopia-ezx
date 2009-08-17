/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: profile_request.cpp,v 1.9 2005/10/21 01:57:54 shebak Exp $
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
#include "hxtypes.h"
#include "hxassert.h"

#include "hxerror.h"
#include "hxstrutl.h"
#include "hxlist.h"
#include "ihxlist.h"
#include "hxlistp.h"
#include "ihxpckts.h"
#include "hxstats.h"
#include "hxclientprofile.h"
#include "hxpssprofile.h"
#include "proc.h"
#include "simple_callback.h"
#include "dispatchq.h"
#include "debug.h" 

#include "hxprofilecache.h"
#include "profile_request.h"
#include "client_profile_mgr.h"

#if defined(HELIX_FEATURE_SERVER_CAPEX)
#include "pss_profile_data.h"
#endif

ProfileRequestHandler::ProfileRequestHandler(ClientProfileManager* pPrfMgr)
: m_ulRefCount(0) 
, m_pContext(NULL)
, m_ulRequestType(PRF_REQ_TYPE_CLIENT)
, m_pRequestId(NULL)
, m_pRequestURI(NULL)
, m_pCacheKey(NULL)
, m_ulCacheMergeRule(HX_CP_CMR_NORMAL)
, m_pRequestHeaders(NULL)
, m_ulRequestStatus(HXR_OK)
, m_pPrfReqInfoList(NULL)
, m_pParsedPrfDataList(NULL)
, m_pDefaultProfile(NULL)
, m_pCurrPrfInfo(NULL)
, m_pCurrPrfDiff(NULL)
, m_pReqInitCB(NULL)
, m_pReqDoneCB(NULL)
, m_pClientProfile(NULL)
, m_pSessionStats(NULL)
, m_pPrfMgr(NULL)
, m_pCache(NULL)
, m_pPrfMgrResp(NULL)
{
    HX_VERIFY(pPrfMgr);

    m_pPrfMgr = pPrfMgr;
    m_pPrfMgr->AddRef();
}

ProfileRequestHandler::~ProfileRequestHandler()
{
    HX_RELEASE(m_pRequestId);
    HX_RELEASE(m_pRequestURI);
    HX_RELEASE(m_pCacheKey);
    HX_RELEASE(m_pRequestHeaders);
    HX_RELEASE(m_pCurrPrfDiff);
    HX_RELEASE(m_pParsedPrfDataList);
    HX_RELEASE(m_pDefaultProfile);
    HX_RELEASE(m_pClientProfile);
    HX_RELEASE(m_pSessionStats);
    HX_RELEASE(m_pPrfMgr);
    HX_RELEASE(m_pCache);
    HX_RELEASE(m_pPrfMgrResp);
    HX_RELEASE(m_pContext);

    delete m_pReqInitCB;
    m_pReqInitCB = NULL;

    delete m_pReqDoneCB;
    m_pReqDoneCB = NULL;

    delete m_pCurrPrfInfo;
    m_pCurrPrfInfo = NULL;

    CHXSimpleList::Iterator liter;
    if (m_pPrfReqInfoList)
    {
        ProfileRequestInfo* pInfo = NULL;
        liter = m_pPrfReqInfoList->Begin();
        for( ; liter != m_pPrfReqInfoList->End(); ++liter)
        {
            pInfo = (ProfileRequestInfo*)(*liter);
            delete pInfo;
            pInfo = NULL;

        }
        m_pPrfReqInfoList->RemoveAll();
        delete m_pPrfReqInfoList;
        m_pPrfReqInfoList = NULL;
    }

}

STDMETHODIMP
ProfileRequestHandler::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSSPTAgentResponse))
    {
        AddRef();
        *ppvObj = (IHXPSSPTAgentResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
ProfileRequestHandler::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
ProfileRequestHandler::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0) 
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

HX_RESULT
ProfileRequestHandler::Init(Process* pProc)
{
    HX_RESULT rc = HXR_OK;
    HX_VERIFY(pProc);

    HX_RELEASE(m_pContext);
    m_pContext = (IUnknown*)(pProc->pc->server_context);
    m_pContext->AddRef();

    if (m_pPrfReqInfoList->IsEmpty())
    {
        //let manager know we're done... 
        //we should never get called with an empty request list...
        m_ulRequestStatus = HXR_FAIL; 
        SendRequestDone();
        return rc;
    }

    IHXFastAlloc* pMemCache = NULL;

    rc = m_pContext->QueryInterface(IID_IHXFastAlloc, (void**)&pMemCache);
    if(SUCCEEDED(rc))
    {
        m_pParsedPrfDataList = (IHXList*) (new CHXList(pMemCache));
        if(m_pParsedPrfDataList)
        {
            m_pParsedPrfDataList->AddRef(); 
        }
        else
        {
            rc = HXR_OUTOFMEMORY;
        }
    }

    if(SUCCEEDED(rc))
    {
        //XXX: When caching is supported, the last mod date and/or
        //etag associated with cached profiles should be passed to the PTAgent.
        m_pCurrPrfInfo = (ProfileRequestInfo*)(m_pPrfReqInfoList->RemoveHead());
        IHXBuffer* pURI = (IHXBuffer*)(m_pCurrPrfInfo->m_pProfileURI);
        IHXBuffer* pURILastMod = NULL;
        IHXBuffer* pURIEtag = NULL;

        if(m_pCurrPrfInfo->m_bProfileURICached)
        {
            ProfileReady(HXR_OK, m_pCurrPrfInfo->m_pProfileData, 
                m_pCurrPrfInfo->m_pProfileURI, NULL, NULL);
        }
        else
        {
            IHXPSSPTAgent* pPrfAgent = m_pPrfMgr->m_pPrfAgent;
            if (pPrfAgent)
            {
                pPrfAgent->GetProfileFromOrigin(pURI, pURIEtag,  pURILastMod,
                                                (IHXPSSPTAgentResponse*)this, NULL);  
            }
        }
    }

    if(FAILED(rc))
    {
        m_ulRequestStatus = rc; 
        SendRequestDone();
    }
    return rc;
}

STDMETHODIMP
ProfileRequestHandler::ProfileReady(HX_RESULT  ulStatus,
                                    IHXPSSProfileData* pProfileData,
                                    IHXBuffer* pProfileURI,
                                    IHXBuffer* pRequestId,
                                    IHXValues* pPrfServerRespHeaders)
{
    HX_RESULT rc = HXR_OK;
    HX_VERIFY(m_pParsedPrfDataList);
    HX_VERIFY(m_pCurrPrfInfo);
    HX_RELEASE(m_pCurrPrfDiff);
    
    //XXX: When caching is supported, handle HXR_NOT_MODIFIED as success 
    if (FAILED(ulStatus))
    {
        delete m_pCurrPrfInfo;
        m_pCurrPrfInfo = 0;
        m_ulRequestStatus = ulStatus; 
        SendRequestDone();
        return rc;
    }

    m_pParsedPrfDataList->InsertTail(pProfileData);
    IHXPSSPTAgent* pPrfAgent = m_pPrfMgr->m_pPrfAgent;
    
    if (m_pCurrPrfInfo->m_pProfileDiffList &&
        !m_pCurrPrfInfo->m_pProfileDiffList->IsEmpty())
    {
        //Process the next set of profile overrides contained in an
        //x-wap-profile-diff header
        m_pCurrPrfDiff = (IHXBuffer*)
                         (m_pCurrPrfInfo->m_pProfileDiffList->RemoveHead());
        pPrfAgent->GetProfileFromBuffer(m_pCurrPrfDiff,
                                       (IHXPSSPTAgentResponse*)this, NULL); 
        //... continues in ::ProfileReady
    }
    else
    {
        // Store the profileData for the last processed URI in the cache
        // XXX Profile data from the x-wap-profile-diff should not be cached.
        if( !m_pCurrPrfInfo->m_bProfileURICached && 
            m_ulRequestType == PRF_REQ_TYPE_CLIENT &&
            m_pCurrPrfInfo->m_pProfileURI)
        {
            pProfileData->SetProfileMergeRule(HX_CP_CMR_NORMAL);

            m_pCache->AddProfile(
                (const char*) m_pCurrPrfInfo->m_pProfileURI->GetBuffer(),
                pProfileData, HX_CP_CMR_NORMAL);
        }

        delete m_pCurrPrfInfo;
        m_pCurrPrfInfo = 0;

        if (m_pPrfReqInfoList->IsEmpty())
        {
            //We're done...
            if(m_pClientProfile)
            {
                SetClientProfile();
            }
            SendRequestDone();
        }
        else
        {
            //Process the next profile URI...
            
            m_pCurrPrfInfo = (ProfileRequestInfo*)
                             (m_pPrfReqInfoList->RemoveHead());
            IHXBuffer* pURI = (IHXBuffer*)(m_pCurrPrfInfo->m_pProfileURI);
            IHXBuffer* pURILastMod = NULL;
            IHXBuffer* pURIEtag = NULL;

            if(m_pCurrPrfInfo->m_bProfileURICached)
            {
                ProfileReady(ulStatus, m_pCurrPrfInfo->m_pProfileData, 
                    m_pCurrPrfInfo->m_pProfileURI, NULL, NULL);
            }
            else
            {
                pPrfAgent->GetProfileFromOrigin(pURI, pURIEtag, pURILastMod,
                    (IHXPSSPTAgentResponse*)this, NULL);
            }
            //... continues in ::ProfileReady
        }
    }

    return rc;
}

HX_RESULT
ProfileRequestHandler::SetClientProfile()
{
    HX_VERIFY(m_pClientProfile);
    HX_RESULT ulStatus = HXR_OK;
    
#if defined(HELIX_FEATURE_SERVER_CAPEX)
    UINT32 ulComponentId = 0;
    IHXListIterator* pProfileIter = NULL; 
    IHXPSSProfileData* pPrfData = NULL;
    IUnknown* pListItem = NULL;
    HX_RESULT res = HXR_OK;

    // Add the default to the end of the list
    if(m_pDefaultProfile)
    {
        m_pParsedPrfDataList->InsertTail(m_pDefaultProfile);
    }

    while (ulComponentId < HX_CP_LAST_COMPONENT_ID)
    {
        pProfileIter = m_pParsedPrfDataList->Begin();

        while (pProfileIter && pProfileIter->HasItem())
        {
            pListItem = pProfileIter->GetItem();
            pProfileIter->MoveNext();

            res = pListItem->QueryInterface(IID_IHXPSSProfileData, 
                (void**)&pPrfData);
            if (FAILED(res))
            {
                HX_ASSERT(!"Bad profile list item!");
                HX_RELEASE(pListItem);
                break;
            }

            if (!pPrfData->IsComponentDefined(ulComponentId))
            {
                HX_RELEASE(pListItem); 
                HX_RELEASE(pPrfData); 
                continue;
            }

            if (!m_pClientProfile->IsComponentDefined(ulComponentId))
            {
                m_pClientProfile->SetComponentAttributes(pPrfData,
                                                         ulComponentId);
            }
            else
            {
                //XXX: Merge current component attributes in all profiles
                //Merge profiles cannot be cached
                HX_RELEASE(pPrfData);
                pPrfData = (IHXPSSProfileData*)(new PSSProfileData);
                pPrfData->AddRef();

                pPrfData->CreateMergedProfile(m_pContext, 
                    m_pParsedPrfDataList, ulComponentId);

                m_pClientProfile->SetComponentAttributes(pPrfData,
                                                         ulComponentId);
				HX_RELEASE(pListItem);
				HX_RELEASE(pPrfData);
                break;
            }

            HX_RELEASE(pListItem);
            HX_RELEASE(pPrfData);
        }
        HX_RELEASE(pProfileIter);
        ulComponentId = HX_CP_GET_NEXT_COMPONENT_ID(ulComponentId); 
    }
#else
    ulStatus = HXR_NOTIMPL;
#endif
    m_ulRequestStatus = ulStatus; 
    
    return HXR_OK;
}

HX_RESULT
ProfileRequestHandler::SetClientProfileFromCache(IUnknown* pContext)
{
    //Invoked in the streamer from the profile manager's ::GetPSSProfileMethod    
    HX_RESULT rc = HXR_OK;
    IHXFastAlloc* pMemCache = NULL;
    
    HX_ASSERT(pContext);
    m_pContext = pContext;
    pContext->AddRef();

    rc = pContext->QueryInterface(IID_IHXFastAlloc, (void**)&pMemCache);

    if(SUCCEEDED(rc))
    {
        m_pParsedPrfDataList = (IHXList*) (new CHXList(pMemCache));
        if(m_pParsedPrfDataList)
        {
            m_pParsedPrfDataList->AddRef();

            // XXXJDG FIXME this is lame
            while(m_pPrfReqInfoList && !m_pPrfReqInfoList->IsEmpty())
            {
                ProfileRequestInfo* pReqInfo = (ProfileRequestInfo*)
                             (m_pPrfReqInfoList->RemoveHead());
                HX_ASSERT(pReqInfo);
                HX_ASSERT(pReqInfo->m_pProfileURI);
                if(pReqInfo && pReqInfo->m_pProfileData)
                {
                    // pass the ref on
                    m_pParsedPrfDataList->InsertTail(pReqInfo->m_pProfileData);
                }
                delete(pReqInfo);
            }

            rc = SetClientProfile();
        }
        else
        {
            rc = HXR_OUTOFMEMORY;
        }
    }

    return rc;
}

HX_RESULT
ProfileRequestHandler::SendRequestDone()
{
    RequestDoneCB* pReqDoneCB = new RequestDoneCB(this, m_pPrfMgr);
    m_pReqDoneCB = pReqDoneCB;
    
    Process* pPluginProc = m_pPrfMgr->m_pPrfAgentProc;
    Process* pStreamerProc = m_pPrfMgr->m_pProc;

    //Transfer control back to the streamer from the plugin proc.
    //The current ProfileRequestHandler object will be destroyed when the
    //profile manager's ::ProfileRequestDone method
    pPluginProc->pc->dispatchq->send(pPluginProc, pReqDoneCB,
                                     pStreamerProc->procnum()); 

    return HXR_OK;
}

/***********************************************************************
*  RequestInitCB implementation
*
* The RequestInitCB object is used when transfering control to the plugin proc
* from the streamer.
*************************************************************************/
ProfileRequestHandler::RequestInitCB::RequestInitCB(ProfileRequestHandler* pRH)
: m_pReqHandler(NULL)
{
    HX_VERIFY(pRH);
    m_pReqHandler = pRH;
    m_pReqHandler->AddRef();
}

ProfileRequestHandler::RequestInitCB::~RequestInitCB()
{
    HX_RELEASE(m_pReqHandler);
}

void
ProfileRequestHandler::RequestInitCB::func(Process* proc)
{
    //... executed in the plugin proc
    HX_VERIFY(m_pReqHandler);
    m_pReqHandler->Init(proc);
}

/***********************************************************************
*  RequestDoneCB implementation
*
* The RequestDoneCB object is used when transferring control back to the
* streamer from the ptagent proc.
*************************************************************************/
ProfileRequestHandler::RequestDoneCB::RequestDoneCB(ProfileRequestHandler* pRH,
                                                 ClientProfileManager* pPrfMgr)
: m_pReqHandler(NULL)
, m_pPrfMgr(NULL)
{
    HX_VERIFY(pRH);
    HX_VERIFY(pPrfMgr);
   
    m_pReqHandler = pRH;
    m_pReqHandler->AddRef();
    
    m_pPrfMgr = pPrfMgr;
    pPrfMgr->AddRef();
}

ProfileRequestHandler::RequestDoneCB::~RequestDoneCB()
{
    HX_RELEASE(m_pReqHandler);
    HX_RELEASE(m_pPrfMgr);
}

void
ProfileRequestHandler::RequestDoneCB::func(Process* proc)
{
    //... executed in the streamer
    HX_VERIFY(m_pPrfMgr);
    HX_VERIFY(m_pReqHandler);
    m_pPrfMgr->ProfileRequestDone(m_pReqHandler);
}

/***********************************************************************
*  ProfileRequestInfo implementation
*
* The ProfileRequestInfo object is used to store the profile URI and all
* associated profile-diff documents that attributes stored in the doc
* referenced by the URI.
*************************************************************************/
ProfileRequestInfo::ProfileRequestInfo()
: m_bProfileURICached(FALSE)
, m_pProfileURI(NULL)
, m_pProfileDiffList(NULL)
, m_pProfileData(NULL)
{

}

ProfileRequestInfo::~ProfileRequestInfo()
{
    HX_RELEASE(m_pProfileURI);
    HX_RELEASE(m_pProfileData);

    if (m_pProfileDiffList)
    {
        CHXSimpleList::Iterator liter = m_pProfileDiffList->Begin();
        for( ; liter != m_pProfileDiffList->End(); ++liter)
        {
            IHXBuffer* pDiffBuff = (IHXBuffer*)(*liter);
            HX_RELEASE(pDiffBuff);
        }

        m_pProfileDiffList->RemoveAll();
        delete m_pProfileDiffList;
        m_pProfileDiffList = NULL;
    }
}

void
ProfileRequestInfo::SetProfileURI(IHXBuffer* pProfileURI)
{
    HX_VERIFY(pProfileURI);
    HX_RELEASE(m_pProfileURI);

    m_pProfileURI = pProfileURI;
    m_pProfileURI->AddRef();
}

void
ProfileRequestInfo::AddProfileDiff(IHXBuffer* pProfileDiff)
{
    HX_VERIFY(pProfileDiff);

    if (!m_pProfileDiffList)
    {
        m_pProfileDiffList = new CHXSimpleList;
    }
    m_pProfileDiffList->AddTail(pProfileDiff);
    pProfileDiff->AddRef();
}

void
ProfileRequestInfo::SetProfileData(IHXPSSProfileData* pProfileData)
{
    HX_VERIFY(pProfileData);
    HX_RELEASE(m_pProfileData);

    m_pProfileData = pProfileData;
    m_pProfileData->AddRef();
}
