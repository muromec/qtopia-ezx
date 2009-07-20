/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxslist.cpp,v 1.14 2007/01/10 20:02:26 ehyche Exp $
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

#include "hxslist.h"
#include "debug.h"

#include "hlxclib/ctype.h"
#ifndef _MACINTOSH
#include "hlxclib/string.h"
#endif


CHXSimpleList::CHXSimpleList()
    : m_nelems(0),
      m_pHead(0),
      m_pTail(0)
{
}

CHXSimpleList::~CHXSimpleList()
{
    RemoveAll();
}

// TRUE if list is internally consistent
HXBOOL 
CHXSimpleList::IsPtrListValid()
{
    if (!m_pHead)
    {
	if (m_pTail)
	    return FALSE;

	if (m_nelems)
	    return FALSE;
    }
    else if (!m_pTail) return FALSE;

    if (m_nelems == 1)
    {
	if (m_pHead != m_pTail)
	    return FALSE;
    }

    if (m_nelems < 0) return FALSE;
	
    return TRUE;
}

// insert list before first element
void 
CHXSimpleList::AddHead(CHXSimpleList* pList)
{
    HX_ASSERT(pList);
    
    for (CNode* pNode = pList->m_pTail; pNode; pNode = pNode->GetPrev())
	(void)InsertBefore((LISTPOSITION)m_pHead, pNode->GetValue());
} 

// insert list after last element
void 
CHXSimpleList::AddTail(CHXSimpleList* pList)
{
    HX_ASSERT(pList);

    for (CNode* pNode = pList->m_pHead; pNode; pNode = pNode->GetNext())
	(void)InsertAfter((LISTPOSITION)m_pTail, pNode->GetValue());	
}

// clear all elements from the list
void 
CHXSimpleList::RemoveAll()
{
    if (m_pHead)
    {
	CNode* pNode = m_pHead;
    
	while (pNode)
	{
	    CNode* pNext = pNode->GetNext();

	    delete pNode;
	    --m_nelems;

	    pNode = pNext;
	}
    }
    m_pHead = m_pTail = 0;
    HX_ASSERT (m_nelems == 0);
}

// return value at current position and incr
void*& 
CHXSimpleList::GetNext(LISTPOSITION& pos)
{
    HX_ASSERT(pos != (LISTPOSITION)NULL);
    CNode* pNode = (CNode*)pos;
    pos = (LISTPOSITION)pNode->GetNext();
    return pNode->GetValue();
}

// return value at current position and incr
void* 
CHXSimpleList::GetNext(LISTPOSITION& pos) const
{
    HX_ASSERT(pos != (LISTPOSITION)NULL);
    CNode* pNode = (CNode*)pos;
    pos = (LISTPOSITION)pNode->GetNext();
    return pNode->GetValue();
}

// return value at current position and decr
void*& 
CHXSimpleList::GetPrev(LISTPOSITION& pos)
{
    HX_ASSERT(pos != (LISTPOSITION)NULL);
    CNode* pNode = (CNode*)pos;
    pos = (LISTPOSITION)pNode->GetPrev();
    return pNode->GetValue();
}

// return value at current position and decr
void* 
CHXSimpleList::GetPrev(LISTPOSITION& pos) const
{
    HX_ASSERT(pos != (LISTPOSITION)NULL);
    CNode* pNode = (CNode*)pos;
    pos = (LISTPOSITION)pNode->GetPrev();
    return pNode->GetValue();
}

// incr and return value at current position 
void*& 
CHXSimpleList::GetAtNext(LISTPOSITION& pos)
{
    HX_ASSERT(pos != (LISTPOSITION)NULL);
    CNode* pNode = (CNode*)pos;
    pNode = pNode->GetNext();
    pos = (LISTPOSITION)pNode;
    return pNode ? pNode->GetValue() : (void*&)_nil();
}

// incr and return value at current position
void* 
CHXSimpleList::GetAtNext(LISTPOSITION& pos) const
{
    HX_ASSERT(pos != (LISTPOSITION)NULL);
    CNode* pNode = (CNode*)pos;
    pNode = pNode->GetNext();
    pos = (LISTPOSITION)pNode;
    return pNode ? (void*)pNode->GetValue() : (void*)_nil();
}

// decr and return value at current position
void*& 
CHXSimpleList::GetAtPrev(LISTPOSITION& pos)
{
    HX_ASSERT(pos != (LISTPOSITION)NULL);
    CNode* pNode = (CNode*)pos;
    pNode = pNode->GetPrev();
    pos = (LISTPOSITION)pNode;
    return pNode ? pNode->GetValue() : (void*&)_nil();
}

// decr and return value at current position
void* 
CHXSimpleList::GetAtPrev(LISTPOSITION& pos) const
{
    HX_ASSERT(pos != (LISTPOSITION)NULL);
    CNode* pNode = (CNode*)pos;
    pNode = pNode->GetPrev();
    pos = (LISTPOSITION)pNode;
    return pNode ? (void*)pNode->GetValue() : (void*)_nil();
}

// get value at LISTPOSITION
void*& 
CHXSimpleList::GetAt(LISTPOSITION pos)
{
    HX_ASSERT(pos != (LISTPOSITION)NULL);
    CNode* pNode = (CNode*)pos;
    return pNode->GetValue();
}

// get value at LISTPOSITION
void* 
CHXSimpleList::GetAt(LISTPOSITION pos) const
{
    HX_ASSERT(pos != (LISTPOSITION)NULL);
    CNode* pNode = (CNode*)pos;
    return pNode->GetValue();
}

// set value at LISTPOSITION
void 
CHXSimpleList::SetAt(LISTPOSITION pos, void* value)
{
    HX_ASSERT(pos != (LISTPOSITION)NULL);
    CNode* pNode = (CNode*)pos;
    pNode->SetValue(value);
}

// remove node
CHXSimpleList::CNode*
CHXSimpleList::RemoveNode(CNode* pNode)
{
    HX_ASSERT(pNode != NULL);
    CNode* pPrev = pNode->GetPrev();
    CNode* pNext = pNode->GetNext();

    if (pPrev)
	pPrev->SetNext(pNext);
    else
	m_pHead = pNext;

    if (pNext)
	pNext->SetPrev(pPrev);
    else
	m_pTail = pPrev;

    delete pNode;
    --m_nelems;

    // Yes, this is kinda goofy...
    return (pNext ? pNext : pPrev);
}
    
// insert before LISTPOSITION
LISTPOSITION 
CHXSimpleList::InsertBefore(LISTPOSITION pos, void* value)
{
    CNode* pNode = CreateNode(value);
    if( !pNode )
    {
        return (LISTPOSITION)NULL;
    }
    CNode* pNext = (CNode*)(pos ? pos : m_pHead);
    CNode* pPrev = NULL;

    if (pNext)
    {
	pPrev = pNext->GetPrev();
	pNext->SetPrev(pNode);
	pNode->SetNext(pNext);
    }
    else m_pTail = pNode;

    if (pNext == m_pHead)
	m_pHead = pNode;

    if (pPrev)
    {
	pPrev->SetNext(pNode);
	pNode->SetPrev(pPrev);
    }

    ++m_nelems;

    return (LISTPOSITION)pNode;
}

// insert after LISTPOSITION
LISTPOSITION 
CHXSimpleList::InsertAfter(LISTPOSITION pos, void* value)
{
    CNode* pNode = CreateNode(value);
    if( !pNode )
    {
        return (LISTPOSITION)NULL;
    }
    CNode* pPrev = (CNode*)(pos ? pos : m_pTail);
    CNode* pNext = NULL;

    if (pPrev)
    {
	pNext = pPrev->GetNext();
	pPrev->SetNext(pNode);
	pNode->SetPrev(pPrev);
    }
    else m_pHead = pNode;

    if (pPrev == m_pTail)
        m_pTail = pNode;

    if (pNext)
    {
	pNext->SetPrev(pNode);
	pNode->SetNext(pNext);
    }
    
    ++m_nelems;

    return (LISTPOSITION)pNode;
}
    
// search for value in list
LISTPOSITION 
CHXSimpleList::Find(void* value, LISTPOSITION start) const
{
    CNode* pNode;

    if (!start)
	pNode = m_pHead;
    else
	pNode = (CNode*)start;

    while (pNode)
    {
	if (pNode->GetValue() == value)
	    return (LISTPOSITION)pNode;

	pNode = pNode->GetNext();
    }

    return (LISTPOSITION)NULL;
}

// get the LISTPOSITION for element at index
LISTPOSITION 
CHXSimpleList::FindIndex(int index) const
{
    if (index >= m_nelems || index < 0) return NULL;

    CNode* pNode = m_pHead;

    while (pNode && index--)
	pNode = pNode->GetNext();

    return (LISTPOSITION)pNode;
}

int
CHXSimpleList::GetIndex(LISTPOSITION pos) const
{
    int iRet = -1; // -1 indicates this position is not in the list

    if (pos && GetCount() > 0)
    {
        int          indx = 0;
        LISTPOSITION lPos = GetHeadPosition();
        do
        {
            if (lPos == pos)
            {
                iRet = indx;
                break;
            }
            GetNext(lPos);
            indx++;
        }
        while (lPos);
    }

    return iRet;
}

LISTPOSITION
CHXSimpleList::ForEach(LISTPOSITION start, LISTPOSITION end, void* pUser,
		       ConditionFunc func)
{
    if (!m_pHead) return NULL;

    CNode* pNode;

    if (!start)
	pNode = m_pHead;
    else
	pNode = (CNode*)start;

    while (pNode != (CNode*)end)
    {
	if (func(pUser, pNode->GetValue()))
	    return (LISTPOSITION)pNode;

	pNode = pNode->GetNext();
    }
    if (func(pUser, pNode->GetValue()))
	return (LISTPOSITION)pNode;

    return (LISTPOSITION)NULL;  
}

LISTPOSITION
CHXSimpleList::ForEach(LISTPOSITION start, LISTPOSITION end, void* pUser,
		       ConditionNodeFunc func) const
{
    if (!m_pHead) return NULL;

    CNode* pNode;

    if (!start)
	pNode = m_pHead;
    else
	pNode = (CNode*)start;

    while (pNode != (CNode*)end)
    {
	if (func(pUser, pNode))
	    return (LISTPOSITION)pNode;

	pNode = pNode->GetNext();
    }
    if (func(pUser, pNode))
	return (LISTPOSITION)pNode;

    return (LISTPOSITION)NULL;  
}

CHXSimpleList::CNode*
CHXSimpleList::CreateNode(void* value)
{
    return new CNode(value);
}

#ifdef _DEBUG
static HXBOOL DumpNode(void* /*pUser*/, const CHXSimpleList::CNode* pNode)
{
    printf("   %p: prev=%p; next=%p; val=%p;\n",
           pNode,
           pNode->GetPrev(), pNode->GetNext(),
           pNode->GetValue());
    return FALSE;               // to continue iteration
}
#endif /* _DEBUG */

void
CHXSimpleList::Dump(const char* label) const
{
#ifdef _DEBUG
    printf("%sthis=(CHXSimpleList*)%p; head=%p; tail=%p; nelems=%d\n",
           label ? label : "", this, m_pHead, m_pTail, m_nelems);
    ForEach(GetHeadPosition(), GetTailPosition(), NULL, &DumpNode);
#endif /* _DEBUG */
}

LISTPOSITION
CHXSimpleList::FindMinOrMax(CHXSimpleListCompareFunc func, HXBOOL bMax, LISTPOSITION startPos)
{
    LISTPOSITION posMinMax = NULL;

    if (GetCount() > 0)
    {
        // Get the starting position. If startPos is NULL,
        // then we start at the head. Otherwise, we start at startPos.
        LISTPOSITION pos = (startPos ? startPos : GetHeadPosition());
        // We initially assume that the element
        // at the starting position is the min or max.
        posMinMax = pos;
        // Go to the next element in the list
        GetNext(pos);
        // Look through the rest of the elements in the 
        // list and compare them to the element at the
        // current position
        while (pos)
        {
            // Compare the current element to the min or max
            int iCmp = func(GetAt(pos), GetAt(posMinMax));
            // Decide if we need to replace the min or max
            if ((!bMax && iCmp < 0) || (bMax && iCmp > 0))
            {
                posMinMax = pos;
            }
            // Go the next element
            GetNext(pos);
        }
    }

    return posMinMax;
}

// This does a simple insertion sort, so it's not 
// very efficient for large lists.
void
CHXSimpleList::InsertionSort(CHXSimpleListCompareFunc func, HXBOOL bDescending)
{
    // Run through the list
    LISTPOSITION pos = GetHeadPosition();
    while (pos)
    {
        // Find the min or max from the current position
        // through the end of the list. If we are sorting
        // in descending order, then we look for the max.
        // If we are sorting in ascending order, then we
        // look for the min.
        LISTPOSITION posMinMax = FindMinOrMax(func, bDescending, pos);
        // Is the min/max position different from the current position?
        if (posMinMax != pos)
        {
            // Swap the elements at the min/max position and the current position
            void* pCur = GetAt(pos);
            SetAt(pos, GetAt(posMinMax));
            SetAt(posMinMax, pCur);
        }
        // Move on to the next position
        GetNext(pos);
    }
}

// pCoSortedlist is an co-sorted list.
// A co-sort is where we sort a primary list and
// then sort a secondary list in the same we we sort
// the primary list. This is useful, for instance, if we
// have name/value pairs and we want to sort them by
// name AND we have the names in one list and the values
// in a separate list. NOTE that the primary list
// and the secondary list MUST have the same number
// of elements; otherwise, no co-sort is done.
void
CHXSimpleList::InsertionCoSort(CHXSimpleListCompareFunc func, CHXSimpleList* pCoSortedList, HXBOOL bDescending)
{
    // The primary list and the co-sorted list must
    // have the same number of elements.
    HXBOOL bDoCoSort = (pCoSortedList && pCoSortedList->GetCount() == GetCount());
    // Run through the list
    LISTPOSITION pos = GetHeadPosition();
    while (pos)
    {
        // Find the min or max from the current position
        // through the end of the list. If we are sorting
        // in descending order, then we look for the max.
        // If we are sorting in ascending order, then we
        // look for the min.
        LISTPOSITION posMinMax = FindMinOrMax(func, bDescending, pos);
        // Is the min/max position different from the current position?
        if (posMinMax != pos)
        {
            // Swap the elements at the min/max position and the current position
            void* pCur = GetAt(pos);
            SetAt(pos, GetAt(posMinMax));
            SetAt(posMinMax, pCur);
            // Should we do the co-sort?
            if (bDoCoSort)
            {
                // Get the indices of current and min/max positions
                int iCur    = GetIndex(pos);
                int iMinMax = GetIndex(posMinMax);
                // Sanity check on the indices
                if (iCur    >= 0 && iCur    < pCoSortedList->GetCount() &&
                    iMinMax >= 0 && iMinMax < pCoSortedList->GetCount())
                {
                    // Get the positions in the co-sorted list
                    LISTPOSITION posCoCur    = pCoSortedList->FindIndex(iCur);
                    LISTPOSITION posCoMinMax = pCoSortedList->FindIndex(iMinMax);
                    // Swap the elements in the co-sorted list
                    pCur = pCoSortedList->GetAt(posCoCur);
                    pCoSortedList->SetAt(posCoCur, pCoSortedList->GetAt(posCoMinMax));
                    pCoSortedList->SetAt(posCoMinMax, pCur);
                }
            }
        }
        // Move on to the next position
        GetNext(pos);
    }
}

//
// CHXStringList methods
//

static int CHXStringCompare(void* pValue1, void* pValue2)
{
    int iRet = 0;

    if (pValue1 && pValue2)
    {
        CHXString* pStr1 = (CHXString*) pValue1;
        CHXString* pStr2 = (CHXString*) pValue2;
        iRet = (int) pStr1->Compare((const char*) *pStr2);
    }

    return iRet;
}

static int CHXStringCompareNoCase(void* pValue1, void* pValue2)
{
    int iRet = 0;

    if (pValue1 && pValue2)
    {
        CHXString* pStr1 = (CHXString*) pValue1;
        CHXString* pStr2 = (CHXString*) pValue2;
        iRet = (int) pStr1->CompareNoCase((const char*) *pStr2);
    }

    return iRet;
}

static HXBOOL IsEqual(void* pUser, void* pData)
{
    const char* s = (const char*)pUser;
    CHXString* pString = (CHXString*)pData;

    return pString->Compare(s) == 0;
}

static HXBOOL IsEqualNoCase(void* pUser, void* pData)
{
    const char* s = (const char*)pUser;
    CHXString* pString = (CHXString*)pData;

    return pString->CompareNoCase(s) == 0;
}

static HXBOOL IsPrefix(void* pUser, void* pData)
{
    const char* prefix = (const char*)pUser;
    const char* s = (const char*)*(CHXString*)pData;

    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static HXBOOL IsPrefixNoCase(void* pUser, void* pData)
{
    const char* prefix = (const char*)pUser;
    const char* s = (const char*)*(CHXString*)pData;

    return strnicmp(s, prefix, strlen(prefix)) == 0;
}

static HXBOOL IsGreaterAlpha(void* pUser, void* pData)
{
    const char* s = (const char*)pUser;
    CHXString* pString = (CHXString*)pData;

    return pString->Compare(s) > 0;
}

static HXBOOL IsGreaterAlphaNoCase(void* pUser, void* pData)
{
    const char* s = (const char*)pUser;
    CHXString* pString = (CHXString*)pData;

    return pString->CompareNoCase(s) > 0;
}

// find a string in the list
LISTPOSITION 
CHXStringList::FindString(const char* pString,
			  LISTPOSITION start,
			  HXBOOL caseSensitive)
{
    LISTPOSITION pos = NULL;

    if (GetCount() > 0)
    {
	if (start)
	    pos = start;
	else
	    pos = GetHeadPosition();

	if (caseSensitive)
	    pos = ForEach(pos, GetTailPosition(),
			  (void*)pString, &IsEqual);
	else
	    pos = ForEach(pos, GetTailPosition(),
			  (void*)pString, &IsEqualNoCase);
    }

    return pos;
}

			 
// find a string that starts with 'pPrefix'
LISTPOSITION 
CHXStringList::FindPrefixSubstring(const char* pPrefix,
				   LISTPOSITION start,
				   HXBOOL caseSensitive)
{
    LISTPOSITION pos = NULL;

    if (GetCount() > 0)
    {
	if (start)
	    pos = start;
	else
	    pos = GetHeadPosition();

	if (caseSensitive)
	    pos = ForEach(pos, GetTailPosition(),
			  (void*)pPrefix, &IsPrefix);
	else
	    pos = ForEach(pos, GetTailPosition(),
			  (void*)pPrefix, &IsPrefixNoCase);
    }

    return pos;
}

// add string in sorted alpha order
LISTPOSITION 
CHXStringList::AddStringAlphabetic(const char* pString, HXBOOL caseSensitive)
{
    LISTPOSITION pos = GetHeadPosition();

    pos = ForEach(pos, GetTailPosition(), (void*)pString,
                  caseSensitive ? &IsGreaterAlpha : &IsGreaterAlphaNoCase);

    if (pos)
	return InsertBefore(pos, new CHXString(pString));
    else
	return AddTail(new CHXString(pString));
}

LISTPOSITION 
CHXStringList::RemoveAt(LISTPOSITION pos)
{
    if (pos)
    {
        CNode* pNode = (CNode*)pos;
        delete (CHXString*)pNode->GetValue();
        return CHXSimpleList::RemoveAt(pos);
    }
    return NULL;
}

CHXStringList::Iterator CHXStringList::RemoveAt(Iterator iter)
{
    if (iter != End())
    {
        LISTPOSITION pos = iter.m_pos;
        pos = RemoveAt(pos);
        return Iterator(this, pos);
    }
    return End();
}

// remove head string and free mem
void 
CHXStringList::RemoveHeadString()
{
    CHXString* pString = (CHXString*)RemoveHead();
    delete pString;
}

// remove tail string and free mem
void 
CHXStringList::RemoveTailString()
{
    CHXString* pString = (CHXString*)RemoveTail();
    delete pString;
}

void
CHXStringList::Sort(HXBOOL bCaseSensitive, HXBOOL bDescending)
{
    InsertionSort((bCaseSensitive ? CHXStringCompare : CHXStringCompareNoCase), bDescending);
}

void
CHXStringList::CoSort(CHXStringList* pCoSortedList, HXBOOL bCaseSensitive, HXBOOL bDescending)
{
    InsertionCoSort((bCaseSensitive ? CHXStringCompare : CHXStringCompareNoCase), pCoSortedList, bDescending);
}

CHXString
CHXStringList::Join(const char* pszDelim)
{
    CHXString strRet;

    LISTPOSITION pos = GetHeadPosition();
    while (pos)
    {
        CHXString* pName = GetNext(pos);
        if (pName)
        {
            strRet += *pName;
            if (pos && pszDelim)
            {
                strRet += pszDelim;
            }
        }
    }

    return strRet;
}

#ifdef _DEBUG
static HXBOOL DumpStringNode(void* /*pUser*/, const CHXSimpleList::CNode* pNode)
{
    const char* sz = (const char*)*(CHXString*)pNode->GetValue();

    printf("   %p: prev=%p; next=%p; val=\"%s\";\n",
           pNode,
           pNode->GetPrev(), pNode->GetNext(),
           sz);
    return FALSE;               // to continue iteration
}
#endif /* _DEBUG */

void
CHXStringList::Dump(const char* label) const
{
#ifdef _DEBUG
    printf("%sthis=(CHXStringList*)%p; head=%p; tail=%p; nelems=%d\n",
           label ? label : "",
           this, GetHeadPosition(), GetTailPosition(), GetCount());
    ForEach(GetHeadPosition(), GetTailPosition(), NULL, &DumpStringNode);
#endif /* _DEBUG */
}


void 
CHXStringList::RemoveAll()
{
    for (LISTPOSITION p = GetHeadPosition(); p != NULL;)
    {
	CHXString* pStr = (CHXString*)GetNext(p);
	HX_ASSERT(pStr);
	delete pStr;
    }
    CHXSimpleList::RemoveAll();
}

