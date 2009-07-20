/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: regdb_dict.cpp,v 1.3 2009/05/30 19:09:56 atin Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "hxassert.h"
#include "debug.h"

#include "regdb_misc.h"
#include "watchlst.h"
#include "regdb_dict.h"
#include "regprop.h"
#include "hxstrutl.h"


// Chris Torek's hash function
UINT32
serv_hash_torek(const char* key)
{
    UINT32 h = 0x31fe;

    while (*key)
    {
	h = h * 33 + *key++;
    }
    return h;
}

void
ServRegDB_dict::init()
{
    HX_ASSERT(0);
}

void
ServRegDB_dict::strtolower(char* str)
{
    for (register int i=0; str[i]; ++i)
    {
        if (str[i] >= 'A' && str[i] <= 'Z')
            str[i] = str[i] - 'A' + 'a';
    }
}
			 
ServRegDB_dict::ServRegDB_dict()
    : _compare(strcasecmp)
    , _hash(serv_hash_torek)
    , _size(16)
    , _count(0)
    , _owner_node(0)
{
    _table = new ServRegDB_node*[_size];
    memset(_table, 0, HX_SAFESIZE_T(sizeof(void *) * _size));

    DPRINTF(D_REGISTRY, ("ServRegDB_dict(this(%p)) -- "
            "ServRegDB_node(parent(%p))\n",
	    this, _owner_node));
}

ServRegDB_dict::ServRegDB_dict(ServRegDB_node* parent,
                               Keytype nbuckets,
	                       Keytype (*hash)(const char*), 
                               int (*compare)(const char*, const char*))
    : _compare(compare)
    , _hash(hash)
    , _size(nbuckets)
    , _count(0)
    , _owner_node(parent)
{
    _table = new ServRegDB_node*[_size];
    memset(_table, 0, HX_SAFESIZE_T(sizeof(void *) * _size));

    DPRINTF(D_REGISTRY, ("ServRegDB_dict(this(%p)) -- "
            "ServRegDB_node(parent(%p))\n",
	    this, _owner_node));
}

ServRegDB_dict::~ServRegDB_dict()
{
    for (Keytype i = 0; i < _size; i++)
    {
	ServRegDB_node* nexte;
	for (ServRegDB_node* e = _table[i]; e != 0; e = nexte)
	{
	    nexte = e->next;
	    delete e->obj;
	    delete e;
	}
    }
    delete[] _table;
    _size = 0;
    _count = 0;
    _owner_node = 0;
}

Keytype
ServRegDB_dict::del(const char * key)
{
    ServRegDB_node* e, **ep;
    char* k = (char *)key;
    strtolower(k);
    Keytype h = _hash(k);
    for (ep = &_table[h%_size]; (e = *ep) != 0; ep = &e->next)
    {
	if (h != e->hash)
	    continue;

	*ep = e->next;
	delete e->obj;
	delete e;
	--_count;
	return h;
    }
    return 0;       
}

Keytype
ServRegDB_dict::del(ServRegDB_node* node)
{
    ServRegDB_node* e, **ep;
    Keytype h = node->hash;
    for (ep = &_table[h%_size]; (e = *ep) != 0; ep = &e->next)
    {
	if (node != e)
	    continue;

	*ep = e->next;
	delete e->obj;
	delete e;
	--_count;
	return h;
    }
    return 0;       
}

ServRegDB_node*
ServRegDB_dict::next(ServRegDB_node* e)
{
    if (e->next)
	return e->next;
    Keytype h = e->hash % _size;
    for (Keytype i = h+1; i < _size; i++)
	if (_table[i] != 0)
	    return _table[i];
    return 0;
}

ServRegDB_node *    	
ServRegDB_dict::add(char* key_str, ServRegProperty * new_p)
{
    strtolower(key_str);
    Keytype h = _hash(key_str);
    ServRegDB_node* e;
    ServRegDB_node* nexte;
    _count++;

    // Grow the table if 66% full
    Keytype nb = _count * 3;
    if (nb > _size * 2)
    {
	ServRegDB_node** tab = new ServRegDB_node*[nb];
	Keytype i;
	memset(tab, 0, HX_SAFESIZE_T(sizeof(void *) * nb));

	for (i = 0; i < _size; i++)
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
	_size = nb;
    }
    e =  new ServRegDB_node();
    if (e)
    {
	e->next = _table[h%_size];
	e->hash = h;
	e->obj = new_p;
	e->owner_db = this;
	_table[h%_size] = e;
	return e;
    }
    else
	return 0;
}

ServRegDB_node *  	
ServRegDB_dict::find(char* key_str)
{
    char* k = key_str;
    strtolower(k);
    Keytype h = _hash(k);
    for (ServRegDB_node* e = _table[h%_size]; e != 0; e = e->next)
	if (strcasecmp(k, e->get_data()->get_key_str()) == 0)
	    return e;
    return 0;
}
