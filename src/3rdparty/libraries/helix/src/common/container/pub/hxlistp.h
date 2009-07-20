/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxlistp.h,v 1.2 2005/03/14 19:33:48 bobclark Exp $
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

#ifndef _HXLISTP_H_
#define _HXLISTP_H_

#include "hxcomm.h"    // IHXFastAlloc
#include "ihxlist.h"

/*
 * Three implementations of IHXList are provided:
 *   - CHXList implements IHXList with a doubly-linked list. Requires
 *     an alloc per element inserted to create the "node".
 *   - CHXVector implements IHXList with a vector. This is 
 *     straightforward and works well for small static lists. 
 *     Removal from head copies every element of the list.
 *   - CHXRingBuffer implements IHXList with an array used as a 
 *     ring buffer.
 *
 * The choice of implementation largely depends on the expected size and
 * behaviour of the list.  Vector is probably the better choice for headers
 * in a message, while linked or ring buffer is a better choice for a queue.
 * Linked will offer more efficient insertion/removal from an arbitrary spot
 * in the list while ring buffer will be better with FIFO/LIFO.
 */

/*
 * This is a const wrapper for an IHXList.  COM does not have a practical
 * method of using const objects (because the IUnknown methods are not const)
 * so we use a wrapper instead.
 */
class CHXConstList : public IHXList
{
public:
    CHXConstList(IHXList* plist);

    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    // IHXList
    STDMETHOD_(HXBOOL,IsEmpty)            (THIS);
    STDMETHOD_(UINT32,GetCount)         (THIS);
    STDMETHOD_(IHXList*,AsConst)        (THIS);
    STDMETHOD(InsertHead)               (THIS_ IUnknown* punkItem);
    STDMETHOD(InsertTail)               (THIS_ IUnknown* punkItem);
    STDMETHOD(InsertBefore)             (THIS_ IHXListIterator* pIter, IUnknown* punkItem);
    STDMETHOD(InsertAfter)              (THIS_ IHXListIterator* pIter, IUnknown* punkItem);
    STDMETHOD(Replace)                  (THIS_ IHXListIterator* pIter, IUnknown* punkItem);
    STDMETHOD_(IUnknown*,Remove)        (THIS_ IHXListIterator* pIter);
    STDMETHOD_(IUnknown*,RemoveHead)    (THIS);
    STDMETHOD_(IUnknown*,RemoveTail)    (THIS);
    STDMETHOD_(IHXListIterator*,Begin) (THIS);
    STDMETHOD_(IHXListIterator*,End)   (THIS);

    STDMETHOD_(IUnknown*,GetHead)       (THIS);
    STDMETHOD_(IUnknown*,GetTail)       (THIS);

    FAST_CACHE_MEM

protected:
    IHXList*           m_plist;

private:
    ULONG32             m_ulRefCount;

    virtual ~CHXConstList(void);
};

class CHXList : public IHXList
{
public:
    CHXList(IHXFastAlloc* pFastAlloc);
    virtual ~CHXList(void);

    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    // IHXList
    STDMETHOD_(HXBOOL,IsEmpty)            (THIS);
    STDMETHOD_(UINT32,GetCount)         (THIS);
    STDMETHOD_(IHXList*,AsConst)       (THIS);
    STDMETHOD(InsertHead)               (THIS_ IUnknown* punkItem);
    STDMETHOD(InsertTail)               (THIS_ IUnknown* punkItem);
    STDMETHOD(InsertBefore)             (THIS_ IHXListIterator* pIter, IUnknown* punkItem);
    STDMETHOD(InsertAfter)              (THIS_ IHXListIterator* pIter, IUnknown* punkItem);
    STDMETHOD(Replace)                  (THIS_ IHXListIterator* pIter, IUnknown* punkItem);
    STDMETHOD_(IUnknown*,Remove)        (THIS_ IHXListIterator* pIter);
    STDMETHOD_(IUnknown*,RemoveHead)    (THIS);
    STDMETHOD_(IUnknown*,RemoveTail)    (THIS);
    STDMETHOD_(IHXListIterator*,Begin) (THIS);
    STDMETHOD_(IHXListIterator*,End)   (THIS);

    // IHXList
    STDMETHOD_(IUnknown*,GetHead)       (THIS);
    STDMETHOD_(IUnknown*,GetTail)       (THIS);

    FAST_CACHE_MEM

public:
    class Node
    {
    public:
        Node(Node* pPrev, Node* pNext, IUnknown* punkItem) : m_pPrev(pPrev), m_pNext(pNext), m_punkItem(punkItem) {}
        ~Node(void) {}
        FAST_CACHE_MEM
        Node*       m_pPrev;
        Node*       m_pNext;
        IUnknown*   m_punkItem;
    };

protected:
    ULONG32             m_ulRefCount;
    Node*               m_pHead;
    Node*               m_pTail;
    UINT32              m_nCount;
    IHXFastAlloc*      m_pFastAlloc;
};

// IHXListIteratorPrivate: 2d3f1b24-6e4e-4fdc-9e53-4b9bd73f3fed
DEFINE_GUID(IID_IHXListIteratorPrivate, 0x2d3f1b24, 0x6e4e, 0x4fdc, 0x9e, 0x53, 0x4b, 0x9b, 0xd7, 0x3f, 0x3f, 0xed);

#undef  INTERFACE
#define INTERFACE   IHXListIteratorPrivate

DECLARE_INTERFACE_(IHXListIteratorPrivate, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;
    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    // IHXListIteratorPrivate
    STDMETHOD_(CHXList*,GetList)       (THIS) PURE;
    STDMETHOD_(CHXList::Node*,GetNode) (THIS) PURE;
    STDMETHOD(Reset)                    (THIS) PURE;
};

class CHXListIterator : public IHXListIterator,
                         public IHXListIteratorPrivate
{
public:
    CHXListIterator(CHXList* plist, CHXList::Node* pNode);
    virtual ~CHXListIterator(void);

    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    // IHXListIterator
    STDMETHOD_(HXBOOL,HasItem)            (THIS);
    STDMETHOD_(HXBOOL,IsEqual)            (THIS_ IHXListIterator* pOther);
    STDMETHOD_(HXBOOL,MoveNext)           (THIS);
    STDMETHOD_(HXBOOL,MovePrev)           (THIS);
    STDMETHOD_(IUnknown*,GetItem)       (THIS);

    // IHXListIteratorPrivate
    STDMETHOD_(CHXList*,GetList)       (THIS);
    STDMETHOD_(CHXList::Node*,GetNode) (THIS);
    STDMETHOD(Reset)                    (THIS);

    FAST_CACHE_MEM

private:
    ULONG32             m_ulRefCount;
    CHXList*           m_plist;
    CHXList::Node*     m_pNode;
};

class CHXVector : public IHXList
{
public:
    CHXVector(IHXFastAlloc* pFastAlloc);
    virtual ~CHXVector(void);

    IUnknown*   GetAt(UINT32 pos) const;
    void        SetSize(UINT32 ulNewSize);
    void        InsertAt(UINT32 pos, IUnknown* punkItem);
    IUnknown*   RemoveAt(UINT32 pos);

    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    // IHXList
    STDMETHOD_(HXBOOL,IsEmpty)            (THIS);
    STDMETHOD_(UINT32,GetCount)         (THIS);
    STDMETHOD_(IHXList*,AsConst)       (THIS);
    STDMETHOD(InsertHead)               (THIS_ IUnknown* punkItem);
    STDMETHOD(InsertTail)               (THIS_ IUnknown* punkItem);
    STDMETHOD(InsertBefore)             (THIS_ IHXListIterator* pIter, IUnknown* punkItem);
    STDMETHOD(InsertAfter)              (THIS_ IHXListIterator* pIter, IUnknown* punkItem);
    STDMETHOD(Replace)                  (THIS_ IHXListIterator* pIter, IUnknown* punkItem);
    STDMETHOD_(IUnknown*,Remove)        (THIS_ IHXListIterator* pIter);
    STDMETHOD_(IUnknown*,RemoveHead)    (THIS);
    STDMETHOD_(IUnknown*,RemoveTail)    (THIS);
    STDMETHOD_(IHXListIterator*,Begin) (THIS);
    STDMETHOD_(IHXListIterator*,End)   (THIS);

    STDMETHOD_(IUnknown*,GetHead)       (THIS);
    STDMETHOD_(IUnknown*,GetTail)       (THIS);

    FAST_CACHE_MEM

private:
    ULONG32             m_ulRefCount;
    UINT32              m_ulGrow;
    UINT32              m_ulSize;
    UINT32              m_ulAlloc;
    IUnknown**          m_ppunkData;
    IHXFastAlloc*      m_pFastAlloc;
};

// IHXVectorIteratorPrivate: 58b7d31f-2261-4608-a634-4e98dc8fa84f
DEFINE_GUID(IID_IHXVectorIteratorPrivate, 0x58b7d31f, 0x2261, 0x4608, 0xa6, 0x34, 0x4e, 0x98, 0xdc, 0x8f, 0xa8, 0x4f);

#undef  INTERFACE
#define INTERFACE   IHXVectorIteratorPrivate

DECLARE_INTERFACE_(IHXVectorIteratorPrivate, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;
    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    // IHXVectorIteratorPrivate
    STDMETHOD_(CHXVector*,GetVector)   (THIS) PURE;
    STDMETHOD_(UINT32,GetPos)           (THIS) PURE;
    STDMETHOD(Reset)                    (THIS) PURE;
};

class CHXVectorIterator : public IHXListIterator,
                           public IHXVectorIteratorPrivate
{
public:
    CHXVectorIterator(CHXVector* pVector, UINT32 pos, HXBOOL bHasOneRef);
    virtual ~CHXVectorIterator(void);

    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    // IHXListIterator
    STDMETHOD_(HXBOOL,HasItem)            (THIS);
    STDMETHOD_(HXBOOL,IsEqual)            (THIS_ IHXListIterator* pOther);
    STDMETHOD_(HXBOOL,MoveNext)           (THIS);
    STDMETHOD_(HXBOOL,MovePrev)           (THIS);
    STDMETHOD_(IUnknown*,GetItem)       (THIS);

    // IHXVectorIteratorPrivate
    STDMETHOD_(CHXVector*,GetVector)   (THIS);
    STDMETHOD_(UINT32,GetPos)           (THIS);
    STDMETHOD(Reset)                    (THIS);

    FAST_CACHE_MEM

private:
    ULONG32             m_ulRefCount;
    CHXVector*         m_pVector;
    UINT32              m_ulPos;
};

class CHXRingBuffer : public IHXList
{
public:
    CHXRingBuffer(IHXFastAlloc* pFastAlloc);
    virtual ~CHXRingBuffer(void);

    IUnknown*   GetAt(UINT32 pos) const;
    void        SetSize(UINT32 ulNewSize);
    void        SetGrow(UINT32 ulGrow);
    void        InsertAt(UINT32 pos, IUnknown* punkItem);
    IUnknown*   RemoveAt(UINT32 pos);
    HXBOOL        HasItem(UINT32 pos);
    UINT32      GetNextPos(UINT32 pos);
    UINT32      GetPrevPos(UINT32 pos);
    UINT32      GetHeadPos();
    UINT32      GetTailPos();

    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    // IHXList
    STDMETHOD_(HXBOOL,IsEmpty)            (THIS);
    STDMETHOD_(UINT32,GetCount)         (THIS);
    STDMETHOD_(IHXList*,AsConst)       (THIS);
    STDMETHOD(InsertHead)               (THIS_ IUnknown* punkItem);
    STDMETHOD(InsertTail)               (THIS_ IUnknown* punkItem);
    STDMETHOD(InsertBefore)             (THIS_ IHXListIterator* pIter, IUnknown* punkItem);
    STDMETHOD(InsertAfter)              (THIS_ IHXListIterator* pIter, IUnknown* punkItem);
    STDMETHOD(Replace)                  (THIS_ IHXListIterator* pIter, IUnknown* punkItem);
    STDMETHOD_(IUnknown*,Remove)        (THIS_ IHXListIterator* pIter);
    STDMETHOD_(IUnknown*,RemoveHead)    (THIS);
    STDMETHOD_(IUnknown*,RemoveTail)    (THIS);
    STDMETHOD_(IHXListIterator*,Begin) (THIS);
    STDMETHOD_(IHXListIterator*,End)   (THIS);

    STDMETHOD_(IUnknown*,GetHead)       (THIS);
    STDMETHOD_(IUnknown*,GetTail)       (THIS);

    FAST_CACHE_MEM

private:
    ULONG32             m_ulRefCount;
    UINT32              m_ulGrow;
    UINT32              m_ulCount;
    UINT32              m_ulAlloc;
    UINT32              m_ulHead;
    UINT32              m_ulTail;
    IUnknown**          m_ppunkData;
    IHXFastAlloc*      m_pFastAlloc;
};

// IHXRingBufferIteratorPrivate: 87d7ca44-7a75-11d7-8b4a-00d0b71035fe
DEFINE_GUID(IID_IHXRingBufferIteratorPrivate, 0x87d7ca44, 0x7a75, 0x11d7, 0x8b, 0x4a, 0x00, 0xd0, 0xb7, 0x10, 0x35, 0xfe);

#undef  INTERFACE
#define INTERFACE   IHXRingBufferIteratorPrivate

DECLARE_INTERFACE_(IHXRingBufferIteratorPrivate, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;
    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    // IHXRingBufferIteratorPrivate
    STDMETHOD_(CHXRingBuffer*,GetRingBuffer)   (THIS) PURE;
    STDMETHOD_(UINT32,GetPos)           (THIS) PURE;
    STDMETHOD(Reset)                    (THIS) PURE;
};

class CHXRingBufferIterator : public IHXListIterator,
                           public IHXRingBufferIteratorPrivate
{
public:
    CHXRingBufferIterator(CHXRingBuffer* pRing, UINT32 pos, HXBOOL bHasOneRef);
    virtual ~CHXRingBufferIterator(void);

    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    // IHXListIterator
    STDMETHOD_(HXBOOL,HasItem)            (THIS);
    STDMETHOD_(HXBOOL,IsEqual)            (THIS_ IHXListIterator* pOther);
    STDMETHOD_(HXBOOL,MoveNext)           (THIS);
    STDMETHOD_(HXBOOL,MovePrev)           (THIS);
    STDMETHOD_(IUnknown*,GetItem)       (THIS);

    // IHXRingBufferIteratorPrivate
    STDMETHOD_(CHXRingBuffer*,GetRingBuffer)   (THIS);
    STDMETHOD_(UINT32,GetPos)           (THIS);
    STDMETHOD(Reset)                    (THIS);

    FAST_CACHE_MEM

private:
    ULONG32             m_ulRefCount;
    CHXRingBuffer*     m_pRing;
    UINT32              m_ulPos;
};


#endif /* ndef _HXLISTP_H_ */
