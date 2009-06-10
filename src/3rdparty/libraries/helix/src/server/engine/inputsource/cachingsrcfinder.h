/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: cachingsrcfinder.h,v 1.3 2007/05/23 19:01:11 seansmith Exp $ 
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

#ifndef _CACHINGSRCFINDER_H_
#define _CACHINGSRCFINDER_H_

#include "hxfiles.h"
#include "hxclientprofile.h"
#include "plgnhand.h"
#include "srcfinder.h"

class Process;
class URL;
class FSManager;


class CPacketHandler;

/*
 * An input source finder with caching.
 */

class CachingSourceFinder : public BasicSourceFinder
{
public:
    CachingSourceFinder(Process* pProc, ClientSession* pPlayerSession);
    virtual ~CachingSourceFinder(void);

    virtual HX_RESULT FindSource(URL* pURL, ServerRequest* pRequest);
    virtual HX_RESULT FindNextSource(void);

protected:
    void CacheSource(URL* pUrl);
    IUnknown* FindCachedSource(URL* pUrl, IHXRequest* pRequest);

private:
    struct CacheEntry
    {
        time_t                  m_tCacheEnter;
        IUnknown*               m_pFileObject;
        ServerRequestWrapper*   m_pFileRequest;
        IHXFileFormatObject*    m_pFileFormatObject;
        ServerRequestWrapper*   m_pFileFormatRequest;
        IHXPSourceControl*      m_pSourceControl;
    };

    class CCachePruneCB : public IHXCallback
    {
    public:
        CCachePruneCB(Process* pProc);
        ~CCachePruneCB(void);

        // IUnknown methods 
        STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
        STDMETHOD_(ULONG32,AddRef)  (THIS);
        STDMETHOD_(ULONG32,Release) (THIS);

        // IHXCallback methods 
        STDMETHOD(Func)             (THIS);

        ULONG32                     m_ulRefCount;
        Process*                    m_pProc;
    };
    friend class CCachePruneCB;

    static void                 CachePrune(void);

    static CHXMapStringToOb*    zm_pCache;
    static CCachePruneCB*       zm_pCachePruneCB;
    static CallbackHandle       zm_hCachePruneCB;
    static UINT32               zm_uCachePruneFreq;
    static UINT32               zm_uCacheMaxAge;
};


#endif /* _CACHINGSRCFINDER_H_ */
