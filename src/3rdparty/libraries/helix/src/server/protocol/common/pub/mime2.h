/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mime2.h,v 1.3 2006/04/04 20:00:01 jgordon Exp $ 
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
#ifndef _MIME2_H_
#define _MIME2_H_

#include "hxrtsp2.h"
#include "hxcomm.h"    // IHXFastAlloc


/*
 * Simple MIME headers:
 *   "CSeq: 5"
 *   "Content-Type: application/sdp"
 *
 * Non-simple MIME headers:
 *   "Foo:"
 *   "Transport: rdt;cp=6970;mode=play,rtp;cp=6970;mode=play"
 *
 *   In the last example, the fields are "rdt;client_port=6970;mode=play"
 *   and "rtp;client_port=6970;mode=play".  Each field is composed of a
 *   list of parameters.  Each parameter is an attr/value pair.  The value
 *   for any parameter may be NULL (non-existent).  Example:
 *
 *   attr  value
 *   ----  -----
 *   rdt   NULL
 *   cp    6970
 *   mode  play
 */

/*****************************************************************************
 *
 * Implementation
 *
 *****************************************************************************/

class CMIMEParameter : public IHXMIMEParameter
{
public:
    CMIMEParameter(IHXFastAlloc* pFastAlloc);
    virtual ~CMIMEParameter(void);

    void Init(IHXBuffer* pbufParam);
    void Parse(void);

public:
    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    // IHXMIMEParameter
    STDMETHOD_(UINT32,GetSize)          (THIS);
    STDMETHOD_(UINT32,Write)            (THIS_ BYTE* pbuf);
    STDMETHOD(Get)                      (THIS_ REF(IHXBuffer*) pbufAttr, REF(IHXBuffer*) pbufVal);
    STDMETHOD(Set)                      (THIS_ IHXBuffer* pbufAttr, IHXBuffer* pbufVal);

    FAST_CACHE_MEM

protected:
    ULONG32         m_ulRefCount;
    IHXBuffer*     m_pBufParam;
    IHXBuffer*     m_pBufAttr;
    IHXBuffer*     m_pBufVal;
    IHXFastAlloc*  m_pFastAlloc;
};

/*
 * A MIME field consists of zero or more parameters.  A parameter consists
 * of exactly one attr/value pair.  The value may be NULL (not present) as
 * in "rdt" above.  The parameters may be iterated with the GetFirstParam/
 * GetNextParam methods.
 *
 * Lazy parsing is used for incoming fields.  The first call to
 * GetFirstParam parses the parameters (but does no copying).  All calls to
 * parameter-related methods for incoming fields will fail until
 * GetFirstParam is called.
 *
 * A parameter may be added using InsertParam or AppendParam.  The
 * InsertParam method takes three arguments.  The first is a position
 * marker.  The second and third are the new parameter, which will be
 * inserted after the marker.  If the marker is NULL, it denotes the head of
 * the list.  The "current" parameter is reset to the first parameter.  This
 * method is O(1) if the marker is NULL and O(n) if not.  The AppendParam
 * method appends the new parameter at the tail of the list.  The "current"
 * parameter remains the same.  This method is O(1).
 *
 * A parameter may be removed using RemoveParam or RemoveCurrentParam.  The
 * RemoveParam method takes a pointer to the parameter attribute to be
 * removed.  This method is O(n).  The RemoveCurrentParam method removes the
 * parameter that was last returned in GetFirstParam/GetNextParam.  The new
 * "current" parameter is the next parameter, if any.  This method is O(1).
 *
 * A parameter's value may be changed using SetValue or SetCurrentValue. 
 * The SetValue method takes a pointer to the parameter attribute to be
 * changed.  This method is O(n).  The SetCurrentValue method changes the
 * parameter that was last returned in GetFirstParam/GetNextParam.  This
 * method is O(1).
 */

class CMIMEField : public IHXMIMEField
{
public:
    CMIMEField(IHXFastAlloc* pFastAlloc);
    virtual ~CMIMEField(void);

    void Init(IHXBuffer* pbufField);
    void Parse(void);

    FAST_CACHE_MEM

public:
    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    // IHXMIMEField
    STDMETHOD_(UINT32,GetSize)          (THIS);
    STDMETHOD_(UINT32,Write)            (THIS_ BYTE* pbuf);
    STDMETHOD(GetFirstParam)            (THIS_ REF(IHXMIMEParameter*) pParam);
    STDMETHOD(GetParamList)             (THIS_ REF(IHXList*) plistParams);
    STDMETHOD(GetParamListConst)        (THIS_ REF(IHXList*) plistParams);

protected:
    ULONG32            m_ulRefCount;
    IHXBuffer*        m_pBufField;
    IHXList*          m_pListParams;
    UINT32	       m_ulParamCount;
    IHXMIMEParameter* m_pParam;
    IHXFastAlloc*     m_pFastAlloc;
};

/*
 * A MIME header is a "key: val" pair, eg. "CSeq: 5".  The value consists of
 * zero or more fields. For simple cases, the value is exactly one field
 * with exactly one token and may be retrieved as a specified type with the
 * GetValueAs methods.  For non-simple cases, the value is a list of fields
 * which may be iterated with the GetFirstField/GetNextField methods.
 *
 * Lazy parsing is used for incoming headers.  The first call to
 * GetFirstField parses the fields (but does no copying).  All calls to
 * field-related methods for incoming headers will fail until GetFirstField
 * is called.
 *
 * A field may be added using InsertField or AppendField.  The InsertField
 * method takes two arguments.  The first is a position marker.  The second
 * is the new field, which will be inserted after the marker.  If the marker
 * is NULL, it denotes the head of the list.  The "current" field is reset
 * to the first field.  This method is O(1) if the marker is NULL and O(n)
 * if not.  The AppendField method appends the new field at the tail of the
 * list.  The "current" field remains the same.  This method is O(1).
 *
 * A field may be removed using RemoveField or RemoveCurrentField.  The
 * RemoveField method takes a pointer to the field to be removed.  This
 * method is O(n).  The RemoveCurrentField method removes the field that was
 * last returned in GetFirstField/GetNextField.  The new "current" field is
 * the next field, if any.  This method is O(1).
 */

class CMIMEHeader : public IHXMIMEHeader
{
public:
    CMIMEHeader(IHXFastAlloc* pFastAlloc);
    virtual ~CMIMEHeader(void);

    HX_RESULT   Init(IHXBuffer* pbuHeader);
    HX_RESULT   Init(IHXList* pHeaders);

    FAST_CACHE_MEM

protected:
    void    ParseHeader(void);
    void    ParseFields(void);
    void    ParseFields(IHXBuffer* pBufVal);
    void    ParseHeaderList(IHXList* pHeaders);

public:
    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    // IHXMIMEHeader
    STDMETHOD_(void,ReplaceDelimiters)  (THIS_
    					BOOL bReplaceDelimiters,
					int nReplacementDelimiter);
    STDMETHOD_(UINT32,GetSize)          (THIS);
    STDMETHOD_(UINT32,Write)            (THIS_ BYTE* pbuf);
    STDMETHOD(GetKey)                   (THIS_ REF(IHXBuffer*) pbufKey);
    STDMETHOD(SetKey)                   (THIS_ IHXBuffer* pbufKey);
    STDMETHOD(GetFirstField)            (THIS_ REF(IHXMIMEField*) pField);
    STDMETHOD(GetFieldList)             (THIS_ REF(IHXList*) plistFields);
    STDMETHOD(GetFieldListConst)        (THIS_ REF(IHXList*) plistFields);
    STDMETHOD(GetValueAsInt)            (THIS_ REF(INT32) val);
    STDMETHOD(SetValueFromInt)          (THIS_ INT32 val);
    STDMETHOD(GetValueAsUint)           (THIS_ REF(UINT32) val);
    STDMETHOD(SetValueFromUint)         (THIS_ UINT32 val);
    STDMETHOD(GetValueAsBuffer)         (THIS_ REF(IHXBuffer*) pbufVal);
    STDMETHOD(SetValueFromBuffer)       (THIS_ IHXBuffer* pbufVal);

    STDMETHOD(SetFromString)            (THIS_ const char* szKey,
                                               const char* szVal);
    STDMETHOD(SetFromBuffer)            (THIS_ const char* szKey,
                                               IHXBuffer* pbufVal);

protected:
    ULONG32         m_ulRefCount;
    IHXBuffer*      m_pBufHeader;
    IHXBuffer*      m_pBufKey;
    IHXBuffer*      m_pBufVal;
    IHXList*        m_pListHeaders;
    IHXList*        m_pListFields;
    UINT32	    m_ulFieldCount;
    IHXMIMEField*   m_pField;
    BOOL	    m_bReplaceDelimiters;
    char	    m_nReplacementDelimiter;
    IHXFastAlloc*   m_pFastAlloc;
};

#endif /* ndef _MIME2_H_ */
