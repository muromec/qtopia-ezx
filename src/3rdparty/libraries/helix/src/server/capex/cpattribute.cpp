#include "hxtypes.h"
#include "hxcom.h"

#include "hxclientprofile.h"
#include "cpattribute.h"

CPAttribute::CPAttribute()
    : m_ulRefCount(0)
    , m_ulAttrType(HX_CP_TYPE_UNKNOWN)
    , m_pAttrRef(NULL)
{
}

CPAttribute::~CPAttribute()
{ 
    HX_RELEASE(m_pAttrRef);
}

STDMETHODIMP
CPAttribute::QueryInterface(REFIID riid, void** ppvObj) 
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCPAttribute))
    {
        AddRef();
        *ppvObj = (IHXCPAttribute*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CPAttribute::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CPAttribute::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0) 
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}
