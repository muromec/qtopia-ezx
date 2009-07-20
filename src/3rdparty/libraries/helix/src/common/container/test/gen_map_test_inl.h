/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: gen_map_test_inl.h,v 1.5 2005/03/14 19:33:49 bobclark Exp $
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

#ifndef GEN_MAP_TEST_I
#define GEN_MAP_TEST_I

#include "./map_spec_tests.h"

template <class T, class K, class V>
inline
GenMapTest<T, K, V>::GenMapTest()
{}

template <class T, class K, class V>
inline
GenMapTest<T, K, V>::~GenMapTest()
{}

// Store manipulation
template <class T, class K, class V>
inline
bool GenMapTest<T, K, V>::CreateElement(int index)
{
    m_store.Create(index);

    return true;
}

template <class T, class K, class V>
inline
bool GenMapTest<T, K, V>::ClearElements()
{
    m_store.Clear();

    return true;
}

template <class T, class K, class V>
inline
void GenMapTest<T, K, V>::PrintElements()
{
    m_store.Print();
}

// Map manipulation
template <class T, class K, class V>
inline
bool GenMapTest<T, K, V>::GetCount(int expected) const
{
    bool ret = (m_map.GetCount() == expected); 
    
    if (!ret)
    {
	DPRINTF (D_ERROR,("GenMapTest<T, K, V>::GetCount() : got %d expected %d\n",
			  m_map.GetCount(), 
			  expected));
    }

    return ret;
}

template <class T, class K, class V>
inline
bool GenMapTest<T, K, V>::IsEmpty(bool expected) const
{
    bool ret = ((m_map.IsEmpty() == TRUE) == expected);

    if (!ret)
    {
	DPRINTF (D_ERROR,("GenMapTest<T, K, V>::IsEmpty() : got %d expected %d\n",
			  m_map.IsEmpty(), 
			  expected));
    }
    return ret;
}

template <class T, class K, class V>
inline
bool GenMapTest<T, K, V>::Lookup(int index, bool expected)
{
    bool ret = false;

    V value;
    HXBOOL result = m_map.Lookup(m_store.GetKey(index), value);

    if ((result == TRUE) != expected)
    {
	DPRINTF (D_ERROR,("GenMapTest<T, K, V>::Lookup() : got %d expected %d\n",
			  result,
			  expected));
    }
    else if (expected && (value != m_store.GetValue(index)))
    {
	char* pValueStr = ClassOps<V>().Print(value);
	char* pExpectStr = ClassOps<V>().Print(m_store.GetValue(index));

	DPRINTF (D_ERROR,("GenMapTest<T, K, V>::Lookup() : got '%s' expected '%s'\n",
			  pValueStr,
			  pExpectStr));

	delete [] pValueStr;
	delete [] pExpectStr;
    }
    else
	ret = true;

    return ret;
}

template <class T, class K, class V>
inline
bool GenMapTest<T, K, V>::SetAt(int index)
{
    bool ret = false;
    
    if (!m_store.IsSet(index))
    {
	DPRINTF(D_ERROR, ("GenMapTest<T, K, V>::SetAt() : value %d is not in store\n", 
			  index));
    }
    else
    {
	m_map.SetAt(m_store.GetKey(index), m_store.GetValue(index));
	ret = true;
    }

    return ret;
}

template <class T, class K, class V>
inline
bool GenMapTest<T, K, V>::RemoveKey(int index, bool expected)
{
    bool ret = false;

    if (!m_store.IsSet(index))
    {
	DPRINTF(D_ERROR, ("GenMapTest<T, K, V>::RemoveKey() : value %d is not in store\n", 
			  index));
    }
    else
    {

	bool result = (m_map.RemoveKey(m_store.GetKey(index)) == TRUE);

	if (result != expected)
	{
	    DPRINTF (D_ERROR, ("GenMapTest<T, K, V>::RemoveKey() : got %d expected %d\n",
			       result,
			       expected));
	}
	else
	    ret = true;
    }

    return ret;
}

template <class T, class K, class V>
inline
bool GenMapTest<T, K, V>::RemoveAll()
{
    m_map.RemoveAll();
    return true;
}

template <class T, class K, class V>
inline
bool GenMapTest<T, K, V>::RhsArrayOp(int index, bool expected)
{
    bool ret = false;

    if (!m_store.IsSet(index))
    {
	DPRINTF(D_ERROR, ("GenMapTest<T, K, V>::RhsArrayOp() : value %d is not in store\n", 
			  index));
    }
    else 
    {
	V result = m_map[m_store.GetKey(index)];
	V expectedVal = m_store.GetValue(index);

	if (!expected)
	{
	    // If we are not expecting the value to be in the
	    // map, then the value returned should be all zeros
	    expectedVal = ClassOps<V>().Null();
	}

	if (result != expectedVal)
	{
	    char* pValueStr = ClassOps<V>().Print(result);
	    char* pExpectStr = ClassOps<V>().Print(expectedVal);
	
	    DPRINTF (D_ERROR,("GenMapTest<T, K, V>::RhsArrayOp() : got '%s' expected '%s'\n",
			      pValueStr,
			      pExpectStr));
	    
	    delete [] pValueStr;
	    delete [] pExpectStr;
	}
	else
	    ret = true;
    }

    return ret;
}

template <class T, class K, class V>
inline
bool GenMapTest<T, K, V>::LhsArrayOp(int index)
{
    bool ret = false;

    if (!m_store.IsSet(index))
    {
	DPRINTF(D_ERROR, ("GenMapTest<T, K, V>::LhsArrayOp() : value %d is not in store\n", 
			  index));
    }
    else 
    {
	m_map[m_store.GetKey(index)] = m_store.GetValue(index);
	ret = true;
    }

    return ret;
}

template <class T, class K, class V>
inline
bool GenMapTest<T, K, V>::IsNull(int index, bool expected)
{
    bool ret = false;

    if (!m_store.IsSet(index))
    {
	DPRINTF(D_ERROR, ("GenMapTest<T, K, V>::IsNull() : value %d is not in store\n", 
			  index));
    }
    else 
    {
	V result;
	V expectedVal = ClassOps<V>().Null();

	if (!m_map.Lookup(m_store.GetKey(index), result))
	{
	    DPRINTF (D_ERROR,("GenMapTest<T, K, V>::IsNull() : lookup failed\n"));
	}
	else
	{
	    bool valuesMatch = (result == expectedVal);

	    if (valuesMatch != expected)
	    {
		DPRINTF (D_ERROR,("GenMapTest<T, K, V>::IsNull() : got %d expected %d\n",
			      valuesMatch,
			      expected));
	    }
	    else
		ret = true;
	}
    }

    return ret;
}

template <class T, class K, class V>
inline
bool GenMapTest<T, K, V>::RunMapSpecificTests()
{
    MapSpecificTests<T> mst;
    return mst();
}

#endif // GEN_MAP_TEST_I
