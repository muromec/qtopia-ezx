/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxslist.h,v 1.13 2007/07/06 20:35:02 jfinnecy Exp $
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

#ifndef HXLIST_H_
#define HXLIST_H_

#include "hxcom.h"
#include "hxstring.h"
#include "hxassert.h"

typedef void* LISTPOSITION;

// Define a function type which compares list elements
// This function should have similar semantics to strcmp().
// It should return return:
//     < 0    if pValue1 is "less" than pValue2
//       0    if pValue1 is "equal" to pValue2
//     > 0    if pValue1 is "greater" than pValue2
//
typedef int (*CHXSimpleListCompareFunc)(void* pValue1, void* pValue2);

//
// CHXSimpleList
//

class CHXSimpleList {
public:
    CHXSimpleList();
    
    virtual ~CHXSimpleList();
				// TRUE if list is internally consistent
    virtual HXBOOL IsPtrListValid();
    
				// return number of elements in list
    int GetCount() const;
				// return TRUE if number of elements == 0
    HXBOOL IsEmpty() const;
				// poke at first element
    void*& GetHead();
				// peek at first element
    void* GetHead() const;
				// poke at last element
    void*& GetTail();
				// peek at last element
    void* GetTail() const;

				// remove first element
    void* RemoveHead();
				// remove last element
    void* RemoveTail();

				// insert value before first element; return pos at new element
    LISTPOSITION AddHead(void* value);
				// insert value after last element; return pos at new element
    LISTPOSITION AddTail(void* value);

				// insert list before first element
    void AddHead(CHXSimpleList* pList);
				// insert list after last element
    void AddTail(CHXSimpleList* pTail);

				// clear all elements from the list
    virtual void RemoveAll();

				// get LISTPOSITION at start of list
    LISTPOSITION GetHeadPosition() const;
				// get LISTPOSITION at end of list
    LISTPOSITION GetTailPosition() const;
				// return value at current position and incr
    void*& GetNext(LISTPOSITION& pos);
				// return value at current position and incr
    void* GetNext(LISTPOSITION& pos) const;
				// return value at current position and decr
    void*& GetPrev(LISTPOSITION& pos);
				// return value at current position and decr
    void* GetPrev(LISTPOSITION& pos) const;
				// incr and return value at current position 
    void*& GetAtNext(LISTPOSITION& pos);
				// incr and return value at current position
    void* GetAtNext(LISTPOSITION& pos) const;
				// decr and return value at current position
    void*& GetAtPrev(LISTPOSITION& pos);
				// decr and return value at current position
    void* GetAtPrev(LISTPOSITION& pos) const;

				// get value at LISTPOSITION
    void*& GetAt(LISTPOSITION pos);
				// get value at LISTPOSITION
    void* GetAt(LISTPOSITION pos) const;
				// set value at LISTPOSITION
    void SetAt(LISTPOSITION pos, void* value);
				// remove value at LISTPOSITION
    virtual LISTPOSITION RemoveAt(LISTPOSITION pos);

				// insert before LISTPOSITION; return pos at new element
    virtual LISTPOSITION InsertBefore(LISTPOSITION pos, void* value);
				// insert after LISTPOSITION; return pos at new element
    virtual LISTPOSITION InsertAfter(LISTPOSITION pos, void* value);
    
				// search for value in list
    virtual LISTPOSITION Find(void* value, LISTPOSITION start=NULL) const;
				// get the LISTPOSITION for element at index
    LISTPOSITION FindIndex(int index) const;
    int          GetIndex(LISTPOSITION pos) const;

    typedef HXBOOL (*ConditionFunc)(void* pUser, void* pData);

    LISTPOSITION ForEach(LISTPOSITION start, LISTPOSITION end, void* pUser,
			 ConditionFunc func);

    class Iterator 
    {

	friend class CHXSimpleList;
    friend class CHXStringList;
    public:
	    Iterator();
				    // increment
	    Iterator& operator++();
				    // comparison
	    HXBOOL operator==(const Iterator& iter) const;
				    // comparison
	    HXBOOL operator!=(const Iterator& iter) const;
				    // get value
	    void* operator*();

    private:
	    Iterator(CHXSimpleList* pList, LISTPOSITION pos);

	    CHXSimpleList* m_pList;
	    LISTPOSITION m_pos;
    };

				// return iterator pointing to start of list
    Iterator Begin();
				// return iterator pointing to end of list
    Iterator End();
                // remove at iter and return iter at next elem (or end)
    virtual Iterator RemoveAt(Iterator iter);
                // insert before iter and return iter at new elem
    virtual Iterator InsertBefore(Iterator iter, void* value);
                // insert after iter and return iter at new elem
    virtual Iterator InsertAfter(Iterator iter, void* value);

    struct CNode 
    {
	    CNode* m_prev;
	    CNode* m_next;
	    void* m_value;

	    CNode() : m_prev(NULL), m_next(NULL), m_value(NULL) {}
	    CNode(void* value) : m_prev(NULL), m_next(NULL), m_value(value) {}
	    void* GetValue() const { return m_value; }
	    void*& GetValue() { return m_value; }
	    void SetValue(void* value) { m_value = value; }
	    CNode* GetPrev() const { return m_prev; }
	    CNode* GetNext() const { return m_next; }
	    void SetPrev(CNode* pPrev) { m_prev = pPrev; }
	    void SetNext(CNode* pNext) { m_next = pNext; }
    };

    // If _DEBUG, Dump() will do a bunch of printf()'s...
    virtual void Dump(const char* label = "Dump: ") const;

    LISTPOSITION FindMinOrMax(CHXSimpleListCompareFunc func, HXBOOL bMax = FALSE, LISTPOSITION startPos = NULL);
    void         InsertionSort(CHXSimpleListCompareFunc func, HXBOOL bDescending = FALSE);
    void         InsertionCoSort(CHXSimpleListCompareFunc func, CHXSimpleList* pCoSortedList, HXBOOL bDescending = FALSE);

protected:
    CNode* CreateNode(void* value);

    typedef HXBOOL (*ConditionNodeFunc)(void* pUser, const CNode* pNode);

    LISTPOSITION ForEach(LISTPOSITION start, LISTPOSITION end, void* pUser,
			 ConditionNodeFunc func) const;

    // Remove an item from the list without doing deallocations of
    // contained data.
    CNode* RemoveNode(CNode* pNode);

private:
				// number of elements in the list
    int m_nelems;
				// pointer to head node
    CNode* m_pHead;
				// pointer to tail node
    CNode* m_pTail;

    inline static const void* const& _nil()
	{
	    static const void* const m_nil = NULL;
	    return (const void*&)m_nil;
	}
};

//
// CHXStringList
//

class CHXString;

class CHXStringList : public CHXSimpleList 
{
public:
    virtual ~CHXStringList() { RemoveAll(); }

    // find a string in the list
    LISTPOSITION FindString(const char* pString,
			    LISTPOSITION start=NULL,
			    HXBOOL caseSensitive=TRUE);

    // find a string that starts with 'pPrefix'
    LISTPOSITION FindPrefixSubstring(const char* pPrefix,
			    LISTPOSITION start=NULL,
			    HXBOOL caseSensitive=TRUE);

    // add string at head
    LISTPOSITION AddHeadString(const char* pString);
    // add string at tail
    LISTPOSITION AddTailString(const char* pTail);

    // add string in sorted alpha order.
    // NOTE: The 'caseSensitive' flag defaults to FALSE to maintain
    //       backwards compat with previous incarnations of this class.
    LISTPOSITION AddStringAlphabetic(const char* pString,
                                     HXBOOL caseSensitive=FALSE);
    // get the next string in the list
    CHXString* GetNext(LISTPOSITION& pos) const;

    // clear all elements from the list
    virtual void RemoveAll();

    // override so we can delete the contained CHXString*
    virtual LISTPOSITION RemoveAt(LISTPOSITION pos);
    virtual Iterator RemoveAt(Iterator iter);

    // remove head string and free mem
    void RemoveHeadString();
    // remove tail string and free mem
    void RemoveTailString();

    void      Sort(HXBOOL bCaseSensitive = TRUE, HXBOOL bDescending = FALSE);
    void      CoSort(CHXStringList* pCoSortedList, HXBOOL bCaseSensitive = TRUE, HXBOOL bDescending = FALSE);
    CHXString Join(const char* pszDelim = NULL);

    // If _DEBUG, Dump() will do a bunch of printf()'s...
    virtual void Dump(const char* label = "Dump: ") const;

private:

    CHXSimpleList* m_pStrings;
};

//
// CHXSimpleList inline methods
//    

// return number of elements in list
inline int 
CHXSimpleList::GetCount() const
{
    return m_nelems;
}

// return TRUE if number of elements == 0
inline HXBOOL 
CHXSimpleList::IsEmpty() const
{
    return m_nelems == 0;
}

// poke at first element
inline void*& 
CHXSimpleList::GetHead()
{
    HX_ASSERT(m_pHead != NULL);
    return m_pHead->GetValue();
}

// peek at first element
inline void* 
CHXSimpleList::GetHead() const
{
    HX_ASSERT(m_pHead != NULL);
    return m_pHead->GetValue();
}

// poke at last element
inline void*& 
CHXSimpleList::GetTail()
{
    HX_ASSERT(m_pTail != NULL);
    return m_pTail->GetValue();
}

// peek at last element
inline void* 
CHXSimpleList::GetTail() const
{
    HX_ASSERT(m_pTail != NULL);
    return m_pTail->GetValue();
}

inline LISTPOSITION
CHXSimpleList::RemoveAt(LISTPOSITION pos)
{
    HX_ASSERT(pos);
    if (! pos) return NULL;
    return (LISTPOSITION)RemoveNode((CNode*)pos);
}

// remove first element
inline void* 
CHXSimpleList::RemoveHead()
{
    HX_ASSERT(m_pHead != NULL);
    void* value = m_pHead->GetValue();

    // Don't call RemoveAt() - that's a virtual method that subclasses
    // might use to deallocate the data contained in this item, but we
    // want to return that data to the caller
    (void)RemoveNode(m_pHead);
    return value;
}

// remove last element
inline void* 
CHXSimpleList::RemoveTail()
{
    HX_ASSERT(m_pTail != NULL);
    void* value = m_pTail->GetValue();

    // Don't call RemoveAt() - that's a virtual method that subclasses
    // might use to deallocate the data contained in this item, but we
    // want to return that data to the caller
    (void)RemoveNode(m_pTail);
    return value;
}

// insert value before first element and return pos at new element
inline LISTPOSITION 
CHXSimpleList::AddHead(void* value)
{
    return InsertBefore((LISTPOSITION)m_pHead, value);
}

// insert value after last element and return pos at new element
inline LISTPOSITION 
CHXSimpleList::AddTail(void* value)
{
    return InsertAfter((LISTPOSITION)m_pTail, value);
}

// get LISTPOSITION at start of list
inline LISTPOSITION 
CHXSimpleList::GetHeadPosition() const
{
    return (LISTPOSITION)m_pHead;
}

// get LISTPOSITION at end of list
inline LISTPOSITION 
CHXSimpleList::GetTailPosition() const
{
    return (LISTPOSITION)m_pTail;
}

// return iterator pointing to start of list
inline CHXSimpleList::Iterator 
CHXSimpleList::Begin()
{
    return Iterator(this, m_pHead);
}
// return iterator pointing to end of list
inline CHXSimpleList::Iterator 
CHXSimpleList::End()
{
    return Iterator(this, NULL);
}

///
/// CHXSimpleList::Iterator methods
///
inline 	
CHXSimpleList::Iterator::Iterator()
    : m_pList(0),
      m_pos(NULL)
{
}

inline 	
CHXSimpleList::Iterator::Iterator(CHXSimpleList* pList, LISTPOSITION pos)
    : m_pList(pList),
      m_pos(pos)
{
}

// increment
inline CHXSimpleList::Iterator& 
CHXSimpleList::Iterator::operator++()
{
    HX_ASSERT (m_pos);
    (void)m_pList->GetNext(m_pos);
    return *this;
}

// comparison
inline HXBOOL 
CHXSimpleList::Iterator::operator==(const Iterator& iter) const
{
    return m_pList == iter.m_pList && m_pos == iter.m_pos;
}

// comparison
inline HXBOOL 
CHXSimpleList::Iterator::operator!=(const Iterator& iter) const
{
    return !operator==(iter);
}

// get value
inline void* 
CHXSimpleList::Iterator::operator*()
{
    HX_ASSERT (m_pList && m_pos);
    return m_pList->GetAt(m_pos);
}

inline
CHXSimpleList::Iterator CHXSimpleList::RemoveAt(Iterator iter)
{
    LISTPOSITION pos = iter.m_pos;
    HX_ASSERT(pos);
    ++iter;
    RemoveAt(pos);
    return iter;
}
inline
CHXSimpleList::Iterator CHXSimpleList::InsertBefore(Iterator iter, void* value)
{
    LISTPOSITION pos = InsertBefore(iter.m_pos, value);
    return Iterator(iter.m_pList, pos); 
}

inline
CHXSimpleList::Iterator CHXSimpleList::InsertAfter(Iterator iter, void* value)
{
    LISTPOSITION pos = InsertAfter(iter.m_pos, value);
    return Iterator(iter.m_pList, pos); 
}

// Some comparison functions for common types
static int CHXCompareUINT32(void* pValue1, void* pValue2)
{
    int iRet = 0;

    UINT32 ulVal1 = (UINT32) pValue1;
    UINT32 ulVal2 = (UINT32) pValue2;

    if (ulVal1 < ulVal2)
    {
        iRet = -1;
    }
    else if (ulVal1 > ulVal2)
    {
        iRet = 1;
    }

    return iRet;
};

//
// CHXStringList inline methods
//

// add string at head
inline LISTPOSITION 
CHXStringList::AddHeadString(const char* pString)
{
    return AddHead(new CHXString(pString));
}

// add string at tail
inline LISTPOSITION 
CHXStringList::AddTailString(const char* pString)
{
    return AddTail(new CHXString(pString));
}

// get the next string in the list
inline CHXString* 
CHXStringList::GetNext(LISTPOSITION& pos) const
{
    return (CHXString*)CHXSimpleList::GetNext(pos);
}

#endif /* HXLIST_H_ */

