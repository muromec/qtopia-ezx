/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ptr2ptr_tests.cpp,v 1.4 2007/07/06 20:35:03 jfinnecy Exp $
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

// #include "hxmap.h"
#include "chxmapptrtoptr.h"
#include "hx_ut_debug.h"
#include "./hxlist.h"
#include "./kv_store.h"
#include "./find_match.h"

template<>
class MapSpecificTests<CHXMapPtrToPtr>
{
public:
    bool operator()();
    
private:
    bool TestIterator();
    bool TestIterator2();
    bool TestGetStartPosition();
    bool TestGetNextAssoc();
    bool TestLookup();
    bool TestRemove();

    POSITION PredictRemovePosition(CHXMapPtrToPtr& map, void* key);
};

bool MapSpecificTests<CHXMapPtrToPtr>::operator() ()
{
    bool ret = (TestIterator() &&
		TestIterator2() &&
		TestGetStartPosition() &&
		TestGetNextAssoc() &&
		TestLookup() &&
		TestRemove());

    DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapPtrToPtr> : %d\n",
		       ret));
    return ret;
}

bool MapSpecificTests<CHXMapPtrToPtr>::TestIterator()
{
    bool ret = true;

    CHXMapPtrToPtr map;
    KeyValueStore<void*, void*> store;

    for (int i = 0; i < 2000; i++)
	store.Create(i);

    for (int j = 0; j < store.GetCount(); j++)
	map.SetAt(store.GetKey(j), store.GetValue(j));

    CHXMapPtrToPtr::Iterator itr = map.Begin();
    HLXList<int> matchList;

    for (; ret && (itr != map.End()); ++itr)
    {
	int index = 0;
	if (FindMatch<void*, void*>()(store, itr.get_key(), *itr, index))
	{
	    if (!matchList.Find(index))
		matchList.AddTail(index);
	    else
            {
		ret = false;
                DPRINTF (D_ERROR,
                         ("MapSpecificTests<CHXMapPtrToPtr>::TestIterator() : "
                          "got duplicate key %p\n",
                          itr.get_key()));
            }
	}
	else
        {
	    ret = false;
            DPRINTF (D_ERROR,
                     ("MapSpecificTests<CHXMapPtrToPtr>::TestIterator() : "
                      "got unknown key %p\n",
                      itr.get_key()));
        }
    }

    if (ret)
    {
	ret = (matchList.GetCount() == map.GetCount());
        if (!ret)
        {
            DPRINTF (D_ERROR,
                     ("MapSpecificTests<CHXMapPtrToPtr>::TestIterator() : "
                      "items visited count doesn't match map item count "
                      "(%ld vs %ld)\n",
                      matchList.GetCount(), map.GetCount()));
        }
    }

#ifdef DO_MAP_DUMP
    map.Dump();
#endif /* DO_MAP_DUMP */

    if (ret)
    {
        CHXMapPtrToPtr::Iterator itr = map.Find(store.GetKey(4));
        if (itr == map.End())
        {
            ret = false;
            DPRINTF (D_ERROR,
                     ("MapSpecificTests<CHXMapPtrToPtr>::TestIterator() : "
                      "couldn't Find() 4th key (%p)\n",
                      store.GetKey(4)));
        }
        else if (*itr != store.GetValue(4) || itr.get_key() != store.GetKey(4))
        {
            ret = false;
            DPRINTF (D_ERROR,
                     ("MapSpecificTests<CHXMapPtrToPtr>::TestIterator() : "
                      "Find() 4th key doesn't match (%p=%p vs %p=%p)\n",
                      store.GetKey(4), store.GetValue(4),
                      itr.get_key(), *itr));
        }

        // FYI: Erase() method isn't implemented in older CHXMap (it's
        //      declared in the class definition, but the method isn't
        //      implemented, so this leads to an undef'd symbol
        //      reference).
        if (ret)
        {
            map.Erase (itr);
            ret = (matchList.GetCount() == (map.GetCount() + 1));
            if (ret)
            {
                itr = map.Find(store.GetKey(4));
                if (itr != map.End())
                {
                    DPRINTF (D_ERROR,
                             ("MapSpecificTests<CHXMapPtrToPtr>::TestIterator() : "
                              "Erase() 4th key left it in map\n"));
                    ret = false;
                    
                }
            }
            else
            {
                DPRINTF (D_ERROR,
                         ("MapSpecificTests<CHXMapPtrToPtr>::TestIterator() : "
                          "Erase() 4th key decrease map count by one\n"));
            }
        }
    }

    return ret;
}

bool MapSpecificTests<CHXMapPtrToPtr>::TestIterator2()
{
    // This test is basically the same as the first part of
    // TestIterator() except it uses the default constructor
    // instead of the copy constructor.
    bool ret = true;

    CHXMapPtrToPtr map;
    KeyValueStore<void*, void*> store;

    for (int i = 0; i < 2000; i++)
	store.Create(i);

    for (int j = 0; j < store.GetCount(); j++)
	map.SetAt(store.GetKey(j), store.GetValue(j));

    CHXMapPtrToPtr::Iterator itr; // Use default constructor
    HLXList<int> matchList;

    for (itr = map.Begin(); ret && (itr != map.End()); ++itr)
    {
	int index = 0;
	if (FindMatch<void*, void*>()(store, itr.get_key(), *itr, index))
	{
	    if (!matchList.Find(index))
		matchList.AddTail(index);
	    else
            {
		ret = false;
                DPRINTF (D_ERROR,
                         ("MapSpecificTests<CHXMapPtrToPtr>::TestIterator2() : "
                          "got duplicate key %p\n",
                          itr.get_key()));
            }
	}
	else
        {
	    ret = false;
            DPRINTF (D_ERROR,
                     ("MapSpecificTests<CHXMapPtrToPtr>::TestIterator2() : "
                      "got unknown key %p\n",
                      itr.get_key()));
        }
    }

    if (ret)
    {
	ret = (matchList.GetCount() == map.GetCount());
        if (!ret)
        {
            DPRINTF (D_ERROR,
                     ("MapSpecificTests<CHXMapPtrToPtr>::TestIterator2() : "
                      "items visited count doesn't match map item count "
                      "(%ld vs %ld)\n",
                      matchList.GetCount(), map.GetCount()));
        }
    }

#ifdef DO_MAP_DUMP
    map.Dump();
#endif /* DO_MAP_DUMP */

    return ret;
}

bool MapSpecificTests<CHXMapPtrToPtr>::TestGetStartPosition()
{
    bool ret = true;

    CHXMapPtrToPtr map;

    POSITION pos = map.GetStartPosition();
    if (pos)
    {
	DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapPtrToPtr>::TestGetStartPosition() : pos not NULL for an empty map\n"));
        ret = false;
    }

    KeyValueStore<void*, void*> store;
    store.Create(0);
    store.Create(1);

    if (ret)
    {
	map.SetAt(store.GetKey(0), store.GetValue(0));

	pos = map.GetStartPosition();
        if (!pos)
	{
	    DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapPtrToPtr>::TestGetStartPosition() : pos NULL for a one-item map\n"));
            ret = false;
	}
    }

    if (ret)
    {
	map.SetAt(store.GetKey(1), store.GetValue(1));
	pos = map.GetStartPosition();
        if (!pos)
	{
	    DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapPtrToPtr>::TestGetStartPosition() : pos NULL for a two-item map\n"));
            ret = false;
	}
    }

    void* key;
    void* value;

    if (ret)
    {
        map.GetNextAssoc(pos, key, value);

        if (! map.RemoveKey(key))
        {
            DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapPtrToPtr>::TestGetStartPosition() : RemoveKey() of start key failed.\n"));
            ret = false;
        }
    }

    if (ret)
    {
        pos = map.GetStartPosition();
        if (!pos)
        {
            DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapPtrToPtr>::TestGetStartPosition() : pos NULL for a one-item map(down from 2)\n"));
            ret = false;
        }
    }

    void* key2;
    void* value2;
    if (ret)
    {
        // We should be on the one (and now only) item
        map.GetNextAssoc(pos, key2, value2);
        if (pos)
        {
            DPRINTF(D_ERROR, ("MapSpecificTests<CHXMapPtrToPtr>::TestGetStartPosition() : pos non-NULL for 2nd item in a one-item map\n"));
            ret = false;
        }
        else if (key == key2)
        {
            DPRINTF(D_ERROR, ("MapSpecificTests<CHXMapPtrToPtr>::TestGetStartPosition() : start key same as now removed key\n"));
            ret = false;
        }
    }

    if (ret)
    {
        pos = map.GetStartPosition();
        if (!pos)
        {
            DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapPtrToPtr>::TestGetStartPosition() : pos NULL for a one-item map (#2)\n"));
            ret = false;
        }
    }

    if (ret)
    {
        map.RemoveKey(key2);

        pos = map.GetStartPosition();
        if (pos)
        {
            DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapPtrToPtr>::TestGetStartPosition() : pos non-NULL for an empty map(down from 2)\n"));
            ret = false;
        }
    }

    return ret;
}

bool MapSpecificTests<CHXMapPtrToPtr>::TestGetNextAssoc()
{
    bool ret = true;

    CHXMapPtrToPtr map;
    KeyValueStore<void*, void*> store;

    for (int i = 0; i < 10; i++)
	store.Create(i);

    for (int j = 0; j < store.GetCount(); j++)
	map.SetAt(store.GetKey(j), store.GetValue(j));

    POSITION pos = map.GetStartPosition();

    HLXList<int> matchList;

    while (ret && pos)
    {
	void* key = 0;
	void* value = 0;
	int index = 0;
	
	map.GetNextAssoc(pos, key, value);

	if (!FindMatch<void*, void*>()(store, key, value, index))
	{
	    DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapPtrToPtr>::TestGetNextAssoc() : failed to find the key/value pair\n"));
	    
	    ret = false;
	}
	else if (matchList.Find(index))
	{
	    DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapPtrToPtr>::TestGetNextAssoc() : Item %d already visited\n", index));
	    ret = false;
	}	
	else
        {
	    matchList.AddTail(index);
        }
    }

    if (ret && (matchList.GetCount() != map.GetCount()))
    {
	DPRINTF (D_ERROR, ("MapSpecificTests<CHXMapPtrToPtr>::TestGetNextAssoc() : items visited count doesn't match map item count (%ld vs %ld)\n",
                           matchList.GetCount(), map.GetCount()));
	ret = false;
    }
    
    return ret;
}

bool MapSpecificTests<CHXMapPtrToPtr>::TestLookup()
{
    bool ret = true;

    CHXMapPtrToPtr map;
    KeyValueStore<void*, void*> store;

    for (int i = 0; i < 10; i++)
	store.Create(i);

    for (int j = 0; j < store.GetCount(); j++)
	map.SetAt(store.GetKey(j), store.GetValue(j));
    
    for (int k = 0; ret && (k < store.GetCount()); k++)
    {
	POSITION pos = map.Lookup(store.GetKey(k));
	void* key = 0;
	void* value = 0;
	int index = -1;

        map.GetNextAssoc(pos, key, value);
	if (FindMatch<void*, void*>()(store, key, value, index))
	{
	    if (index != k)
	    {
		DPRINTF (D_ERROR,("MapSpecificTests<CHXMapPtrToPtr>::TestLookup() : got %d expected %d\n",
				  index,
				  k));
		ret = false;
	    }
	}
	else
	{
	    DPRINTF (D_ERROR,("MapSpecificTests<CHXMapPtrToPtr>::TestLookup() : failed to find key/value pair\n"));
	    ret = false;
	}
	
    }

    return ret;
}

bool MapSpecificTests<CHXMapPtrToPtr>::TestRemove()
{
    bool ret = true;

    CHXMapPtrToPtr map;
    KeyValueStore<void*, void*> store;

    for (int i = 0; i < 10; i++)
	store.Create(i);

    for (int j = 0; j < store.GetCount(); j++)
	map.SetAt(store.GetKey(j), store.GetValue(j));

    
    for (int k = 0; ret && (k < store.GetCount()); k++)
    {
	int expectedCount = store.GetCount() - k;
	POSITION resultPos = 0;
	POSITION expectedPos = 0;


	if (map.GetCount() != expectedCount)
	{
	    DPRINTF(D_ERROR,("MapSpecificTests<CHXMapPtrToPtr>::TestRemove() : %d item count %d expected %d\n",
			     k,
			     map.GetCount(),
			     expectedCount));
	    ret = false;
	}
	else
	{
	    expectedPos = PredictRemovePosition(map, store.GetKey(k));
	    resultPos = map.Remove(store.GetKey(k));

	    if (resultPos != expectedPos)
	    {
		DPRINTF(D_ERROR,("MapSpecificTests<CHXMapPtrToPtr>::TestRemove() : %d got pos %p expected %p\n",
				 k,
				 resultPos,
				 expectedPos));
		ret = false;
	    }
	}
    }

    return ret;
}

POSITION MapSpecificTests<CHXMapPtrToPtr>::PredictRemovePosition(CHXMapPtrToPtr& map, void* key)
{
    POSITION ret = map.GetStartPosition();

    bool foundIt = false;

    while (ret && !foundIt)
    {
	void* pKey = 0;
	void* pValue = 0;
	map.GetNextAssoc(ret, pKey, pValue);

	if (key == pKey)
	    foundIt = true;
    }

    return ret;
}
