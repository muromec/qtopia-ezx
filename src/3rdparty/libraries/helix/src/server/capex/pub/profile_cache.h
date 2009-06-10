/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: profile_cache.h,v 1.3 2003/08/06 23:23:07 jgordon Exp $
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

#ifndef _PROFILE_CACHE_H_
#define _PROFILE_CACHE_H_

_INTERFACE IUnknown;
_INTERFACE IHXPSSProfileData;

/*
 * class StaticProfileCache
 * Simple profile cache for profiles that will be cached at startup
 * (default profiles).
 * Add and create are not threadsafe and must never occur while 
 * another thread or process might be accessing the cache.
 */

class StaticProfileCache : public IHXProfileCache
{
public:
    StaticProfileCache();
    virtual ~StaticProfileCache();

    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXProfileCache methods
    STDMETHOD (InitCache)           (THIS_ UINT32 ulMaxSize = 0);
    STDMETHOD (GetProfile)          (THIS_ const char* szKey,
                                     REF(IHXPSSProfileData*) pProfile,
                                     REF(UINT32) ulMergeRule);
    STDMETHOD (AddProfile)          (THIS_ const char* szKey,
                                     IHXPSSProfileData* pProfile,
                                     UINT32 ulMergeRule = HX_CP_CMR_NORMAL);

    STDMETHOD (RemoveProfile)       (THIS_ const char* szKey);

protected:
    UINT32 m_ulRefCount;
    Dict* m_pCacheTable;

    struct ProfileData
    {
        IHXPSSProfileData* pProfile;
        UINT32 ulMergeRule;
    };
};

// XXXJDG replace IHXProfileCache with http cache interface
class HTTPProfileCache : public IHXProfileCache
{
public:
    HTTPProfileCache();
    virtual ~HTTPProfileCache();

    // IUnknown methods
    STDMETHOD (QueryInterface)      (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXProfileCache methods
    STDMETHOD (InitCache)           (THIS_ UINT32 ulMaxSize = 0);
    STDMETHOD (GetProfile)          (THIS_ const char* szKey,
                                     REF(IHXPSSProfileData*) pProfile,
                                     REF(UINT32) ulMergeRule);
    STDMETHOD (AddProfile)          (THIS_ const char* szKey,
                                     IHXPSSProfileData* pProfile,
                                     UINT32 ulMergeRule = HX_CP_CMR_NORMAL);
    STDMETHOD (RemoveProfile)       (THIS_ const char* szKey);

protected:
    UINT32 m_ulRefCount;
    UINT32 m_ulMaxSize;

    // XXXJDG replace this with a more thread safe hash table
    Dict* m_pCacheTable;
    HX_MUTEX m_Mutex;

    struct ProfileData
    {
        IHXPSSProfileData* pProfile;
        UINT32 ulMergeRule;
    };
};

#endif /* _PROFILE_CACHE_H_ */
