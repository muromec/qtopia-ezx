/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxpckts.h,v 1.17 2007/03/07 13:58:56 ehyche Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#ifndef _HXPCKTS_H_

#include "hxcppflags.h"
#ifndef HX_CPP_BASIC_TEMPLATES
#include "old_hxpckts.h"
#else

#define _HXPCKTS_H_

#include "assocvector.h"
#include "unkimp.h"

#include "hxtypes.h"
#include "hlxclib/string.h"
#include "hxmap.h"
#include "ihxpckts.h"
#include "hxbuffer.h" /* For all the places that just include this */

struct HXPacketInfo
{
    UINT8   lost;
    UINT8   ASMFlags;
    UINT16  ASMRuleNumber;
    UINT16  streamNum;
    UINT32  time;
};

/****************************************************************************
 * 
 *        Class:
 *
 *                CHXPacket
 *
 *        Purpose:
 *
 *                PN implementation of a basic packet.
 *
 */
class CHXPacket : public IHXRTPPacket
{
protected:
    LONG32                m_lRefCount;
    IHXBuffer*                m_pBuffer;
    ULONG32                m_ulTime;
    UINT16                m_uStreamNumber;        
    UINT8                m_unASMFlags;
    UINT16                m_unASMRuleNumber;
    HXBOOL                m_bIsLost;
    
    virtual ~CHXPacket()
    {
        if (m_pBuffer)
        {
            m_pBuffer->Release();
        }
    };
    
    inline HX_RESULT _Get(
        IHXBuffer*& pBuffer, 
        ULONG32 &ulTime,
        UINT16 &uStreamNumber,
        UINT8 &unASMFlags,
        UINT16 &unASMRuleNumber);

    inline HX_RESULT _Set(
           IHXBuffer* pBuffer, 
           ULONG32 ulTime,
           UINT16 uStreamNumber,
           UINT8 unASMFlags,
           UINT16 unASMRuleNumber);

public:
    CHXPacket()
        : m_lRefCount(0)
        , m_pBuffer(NULL)
        , m_ulTime(0)
        , m_uStreamNumber(0)
        , m_unASMFlags(0)
        , m_unASMRuleNumber(0)
        , m_bIsLost(FALSE)
    {
        ;
    }

    /*
     *        IUnknown methods
     */
    STDMETHOD(QueryInterface)        (THIS_
                                REFIID riid,
                                void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)        (THIS);

    STDMETHOD_(ULONG32,Release)        (THIS);

    /*
     *        IHXPacket methods
     */
    STDMETHOD(Get)  (THIS_
                    REF(IHXBuffer*)        pBuffer, 
                    REF(ULONG32)                ulTime,
                    REF(UINT16)                uStreamNumber,
                    REF(UINT8)                unASMFlags,
                    REF(UINT16)                unASMRuleNumber);

    STDMETHOD_(IHXBuffer*,GetBuffer)                (THIS);

    STDMETHOD_(ULONG32,GetTime)                        (THIS);

    STDMETHOD_(UINT16,GetStreamNumber)                (THIS);

    STDMETHOD_(UINT8,GetASMFlags)                (THIS);

    STDMETHOD_(UINT16,GetASMRuleNumber)                (THIS);

    STDMETHOD_(HXBOOL,IsLost)                        (THIS);

    STDMETHOD(SetAsLost)                        (THIS);

    STDMETHOD(Set)                                (THIS_
                                                IHXBuffer*                pBuffer, 
                                                ULONG32                        ulTime,
                                                UINT16                        uStreamNumber,
                                                UINT8                        unASMFlags,
                                                UINT16                        unASMRuleNumber);

    /*
     *        IHXRTPPacket methods
     */
   STDMETHOD(GetRTP)                                (THIS_
                                                REF(IHXBuffer*)        pBuffer, 
                                                REF(ULONG32)                ulTime,
                                                REF(ULONG32)                ulRTPTime,
                                                REF(UINT16)                uStreamNumber,
                                                REF(UINT8)                unASMFlags,
                                                REF(UINT16)                unASMRuleNumber);

   STDMETHOD_(ULONG32,GetRTPTime)                (THIS);

   STDMETHOD(SetRTP)                                (THIS_
                                                IHXBuffer*                pBuffer, 
                                                ULONG32                        ulTime,
                                                ULONG32                        ulRTPTime,
                                                UINT16                        uStreamNumber,
                                                UINT8                        unASMFlags,
                                                UINT16                        unASMRuleNumber);

   // serialization method
   static void        Pack        (IHXPacket* pPacket, char* pData, UINT32& ulSize, IUnknown* pContext);
   static void        UnPack        (IHXPacket*& pPacket, char* pData, UINT32 ulSize, IUnknown* pContext);
};

/****************************************************************************
 * 
 *        Class:
 *
 *                CHXRTPPacket
 *
 *        Purpose:
 *
 *                PN implementation of a RTP packet.
 *
 */
class CHXRTPPacket : public CHXPacket
{
protected:
    ULONG32                        m_ulRTPTime;

    virtual ~CHXRTPPacket()
    {
        ;
    }

public:
    CHXRTPPacket()
        : m_ulRTPTime(0)
    {
        ;
    }

    /*
     *        IUnknown methods - override
     */
    STDMETHOD(QueryInterface)        (THIS_
                                REFIID riid,
                                void** ppvObj);

    STDMETHOD_(ULONG32,Release)        (THIS);

    /*
     *        IHXPacket methods - override
     */
    STDMETHOD(Set)                                (THIS_
                                                IHXBuffer*                pBuffer, 
                                                ULONG32                        ulTime,
                                                UINT16                        uStreamNumber,
                                                UINT8                        unASMFlags,
                                                UINT16                        unASMRuleNumber);

    /*
     *        IHXRTPPacket methods - override
     */
    STDMETHOD(GetRTP)                                (THIS_
                                                REF(IHXBuffer*)        pBuffer, 
                                                REF(ULONG32)                ulTime,
                                                REF(ULONG32)                ulRTPTime,
                                                REF(UINT16)                uStreamNumber,
                                                REF(UINT8)                unASMFlags,
                                                REF(UINT16)                unASMRuleNumber);

    STDMETHOD_(ULONG32,GetRTPTime)                (THIS);

    STDMETHOD(SetRTP)                                (THIS_
                                                IHXBuffer*                pBuffer, 
                                                ULONG32                        ulTime,
                                                ULONG32                        ulRTPTime,
                                                UINT16                        uStreamNumber,
                                                UINT8                        unASMFlags,
                                                UINT16                        unASMRuleNumber);
};

class CHXMultiPayloadPacket : public IHXMultiPayloadPacket
{
public:
    CHXMultiPayloadPacket();

    /* IUnknown methods */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    /* IHXPacket methods */
    STDMETHOD(Get)                      (THIS_ REF(IHXBuffer*) rpBuffer, 
                                               REF(UINT32)     rulTime,
                                               REF(UINT16)     rusStreamNumber,
                                               REF(UINT8)      rucASMFlags,
                                               REF(UINT16)     rusASMRuleNumber);
    STDMETHOD_(IHXBuffer*,GetBuffer)    (THIS);
    STDMETHOD_(ULONG32,GetTime)         (THIS);
    STDMETHOD_(UINT16,GetStreamNumber)  (THIS);
    STDMETHOD_(UINT8,GetASMFlags)       (THIS);
    STDMETHOD_(UINT16,GetASMRuleNumber) (THIS);
    STDMETHOD_(HXBOOL,IsLost)           (THIS);
    STDMETHOD(SetAsLost)                (THIS);
    STDMETHOD(Set)                      (THIS_ IHXBuffer* pBuffer,
                                               UINT32     ulTime,
                                               UINT16     usStreamNumber,
                                               UINT8      ucASMFlags,
                                               UINT16     usASMRuleNumber);

    /* IHXMultiPayloadPacket methods */
    STDMETHOD_(UINT32,GetNumPayloads) (THIS);
    STDMETHOD(SetNumPayloads)         (THIS_ UINT32 ulNum);
    STDMETHOD(GetPayloadInfo)         (THIS_ UINT32 i, REF(UINT32) rulOffset, REF(UINT32) rulSize);
    STDMETHOD(SetPayloadInfo)         (THIS_ UINT32 i, UINT32 ulOffset, UINT32 ulSize);
protected:
    typedef struct _HXPayloadInfo
    {
        UINT32 m_ulOffset;
        UINT32 m_ulSize;
    }
    HXPayloadInfo;

    INT32          m_lRefCount;
    CHXPacket*     m_pPacket;
    UINT32         m_ulNumPayloads;
    HXBOOL         m_bPayloadInfoValid;
    HXPayloadInfo* m_pPayload;

    virtual ~CHXMultiPayloadPacket();
};

class _CStoreName
{
public:
    _CStoreName();
    virtual ~_CStoreName();

    const CHXString& GetName() const;
    void SetName(const char* szName);
private:
    CHXString m_strName;
};

class _CStoreNameUINT32Pair : public _CStoreName
{
public:
    _CStoreNameUINT32Pair();
    ~_CStoreNameUINT32Pair();

    UINT32 GetValue();
    void SetValue(UINT32 ulValue);
private:
    UINT32 m_ulValue;
};

class _CStoreNameBufferPair : public _CStoreName
{
public:
    _CStoreNameBufferPair();
    ~_CStoreNameBufferPair();

    IHXBuffer* GetValue();
    void SetValue(IHXBuffer* pbufValue);
private:
    IHXBuffer* m_pbufValue;
};

/****************************************************************************
 * 
 *        Class:
 *
 *                CHXHeader
 *
 *        Purpose:
 *
 *                PN implementation of a basic header.
 *
 */

class CHXHeader : public CUnknownIMP,
                  public IHXValues2,
		  public IHXValues3,
                  public IHXValuesRemove
{

protected:
    struct CValue
    {
        UINT32 m_lVal;
        HXBOOL m_lValValid;
        IHXBuffer* m_pIBuffer;
        IHXBuffer* m_pIString;
        IUnknown*  m_pIObject;
        const char* m_Key;

        CValue() : m_lValValid(FALSE), m_pIBuffer(NULL), 
            m_pIString(NULL), m_pIObject(NULL), m_Key(NULL)
        {
        }

        CValue(CValue const& rhs)
        :   m_lVal(rhs.m_lVal), 
            m_lValValid(rhs.m_lValValid), 
            m_pIBuffer(rhs.m_pIBuffer),
            m_pIString(rhs.m_pIString),
            m_pIObject(rhs.m_pIObject),
            m_Key(rhs.m_Key)
        {
            if (m_pIBuffer) m_pIBuffer->AddRef();
            if (m_pIString) m_pIString->AddRef();
            if (m_pIObject) m_pIObject->AddRef();
        }
        CValue& operator= (CValue const&);
        ~CValue();

        HX_RESULT Get(IUnknown* CValue::*pMem, IUnknown*& value) const
        {
            if (!(this->*pMem)) return HXR_FAIL;
            value = this->*pMem;
            value->AddRef ();
            return HXR_OK;
        }

        HX_RESULT Get(IHXBuffer* CValue::*pMem, IHXBuffer*& value) const
        {
            if (!(this->*pMem)) return HXR_FAIL;
            value = this->*pMem;
            value->AddRef ();
            return HXR_OK;
        }
        
        HX_RESULT Get(UINT32 CValue::*pMem, UINT32& value) const
        {
            HX_ASSERT(pMem == &CValue::m_lVal);
            if (!m_lValValid) return HXR_FAIL;
            value = m_lVal;
            return HXR_OK;
        }
        
        void Set(IUnknown* CValue::*pMem, IUnknown* value)
        {
            HX_ASSERT(value);
            HX_RELEASE (this->*pMem);
            this->*pMem = value;
            value->AddRef();
        }

        void Set(IHXBuffer* CValue::*pMem, IHXBuffer* value)
        {
            HX_ASSERT(value);
            HX_RELEASE (this->*pMem);
            this->*pMem = value;
            value->AddRef();
        }
        
        void Set(UINT32 CValue::*pMem, UINT32 value)
        {
                m_lVal = value;
                m_lValValid = TRUE;
        }

        void Remove(IUnknown* CValue::*pMem)
        {
            HX_RELEASE (this->*pMem);
        }

	void Remove(IHXBuffer* CValue::*pMem)
        {
            HX_RELEASE (this->*pMem);
        }
        
        void Remove(UINT32 CValue::*pMem)
        {
	    m_lValValid = FALSE;
        }

	HXBOOL IsSet(void)
	{
	    return (m_lValValid || m_pIString || m_pIBuffer || m_pIObject);
	}
        
        void Swap(CValue& rhs)
        {
            std::swap(m_lVal, rhs.m_lVal);
            std::swap(m_lValValid, rhs.m_lValValid);
            std::swap(m_pIBuffer, rhs.m_pIBuffer);
            std::swap(m_pIString, rhs.m_pIString);
            std::swap(m_pIObject, rhs.m_pIObject);
            std::swap(m_Key, rhs.m_Key);
        }
    };

    struct MyCompare : public std::binary_function<const char*, const char*, bool>
    {
        char m_PreserveCase;
        
        explicit MyCompare(char PreserveCase) : m_PreserveCase(PreserveCase) 
        {
        }

        bool operator()(const char* p1, const char* p2) const;
    };
    
    typedef Loki::AssocVector<const char*, CValue, MyCompare> Map;
    //typedef std::map<std::string, CValue/*, MyCompare*/> Map;

    Map m_Map;

    template <typename TVal>
    HX_RESULT GetProperty(
        const char* pPropertyName,
        TVal CValue::*pMem,
        TVal& value)
    {
        HX_ASSERT(pPropertyName);
        //HX_ASSERT(pMem);
        Map::const_iterator i = m_Map.find(pPropertyName);
        return (i == m_Map.end()) ? HXR_FAIL : i->second.Get(pMem, value);
    }
    
    template <typename TVal>
    HX_RESULT SetProperty(
            const char* pPropertyName,
            TVal CValue::*pMem,
            TVal value)
    {
        HX_ASSERT(pPropertyName);

        Map::iterator i = m_Map.find(pPropertyName);
        CValue* pVal; // uninitialized
        if (i == m_Map.end())
        {
            char* pTmp = new char[strlen(pPropertyName)+1];
            strcpy(pTmp, pPropertyName); /* Flawfinder: ignore */
	    pVal = &m_Map[pTmp];
            pVal->m_Key = pTmp;
        }
        else
        {
            pVal = &i->second;
        }

        pVal->Set(pMem, value);
        
        return HXR_OK;
    }
    
    template <typename TVal>
    HX_RESULT GetNextProperty(
            const char*& pPropertyName,
            TVal CValue::*pMem,
            TVal& value,
            Map::const_iterator& iter)
    {
            for (; iter != m_Map.end(); ++iter)
            {
                    if (SUCCEEDED(iter->second.Get(pMem, value)))
                    {
                            pPropertyName = iter->second.m_Key;
                            return HXR_OK;
                    }
            }
            return HXR_FAIL;
    }

    template <typename TVal>
    HX_RESULT RemoveProperty(
            const char* pPropertyName,
            TVal CValue::*pMem)
    {
        HX_ASSERT(pPropertyName);

        Map::iterator i = m_Map.find(pPropertyName);
        CValue* pVal; // uninitialized
        if (i != m_Map.end())
        {
	    // Entry exists
	    pVal = &i->second;	 // Get Handle on CValue
	    pVal->Remove(pMem);  // Remove one of ULONG32, Buffer or CString
	    /* Take the greedy approach and try to free up memory as
	     * soon as possible.
	     * If something is still set, we cannot delete the entry.
	     */
	    if (!pVal->IsSet())	 
	    {
		delete[] (char*)i->first;
		m_Map.erase(i);
	    }
        }
        
        return HXR_OK;
    }


        Map::const_iterator 
                m_ULONG32Position,
                m_BufferPosition,
                m_CStringPosition,
                m_ObjectPosition;

        virtual ~CHXHeader();


protected:
#if 0
// function to encode CString property escape sequences
        IHXBuffer*                encodeCString(IHXBuffer* pBuffer);

// function to encode buffer property int MIME Base64
        IHXBuffer*                encodeBuffer(IHXBuffer* pBuffer);
#endif

public:
        CHXHeader()
                : m_Map(MyCompare(FALSE)) // case-insensitive by default
        {
                m_ULONG32Position = m_BufferPosition = m_CStringPosition = m_Map.end();
        }

    /*
     *        IUnknown methods
     */
    DECLARE_UNKNOWN(CHXHeader)

    /*
     *        IHXValues methods
     */

    STDMETHOD(SetPropertyULONG32)        (THIS_
                                        const char*         pPropertyName,
                                        ULONG32                 uPropertyValue);

    STDMETHOD(GetPropertyULONG32)        (THIS_
                                        const char*         pPropertyName,
                                        REF(ULONG32)         uPropertyValue);

    STDMETHOD(GetFirstPropertyULONG32)        (THIS_
                                        REF(const char*) pPropertyName,
                                        REF(ULONG32)         uPropertyValue);

    STDMETHOD(GetNextPropertyULONG32)        (THIS_
                                        REF(const char*) pPropertyName,
                                        REF(ULONG32)         uPropertyValue);


    STDMETHOD(SetPropertyBuffer)        (THIS_
                                        const char*         pPropertyName,
                                        IHXBuffer*         pPropertyValue);

    STDMETHOD(GetPropertyBuffer)        (THIS_
                                        const char*         pPropertyName,
                                        REF(IHXBuffer*) pPropertyValue);

#if 0
    STDMETHOD(GetEncodedPropertyBuffer)        (THIS_
                                        const char*         pPropertyName,
                                        REF(IHXBuffer*) pPropertyValue);
#endif

    STDMETHOD(GetFirstPropertyBuffer)        (THIS_
                                        REF(const char*) pPropertyName,
                                        REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(GetNextPropertyBuffer)        (THIS_
                                        REF(const char*) pPropertyName,
                                        REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(SetPropertyCString)        (THIS_
                                        const char*         pPropertyName,
                                        IHXBuffer*         pPropertyValue);

    STDMETHOD(GetPropertyCString)        (THIS_
                                        const char*         pPropertyName,
                                        REF(IHXBuffer*) pPropertyValue);

#if 0
    STDMETHOD(GetEncodedPropertyCString)(THIS_
                                        const char*         pPropertyName,
                                        REF(IHXBuffer*) pPropertyValue);
#endif

    STDMETHOD(GetFirstPropertyCString)        (THIS_
                                        REF(const char*) pPropertyName,
                                        REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(GetNextPropertyCString)        (THIS_
                                        REF(const char*) pPropertyName,
                                        REF(IHXBuffer*) pPropertyValue);

    /*
     *	IHXValues2 methods
     */

    STDMETHOD(SetPropertyObject) (THIS_
                                 const char* pPropertyName,
                                 IUnknown* pPropertyValue);

    STDMETHOD(GetPropertyObject) (THIS_
                                  const char* pPropertyName,
                                  REF(IUnknown*) pPropertyValue);

    STDMETHOD(GetFirstPropertyObject) (THIS_
                                       REF(const char*) pPropertyName,
                                       REF(IUnknown*) pPropertyValue);

    STDMETHOD(GetNextPropertyObject) (THIS_
                                      REF(const char*) pPropertyName,
                                      REF(IUnknown*) pPropertyValue);

    /************************************************************************
     *	Method:
     *	    IHXValues3::PreserveCase
     */
    STDMETHOD(PreserveCase)	(HXBOOL bPreserve);

    /*
     *  These RemoveXXX() methods are in the IHXValuesRemove and IHXValues2
     *  interfaces
     */
    STDMETHOD(Remove)		(THIS_ const char* pKey);
    STDMETHOD(RemoveULONG32)	(THIS_ const char* pKey);
    STDMETHOD(RemoveBuffer)	(THIS_ const char* pKey);
    STDMETHOD(RemoveCString)	(THIS_ const char* pKey);
    
    /* RemoveObject() is only in IHXValues2 */
    STDMETHOD(RemoveObject) (const char* pKey);

public:
    static void mergeHeaders
    (
        IHXValues* pIHXValuesDestHeaders, 
        IHXValues* pIHXValuesSourceHeaders
    );

};

#endif
#endif /* _HXPCKTS_H_ */
