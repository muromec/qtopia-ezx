/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: long2ptr_tests.cpp,v 1.4 2007/07/06 20:35:03 jfinnecy Exp $
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

#include "chxmaplongtoobj.h"
#include "hx_ut_debug.h"
#include "./kv_store.h"
#include "./hxlist.h"
#include "./find_match.h"

template<>
class MapSpecificTests<CHXMapLongToObj>
{
 public:
    bool operator()();

private:
    bool TestIterator();
    bool TestIterator2();
    bool TestGetStartPosition();
    bool TestGetNextAssoc();
};

bool MapSpecificTests<CHXMapLongToObj>::operator() ()
{
    bool ret = (TestIterator() &&
		TestIterator2() &&
		TestGetStartPosition() &&
		TestGetNextAssoc());

    DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapLongToObj> : %d\n",
		       ret));
    return ret;
}

bool MapSpecificTests<CHXMapLongToObj>::TestIterator()
{
    bool ret = true;

    CHXMapLongToObj map;
    KeyValueStore<LONG32, void*> store;

    for (int i = 0; i < 10; i++)
	store.Create(i);

    for (int j = 0; j < store.GetCount(); j++)
	map.SetAt(store.GetKey(j), store.GetValue(j));

    CHXMapLongToObj::Iterator itr = map.Begin();
    HLXList<int> matchList;

    for (; ret && (itr != map.End()); ++itr)
    {
	int index = 0;
	if (FindMatch<LONG32, void*>()(store, itr.get_key(), *itr, index))
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

bool MapSpecificTests<CHXMapLongToObj>::TestIterator2()
{
    // This test is the same as TestIterator() except that
    // it uses the default constructor of the iterator.
    bool ret = true;

    CHXMapLongToObj map;
    KeyValueStore<LONG32, void*> store;

    for (int i = 0; i < 10; i++)
	store.Create(i);

    for (int j = 0; j < store.GetCount(); j++)
	map.SetAt(store.GetKey(j), store.GetValue(j));

    CHXMapLongToObj::Iterator itr;
    HLXList<int> matchList;

    for (itr  = map.Begin(); ret && (itr != map.End()); ++itr)
    {
	int index = 0;
	if (FindMatch<LONG32, void*>()(store, itr.get_key(), *itr, index))
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

bool MapSpecificTests<CHXMapLongToObj>::TestGetStartPosition()
{
    bool ret = false;

    CHXMapLongToObj map;

    POSITION pos = map.GetStartPosition();

    if (!pos)
    {
	KeyValueStore<LONG32, void*> store;
	store.Create(0);
	
	map.SetAt(store.GetKey(0), store.GetValue(0));

	pos = map.GetStartPosition();

	if (pos)
	    ret = true;
	else
	{
	    DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapLongToObj>::TestGetStartPosition() : pos NULL for a non-empty map\n"));
	}
    }
    else
    {
	DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapLongToObj>::TestGetStartPosition() : pos not NULL for an empty map\n"));
    }

    return ret;
}

bool MapSpecificTests<CHXMapLongToObj>::TestGetNextAssoc()
{
    bool ret = true;

    CHXMapLongToObj map;
    KeyValueStore<LONG32, void*> store;

    for (int i = 0; i < 10; i++)
	store.Create(i);

    for (int j = 0; j < store.GetCount(); j++)
	map.SetAt(store.GetKey(j), store.GetValue(j));

    POSITION pos = map.GetStartPosition();

    HLXList<int> matchList;

    while (ret && pos)
    {
	LONG32 key = 0;
	void* value = 0;
	int index = 0;
	
	map.GetNextAssoc(pos, key, value);

	if (!FindMatch<LONG32, void*>()(store, key, value, index))
	{
	    DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapLongToObj>::TestGetNextAssoc() : failed to find the key/value pair\n"));
	    
	    ret = false;
	}
	else if (matchList.Find(index))
	{
	    DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapLongToObj>::TestGetNextAssoc() : Item %d already visited\n", index));
	    ret = false;
	}	
	else
	    matchList.AddTail(index);
    }

    if (ret && (matchList.GetCount() != map.GetCount()))
    {
	DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapLongToObj>::TestGetNextAssoc() : items visited count doesn't match map item count\n"));
	ret = false;
    }
    
    return ret;
}
