/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: profile_cache.cpp,v 1.4 2005/06/09 23:04:51 jgordon Exp $
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

#include "hlxclib/string.h"
#include "hxtypes.h"
#include "hxcom.h"
#include "hxassert.h"
#include "ihxpckts.h"
#include "hxmutexlock.h"

#include "dict.h"

#include "hxclientprofile.h"
#include "hxpssprofile.h"
#include "hxprofilecache.h"
#include "profile_cache.h"


StaticProfileCache::StaticProfileCache()
: m_ulRefCount(0)
, m_pCacheTable(NULL)
{
}

StaticProfileCache::~StaticProfileCache()
{
    if(m_pCacheTable)
    {
        Dict_iterator iterator(m_pCacheTable);
        ProfileData* pPrfData;
        for(Dict_entry* pEntry=NULL; *iterator; ++iterator)
        {
            pEntry = *iterator;
            pPrfData = (ProfileData*)pEntry->obj;
            if(pPrfData)
            {
                HX_RELEASE(pPrfData->pProfile);
                delete pPrfData;
            }
        }

        delete(m_pCacheTable);
    }
}

STDMETHODIMP
StaticProfileCache::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXProfileCache))
    {
        AddRef();
        *ppvObj = (IHXProfileCache*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
StaticProfileCache::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
StaticProfileCache::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0) 
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
StaticProfileCache::InitCache(UINT32 ulMaxSize)
{
    HX_RESULT rc = HXR_OK;

    if(m_pCacheTable)
    {
        rc = HXR_ALREADY_INITIALIZED;
    }
    
    if(SUCCEEDED(rc))
    {
        m_pCacheTable = new Dict();
        if(!m_pCacheTable)
        {
            rc = HXR_OUTOFMEMORY;
        }
    }

    return rc;
}

STDMETHODIMP
StaticProfileCache::GetProfile(const char* szKey, 
                               REF(IHXPSSProfileData*) pProfile,
                               REF(UINT32) ulMergeRule)
{
    pProfile = NULL;
    ulMergeRule = 0;

    if(!m_pCacheTable)
    {
        return HXR_NOT_INITIALIZED;
    }

    // XXXSCR - Turns out there are many, many Nokia User-Agents
    // so we need to be able to match more.  I admit that iterating through
    // removes all reasons for having this in a hash table, but
    // in the interest of simplicity, I'll leave it.  We'll do a better
    // job on the head.
    if(m_pCacheTable)
    {
        Dict_iterator iterator(m_pCacheTable);
        for(Dict_entry* pEntry=NULL; *iterator; ++iterator)
        {
            pEntry = *iterator;
            if (pEntry && pEntry->key && pEntry->obj && 
                    !strncmp(pEntry->key, szKey, strlen(pEntry->key)))
            {
                ProfileData* pPrfData = (ProfileData*)pEntry->obj;
                pProfile = pPrfData->pProfile;
                if(pProfile)
                {
                    pProfile->AddRef();
                    ulMergeRule = pPrfData->ulMergeRule;
                    return HXR_OK;
                }
            }
        }
    }

    return HXR_FAIL;
}

STDMETHODIMP
StaticProfileCache::AddProfile(const char* szKey, 
                               IHXPSSProfileData* pProfile,
                               UINT32 ulMergeRule)
{
    HX_ASSERT(pProfile);
    HX_RESULT rc = HXR_OK;

    if(!m_pCacheTable)
    {
        return HXR_NOT_INITIALIZED;
    }

    ProfileData* pPrfData = new ProfileData();
    if(pPrfData)
    {
        pProfile->AddRef();
        pPrfData->pProfile = pProfile;
        pPrfData->ulMergeRule = ulMergeRule;

        Dict_entry* pEntry = m_pCacheTable->enter(szKey, (void*)pPrfData);
        if(!pEntry)
        {
            pProfile->Release();
            delete pPrfData;
            rc = HXR_OUTOFMEMORY;
        }
        else if(pEntry->obj != (void*)pPrfData)
        {
            // That key already existed, so replace the value
            ProfileData* pOldPrf = (ProfileData*)pEntry->obj;
            pEntry->obj = (void*)pPrfData;
            HX_RELEASE(pOldPrf->pProfile);
            delete pOldPrf;
        }
    }
    else
    {
        rc = HXR_OUTOFMEMORY;
    }

    return rc;
}

STDMETHODIMP
StaticProfileCache::RemoveProfile(const char* szKey)
{
    return HXR_NOTIMPL;
}


HTTPProfileCache::HTTPProfileCache()
: m_ulRefCount(0)
, m_ulMaxSize(0)
, m_pCacheTable(NULL)
{
    m_Mutex = HXMutexCreate();
    HXMutexInit(m_Mutex);
}

HTTPProfileCache::~HTTPProfileCache()
{
    if(m_pCacheTable)
    {
        Dict_iterator iterator(m_pCacheTable);
        ProfileData* pPrfData;
        for(Dict_entry* pEntry = *iterator; pEntry; ++iterator)
        {
            pPrfData = (ProfileData*)pEntry->obj;
            if(pPrfData)
            {
                HX_RELEASE(pPrfData->pProfile);
                delete pPrfData;
            }
        }

        delete(m_pCacheTable);
    }

    HXMutexDestroy(m_Mutex);
}

STDMETHODIMP
HTTPProfileCache::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXProfileCache))
    {
        AddRef();
        *ppvObj = (IHXProfileCache*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
HTTPProfileCache::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
HTTPProfileCache::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0) 
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
HTTPProfileCache::InitCache(UINT32 ulMaxSize)
{
    HX_RESULT rc = HXR_OK;

    if(m_pCacheTable)
    {
        rc = HXR_ALREADY_INITIALIZED;
    }
    
    if(SUCCEEDED(rc))
    {
        m_ulMaxSize = ulMaxSize;
        m_pCacheTable = new Dict();
        if(!m_pCacheTable)
        {
            rc = HXR_OUTOFMEMORY;
        }
    }

    return rc;
}

STDMETHODIMP
HTTPProfileCache::GetProfile(const char* szKey, 
                             REF(IHXPSSProfileData*) pProfile,
                             REF(UINT32) ulMergeRule)
{
    pProfile = NULL;
    ulMergeRule = 0;

    if(!m_pCacheTable)
    {
        return HXR_NOT_INITIALIZED;
    }

    HXMutexLock(m_Mutex);
    Dict_entry* pEntry = m_pCacheTable->find(szKey);
    HXMutexUnlock(m_Mutex);

    ProfileData* pPrfData = NULL;

    if(pEntry && pEntry->obj)
    {
        pPrfData = (ProfileData*)pEntry->obj;    
        pProfile = pPrfData->pProfile;
        if(pProfile)
        {
            pProfile->AddRef();;
            ulMergeRule = pPrfData->ulMergeRule;

            // XXXJDG TODO: determine whether the cached profile is stale
            return HXR_OK;
        }
    }
    return HXR_FAIL;
}

STDMETHODIMP
HTTPProfileCache::AddProfile(const char* szKey, 
                             IHXPSSProfileData* pProfile,
                             UINT32 ulMergeRule)
{
    HX_ASSERT(pProfile);

    HX_RESULT rc = HXR_OK;

    if(!m_pCacheTable)
    {
        return HXR_NOT_INITIALIZED;
    }

    ProfileData* pPrfData = new ProfileData();
    if(pPrfData)
    {
        pProfile->AddRef();
        pPrfData->pProfile = pProfile;
        pPrfData->ulMergeRule = ulMergeRule;

        HXMutexLock(m_Mutex);
        // XXJDG hash table needs to take care of checking size
        if(m_ulMaxSize && (UINT32)m_pCacheTable->size() < m_ulMaxSize)
        {
            Dict_entry* pEntry = m_pCacheTable->enter(szKey, (void*)pPrfData);
            if(!pEntry)
            {
                pProfile->Release();
                delete pPrfData;
                rc = HXR_OUTOFMEMORY;
            }
            else if(pEntry->obj != (void*)pPrfData)
            {
                // That key already existed, so replace the value
                ProfileData* pOldPrf = (ProfileData*)pEntry->obj;
                pEntry->obj = (void*)pPrfData;
                HX_RELEASE(pOldPrf->pProfile);
                delete pOldPrf;
            }
        }
        else
        {
            pProfile->Release();
            delete pPrfData;
            rc = HXR_IGNORE;
        }
        HXMutexUnlock(m_Mutex);
    }
    else
    {
        rc = HXR_OUTOFMEMORY;
    }

    return rc;
}

STDMETHODIMP
HTTPProfileCache::RemoveProfile(const char* szKey)
{
    return HXR_NOTIMPL;
}
