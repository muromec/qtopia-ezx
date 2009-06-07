/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: carray.cpp,v 1.6 2005/03/11 01:43:20 liam_murray Exp $
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

#include "carray.h"
#include "hlxclib/string.h"

static const int KInitDefaultGrowSize = 16;

///
/// public methods
///

CHXPtrArray::CHXPtrArray()
    : m_size(0),
      m_nelems(0),
      m_userGrowSize(-1),
      m_defGrowSize(KInitDefaultGrowSize),
      m_pData(0)
{
    
}

CHXPtrArray::~CHXPtrArray()
{
    delete [] m_pData;
}

///
/// SetSize(int nelems, int growSize)
///
/// set size and grow size
///
void 
CHXPtrArray::SetSize(int nelems, int growSize)
{
    if (growSize != -1)
	m_userGrowSize = growSize;

    if (nelems > m_size)
	Resize(nelems);
    else if (nelems < m_nelems)
	::memset(&m_pData[nelems], 0, (m_nelems - nelems) * sizeof(void*));
    m_nelems = nelems;
}

///
/// FreeExtra()
///
/// free un-assigned slots
///
void 
CHXPtrArray::FreeExtra()
{
    if (m_size > m_nelems)
	Resize(m_nelems);
}

///
/// RemoveAll()
///
/// free the entire array
///
void 
CHXPtrArray::RemoveAll()
{
    delete [] m_pData;
    m_pData = 0;
    m_nelems = 0;
    m_size = 0;
}

///
/// SetAtGrow(int index, void* value)
///
/// set value, grow array if needed
///
void 
CHXPtrArray::SetAtGrow(int index, void* value)
{
    HX_ASSERT(index >= 0);

    int nelems = index + 1;
    if (nelems > m_size)
	Resize(m_size + GetGrowSize(nelems));

    if (nelems > m_nelems) m_nelems = nelems;
    m_pData[index] = value;
}


/// 
/// InsertAt(int index, void* value, int repeat=1)
///
/// insert value(s) at index -- list operation
///
void 
CHXPtrArray::InsertAt(int index, void* value, int repeat)
{
    InsertCommon(index, repeat);
    for (int i=0; i < repeat; ++i)
	m_pData[i + index] = value;
}

///
/// InsertAt(int index, CHXPtrArray* pPtrArray)
///
/// insert array at index
///
void 
CHXPtrArray::InsertAt(int index, CHXPtrArray* pPtrArray)
{
    InsertCommon(index, pPtrArray->m_nelems);
    ::memmove(&m_pData[index], pPtrArray->m_pData,
	      pPtrArray->m_nelems * sizeof(void*));
}

void CHXPtrArray::Append(const CHXPtrArray& other)
{
    for (int idx = 0; idx < other.GetSize(); ++idx)
    {
        Add(other[idx]);
    }
}

///
/// RemoveAt(int index, int repeat=1)
///
/// remove value at index
///
void 
CHXPtrArray::RemoveAt(int index, int repeat)
{
    HX_ASSERT(index >= 0 && index < m_nelems);

    int relems = MIN(repeat, m_nelems - index); // number of elements to remove
    int numBytes = (m_nelems - index - relems) * sizeof(void*);
    if (numBytes > 0)
        ::memmove(&m_pData[index], &m_pData[index + relems], numBytes);

    SetSize(m_nelems - relems);
}

///
/// private methods
///

///
/// Resize()
///
/// common resize method
///
void
CHXPtrArray::Resize(int size)
{
    void** pData = new void*[size];
    HX_ASSERT(pData);

    if (pData)
    {
	// copy the existing data
	int celems = MIN(size, m_nelems); // number of elements to copy
	if (celems > 0)
	    ::memcpy(pData, m_pData, celems * sizeof(void*)); /* Flawfinder: ignore */
	if (size > celems)
	    ::memset(&pData[celems], 0, (size - celems) * sizeof(void*));
	delete [] m_pData;
	m_pData = pData;
	m_size = size;
	m_nelems = celems;
    }
}

///
/// GetGrowSize() 
///
/// return the size to grow the array, this may be set by the user in
/// SetSize() but will default to a doubling algorithm if not
///
int
CHXPtrArray::GetGrowSize(int newSize) 
{
    int growSize = 0;


    if (m_userGrowSize != -1)
    {
	// use multiple of user set size
	while (m_size + growSize < newSize)
	    growSize += m_userGrowSize;
    }
    else
    {
	// or use doubling if user did not set grow size
	while (m_size + m_defGrowSize < newSize)
	    m_defGrowSize <<= 1;

	growSize = m_defGrowSize;
    }

    return growSize;
}


///
/// InsertCommon(int index, int len)
///
/// common implementation for InsertAt methods
/// makes room in array to insert 'len' values at 'index'
///
void
CHXPtrArray::InsertCommon(int index, int len)
{
    HX_ASSERT(index >= 0);

    int nelems;		// total number of elements post insert

    // are we adding off the end or adding in the middle?
    if (index > m_nelems)
	nelems = index + len;
    else 
	nelems = m_nelems + len;

    // check to see if we need to grow the array
    void** pData = m_pData;
    if (nelems > m_size) 
    {
	int newSize = m_size + GetGrowSize(nelems);
	pData = new void*[newSize];
	// zero the extra data
	::memset(&pData[m_nelems], 0, (newSize - m_nelems) * sizeof(void*));
	m_size = newSize;

	// copy the prefix
	if (index > 0)
	    ::memcpy(pData, m_pData, MIN(m_nelems, index) * sizeof(void*)); /* Flawfinder: ignore */
    }

    // copy the suffix -- uses memmove in case of overlap
    if (index < m_nelems)
	::memmove(&pData[index + len], &m_pData[index], 
		  (m_nelems - index) * sizeof(void*));

    m_nelems = nelems;

    // if we had to resize, delete the old storage
    if (pData != m_pData)
    {
	delete [] m_pData;
	m_pData = pData;
    }
}
