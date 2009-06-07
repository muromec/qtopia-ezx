/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxvalues.cpp,v 1.10 2005/03/24 00:01:37 liam_murray Exp $
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

//
// The CKeyValueList is a strange class, because it has some backward  
// compatibilities w/the old-style IHXValues.  It supports these interfaces:
// 
//    IHXKeyValueList - non-uniquely list of strings
//    IHXValues       - uniquely keyed lists of strings, buffers, ulongs
//
// FAQ:
// ====
//
// Q: How can CKeyValueList support a non-uniquely keyed list of strings, while 
// also supporting a uniquely keyed list of strings?  
// 
// A: The class as a whole does not enforces uniqueness.  If you exclusively use 
// GetPropertyCString/SetPropertyCString for any given key, though, you can be
// sure that only one instance of that key exists.  Read on.
// 
// OVERVIEW OF CLASS INTERFACE:
// ============================
//
// Basically, this class allows you to store three types of data:
//
//    strings  
//    buffers
//    ULONGs
// 
// The data types are stored in completely separate data structures.
//
// Buffers and ULONGS are always uniquely keyed.  Use these functions to 
// get/set Buffer/ULONG values:
// 
//  SetPropertyULONG32/GetPropertyULONG32
//  SetPropertyBuffer/GetPropertyBuffer
//
// Strings can be non-uniquely keyed.  If you add a string using AddKeyValue,
// you're gonna create a new key/string tuple, even if that key had already been 
// used for a string.  
//
//    AddKeyValue - add new tuple, even if key was already used
//    GetIterOneKey - see "Iterating"
//
//    GetPropertyCString - fail if no strings for that key; otherwise, return
//          first string inserted for that key
//    SetPropertyCString - write new tuple if no strings for that key; otherwise,
//          change first tuple for that key to use new string
//    
// Mixing and matching AddKeyValue calls w/SetPropertyCString calls is allowable,
// but generally indicates a muddled understanding of the interfaces.  
//
// Iterating --
// -------------
// To iterate over all your ULONG keys, use these functions:
//
//   GetFirstPropertyULONG32/GetNextPropertyULONG32
//
// To iterate over all your buffer keys, use these functions:
//
//   GetFirstPropertyBuffer/GetNextPropertyBuffer
//
// To iterate over all your strings, use these functions:
//
//   GetIter ... while (GetNextPair == HXR_OK)
//
// These are preferred to GetFirstPropertyCString/GetNextPropertyCString, 
// because they are reentrant.  You can use the old interface, though.
//
// To iterate all the strings for a key, use these functions:
//
//   GetIterOneKey ... while (GetNextString == HXR_OK)
//
// While iterating, you can replace the current string by calling ReplaceCurr.
// This is particulary useful when you are replacing several strings for the 
// same key, like for mangling all cookies.
//
// 
// SOME HISTORY:
// =============
// The predecessor to this class was CKeyedIList, and various cousins that were 
// never really used.  The classes were replaced, due to strange interfaces and 
// abusive precompiler usage.
//
//
// USAGE/PERFORMANCE CONSIDERATIONS:
// =================================
// Don't use this class if you know all your keys are always going to be unique.
// You'd be better off using CHXHeader for that, or some other class that 
// supports only IHXValues.  On the other hand, even w/the extra baggage of 
// IHXKeyValueList, this class is simpler than most other IHXValue variants,
// so it might be good to use if you don't have a lot of keys.  This class uses
// linked lists, and linear seaches.  Most other classes use hashes.
//
// This class should have the exact behavior as CHXHeaders for buffers and ULONGs.
// Most users of this class barely do anything w/buffers and ULONGs.  If, however,
// this class ever becomes a performance bottleneck, a good thing to do may be to
// steal the CHXHeader code that relates to buffers and ulongs.  You're probably
// best off cutting-and-pasting CHXHeader code, or in a pinch, you can have this
// class wrap an actual CHXHeader object, and the string methods just get ignored.
// Another option is to add a hash table to this implementation to speed up lookups.
// 

#include "hxtypes.h"
#include "hxcom.h"
#include "hxstrutl.h"
#include "hxstring.h"

#include "hxvalue.h"
#include "ihxpckts.h"

#include "hxbuffer.h"

#include "hxvalues.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

// CKeyValueList is further down....

/////////////// CSimpleUlongMap

CSimpleUlongMap::CSimpleUlongMap()
{
    m_pHead = NULL;
    m_pTail = NULL;
    m_pCurr = NULL;
}
 
CSimpleUlongMap::~CSimpleUlongMap()
{
    while (m_pHead)
    {
	node *p = m_pHead;
	delete[] p->pKey;
	m_pHead = p->pNext;
	delete p;
    }
    m_pHead = NULL; // paranoid
    m_pTail = NULL; // paranoid
}

HX_RESULT CSimpleUlongMap::SetProperty(const char* pKey, ULONG32 ulValue)
{
    for (node *p = m_pHead; p; p = p->pNext)
    {
	if (!StrCmpFunc(p->pKey, pKey)) 
	{
	    p->ulValue = ulValue;
	    return HXR_OK;
	}
    }

    node *pNew    = new node;
    pNew->pNext   = NULL;
    pNew->pKey    = new_string(pKey);
    pNew->ulValue = ulValue;

    if (m_pTail)
    {
	m_pTail->pNext = pNew;
    }
    else
    {
	m_pHead = pNew;
    }
    m_pTail = pNew;

    return HXR_OK;
}


HX_RESULT CSimpleUlongMap::GetProperty(const char* pKey, REF(ULONG32) ulValue)
{
    for (node *p = m_pHead; p; p = p->pNext)
    {
	if (!StrCmpFunc(p->pKey, pKey))
	{
	    ulValue = p->ulValue;
	    return HXR_OK;
	}
    }

    return HXR_FAILED;
}
    
HX_RESULT CSimpleUlongMap::GetFirstProperty(REF(const char*) pKey, REF(ULONG32) ulValue)
{
    m_pCurr = m_pHead;
    if (m_pCurr)
    {
	pKey    = m_pCurr->pKey;
	ulValue = m_pCurr->ulValue;
    	return HXR_OK;
    }
    else
    {
	return HXR_FAILED;
    }
}
    
HX_RESULT CSimpleUlongMap::GetNextProperty(REF(const char*) pKey, REF(ULONG32) ulValue)
{
    if (!m_pCurr)
    {
	HX_ASSERT(0);
	return HXR_FAILED;
    }

    m_pCurr = m_pCurr->pNext;
    if (m_pCurr)
    {
	pKey    = m_pCurr->pKey;
	ulValue = m_pCurr->ulValue;
    	return HXR_OK;
    }
    else
    {
	return HXR_FAILED;
    }
}
    
void CSimpleUlongMap::Remove(const char* pKey)
{
    if (!m_pHead) 
    {
	return;
    }

    if (m_pHead && !StrCmpFunc(pKey,m_pHead->pKey))
    {
	node *pNext = m_pHead->pNext;
	delete[] m_pHead->pKey;
	delete m_pHead;
	m_pHead = pNext;
	if (pNext == NULL)
	{
	    m_pTail = NULL;
	}
	return;
    }

    node *p1 = m_pHead;
    node *p2 = p1->pNext;
    while (p2)
    {
	if (!StrCmpFunc(pKey,p2->pKey))
	{
	    p1->pNext = p2->pNext;
	    if (p1->pNext == NULL)
	    {
		m_pTail = p1;
	    }
	    delete[] p2->pKey;
	    delete p2;
	    return;
	}
	p1 = p2;
	p2 = p2->pNext;
    }
}


/////////////// CSimpleBufferMap

CSimpleBufferMap::CSimpleBufferMap()
{
    m_pHead = NULL;
    m_pTail = NULL;
    m_pCurr = NULL;
}
 
CSimpleBufferMap::~CSimpleBufferMap()
{
    while (m_pHead)
    {
	node *p = m_pHead;
	delete[] p->pKey;
        HX_RELEASE(p->pValue);
	m_pHead = p->pNext;
	delete p;
    }
    m_pHead = NULL; // paranoid
    m_pTail = NULL; // paranoid
}

HX_RESULT CSimpleBufferMap::SetProperty(const char* pKey, IHXBuffer* pValue)
{
    if (!pValue)
    {
	HX_ASSERT(0);
	return HXR_FAIL;
    }

    for (node *p = m_pHead; p; p = p->pNext)
    {
	if (!StrCmpFunc(p->pKey, pKey))
	{
	    IHXBuffer* pOld = p->pValue;           
	    p->pValue = pValue;
	    p->pValue->AddRef();
	    HX_RELEASE(pOld);
	    return HXR_OK;
	}
    }

    node *pNew    = new node;
    pNew->pNext   = NULL;
    pNew->pKey    = new_string(pKey);
    pNew->pValue = pValue;
    pNew->pValue->AddRef();

    if (m_pTail)
    {
	m_pTail->pNext = pNew;
    }
    else
    {
	m_pHead = pNew;
    }
    m_pTail = pNew;

    return HXR_OK;
}


HX_RESULT CSimpleBufferMap::GetProperty(const char* pKey, REF(IHXBuffer*) pValue)
{
    for (node *p = m_pHead; p; p = p->pNext)
    {
	if (!StrCmpFunc(p->pKey, pKey))
	{
	    pValue = p->pValue;
	    pValue->AddRef();
	    return HXR_OK;
	}
    }

    return HXR_FAILED;
}
    
HX_RESULT CSimpleBufferMap::GetFirstProperty(REF(const char*) pKey, REF(IHXBuffer*) pValue)
{
    m_pCurr = m_pHead;
    if (m_pCurr)
    {
	pKey    = m_pCurr->pKey;
	pValue = m_pCurr->pValue;
	pValue->AddRef();
    	return HXR_OK;
    }
    else
    {
	return HXR_FAILED;
    }
}
    
HX_RESULT CSimpleBufferMap::GetNextProperty(REF(const char*) pKey, REF(IHXBuffer*) pValue)
{
    if (!m_pCurr)
    {
	HX_ASSERT(0);
	return HXR_FAILED;
    }

    m_pCurr = m_pCurr->pNext;
    if (m_pCurr)
    {
	pKey    = m_pCurr->pKey;
	pValue = m_pCurr->pValue;
	pValue->AddRef();
    	return HXR_OK;
    }
    else
    {
	return HXR_FAILED;
    }
}
    
void CSimpleBufferMap::Remove(const char* pKey)
{
    if (!m_pHead) 
    {
	return;
    }

    if (m_pHead && !StrCmpFunc(pKey,m_pHead->pKey))
    {
	node *pNext = m_pHead->pNext;
        HX_RELEASE(m_pHead->pValue);
	delete[] m_pHead->pKey;
	delete m_pHead;
	m_pHead = pNext;
	if (pNext == NULL)
	{
	    m_pTail = NULL;
	}
	return;
    }

    node *p1 = m_pHead;
    node *p2 = p1->pNext;
    while (p2)
    {
	if (!StrCmpFunc(pKey,p2->pKey))
	{
	    p1->pNext = p2->pNext;
	    if (p1->pNext == NULL)
	    {
		m_pTail = p1;
	    }
            HX_RELEASE(p2->pValue);
	    delete[] p2->pKey;
	    delete p2;
	    return;
	}
	p1 = p2;
	p2 = p2->pNext;
    }
}


//////////////// CKeyValueList

STDMETHODIMP CKeyValueList::QueryInterface(REFIID riid, void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), this },
		{ GET_IIDHANDLE(IID_IHXKeyValueList), (IHXKeyValueList*) this },
		{ GET_IIDHANDLE(IID_IHXValues), (IHXValues*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);   
}

STDMETHODIMP_(ULONG32) CKeyValueList::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CKeyValueList::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

CKeyValueList::CKeyValueList() : m_lRefCount(0), m_pTail(NULL)
{
    m_pList = new list;
    m_pList->AddRef();
}

CKeyValueList::~CKeyValueList()
{
    if (m_pList)
    {
        m_pList->Release();
        m_pList = NULL;
    }
    m_pTail = NULL; // paranoid
}

STDMETHODIMP
CKeyValueList::AddKeyValue(const char* pKey, IHXBuffer* pStr)
{
    node *pNew  = new node;
    pNew->pNext = NULL;
    pNew->pStr  = pStr;
    pNew->pStr->AddRef();
    pNew->pKey  = new_string(pKey);

    if (m_pTail)
    {
	m_pTail->pNext = pNew;
    }
    else
    {
	m_pList->m_pHead = pNew;
    }
    m_pTail = pNew;

    return HXR_OK;
}

STDMETHODIMP
CKeyValueList::GetIterOneKey(const char* pKey, REF(IHXKeyValueListIterOneKey*) pIter)
{
    pIter = new CKeyValueListIterOneKey(pKey,m_pList);
    pIter->AddRef();

    return HXR_OK;
}

STDMETHODIMP
CKeyValueList::GetIter(REF(IHXKeyValueListIter*) pIter)
{
    pIter = new CKeyValueListIter(m_pList);
    pIter->AddRef();

    return HXR_OK;
}

STDMETHODIMP 
CKeyValueList::AppendAllListItems(IHXKeyValueList* pList)
{
    IHXKeyValueListIter *pListIter = NULL;

    HX_RESULT rc = pList->GetIter(pListIter);

    if (rc == HXR_OK)
    {
	const char* pKey = NULL;
	IHXBuffer* pBuffer = NULL;
	while (pListIter->GetNextPair(pKey, pBuffer) == HXR_OK)
	{
	    this->AddKeyValue(pKey,pBuffer);
	    HX_RELEASE(pBuffer);
	}
	HX_RELEASE(pListIter);
    }

    return rc;
}
    
STDMETHODIMP_(HXBOOL)
CKeyValueList::KeyExists(const char* pKey)
{
    node *p;
    for (p = m_pList->m_pHead; p; p = p->pNext)
    {
	if (!strcasecmp(pKey,p->pKey))
	{
	    return TRUE;
	}
    }
    return FALSE;
}

STDMETHODIMP
CKeyValueList::CreateObject(REF(IHXKeyValueList*) pNewList)
{
    pNewList = new CKeyValueList;
    if (pNewList)
    {
	pNewList->AddRef();
	return HXR_OK;
    }
    else
    {
	return HXR_OUTOFMEMORY;
    }
}

STDMETHODIMP 
CKeyValueList::ImportValues(THIS_
			    IHXValues* pValues)
{
    HX_RESULT hResult = HXR_OK;
    const char* pName  = NULL;
    IHXBuffer* pBuffer = NULL;

    // Copy all CStrings
    hResult = pValues->GetFirstPropertyCString(pName, pBuffer);
    while (hResult == HXR_OK)
    {
	this->AddKeyValue(pName,pBuffer);
	HX_RELEASE(pBuffer);

	hResult = pValues->GetNextPropertyCString(pName, pBuffer);
    }

    // Copy all ULONGs
    ULONG32 ul;
    hResult = pValues->GetFirstPropertyULONG32(pName, ul);
    while (hResult == HXR_OK)
    {
	this->SetPropertyULONG32(pName,ul);
	hResult = pValues->GetNextPropertyULONG32(pName, ul);
    }

    // Copy all Buffers
    hResult = pValues->GetFirstPropertyBuffer(pName, pBuffer);
    while (hResult == HXR_OK)
    {
	this->SetPropertyBuffer(pName,pBuffer);
	HX_RELEASE(pBuffer);
	hResult = pValues->GetNextPropertyBuffer(pName, pBuffer);
    }

    return HXR_OK;
}


////////// Support IHXValues interface for CKeyValueList

STDMETHODIMP
CKeyValueList::SetPropertyULONG32(const char*      pPropertyName,
				  ULONG32          uPropertyValue)
{
    return m_UlongMap.SetProperty(pPropertyName,uPropertyValue);
}

STDMETHODIMP
CKeyValueList::GetPropertyULONG32(const char*      pPropertyName,
				  REF(ULONG32)     uPropertyName)
{
    return m_UlongMap.GetProperty(pPropertyName,uPropertyName);
}

STDMETHODIMP
CKeyValueList::GetFirstPropertyULONG32(REF(const char*) pPropertyName,
				       REF(ULONG32)     uPropertyValue)
{
    return m_UlongMap.GetFirstProperty(pPropertyName, uPropertyValue);
}

STDMETHODIMP
CKeyValueList::GetNextPropertyULONG32(REF(const char*) pPropertyName,
				      REF(ULONG32)     uPropertyValue)
{
    return m_UlongMap.GetNextProperty(pPropertyName, uPropertyValue);
}


STDMETHODIMP
CKeyValueList::SetPropertyBuffer(const char*      pPropertyName,
				 IHXBuffer*      pPropertyValue)
{
    return m_BufferMap.SetProperty(pPropertyName,pPropertyValue);
}

STDMETHODIMP
CKeyValueList::GetPropertyBuffer(const char*      pPropertyName,
				 REF(IHXBuffer*) pPropertyValue)
{
    return m_BufferMap.GetProperty(pPropertyName,pPropertyValue);
}

STDMETHODIMP
CKeyValueList::GetFirstPropertyBuffer(REF(const char*) pPropertyName,
				      REF(IHXBuffer*) pPropertyValue)
{
    return m_BufferMap.GetFirstProperty(pPropertyName,pPropertyValue);
}

STDMETHODIMP
CKeyValueList::GetNextPropertyBuffer(REF(const char*) pPropertyName,
				     REF(IHXBuffer*) pPropertyValue)
{
    return m_BufferMap.GetNextProperty(pPropertyName,pPropertyValue);
}


STDMETHODIMP
CKeyValueList::SetPropertyCString(const char*      pPropertyName,
				  IHXBuffer*      pPropertyValue)
{
    if (!pPropertyValue)
    {
	HX_ASSERT(0);
	return HXR_FAIL;
    }

    // I am trying to preserve the semantics that Kirk had here.
    // I replace the first instance of the key, if any, or if the
    // key does not exist yet, I simply add to the list.
    node *p;
    for (p = m_pList->m_pHead; p; p = p->pNext)
    {
	if (!strcasecmp(pPropertyName,p->pKey))
	{
	    IHXBuffer* pOldStr = p->pStr;
	    p->pStr = pPropertyValue;
	    p->pStr->AddRef();
    	    HX_RELEASE(pOldStr);
	    return HXR_OK;
	}
    }
    
    return AddKeyValue(pPropertyName,pPropertyValue);
}

STDMETHODIMP 
CKeyValueList::GetPropertyCString(const char*      pPropertyName,
				  REF(IHXBuffer*) pPropertyValue)
{
    node *p;
    for (p = m_pList->m_pHead; p; p = p->pNext)
    {
	if (!strcasecmp(pPropertyName,p->pKey))
	{
	    pPropertyValue = p->pStr;
	    pPropertyValue->AddRef();
	    return HXR_OK;
	}
    }
    return HXR_FAIL;
}

STDMETHODIMP
CKeyValueList::GetFirstPropertyCString(REF(const char*) pPropertyName,
				       REF(IHXBuffer*) pPropertyValue)
{
    m_NonReentrantIterator.m_pCurr = m_pList->m_pHead;

    if (m_NonReentrantIterator.m_pCurr)
    {
	pPropertyName  = m_NonReentrantIterator.m_pCurr->pKey;
	pPropertyValue = m_NonReentrantIterator.m_pCurr->pStr;
	pPropertyValue->AddRef();
	return HXR_OK;
    }
    else
    {
	return HXR_FAIL;
    }
}

STDMETHODIMP
CKeyValueList::GetNextPropertyCString(REF(const char*) pPropertyName,
	    			      REF(IHXBuffer*) pPropertyValue)
{
    if (!m_NonReentrantIterator.m_pCurr)
    {
	HX_ASSERT(0);
	return HXR_UNEXPECTED;
    }

    m_NonReentrantIterator.m_pCurr = m_NonReentrantIterator.m_pCurr->pNext;

    if (m_NonReentrantIterator.m_pCurr)
    {
	pPropertyName  = m_NonReentrantIterator.m_pCurr->pKey;
	pPropertyValue = m_NonReentrantIterator.m_pCurr->pStr;
	pPropertyValue->AddRef();
	return HXR_OK;
    }
    else
    {
	return HXR_FAIL;
    }
}


/////////  Implement CKeyValueList::list

void CKeyValueList::list::AddRef()
{
    InterlockedIncrement(&m_lRefCount);
}

void CKeyValueList::list::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return;
    }
    else
    {
    	delete this;
    }
}

CKeyValueList::list::list() : m_pHead(NULL), m_lRefCount(0) 
{
}

CKeyValueList::list::~list()
{
    while (m_pHead)
    {
	node *p = m_pHead;
	HX_RELEASE(p->pStr);
	delete[] p->pKey;
	m_pHead = p->pNext;
	delete p;
    }
    m_pHead = NULL; // paranoid
}

///////////// Implement CKeyValueListIter

STDMETHODIMP CKeyValueListIter::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXKeyValueListIter))
    {
	AddRef();
	*ppvObj = (IHXKeyValueListIter*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) CKeyValueListIter::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CKeyValueListIter::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

CKeyValueListIter::CKeyValueListIter(CKeyValueList::list* pList) 
    :	m_pList(pList), 
	m_pCurr(NULL),
	m_lRefCount(0) 
{
    m_pList->AddRef();
}
    
CKeyValueListIter::~CKeyValueListIter() 
{
    if (m_pList)
    {
        m_pList->Release();
        m_pList = NULL;
    }
}


STDMETHODIMP 
CKeyValueListIter::GetNextPair(REF(const char*) pKey, REF(IHXBuffer*) pStr)
{
    if (m_pCurr == NULL)
    {
	m_pCurr = m_pList->m_pHead;
    }
    else
    {
	m_pCurr = m_pCurr->pNext;
    }
    if (m_pCurr)
    {
	pKey = m_pCurr->pKey;
	pStr = m_pCurr->pStr;
	pStr->AddRef();
	// important -- don't advance m_pCurr till next
	// call, or you'll break ReplaceCurr
	return HXR_OK;
    }
    else
    {
	return HXR_FAILED;
    }
}

STDMETHODIMP 
CKeyValueListIter::ReplaceCurr(IHXBuffer* pStr)
{
    // overwrites string from last call to GetNextPair
    if (!m_pCurr)
    {
	HX_ASSERT(0);
	return HXR_UNEXPECTED;
    }
    else
    {
	IHXBuffer *pOld = m_pCurr->pStr;
	m_pCurr->pStr = pStr;
	m_pCurr->pStr->AddRef();
	HX_RELEASE(pOld);
    }
    return HXR_OK;
}


////////////////// Implement CKeyValueListIterOneKey


STDMETHODIMP CKeyValueListIterOneKey::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXKeyValueListIterOneKey))
    {
	AddRef();
	*ppvObj = (IHXKeyValueListIterOneKey*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) CKeyValueListIterOneKey::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CKeyValueListIterOneKey::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

CKeyValueListIterOneKey::CKeyValueListIterOneKey(const char* pKey,
						 CKeyValueList::list* pList) 
    :	m_pList(pList), 
	m_pCurr(pList->m_pHead),
	m_pReplace(0),
	m_lRefCount(0) 
{
    m_pList->AddRef();
    m_pKey = new_string(pKey);
}
    
CKeyValueListIterOneKey::~CKeyValueListIterOneKey() 
{
    if (m_pList)
    {
        m_pList->Release();
        m_pList = NULL;
    }
    delete[] m_pKey;
}


STDMETHODIMP 
CKeyValueListIterOneKey::GetNextString(REF(IHXBuffer*) pStr)
{
    while (m_pCurr)
    {
	if (!strcasecmp(m_pCurr->pKey,m_pKey))
	{
	    pStr = m_pCurr->pStr;
	    pStr->AddRef();
	    m_pReplace = m_pCurr;
       	    m_pCurr = m_pCurr->pNext;
	    return HXR_OK;
	}
	m_pCurr = m_pCurr->pNext;
    }

    return HXR_FAILED;
}

STDMETHODIMP 
CKeyValueListIterOneKey::ReplaceCurr(IHXBuffer* pStr)
{
    // overwrites string from last call to GetNextString
    if (!m_pReplace)
    {
	HX_ASSERT(0);
	return HXR_UNEXPECTED;
    }
    else
    {
	IHXBuffer *pOld = m_pReplace->pStr;
	m_pReplace->pStr = pStr;
	m_pReplace->pStr->AddRef();
	HX_RELEASE(pOld);
    }
    return HXR_OK;
}

////////////// CHXUniquelyKeyedList

STDMETHODIMP CHXUniquelyKeyedList::QueryInterface(REFIID riid, void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), this },
		{ GET_IIDHANDLE(IID_IHXValues), (IHXValues*) this },
		{ GET_IIDHANDLE(IID_IHXValuesRemove), (IHXValuesRemove*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);   
}

STDMETHODIMP_(ULONG32) CHXUniquelyKeyedList::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXUniquelyKeyedList::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

CHXUniquelyKeyedList::CHXUniquelyKeyedList() : m_lRefCount(0)
{
    // most members construct themselves
}

CHXUniquelyKeyedList::~CHXUniquelyKeyedList()
{
    // members destruct themselves
}

STDMETHODIMP
CHXUniquelyKeyedList::SetPropertyULONG32(const char*      pPropertyName,
				          ULONG32          uPropertyValue)
{
    return m_UlongMap.SetProperty(pPropertyName,uPropertyValue);
}

STDMETHODIMP
CHXUniquelyKeyedList::GetPropertyULONG32(const char*      pPropertyName,
				          REF(ULONG32)     uPropertyName)
{
    return m_UlongMap.GetProperty(pPropertyName,uPropertyName);
}

STDMETHODIMP
CHXUniquelyKeyedList::GetFirstPropertyULONG32(REF(const char*) pPropertyName,
				               REF(ULONG32)     uPropertyValue)
{
    return m_UlongMap.GetFirstProperty(pPropertyName, uPropertyValue);
}

STDMETHODIMP
CHXUniquelyKeyedList::GetNextPropertyULONG32(REF(const char*) pPropertyName,
				              REF(ULONG32)     uPropertyValue)
{
    return m_UlongMap.GetNextProperty(pPropertyName, uPropertyValue);
}

STDMETHODIMP
CHXUniquelyKeyedList::SetPropertyBuffer(const char*      pPropertyName,
				         IHXBuffer*      pPropertyValue)
{
    return m_BufferMap.SetProperty(pPropertyName,pPropertyValue);
}

STDMETHODIMP
CHXUniquelyKeyedList::GetPropertyBuffer(const char*      pPropertyName,
				         REF(IHXBuffer*) pPropertyValue)
{
    return m_BufferMap.GetProperty(pPropertyName,pPropertyValue);
}

STDMETHODIMP
CHXUniquelyKeyedList::GetFirstPropertyBuffer(REF(const char*) pPropertyName,
		    		              REF(IHXBuffer*) pPropertyValue)
{
    return m_BufferMap.GetFirstProperty(pPropertyName,pPropertyValue);
}

STDMETHODIMP
CHXUniquelyKeyedList::GetNextPropertyBuffer(REF(const char*) pPropertyName,
				             REF(IHXBuffer*) pPropertyValue)
{
    return m_BufferMap.GetNextProperty(pPropertyName,pPropertyValue);
}

STDMETHODIMP
CHXUniquelyKeyedList::SetPropertyCString(const char*      pPropertyName,
				         IHXBuffer*      pPropertyValue)
{
    return m_CStringMap.SetProperty(pPropertyName,pPropertyValue);
}

STDMETHODIMP
CHXUniquelyKeyedList::GetPropertyCString(const char*      pPropertyName,
				          REF(IHXBuffer*) pPropertyValue)
{
    return m_CStringMap.GetProperty(pPropertyName,pPropertyValue);
}

STDMETHODIMP
CHXUniquelyKeyedList::GetFirstPropertyCString(REF(const char*) pPropertyName,
		    		              REF(IHXBuffer*) pPropertyValue)
{
    return m_CStringMap.GetFirstProperty(pPropertyName,pPropertyValue);
}

STDMETHODIMP
CHXUniquelyKeyedList::GetNextPropertyCString(REF(const char*) pPropertyName,
				              REF(IHXBuffer*) pPropertyValue)
{
    return m_CStringMap.GetNextProperty(pPropertyName,pPropertyValue);
}

STDMETHODIMP
CHXUniquelyKeyedList::Remove(const char* pPropertyName)
{
    m_UlongMap.Remove(pPropertyName);
    m_BufferMap.Remove(pPropertyName);
    m_CStringMap.Remove(pPropertyName);
    return HXR_OK;
}

STDMETHODIMP
CHXUniquelyKeyedList::RemoveULONG32(const char* pPropertyName)
{
    m_UlongMap.Remove(pPropertyName);
    return HXR_OK;
}

STDMETHODIMP
CHXUniquelyKeyedList::RemoveBuffer(const char* pPropertyName)
{
    m_BufferMap.Remove(pPropertyName);
    return HXR_OK;
}

STDMETHODIMP
CHXUniquelyKeyedList::RemoveCString(const char* pPropertyName)
{
    m_CStringMap.Remove(pPropertyName);
    return HXR_OK;
}

