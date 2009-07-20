/* 
 * $Id: hxclientprofile.h,v 1.8 2004/05/21 19:53:25 jgordon Exp $
 */

#ifndef _HX_CLIENT_PROFILE_H_
#define _HX_CLIENT_PROFILE_H_

_INTERFACE IHXClientProfile;
_INTERFACE IHXClientProfileInfo;
_INTERFACE IHXClientProfileManagerResponse;
_INTERFACE IHXPSSProfileData;
_INTERFACE IHXBuffer;
_INTERFACE IHXList;
_INTERFACE IHXValues;
_INTERFACE IHXSessionStats;

enum HXCPComponentID
{
     HX_CP_COMPONENT_UNKNOWN = 0x0
    ,HX_CP_COMPONENT_STREAMING = (0x1<<16)
    ,HX_CP_COMPONENT_HARDWARE =  (0x2<<16)
    ,HX_CP_COMPONENT_SOFTWARE =  (0x3<<16)
    ,HX_CP_LAST_COMPONENT_ID
};

enum HXCPAttributeID
{
     HX_CP_ATTR_UNKNOWN = 0x0
     /* Streaming component*/
    ,HX_CP_ATTR_AUDIO_CH            = (HX_CP_COMPONENT_STREAMING | 0x1)
    ,HX_CP_ATTR_PSS_ACCEPT          = (HX_CP_COMPONENT_STREAMING | 0x2)
    ,HX_CP_ATTR_PSS_ACCEPT_SUBSET   = (HX_CP_COMPONENT_STREAMING | 0x3)
    ,HX_CP_ATTR_PSS_VERSION         = (HX_CP_COMPONENT_STREAMING | 0x4)
    ,HX_CP_ATTR_RND_SCREEN_SIZE     = (HX_CP_COMPONENT_STREAMING | 0x5)
    ,HX_CP_ATTR_VDEC_BYTE_RATE      = (HX_CP_COMPONENT_STREAMING | 0x6)
    ,HX_CP_ATTR_VPOSTDEC_BUF_PERIOD = (HX_CP_COMPONENT_STREAMING | 0x7)
    ,HX_CP_ATTR_VPREDEC_BUF_SIZE    = (HX_CP_COMPONENT_STREAMING | 0x8)
    ,HX_CP_LAST_STREAMING_ATTR_ID
    
     /* Hardware Platform component */
    ,HX_CP_ATTR_COLOR_CAP       = (HX_CP_COMPONENT_HARDWARE | 0x1)
    ,HX_CP_ATTR_IMAGE_CAP       = (HX_CP_COMPONENT_HARDWARE | 0x2)
    ,HX_CP_ATTR_SOUND_CAP       = (HX_CP_COMPONENT_HARDWARE | 0x3)
    ,HX_CP_ATTR_SCREEN_SIZE     = (HX_CP_COMPONENT_HARDWARE | 0x4)
    ,HX_CP_ATTR_MODEL           = (HX_CP_COMPONENT_HARDWARE | 0x5)
    ,HX_CP_ATTR_VENDOR          = (HX_CP_COMPONENT_HARDWARE | 0x6)
    ,HX_CP_LAST_HARDWARE_ATTR_ID
     
    /* Software Platform component */
    ,HX_CP_ATTR_CCPP_ACPT       = (HX_CP_COMPONENT_SOFTWARE | 0x1) 
    ,HX_CP_ATTR_CCPP_ACPT_LANG  = (HX_CP_COMPONENT_SOFTWARE | 0x2) 
    ,HX_CP_LAST_SOFTWARE_ATTR_ID
};

enum HXCPAttributeType
{
    HX_CP_TYPE_UNKNOWN,
    HX_CP_TYPE_INT,
    HX_CP_TYPE_BOOL,
    HX_CP_TYPE_STR,
    HX_CP_TYPE_DIM,

    HX_CP_TYPE_LST,
    HX_CP_LAST_ATTR_TYPE
};

enum HXCPAttributeMergeRule
{
    HX_CP_MR_UNKNOWN,
    HX_CP_MR_LCK,
    HX_CP_MR_OVR,
    HX_CP_MR_APND,
    HX_CP_LAST_MR
};

typedef UINT32 HXCPDimension[2];
typedef union _HXCPAttribute
{
    UINT32          ulInt;
    BOOL            bBool;
    IHXBuffer*      pString;
    HXCPDimension   cpDimension;
    IHXList*        pList;
} HXCPAttribute;

#define HX_CP_COMPONENT_MASK 0xFFFF0000
#define HX_CP_ATTRIBUTE_MASK 0x0FFFF

#define HX_CP_NUM_COMPONENTS (HIWORD(HX_CP_LAST_COMPONENT_ID) + 1)
#define HX_CP_NUM_STREAMING_ATTRS \
    ((HX_CP_LAST_STREAMING_ATTR_ID - 1) - HX_CP_COMPONENT_STREAMING)
#define HX_CP_NUM_HARDWARE_ATTRS \
    ((HX_CP_LAST_HARDWARE_ATTR_ID - 1) - HX_CP_COMPONENT_HARDWARE)
#define HX_CP_NUM_SOFTWARE_ATTRS \
    ((HX_CP_LAST_SOFTWARE_ATTR_ID - 1) - HX_CP_COMPONENT_SOFTWARE)

// XXXJDG FIXME this is really stupid
#define HX_CP_GET_NUM_ATTRS(ComponentId) \
((ComponentId) == HX_CP_COMPONENT_STREAMING ? HX_CP_NUM_STREAMING_ATTRS : \
((ComponentId) == HX_CP_COMPONENT_HARDWARE ? HX_CP_NUM_HARDWARE_ATTRS : \
((ComponentId) == HX_CP_COMPONENT_SOFTWARE ? HX_CP_NUM_SOFTWARE_ATTRS : \
0)))

#define HX_CP_GET_COMPONENT_INDEX(ComponentId) (HIWORD(ComponentId))
#define HX_CP_GET_ATTRIBUTE_INDEX(AttributeId) (LOWORD(AttributeId))

#define HX_CP_GET_NEXT_COMPONENT_ID(ComponentId) \
    ((ComponentId) + (0x1 << 16))

#define HX_CP_GET_FIRST_ATTRIBUTE_ID(ComponentId) \
    ((ComponentId) | 0x01)
#define HX_CP_GET_NEXT_ATTRIBUTE_ID(AttributeId) \
    ((AttributeId) + 1)
#define HX_CP_GET_LAST_ATTRIBUTE_ID(ComponentId) \
    ((ComponentId) | HX_CP_GET_NUM_ATTRS(ComponentId))

#define HX_CP_GET_COMPONENT_ID_FROM_ATTRIBUTE(AttributeId) \
    ((AttributeId) & HX_CP_COMPONENT_MASK)

#define HX_CP_IS_LIST_ATTR(AttributeId) \
    (AttributeId == HX_CP_TYPE_LST)


/***********************************************************************
 * IHXClientProfileManager
 ************************************************************************/
DEFINE_GUID_ENUM(IID_IHXClientProfileManager, 0x00004400, 0x901, 0x11d1,
                 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59)

#define CLSID_IHXClientProfileManager IID_IHXClientProfileManager

#undef  INTERFACE
#define INTERFACE IHXClientProfileManager

DECLARE_INTERFACE_(IHXClientProfileManager, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXClientProfileManager methods
     
    STDMETHOD(GetPSSProfile) (THIS_
                              IHXClientProfileManagerResponse* pResponse,
                              REF(IHXClientProfile*) pClientInfo,
                              IHXSessionStats* pStats,
                              IHXBuffer* pRequestId,
                              IHXBuffer* pRequestURI,
                              IHXValues* pRequestHeaders) PURE;
}; 

/***********************************************************************
 * IHXClientProfileManagerResponse
 ************************************************************************/
DEFINE_GUID_ENUM(IID_IHXClientProfileManagerResponse, 0x00004401, 0x901, 0x11d1,
                 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59)

#define CLSID_IHXClientProfileManagerResponse IID_IHXClientProfileManagerResponse

#undef  INTERFACE
#define INTERFACE IHXClientProfileManagerResponse

DECLARE_INTERFACE_(IHXClientProfileManagerResponse, IUnknown)
{

    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXClientProfileManagerResponse methods
    STDMETHOD(PSSProfileReady)      (THIS_ HX_RESULT ulStatus,
                                      IHXClientProfile* pInfo, 
                                      IHXBuffer* pRequestId,
                                      IHXBuffer* pRequestURI,
                                      IHXValues* pRequestHeaders) PURE;
};

/***********************************************************************
 * IHXClientProfileInfo 
 ************************************************************************/
DEFINE_GUID_ENUM(IID_IHXClientProfileInfo, 0x00004402, 0x901, 0x11d1,
                0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59)

#define CLSID_IHXClientProfileInfo IID_IHXClientProfileInfo

#undef  INTERFACE
#define INTERFACE IHXClientProfileInfo

DECLARE_INTERFACE_(IHXClientProfileInfo, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    // IHXClientProfileInfo methods
    // See HXCPAttributeID above for attribute ID definitions.
    STDMETHOD(GetAttribute)     (THIS_ const UINT32 ulAttributeId,
                                 REF(UINT32) ulAttrType,
                                 REF(HXCPAttribute) attrValue) PURE;
};

/***********************************************************************
 * IHXClientProfile
 ************************************************************************/
DEFINE_GUID_ENUM(IID_IHXClientProfile, 0x00004403, 0x901, 0x11d1,
                 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59)

#define CLSID_IHXClientProfile IID_IHXClientProfile

#undef  INTERFACE
#define INTERFACE IHXClientProfile

DECLARE_INTERFACE_(IHXClientProfile, IHXClientProfileInfo)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    // IHXClientProfileInfo methods
    // See HXCPAttributeID above for attribute ID definitions.
    STDMETHOD(GetAttribute)     (THIS_ const UINT32 ulAttributeId,
                                 REF(UINT32) ulAttrType,
                                 REF(HXCPAttribute) attrValue) PURE;

    // IHXClientProfile methods
    STDMETHOD(SetComponentAttributes)       (THIS_ IHXPSSProfileData* pPrfData,
                                             UINT32 ulComponentId) PURE;
     
    STDMETHOD_(BOOL, IsComponentDefined)    (THIS_ UINT32 ulComponentId) PURE;
    
};

/***********************************************************************
 * IHXCPAttribute
 ************************************************************************/
DEFINE_GUID_ENUM(IID_IHXCPAttribute, 0x00004404, 0x901, 0x11d1,
                 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59)

#define CLSID_IHXCPAttribute IID_IHXCPAttribute

#undef  INTERFACE
#define INTERFACE IHXCPAttribute

DECLARE_INTERFACE_(IHXCPAttribute, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXCPAttribute methods
    STDMETHOD(Get)                  (THIS_ REF(UINT32) ulAttrType,
                                     REF(HXCPAttribute) attrValue) PURE;
    STDMETHOD_(UINT32, GetType)     (THIS) PURE;
    STDMETHOD_(HXCPAttribute, GetValue) (THIS) PURE;
};

#endif /* ndef _HX_CLIENT_PROFILE_H */
