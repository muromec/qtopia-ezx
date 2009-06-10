#ifndef _CPATTRIBUTE_H_
#define _CPATTRIBUTE_H_

class CPAttribute : public IHXCPAttribute
{
public:
    CPAttribute();
    virtual ~CPAttribute();

    /***********************************************************************
     * IUnknown methods
     ***********************************************************************/
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    /***********************************************************************
     * IHXCPAttribute methods
     ***********************************************************************/
    inline STDMETHOD(Get)               (THIS_ REF(UINT32) ulAttrType,
                                         REF(HXCPAttribute) attrValue);
    inline STDMETHOD_(UINT32, GetType)  (THIS) { return m_ulAttrType; }
    inline STDMETHOD_(HXCPAttribute, GetValue)  (THIS);

    inline HX_RESULT Set                (const UINT32 ulAttrType,
                                         const HXCPAttribute attrValue);

private:
    UINT32          m_ulRefCount;
    HXCPAttribute   m_Attr;
    UINT32          m_ulAttrType;
    IUnknown*       m_pAttrRef;
};

inline STDMETHODIMP_(HXCPAttribute)
CPAttribute::GetValue(void)
{
    HX_ADDREF(m_pAttrRef);
    return m_Attr; 
}

inline STDMETHODIMP
CPAttribute::Get(REF(UINT32) ulAttrType, REF(HXCPAttribute) attrValue)
{
    HX_ADDREF(m_pAttrRef);
    ulAttrType = m_ulAttrType;
    attrValue = m_Attr;

    return HXR_OK;
}

inline HX_RESULT
CPAttribute::Set(const UINT32 ulAttrType, const HXCPAttribute attrValue)
{
    m_ulAttrType = ulAttrType;
    m_Attr = attrValue;

    switch(m_ulAttrType)
    {
        case HX_CP_TYPE_STR:
        {
            m_pAttrRef = (IUnknown*)m_Attr.pString;
            HX_ADDREF(m_pAttrRef);
        }
        case HX_CP_TYPE_LST:
        {
            m_pAttrRef = (IUnknown*)m_Attr.pList;
            HX_ADDREF(m_pAttrRef);
        }
    }
    return HXR_OK;
}

#endif /* _CPATTRIBUTE_H_ */
