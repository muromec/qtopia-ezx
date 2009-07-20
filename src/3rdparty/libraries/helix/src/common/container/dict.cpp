/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dict.cpp,v 1.8 2007/07/06 20:34:58 jfinnecy Exp $
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

#include "hlxclib/string.h"
#include "hxtypes.h"
#include "dict.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


unsigned int
default_strhash(const char *key)
{	// Chris Torek's hash function
    unsigned int h = 0x31fe;

    while (*key)
	h = h*33 + *key++;
    return h;
}

void
Dict::init()
{
    _table = new Dict_entry*[_nbuckets];
    for (unsigned int i = 0; i < _nbuckets; i++) 
	_table[i] = 0;
}

// Do this as a function (in case it's really a macro with args...need a
// function ptr to use in Dict ctor.
static int func_strcasecmp(const char* s1, const char* s2)
{
    return strcasecmp(s1, s2);
}

Dict::Dict(unsigned int nbuckets):
    _count(0), _nbuckets(nbuckets),
    _compare(func_strcasecmp), _hash(default_strhash)
{
    init();
}

Dict::Dict(int (*compare)(const char*, const char*),
	   unsigned int (*hash)(const char*), unsigned int nbuckets):
    _count(0), _nbuckets(nbuckets),
    _compare(compare), _hash(default_strhash)
{
    init();
}

Dict::~Dict()
{
    for (unsigned int i = 0; i < _nbuckets; i++)
    {
	Dict_entry* nexte;
	for (Dict_entry* e = _table[i]; e != 0; e = nexte)
	{
	    nexte = e->next;
	    delete[] e->key;
	    delete e;
	}
    }
    delete[] _table;
}

Dict_entry*
Dict::enter(Key key, void* obj)
{
    unsigned int h = _hash(key);
    Dict_entry* e, *nexte;
    for (e = _table[h%_nbuckets]; e != 0; e = e->next)
	if (_compare(key, e->key) == 0)
	    return e;
    _count++;

    // Grow the table if 66% full
    unsigned int nb = _count*3;
    if (nb > _nbuckets*2)
    {
	Dict_entry** tab = new Dict_entry*[nb];
	unsigned int i;
	for (i = 0; i < nb; i++)
	{
	    tab[i] = 0;
	}
#ifdef XXXAAK_AWAITING_CR
	memset(tab, 0, nb * sizeof(Dict_entry *));
#endif
	for (i = 0; i < _nbuckets; i++)
	{
	    for (e = _table[i]; e != 0; e = nexte)
	    {
		nexte = e->next;
		e->next = tab[e->hash%nb];
		tab[e->hash%nb] = e;
	    }
	}
	delete[] _table;
	_table = tab;
	_nbuckets = nb;
    }
    e = new Dict_entry;
    e->next = _table[h%_nbuckets];
    e->key = new char[strlen(key)+1];
    e->hash = h;
    strcpy(e->key, key); /* Flawfinder: ignore */
    e->obj = obj;
    _table[h%_nbuckets] = e;
    return e;
}

void*
Dict::remove(Key key)
{
    Dict_entry* e, **ep;
    unsigned int h = _hash(key);
    for (ep = &_table[h%_nbuckets]; (e = *ep) != 0; ep = &e->next)
    {
	if (_compare(key, e->key) != 0)
	    continue;
	void* obj = e->obj;
	*ep = e->next;
	delete[] e->key;
	delete e;
	--_count;
	return obj;
    }
    return 0;       
}

Dict_entry*
Dict::find(Key key)
{
    unsigned int h = _hash(key);
    for (Dict_entry* e = _table[h%_nbuckets]; e != 0; e = e->next)
	if (strcasecmp(key, e->key) == 0)
	    return e;
    return 0;
}

void
Dict::next(unsigned int& h, Dict_entry*& e)
{
    e = e->next;
    if (e)
	return;
    for (unsigned int i = h+1; i < _nbuckets; i++)
    {
	if (_table[i] == 0)
	    continue;
	e = _table[i];
	h = i;
	return;
    }
    e = 0;
}

#ifdef XXXAAK_AWAITING_CR
Dict_entry*
Dict::enter(Key key, void* obj, UINT32& hashId)
{
    unsigned int h = hashId =_hash(key);
    Dict_entry* e, *nexte;
    for (e = _table[h%_nbuckets]; e != 0; e = e->next)
	if (h == e->hash)
	    return e;
    _count++;

    // Grow the table if 66% full
    unsigned int nb = _count*3;
    if (nb > _nbuckets*2)
    {
	Dict_entry** tab = new Dict_entry*[nb];
	unsigned int i;
	memset(tab, 0, nb * sizeof(Dict_entry *));
	for (i = 0; i < _nbuckets; i++)
	{
	    for (e = _table[i]; e != 0; e = nexte)
	    {
		nexte = e->next;
		e->next = tab[e->hash%nb];
		tab[e->hash%nb] = e;
	    }
	}
	delete[] _table;
	_table = tab;
	_nbuckets = nb;
    }
    e = new Dict_entry;
    e->next = _table[h%_nbuckets];
    e->key = new char[strlen(key)+1];
    e->hash = h;
    strcpy(e->key, key); /* Flawfinder: ignore */
    e->obj = obj;
    _table[h%_nbuckets] = e;
    return e;
}

void*
Dict::remove(UINT32 hashId)
{
    if (!hashId)
	return 0;

    Dict_entry* e, **ep;
    int h_index = hashId%_nbuckets;
    for (ep = &_table[hashId%_nbuckets]; (e = *ep) != 0; ep = &e->next)
    {
	if (hashId != e->hash)
	    continue;
	void* obj = e->obj;
	*ep = e->next;
	delete[] e->key;
	delete e;
	--_count;
	return obj;
    }
    return 0;       
}

Dict_entry*
Dict::find(UINT32 hashId)
{
    if (!hashId)
	return 0;
    for (Dict_entry* e = _table[hashId%_nbuckets]; e != 0; e = e->next)
	if (hashId == e->hash)
	    return e;
    return 0;
}
#endif

