/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: str2str_tests.cpp,v 1.4 2007/07/06 20:35:03 jfinnecy Exp $
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

#include "./map_spec_tests.h"
#include "chxmapstringtostring.h"
#include "hx_ut_debug.h"
#include "./kv_store.h"
#include "./hxlist.h"
#include "./find_match.h"

template<>
class MapSpecificTests<CHXMapStringToString>
{
public:
    bool operator()();

private:
    bool TestIterator();
    bool TestIterator2();
    bool TestGetStartPosition();
    bool TestGetNextAssoc();
    bool TestSetCaseSensitive();
};

bool MapSpecificTests<CHXMapStringToString>::operator() ()
{
    bool ret = (TestIterator() &&
		TestIterator2() &&
		TestGetStartPosition() &&
		TestGetNextAssoc() &&
		TestSetCaseSensitive());

    DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapStringToString> : %d\n",
		       ret));
    return ret;
}

bool MapSpecificTests<CHXMapStringToString>::TestIterator()
{
    bool ret = true;

    CHXMapStringToString map;
    KeyValueStore<CHXString, CHXString> store;

    for (int i = 0; i < 10; i++)
	store.Create(i);

    for (int j = 0; j < store.GetCount(); j++)
	map.SetAt(store.GetKey(j), store.GetValue(j));

    CHXMapStringToString::Iterator itr = map.Begin();
    HLXList<int> matchList;

    for (; ret && (itr != map.End()); ++itr)
    {
	int index = 0;
	if (FindMatch<CHXString, CHXString>()(store, itr.get_key(), *itr, index))
	{
	    if (!matchList.Find(index))
		matchList.AddTail(index);
	    else
		ret = false;
	}
	else
	    ret = false;
    }

    if (ret)
	ret = (matchList.GetCount() == map.GetCount());

    return ret;
}

bool MapSpecificTests<CHXMapStringToString>::TestIterator2()
{
    // This test is the same as TestIterator() except that
    // it uses the default constructor of the iterator.
    bool ret = true;

    CHXMapStringToString map;
    KeyValueStore<CHXString, CHXString> store;

    for (int i = 0; i < 10; i++)
	store.Create(i);

    for (int j = 0; j < store.GetCount(); j++)
	map.SetAt(store.GetKey(j), store.GetValue(j));

    CHXMapStringToString::Iterator itr;
    HLXList<int> matchList;

    for (itr = map.Begin(); ret && (itr != map.End()); ++itr)
    {
	int index = 0;
	if (FindMatch<CHXString, CHXString>()(store, itr.get_key(), *itr, index))
	{
	    if (!matchList.Find(index))
		matchList.AddTail(index);
	    else
		ret = false;
	}
	else
	    ret = false;
    }

    if (ret)
	ret = (matchList.GetCount() == map.GetCount());

    return ret;
}

bool MapSpecificTests<CHXMapStringToString>::TestGetStartPosition()
{
    bool ret = false;

    CHXMapStringToString map;

    POSITION pos = map.GetStartPosition();

    if (!pos)
    {
	KeyValueStore<CHXString, CHXString> store;
	store.Create(0);
	
	map.SetAt(store.GetKey(0), store.GetValue(0));

	pos = map.GetStartPosition();

	if (pos)
	    ret = true;
	else
	{
	    DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapStringToString>::TestGetStartPosition() : pos NULL for a non-empty map\n"));
	}
    }
    else
    {
	DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapStringToString>::TestGetStartPosition() : pos not NULL for an empty map\n"));
    }


    return ret;
}

bool MapSpecificTests<CHXMapStringToString>::TestGetNextAssoc()
{
    bool ret = true;

    CHXMapStringToString map;
    KeyValueStore<CHXString, CHXString> store;

    for (int i = 0; i < 10; i++)
	store.Create(i);

    for (int j = 0; j < store.GetCount(); j++)
	map.SetAt(store.GetKey(j), store.GetValue(j));

#ifdef DO_MAP_DUMP
    map.Dump();
#endif /* DO_MAP_DUMP */

    POSITION pos = map.GetStartPosition();

    HLXList<int> matchList;

    while (ret && pos)
    {
	CHXString key;
	CHXString value;
	int index = 0;
	
	map.GetNextAssoc(pos, key, value);

	if (!FindMatch<CHXString, CHXString>()(store, key, value, index))
	{
	    DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapStringToString>::TestGetNextAssoc() : failed to find the key/value pair\n"));
	    
	    ret = false;
	}
	else if (matchList.Find(index))
	{
	    DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapStringToString>::TestGetNextAssoc() : Item %d already visited\n", index));
	    ret = false;
	}	
	else
	    matchList.AddTail(index);
    }

    if (ret && (matchList.GetCount() != map.GetCount()))
    {
	DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapStringToString>::TestGetNextAssoc() : items visited count doesn't match map item count\n"));
	ret = false;
    }
    
    return ret;
}

struct StrToStrTestData 
{
    const char* m_pKey;
    bool m_success;
    const char* m_value;
};

static const StrToStrTestData testData[] = {
    {"aabcd", true  , "efghijk"},
    {"Aabcd", false , "efghijk"},
    {"Abcde", false , "lmnopqr"},
    {"abcde", false , "lmnopqr"},
    {"aBcde", true  , "lmnopqr"},
    {"HeLiX", true  , "Client"},
    {"HELIX", false , "Client"},
    {"helix", false , "Client"},
    {"heLIX", false , "Client"}
};

bool MapSpecificTests<CHXMapStringToString>::TestSetCaseSensitive()
{
    bool ret = true;

    CHXMapStringToString map;

    
    int testCount = sizeof(testData) / sizeof(struct StrToStrTestData);

    map.SetAt("aabcd", "efghijk");
    map.SetAt("aBcde", "lmnopqr");
    map.SetAt("HeLiX", "Client");

    
    for (int i = 0; ret && (i < testCount); i++)
    {
	CHXString value;
	bool result = (map.Lookup(testData[i].m_pKey, value) == TRUE);

	if (result != testData[i].m_success)
	{
	    DPRINTF(D_ERROR, ("MapSpecificTests<CHXMapStringToOb>::TestSetCaseSensitive() : Test %d lookup got %d expected %d\n",
			      i,
			      result,
			      testData[i].m_success));
	    ret = false;
	}
	else if (result && (value != testData[i].m_value))
	{
	    DPRINTF(D_ERROR, ("MapSpecificTests<CHXMapStringToOb>::TestSetCaseSensitive() : Test %d value got '%s' expected '%s'\n",
			      i,
			      (const char*)value,
			      (const char*)testData[i].m_value));
	    ret = false;
	}
    }

    if (ret)
    {
	map.RemoveAll();

	map.SetCaseSensitive(false);

	map.SetAt("aabcd", "efghijk");
	map.SetAt("aBcde", "lmnopqr");
	map.SetAt("HeLiX", "Client");

	for (int j = 0; ret && (j < testCount); j++)
	{
	    CHXString value;
	    bool result = (map.Lookup(testData[j].m_pKey, value) == TRUE);

	    if (!result)
	    {
		DPRINTF(D_ERROR, ("MapSpecificTests<CHXMapStringToOb>::TestSetCaseSensitive() : Test %d lookup failed\n",
				  j));
		ret = false;
	    }
	    else if (value != testData[j].m_value)
	    {
		DPRINTF(D_ERROR, ("MapSpecificTests<CHXMapStringToOb>::TestSetCaseSensitive() : Test %d value got '%s' expected '%s'\n",
				  j,
				  (const char*)value,
				  (const char*)testData[j].m_value));
		ret = false;
	    }
	}
    }

    return ret;
}
