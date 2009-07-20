/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: client_profile_mgr.h,v 1.6 2007/09/28 06:19:03 npatil Exp $
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

#ifndef _CLIENT_PROFILE_MANAGER_H
#define _CLIENT_PROFILE_MANAGER_H

class ProfileRequestHandler;
class ProfileRequestInfo;
_INTERFACE IHXPSSPTAgent;
_INTERFACE IHXRegistry2;
_INTERFACE IHXProfileCache;

class ClientProfileManager : public IHXClientProfileManager
{
public:
    ClientProfileManager(Process* pStreamerProc);
    virtual ~ClientProfileManager();

    /***********************************************************************
     *  IUnknown methods
     *************************************************************************/
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    /***********************************************************************
     *  IHXClientProfileManager methods
     *************************************************************************/
    STDMETHOD(GetPSSProfile) (THIS_
                              IHXClientProfileManagerResponse* pResponse,
                              REF(IHXClientProfile*) pClientProfile,
                              IHXSessionStats* pStats,
                              IHXBuffer* pRequestId, IHXBuffer* pRequestURI,
                              IHXValues* pRequestHeaders);

    /***********************************************************************
     * Non-interface methods 
     *************************************************************************/
    HX_RESULT InitGlobalCache(IHXPSSPTAgent* pPrfAgent, 
                              Process* pPrfAgentProc);

    HX_RESULT Init();
    HX_RESULT RegisterTransferAgent();
    HX_RESULT ProfileRequestDone(ProfileRequestHandler* pReqHandler);
    HX_RESULT InitProfileReqList(IHXBuffer* pXwapPrfHdr,
                                 IHXBuffer* pXwapPrfDiffHdr,
                                 CHXSimpleList*& pPrfReqInfoList,
                                 BOOL& bCached);
    HX_RESULT ParseProfileHeader(char* pszProfileHdr,
                                 CHXSimpleList* pPrfInfoList,
                                 IHXValues* pPrfDiffList,
                                 ProfileRequestInfo*& pReqInfo);
    HX_RESULT CreateProfileDiffList(IHXValues*& pList,
                                    IHXBuffer* pXwapPrfDiffHdr);

protected:
    HX_RESULT CacheDefaultProfiles  (void);
    HX_RESULT GetClientProfiles     (IHXBuffer* pXwapPrfHdr,
                                     IHXBuffer* pXwapPrfDiffHdr,
                                     CHXSimpleList*& pPrfReqInfoList,
                                     BOOL& bCached);
    HX_RESULT GetDefaultProfile     (IHXBuffer* pUserAgentHdr, 
                                     IHXPSSProfileData*& pDefaultProfile,
                                     UINT32& ulMergeRule);
    HX_RESULT CacheStaticProfile    (IHXBuffer* pKey, 
                                     UINT32 ulMergeRule);
    HX_RESULT StoreURIs             (ProfileRequestHandler* pRequest);

    UINT32 m_ulRefCount;
    Process* m_pProc;
    Process* m_pPrfAgentProc;
    IHXPSSPTAgent* m_pPrfAgent;
    BOOL m_bInitialized;
    IHXProfileCache* m_pStaticCache;
    IHXProfileCache* m_pCache;
    IHXPSSProfileData* m_pDefaultProfile;
    UINT32 m_ulDfltPrfMergeRule;

    friend class ProfileRequestHandler; 
};

#endif //_CLIENT_PROFILE_MANAGER_H

