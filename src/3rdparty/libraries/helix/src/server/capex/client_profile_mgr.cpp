/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: client_profile_mgr.cpp,v 1.15 2007/12/13 06:10:50 yphadke Exp $
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
#include "hlxclib/stdio.h"

#include "hxerror.h"
#include "hxstrutl.h"
#include "hxcomm.h"
#include "ihxlist.h"
#include "hxlist.h"
#include "ihxpckts.h"
#include "hxstats.h"
#include "hxclientprofile.h"
#include "hxpssprofile.h"
#include "hxplugn.h"
#include "hxmon.h"
#include "plgnhand.h"
#include "proc.h"
#include "simple_callback.h"
#include "dispatchq.h"
#include "misc_plugin.h"
#include "chxmapptrtoptr.h"
#include "debug.h"

#include "hxprofilecache.h"
#include "profile_cache.h"
#include "profile_request.h"
#include "client_profile.h"
#include "client_profile_mgr.h"

#define MAX_PROFILE_CACHE_KEY "config.CapabilityExchange.ProfileCacheSize"
#include "hxqos.h"
#include "qos_prof_conf.h"
#include "qos_cfg_names.h"

ClientProfileManager::ClientProfileManager(Process* pProc)
: m_ulRefCount(0)
, m_pProc(pProc)
, m_pPrfAgentProc(NULL)
, m_pPrfAgent(NULL)
, m_bInitialized(FALSE)
, m_pStaticCache(NULL)
, m_pCache(NULL)
, m_pDefaultProfile(NULL)
, m_ulDfltPrfMergeRule(HX_CP_CMR_OVER)
{
}

ClientProfileManager::~ClientProfileManager()
{
    HX_RELEASE(m_pPrfAgent);
    HX_RELEASE(m_pStaticCache);
    HX_RELEASE(m_pCache);
    HX_RELEASE(m_pDefaultProfile);
}

STDMETHODIMP
ClientProfileManager::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXClientProfileManager))
    {
        AddRef();
        *ppvObj = (IHXClientProfileManager*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
ClientProfileManager::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
ClientProfileManager::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0) 
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
ClientProfileManager::GetPSSProfile(IHXClientProfileManagerResponse* pResponse,
                                    REF(IHXClientProfile*) pClientProfile,
                                    IHXSessionStats* pStats,
                                    IHXBuffer* pRequestId, 
                                    IHXBuffer* pRequestURI,
                                    IHXValues* pRequestHeaders)
{
    HX_RESULT rc = HXR_OK;
    HX_VERIFY(pResponse);
   
    // Get the ptagent plugin if it is not already registered
    if (!m_bInitialized)
    {
        rc = Init();
    }

    if (FAILED(rc))
    {
        pResponse->PSSProfileReady(HXR_NOTIMPL, pClientProfile, pRequestId,
                                   pRequestURI, pRequestHeaders);
        return rc;
    }

    HX_VERIFY(m_pProc);
    HX_VERIFY(m_pPrfAgentProc);
    HX_VERIFY(m_pPrfAgent);

    BOOL bProfilesCached = TRUE;
    IHXBuffer* pUserAgentHdr = NULL;
    IHXPSSProfileData* pDefaultProfile = NULL;
    IHXBuffer* pXwapPrfHdr = NULL;
    IHXBuffer* pXwapPrfDiffHdr = NULL;
    CHXSimpleList* pPrfReqList = NULL;
    UINT32 ulDfltPrfMergeRule = HX_CP_CMR_NORMAL;
    IUnknown* pContext = (IUnknown*)(m_pProc->pc->server_context);
    IHXCommonClassFactory* pCCF = (IHXCommonClassFactory*)
                                  (m_pProc->pc->common_class_factory);

    // Find the default profile
    pRequestHeaders->GetPropertyCString("user-agent", pUserAgentHdr);

    IHXQoSProfileSelector* pProfileSelector = NULL;
    IHXUserAgentSettings* pUAS = NULL;
    IHXQoSProfileConfigurator* pProfileConfigurator = NULL;
    IHXBuffer* pLocalRDF = NULL;
    BOOL bRetriveXWAPProfile = TRUE;
    INT32 lTemp=0;

    pContext->QueryInterface(IID_IHXQoSProfileSelector, (void**)&pProfileSelector);
    pContext->QueryInterface(IID_IHXQoSProfileConfigurator, (void**)&pProfileConfigurator);

    if(pProfileSelector && pProfileConfigurator)
    {
        pProfileSelector->SelectProfile(pUserAgentHdr, NULL, NULL, pUAS);

        pProfileConfigurator->SetUserAgentSettings(pUAS);
        pProfileConfigurator->GetConfigBuffer(QOS_CFG_CC_CE_LOCAL_RDF, pLocalRDF);
        if(SUCCEEDED(pProfileConfigurator->GetConfigInt(QOS_CFG_CC_CE_RET_XWAP_PROFILE, lTemp)))
        {
            bRetriveXWAPProfile = (BOOL)lTemp;
        }
    }

    GetDefaultProfile(pLocalRDF, pDefaultProfile, ulDfltPrfMergeRule);

    HX_RELEASE(pUserAgentHdr);
    HX_RELEASE(pLocalRDF);
    HX_RELEASE(pUAS);
    HX_RELEASE(pProfileSelector);
    HX_RELEASE(pProfileConfigurator);

    if(bRetriveXWAPProfile)
    {
        // Extract the list of all profile URI's (and overrides) from the request
        // headers
        if (ulDfltPrfMergeRule != HX_CP_CMR_DOMINATE && 
            SUCCEEDED(pRequestHeaders->GetPropertyCString("x-wap-profile",
                                                        pXwapPrfHdr)))
        {
            pRequestHeaders->GetPropertyCString("x-wap-profile-diff", 
                pXwapPrfDiffHdr);

            rc = InitProfileReqList(pXwapPrfHdr, pXwapPrfDiffHdr, pPrfReqList, 
                    bProfilesCached);
            HX_RELEASE (pXwapPrfHdr);
            HX_RELEASE (pXwapPrfDiffHdr);
        }
    }

    // Initialize the profile request handler
    ProfileRequestHandler* pReqHandler = NULL; 
    if(SUCCEEDED(rc))
    {
        pReqHandler = new ProfileRequestHandler(this);
        if(!pReqHandler)
        {
            rc = HXR_OUTOFMEMORY;
        }
    }

    if(SUCCEEDED(rc))
    {
        pClientProfile = new ClientProfile;
        if (!pClientProfile)
        {
            rc = HXR_OUTOFMEMORY;
        }
    }

    if(SUCCEEDED(rc))
    {
        pReqHandler->AddRef();
        pReqHandler->m_ulRequestType = 
            ProfileRequestHandler::PRF_REQ_TYPE_CLIENT;
        pReqHandler->m_pPrfReqInfoList = pPrfReqList;
        pReqHandler->m_pDefaultProfile = pDefaultProfile;
        pReqHandler->m_pPrfMgrResp = pResponse;
        pResponse->AddRef();
        pReqHandler->m_pCache = m_pCache;
        m_pCache->AddRef();

        pReqHandler->m_pClientProfile = pClientProfile;
        pClientProfile->AddRef();

        if (pRequestId)
        {
            pReqHandler->m_pRequestId = pRequestId;
            pRequestId->AddRef();
        }
    
        if (pRequestURI)
        {
            pReqHandler->m_pRequestURI = pRequestURI;
            pRequestURI->AddRef();
        }

        if (pRequestHeaders)
        {
            pReqHandler->m_pRequestHeaders = pRequestHeaders;
            pRequestHeaders->AddRef();
        }

        if (pStats)
        {
            pReqHandler->m_pSessionStats = pStats;
            pStats->AddRef();
        }

        if (bProfilesCached)
        {
            //If all profiles are cached and not stale, just update the attributes
            //in the client using the cached values 
            rc = pReqHandler->SetClientProfileFromCache(pContext);
            if(SUCCEEDED(rc))
            {
                StoreURIs(pReqHandler);
            }
        }
        else
        {
            //If at least one of the profiles is not cached or is stale, send the
            //request off to the plugin proc 
            ProfileRequestHandler::RequestInitCB* pReqInitCB = NULL; 
            pReqInitCB = new ProfileRequestHandler::RequestInitCB(pReqHandler);
            pReqHandler->m_pReqInitCB = pReqInitCB;

            m_pProc->pc->dispatchq->send(m_pProc, pReqInitCB,
                                     m_pPrfAgentProc->procnum());

            return rc;
            //... request handler will be destroyed in ProfileRequestDone
        }
    }
    pResponse->PSSProfileReady(rc, pClientProfile, pRequestId,
                               pRequestURI, pRequestHeaders);
    HX_RELEASE(pReqHandler);

    return rc;
}

HX_RESULT
ClientProfileManager::Init()
{
    HX_ASSERT(m_pProc);
    HX_ASSERT(m_pProc->pc);

    m_pStaticCache = (IHXProfileCache*)m_pProc->pc->capex_static_cache;
    m_pCache = (IHXProfileCache*)m_pProc->pc->capex_profile_cache;
    HX_ASSERT(m_pStaticCache);
    HX_ASSERT(m_pCache);
    m_pStaticCache->AddRef();
    m_pCache->AddRef();
    
    HX_RESULT rc = RegisterTransferAgent();
    if(SUCCEEDED(rc))
    {
        m_bInitialized = TRUE;
    }

    return rc;
}

HX_RESULT
ClientProfileManager::RegisterTransferAgent()
{
    HX_RESULT rc = HXR_OK;
 
    PluginHandler* pPluginHandler = m_pProc->pc->plugin_handler;
    PluginHandler::Plugin* pPlugin = NULL;
    IHXPluginEnumerator* pEnum = NULL;
    IUnknown* pHXPlugin = NULL;
    IHXPSSPTAgent* pPTAgent = NULL; 

    if (HXR_OK == pPluginHandler->QueryInterface(IID_IHXPluginEnumerator,
                                                     (void**)&pEnum))
    {
        INT32 ilPlgnIndx = pEnum->GetNumOfPlugins() - 1;
        for ( ; (ilPlgnIndx >= 0 && !pPTAgent);  --ilPlgnIndx)
        {
            rc = pEnum->GetPlugin(ilPlgnIndx, pHXPlugin);
            if (HXR_OK != rc)
            {
                HX_RELEASE(pHXPlugin);
                continue;
            }

            CHXSimpleList* pPluginList = NULL;
            if (SUCCEEDED(rc = pHXPlugin->QueryInterface(IID_IHXPSSPTAgent,
                                                      (void**)&pPTAgent)))
            {
                pPluginList = pPluginHandler->m_general_handler->m_pPlugins;
                CHXSimpleList::Iterator litr = pPluginList->Begin();

                for (INT32 lIndx = 0; litr != pPluginList->End();
                    ++litr, ++lIndx)
                {
                    if (lIndx == ilPlgnIndx)
                    {
                        pPlugin = (PluginHandler::Plugin*)(*litr);
                        break;
                    }
                }

                break;
            }

            HX_RELEASE(pHXPlugin);
        }
    }
    HX_RELEASE(pEnum);
    HX_RELEASE(pHXPlugin);
    HX_RELEASE(pPTAgent);

    if (HXR_OK == rc && pPlugin)
    {
        BOOL bFoundPluginInfo = FALSE; 
        PTAgentPlugin* pPluginInfo = NULL;
        bFoundPluginInfo = m_pProc->pc->misc_plugins->Lookup(pPlugin,
                                                         (void*&)pPluginInfo);
        if (bFoundPluginInfo)
        {
            m_pPrfAgentProc = pPluginInfo->m_pProc;
            m_pPrfAgent =  pPluginInfo->m_pPTAgent;
            m_pPrfAgent->AddRef();
        }
        else
        {
            rc = HXR_FAIL;
        }
    }

    return rc;
}

HX_RESULT
ClientProfileManager::ProfileRequestDone(ProfileRequestHandler* pReqHandler)
{
    HX_RESULT rc = HXR_OK;
    HX_VERIFY(pReqHandler);

    if(pReqHandler->m_ulRequestType == 
        ProfileRequestHandler::PRF_REQ_TYPE_STATIC_CACHE)
    {
        // Store it to the cache
        if(SUCCEEDED(pReqHandler->m_ulRequestStatus))
        {
            IHXPSSProfileData* pPrfData = NULL;
            IHXListIterator* pListIterator = 
                pReqHandler->m_pParsedPrfDataList->Begin();
            IUnknown* pListItem = NULL;
            rc = HXR_FAIL;
            if(pListIterator && pListIterator->HasItem())
            {
                pListItem = pListIterator->GetItem();
                if(pListItem && SUCCEEDED(pListItem->QueryInterface(
                    IID_IHXPSSProfileData, (void**)&pPrfData)))
                {
                    pPrfData->SetProfileMergeRule(
                        pReqHandler->m_ulCacheMergeRule);

                    rc = m_pStaticCache->AddProfile((const char*) 
                        pReqHandler->m_pCacheKey->GetBuffer(), pPrfData,
                        pReqHandler->m_ulCacheMergeRule);
                }
            }

            HX_RELEASE(pListIterator);
            HX_RELEASE(pListItem);
            HX_RELEASE(pPrfData);
        }
    }
    else
    {
        StoreURIs(pReqHandler);

        //... pass the updated client profile back to the callee
        IHXClientProfileManagerResponse* pResponse = pReqHandler->m_pPrfMgrResp;
        pResponse->PSSProfileReady(pReqHandler->m_ulRequestStatus,
                                pReqHandler->m_pClientProfile,
                                pReqHandler->m_pRequestId,
                                pReqHandler->m_pRequestURI,
                                pReqHandler->m_pRequestHeaders);
    }

    HX_DELETE(pReqHandler->m_pReqInitCB);
    HX_DELETE(pReqHandler->m_pReqDoneCB);
    HX_RELEASE(pReqHandler);

    return rc;
}

HX_RESULT
ClientProfileManager::InitProfileReqList(IHXBuffer* pXwapPrfHdr,
                                         IHXBuffer* pXwapPrfDiffHdr,
                                         CHXSimpleList*& pPrfReqInfoList,
                                         BOOL& bCached)
{
    HX_RESULT rc = HXR_OK;


    IHXValues* pPrfDiffBuffList = NULL;
    const char* pszCurrProfile = (const char*)pXwapPrfHdr->GetBuffer();
    const char* pszHeaderEnd = pszCurrProfile + pXwapPrfHdr->GetSize();
    const char* pszNextProfile = NULL;
    ProfileRequestInfo* pCurrReqInfo = NULL;
   
    //Extract profile override values from the x-wap-profile-diff headers 
    if(pXwapPrfDiffHdr)
    {
        CreateProfileDiffList(pPrfDiffBuffList, pXwapPrfDiffHdr);
    }
    pPrfReqInfoList = new CHXSimpleList;
    if(!pPrfReqInfoList)
    {
        return HXR_OUTOFMEMORY;
    }

    INT32 lHeaderBuffLen = pXwapPrfHdr->GetSize();
    
    while (lHeaderBuffLen > 0 && HXR_OK == rc)
    {
        //Extract one profile reference at a time from the header, saving the
        //profile URI or profile-diff document in the pPrfReqInfoList 
        pszNextProfile = strchr(pszCurrProfile, ',');
        UINT32 ulBuffLen = (pszNextProfile) ? (pszNextProfile - pszCurrProfile)
                           : (pszHeaderEnd - pszCurrProfile);

        ++ulBuffLen;
        NEW_FAST_TEMP_STR(pszProfileHdr, 512, ulBuffLen);
        snprintf(pszProfileHdr, ulBuffLen, pszCurrProfile);
        pszProfileHdr[ulBuffLen - 1] = '\0';

        lHeaderBuffLen -= ulBuffLen;
        pszCurrProfile = (pszNextProfile) ? (pszCurrProfile + ulBuffLen)
                        : pszHeaderEnd; 
       
        rc = ParseProfileHeader(pszProfileHdr, pPrfReqInfoList,
                                pPrfDiffBuffList, pCurrReqInfo);
        if (SUCCEEDED(rc))
        {
            HX_VERIFY(pCurrReqInfo);
            if (!pCurrReqInfo->m_bProfileURICached)
            {
                bCached = FALSE;
            }
        }
        DELETE_FAST_TEMP_STR(pszProfileHdr);
    }
    HX_RELEASE(pPrfDiffBuffList);

    if (FAILED(rc) && pPrfReqInfoList)
    {
        CHXSimpleList::Iterator liter = pPrfReqInfoList->Begin();
        ProfileRequestInfo* pInfo = NULL;
        for( ; liter != pPrfReqInfoList->End(); ++liter)
        {
            pInfo = (ProfileRequestInfo*)(*liter);
            delete pInfo;
            pInfo = NULL;
        }

        pPrfReqInfoList->RemoveAll();
        delete pPrfReqInfoList;
        pPrfReqInfoList = NULL;
    }
    return rc;
}

HX_RESULT
ClientProfileManager::ParseProfileHeader(char* pszProfileHdr,
                                         CHXSimpleList* pPrfReqInfoList,
                                         IHXValues* pPrfDiffList,
                                         ProfileRequestInfo*& pReqInfo)
{
    HX_RESULT rc = HXR_OK;
    HX_VERIFY(pPrfReqInfoList);
    HX_VERIFY(pszProfileHdr);

    IHXCommonClassFactory* pCCF = m_pProc->pc->common_class_factory;
    HX_ASSERT(pCCF);
    UINT32 ulHeaderLen = strlen(pszProfileHdr);
    while (!isalnum(*pszProfileHdr) && ulHeaderLen)
    {
        ++pszProfileHdr;
        --ulHeaderLen;
    }

    //Determine if the profile header contains a profile URI or a reference to a
    //profile-diff
    if (ulHeaderLen > 5 && ((strncasecmp(pszProfileHdr, "http:", 5) == 0) ||
        (strncasecmp(pszProfileHdr, "file:", 5) == 0)))
    {
        //We've found a profile URI
        IHXBuffer* pProfileURI = NULL;
        pReqInfo = new ProfileRequestInfo;
        pPrfReqInfoList->AddTail(pReqInfo);

        //Remove the quote at the end of the URI
        UINT32 ulBuffSize = ulHeaderLen + 1; 
        if ( pszProfileHdr[ulBuffSize - 2] ==  '"')
        {
            pszProfileHdr[ulBuffSize - 2] = '\0';
            --ulBuffSize;
        }

        pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pProfileURI);
        pProfileURI->Set((UCHAR*)pszProfileHdr, ulBuffSize);
        pReqInfo->SetProfileURI(pProfileURI);

        //XXX: STUB
        //When caching is supported, mark URI as cached pReqInfo if it is in our
        //cache and is fresh
        // Check the cache for the URI
        // XXXJDG TODO check if stale
        IHXPSSProfileData* pProfileData = NULL;
        UINT32 ulMergeRule = 0;
        if(SUCCEEDED(m_pCache->GetProfile(pszProfileHdr, pProfileData, 
            ulMergeRule)))
        {
            pReqInfo->SetProfileData(pProfileData);
            pReqInfo->m_bProfileURICached = TRUE;
        }
        HX_RELEASE(pProfileURI);
    }
    else if (pPrfDiffList)
    {
        //We may have a profile diff
        const char* pszDiffSeq = pszProfileHdr;
        char* pszDiffMD5Digest = strchr(pszProfileHdr, '-');
        BOOL bFoundPrfDiff = FALSE;

        if (pszDiffMD5Digest) 
        {
            *pszDiffMD5Digest = '\0';
            ++pszDiffMD5Digest;
            
            char *pTmp = NULL;
            UINT32 ulPrfDiffSeqNum = strtoul(pszDiffSeq, &pTmp, 10);
            IHXBuffer* pPrfDiff = NULL;

            if (ulPrfDiffSeqNum && SUCCEEDED(
                pPrfDiffList->GetPropertyCString(pszDiffSeq, pPrfDiff)))
            {
                if (!pReqInfo)
                {
                    pReqInfo = new ProfileRequestInfo;
                    pPrfReqInfoList->AddTail(pReqInfo);
                }
                pReqInfo->AddProfileDiff(pPrfDiff);
                bFoundPrfDiff = TRUE; 
            }
            HX_RELEASE(pPrfDiff);
        }

        if (!bFoundPrfDiff)
        {
            rc = HXR_FAIL;
        }
    }
    else
    {
        // Not a valid URI or diff
        rc = HXR_FAIL;
    }

    return rc;
}

HX_RESULT
ClientProfileManager::CreateProfileDiffList(IHXValues*& pList,
                                            IHXBuffer* pXwapPrfDiffHdr)
{
    HX_VERIFY(!pList);

    IHXCommonClassFactory* pCCF = m_pProc->pc->common_class_factory;
    HX_VERIFY(pCCF);
    
    pCCF->CreateInstance(CLSID_IHXValues, (void**)&pList);

    //XXX STUB:
    //Insert profile diff documents into the list using the diff sequence number
    //as the key

    return HXR_OK;
}

HX_RESULT
ClientProfileManager::InitGlobalCache(IHXPSSPTAgent* pPrfAgent, 
                                      Process* pPrfAgentProc)
{
    HX_ASSERT(pPrfAgent);
    HX_ASSERT(pPrfAgentProc);
    HX_ASSERT(m_pProc);

    m_pStaticCache = (IHXProfileCache*)m_pProc->pc->capex_static_cache;
    m_pCache = (IHXProfileCache*)m_pProc->pc->capex_profile_cache;
    HX_ASSERT(m_pStaticCache);
    HX_ASSERT(m_pCache);
    m_pStaticCache->AddRef();
    m_pCache->AddRef();

    m_pPrfAgent = pPrfAgent;
    m_pPrfAgentProc = pPrfAgentProc;

    // Initialize the cache objects. If it's already initialized, we must 
    // have already populated it, so there's nothing more to do
    // Should never happen
    HX_RESULT rc = m_pStaticCache->InitCache();
    if(rc == HXR_ALREADY_INITIALIZED)
    {
        rc = HXR_OK;
    }
    else if(SUCCEEDED(rc))
    {
        rc = CacheDefaultProfiles();
    }
    
    if(SUCCEEDED(rc))
    {
        INT32 lMaxCache = 0;
        IUnknown* pContext = (IUnknown*)m_pProc->pc->server_context;
        IHXRegistry2* pRegistry = NULL;
        if(pContext && SUCCEEDED(pContext->QueryInterface(IID_IHXRegistry2, 
            (void**)&pRegistry)))
        {
            pRegistry->GetIntByName(MAX_PROFILE_CACHE_KEY, lMaxCache);
            pRegistry->Release();
        }
        rc = m_pCache->InitCache((UINT32)lMaxCache);
        if(rc == HXR_ALREADY_INITIALIZED)
        {
            rc = HXR_OK;
        }
    }

    if(SUCCEEDED(rc))
    {
        m_bInitialized = TRUE;
    }

    return rc;
}

HX_RESULT
ClientProfileManager::GetDefaultProfile(IHXBuffer* pUserAgentHdr, 
                                        IHXPSSProfileData*& pDefaultProfile,
                                        UINT32& ulMergeRule)
{
    HX_RESULT rc = HXR_FAIL;

    if(!m_pStaticCache)
    {
        m_pStaticCache = (IHXProfileCache*)m_pProc->pc->capex_static_cache;
        m_pStaticCache->AddRef();
    }

    // Look for a default profile for this user-agent
    if(pUserAgentHdr)
    {
        rc = m_pStaticCache->GetProfile(
            (const char*)pUserAgentHdr->GetBuffer(), pDefaultProfile, 
            ulMergeRule);
    }

    // If there is none specified for this user-agent, go with the default
    if(FAILED(rc))
    {
        if(!m_pDefaultProfile)
        {
            // Store the default so we won't have to fetch it again
            rc = m_pStaticCache->GetProfile("Default", m_pDefaultProfile, 
                    m_ulDfltPrfMergeRule);
        }
        else
        {
            rc = HXR_OK;
        }
        pDefaultProfile = m_pDefaultProfile;
        ulMergeRule = m_ulDfltPrfMergeRule;
        if(pDefaultProfile)
        {
            pDefaultProfile->AddRef();
        }
    }

    return rc;
}

HX_RESULT
ClientProfileManager::CacheStaticProfile(IHXBuffer* pKey, 
                                         UINT32 ulMergeRule)
{
    HX_RESULT rc = HXR_OK;

    HX_ASSERT(pKey);
    HX_VERIFY(m_pProc);
    HX_VERIFY(m_pPrfAgentProc);
    HX_VERIFY(m_pPrfAgent);

    // Parse the URI
    CHXSimpleList* pPrfReqList = new CHXSimpleList;;
    ProfileRequestInfo* pReqInfo = NULL;
    IHXValues* pDiff = NULL;
    if(!pPrfReqList)
    {
        return HXR_OUTOFMEMORY;
    }

    rc = ParseProfileHeader((char*)pKey->GetBuffer(), pPrfReqList, 
            pDiff, pReqInfo);

    if(FAILED(rc))
    {
        return rc;
    }

    // Initialize the profile request handler
    ProfileRequestHandler* pReqHandler = new ProfileRequestHandler(this);
    if(!pReqHandler)
    {
        return HXR_OUTOFMEMORY;
    }

    pReqHandler->AddRef();
    pReqHandler->m_ulRequestType = 
        ProfileRequestHandler::PRF_REQ_TYPE_STATIC_CACHE;
    pReqHandler->m_pPrfReqInfoList = pPrfReqList;   
    pReqHandler->m_ulCacheMergeRule = ulMergeRule;
    if (pKey)
    {
        pReqHandler->m_pCacheKey = pKey;
        pKey->AddRef();

        pReqHandler->m_pRequestURI = pKey;
        pKey->AddRef();
    }

    //send the request off to the plugin proc 
    ProfileRequestHandler::RequestInitCB* pReqInitCB = NULL; 
    pReqInitCB = new ProfileRequestHandler::RequestInitCB(pReqHandler);
    if(!pReqInitCB)
    {
        return HXR_OUTOFMEMORY;
    }

    pReqHandler->m_pReqInitCB = pReqInitCB;

    m_pProc->pc->dispatchq->send(m_pProc, pReqInitCB,
                             m_pPrfAgentProc->procnum());
    
    //... request handler will be destroyed in ProfileRequestDone

    return rc; 
}

HX_RESULT
ClientProfileManager::CacheDefaultProfiles()
{
    IUnknown* pContext = (IUnknown*)m_pProc->pc->server_context;
    IHXRegistry2* pRegistry = NULL;
    if(!pContext || FAILED(pContext->QueryInterface(IID_IHXRegistry2, 
        (void**)&pRegistry)))
    {
        return HXR_FAIL;
    }

    UINT32 ulRegId = 0;
    const char* szRegName = NULL;
    IHXBuffer* pKey = NULL;
    IHXBuffer* pURI = NULL;
    IHXBuffer* pPropName = NULL;
    char szProp[512];
    UINT32* pulProfiles = NULL;
    UINT32 ulNumProfiles = 0;
    UINT32 i;

    IHXQoSProfileSelector* pProfileSelector = NULL;
    IHXUserAgentSettings* pUAS = NULL;
    IHXQoSProfileConfigurator* pProfileConfigurator = NULL;
    CHXMapStringToOb::Iterator itrUASConfigTree;
    IHXBuffer* pLocalRDF = NULL;

    pContext->QueryInterface(IID_IHXQoSProfileSelector, (void**)&pProfileSelector);
    pContext->QueryInterface(IID_IHXQoSProfileConfigurator, (void**)&pProfileConfigurator);
    if(pProfileSelector && pProfileConfigurator)
    {
        itrUASConfigTree = pProfileSelector->GetBegin();
       
        for(; itrUASConfigTree != pProfileSelector->GetEnd(); itrUASConfigTree++)
        {
            pUAS = (IHXUserAgentSettings*) *itrUASConfigTree;
            pProfileConfigurator->SetUserAgentSettings((IHXUserAgentSettings*) *itrUASConfigTree);
            pProfileConfigurator->GetConfigBuffer(QOS_CFG_CC_CE_LOCAL_RDF, pLocalRDF);

            if(pLocalRDF)
            {
                CacheStaticProfile(pLocalRDF, HX_CP_CMR_OVER);
                HX_RELEASE(pLocalRDF);
            }
        }
    }
    HX_RELEASE(pProfileSelector);
    HX_RELEASE(pProfileConfigurator);

    HX_RELEASE(pRegistry);
    return HXR_OK;
}

HX_RESULT
ClientProfileManager::StoreURIs(ProfileRequestHandler* pRequest)
{
    HX_ASSERT(pRequest);
    HX_ASSERT(m_pProc);
    HX_ASSERT(m_pProc->pc);

    HX_RESULT rc = HXR_OK;
    IHXPSSProfileData* pPrfData = NULL;
    IUnknown* pListItem = NULL;
    UINT32 ulBufSize = 0;
    IHXBuffer* pURI;
    IHXListIterator* pProfileIter;
    BOOL bFirstURI = TRUE;
    char* pURIStr;
    IHXBuffer* pProfileURIs;
    IHXList* pProfileList = pRequest->m_pParsedPrfDataList;
    IHXCommonClassFactory* pCCF = m_pProc->pc->common_class_factory;

    HX_ASSERT(pCCF);

    if(!pRequest->m_pParsedPrfDataList || !pRequest->m_pSessionStats)
    {
        return HXR_UNEXPECTED;
    }

    pProfileIter = pProfileList->Begin();
    if (!pProfileIter || !pProfileIter->HasItem())
    {
        // no URIs
        HX_RELEASE(pProfileIter);
        return HXR_OK;
    }
    
    rc = pCCF->CreateInstance(IID_IHXBuffer, (void**)&pProfileURIs);
    if(FAILED(rc))
    {
        return rc;
    }

    // count the space needed. just count the whole buffer size.
    while (pProfileIter->HasItem())
    {
        pListItem = pProfileIter->GetItem();
        pProfileIter->MoveNext();
        if(pListItem && SUCCEEDED(pListItem->QueryInterface(
            IID_IHXPSSProfileData, (void**)&pPrfData)))
        {
            if(SUCCEEDED(pPrfData->GetProfileURI(pURI)))
            {
                ulBufSize += pURI->GetSize();
                pURI->Release();
            }
            pPrfData->Release();
        }

        HX_RELEASE(pListItem);
    }

    pProfileIter->Release();
    int nBytesCopied;

    // Copy the URIs into the buffer
    rc = pProfileURIs->SetSize(ulBufSize);
    pProfileIter = pProfileList->Begin();
    pURIStr = (char*)pProfileURIs->GetBuffer();
    while (SUCCEEDED(rc) && pProfileIter && pProfileIter->HasItem() 
            && ulBufSize)
    {
        pListItem = pProfileIter->GetItem();
        pProfileIter->MoveNext();
        if(pListItem && SUCCEEDED(pListItem->QueryInterface(
            IID_IHXPSSProfileData, (void**)&pPrfData)))
        {
            if(SUCCEEDED(pPrfData->GetProfileURI(pURI)))
            {
                if(bFirstURI)
                {
                    nBytesCopied = SafeSprintf(pURIStr, ulBufSize, "%s",
                                    (const char*)pURI->GetBuffer());
                    bFirstURI = FALSE;
                }
                else
                {
                    nBytesCopied = SafeSprintf(pURIStr, ulBufSize, ";%s",
                                    (const char*)pURI->GetBuffer());
                }
                
                if(nBytesCopied < 0)
                {
                    ulBufSize = 0;
                    rc = HXR_UNEXPECTED;
                }
                else
                {
                    ulBufSize -= nBytesCopied;
                    pURIStr += nBytesCopied;
                }

                pURI->Release();
            }
            pPrfData->Release();
        }

        HX_RELEASE(pListItem);
    }
    HX_RELEASE(pProfileIter);

    if(SUCCEEDED(rc))
    {
        rc = pRequest->m_pSessionStats->SetClientProfileURIs(pProfileURIs);
    }

    HX_RELEASE(pProfileURIs);

    return rc;
}

