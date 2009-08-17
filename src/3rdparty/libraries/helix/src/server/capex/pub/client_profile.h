/* 
 * $Id: client_profile.h,v 1.3 2003/07/07 22:11:11 jgordon Exp $
 */

#ifndef _CLIENT_PROFILE_H
#define _CLIENT_PROFILE_H

_INTERFACE IHXPSSProfileData;

class ClientProfile:  public IHXClientProfile
{
public:

    ClientProfile();
    virtual ~ClientProfile();

    /***********************************************************************
     * IUnknown methods
     ***********************************************************************/
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    /***********************************************************************
     * IHXClientProfileInfo methods
     ***********************************************************************/
    STDMETHOD(GetAttribute)             (THIS_ const UINT32 ulAttributeId,
                                         REF(UINT32) ulAttrTrype,
                                         REF(HXCPAttribute) attrValue);

    /***********************************************************************
     * IHXClientProfile methods
     ***********************************************************************/
    STDMETHOD(SetComponentAttributes)       (THIS_ IHXPSSProfileData* pPrfData,
                                             UINT32 ulComponentId);
 
    STDMETHOD_(BOOL, IsComponentDefined)    (THIS_ UINT32 ulComponentId);

protected:
    inline HX_RESULT GetComponentData(IHXPSSProfileData*& pPrfData,
                                      UINT32& ulCompId,
                                      UINT32 ulAttributeId);
    UINT32 m_ulRefCount;
    IHXPSSProfileData** m_pPSSPrfComponents;
};

HX_RESULT
ClientProfile::GetComponentData(IHXPSSProfileData*& pPrfData,
                                UINT32& ulCompId,
                                UINT32 ulAttributeId)
{
    ulCompId = HX_CP_GET_COMPONENT_ID_FROM_ATTRIBUTE(ulAttributeId);
    UINT32 ulCompIndex = HX_CP_GET_COMPONENT_INDEX(ulCompId);

    if (ulCompIndex >=  HX_CP_NUM_COMPONENTS)
    {
        return HXR_INVALID_PARAMETER;
    }

    if (!m_pPSSPrfComponents || 
        !m_pPSSPrfComponents[ulCompIndex])
    {
        return HXR_FAIL;
    }

    pPrfData = m_pPSSPrfComponents[ulCompIndex];
    return HXR_OK;
}

#endif
