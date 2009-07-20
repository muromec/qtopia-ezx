/* 
 * $Id: hxpssprofile.h,v 1.4 2004/05/21 19:53:25 jgordon Exp $
 */

#ifndef _HX_PSS_PROFILE_H_
#define _HX_PSS_PROFILE_H_

_INTERFACE IHXPSSPTAgentResponse; 
_INTERFACE IHXList; 

enum HXCPProfileMergeRule
{
    HX_CP_CMR_NORMAL,   // use normal merge rules
    HX_CP_CMR_OVER,     // override all attributes
    HX_CP_CMR_LOCK,     // lock all attributes
    HX_CP_CMR_IGNORE,   // ignore this profile
    HX_CP_CMR_DOMINATE  // ignore all other profiles
};

/***********************************************************************
 * IHXPSSProfileData
 ************************************************************************/
DEFINE_GUID_ENUM(IID_IHXPSSProfileData, 0x00004410, 0x901, 0x11d1, 0x8b, 0x6,
                 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59)

#define CLSID_IHXPSSProfileData IID_IHXPSSProfileData

#undef  INTERFACE
#define INTERFACE IHXPSSProfileData

DECLARE_INTERFACE_(IHXPSSProfileData, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;


    // IHXPSSProfileData methods
    STDMETHOD(CreateProfile)        (THIS_ IUnknown* pContext,
                                     IHXBuffer* pProfileName, 
                                     IHXBuffer* pProfileURI, 
                                     REF(UINT32) ulProfileId) PURE; 

    STDMETHOD(CreateMergedProfile)  (THIS_ IUnknown* pContext,
                                     IHXList* pProfileList,
                                     UINT32 ulComponentId) PURE;

    STDMETHOD(StoreAttribute)       (THIS_ IHXBuffer* pValue,
                                     const char* pszAttributeName) PURE;

    STDMETHOD(StoreListAttribute)   (THIS_ IHXList* pValue,
                                     const char* pszAttributeName) PURE;

    STDMETHOD(GetAttribute)         (THIS_ UINT32 ulComponentId,
                                     UINT32 ulAttributeId,
                                     REF(UINT32) ulAttrType,
                                     REF(HXCPAttribute) attrValue) PURE;

    STDMETHOD(AddComponent)         (THIS_ IHXBuffer* pComponentName) PURE;

    STDMETHOD_(BOOL,IsComponentDefined)     (THIS_ UINT32 ulComponentId) PURE;

    STDMETHOD(GetProfileURI)                (THIS_ REF(IHXBuffer*) pURI) PURE;

    STDMETHOD_(ULONG32,GetProfileMergeRule) (THIS) PURE;

    STDMETHOD(SetProfileMergeRule)          (THIS_ UINT32 ulMergeRule) PURE;
};


/***********************************************************************
 * IHXPSSPTAgent
 ************************************************************************/
DEFINE_GUID_ENUM(IID_IHXPSSPTAgent, 0x00004411, 0x901, 0x11d1, 0x8b, 0x6,
                 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59)

#define CLSID_IHXPSSPTAgent IID_IHXPSSPTAgent

#undef  INTERFACE
#define INTERFACE IHXPSSPTAgent

DECLARE_INTERFACE_(IHXPSSPTAgent, IUnknown)
{

    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    STDMETHOD(GetProfileFromOrigin)  (THIS_ IHXBuffer* pURI, IHXBuffer* pEtag,  
                                      IHXBuffer* pLastModDate, 
                                      IHXPSSPTAgentResponse* pResponse,
                                      IHXBuffer* pRequestId) PURE;
    STDMETHOD(GetProfileFromBuffer)  (THIS_ IHXBuffer* pProfileBuff,
                                      IHXPSSPTAgentResponse* pResponse,
                                      IHXBuffer* pRequestId) PURE;
};

/***********************************************************************
 * IHXPSSPTAgentResponse 
 ************************************************************************/
DEFINE_GUID_ENUM(IID_IHXPSSPTAgentResponse, 0x00004412, 0x901, 0x11d1, 0x8b,
                0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59)

#define CLSID_IHXPSSPTAgentResponse IID_IHXPSSPTAgentResponse

#undef  INTERFACE
#define INTERFACE IHXPSSPTAgentResponse

DECLARE_INTERFACE_(IHXPSSPTAgentResponse, IUnknown)
{

    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;


    STDMETHOD(ProfileReady)        (THIS_
                                     HX_RESULT  ulStatus,
                                     IHXPSSProfileData* pProfileData,
                                     IHXBuffer* pProfileURI,
                                     IHXBuffer* pRequestId,
                                     IHXValues* pPrfServerRespHeaders) PURE; 
};
#endif /* ndef _HX_PSS_PROFILE_H */
