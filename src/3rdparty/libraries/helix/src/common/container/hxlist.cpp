/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxlist.cpp,v 1.4 2005/03/24 00:01:37 liam_murray Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "ihxpckts.h"
#include "chxpckts.h"

#include "ihxlist.h"
#include "hxlistp.h"

#define INVALID_POS 0xFFFFFFFF

/*****************************************************************************
 *
 * CHXConstList
 *
 *****************************************************************************/

CHXConstList::CHXConstList(IHXList* plist) :
    m_plist(plist),
    m_ulRefCount(0)
{
    HX_ASSERT(plist != NULL);
    plist->AddRef();
}

CHXConstList::~CHXConstList(void)
{
    HX_ASSERT_VALID_PTR(this);

    HX_RELEASE(m_plist);
}

/*** IUnknown methods ***/

STDMETHODIMP
CHXConstList::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXList))
    {
        AddRef();
        *ppvObj = (IHXList*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXConstList::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CHXConstList::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXList methods ***/

STDMETHODIMP_(HXBOOL)
CHXConstList::IsEmpty(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);

    return m_plist->IsEmpty();
}

STDMETHODIMP_(UINT32)
CHXConstList::GetCount(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);

    return m_plist->GetCount();
}

STDMETHODIMP_(IHXList*)
CHXConstList::AsConst(void)
{
    AddRef();
    return (IHXList*)this;
}

STDMETHODIMP
CHXConstList::InsertHead(IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);
    HX_ASSERT(punkItem != NULL);

    HX_ASSERT(FALSE);
    return HXR_FAIL;
}

STDMETHODIMP
CHXConstList::InsertTail(IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);
    HX_ASSERT(punkItem != NULL);

    HX_ASSERT(FALSE);
    return HXR_FAIL;
}

STDMETHODIMP
CHXConstList::InsertBefore(IHXListIterator* pIter, IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);
    HX_ASSERT(pIter != NULL && punkItem != NULL);

    HX_ASSERT(FALSE);
    return HXR_FAIL;
}

STDMETHODIMP
CHXConstList::InsertAfter(IHXListIterator* pIter, IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);
    HX_ASSERT(pIter != NULL && punkItem != NULL);

    HX_ASSERT(FALSE);
    return HXR_FAIL;
}

STDMETHODIMP
CHXConstList::Replace(IHXListIterator* pIter, IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);
    HX_ASSERT(pIter != NULL && punkItem != NULL);

    HX_ASSERT(FALSE);
    return HXR_FAIL;
}

STDMETHODIMP_(IUnknown*)
CHXConstList::Remove(IHXListIterator* pIter)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);
    HX_ASSERT(pIter != NULL);

    HX_ASSERT(FALSE);
    return NULL;
}

STDMETHODIMP_(IUnknown*)
CHXConstList::RemoveHead(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);

    HX_ASSERT(FALSE);
    return NULL;
}

STDMETHODIMP_(IUnknown*)
CHXConstList::RemoveTail(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);

    HX_ASSERT(FALSE);
    return NULL;
}

STDMETHODIMP_(IHXListIterator*)
CHXConstList::Begin(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);

    return m_plist->Begin();
}

STDMETHODIMP_(IHXListIterator*)
CHXConstList::End(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);

    return m_plist->End();
}

STDMETHODIMP_(IUnknown*)
CHXConstList::GetHead(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);

    return m_plist->GetHead();
}

STDMETHODIMP_(IUnknown*)
CHXConstList::GetTail(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);

    return m_plist->GetTail();
}


/*****************************************************************************
 *
 * CHXListIterator
 *
 *****************************************************************************/

CHXListIterator::CHXListIterator(CHXList* plist, CHXList::Node* pNode) :
    m_ulRefCount(0),
    m_plist(plist),
    m_pNode(pNode)
{
    HX_ASSERT(plist != NULL);
    plist->AddRef();
}

CHXListIterator::~CHXListIterator(void)
{
    HX_ASSERT_VALID_PTR(this);

    HX_RELEASE(m_plist);
}

/*** IUnknown methods ***/

STDMETHODIMP
CHXListIterator::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXListIterator))
    {
        AddRef();
        *ppvObj = (IHXListIterator*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXListIteratorPrivate))
    {
        AddRef();
        *ppvObj = (IHXListIteratorPrivate*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXListIterator::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CHXListIterator::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXListIterator methods ***/

STDMETHODIMP_(HXBOOL)
CHXListIterator::HasItem(void)
{
    HX_ASSERT_VALID_PTR(this);

    return (m_pNode != NULL);
}

STDMETHODIMP_(HXBOOL)
CHXListIterator::IsEqual(IHXListIterator* pOther)
{
    HX_ASSERT_VALID_PTR(this);

    HXBOOL bIsEqual = FALSE;
    IHXListIteratorPrivate* pOtherPriv = NULL;

    if (pOther->QueryInterface(IID_IHXListIteratorPrivate, (void**)&pOtherPriv) == HXR_OK)
    {
        bIsEqual = (pOtherPriv->GetList() == m_plist && pOtherPriv->GetNode() == m_pNode);
        pOtherPriv->Release();
    }

    return bIsEqual;
}

STDMETHODIMP_(HXBOOL)
CHXListIterator::MoveNext(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL && m_pNode != NULL);

    m_pNode = m_pNode->m_pNext;
    return (m_pNode != NULL);
}

STDMETHODIMP_(HXBOOL)
CHXListIterator::MovePrev(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL && m_pNode != NULL);

    m_pNode = m_pNode->m_pPrev;
    return (m_pNode != NULL);
}

STDMETHODIMP_(IUnknown*)
CHXListIterator::GetItem(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL && m_pNode != NULL);

    m_pNode->m_punkItem->AddRef();
    return m_pNode->m_punkItem;
}

/*** IHXListIteratorPrivate methods ***/

STDMETHODIMP_(CHXList*)
CHXListIterator::GetList(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_plist != NULL);

    return m_plist;
}

STDMETHODIMP_(CHXList::Node*)
CHXListIterator::GetNode(void)
{
    HX_ASSERT_VALID_PTR(this);

    return m_pNode;
}

STDMETHODIMP
CHXListIterator::Reset(void)
{
    m_pNode = NULL;
    return HXR_OK;
}

/*****************************************************************************
 *
 * CHXList
 *
 *****************************************************************************/

CHXList::CHXList(IHXFastAlloc* pFastAlloc) :
    m_ulRefCount(0),
    m_pHead(NULL),
    m_pTail(NULL),
    m_nCount(0),
    m_pFastAlloc(pFastAlloc)
{
    if (m_pFastAlloc)
    {
        m_pFastAlloc->AddRef();
    }
}

CHXList::~CHXList(void)
{
    HX_ASSERT_VALID_PTR(this);

    while (! IsEmpty())
    {
        IUnknown* punkItem = RemoveHead();
        HX_ASSERT(punkItem != NULL);
        HX_RELEASE(punkItem);
    }

    HX_RELEASE(m_pFastAlloc);
}

/*** IUnknown methods ***/

STDMETHODIMP
CHXList::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXList))
    {
        AddRef();
        *ppvObj = (IHXList*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXList::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CHXList::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXList methods ***/

STDMETHODIMP_(HXBOOL)
CHXList::IsEmpty(void)
{
    HX_ASSERT_VALID_PTR(this);

    return (m_nCount == 0);
}

STDMETHODIMP_(UINT32)
CHXList::GetCount(void)
{
    HX_ASSERT_VALID_PTR(this);

    return m_nCount;
}

STDMETHODIMP_(IHXList*)
CHXList::AsConst(void)
{
    IHXList* pConstList = new (m_pFastAlloc) CHXConstList(this);
    pConstList->AddRef();
    return pConstList;
}

STDMETHODIMP
CHXList::InsertHead(IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(punkItem != NULL);

    punkItem->AddRef();
    Node* pNode = new (m_pFastAlloc) Node(NULL, m_pHead, punkItem);
    if (m_pHead != NULL)
    {
        m_pHead->m_pPrev = pNode;
    }
    m_pHead = pNode;
    if (m_pTail == NULL)
    {
        m_pTail = pNode;
    }
    m_nCount++;

    return HXR_OK;
}

STDMETHODIMP
CHXList::InsertTail(IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(punkItem != NULL);

    punkItem->AddRef();
    Node* pNode = new (m_pFastAlloc) Node(m_pTail, NULL, punkItem);
    if (m_pTail != NULL)
    {
        m_pTail->m_pNext = pNode;
    }
    m_pTail = pNode;
    if (m_pHead == NULL)
    {
        m_pHead = pNode;
    }
    m_nCount++;

    return HXR_OK;
}

STDMETHODIMP
CHXList::InsertBefore(IHXListIterator* pIter, IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(pIter != NULL && punkItem != NULL);
    HX_ASSERT(m_pHead != NULL && m_pTail != NULL && m_nCount > 0);

    IHXListIteratorPrivate* pIterPriv = NULL;
    pIter->QueryInterface(IID_IHXListIteratorPrivate, (void**)&pIterPriv);
    HX_ASSERT(pIterPriv != NULL);
    HX_ASSERT(pIterPriv->GetList() == this && pIterPriv->GetNode() != NULL);
    Node* pIterNode = pIterPriv->GetNode();
    pIterPriv->Release();

    punkItem->AddRef();
    Node* pNode = new (m_pFastAlloc) Node(NULL, pIterNode, punkItem);
    if (pIterNode->m_pPrev != NULL)
    {
        pIterNode->m_pPrev->m_pNext = pNode;
        pNode->m_pPrev = pIterNode->m_pPrev;
    }
    else
    {
        HX_ASSERT(m_pHead == pIterNode);
        m_pHead = pNode;
    }
    pIterNode->m_pPrev = pNode;
    m_nCount++;

    return HXR_OK;
}

STDMETHODIMP
CHXList::InsertAfter(IHXListIterator* pIter, IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(pIter != NULL && punkItem != NULL);
    HX_ASSERT(m_pHead != NULL && m_pTail != NULL && m_nCount > 0);

    IHXListIteratorPrivate* pIterPriv = NULL;
    pIter->QueryInterface(IID_IHXListIteratorPrivate, (void**)&pIterPriv);
    HX_ASSERT(pIterPriv != NULL);
    HX_ASSERT(pIterPriv->GetList() == this && pIterPriv->GetNode() != NULL);
    Node* pIterNode = pIterPriv->GetNode();
    pIterPriv->Release();

    punkItem->AddRef();
    Node* pNode = new (m_pFastAlloc) Node(pIterNode, NULL, punkItem);
    if (pIterNode->m_pNext != NULL)
    {
        pIterNode->m_pNext->m_pPrev = pNode;
        pNode->m_pNext = pIterNode->m_pNext;
    }
    else
    {
        HX_ASSERT(m_pTail == pIterNode);
        m_pTail = pNode;
    }
    pIterNode->m_pNext = pNode;
    m_nCount++;

    return HXR_OK;
}

STDMETHODIMP
CHXList::Replace(IHXListIterator* pIter, IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(pIter != NULL && punkItem != NULL);
    HX_ASSERT(m_pHead != NULL && m_pTail != NULL && m_nCount > 0);

    IHXListIteratorPrivate* pIterPriv = NULL;
    pIter->QueryInterface(IID_IHXListIteratorPrivate, (void**)&pIterPriv);
    HX_ASSERT(pIterPriv != NULL);
    HX_ASSERT(pIterPriv->GetList() == this && pIterPriv->GetNode() != NULL);
    Node* pIterNode = pIterPriv->GetNode();
    pIterPriv->Release();

    HX_ASSERT(pIterNode->m_punkItem != NULL);
    pIterNode->m_punkItem->Release();
    punkItem->AddRef();
    pIterNode->m_punkItem = punkItem;

    return HXR_OK;
}

STDMETHODIMP_(IUnknown*)
CHXList::Remove(IHXListIterator* pIter)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(pIter != NULL);
    HX_ASSERT(m_pHead != NULL && m_pTail != NULL && m_nCount > 0);

    IHXListIteratorPrivate* pIterPriv = NULL;
    pIter->QueryInterface(IID_IHXListIteratorPrivate, (void**)&pIterPriv);
    HX_ASSERT(pIterPriv != NULL);
    HX_ASSERT(pIterPriv->GetList() == this && pIterPriv->GetNode() != NULL);
    Node* pIterNode = pIterPriv->GetNode();
    pIterPriv->Reset();
    pIterPriv->Release();

    if (pIterNode->m_pPrev != NULL)
    {
        pIterNode->m_pPrev->m_pNext = pIterNode->m_pNext;
    }
    else
    {
        HX_ASSERT(m_pHead == pIterNode);
        m_pHead = pIterNode->m_pNext;
    }
    if (pIterNode->m_pNext != NULL)
    {
        pIterNode->m_pNext->m_pPrev = pIterNode->m_pPrev;
    }
    else
    {
        HX_ASSERT(m_pTail == pIterNode);
        m_pTail = pIterNode->m_pPrev;
    }
    IUnknown* punkItem = pIterNode->m_punkItem;
    delete pIterNode;
    m_nCount--;

    return punkItem;
}

STDMETHODIMP_(IUnknown*)
CHXList::RemoveHead(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_pHead != NULL && m_pTail != NULL && m_nCount > 0);

    Node* pNode = m_pHead;
    m_pHead = pNode->m_pNext;
    if (m_pHead == NULL)
    {
        HX_ASSERT(m_nCount == 1);
        m_pTail = NULL;
    }
    IUnknown* punkItem = pNode->m_punkItem;
    delete pNode;
    m_nCount--;

    return punkItem;
}

STDMETHODIMP_(IUnknown*)
CHXList::RemoveTail(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_pHead != NULL && m_pTail != NULL && m_nCount > 0);

    Node* pNode = m_pTail;
    m_pTail = pNode->m_pPrev;
    if (m_pTail == NULL)
    {
        HX_ASSERT(m_nCount == 1);
        m_pHead = NULL;
    }
    IUnknown* punkItem = pNode->m_punkItem;
    delete pNode;
    m_nCount--;

    return punkItem;
}

STDMETHODIMP_(IHXListIterator*)
CHXList::Begin(void)
{
    HX_ASSERT_VALID_PTR(this);
    CHXListIterator* pIter = new (m_pFastAlloc) CHXListIterator(this, m_pHead);
    pIter->AddRef();
    return pIter;
}

STDMETHODIMP_(IHXListIterator*)
CHXList::End(void)
{
    HX_ASSERT_VALID_PTR(this);

    CHXListIterator* pIter = new (m_pFastAlloc) CHXListIterator(this, NULL);
    pIter->AddRef();

    return pIter;
}

/*** IHXList methods ***/

STDMETHODIMP_(IUnknown*)
CHXList::GetHead(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_pHead != NULL && m_pTail != NULL && m_nCount > 0);

    Node* pNode = m_pHead;

    IUnknown* punkItem = pNode->m_punkItem;
    punkItem->AddRef();

    return punkItem;
}

STDMETHODIMP_(IUnknown*)
CHXList::GetTail(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_pHead != NULL && m_pTail != NULL && m_nCount > 0);

    Node* pNode = m_pTail;

    IUnknown* punkItem = pNode->m_punkItem;
    punkItem->AddRef();

    return punkItem;
}



/*****************************************************************************
 *
 * CHXVectorIterator
 *
 *****************************************************************************/

CHXVectorIterator::CHXVectorIterator(CHXVector* pVector,
                                       UINT32      pos,
                                       HXBOOL        bHasOneRef) :
    m_pVector(pVector),
    m_ulPos(pos)
{
    m_ulRefCount = bHasOneRef ? 1 : 0;
}

CHXVectorIterator::~CHXVectorIterator(void)
{
    HX_ASSERT_VALID_PTR(this);
}

/*** IUnknown methods ***/

STDMETHODIMP
CHXVectorIterator::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXListIterator))
    {
        AddRef();
        *ppvObj = (IHXListIterator*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXVectorIteratorPrivate))
    {
        AddRef();
        *ppvObj = (IHXVectorIteratorPrivate*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXVectorIterator::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CHXVectorIterator::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXListIterator methods ***/

STDMETHODIMP_(HXBOOL)
CHXVectorIterator::HasItem(void)
{
    HX_ASSERT_VALID_PTR(this);

    return (m_ulPos < m_pVector->GetCount());
}

STDMETHODIMP_(HXBOOL)
CHXVectorIterator::IsEqual(IHXListIterator* pOther)
{
    HX_ASSERT_VALID_PTR(this);

    HXBOOL bIsEqual = FALSE;
    IHXVectorIteratorPrivate* pOtherPriv = NULL;

    if (pOther->QueryInterface(IID_IHXVectorIteratorPrivate, (void**)&pOtherPriv) == HXR_OK)
    {
        bIsEqual = (pOtherPriv->GetVector() == m_pVector && pOtherPriv->GetPos() == m_ulPos);
        pOtherPriv->Release();
    }

    return bIsEqual;
}

STDMETHODIMP_(HXBOOL)
CHXVectorIterator::MoveNext(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_pVector != NULL && m_ulPos < m_pVector->GetCount());

    m_ulPos++;
    return TRUE;
}

STDMETHODIMP_(HXBOOL)
CHXVectorIterator::MovePrev(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_pVector != NULL && m_ulPos > 0);

    if (m_ulPos == 0)
    {
        return FALSE;
    }

    m_ulPos--;
    return TRUE;
}

STDMETHODIMP_(IUnknown*)
CHXVectorIterator::GetItem(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_pVector != NULL);

    return m_pVector->GetAt(m_ulPos);
}

/*** IHXVectorIteratorPrivate methods ***/

STDMETHODIMP_(CHXVector*)
CHXVectorIterator::GetVector(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_pVector != NULL);

    return m_pVector;
}

STDMETHODIMP_(UINT32)
CHXVectorIterator::GetPos(void)
{
    return m_ulPos;
}

STDMETHODIMP
CHXVectorIterator::Reset(void)
{
    m_ulPos = INVALID_POS;
    return HXR_OK;
}

/*****************************************************************************
 *
 * CHXVector
 *
 *****************************************************************************/

CHXVector::CHXVector(IHXFastAlloc* pFastAlloc) :
    m_ulRefCount(0),
    m_ulGrow(4), //XXX: make mnemonic and provide accessors
    m_ulSize(0),
    m_ulAlloc(0),
    m_ppunkData(NULL),
    m_pFastAlloc(pFastAlloc)
{
    if (m_pFastAlloc)
    {
        m_pFastAlloc->AddRef();
    }
}

CHXVector::~CHXVector(void)
{
    HX_ASSERT_VALID_PTR(this);

    UINT32 i;
    for (i = 0; i < m_ulSize; i++)
    {
        m_ppunkData[i]->Release();
    }
    delete[] m_ppunkData;
    HX_RELEASE(m_pFastAlloc);
}

IUnknown*
CHXVector::GetAt(UINT32 pos) const
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(pos < m_ulSize);

    m_ppunkData[pos]->AddRef();
    return m_ppunkData[pos];
}

void
CHXVector::SetSize(UINT32 ulNewSize)
{
    HX_ASSERT_VALID_PTR(this);

    if (ulNewSize == 0)
    {
        m_ulSize = 0;
        m_ulAlloc = 0;
        delete[] m_ppunkData;
        m_ppunkData = NULL;
    }
    else if (m_ppunkData == NULL)
    {
        m_ppunkData = new IUnknown*[ ulNewSize ];
        m_ulAlloc = ulNewSize;
        m_ulSize = ulNewSize;
    }
    else
    {
        if (ulNewSize > m_ulAlloc)
        {
            UINT32      ulGrow;
            UINT32      ulNewAlloc;
            IUnknown**  ppunkNewData;

            ulGrow = m_ulGrow;
            if (ulGrow == 0)
            {
                // Default is to grow by 1/8 with bounds of [4,1024]
                ulGrow = HX_MIN(1024, HX_MAX(4, m_ulSize/8));
            }
            ulNewAlloc = HX_MAX(ulNewSize, m_ulAlloc + ulGrow);
            HX_ASSERT(ulNewAlloc > m_ulAlloc); // Catch overflow

            ppunkNewData = new IUnknown*[ ulNewAlloc ];
            memcpy(ppunkNewData, m_ppunkData, m_ulSize * sizeof(IUnknown*));
            delete[] m_ppunkData;
            m_ppunkData = ppunkNewData;
            m_ulAlloc = ulNewAlloc;
        }

        m_ulSize = ulNewSize;
    }
}

void
CHXVector::InsertAt(UINT32 pos, IUnknown* punkItem)
{
    UINT32 nOldSize = m_ulSize;
    SetSize(nOldSize+1);
    if (pos < nOldSize)
    {
        memmove(m_ppunkData+(pos+1), m_ppunkData+(pos),
                 (nOldSize-pos) * sizeof(IUnknown*));
    }
    punkItem->AddRef();
    m_ppunkData[pos] = punkItem;
}

IUnknown*
CHXVector::RemoveAt(UINT32 pos)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_ppunkData != NULL && m_ulSize > 0 && pos < m_ulSize);

    IUnknown* punkItem = m_ppunkData[pos];
    if (pos < m_ulSize-1)
    {
        memmove(m_ppunkData+(pos), m_ppunkData+(pos+1),
                 (m_ulSize-pos-1) * sizeof(IUnknown*));
    }
    m_ulSize--;

    return punkItem;
}

/*** IUnknown methods ***/

STDMETHODIMP
CHXVector::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXList))
    {
        AddRef();
        *ppvObj = (IHXList*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXVector::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CHXVector::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXList methods ***/

STDMETHODIMP_(HXBOOL)
CHXVector::IsEmpty(void)
{
    HX_ASSERT_VALID_PTR(this);

    return (m_ulSize == 0);
}

STDMETHODIMP_(UINT32)
CHXVector::GetCount(void)
{
    HX_ASSERT_VALID_PTR(this);

    return m_ulSize;
}

STDMETHODIMP_(IHXList*)
CHXVector::AsConst(void)
{
    IHXList* pConstList = new (m_pFastAlloc) CHXConstList(this);
    pConstList->AddRef();
    return pConstList;
}

STDMETHODIMP
CHXVector::InsertHead(IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(punkItem != NULL);

    punkItem->AddRef();
    InsertAt(0, punkItem);

    return HXR_OK;
}

STDMETHODIMP
CHXVector::InsertTail(IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(punkItem != NULL);

    InsertAt(m_ulSize, punkItem);

    return HXR_OK;
}

STDMETHODIMP
CHXVector::InsertBefore(IHXListIterator* pIter, IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(pIter != NULL && punkItem != NULL);

    IHXVectorIteratorPrivate* pIterPriv = NULL;
    pIter->QueryInterface(IID_IHXVectorIteratorPrivate, (void**)&pIterPriv);
    HX_ASSERT(pIterPriv != NULL);
    HX_ASSERT(pIterPriv->GetVector() == this && pIterPriv->GetPos() != INVALID_POS);
    UINT32 posIter = pIterPriv->GetPos();
    pIterPriv->Reset();
    pIterPriv->Release();

    InsertAt(posIter, punkItem);

    return HXR_OK;
}

STDMETHODIMP
CHXVector::InsertAfter(IHXListIterator* pIter, IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(pIter != NULL && punkItem != NULL);

    IHXVectorIteratorPrivate* pIterPriv = NULL;
    pIter->QueryInterface(IID_IHXVectorIteratorPrivate, (void**)&pIterPriv);
    HX_ASSERT(pIterPriv != NULL);
    HX_ASSERT(pIterPriv->GetVector() == this && pIterPriv->GetPos() != INVALID_POS);
    UINT32 posIter = pIterPriv->GetPos();
    pIterPriv->Reset();
    pIterPriv->Release();

    InsertAt(posIter+1, punkItem);

    return HXR_OK;
}

STDMETHODIMP
CHXVector::Replace(IHXListIterator* pIter, IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(pIter != NULL && punkItem != NULL);

    IHXVectorIteratorPrivate* pIterPriv = NULL;
    pIter->QueryInterface(IID_IHXVectorIteratorPrivate, (void**)&pIterPriv);
    HX_ASSERT(pIterPriv != NULL);
    HX_ASSERT(pIterPriv->GetVector() == this && pIterPriv->GetPos() != INVALID_POS);
    UINT32 posIter = pIterPriv->GetPos();
    pIterPriv->Reset();
    pIterPriv->Release();

    m_ppunkData[posIter]->Release();
    punkItem->AddRef();
    m_ppunkData[posIter] = punkItem;

    return HXR_OK;
}

STDMETHODIMP_(IUnknown*)
CHXVector::Remove(IHXListIterator* pIter)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(pIter != NULL);

    IHXVectorIteratorPrivate* pIterPriv = NULL;
    pIter->QueryInterface(IID_IHXVectorIteratorPrivate, (void**)&pIterPriv);
    HX_ASSERT(pIterPriv != NULL);
    HX_ASSERT(pIterPriv->GetVector() == this && pIterPriv->GetPos() != INVALID_POS);
    UINT32 posIter = pIterPriv->GetPos();
    pIterPriv->Reset();
    pIterPriv->Release();

    return RemoveAt(posIter);
}

STDMETHODIMP_(IUnknown*)
CHXVector::RemoveHead(void)
{
    HX_ASSERT_VALID_PTR(this);

    return RemoveAt(0);
}

STDMETHODIMP_(IUnknown*)
CHXVector::RemoveTail(void)
{
    HX_ASSERT_VALID_PTR(this);

    return RemoveAt(m_ulSize-1);
}

STDMETHODIMP_(IHXListIterator*)
CHXVector::Begin(void)
{
    HX_ASSERT_VALID_PTR(this);

    return new (m_pFastAlloc) CHXVectorIterator(this, 0, TRUE);
}

STDMETHODIMP_(IHXListIterator*)
CHXVector::End(void)
{
    HX_ASSERT_VALID_PTR(this);

    return new (m_pFastAlloc) CHXVectorIterator(this, m_ulSize, TRUE);
}

STDMETHODIMP_(IUnknown*)
CHXVector::GetHead(void)
{
    HX_ASSERT_VALID_PTR(this);

    if (m_ppunkData != NULL && m_ulSize > 0)
    {
        IUnknown* punkItem = m_ppunkData[0];

        if (punkItem)
        {
            punkItem->AddRef();
        }

        return punkItem;
    }
    else
    {
        return NULL;
    }
}

STDMETHODIMP_(IUnknown*)
CHXVector::GetTail(void)
{
    HX_ASSERT_VALID_PTR(this);

    if (m_ppunkData != NULL && m_ulSize > 0)
    {
        IUnknown* punkItem = m_ppunkData[m_ulSize-1];

        if (punkItem)
        {
            punkItem->AddRef();
        }

        return punkItem;
    }
    else
    {
        return NULL;
    }
}


/*****************************************************************************
 *
 * CHXRingBufferIterator
 *
 *****************************************************************************/

CHXRingBufferIterator::CHXRingBufferIterator(CHXRingBuffer* pRing,
                                       UINT32      pos,
                                       HXBOOL        bHasOneRef) :
    m_pRing(pRing),
    m_ulPos(pos)
{
    m_ulRefCount = bHasOneRef ? 1 : 0;
}

CHXRingBufferIterator::~CHXRingBufferIterator(void)
{
    HX_ASSERT_VALID_PTR(this);
}

/*** IUnknown methods ***/

STDMETHODIMP
CHXRingBufferIterator::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXListIterator))
    {
        AddRef();
        *ppvObj = (IHXListIterator*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRingBufferIteratorPrivate))
    {
        AddRef();
        *ppvObj = (IHXRingBufferIteratorPrivate*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXRingBufferIterator::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CHXRingBufferIterator::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXListIterator methods ***/

STDMETHODIMP_(HXBOOL)
CHXRingBufferIterator::HasItem(void)
{
    HX_ASSERT_VALID_PTR(this);

    return (m_pRing->HasItem(m_ulPos));
}

STDMETHODIMP_(HXBOOL)
CHXRingBufferIterator::IsEqual(IHXListIterator* pOther)
{
    HX_ASSERT_VALID_PTR(this);

    HXBOOL bIsEqual = FALSE;
    IHXRingBufferIteratorPrivate* pOtherPriv = NULL;

    if (pOther->QueryInterface(IID_IHXRingBufferIteratorPrivate, (void**)&pOtherPriv) == HXR_OK)
    {
        bIsEqual = (pOtherPriv->GetRingBuffer() == m_pRing && pOtherPriv->GetPos() == m_ulPos);
        pOtherPriv->Release();
    }

    return bIsEqual;
}

STDMETHODIMP_(HXBOOL)
CHXRingBufferIterator::MoveNext(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_pRing != NULL);

    if (m_ulPos == INVALID_POS)
    {
        m_ulPos = m_pRing->GetHeadPos();
    }
    else
    {
        m_ulPos = m_pRing->GetNextPos(m_ulPos);
    }

    if (!m_pRing->HasItem(m_ulPos))
    {
        return FALSE;
    }

    return TRUE;
}

STDMETHODIMP_(HXBOOL)
CHXRingBufferIterator::MovePrev(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_pRing != NULL);

    if (m_ulPos == INVALID_POS)
    {
        m_ulPos = m_pRing->GetTailPos();
    }
    else
    {
        m_ulPos = m_pRing->GetPrevPos(m_ulPos);
        if (!m_pRing->HasItem(m_ulPos))
        {
            return FALSE;
        }
    }

    return TRUE;
}

STDMETHODIMP_(IUnknown*)
CHXRingBufferIterator::GetItem(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_pRing != NULL);

    return m_pRing->GetAt(m_ulPos);
}

/*** IHXRingBufferIteratorPrivate methods ***/

STDMETHODIMP_(CHXRingBuffer*)
CHXRingBufferIterator::GetRingBuffer(void)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_pRing != NULL);

    return m_pRing;
}

STDMETHODIMP_(UINT32)
CHXRingBufferIterator::GetPos(void)
{
    return m_ulPos;
}

STDMETHODIMP
CHXRingBufferIterator::Reset(void)
{
    m_ulPos = INVALID_POS;
    return HXR_OK;
}

/*****************************************************************************
 *
 * CHXRingBuffer
 *
 *****************************************************************************/

CHXRingBuffer::CHXRingBuffer(IHXFastAlloc* pFastAlloc) :
    m_ulRefCount(0),
    m_ulGrow(8),
    m_ulCount(0),
    m_ulAlloc(0),
    m_ulHead(0),
    m_ulTail(0),
    m_ppunkData(NULL),
    m_pFastAlloc(pFastAlloc)
{
    if (m_pFastAlloc)
    {
        m_pFastAlloc->AddRef();
    }
}

CHXRingBuffer::~CHXRingBuffer(void)
{
    HX_ASSERT_VALID_PTR(this);

    UINT32 i;
    for (i = 0; i < m_ulAlloc; i++)
    {
        if (m_ppunkData[i])
        {
            m_ppunkData[i]->Release();
        }
    }
    delete[] m_ppunkData;
    HX_RELEASE(m_pFastAlloc);
}

IUnknown*
CHXRingBuffer::GetAt(UINT32 pos) const
{
    HX_ASSERT_VALID_PTR(this);

    if (m_ulCount && ((pos < m_ulTail) || (pos >= m_ulHead)))
    {
        m_ppunkData[pos]->AddRef();
        return m_ppunkData[pos];
    }

    return NULL;
}


UINT32
CHXRingBuffer::GetHeadPos()
{
    HX_ASSERT_VALID_PTR(this);
    return m_ulHead;
}


UINT32
CHXRingBuffer::GetTailPos()
{
    HX_ASSERT_VALID_PTR(this);
    return (m_ulTail ? m_ulTail-1 : m_ulAlloc - 1);
}


UINT32
CHXRingBuffer::GetNextPos(UINT32 pos)
{
    HX_ASSERT_VALID_PTR(this);
    UINT32 ulNext;

    ulNext = (pos == m_ulAlloc - 1) ? 0 : pos+1;

    if (ulNext < m_ulTail)
    {
        return ulNext;
    }
    else if ((ulNext != m_ulTail) && (ulNext > m_ulHead))
    {
        return ulNext;
    }

    return INVALID_POS;
}


UINT32
CHXRingBuffer::GetPrevPos(UINT32 pos)
{
    HX_ASSERT_VALID_PTR(this);
    UINT32 ulPrev;

    ulPrev = pos ? pos-1 : m_ulAlloc - 1;

    if (pos == m_ulHead)
    {
        return INVALID_POS;
    }
    else if ((ulPrev < m_ulTail) || (ulPrev >= m_ulHead))
    {
        return ulPrev;
    }

    return INVALID_POS;
}


HXBOOL
CHXRingBuffer::HasItem(UINT32 pos)
{
    if (!m_ulCount || (pos >= m_ulAlloc) ||
        ((pos >= m_ulTail) && (pos < m_ulHead)))
    {
        return FALSE;
    }

    return TRUE;
}


void
CHXRingBuffer::SetGrow(UINT32 ulGrow)
{
    m_ulGrow = ulGrow;
}


void
CHXRingBuffer::SetSize(UINT32 ulNewSize)
{
    HX_ASSERT_VALID_PTR(this);

    if (ulNewSize == 0)
    {
        UINT32 i;
        for (i = 0; i < m_ulAlloc; i++)
        {
            if (m_ppunkData[i])
            {
                m_ppunkData[i]->Release();
            }
        }
        delete[] m_ppunkData;
        m_ppunkData = NULL;
        m_ulAlloc = 0;
        m_ulCount = 0;
        m_ulHead = 0;
        m_ulTail= 0;
    }
    else
    {
        if (ulNewSize > m_ulAlloc)
        {
            UINT32      ulGrow;
            UINT32      ulNewAlloc;
            IUnknown**  ppunkNewData;

            ulGrow = m_ulGrow;
            if (ulGrow == 0)
            {
                // Default is to grow by 1/4 with bounds of [8,1024]
                ulGrow = HX_MIN(1024, HX_MAX(8, ulNewSize/4));
            }
            ulNewAlloc = HX_MAX(ulNewSize, m_ulAlloc + ulGrow);
            HX_ASSERT(ulNewAlloc > m_ulAlloc); // Catch overflow

            ppunkNewData = new IUnknown*[ ulNewAlloc ];

            memset((void*)ppunkNewData, 0, sizeof(IUnknown*) * ulNewAlloc);

            UINT32      i;
            UINT32      j=0;
            if (m_ppunkData)
            {
                if (m_ppunkData[m_ulHead])
                {
                    for (i = m_ulHead; (i < m_ulAlloc); i++)
                    {
                        ppunkNewData[j++] = m_ppunkData[i];
                    }

                    if (m_ulHead >= m_ulTail)
                    {
                        for (i = 0; (i < m_ulTail); i++)
                        {
                            ppunkNewData[j++] = m_ppunkData[i];
                        }
                    }
                }

                delete[] m_ppunkData;
            }

            m_ppunkData = ppunkNewData;
            m_ulAlloc = ulNewAlloc;
            m_ulHead = 0;
            m_ulTail = j;
        }
    }
}

void
CHXRingBuffer::InsertAt(UINT32 pos, IUnknown* punkItem)
{
    if (!m_ulAlloc)
    {
        SetSize(m_ulCount+1);
    }

    if (!m_ppunkData[m_ulTail])
    {
        if (pos <= m_ulTail)
        {
            UINT32 ulMove = m_ulTail - pos;
            memmove(m_ppunkData+(pos+1), m_ppunkData+(pos),
                     ulMove * sizeof(IUnknown*));
        }
        else if (pos >= m_ulHead)
        {
            // we know there has to be a wraparound or pos would
            // have been <= tail
            UINT32 ulMove = m_ulAlloc - pos - 1;
            IUnknown* punkTmp = m_ppunkData[m_ulAlloc - 1];
            memmove(m_ppunkData+(pos+1), m_ppunkData+(pos),
                     ulMove * sizeof(IUnknown*));

            ulMove = m_ulTail;
            memmove(m_ppunkData+1, m_ppunkData,
                     ulMove * sizeof(IUnknown*));

            m_ppunkData[0] = punkTmp;
        }
        else
        {
            HX_ASSERT(0);
            //printf("WARNING! INSERT TO INVALID REGION OF QUEUE\n");
        }

        punkItem->AddRef();
        m_ppunkData[pos] = punkItem;

        m_ulTail = (m_ulTail == m_ulAlloc - 1) ? 0 : m_ulTail+1;

        m_ulCount++;

        if (m_ulTail == m_ulHead)
        {
            SetSize(m_ulCount+1);
        }
    }
}


IUnknown*
CHXRingBuffer::RemoveAt(UINT32 pos)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(m_ppunkData != NULL && m_ulCount > 0 && HasItem(pos));

    if (!m_ulCount)
    {
        return NULL;
    }

    IUnknown* punkItem = m_ppunkData[pos];

    // Optimized for LIFO and FIFO
    if (pos == m_ulHead)
    {
        m_ppunkData[m_ulHead] = NULL;

        m_ulHead = (m_ulHead + 1 == m_ulAlloc) ? 0 : m_ulHead+1;

        m_ulCount--;
    }
    else
    {
        UINT32 ulNewTail = m_ulTail ? m_ulTail-1 : m_ulAlloc - 1;
        if (pos < ulNewTail)
        {
            memmove(m_ppunkData+(pos), m_ppunkData+(pos+1),
                     (ulNewTail-pos) * sizeof(IUnknown*));
        }
        else if (pos != ulNewTail)
        {
            memmove(m_ppunkData+(pos), m_ppunkData+(pos+1),
                     (m_ulAlloc-pos-1) * sizeof(IUnknown*));

            m_ppunkData[m_ulAlloc-1] = m_ppunkData[0];

            memmove(m_ppunkData, m_ppunkData+1,
                     (ulNewTail) * sizeof(IUnknown*));
        }

        m_ulTail = ulNewTail;
        m_ppunkData[m_ulTail] = NULL;
        m_ulCount--;
    }
    
    return punkItem;
}

/*** IUnknown methods ***/

STDMETHODIMP
CHXRingBuffer::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXList))
    {
        AddRef();
        *ppvObj = (IHXList*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXRingBuffer::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CHXRingBuffer::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXList methods ***/

STDMETHODIMP_(HXBOOL)
CHXRingBuffer::IsEmpty(void)
{
    HX_ASSERT_VALID_PTR(this);

    return (m_ulCount == 0);
}

STDMETHODIMP_(UINT32)
CHXRingBuffer::GetCount(void)
{
    HX_ASSERT_VALID_PTR(this);

    return m_ulCount;
}

STDMETHODIMP_(IHXList*)
CHXRingBuffer::AsConst(void)
{
    IHXList* pConstList = new (m_pFastAlloc) CHXConstList(this);
    pConstList->AddRef();
    return pConstList;
}

STDMETHODIMP
CHXRingBuffer::InsertHead(IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(punkItem != NULL);

    InsertAt(m_ulHead, punkItem);

    return HXR_OK;
}

STDMETHODIMP
CHXRingBuffer::InsertTail(IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(punkItem != NULL);

    InsertAt(m_ulTail, punkItem);

    return HXR_OK;
}

STDMETHODIMP
CHXRingBuffer::InsertBefore(IHXListIterator* pIter, IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(pIter != NULL && punkItem != NULL);

    IHXRingBufferIteratorPrivate* pIterPriv = NULL;
    pIter->QueryInterface(IID_IHXRingBufferIteratorPrivate, (void**)&pIterPriv);
    HX_ASSERT(pIterPriv != NULL);
    HX_ASSERT(pIterPriv->GetRingBuffer() == this && pIterPriv->GetPos() != INVALID_POS);
    UINT32 posIter = pIterPriv->GetPos();
    pIterPriv->Reset();
    pIterPriv->Release();

    InsertAt(posIter, punkItem);

    return HXR_OK;
}

STDMETHODIMP
CHXRingBuffer::InsertAfter(IHXListIterator* pIter, IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(pIter != NULL && punkItem != NULL);

    IHXRingBufferIteratorPrivate* pIterPriv = NULL;
    pIter->QueryInterface(IID_IHXRingBufferIteratorPrivate, (void**)&pIterPriv);
    HX_ASSERT(pIterPriv != NULL);
    HX_ASSERT(pIterPriv->GetRingBuffer() == this && pIterPriv->GetPos() != INVALID_POS);
    UINT32 posIter = pIterPriv->GetPos();
    pIterPriv->Reset();
    pIterPriv->Release();

    InsertAt(((posIter == m_ulAlloc - 1) ? 0 : posIter+1), punkItem);

    return HXR_OK;
}

STDMETHODIMP
CHXRingBuffer::Replace(IHXListIterator* pIter, IUnknown* punkItem)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(pIter != NULL && punkItem != NULL);

    IHXRingBufferIteratorPrivate* pIterPriv = NULL;
    pIter->QueryInterface(IID_IHXRingBufferIteratorPrivate, (void**)&pIterPriv);
    HX_ASSERT(pIterPriv != NULL);
    HX_ASSERT(pIterPriv->GetRingBuffer() == this && pIterPriv->GetPos() != INVALID_POS);
    UINT32 posIter = pIterPriv->GetPos();
    pIterPriv->Reset();
    pIterPriv->Release();

    m_ppunkData[posIter]->Release();
    punkItem->AddRef();
    m_ppunkData[posIter] = punkItem;

    return HXR_OK;
}

STDMETHODIMP_(IUnknown*)
CHXRingBuffer::Remove(IHXListIterator* pIter)
{
    HX_ASSERT_VALID_PTR(this);
    HX_ASSERT(pIter != NULL);

    IHXRingBufferIteratorPrivate* pIterPriv = NULL;
    pIter->QueryInterface(IID_IHXRingBufferIteratorPrivate, (void**)&pIterPriv);
    HX_ASSERT(pIterPriv != NULL);
    HX_ASSERT(pIterPriv->GetRingBuffer() == this && pIterPriv->GetPos() != INVALID_POS);
    UINT32 posIter = pIterPriv->GetPos();
    pIterPriv->Reset();
    pIterPriv->Release();

    return RemoveAt(posIter);
}

STDMETHODIMP_(IUnknown*)
CHXRingBuffer::RemoveHead(void)
{
    HX_ASSERT_VALID_PTR(this);

    return RemoveAt(m_ulHead);
}

STDMETHODIMP_(IUnknown*)
CHXRingBuffer::RemoveTail(void)
{
    HX_ASSERT_VALID_PTR(this);

    return RemoveAt(m_ulTail ? m_ulTail - 1 : m_ulAlloc - 1);
}

STDMETHODIMP_(IHXListIterator*)
CHXRingBuffer::Begin(void)
{
    HX_ASSERT_VALID_PTR(this);

    return new (m_pFastAlloc) CHXRingBufferIterator(this, m_ulHead, TRUE);
}

STDMETHODIMP_(IHXListIterator*)
CHXRingBuffer::End(void)
{
    HX_ASSERT_VALID_PTR(this);
    UINT32 ulEnd = m_ulTail ? m_ulTail-1 : m_ulAlloc-1;
    return new (m_pFastAlloc) CHXRingBufferIterator(this, ulEnd, TRUE);
}

STDMETHODIMP_(IUnknown*)
CHXRingBuffer::GetHead(void)
{
    HX_ASSERT_VALID_PTR(this);

    if (m_ppunkData != NULL && m_ulCount > 0)
    {
        IUnknown* punkItem = m_ppunkData[m_ulHead];

        if (punkItem)
        {
            punkItem->AddRef();
        }

        return punkItem;
    }
    else
    {
        return NULL;
    }
}

STDMETHODIMP_(IUnknown*)
CHXRingBuffer::GetTail(void)
{
    HX_ASSERT_VALID_PTR(this);

    if (m_ppunkData != NULL && m_ulCount > 0)
    {
        IUnknown* punkItem = m_ppunkData[m_ulTail ? m_ulTail-1 : m_ulAlloc-1];

        if (punkItem)
        {
            punkItem->AddRef();
        }

        return punkItem;
    }
    else
    {
        return NULL;
    }
}



