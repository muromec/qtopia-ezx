/* 
 * $Id: client_profile.cpp,v 1.2 2003/07/07 22:11:11 jgordon Exp $
 *   
 */

#include "hxcom.h"
#include "hxtypes.h"
#include "hxassert.h"

#include "hxcomm.h"
#include "hxerror.h"
#include "hxmon.h"
#include "hxstrutl.h"
#include "ihxpckts.h"
#include "ihxlist.h"
#include "dict.h"

#include "debug.h"
#include "hxclientprofile.h"
#include "hxpssprofile.h"
#include "client_profile.h"

#if defined(HELIX_FEATURE_SERVER_CAPEX)
#include "pss_profile_data.h"
#endif

ClientProfile::ClientProfile()
: m_ulRefCount(0)
, m_pPSSPrfComponents(NULL)
{
    m_pPSSPrfComponents = new IHXPSSProfileData*[HX_CP_NUM_COMPONENTS];
    for (UINT32 ulIndx = 0; ulIndx < HX_CP_NUM_COMPONENTS; ++ulIndx)
    {
        m_pPSSPrfComponents[ulIndx] = NULL;
    }
}

ClientProfile::~ClientProfile()
{
    for (UINT32 ulIndx = 0; ulIndx < HX_CP_NUM_COMPONENTS; ++ulIndx)
    {
        HX_RELEASE(m_pPSSPrfComponents[ulIndx]);
    }
    delete[] m_pPSSPrfComponents; 
    m_pPSSPrfComponents = NULL;
}

STDMETHODIMP
ClientProfile::QueryInterface(REFIID riid, void** ppvObj) 
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this; 
        return HXR_OK; 
    }
    else if (IsEqualIID(riid, IID_IHXClientProfile))
    {
        AddRef();
        *ppvObj = (IHXClientProfile*)this;
        return HXR_OK; 
    }
    else if (IsEqualIID(riid, IID_IHXClientProfileInfo))
    {
        AddRef();
        *ppvObj = (IHXClientProfileInfo*)this;
        return HXR_OK; 
    }

    *ppvObj = NULL; 
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
ClientProfile::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
ClientProfile::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0) 
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
ClientProfile::GetAttribute(const UINT32 ulAttributeId, 
                            REF(UINT32) ulAttrType,
                            REF(HXCPAttribute) attrValue)
{
    IHXPSSProfileData* pData = NULL;
    UINT32 ulComponentId = 0;
    HX_RESULT rc = GetComponentData(pData, ulComponentId, ulAttributeId);
    if (SUCCEEDED(rc))
    {
        rc = pData->GetAttribute(ulComponentId, ulAttributeId, ulAttrType,
                                 attrValue);
    }
    return rc;
}

STDMETHODIMP_(BOOL)
ClientProfile::IsComponentDefined(UINT32 ulComponentId)
{
    BOOL rc = FALSE;

    UINT32 ulComponentIndex = HX_CP_GET_COMPONENT_INDEX(ulComponentId);
    if (ulComponentIndex >=  HX_CP_NUM_COMPONENTS)
    {
        return HXR_INVALID_PARAMETER;
    }
    
    if (m_pPSSPrfComponents && 
        m_pPSSPrfComponents[ulComponentIndex])
    {
        rc = TRUE;
    }
    return rc;
}

STDMETHODIMP
ClientProfile::SetComponentAttributes(IHXPSSProfileData* pPrfData,
                                      UINT32 ulComponentId)
{
    if (!m_pPSSPrfComponents || !pPrfData)
    {
        return HXR_FAIL;
    }

    UINT32 ulIndex = HX_CP_GET_COMPONENT_INDEX(ulComponentId);
    if (ulIndex >=  HX_CP_NUM_COMPONENTS)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_pPSSPrfComponents[ulIndex]);
    m_pPSSPrfComponents[ulIndex] = pPrfData;
    pPrfData->AddRef();
    return HXR_OK;
}


