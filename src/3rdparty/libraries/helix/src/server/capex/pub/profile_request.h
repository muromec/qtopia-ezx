/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: profile_request.h,v 1.5 2004/05/21 19:53:26 jgordon Exp $
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

#ifndef _PROFILE_REQUEST_H
#define _PROFILE_REQUEST_H

class ClientProfileManager;
class ProfileRequestInfo;

class ProfileRequestHandler : public IHXPSSPTAgentResponse
{
public:
    ProfileRequestHandler(ClientProfileManager* pPrfMgr);
    virtual ~ProfileRequestHandler();
    
    /***********************************************************************
     *  IUnknown methods
     *************************************************************************/
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    /***********************************************************************
     *  IHXPSSPTAgentResponse methods
     *************************************************************************/
    STDMETHOD(ProfileReady)     (THIS_ HX_RESULT  ulStatus,
                                     IHXPSSProfileData* pProfileData,
                                     IHXBuffer* pProfileURI,
                                     IHXBuffer* pRequestId,
                                     IHXValues* pPrfServerRespHeaders);

    /***********************************************************************
     *  Non-interface methods
     *************************************************************************/
    HX_RESULT Init(Process* pProc);
    HX_RESULT SetClientProfile();
    HX_RESULT SetClientProfileFromCache(IUnknown* pContext);
    HX_RESULT SendRequestDone();

    /***********************************************************************
    *  Wrapper classes
    *************************************************************************/
    class RequestInitCB : public SimpleCallback
    {
    public:
        RequestInitCB(ProfileRequestHandler* pReqHandler); 
        ~ RequestInitCB();
        void                func(Process* proc);
    
    private:
        ProfileRequestHandler*   m_pReqHandler;

    friend class ProfileRequestHandler; 
    };

    class RequestDoneCB : public SimpleCallback
    {
    public:
        RequestDoneCB(ProfileRequestHandler* pReqHandler,
                      ClientProfileManager* pPrfMgr);
        ~RequestDoneCB();
        void                func(Process* proc);
    
    private:
        HX_RESULT               m_ulStatus;
        ProfileRequestHandler*  m_pReqHandler;
        ClientProfileManager*   m_pPrfMgr;
    
    friend class ClientProfileManager; 
    friend class ProfileRequestHandler;
    };

    typedef enum
    {
        PRF_REQ_TYPE_CLIENT = 0,
        PRF_REQ_TYPE_STATIC_CACHE
    } ProfileRequestType;
protected:
    UINT32                  m_ulRefCount;

    IUnknown*               m_pContext;
    ProfileRequestType      m_ulRequestType;
    IHXBuffer*              m_pRequestId;
    IHXBuffer*              m_pRequestURI;
    IHXBuffer*              m_pCacheKey;
    UINT32                  m_ulCacheMergeRule;
    IHXValues*              m_pRequestHeaders; 
    HX_RESULT               m_ulRequestStatus;
    CHXSimpleList*          m_pPrfReqInfoList;
    IHXList*                m_pParsedPrfDataList;
    IHXPSSProfileData*      m_pDefaultProfile;
    ProfileRequestInfo*     m_pCurrPrfInfo;
    IHXBuffer*              m_pCurrPrfDiff;
    RequestInitCB*          m_pReqInitCB;
    RequestDoneCB*          m_pReqDoneCB;
    IHXClientProfile*       m_pClientProfile;
    IHXSessionStats*        m_pSessionStats;
    ClientProfileManager*   m_pPrfMgr;
    IHXProfileCache*        m_pCache;
    IHXClientProfileManagerResponse* m_pPrfMgrResp;
   
    friend class ClientProfileManager;
};

class ProfileRequestInfo
{
public:
    ProfileRequestInfo();
    ~ProfileRequestInfo();

    void SetProfileURI(IHXBuffer* pProfileURI);
    void AddProfileDiff(IHXBuffer* pProfileDiff);
    void SetProfileData(IHXPSSProfileData* pProfileData);
private:
    BOOL            m_bProfileURICached;
    IHXBuffer*      m_pProfileURI;
    CHXSimpleList*  m_pProfileDiffList;
    IHXPSSProfileData* m_pProfileData;
    friend class ProfileRequestHandler;
    friend class ClientProfileManager;
};
 



#endif //_PROFILE_REQUEST_H

