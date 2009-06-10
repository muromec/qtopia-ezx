/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxvalues.h,v 1.7 2007/07/06 20:35:02 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef _HXVALUES_H_
#define _HXVALUES_H_


// History: The rnvalues interfaces were completely rewritten 10/25/99.

class CSimpleUlongMap
{
public:
    CSimpleUlongMap();
    ~CSimpleUlongMap();

    HX_RESULT SetProperty(const char* pKey, ULONG32 ulValue);
    HX_RESULT GetProperty(const char* pKey, REF(ULONG32) ulValue);
    HX_RESULT GetFirstProperty(REF(const char*) pKey, REF(ULONG32) ulValue);
    HX_RESULT GetNextProperty(REF(const char*) pKey, REF(ULONG32) ulValue);
    void      Remove(const char* pKey);
private:
    virtual int StrCmpFunc(const char* s1, const char* s2) = 0;

    struct node 
    {
	char*        pKey;
	ULONG32      ulValue;
	struct node* pNext;
    };
    struct node *m_pHead;
    struct node *m_pTail;
    struct node *m_pCurr;
};

class CSimpleUlongMapStrCmp : public CSimpleUlongMap
{
private:
    virtual int StrCmpFunc(const char* s1, const char* s2)
    {
	return strcmp(s1,s2);
    }
};

class CSimpleUlongMapStrCaseCmp : public CSimpleUlongMap
{
private:
    virtual int StrCmpFunc(const char* s1, const char* s2)
    {
	return strcasecmp(s1,s2);
    }
};

class CSimpleBufferMap
{
public:
    CSimpleBufferMap();
    ~CSimpleBufferMap();

    HX_RESULT SetProperty(const char* pKey, IHXBuffer* pValue);
    HX_RESULT GetProperty(const char* pKey, REF(IHXBuffer*) pValue);
    HX_RESULT GetFirstProperty(REF(const char*) pKey, REF(IHXBuffer*) pValue);
    HX_RESULT GetNextProperty(REF(const char*) pKey, REF(IHXBuffer*) pValue);
    void      Remove(const char* pKey);
private:
    virtual int StrCmpFunc(const char* s1, const char* s2) = 0;

    struct node 
    {
	char*        pKey;
	IHXBuffer*  pValue;
	struct node* pNext;
    };
    struct node *m_pHead;
    struct node *m_pTail;
    struct node *m_pCurr;
};

class CSimpleBufferMapStrCmp : public CSimpleBufferMap
{
private:
    virtual int StrCmpFunc(const char* s1, const char* s2)
    {
	return strcmp(s1,s2);
    }
};

class CSimpleBufferMapStrCaseCmp : public CSimpleBufferMap
{
private:
    virtual int StrCmpFunc(const char* s1, const char* s2)
    {
	return strcasecmp(s1,s2);
    }
};


class CKeyValueListIter;
class CKeyValueListIterOneKey;

class CKeyValueList : 
	public IHXKeyValueList,
	public IHXValues
{
    // see big comment in hxvalues.cpp
public:
    friend class CKeyValueListIter;
    friend class CKeyValueListIterOneKey;
    CKeyValueList();
    ~CKeyValueList();
    
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);


    // IHXKeyValueList methods
    STDMETHOD(AddKeyValue)	(THIS_
				const char* pKey,
				IHXBuffer* pStr);

    STDMETHOD(GetIter)		(THIS_
				REF(IHXKeyValueListIter*) pIter);


    STDMETHOD(GetIterOneKey)	(THIS_
				const char* pKey,
				REF(IHXKeyValueListIterOneKey*) pIter);

    STDMETHOD(AppendAllListItems)   (THIS_
				    IHXKeyValueList* pList);

    STDMETHOD_(HXBOOL,KeyExists)  (THIS_
				const char* pKey);

    STDMETHOD(CreateObject)	(THIS_
				REF(IHXKeyValueList*) pNewList);

    STDMETHOD(ImportValues)	(THIS_
				IHXValues* pValues);

    // IHXValues methods

    STDMETHOD(SetPropertyULONG32)	(THIS_
					const char*      pPropertyName,
					ULONG32          uPropertyValue);

    STDMETHOD(GetPropertyULONG32)	(THIS_
					const char*      pPropertyName,
					REF(ULONG32)     uPropertyName);

    STDMETHOD(GetFirstPropertyULONG32)	(THIS_
					REF(const char*) pPropertyName,
					REF(ULONG32)     uPropertyValue);

    STDMETHOD(GetNextPropertyULONG32)	(THIS_
					REF(const char*) pPropertyName,
					REF(ULONG32)     uPropertyValue);

    STDMETHOD(SetPropertyBuffer)	(THIS_
					const char*      pPropertyName,
					IHXBuffer*      pPropertyValue);

    STDMETHOD(GetPropertyBuffer)	(THIS_
					const char*      pPropertyName,
					REF(IHXBuffer*) pPropertyValue);
    
    STDMETHOD(GetFirstPropertyBuffer)	(THIS_
					REF(const char*) pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(GetNextPropertyBuffer)	(THIS_
					REF(const char*) pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(SetPropertyCString)	(THIS_
					const char*      pPropertyName,
					IHXBuffer*      pPropertyValue);

    STDMETHOD(GetPropertyCString)	(THIS_
					const char*      pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(GetFirstPropertyCString)	(THIS_
					REF(const char*) pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(GetNextPropertyCString)	(THIS_
					REF(const char*) pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    // support non-uniquely-keyed list of strings; use a linked
    // list of nodes, and keep an internal ref count on the list
    struct node 
    {
	char*        pKey;
	IHXBuffer*  pStr;
	struct node* pNext;
    };

private:
    // support reference counting
    LONG32 m_lRefCount;

    struct list 
    {
	void AddRef();
	void Release();
	list();
	~list();
	struct node *m_pHead;
	LONG32 m_lRefCount;
    } *m_pList;
    struct node *m_pTail;
    friend struct list;
    friend struct node;

    // support GetFirstPropertyCString/GetNextPropertyCString
    struct NonReentrantIterator
    {
	NonReentrantIterator() : m_pCurr(NULL) {}
	struct node *m_pCurr;
    } m_NonReentrantIterator;

    // support ULONG32 and Buffer functions
    CSimpleUlongMapStrCaseCmp  m_UlongMap;
    CSimpleBufferMapStrCaseCmp m_BufferMap;
};

class CKeyValueListIter : public IHXKeyValueListIter
{
public:
    CKeyValueListIter(CKeyValueList::list* pList);
    ~CKeyValueListIter();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    // Regular methods
    STDMETHOD(GetNextPair)	(THIS_
				REF(const char*) pKey,
				REF(IHXBuffer*) pStr);
    STDMETHOD(ReplaceCurr)	(THIS_
				IHXBuffer* pStr);
private:
    CKeyValueListIter();
    CKeyValueList::list *m_pList;
    CKeyValueList::node *m_pCurr;
    LONG32 m_lRefCount;
};

class CKeyValueListIterOneKey : public IHXKeyValueListIterOneKey
{
public:
    CKeyValueListIterOneKey(const char* pKey, CKeyValueList::list* pList);
    ~CKeyValueListIterOneKey();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    // Regular methods
    STDMETHOD(GetNextString)	(THIS_
				REF(IHXBuffer*) pStr);
    
    STDMETHOD(ReplaceCurr)	(THIS_
				IHXBuffer* pStr);
private:
    CKeyValueListIterOneKey();
    CKeyValueList::list *m_pList;
    CKeyValueList::node *m_pCurr;
    CKeyValueList::node *m_pReplace;
    char* m_pKey;
    LONG32 m_lRefCount;
};

class CHXUniquelyKeyedList : 
	public IHXValues,
	public IHXValuesRemove
{
    // see big comment in hxvalues.cpp
public:
    CHXUniquelyKeyedList();
    ~CHXUniquelyKeyedList();
    
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);


    // IHXValues methods

    STDMETHOD(SetPropertyULONG32)	(THIS_
					const char*      pPropertyName,
					ULONG32          uPropertyValue);

    STDMETHOD(GetPropertyULONG32)	(THIS_
					const char*      pPropertyName,
					REF(ULONG32)     uPropertyName);

    STDMETHOD(GetFirstPropertyULONG32)	(THIS_
					REF(const char*) pPropertyName,
					REF(ULONG32)     uPropertyValue);

    STDMETHOD(GetNextPropertyULONG32)	(THIS_
					REF(const char*) pPropertyName,
					REF(ULONG32)     uPropertyValue);

    STDMETHOD(SetPropertyBuffer)	(THIS_
					const char*      pPropertyName,
					IHXBuffer*      pPropertyValue);

    STDMETHOD(GetPropertyBuffer)	(THIS_
					const char*      pPropertyName,
					REF(IHXBuffer*) pPropertyValue);
    
    STDMETHOD(GetFirstPropertyBuffer)	(THIS_
					REF(const char*) pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(GetNextPropertyBuffer)	(THIS_
					REF(const char*) pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(SetPropertyCString)	(THIS_
					const char*      pPropertyName,
					IHXBuffer*      pPropertyValue);

    STDMETHOD(GetPropertyCString)	(THIS_
					const char*      pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(GetFirstPropertyCString)	(THIS_
					REF(const char*) pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    STDMETHOD(GetNextPropertyCString)	(THIS_
					REF(const char*) pPropertyName,
					REF(IHXBuffer*) pPropertyValue);

    /*
     * IHXValuesRemove methods
     */

    STDMETHOD(Remove)		(THIS_ const char* pKey);
    STDMETHOD(RemoveULONG32)	(THIS_ const char* pKey);
    STDMETHOD(RemoveBuffer)	(THIS_ const char* pKey);
    STDMETHOD(RemoveCString)	(THIS_ const char* pKey);

private:
    CSimpleUlongMapStrCmp   m_UlongMap;
    CSimpleBufferMapStrCmp  m_BufferMap;
    CSimpleBufferMapStrCmp  m_CStringMap;
    LONG32 m_lRefCount;
};

#endif // header guard


